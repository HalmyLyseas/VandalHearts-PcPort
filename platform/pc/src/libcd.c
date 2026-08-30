/* PC backend for PsyQ libcd: file loads, CdControl, XA streaming and STR movie demux over a raw
 * 2352-byte/sector disc image. Transfers run synchronously; only completion is paced to hardware.
 * See docs/pc-port/subsystems/cd-xa.md. */
#include <SDL2/SDL.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PsyQ/libcd.h"
#include "pc_lang.h"
#include "pc_platform.h"
#include "pc_movie_subs.h"
#include "pc_xa.h"

#define SECTOR_RAW_SIZE 2352
#define SECTOR_DATA_OFFSET 24
#define SECTOR_DATA_SIZE 2048
#define MSF_PREGAP_SECTORS 150 /* Red Book: LBA 0 == MSF 00:02:00 */

/* Private LCG for CalcSeekTimeMs()'s jitter term. Seek jitter is mechanical variance that never
 * touches the game's software RNG, so it must not draw from the shared rand() (one extra draw per
 * CdRead desyncs the LCG from hardware). A fixed seed keeps debug runs repeatable. */
static unsigned int s_seekJitterSeed = 12345U;

static unsigned int SeekJitterRand(void) {
    s_seekJitterSeed = s_seekJitterSeed * 0x41c64e6dU + 0x3039U;
    return (s_seekJitterSeed >> 16) & 0x7fffU;
}

/* CD-ROM timing constants ported from octoshock's cdc.cpp (PS_CDC::CalcSeekTime), converted from
 * 33868800 Hz clock units to milliseconds. See cd-xa.md, "The seek/transfer model". */
#define CDC_CLOCK_HZ 33868800.0
#define SECTORS_PER_SEC_1X 75.0
#define SEEK_BASE_DIVISOR (72.0 * 60.0 * 75.0) /* octoshock's crude "72x" coarse seek-speed assumption */
#define SEEK_FLOOR_MS (20000.0 / (CDC_CLOCK_HZ / 1000.0))
#define SEEK_BIG_JUMP_SECTORS 2250   /* >= this many sectors of travel: +300ms */
#define SEEK_BIG_JUMP_BONUS_MS 300.0
#define SEEK_SMALL_JUMP_MIN_SECTORS 3   /* [3,12) sectors of travel: +4 sector-times */
#define SEEK_SMALL_JUMP_MAX_SECTORS 12
#define SEEK_JITTER_MAX_CYCLES 25000.0 /* up to ~0.74ms of jitter, matching octoshock's PSX_GetRandU32(0,25000) */
#define MOTOR_STARTUP_PENALTY_MS 1000.0 /* one-time disc-spinup cost, this process's first CdRead only */

/* MDEC FMV video state, shared by CdRead2 and MovieRenderFrame below. */
static void MovieRenderFrame(int frameNo);
static int  s_movieActive    = 0;
static int s_movieBaseLBA   = 0;
static int s_movieScanLBA   = 0;   /* forward demux cursor (frames are stored in order) */
static int  s_movieScanFrame = 0;   /* highest frame the cursor has passed */

/* HD FMV: a movie with an HD replacement (hdpacks/videos/<baseLBA hex>.mp4) shows that picture
 * instead of the STR video. The STR is still read for XA audio + frame timing, so sync is untouched. */
extern int  PC_HdVideoOpen(const char *path);
extern const unsigned char *PC_HdVideoFrame(int frameIdx, int *w, int *h);
extern void PC_HdVideoClose(void);
extern const char *PC_HdPackVideosDir(void);
extern void PC_GpuSetMovieOverlayRGB(const unsigned char *rgb, int w, int h);
static int s_movieHd = 0;
static int s_movieHdLBA = -1;
static void MovieHdClose(void) { if (s_movieHd) { PC_HdVideoClose(); s_movieHd = 0; } s_movieHdLBA = -1; }
static void MovieHdTryOpen(int baseLBA) {
    const char *dir;
    char path[1100];
    if (s_movieHd && s_movieHdLBA == baseLBA) return;   /* the stream-start block re-runs several times per
                                                         * movie -> open the decoder ONCE, not each call */
    MovieHdClose();
    dir = PC_HdPackVideosDir();
    if (!dir) return;
    snprintf(path, sizeof(path), "%s/%x.mp4", dir, baseLBA);
    s_movieHd = PC_HdVideoOpen(path);
    s_movieHdLBA = s_movieHd ? baseLBA : -1;
    if (PC_Verbose()) fprintf(stderr, "[HDvideo] movie baseLBA=0x%x -> %s\n", baseLBA, s_movieHd ? "HD" : "native MDEC");
}
static unsigned short s_movieFb[320 * 240];
static unsigned char  s_movieBs[32 * 1024];   /* one frame's BS (<= 9 sectors * 0x7E0 ~= 18KB) */

static FILE *s_disc = NULL;
static int s_headLBA = 0;   /* where the simulated laser head physically is */
static int s_targetLBA = 0; /* where CdlSetloc last pointed it */
static unsigned char s_mode = 0;

