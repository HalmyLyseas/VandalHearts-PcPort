/*
 * PC backend for PsyQ/libcd.h, backed by a raw 2352-byte/sector disc image
 * (the .bin produced by `chdman extractcd` from the user's own CHD).
 *
 * The real PSX CD-ROM API is asynchronous: CdControl/CdRead issue a command
 * and return immediately ("accepted" or not), and the caller polls CdSync/
 * CdReadSync until it completes. This implementation does the actual data
 * transfer synchronously inside CdRead (a local disc image has no real
 * seek/read latency to hide at the I/O level), but gates *when CdReadSync
 * reports completion* behind a simulated wall-clock delay -- see "CD-ROM
 * seek/transfer timing simulation" below. Real hardware's read pacing turns
 * out to matter beyond just "the loading screen takes the right amount of
 * time": several battle-start camera/pan effects run their own interpolation
 * throughout the loading period, so a loading screen that's dramatically
 * shorter than real hardware's leaves those effects far short of where real
 * hardware's equivalent moment finds them once the scene becomes visible
 * (see exchange/12-phase-c-bootstrap.md's Bug 19 writeup) -- this isn't just
 * a cosmetic timing nicety, it's load-bearing for matching real hardware's
 * visible behavior at scene start.
 *
 * Sector addressing: verified against the actual disc (2026-07-10) --
 * gCdFiles[]'s baked-in startingSector values are plain ISO9660 LBAs
 * (confirmed by cross-checking gCdFiles[CDF_SIBAI1_1_DAT] == 0x27e8 against
 * the real ISO9660 directory entry for SIBAI1_1.DAT, LBA 10216 == 0x27e8),
 * and this project's extracted .bin has zero pregap: raw sector N in the
 * file *is* LBA N (file_byte_offset = N * 2352 + 24, confirmed by locating
 * SLUS_004.47's "PS-X EXE" header at exactly sector 23, matching its own
 * ISO9660 LBA). CdIntToPos/CdControl(CdlSetloc) apply the standard Red
 * Book 150-sector (00:02:00) MSF offset symmetrically, so the round trip
 * reproduces the original LBA regardless.
 *
 * CD-ROM seek/transfer timing simulation (2026-07-11): ported from
 * BizHawk's octoshock PS1 core (BizHawk/psx/octoshock/psx/cdc.cpp,
 * PS_CDC::CalcSeekTime(), ~line 1687), the only one of BizHawk's two PS1
 * cores that actually models drive timing (mednadisc's CD layer is pure
 * disc-image file I/O, no timing at all). Octoshock's own model is itself a
 * hand-tuned empirical approximation reverse-engineered from real hardware
 * (its own comments say as much), not a physical seek-curve derivation --
 * ported here as a piecewise-linear distance-based estimate in milliseconds
 * (their cycle-clock unit, 33868800 Hz, divided out) rather than their
 * native CPU-cycle units, since this backend has no cycle-exact clock, just
 * SDL_GetTicks() wall-clock ms (same clock VSync() already paces against).
 * Simplifications versus the source model: no motor-off/re-spin-up penalty
 * beyond a single one-time cost on this process's first ever CdRead (real
 * hardware's per-session disc-insert delay isn't meaningfully re-triggered
 * within one game session the way it can be on real hardware), and no
 * separate "resume from pause" bonus (this game's CD access pattern here is
 * plain sequential file loads, not CD-DA/XA playback pause/resume). Only
 * CdRead()/CdReadSync() (used by cd.c's LoadCdFile/ContinueLoadingCdFile
 * for regular file loads) are covered -- audio.c's ExecuteCdControl-driven
 * CdlSeekL/CdlReadN XA-streaming path is a separate, currently-unexercised
 * command sequence (CdControl() has never recognized those command codes;
 * this predates the timing work and is out of scope for it).
 *
 * Not yet implemented: CdRead2 (streaming/XA mode) and the DecDCT* (MDEC)
 * functions -- FMV/movie playback is deferred as a separate follow-up, see
 * the checkpoint doc. CdMix is a no-op until the Audio subsystem exists.
 */
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