/* ---- XA-ADPCM streaming (intro-movie audio + streamed spell SFX), see pc_xa.c ---- */
static int  s_xaFile = -1, s_xaChan = -1;  /* active CdlSetfilter selection    */
static int  s_xaStreaming = 0;
static int s_xaBaseLBA = -1;              /* current track's start LBA        */
static int s_xaCursorLBA = 0;             /* next sector to feed the decoder  */
static int  s_xaMatchedYet = 0;            /* got >=1 matching sector this track (see PC_CdXaUpdate) */
static int s_lastReadResult = 0; /* 0 = idle/complete, -1 = error */
static int s_motorStarted = 0;   /* set after the first-ever CdRead this process */
static Uint32 s_pendingReadUntilMs = 0; /* 0 = no read pending; else SDL_GetTicks() target */

/* Boot-grace fast loads: from process start until the first movie stream begins (one-way latch at
 * s_movieActive=1) the CD delay model runs 16x scaled; VH_FAST_BOOT=0 disables it.
 * See cd-xa.md, "Boot-grace fast loads". */
static int s_bootGrace = -1;   /* -1 = env unresolved, 1 = active, 0 = over */
static int BootGraceActive(void) {
    if (s_bootGrace < 0) {
        const char *e = getenv("VH_FAST_BOOT");
        s_bootGrace = (e && e[0] == '0') ? 0 : 1;
    }
    return s_bootGrace == 1;
}
static double s_pendingPerSectorMs = 0.0; /* for CdReadSync()'s remaining-sector estimate */

/* Ported from octoshock's PS_CDC::CalcSeekTime() in milliseconds instead of cycles; no motor-off or
 * pause bonuses beyond the one-time startup cost the caller adds. */
static double CalcSeekTimeMs(int fromLBA, int toLBA) {
    int absDiff = abs(fromLBA - toLBA);
    int speed = (s_mode & CdlModeSpeed) ? 2 : 1;
    double ms;

    ms = (double)absDiff * 1000.0 / SEEK_BASE_DIVISOR;
    if (ms < SEEK_FLOOR_MS) {
        ms = SEEK_FLOOR_MS;
    }

    if (absDiff >= SEEK_BIG_JUMP_SECTORS) {
        ms += SEEK_BIG_JUMP_BONUS_MS;
    } else if (absDiff >= SEEK_SMALL_JUMP_MIN_SECTORS && absDiff < SEEK_SMALL_JUMP_MAX_SECTORS) {
        ms += (1000.0 / (SECTORS_PER_SEC_1X * speed)) * 4.0;
    }

    ms += (double)(SeekJitterRand() % (int)SEEK_JITTER_MAX_CYCLES) / (CDC_CLOCK_HZ / 1000.0);
    return ms;
}

static unsigned char toBCD(int v) {
    return (unsigned char)(((v / 10) << 4) | (v % 10));
}

static int fromBCD(unsigned char v) {
    return ((v >> 4) * 10) + (v & 0x0f);
}

/* Wrong-disc guard: each region's boot exe starts with the "PS-X EXE" magic at a fixed LBA. The
 * probe costs one sector read and passes for any genuine dump of that region; it is a signature
 * check, not an MD5, so valid alternate dumps are accepted. See cd-xa.md, "Region boot signatures". */
#define SLUS_BOOT_LBA 23      /* US/Asia master */
#define SLPM_BOOT_LBA 15200   /* Japan master (SLPM_860.07; 283,860-sector image) */

/* "PS-X EXE" boot-exe magic probe at a given LBA (raw sector, data at +24 as everywhere). */
static int BootSigAtLba(long bootLba) {
    unsigned char sec[SECTOR_RAW_SIZE];
    if (!s_disc) return 0;
    if (fseek(s_disc, bootLba * SECTOR_RAW_SIZE, SEEK_SET) != 0) return 0;
    if (fread(sec, 1, SECTOR_RAW_SIZE, s_disc) != (size_t)SECTOR_RAW_SIZE) return 0;
    return memcmp(sec + SECTOR_DATA_OFFSET, "PS-X EXE", 8) == 0;
}

/* Public: does the currently-mounted image carry THIS REGION's boot signature at its boot LBA?
 * 1 = yes, 0 = no / no disc mounted / read failed. The region-mismatch case (a valid Vandal
 * Hearts disc of the OTHER region) is diagnosed by the caller via PC_CdDiscRelease(). */
int PC_CdDiscSignatureOk(void) {
    return BootSigAtLba(VH_REGION_BOOT_LBA);
}

/* The memory-card save id embedded in each boot exe is the canonical release tag ("BASLUS-00447VH"
 * USA / "BISCPS-45183VH" Asia at VRAM 0x800f5551; "BISLPM-86007VH" at 0x800f76a9). Each sits at a
 * fixed raw-image offset (contiguous ISO sectors, no boundary crossing); both layouts are probed. */
#define VH_EXE_LOAD_VRAM 0x80010000
#define VH_EXE_HDR_SIZE  0x800

static int CardIdAt(long bootLba, unsigned long cardVram, char id[14]) {
    unsigned long imgOff = (cardVram - VH_EXE_LOAD_VRAM) + VH_EXE_HDR_SIZE;
    long lba = bootLba + (long)(imgOff / SECTOR_DATA_SIZE);
    long raw = lba * SECTOR_RAW_SIZE + SECTOR_DATA_OFFSET + (long)(imgOff % SECTOR_DATA_SIZE);
    if (fseek(s_disc, raw, SEEK_SET) != 0) return 0;
    return fread(id, 1, 14, s_disc) == 14;
}

PC_DiscRelease PC_CdDiscRelease(void) {
    char id[14];
    if (!s_disc) return VH_DISC_UNKNOWN;
    if (BootSigAtLba(SLUS_BOOT_LBA) && CardIdAt(SLUS_BOOT_LBA, 0x800f5551, id)) {
        if (memcmp(id, "BASLUS-00447VH", sizeof id) == 0) return VH_DISC_USA;
        if (memcmp(id, "BISCPS-45183VH", sizeof id) == 0) return VH_DISC_ASIA;
    }
    if (BootSigAtLba(SLPM_BOOT_LBA) && CardIdAt(SLPM_BOOT_LBA, 0x800f76a9, id)) {
        if (memcmp(id, "BISLPM-86007VH", sizeof id) == 0) return VH_DISC_JAPAN;
    }
    return VH_DISC_UNKNOWN;
}

static char s_discPath[1024];   /* kept for the corruption guards' error messages */

int PC_CdMount(const char *discImagePath) {
    if (s_disc) {
        fclose(s_disc);
    }
    s_disc = fopen(discImagePath, "rb");
    snprintf(s_discPath, sizeof(s_discPath), "%s", discImagePath);
    s_headLBA = 0;
    s_targetLBA = 0;
    s_motorStarted = 0;
    s_pendingReadUntilMs = 0;
    return s_disc != NULL;   /* whether the file opened; content is validated via PC_CdDiscSignatureOk */
}

/* The mounted image's size in bytes (-1 if none/unknown) -- the bootstrap's truncation gate
 * (a raw .bin must be a whole number of 2352-byte sectors). */
long long PC_CdImageBytes(void) {
    long long sz;
    if (!s_disc) return -1;
    if (fseek(s_disc, 0, SEEK_END) != 0) return -1;
    sz = (long long)ftell(s_disc);
    fseek(s_disc, 0, SEEK_SET);
    return sz;
}

/* Sector-integrity guards. Every raw sector starts with a 12-byte sync pattern (00 FF x10 00) and
 * its own BCD MSF address, so a 15-byte compare proves the sector is intact and the one sought.
 * A mismatch or EOF on a game-file/movie read is fatal (damaged image); see cd-xa.md. */
static void CdFatalCorrupt(int lba, const char *why) {
    char body[512];
    snprintf(body, sizeof(body),
             "The disc image is DAMAGED: sector %d %s.\n\n"
             "This usually means a bad or incomplete copy (the game would otherwise hang here).\n"
             "Re-copy or re-dump your Vandal Hearts (USA) disc, then verify the file: its size\n"
             "must be an exact multiple of 2352 bytes.", lba, why);
    PC_FatalDiscError("Vandal Hearts - disc image is corrupted", body, s_discPath);
}

static const unsigned char CD_SYNC[12] = { 0x00,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0xFF,0x00 };

static void CdCheckRawSector(const unsigned char *raw, int lba) {
    if (memcmp(raw, CD_SYNC, sizeof(CD_SYNC)) != 0) {
        CdFatalCorrupt(lba, "has no CD sync pattern (zeroed or garbage data)");
    }
    {   /* the sector's own BCD MSF address must be the one we sought to (catches shifted/spliced images) */
        int frames = lba + MSF_PREGAP_SECTORS;
        if (raw[12] != toBCD(frames / (60 * 75)) ||
            raw[13] != toBCD((frames / 75) % 60) ||
            raw[14] != toBCD(frames % 75)) {
            CdFatalCorrupt(lba, "carries the wrong sector address (image shifted or spliced)");
        }
    }
}

int CdInit(void) {
    return s_disc != NULL;
}

CdlLOC *CdIntToPos(int i, CdlLOC *p) {
    int frames = (int)i + MSF_PREGAP_SECTORS;
    p->minute = toBCD((int)(frames / (60 * 75)));
    p->second = toBCD((int)((frames / 75) % 60));
    p->sector = toBCD((int)(frames % 75));
    p->track = 0;
    return p;
}

static int CdPosToLBA(const CdlLOC *p) {
    int frames = (int)fromBCD(p->minute) * 60 * 75 + (int)fromBCD(p->second) * 75 + fromBCD(p->sector);
    return frames - MSF_PREGAP_SECTORS;
}