/* Private PRNG for CalcSeekTimeMs()'s jitter term only (2026-07-12, exchange/
 * 20-camera-viewport-coordinates.md's "ROOT CAUSE FOUND" section). This used to call the
 * shared game-logic rand() (platform/pc/src/libkernel.c's byte-exact PS1 BIOS LCG) -- but real
 * hardware's physical seek jitter is actual mechanical/electronic timing variance, and never
 * touches the game's software RNG at all. Sharing that stream here injected an extra draw into
 * it on every single CdRead() (many per boot/menu/loading sequence, all before battle logic
 * ever runs), fully desyncing the LCG from real hardware's sequence -- confirmed empirically via
 * exchange/25-rand-seed-full-watch-log.lua: our build showed 1219 shared-rand() transitions in
 * the first 2000 frames vs. real hardware's 51. This is a separate, private LCG (same algorithm
 * shape, arbitrary fixed seed -- correctness here isn't about matching a real sequence, since
 * there's no real sequence to match; it's cosmetic simulated jitter nobody observes precisely,
 * so a fixed seed just keeps repeated debug runs consistent). */
static unsigned int s_seekJitterSeed = 12345U;

static unsigned int SeekJitterRand(void) {
    s_seekJitterSeed = s_seekJitterSeed * 0x41c64e6dU + 0x3039U;
    return (s_seekJitterSeed >> 16) & 0x7fffU;
}

/* CD-ROM timing constants, ported from octoshock's cdc.cpp (see file header
 * comment) -- all originally in units of a 33868800Hz clock, converted here
 * to plain milliseconds. */
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

/* MDEC FMV video state (used by CdRead2 above the definition; see the big comment at the
 * MovieRenderFrame definition below). */
static void MovieRenderFrame(int frameNo);
static int  s_movieActive    = 0;
static int s_movieBaseLBA   = 0;
static int s_movieScanLBA   = 0;   /* forward demux cursor (frames are stored in order) */
static int  s_movieScanFrame = 0;   /* highest frame the cursor has passed */

/* 1.6 HD FMV: when a movie has an HD replacement (hdpacks/videos/<baseLBA>.mp4), decode THAT instead of
 * the STR video. The game keeps reading the STR for its XA audio + frame timing, so we swap only the
 * picture -- audio and sync are untouched. Keyed by the movie's base LBA (== its start sector). */
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
static double s_pendingPerSectorMs = 0.0; /* for CdReadSync()'s remaining-sector estimate */

/* Ported from octoshock's PS_CDC::CalcSeekTime() (BizHawk/psx/octoshock/
 * psx/cdc.cpp, ~line 1687) -- see the file header comment for the porting
 * notes (ms instead of cycles, no motor-off/pause bonuses beyond a one-time
 * startup cost handled by the caller). */
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

/* Cheap wrong-disc guard (Stage 2.4 QoL): Vandal Hearts (USA)'s boot executable SLUS_004.47 begins
 * with the "PS-X EXE" magic at LBA 23 (see the file-layout note at the top of this file). A raw
 * 2352-byte sector carries its 2048 data bytes at offset 24. Catches the common mistakes -- a
 * different game (its exe won't be at LBA 23), the wrong region, a .cue/.iso/.mp3 chosen by mistake,
 * or a truncated image -- for the price of one sector read. The caller (pc_bootstrap.c) treats a
 * failure as fatal and tells the user to supply the right image, rather than booting into garbage.
 * (Region-EXACT verification would MD5 the boot exe from LBA 23 against 596bb082...; not done here
 * to keep mount instant and avoid rejecting valid alternate dumps -- this magic check passes for any
 * genuine Vandal Hearts (USA) .bin, since the ISO layout fixes SLUS_004.47 at LBA 23.) */
#define SLUS_BOOT_LBA 23

/* Public: does the currently-mounted image carry Vandal Hearts (USA)'s SLUS_004.47 boot signature at
 * LBA 23? 1 = yes, 0 = no / no disc mounted / read failed. */