int CdControl(u_char com, u_char *param, u_char *result) {
    (void)result;
    switch (com) {
        case CdlNop:
            return 1;
        case CdlSetloc:
            /* Real hardware doesn't seek here -- the seek happens (and its
             * time is spent) as part of the next CdRead, folded into that
             * read's completion delay, matching octoshock's ReadBase(). */
            s_targetLBA = CdPosToLBA((CdlLOC *)param);
            return 1;
        case CdlSetmode:
            s_mode = *param;
            return 1;
        case CdlPause:
            s_lastReadResult = 0;
            s_pendingReadUntilMs = 0;
            /* Soft-pause: the game's XA loop path pauses+replays the same track several times a
             * second, so tearing the stream down here would underrun to silence. Keep the source,
             * queued audio, base LBA and ADPCM history; only stop feeding sectors. */
            s_xaStreaming = 0;
            /* A movie's CdlPause stops decoding frames but leaves the last frame on the overlay
             * (a wait-for-button ending shows it); ClearScreen drops the overlay later. */
            if (s_movieActive) {
                /* Movie pause (end or START-skip), not the XA loop's polling: flush the queued
                 * audio tail, which a soft-pause would only mute and a later serial-volume
                 * restore would replay. The video overlay is dropped separately by ClearScreen. */
                PC_XaReset();
                s_xaBaseLBA = -1;
                s_movieActive = 0;
                MovieHdClose();
                /* Subtitle cues stay open: the held final frame keeps its cover until ClearScreen. */
            }
            return 1;
        case CdlReset:
            s_lastReadResult = 0;
            s_pendingReadUntilMs = 0;
            /* Hard stop (not part of the pause/replay polling loop). */
            s_xaStreaming = 0;
            s_xaBaseLBA = -1;
            PC_XaReset();
            if (s_movieActive) { s_movieActive = 0; PC_GpuSetMovieOverlay(NULL, 0, 0); MovieHdClose(); PC_MovieSubsClose(); }
            return 1;
        case CdlSetfilter:
            /* Which interleaved XA file/channel to play (core/audio.c AudioJob_PrepareXa/PlayXa). */
            if (param) {
                CdlFILTER *f = (CdlFILTER *)param;
                s_xaFile = f->file;
                s_xaChan = f->chan;
            }
            return 1;
        case CdlSeekL:
        case CdlReadN:
            /* core/audio.c's XA path passes the CdlLOC directly to CdlSeekL with no preceding
             * CdlSetloc, so honor it here; NULL param means use the last CdlSetloc (file reads). */
            if (param) s_targetLBA = CdPosToLBA((CdlLOC *)param);
            { static FILE *lg = NULL; static int tried = 0;   /* XA event log (set VH_XA_LOG=1) */
              extern unsigned int SDL_GetTicks(void);
              if (!tried) { tried = 1; if (getenv("VH_XA_LOG")) lg = fopen("vh_xa_log.txt", "w"); }
              if (lg) { fprintf(lg, "t=%6u %s lba=%d base=%d file=%d chan=%d rt=%d %s\n",
                        SDL_GetTicks(), com == CdlSeekL ? "SeekL" : "ReadN", s_targetLBA, s_xaBaseLBA,
                        s_xaFile, s_xaChan, (s_mode & CdlModeRT) ? 1 : 0,
                        (s_mode & CdlModeRT) && s_xaBaseLBA == s_targetLBA ? "(same-track replay)" : "(new/seek)");
                        fflush(lg); } }
            /* In RT mode this begins/continues XA streaming from the seeked LBA. Only (re)start
             * when the track base changes: the game re-issues these while polling. */
            if (s_mode & CdlModeRT) {
                if (s_xaBaseLBA != s_targetLBA) {
                    /* New track (or restart after track-end, which sets base=-1): flush the
                     * old stream + ADPCM history. */
                    PC_XaReset();
                    s_xaBaseLBA = s_targetLBA;
                    s_xaMatchedYet = 0;   /* haven't seen this track's (file,chan) sectors yet */
                }
                if (com == CdlSeekL) {
                    /* Seek: reposition the cursor and stay silent. Hardware emits no XA during a
                     * seek; audio begins at the following CdlReadN, where the game captures
                     * gXaStartTime (~2.7s later). See cd-xa.md, "XA audio must start at CdlReadN". */
                    s_xaCursorLBA = s_targetLBA;
                    s_xaStreaming = 0;
                } else {
                    /* CdlReadN: seek done -> begin/continue streaming from wherever the cursor is. */
                    s_xaStreaming = 1;
                }
            }
            return 1;
        default:
            /* Unrecognized command -- extend as new call sites need it. */
            return 0;
    }
}

/* XA streaming pump, called once per VSync (libetc.c): reads raw sectors at the stream cursor and
 * feeds channel-matching audio sectors to the decoder, flow-controlled by the OpenAL queue depth.
 * See cd-xa.md, "Flow control and end-of-track". */
#define XA_TARGET_BUFFERS 12    /* keep ~0.6s of stereo buffered (FMV: robust vs underrun) */
/* Outside a movie XA carries only short hit-sound clips, and the game mutes the source on its own
 * frame schedule; a small target keeps buffered audio near the playhead so the mute lands on time. */
#define XA_TARGET_BUFFERS_CLIP 4
#define XA_MAX_SECTORS_PER_PUMP 400 /* guard: don't scan the whole disc if starved   */

/* Consecutive non-matching sectors that end a track: well above the ~8-sector interleave gap.
 * Applied only after the first matching sector -- some tracks' (file,chan) data begins hundreds of
 * sectors past the seek LBA inside a shared interleave, and hardware reads through until it matches. */
#define XA_END_MISS_LIMIT 150
/* Bounded search for the first matching sector after a seek: hardware reads through indefinitely,
 * but a bad filter/LBA must not scan the whole disc. ~400 is the largest real lead-in (XA 187). */
#define XA_PREMATCH_SCAN_LIMIT 4500
void PC_CdXaUpdate(void) {
    static int s_miss = 0;               /* consecutive non-matching sectors -> track end        */
    if (s_xaStreaming && s_disc) {
        int target = s_movieActive ? XA_TARGET_BUFFERS : XA_TARGET_BUFFERS_CLIP;
        int guard = 0;
        while (PC_XaQueuedBuffers() < target && guard++ < XA_MAX_SECTORS_PER_PUMP) {
            unsigned char raw[SECTOR_RAW_SIZE];
            int off = s_xaCursorLBA * (int)SECTOR_RAW_SIZE;
            if (fseek(s_disc, off, SEEK_SET) != 0 ||
                fread(raw, 1, SECTOR_RAW_SIZE, s_disc) != (size_t)SECTOR_RAW_SIZE) {
                if (s_movieActive && !s_xaMatchedYet) {
                    /* A movie's XA is its frame clock: a stream entirely past the image's end
                     * would make Movie_SyncFrame wait forever, so a truncated image fails loudly.
                     * Mid-stream EOF stays graceful (legitimate end-of-track). */
                    CdFatalCorrupt(s_xaCursorLBA, "is missing (a movie's audio stream lies past the "
                                                  "image's end -- truncated)");
                }
                s_xaStreaming = 0; s_xaBaseLBA = -1;   /* end of disc -- graceful by design: the XA
                                                        * pump legitimately scans toward track ends */
                PC_XaEndStream();
                break;
            }
            CdCheckRawSector(raw, s_xaCursorLBA);   /* successfully-read sectors must still be intact */
            int queued = PC_XaSubmitSector(raw, s_xaFile, s_xaChan);
            if (queued) s_xaMatchedYet = 1;
            s_miss = queued ? 0 : (s_miss + 1);
            if (s_miss >= (s_xaMatchedYet ? XA_END_MISS_LIMIT : XA_PREMATCH_SCAN_LIMIT)) {
                /* Track ended: stop reading, let queued audio finish, base=-1 so the next seek
                 * restarts cleanly. PC_XaEndStream distinguishes this from an underrun so the
                 * drained source is not replayed (the "afterhit" blip train). */
                s_xaStreaming = 0; s_xaBaseLBA = -1; s_miss = 0;
                PC_XaEndStream();
                break;
            }
            s_xaCursorLBA++;
        }
    }
    PC_XaService();

    /* XA runtime instrumentation (VH_XA_CSV=1): one row per VSync while a clip plays or the stream
     * is active, pairing the game's intent (gXaCurrentID/Duration/StartTime) with backend state
     * (streaming, queued, source state) for diffing against a hardware track-map capture. */
    {
        extern unsigned char gXaCurrentID;
        extern int   gXaDuration, gXaStartTime;
        extern short gXaCurrentVolume;
        extern int PC_XaSourceState(void);
        extern unsigned int SDL_GetTicks(void);
        static FILE *lg = NULL; static int tried = 0; static unsigned frame = 0;
        static unsigned char prevId = 0; static int prevStream = -1;
        if (!tried) { tried = 1; if (getenv("VH_XA_CSV")) lg = fopen("vh_xa_ours.csv", "w");
            if (lg) fprintf(lg, "frame,t_ms,xaId,xaDur,xaStart,curVol,streaming,queued,srcState,file,chan,cursorLBA\n"); }
        frame++;
        int active = (gXaCurrentID != 0) || s_xaStreaming || (prevId != 0) || (prevStream == 1);
        if (lg && active) {
            fprintf(lg, "%u,%u,%u,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                    frame, SDL_GetTicks(), gXaCurrentID, gXaDuration, gXaStartTime, gXaCurrentVolume,
                    s_xaStreaming, PC_XaQueuedBuffers(), PC_XaSourceState(),
                    s_xaFile, s_xaChan, s_xaCursorLBA);
            fflush(lg);
        }
        prevId = gXaCurrentID; prevStream = s_xaStreaming;
    }
}

int CdControlB(u_char com, u_char *param, u_char *result) {
    return CdControl(com, param, result);
}