int PC_CdDiscSignatureOk(void) {
    unsigned char sec[SECTOR_RAW_SIZE];
    long off = (long)SLUS_BOOT_LBA * SECTOR_RAW_SIZE;
    if (!s_disc) return 0;
    if (fseek(s_disc, off, SEEK_SET) != 0) return 0;
    if (fread(sec, 1, SECTOR_RAW_SIZE, s_disc) != (size_t)SECTOR_RAW_SIZE) return 0;
    return memcmp(sec + SECTOR_DATA_OFFSET, "PS-X EXE", 8) == 0;   /* same data offset as CdRead */
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

/* ---- corruption guards 2/3 + 3/3 (post-1.6.1, from a real user report) -------------------------
 * A corrupted .bin used to pass the one-sector LBA-23 signature check and then HANG the game with
 * no output at all: garbage sectors load as garbage (loader state machines spin forever), and reads
 * past a truncated image's end just reported failure the retry loops never escape. Both reproduced
 * with deliberately-corrupted images (zeroed 8-40MB span => hang right after the HD-pack banner;
 * truncation => hang when the intro movie's XA audio never arrives).
 *
 * Every raw 2352-byte sector starts with a fixed 12-byte sync pattern (00 FF x10 00) followed by
 * its own BCD minute/second/frame address -- so each read can prove the sector is intact and IS the
 * sector we asked for, for the price of a 15-byte compare. On mismatch (or a read past EOF on the
 * game-file/movie paths, which a complete dump never does) we stop with a message naming the sector
 * instead of hanging: the image is damaged and only a re-copy/re-dump fixes it. */
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
            /* Soft-pause. The game's XA play/loop path (QueuePlayXa) pauses+replays the SAME
             * track several times a second as it polls/loops; tearing the OpenAL stream down
             * here caused a reset storm -> constant underrun -> silence. Just stop feeding new
             * sectors and keep the source, its ~0.6s of queued audio, the base LBA and the
             * ADPCM history, so the same-track resume (CdlSeekL/CdlReadN below) continues
             * seamlessly. A genuinely int pause simply drains the queue and goes quiet. */
            s_xaStreaming = 0;
            /* Movie_Finish (src/cd.c) issues CdlPause at movie end -> stop decoding new movie
             * frames, but LEAVE the last decoded frame on the overlay so a wait-for-button movie
             * end shows the final image instead of black. The overlay is dropped by ClearScreen
             * (PERMUTER hook in src/movie_state.c) when the game redraws the next scene. */
            if (s_movieActive) {
                /* Pausing a MOVIE (one-shot end or player START-skip) -- not the battle XA-loop's
                 * rapid pause/replay polling (that runs with s_movieActive==0 and must stay a soft
                 * pause to avoid the reset storm). A SKIPPED movie is cut mid-stream with ~1s of
                 * decoded audio still queued; the soft-pause only mutes it via serial-volume=0, so
                 * it becomes audible again when a later scene restores the serial volume ("last note
                 * stuck in the buffer"). Flush the audio queue here so the movie's tail is truly
                 * gone and the next XA user (battle hit sounds) inherits a clean stream. This does
                 * NOT touch the video overlay -- that's dropped separately by ClearScreen. */
                PC_XaReset();
                s_xaBaseLBA = -1;
                s_movieActive = 0;
                MovieHdClose();
                /* Subtitle cues deliberately NOT closed here: the held final frame keeps its
                 * cover until ClearScreen drops the overlay (else burned-in text pops back). */
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
            /* Which interleaved XA file/channel to play (audio.c AudioJob_PrepareXa/PlayXa). */
            if (param) {
                CdlFILTER *f = (CdlFILTER *)param;
                s_xaFile = f->file;
                s_xaChan = f->chan;
            }
            return 1;
        case CdlSeekL:
        case CdlReadN:
            /* Honor a location passed directly in the seek/read (audio.c's XA path does
             * CdControl(CdlSeekL, &gXaCdlLOC) WITHOUT a preceding CdlSetloc). Was ignored ->
             * XA seeked to a stale s_targetLBA. NULL param = use the last CdlSetloc (file reads). */
            if (param) s_targetLBA = CdPosToLBA((CdlLOC *)param);
            { static FILE *lg = NULL; static int tried = 0;   /* XA event log (set VH_XA_LOG=1) */
              extern unsigned int SDL_GetTicks(void);
              if (!tried) { tried = 1; if (getenv("VH_XA_LOG")) lg = fopen("vh_xa_log.txt", "w"); }
              if (lg) { fprintf(lg, "t=%6u %s lba=%d base=%d file=%d chan=%d rt=%d %s\n",
                        SDL_GetTicks(), com == CdlSeekL ? "SeekL" : "ReadN", s_targetLBA, s_xaBaseLBA,
                        s_xaFile, s_xaChan, (s_mode & CdlModeRT) ? 1 : 0,
                        (s_mode & CdlModeRT) && s_xaBaseLBA == s_targetLBA ? "(same-track replay)" : "(new/seek)");
                        fflush(lg); } }
            /* In real-time (RT) mode this begins/continues XA-ADPCM streaming from the
             * seeked LBA. Only (re)start when the track's BASE changes -- the game re-issues
             * these while polling, and resetting mid-stream would drop audio. */
            if (s_mode & CdlModeRT) {
                if (s_xaBaseLBA != s_targetLBA) {
                    /* New track (or restart after track-end, which sets base=-1): flush the
                     * old stream + ADPCM history. */
                    PC_XaReset();
                    s_xaBaseLBA = s_targetLBA;
                    s_xaMatchedYet = 0;   /* haven't seen this track's (file,chan) sectors yet */
                }
                if (com == CdlSeekL) {
                    /* SEEKING: reposition the cursor and DO NOT feed audio yet. On real hardware a
                     * CdlSeekL starts a physical seek during which no XA audio flows; audio only
                     * begins at the following CdlReadN once the seek completes. The game's
                     * AudioJob_PlayXa does SeekL(state5) -> CdSync-wait -> ReadN(state7) ~2.7s later
                     * and captures gXaStartTime there, so the animation is timed to the ReadN.
                     * Previously we started feeding right here at SeekL, so every spell SFX began
                     * ~2.7s EARLY and finished ~2.7s before the impact ("Avalanche ends at apex",
                     * climax shifted earlier) -- confirmed by diffing vh_xa_ours.csv (audio started
                     * frame 6494/SeekL, gXaStartTime frame 6576/ReadN) against the real-HW capture.
                     * Rewinding the cursor here is also what lets a loop restart replay from the
                     * base. (Movies stream via CdRead2, unaffected; PrepareXa seeks with no ReadN so
                     * it now correctly stays silent until the matching PLAY.) */
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

/* XA streaming pump -- called once per VSync (from libetc.c). Reads raw sectors from the .bin
 * at the stream cursor and feeds channel-matching audio sectors to the decoder, flow-controlled
 * by the OpenAL queue depth so we never over-read (drop) or under-feed (underrun). */
#define XA_TARGET_BUFFERS 12    /* keep ~0.6s of stereo buffered (FMV: robust vs underrun) */
/* The game mutes the shared XA source in realtime (SsSetSerialVol) on its own frame schedule, but
 * our OpenAL output lags by the buffer depth -- so a big read-ahead makes that mute land on audio
 * not yet played, cutting clip tails and swallowing hits. For the battle hit-sound clips (battle
 * music is SEQ, not XA -- so outside a movie the XA path carries only these short clips) a small
 * target keeps the buffered audio close to the playhead, minimising that desync. */
#define XA_TARGET_BUFFERS_CLIP 4
#define XA_MAX_SECTORS_PER_PUMP 400 /* guard: don't scan the whole disc if starved   */

/* Stop reading after this many consecutive sectors with no matching-channel audio -> the track's
 * interleaved data has ended (the CD would EOF/loop here). Well above the ~8-sector 1/8 interleave
 * gap so we don't cut a live track. Setting base=-1 lets the game's next (loop/new-track) seek restart.
 * IMPORTANT: only applied AFTER the first matching sector (s_xaMatchedYet). A track whose (file,chan)
 * stream begins deep inside a shared multi-song interleave (e.g. the epilogue ocarina, XA 187: its
 * (9,5) sectors don't start until ~400 sectors past its seek LBA -- everything before is other files'
 * audio) would otherwise hit this limit and give up BEFORE its data begins. Real hardware reads
 * through non-matching sectors until the filter matches; mirror that -- read through until we've seen
 * at least one matching sector, only then treat a int miss run as end-of-track. See bugreport-04. */
#define XA_END_MISS_LIMIT 150
/* Bounded search for the FIRST matching sector after a seek (before streaming has started). Real
 * hardware reads through non-matching sectors indefinitely, but we cap it so a genuinely bad
 * filter/LBA can't scan the whole disc forever. ~400 is the largest real lead-in seen (XA 187);
 * this leaves a wide margin yet ends within ~a dozen frames if nothing ever matches. */
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
                    /* An active movie's XA is its frame clock: if the stream lies entirely past the
                     * image's end (not one sector ever matched), Movie_SyncFrame would busy-wait on
                     * an audio position that can never advance -- the exact silent hang a truncated
                     * image used to produce. A valid dump always contains its movies, so this only
                     * fires on damage. (Mid-stream EOF stays graceful: legitimate end-of-track.) */
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
                /* track's interleaved data ended -- stop reading (let queued audio finish);
                 * base=-1 so the game's next seek (loop or new track) restarts cleanly. Tell pc_xa the
                 * stream ENDED (vs a momentary underrun) so PC_XaService stops re-issuing alSourcePlay
                 * on the drained source -- that replay of the last stale buffer is the "afterhit" blip
                 * train heard after each short spell-hit clip. */
                s_xaStreaming = 0; s_xaBaseLBA = -1; s_miss = 0;
                PC_XaEndStream();
                break;
            }
            s_xaCursorLBA++;
        }
    }
    PC_XaService();

    /* --- XA runtime instrumentation (diff vs the real-hardware 45-xa-audio-track-map CSV) ---
     * One row per VSync while a clip is playing or our stream is active. Lets us line up the game's
     * intent (gXaCurrentID / gXaDuration / gXaStartTime -- identical to hardware, same game code)
     * against what our BACKEND actually does (streaming flag, queued buffers, OpenAL source state).
     * The tell for a "cut short" (e.g. Avalanche): gXaCurrentID still set + gXaDuration not yet
     * elapsed, but srcState/queued have gone to 0 -> our audio stopped early. Columns mirror the
     * hardware CSV's key fields plus ours. */
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
    /* Previously ignored `result` entirely -- callers that read status bits
     * out of it (e.g. AudioJob_PlayXa case 6 checking CdlStatSeek) were
     * reading whatever was already in that memory. Happened to work only
     * because BSS-zeroed static buffers start at 0, which coincidentally
     * matches "idle, not seeking" -- but that was incidental, not
     * guaranteed. Since this backend has no real background seek state (see
     * CdRead's own comment: reads complete synchronously, only *reporting*
     * completion is paced), explicitly reporting idle/no-flags-set is the
     * correct, non-fragile value here, not just a leftover default. */
    if (result) {
        result[0] = 0;
    }
    return CdlComplete;
}

int CdRead(int sectors, unsigned int *buf, int mode) {
    (void)mode;
    if (!s_disc || sectors <= 0) {
        s_lastReadResult = -1;
        s_pendingReadUntilMs = 0;
        return 0;
    }

    /* Real hardware returns immediately here and streams data in as the
     * drive physically reads it; a local file has no such latency at the
     * I/O level, so the actual transfer still happens synchronously right
     * now -- only *reporting completion* (CdReadSync, below) is delayed to
     * match real hardware's pacing. */
    unsigned char *out = (unsigned char *)buf;
    for (int i = 0; i < sectors; i++) {
        /* Read the WHOLE raw sector (not just its 2048 data bytes) so the sync/address guard can
         * prove it intact -- a game-file read past EOF or into garbage means a damaged image, and
         * proceeding used to mean an unexplained hang (the loader retries forever). */
        unsigned char raw[SECTOR_RAW_SIZE];
        int lba = s_targetLBA + i;
        if (fseek(s_disc, (long)lba * SECTOR_RAW_SIZE, SEEK_SET) != 0 ||
            fread(raw, 1, SECTOR_RAW_SIZE, s_disc) != (size_t)SECTOR_RAW_SIZE) {
            CdFatalCorrupt(lba, "is missing (the image ends before data the game needs -- truncated)");
        }
        CdCheckRawSector(raw, lba);
        memcpy(out + (size_t)i * SECTOR_DATA_SIZE, raw + SECTOR_DATA_OFFSET, SECTOR_DATA_SIZE);
    }

    /* Language pack (pc_lang.c): a translated on-disc text file is substituted here, keyed by the
     * read's LBA. cd.c reads a text file whole in one CdRead from gCdFiles[cdf].startingSector --
     * the plain ISO9660 LBA -- so this is indistinguishable from the disc having held those bytes,
     * and the game parses them with its own unmodified LoadText. No-op without a pack. */
    PC_LangPatchRead(s_targetLBA, sectors, out);

    double seekMs = CalcSeekTimeMs(s_headLBA, s_targetLBA);
    if (!s_motorStarted) {
        seekMs += MOTOR_STARTUP_PENALTY_MS;
        s_motorStarted = 1;
    }
    int speed = (s_mode & CdlModeSpeed) ? 2 : 1;
    s_pendingPerSectorMs = 1000.0 / (SECTORS_PER_SEC_1X * speed);
    s_pendingReadUntilMs = SDL_GetTicks() + (Uint32)(seekMs + s_pendingPerSectorMs * sectors);

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
    /* Real MDEC frame decode is still deferred (movies render black), but CdRead2 in
     * Stream|RT mode is also what STARTS a movie's interleaved XA-ADPCM audio on real
     * hardware -- and that we CAN do. Previously this was a pure no-op, so movie audio
     * only ever streamed as a side effect of CdControl(CdlSeekL) seeing RT already set
     * in s_mode. That left the FIRST movie silent: Movie_Start (src/cd.c) seeks (state 2)
     * BEFORE it calls CdRead2(0x1c0) (state 6) that sets the RT bit, so the boot logo's
     * seek ran with RT clear and never began streaming; every later movie inherited the
     * now-stale RT s_mode and did stream. Hence "logo silent, intro plays." Start the
     * stream here from the seeked LBA (s_targetLBA, set by that state-2 CdlSeekL), which
     * fixes the logo without disturbing an already-running stream (cursor only rewinds
     * when the track base actually changes). */
    if (s_mode & CdlModeRT) {
        if (s_xaBaseLBA != s_targetLBA) {
            PC_XaReset();
            s_xaBaseLBA   = s_targetLBA;
            s_xaCursorLBA = s_targetLBA;
        }
        s_xaStreaming = 1;
        /* CdRead2(Stream|RT) is issued only by Movie_Start (src/cd.c) -> this also marks the start
         * of a .STR movie's VIDEO. Seed the demux cursor at the movie base (the just-seeked LBA)
         * and show frame 1 immediately so the first tick isn't black. */
        s_movieActive   = 1;
        s_movieBaseLBA  = s_targetLBA;
        s_movieScanLBA  = s_targetLBA;
        s_movieScanFrame = 0;
        /* Play the movie's own interleaved XA regardless of any stale CdlSetfilter left by prior
         * gameplay (battle/spell XA sets s_xaFile/s_xaChan; a story movie's audio is a different
         * file/chan and would be filtered out -> silent video). A movie's sectors carry only its
         * own audio stream, so match-any is correct here -- this is why the boot logo (filter still
         * -1 at boot) had sound but post-gameplay story movies didn't. */
        s_xaFile = -1;
        s_xaChan = -1;
        MovieHdTryOpen(s_movieBaseLBA);   /* 1.6: use an HD replacement for this movie if one is installed */
        PC_MovieSubsOpen(s_movieBaseLBA); /* langpack F3: subtitle cues for this movie, if provided */
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
    /* Real MDEC video decode is deferred (a real video codec undertaking,
     * out of scope -- see the file header comment). Doing nothing here
     * left the movie's on-screen region showing whatever was already in
     * that VRAM area -- often visible as colorful "static" once real
     * gameplay is reached (see exchange/12-phase-c-bootstrap.md), easy to
     * mistake for a texture/CLUT bug rather than what it actually is: an
     * unimplemented video frame. Zeroing it instead shows a clean black
     * region for the movie's duration -- still not real video, but an
     * honest "nothing rendered here" instead of misleading garbage. */
    if (buf) memset(buf, 0, (size_t)size * sizeof(unsigned int));
}

unsigned int DecDCToutCallback(void (*func)()) {
    (void)func;
    return 0;
}

/* Timing-accurate movie skip (2026-07-12, exchange/20-camera-viewport-coordinates.md's "ROOT
 * CAUSE FOUND" section). Real FMV/MDEC decode is still not implemented (see file header) -- but
 * StGetNext() used to always fail immediately, which skipped past cd.c's *own*, already-correct
 * completion-detection logic (Movie_GetNextFrame(), src/cd.c:1316-1321: `if
 * (sMovieSectorHeader->frameCount >= s_totalFrames_80123268) s_movieFinished_8012326c = 1;`)
 * entirely, so movies "finished" almost instantly regardless of their real length -- confirmed
 * via a dense per-tick trace on both platforms: real hardware spends 2556 ticks in STATE_MOVIE
 * (BizHawk capture, frames 1771-4326) vs. our build's 28. Since rand() gets called once per
 * tick throughout (src/engine.c's UpdateEngine()), that gap alone was enough to fully desync the
 * shared RNG stream before any gameplay logic ever runs, on top of the two RNG bugs fixed
 * alongside this investigation.
 *
 * Rather than reimplement that completion logic, this feeds it a paced sequence of fake
 * frame-header data instead -- no real video decode, just correct timing, so the *same* real
 * decompiled code naturally signals "finished" at roughly the right tick.
 *
 * THREE failed attempts before landing on the correct model -- worth recording all of them,
 * since the eventual fix only makes sense in contrast:
 *   1. A "succeed every Nth call" counter measured only 642 ticks total instead of the targeted
 *      ~2500: cd.c's Movie_GetNextFrame() retries StGetNext() in a tight, un-yielding
 *      `while (... != 0) { if (--tries==0) return NULL; }` loop (tries starts at 0x100000), so a
 *      call-counter gets exhausted within a single real tick's near-instant loop, never actually
 *      gating against real elapsed time.
 *   2. Calling SDL_GetTicks() directly inside this function (mirroring CdReadSync()'s pattern)
 *      caused a real, user-visible multi-second stall/catch-up -- that same up-to-0x100000-
 *      iteration retry loop calls StGetNext() over a million times in a single tick whenever the
 *      target hasn't been reached (nearly every tick), turning into 1M+ SDL_GetTicks() syscalls
 *      *per tick*, repeated every tick.
 *   3. Gating on a real-tick counter (incremented once per VSync() call, avoiding the syscall
 *      storm) fixed the responsiveness problem but reproduced attempt 1's original symptom (~28
 *      ticks total, effectively no pacing at all) via a DIFFERENT, more fundamental mechanism:
 *      Movie_DecodeNextFrame() only retries Movie_GetNextFrame() ONCE per call (`tries = 1`,
 *      not 0x100000 -- that budget belongs to StGetNext's own inner retry, one level down). So
 *      the FIRST time StGetNext() genuinely fails (correctly reporting "not paced yet"),
 *      Movie_DecodeNextFrame() immediately returns -1, and Movie_PlayNextFrame() hits its `if
 *      (Movie_DecodeNextFrame() == 0) {...} return 1;` fallback -- returning 1 unconditionally.
 *      State_Movie()'s case 11 treats ANY nonzero Movie_PlayNextFrame() result as "finished,"
 *      with zero distinction between "genuinely done" and "no new sector this tick, try later."
 *      Deliberately pacing StGetNext() to fail on most ticks was therefore never viable through
 *      this call path, regardless of what it's gated on.
 *
 * Correct model: real hardware's StGetNext() succeeds on essentially every call (sectors arrive
 * at the CD's own transfer rate, ~75-150/sec, far faster than the ~60 calls/sec this gets
 * invoked at) -- video frame rate isn't StGetNext() blocking, it's MULTIPLE sector-arrivals
 * accumulating before the frame counter advances (psx-spx, cdromfileformats.md, "STR Frame
 * Rate": frame rate = sector rate / sectors-per-frame). So StGetNext() here always succeeds
 * immediately (matching Movie_DecodeNextFrame()'s single-try expectation, and avoiding every
 * failure mode above), and only the reported frameCount is paced -- advanced once every
 * CALLS_PER_MOVIE_FRAME successful calls, not gated on real time or call-retry counts at all.
 * Since each successful call already corresponds to one real tick (case 11 calls
 * Movie_PlayNextFrame() once per tick, and it now always succeeds on the first inner try), a
 * plain per-call counter here IS a real-tick counter, safely this time.
 *
 * Pacing rate: 4 calls/frame (15fps at this backend's 60Hz tick rate) is psx-spx's cited
 * standard STR rate, and reproduces the measured 2556-tick span closely: this demo's intro
 * chains two movies via src/movie_state.c's case 100 re-entry logic (logo, frameCt=0x8f=143,
 * then title, frameCt=0x1db=475 -- 618 frames total), and `(2556 - ~20 known per-movie startup
 * delay ticks) / 618 frames ≈ 4.1 ticks/frame ≈ 14.6fps`, matching 15fps well within measurement
 * tolerance. */
#define CALLS_PER_MOVIE_FRAME 4

static unsigned int s_movieFrameCounter = 0;
static int s_movieCallsSinceFrame = 0;
static StHEADER s_fakeMovieHeader;
static unsigned int s_fakeMovieSectorData[2];

/* ---- MDEC movie VIDEO decode (real FMV, 2026-07-15) ------------------------------------------
 * The frame PACING above is faked (StGetNext just advances a counter); this decodes the ACTUAL
 * STR video for the current frame and hands it to the GPU backend as a fullscreen overlay
 * (PC_GpuSetMovieOverlay), so the intro FMVs (Konami logo v3, TITLE_WS v2) actually play. Decode
 * lives in pc_mdec.c (reimplemented from psx-spx, ffmpeg-validated). We read the STR sectors
 * straight from the disc image (same s_disc we stream XA from), demux the video sectors of the
 * wanted frame, and decode the assembled BS bitstream to BGR555. State declared up top (shared
 * with CdRead2). */
static void MovieRenderFrame(int frameNo) {
    if (!s_disc || !s_movieActive || frameNo < 1) return;
    PC_MovieSubsFrame(frameNo);   /* langpack F3: this frame is becoming current (native or HD) */
    if (s_movieHd) {                             /* 1.6 HD FMV: present the HD frame, skip MDEC */
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
     * this movie's own s_totalFrames_80123268 (set separately by cd.c's PlayMovie()) start
     * from zero, not wherever a previous chained movie left off. */
    s_movieFrameCounter = 0;
    s_movieCallsSinceFrame = 0;
}

unsigned int StGetNext(unsigned int **addr, unsigned int **header) {
    /* Always succeeds immediately (see the comment above for why) -- only the reported
     * frameCount is paced, advancing once every CALLS_PER_MOVIE_FRAME calls. cd.c's own
     * Movie_GetNextFrame() then correctly compares that against the real s_totalFrames_80123268
     * and sets s_movieFinished_8012326c itself, no different from a real MDEC decoder reaching
     * the end of a real stream. addr[0]/addr[1] must match the header's dummy1/dummy2 fields
     * exactly (src/cd.c:1311's sanity check) or the frame gets silently discarded without
     * advancing -- keep both zeroed and consistent. */
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