int CdSync(int mode, u_char *result) {
    (void)mode;
    /* Callers read status bits out of `result` (AudioJob_PlayXa checks CdlStatSeek), so write it:
     * this backend has no background seek state, so idle/no-flags is the correct value. */
    if (result) {
        result[0] = 0;
    }
    return CdlComplete;
}

int CdRead(int sectors, unsigned int *buf, int mode) {
    /* Real CdRead compares its mode argument with the drive-mode shadow and issues CdlSetmode on
     * mismatch; the JP loader relies on it (per-read CdlModeSpeed, no separate Setmode). A zero mode
     * keeps the current mode (US passes 0 after Setmode(0x80)). See cd-xa.md, "The mode argument". */
    if ((mode & 0xff) != 0 && (unsigned char)mode != s_mode) {
        s_mode = (unsigned char)mode;
    }
    if (!s_disc || sectors <= 0) {
        s_lastReadResult = -1;
        s_pendingReadUntilMs = 0;
        return 0;
    }

    /* Hardware returns immediately and streams data in as the drive reads; here the transfer
     * happens now and only completion reporting (CdReadSync) is paced. */
    unsigned char *out = (unsigned char *)buf;
    for (int i = 0; i < sectors; i++) {
        /* Read the whole raw sector so the sync/address guard can prove it intact: a game-file
         * read past EOF or into garbage means a damaged image and otherwise hangs the loader. */
        unsigned char raw[SECTOR_RAW_SIZE];
        int lba = s_targetLBA + i;
        if (fseek(s_disc, (long)lba * SECTOR_RAW_SIZE, SEEK_SET) != 0 ||
            fread(raw, 1, SECTOR_RAW_SIZE, s_disc) != (size_t)SECTOR_RAW_SIZE) {
            CdFatalCorrupt(lba, "is missing (the image ends before data the game needs -- truncated)");
        }
        CdCheckRawSector(raw, lba);
        memcpy(out + (size_t)i * SECTOR_DATA_SIZE, raw + SECTOR_DATA_OFFSET, SECTOR_DATA_SIZE);
    }

    /* Language pack (pc_lang.c): a translated text file is substituted here keyed by LBA. core/cd.c
     * reads each text file whole in one CdRead, so this is indistinguishable from disc content. */
    PC_LangPatchRead(s_targetLBA, sectors, out);

    double seekMs = CalcSeekTimeMs(s_headLBA, s_targetLBA);
    if (!s_motorStarted) {
        seekMs += MOTOR_STARTUP_PENALTY_MS;
        s_motorStarted = 1;
    }
    int speed = (s_mode & CdlModeSpeed) ? 2 : 1;
    s_pendingPerSectorMs = 1000.0 / (SECTORS_PER_SEC_1X * speed);
    /* Boot-grace: until the first movie stream, scale the delay 16x (delays stay nonzero so
     * completion still lands on a later VSync). See cd-xa.md, "Boot-grace fast loads". */
    if (BootGraceActive()) {
        seekMs /= 16.0;
        s_pendingPerSectorMs /= 16.0;
    }
    double startMs = 0.0;
    /* Per-read start overhead: the drive pays a pause->ReadN start cost on every read, and
     * game-side idle since the previous read's completion counts toward it. READ_START_MS is
     * fitted to both regions' BizHawk baselines. See cd-xa.md, "Per-read start overhead". */
    #define READ_START_MS 145.0
    {
        static Uint32 s_prevReadDoneMs = 0;
        Uint32 now = SDL_GetTicks();
        if (s_prevReadDoneMs != 0 && !BootGraceActive()) {   /* boot grace: skip the start charge */
            double idleMs = (double)(now - s_prevReadDoneMs);
            if (idleMs < 0.0) idleMs = 0.0;
            if (idleMs < READ_START_MS) startMs = READ_START_MS - idleMs;
        }
        s_prevReadDoneMs = now + (Uint32)(seekMs + startMs + s_pendingPerSectorMs * sectors);
    }
    s_pendingReadUntilMs = SDL_GetTicks() + (Uint32)(seekMs + startMs + s_pendingPerSectorMs * sectors);

    { /* CD load accounting (VH_CD_LOG=1): per-read sector count / seek / start / transfer plus
       * running totals, for comparing a capture against a hardware baseline. Read-only. */
      static FILE *lg = NULL; static int tried = 0;
      static double cumMs = 0.0; static long cumSectors = 0; static int nreads = 0;
      if (!tried) { tried = 1; if (getenv("VH_CD_LOG")) lg = fopen("vh_cd_log.txt", "w"); }
      if (lg) {
          double xferMs = s_pendingPerSectorMs * sectors;
          nreads++; cumSectors += sectors; cumMs += seekMs + startMs + xferMs;
          /* t = wall-clock ms since startup, so the battle-load window can be isolated from
           * boot/movie reads when aligning against a harness's elapsed-time markers. */
          fprintf(lg, "t=%-8u #%-4d lba=%-7d sectors=%-5d speed=%dx seek=%7.2f start=%6.2f xfer=%8.2f read=%8.2f | "
                      "cum reads=%-4d sectors=%-7ld ms=%9.2f\n",
                  SDL_GetTicks(), nreads, s_targetLBA, sectors, speed, seekMs, startMs, xferMs,
                  seekMs + startMs + xferMs, nreads, cumSectors, cumMs);
          fflush(lg);
      }
    }

    s_headLBA = s_targetLBA + sectors;
    s_lastReadResult = 0;
    return 1;
}

int CdReadSync(int mode, u_char *result) {
    (void)mode;
    (void)result;

    if (s_pendingReadUntilMs == 0) {
        return s_lastReadResult;
    }

    Uint32 now = SDL_GetTicks();
    if ((Sint32)(s_pendingReadUntilMs - now) > 0) {
        /* Still "reading" -- report an approximate sectors-remaining count,
         * matching real hardware's positive-while-busy CdReadSync return
         * (callers only check "> 0", not the exact value). */
        double remainingMs = (double)(s_pendingReadUntilMs - now);
        int remaining = (int)(remainingMs / s_pendingPerSectorMs) + 1;
        return remaining > 0 ? remaining : 1;
    }

    s_pendingReadUntilMs = 0;
    return s_lastReadResult;
}

int CdRead2(int mode) {
    s_mode = (unsigned char)mode;
    /* CdRead2(Stream|RT) is what starts a movie's interleaved XA audio on hardware. Movie_Start
     * seeks (state 2) before this call sets the RT bit (state 6), so the stream must start here
     * from s_targetLBA or the first movie plays silent. The cursor rewinds only on a base change. */
    if (s_mode & CdlModeRT) {
        if (s_xaBaseLBA != s_targetLBA) {
            PC_XaReset();
            s_xaBaseLBA   = s_targetLBA;
            s_xaCursorLBA = s_targetLBA;
        }
        s_xaStreaming = 1;
        /* Only Movie_Start issues CdRead2(Stream|RT), so this also marks a .STR movie's video start:
         * seed the demux cursor at the base LBA and show frame 1 immediately. */
        s_movieActive   = 1;
        s_bootGrace     = 0;   /* first movie stream = boot is over; one-way latch, every later
                                * load pays the hardware-exact model (a START skip can't outrun
                                * this -- the stream starts before the skip input is read) */
        s_movieBaseLBA  = s_targetLBA;
        s_movieScanLBA  = s_targetLBA;
        s_movieScanFrame = 0;
        /* Play the movie's own XA regardless of a stale CdlSetfilter left by spell XA: a movie's
         * sectors carry only its own audio, so match-any is correct (else story movies go silent). */
        s_xaFile = -1;
        s_xaChan = -1;
        MovieHdTryOpen(s_movieBaseLBA);   /* use an HD replacement for this movie if one is installed */
        PC_MovieSubsOpen(s_movieBaseLBA); /* language-pack subtitle cues for this movie, if provided */
        MovieRenderFrame(1);
    }
    return 0;
}

unsigned int CdReadyCallback(void (*func)()) {
    (void)func;
    return 0; /* no async callback delivery -- CdRead already completed by the time it returns */
}

int CdMix(CdlATV *vol) {
    (void)vol;
    return 1;
}

void DecDCTReset(int mode) {
    (void)mode;
}

int DecDCTvlc(unsigned int *bs, unsigned int *buf) {
    (void)bs;
    (void)buf;
    return 0;
}

void DecDCTin(unsigned int *buf, int mode) {
    (void)buf;
    (void)mode;
}

void DecDCTout(unsigned int *buf, int size) {
    /* MDEC decode runs out-of-band (MovieRenderFrame -> pc_mdec.c); zeroing the buffer shows clean
     * black in the movie region instead of stale VRAM garbage that looks like a CLUT bug. */
    if (buf) memset(buf, 0, (size_t)size * sizeof(unsigned int));
}

unsigned int DecDCToutCallback(void (*func)()) {
    (void)func;
    return 0;
}

/* Movie frame pacing: StGetNext always succeeds and only the reported frameCount is paced, one
 * frame per CALLS_PER_MOVIE_FRAME calls (15 fps at the 60 Hz tick; hardware spends 2556 ticks in
 * STATE_MOVIE for the 618-frame intro pair). See cd-xa.md, "Movie frame pacing". */
#define CALLS_PER_MOVIE_FRAME 4

static unsigned int s_movieFrameCounter = 0;
static int s_movieCallsSinceFrame = 0;
static StHEADER s_fakeMovieHeader;
static unsigned int s_fakeMovieSectorData[2];

/* Decode the STR video for frame `frameNo` and hand it to the GPU as a fullscreen overlay: demux
 * the frame's video sectors from the disc image, assemble the BS bitstream, decode in pc_mdec.c. */
static void MovieRenderFrame(int frameNo) {
    if (!s_disc || !s_movieActive || frameNo < 1) return;
    PC_MovieSubsFrame(frameNo);   /* this frame is becoming current (native or HD) */
    if (s_movieHd) {                             /* HD FMV: present the HD frame, skip MDEC */
        int w = 0, h = 0;
        const unsigned char *rgb = PC_HdVideoFrame(frameNo - 1, &w, &h);   /* game frame 1 -> mp4 frame 0 */
        if (rgb) { PC_GpuSetMovieOverlayRGB(rgb, w, h); s_movieScanFrame = frameNo; return; }
        MovieHdClose();                          /* decode ended/failed -> native MDEC for the rest */
    }
    if (frameNo < s_movieScanFrame) {           /* looped/restarted -> rewind cursor */
        s_movieScanLBA = s_movieBaseLBA; s_movieScanFrame = 0;
    }
    int lba = s_movieScanLBA;
    int bsLen = 0, w = 320, h = 240, got = 0, ssize = 9, guard = 0;
    unsigned char sec[SECTOR_RAW_SIZE];
    while (guard++ < 6000) {
        if (fseek(s_disc, lba * (int)SECTOR_RAW_SIZE, SEEK_SET) != 0 ||
            fread(sec, 1, SECTOR_RAW_SIZE, s_disc) != SECTOR_RAW_SIZE) {
            /* A complete dump contains every sector its movies reference; running off the end mid-
             * movie means a truncated image -- fail loudly instead of the frame-sync hanging forever
             * waiting for a frame that can never arrive. */
            CdFatalCorrupt(lba, "is missing mid-movie (the image ends early -- truncated)");
        }
        CdCheckRawSector(sec, lba);
        lba++;
        if (sec[16 + 2] & 0x04) continue;                       /* audio sector -> skip */
        unsigned char *d = sec + SECTOR_DATA_OFFSET;
        if ((d[2] | (d[3] << 8)) != 0x8001) continue;           /* not an MDEC video sector */
        int fn = d[8] | (d[9] << 8) | (d[10] << 16) | (d[11] << 24);
        if (fn < frameNo) continue;                             /* earlier frame -> keep scanning */
        if (fn > frameNo) { lba--; break; }                     /* frame not present -> bail */
        int sofs = d[4] | (d[5] << 8);
        if (sofs == 0) { w = d[0x10] | (d[0x11] << 8); h = d[0x12] | (d[0x13] << 8);
                         ssize = d[6] | (d[7] << 8); bsLen = 0; got = 1; }
        if (got && bsLen + 0x7E0 <= (int)sizeof(s_movieBs)) {
            memcpy(s_movieBs + bsLen, d + 0x20, 0x7E0); bsLen += 0x7E0;
        }
        if (got && sofs >= ssize - 1) break;                    /* last sector of this frame */
    }
    s_movieScanLBA = lba; s_movieScanFrame = frameNo;
    if (got && bsLen >= 8 && w > 0 && h > 0 && w <= 320 && h <= 240) {
        if (PC_MdecDecodeBS(s_movieBs, bsLen, w, h, s_movieFb) == 0)
            PC_GpuSetMovieOverlay(s_movieFb, w, h);
    }
}

void StSetRing(unsigned int *ring_addr, unsigned int ring_size) {
    (void)ring_addr;
    (void)ring_size;
}

void StUnSetRing(void) {}

unsigned int StFreeRing(unsigned int *base) {
    (void)base;
    return 0;
}

void StSetStream(unsigned int mode, unsigned int start_frame, unsigned int end_frame, int (*func1)(), int (*func2)()) {
    (void)mode;
    (void)start_frame;
    (void)end_frame;
    (void)func1;
    (void)func2;
    /* New movie starting -- reset the paced frame counter so frameCount comparisons against
     * this movie's own s_totalFrames_80123268 (set separately by core/cd.c's PlayMovie()) start
     * from zero, not wherever a previous chained movie left off. */
    s_movieFrameCounter = 0;
    s_movieCallsSinceFrame = 0;
}

unsigned int StGetNext(unsigned int **addr, unsigned int **header) {
    /* Always succeeds; only frameCount is paced, so core/cd.c's own Movie_GetNextFrame detects
     * completion against s_totalFrames_80123268. addr[0]/addr[1] must equal the header's
     * dummy1/dummy2 (src/core/cd.c sanity check) or the frame is silently discarded. */
    s_movieCallsSinceFrame++;
    if (s_movieCallsSinceFrame >= CALLS_PER_MOVIE_FRAME) {
        s_movieCallsSinceFrame = 0;
        s_movieFrameCounter++;
        MovieRenderFrame((int)s_movieFrameCounter);   /* decode+show this frame's real video */
    }

    s_fakeMovieHeader.frameCount = s_movieFrameCounter;
    s_fakeMovieHeader.dummy1 = 0;
    s_fakeMovieHeader.dummy2 = 0;
    s_fakeMovieSectorData[0] = 0;
    s_fakeMovieSectorData[1] = 0;

    *addr = s_fakeMovieSectorData;
    *header = (unsigned int *)&s_fakeMovieHeader;
    return 0;
}

void StCdInterrupt(void) {}
