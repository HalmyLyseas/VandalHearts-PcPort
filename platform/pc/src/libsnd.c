/* PC backend for PsyQ libsnd: VAB bank parsing (public PS1 format, clean-room), SPU-ADPCM decode,
 * the SEQ sequencer and PsyQ's key-on volume law, all driving the software SPU in pc_spu.c.
 * See docs/pc-port/subsystems/spu.md, "PsyQ libsnd and the SEQ sequencer". */
#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PsyQ/libsnd.h"
#include "libsnd_internal.h"
#include "cd_files.h" /* CdFileInfo, gCdFiles -- the VAB body transfer's completion accounting */

/* The VAB body transfer's "whole body received" test must match core/cd.c's own completion
 * accounting (gCdLoader.sectorsRead vs gCdFiles[cdf].sectorCt) or the loader stalls; gVabLoader is
 * mirrored field-for-field. See docs/pc-port/subsystems/spu.md, "VAB body transfer and CD completion accounting". */
extern CdFileInfo gCdFiles[712];
extern struct {
    s32 state;
    s32 vabId;
    s32 headerCdf;
    s32 bodyCdf;
    s32 bodyTransferResult;
} gVabLoader;

#define MAX_VAB 8
#define MAX_PROGRAMS 128
#define TONES_PER_PROGRAM 16
#define MAX_VAG 256
#define MAX_VOICES 32

typedef struct {
    short vag;    /* which decoded VAG sample this tone plays */
    signed char centre; /* center note for pitch = 1.0x */
    signed char shift;  /* fine tune, cents-ish; see PitchRatio() */
    unsigned char vol;
    unsigned char pan;
    unsigned char min, max; /* key range -- which note picks this tone (SEQ note-on) */
    unsigned short adsr1, adsr2; /* PS1 ADSR envelope words (SEQ note shaping) */
    unsigned char mode;   /* VagAtr +1. Bit 2 = per-voice reverb enable (hardware EON). The music
                           * bank's tones are all mode=4, every SFX bank's are mode=0, so music is
                           * wet and SFX are dry (a menu beep gets no reverb tail). */
} Tone;

typedef struct {
    int inUse;
    short numPrograms;
    short numVag;
    /* ProgAtr (VAB program table). PsyQ's key-on path (0x800d74b8) multiplies in ProgAtr.mvol;
     * the music bank sets program 0 (the bass) to 127 and every other program to 85..104, so
     * skipping it leaves the non-bass mix ~2.3 dB hot relative to the bass. */
    unsigned char masterVol;              /* VabHdr +24 mvol -- PsyQ reads it at key-on
                                           * (0x800d6d98). 127 in every VAB on this disc, so a
                                           * no-op here, applied for faithfulness. */
    unsigned char progVol[MAX_PROGRAMS];  /* ProgAtr +1 mvol (0..127) */
    unsigned char progPan[MAX_PROGRAMS];  /* ProgAtr +4 mpan (0..127, 64 = centre) */
    unsigned char progTones[MAX_PROGRAMS];/* ProgAtr +0 nTone -- 0 means the program owns NO tone
                                           * block and every later program shifts down by one */
    Tone tones[MAX_PROGRAMS][TONES_PER_PROGRAM]; /* only [0..numPrograms) populated */
    ALuint vagBuffers[MAX_VAG]; /* 0 = no buffer (unused/dummy VAG) */
    unsigned char vagLoops[MAX_VAG]; /* 1 = sustain-looping sample (hold notes ring) */
    /* Decoded PCM retained for the software SPU, which steps through it directly (the OpenAL
     * buffers above are uploaded but play no part in rendering). */
    short *vagPcm[MAX_VAG];      /* decoded 16-bit PCM (freed in SsVabClose), 0 = none */
    int    vagLen[MAX_VAG];      /* sample count                                       */
    int    vagLoopS[MAX_VAG], vagLoopE[MAX_VAG]; /* sustain loop [S,E), E<=S = none     */
} Vab;

/* Software SPU (pc_spu.c): the sample-accurate voice renderer every key-on goes through. */
extern void PC_SpuKeyOn(int voice, const short *pcm, int len, int loopS, int loopE,
                        unsigned int step, float gainL, float gainR,
                        unsigned short adsr1, unsigned short adsr2, int revOn);

/* PS1 pan (0..127, 64 = centre) -> per-channel gain factors, as in PsyQ's key-on path at
 * 0x800d75c4: the divisor is 63 (magic multiplier 0x04104105 with the >>5 add-form) and the test
 * is `< 64`, so pan 64 is unity on both sides. See docs/pc-port/subsystems/spu.md, "The PsyQ volume law". */
static void PanFactors(int pan, float *l, float *r) {
    if (pan < 0) { pan = 0; } if (pan > 127) { pan = 127; }
    *l = (pan < 64) ? 1.0f : (float)(127 - pan) / 63.0f;
    *r = (pan < 64) ? (float)pan / 63.0f : 1.0f;
}

/* PsyQ's square law, the final key-on volume stage (0x800d6d8c): VolL = L*L/16383, applied after
 * the pan stages so the pan factors are squared too; every dB of attenuation is doubled.
 * See docs/pc-port/subsystems/spu.md, "The PsyQ volume law". */
static float ApplyVolumeLaw(float g) { return g * g; }
extern void PC_SpuSetVoiceDebugId(int voice, int vag, int prog, int note);
extern void PC_SpuKeyOff(int voice);
extern void PC_SpuStopAll(void);
extern void PC_SpuSetMaster(int l127, int r127);
extern void PC_SpuService(void);
extern int  PC_SpuVoiceActive(int voice);
extern int  PC_SpuVoiceReleasing(int voice);
extern void PC_SpuSetReverb(int on, float depth);

/* Master output trim for the software mix, calibrated on the rendered output against the
 * octoshock reference (RMS within 0.01 dB, peak -5.6 vs -5.9 dBFS).
 * See docs/pc-port/subsystems/spu.md, "Master trim". */
static float SpuSeqGain(void) {
    static float g = -1.0f;
    if (g < 0.0f) g = 1.012f;   /* calibrated for the square law */
    return g;
}

/* Debug/QoL: VH_SEQ_MUTE=1 silences only the SEQ MUSIC (note-ons with ownSeq>=0), leaving VAG SFX
 * (ownSeq==-1) and the XA stream audible -- so a capture has SFX without the music masking them. */
static int SeqMuted(void) {
    static int m = -1;
    if (m < 0) { const char *e = getenv("VH_SEQ_MUTE"); m = (e && e[0] && e[0] != '0') ? 1 : 0; }
    return m;
}

#ifndef AL_LOOP_POINTS_SOFT
#define AL_LOOP_POINTS_SOFT 0x2015 /* AL_SOFT_loop_points: loop a sub-range of a buffer */
#endif

/* Voice-pool bookkeeping for the allocator. Actual synthesis (pitch/interp/ADSR/reverb)
 * lives in the software SPU (pc_spu.c); this just tracks which slots are in use and who
 * owns them so note-off / voice-steal target the right note. */
typedef struct {
    int active;
    short vabId, prog; /* for SsVoKeyOff's (vabId, prog) lookup */
    int   ownSeq;      /* owning sequence slot, or -1 (free / SFX) */
    short ownCh, ownNote;
} Voice;

static ALCdevice *s_device = NULL;
static ALCcontext *s_context = NULL;
static Vab s_vabs[MAX_VAB];
static Voice s_voices[MAX_VOICES];
static short s_masterVolL = 0x7f, s_masterVolR = 0x7f;
static const unsigned char *s_vagSizeTablePtr[MAX_VAB];

/* Staging for the chunked VAB body: core/cd.c streams the .VB in fixed 90-sector chunks through
 * one buffer, and its loader only advances once this backend reports -2 ("need more") or done.
 * See docs/pc-port/subsystems/spu.md, "VAB body transfer and CD completion accounting". */
static unsigned char *s_vabBodyStaging[MAX_VAB];
static int s_vabBodyTotal[MAX_VAB];
static int s_vabBodyReceived[MAX_VAB];

/* ---- SPU-ADPCM (VAG) decode: 16-byte blocks, byte0 = (filter<<4)|shift, byte1 = loop flags,
 * 14 bytes of packed 4-bit signed nibbles (28 samples/block). The 5-tap filter pair is the
 * standard PS1 SPU-ADPCM coefficient set (psx-spx). */
static const int s_filterPos[5] = {0, 60, 115, 98, 122};
static const int s_filterNeg[5] = {0, 0, -52, -55, -60};

/* Decodes raw VAG bytes to signed 16-bit PCM; returns the sample count, *outPcm is caller-freed.
 * Block flag bits (psx-spx): 0x04 = loop start, 0x01 = end block, 0x02 = repeat. The sample loops
 * [loop start, end) iff the end block also has the repeat bit; otherwise the loop indices are -1. */
static int DecodeVag(const unsigned char *data, int size, short **outPcm,
                     int *outLoopStart, int *outLoopEnd) {
    int numBlocks = size / 16;
    short *pcm = malloc(sizeof(short) * numBlocks * 28);
    int outIdx = 0;
    int older = 0, old = 0;
    int loopStart = 0, loopEnd = -1;

    for (int b = 0; b < numBlocks; b++) {
        const unsigned char *block = data + b * 16;
        int shift = block[0] & 0x0f;
        int filter = (block[0] >> 4) & 0x0f;
        int flags = block[1];
        if (filter > 4) filter = 4; /* be forgiving of malformed data */
        int f0 = s_filterPos[filter];
        int f1 = s_filterNeg[filter];
        if (flags & 0x04) loopStart = outIdx;          /* loop-start marker */

        for (int n = 0; n < 28; n++) {
            int byte = block[2 + n / 2];
            int nibble = (n & 1) ? (byte >> 4) : (byte & 0x0f);
            int d = (short)(nibble << 12); /* sign-extend 4-bit -> 16-bit */
            int s = (d >> shift) + ((old * f0 + older * f1 + 32) >> 6);
            if (s > 32767) s = 32767;
            if (s < -32768) s = -32768;
            older = old;
            old = s;
            pcm[outIdx++] = (short)s;
        }
        if ((flags & 0x01) && (flags & 0x02)) loopEnd = outIdx; /* end block that repeats -> loops */
    }
    *outPcm = pcm;
    if (outLoopStart) *outLoopStart = (loopEnd >= 0) ? loopStart : -1;
    if (outLoopEnd)   *outLoopEnd = loopEnd;
    return outIdx;
}

int LibSnd_DecodeVagForTest(const unsigned char *data, int size, short **outPcm) {
    return DecodeVag(data, size, outPcm, NULL, NULL);
}

/* ---- VAB header/body parsing ------------------------------------- */

static short ReadU16(const unsigned char *p) { return (short)(p[0] | (p[1] << 8)); }
static int ReadU32(const unsigned char *p) {
    return p[0] | (p[1] << 8) | (p[2] << 16) | (p[3] << 24);
}

short SsVabOpenHeadSticky(unsigned char *vabHead, short vabId, unsigned int dummy) {
    (void)dummy;
    if (vabId < 0 || vabId >= MAX_VAB) return -1;
    if (ReadU32(vabHead) != 0x56414270 && memcmp(vabHead, "pBAV", 4) != 0) {
        /* Not a recognized VAB header -- refuse rather than parse garbage. */
        return -1;
    }

    Vab *vab = &s_vabs[vabId];
    memset(vab, 0, sizeof(*vab));
    vab->numPrograms = ReadU16(vabHead + 18);
    vab->numVag = ReadU16(vabHead + 22);
    vab->masterVol = vabHead[24];         /* VabHdr.mvol -- see the struct comment */
    if (vab->numPrograms > MAX_PROGRAMS) vab->numPrograms = MAX_PROGRAMS;
    if (vab->numVag > MAX_VAG) vab->numVag = MAX_VAG;

    const unsigned char *toneTable = vabHead + 32 + 128 * 16; /* fixed 128-slot program table */

    /* Program -> tone-block mapping: the tone table holds one 16-slot block per program that has
     * tones, so a program with nTone == 0 consumes no block and every later program shifts down;
     * the header's `ps` counts tone-bearing programs. See docs/pc-port/subsystems/spu.md, "VAB tone-block packing". */
    int block[MAX_PROGRAMS];
    int nBlocks = 0, progsWithTones = 0, highestProg = 0;
    for (int p = 0; p < MAX_PROGRAMS; p++) {
        const unsigned char *pa = vabHead + 32 + p * 16;
        block[p] = nBlocks;                 /* = number of preceding tone-bearing programs */
        vab->progVol[p] = pa[1];
        vab->progPan[p] = pa[4];
        vab->progTones[p] = pa[0];
        if (pa[0] > 0 && progsWithTones < vab->numPrograms) {
            nBlocks++; progsWithTones++; highestProg = p;
        }
    }
    vab->numPrograms = highestProg + 1;      /* highest USABLE program index, not the count */

    for (int p = 0; p < vab->numPrograms; p++) {
        for (int t = 0; t < TONES_PER_PROGRAM; t++) {
            const unsigned char *te = toneTable + (block[p] * TONES_PER_PROGRAM + t) * 32;
            vab->tones[p][t].centre = (signed char)te[4];
            vab->tones[p][t].shift = (signed char)te[5];
            vab->tones[p][t].mode = te[1];
            vab->tones[p][t].vol = te[2];
            vab->tones[p][t].pan = te[3];
            vab->tones[p][t].min = te[6];  /* key-range low  */
            vab->tones[p][t].max = te[7];  /* key-range high */
            vab->tones[p][t].adsr1 = ReadU16(te + 16);
            vab->tones[p][t].adsr2 = ReadU16(te + 18);
            vab->tones[p][t].vag = ReadU16(te + 22);
        }
    }

    /* VAG size table follows the tone table; stash the offset -- the body
     * transfer (SsVabTransBodyPartly) needs it to compute per-VAG offsets. */
    vab->inUse = 1;
    { extern short g_seqLastVabOpened; g_seqLastVabOpened = vabId; } /* SEQ binds to it (see sequencer) */
    /* nBlocks, not numPrograms: the tone table holds one block per tone-bearing program, and
     * numPrograms is the highest usable program index. Mixing them up shifts every VAG offset. */
    s_vagSizeTablePtr[vabId] = toneTable + nBlocks * TONES_PER_PROGRAM * 32;

    /* The body total to wait for is core/cd.c's own completion count (sectorCt * 2048), not the
     * sum of the VAG size table: the two can differ, and a smaller total stalls gCdLoader for good.
     * See docs/pc-port/subsystems/spu.md, "VAB body transfer and CD completion accounting". */
    {
        int total = (int)gCdFiles[gVabLoader.bodyCdf].sectorCt * 2048;
        free(s_vabBodyStaging[vabId]);
        s_vabBodyStaging[vabId] = malloc((size_t)total);
        s_vabBodyTotal[vabId] = total;
        s_vabBodyReceived[vabId] = 0;
    }

    return vabId;
}

short SsVabTransBodyPartly(unsigned char *vabBody, unsigned int size, short vabId) {
    if (vabId < 0 || vabId >= MAX_VAB || !s_vabs[vabId].inUse) return -1;
    Vab *vab = &s_vabs[vabId];
    const unsigned char *sizeTable = s_vagSizeTablePtr[vabId];

    if (!s_context) return 1; /* SsInit() not called -- nothing to upload to */

    /* Accumulate this chunk. `size` is the caller's fixed maximum chunk size (90*2048), not the
     * bytes remaining, so clamp to the tracked total or the last chunk reads past the body end. */
    {
        int remaining = s_vabBodyTotal[vabId] - s_vabBodyReceived[vabId];
        int take = (int)size < remaining ? (int)size : remaining;
        if (take > 0 && s_vabBodyStaging[vabId]) {
            memcpy(s_vabBodyStaging[vabId] + s_vabBodyReceived[vabId], vabBody, (size_t)take);
            s_vabBodyReceived[vabId] += take;
        }
    }

    if (s_vabBodyReceived[vabId] < s_vabBodyTotal[vabId]) {
        return -2; /* incomplete -- caller must send another chunk */
    }

    /* Full body received -- decode every VAG from the assembled buffer. */
    int offset = 0;
    for (int v = 0; v <= vab->numVag && v < MAX_VAG; v++) {
        /* VAG size table entries are in 8-byte SPU units, not bytes (sum*8 == the .VB body size,
         * and every entry*8 is a whole number of 16-byte ADPCM blocks). Reading them as bytes
         * decodes 1/8 of each sample and reads every later VAG from an 8x-too-small offset. */
        int vagSize = (ReadU16(sizeTable + v * 2) & 0xffff) * 8;
        if (v > 0 && vagSize > 0 && s_vabBodyStaging[vabId]) {
            short *pcm;
            int loopStart, loopEnd;
            int n = DecodeVag(s_vabBodyStaging[vabId] + offset, vagSize, &pcm, &loopStart, &loopEnd);
            ALuint buf;
            alGenBuffers(1, &buf);
            alBufferData(buf, AL_FORMAT_MONO16, pcm, n * (int)sizeof(short), 22050);
            if (loopEnd > loopStart && loopStart >= 0) {
                /* sustain loop: hold notes ring by looping [loopStart,loopEnd) (AL_SOFT_loop_points) */
                ALint lp[2] = { loopStart, loopEnd };
                alBufferiv(buf, AL_LOOP_POINTS_SOFT, lp);
                vab->vagLoops[v] = 1;
            }
            vab->vagBuffers[v] = buf;
            /* Retain the decoded PCM + loop for the software SPU (freed in SsVabClose). */
            vab->vagPcm[v]   = pcm;   /* freed in SsVabClose */
            vab->vagLen[v]   = n;
            vab->vagLoopS[v] = loopStart;
            vab->vagLoopE[v] = loopEnd;
            /* VH_SPU_DUMPVAG=1 writes every decoded VAG as vh_vag_<vab>_<n>.wav plus a manifest.
             * This is the PCM the game really plays; offline analysis of the ISO does not give the
             * same body offsets. See docs/pc-port/subsystems/spu.md, "Diagnostics". */
            { static int dump = -1;
              if (dump < 0) { const char *e = getenv("VH_SPU_DUMPVAG"); dump = (e && e[0] == '1'); }
              if (dump && n > 0) {
                  char nm[64]; FILE *w;
                  snprintf(nm, sizeof(nm), "vh_vag_%d_%d.wav", (int)vabId, v);
                  if ((w = fopen(nm, "wb")) != NULL) {
                      unsigned int dl = (unsigned int)n * 2, rl = 36 + dl;
                      unsigned int sr = 44100, br = 44100 * 2; unsigned short one = 1, bs = 2, bits = 16;
                      unsigned int fs = 16; unsigned short fmt = 1;
                      fwrite("RIFF", 1, 4, w); fwrite(&rl, 4, 1, w); fwrite("WAVEfmt ", 1, 8, w);
                      fwrite(&fs, 4, 1, w); fwrite(&fmt, 2, 1, w); fwrite(&one, 2, 1, w);
                      fwrite(&sr, 4, 1, w); fwrite(&br, 4, 1, w); fwrite(&bs, 2, 1, w);
                      fwrite(&bits, 2, 1, w); fwrite("data", 1, 4, w); fwrite(&dl, 4, 1, w);
                      fwrite(pcm, 2, (size_t)n, w); fclose(w);
                  }
                  /* append-only, header once per process -- several VABs load (SFX banks + the
                   * music bank) and each restarts its VAG numbering at 1 */
                  { static int hdr = 0;
                    if ((w = fopen("vh_vag_manifest.csv", hdr ? "a" : "w")) != NULL) {
                        if (!hdr) { fprintf(w, "vab,vag,len,loopS,loopE\n"); hdr = 1; }
                        fprintf(w, "%d,%d,%d,%d,%d\n", (int)vabId, v, n, loopStart, loopEnd);
                        fclose(w);
                    } }
              } }
        }
        offset += vagSize;
    }
    return 1; /* transfer completed */
}

short SsVabTransCompleted(short flag) {
    (void)flag;
    return 1; /* SsVabTransBodyPartly already finished synchronously above */
}

void SsVabClose(short vabId) {
    if (vabId < 0 || vabId >= MAX_VAB) return;
    Vab *vab = &s_vabs[vabId];
    for (int v = 0; v < MAX_VAG; v++) {
        if (vab->vagBuffers[v]) {
            alDeleteBuffers(1, &vab->vagBuffers[v]);
        }
        free(vab->vagPcm[v]);   /* retained decoded PCM for the software SPU */
    }
    memset(vab, 0, sizeof(*vab));
    free(s_vabBodyStaging[vabId]);
    s_vabBodyStaging[vabId] = NULL;
    s_vabBodyTotal[vabId] = 0;
    s_vabBodyReceived[vabId] = 0;
}

/* ---- lifecycle / config -------------------------------------------- */

/* Reverb state -- the game selects SPU_REV_MODE_STUDIO_C via SsUtSetReverbType + SsUtReverbOn;
 * the actual comb/all-pass network is implemented faithfully in the software SPU (pc_spu.c),
 * which reads these two flags each tick via PC_SpuSetReverb. */
static int    s_reverbOn = 0;      /* SsUtReverbOn() called      */
static float  s_reverbDepth = 1.0f;/* SsUtSetReverbDepth, 0..1   */
static FILE  *SeqLog(void);        /* fwd: diagnostic sink (vh_seq_log.txt) */

void SsInit(void) {
    /* Just brings up the OpenAL device/context; the software SPU (pc_spu.c) and XA (pc_xa.c)
     * lazily create their own streaming sources on this shared context. No per-voice OpenAL
     * sources or EFX -- synthesis and reverb are done in software. */
    s_device = alcOpenDevice(NULL);
    if (!s_device) return;
    s_context = alcCreateContext(s_device, NULL);
    alcMakeContextCurrent(s_context);
    memset(s_vabs, 0, sizeof(s_vabs));
    memset(s_voices, 0, sizeof(s_voices));
    for (int i = 0; i < MAX_VOICES; i++) s_voices[i].ownSeq = -1;
}

void SsQuit(void) {
    LibSnd_StopAllVoices();
    if (s_context) {
        alcMakeContextCurrent(NULL);
        alcDestroyContext(s_context);
        s_context = NULL;
    }
    if (s_device) {
        alcCloseDevice(s_device);
        s_device = NULL;
    }
}

void SsStart(void) {}
void SsEnd(void) {}
void SsSetTickMode(int mode) { (void)mode; }
void SsSetTableSize(char *table, short maxVab, short maxSeq) {
    (void)table; (void)maxVab; (void)maxSeq;
}
char SsSetReservedVoice(char n) { return n; }
void SsSetMono(void) {}
void SsSetStereo(void) {}

void SsSetMVol(short lVol, short rVol) {
    s_masterVolL = lVol;
    s_masterVolR = rVol;
    if (s_context) {
        alListenerf(AL_GAIN, ((lVol + rVol) / 2) / 127.0f);
    }
}

void SsSetSerialAttr(char port, char attr, char mode) {
    (void)port; (void)attr; (void)mode; /* serial (CD/XA) routing flags; the XA stream always mixes in here */
}
void SsSetSerialVol(char port, short lVol, short rVol) {
    /* Serial-A carries the CD/XA input on real hardware -> route to the XA stream gain. */
    (void)port;
    { extern void PC_XaSetVolume(int, int); PC_XaSetVolume(lVol, rVol); }
}

void SsUtReverbOn(void) { s_reverbOn = 1; }
short SsUtSetReverbType(short type) { (void)type; return 0; } /* only STUDIO_C used; done in pc_spu.c */
void SsUtSetReverbDepth(short depth1, short depth2) {
    /* PsyQ reverb depth is a 0..127 scale (the game uses ~8..60); mapped to the software SPU's
     * wet mix (PC_SpuSetReverb, pushed each tick). */
    int d = (depth1 + depth2) / 2; if (d < 0) d = 0; if (d > 127) d = 127;
    s_reverbDepth = (float)d / 127.0f;
}

/* ---- voice triggering ------------------------------------------------ */

static float PitchRatio(int centre, int shift, int note, int fine) {
    float semitones = (float)(note - centre) + (float)(fine - shift) / 128.0f;
    return powf(2.0f, semitones / 12.0f); /* standard equal temperament; not PS1's exact table */
}

void LibSnd_StopAllVoices(void) {
    PC_SpuStopAll();
    for (int i = 0; i < MAX_VOICES; i++) {
        s_voices[i].active = 0;
    }
}

/* A VAG loop shorter than this (samples) is treated as a one-shot "hold" tail, not a real sustain
 * loop. The measured hold loops are exactly one ADPCM block (28 samples); genuine sustain loops
 * (spell casting sounds) are 900..8500 samples -- so this threshold cleanly separates them. */
#define SFX_HOLD_LOOP_MAX 128

/* Diagnostic sink for SFX key-on/off: opt-in via VH_SFX_LOG (off for end users). */
static FILE *SfxLog(void) {
    static FILE *f = NULL; static int tried = 0;
    if (!tried) { tried = 1; if (getenv("VH_SFX_LOG") != NULL) f = fopen("vh_sfx_log.txt", "w"); }
    return f;
}

/* Play an SFX voice through the software SPU (same faithful path as SEQ music:
 * per-sample pitch + Gaussian interp + the tone's ADSR + STUDIO_C reverb). */
static void PlayOnVoice(int voiceIdx, short vabId, short prog, short tone, int note, int fine, int voll, int volr) {
    Tone *t;
    int vg;
    if (voiceIdx < 0 || voiceIdx >= MAX_VOICES) return;
    if (vabId < 0 || vabId >= MAX_VAB || !s_vabs[vabId].inUse) return;
    if (prog < 0 || prog >= s_vabs[vabId].numPrograms || tone < 0 || tone >= TONES_PER_PROGRAM) return;
    t = &s_vabs[vabId].tones[prog][tone];
    vg = t->vag;
    if (vg < 0 || vg >= MAX_VAG || !s_vabs[vabId].vagPcm[vg]) return;
    {
        int ls = s_vabs[vabId].vagLoopS[vg], le = s_vabs[vabId].vagLoopE[vg];
        /* SFX ADSRs never fade and most SFX get no key-off, so a one-block hold loop would keep
         * the voice active forever; treat it as one-shot. A genuine sustain loop (spell casting,
         * thousands of samples) rings until the game's SFX_ROLE_RELEASE key-off. */
        int plen = s_vabs[vabId].vagLen[vg];
        /* Truncate at loopS, where the real audio ends: the hold block is a flat +28672 DC parking
         * spot and cutting inside it emits a rectangular pulse (the menu "pop").
         * See docs/pc-port/subsystems/spu.md, "SFX voices and hold loops". */
        if (le - ls < SFX_HOLD_LOOP_MAX) { if (ls > 0) plen = ls; ls = 0; le = 0; }
        /* PsyQ's full key-on chain, shared with the SEQ path: (V/127) x VabHdr.mvol x ProgAtr.mvol
         * x VagAtr.vol, then the square law; SFX skip only the per-channel stage. voll/volr are
         * per-side gains, not averaged. See docs/pc-port/subsystems/spu.md, "SFX key-on volume". */
        float sfxL = (float)voll / 127.0f, sfxR = (float)volr / 127.0f;
        sfxL *= (float)s_vabs[vabId].masterVol / 127.0f;
        sfxR *= (float)s_vabs[vabId].masterVol / 127.0f;
        sfxL *= (float)s_vabs[vabId].progVol[prog] / 127.0f;
        sfxR *= (float)s_vabs[vabId].progVol[prog] / 127.0f;
        sfxL *= (float)t->vol / 127.0f;
        sfxR *= (float)t->vol / 127.0f;
        { float tpL, tpR, ppL, ppR;
          PanFactors(t->pan, &tpL, &tpR);
          PanFactors(s_vabs[vabId].progPan[prog], &ppL, &ppR);
          sfxL *= tpL * ppL; sfxR *= tpR * ppR; }
        PC_SpuKeyOn(voiceIdx, s_vabs[vabId].vagPcm[vg], plen, ls, le,
                    (unsigned int)(PitchRatio(t->centre, t->shift, note, fine) * 4096.0f),
                    ApplyVolumeLaw(sfxL) * SpuSeqGain(),
                    ApplyVolumeLaw(sfxR) * SpuSeqGain(), t->adsr1, t->adsr2,
                    (t->mode & 0x04) != 0);
        /* tag SFX voices too, else the VH_SPU_TRACE rows show their zero-init ids and they look
         * like a phantom "vag 0" instrument */
        PC_SpuSetVoiceDebugId(voiceIdx, vg, prog, note);
        if (SfxLog()) { extern unsigned int SDL_GetTicks(void);
            fprintf(SfxLog(), "t=%6u keyon  v=%d vag=%d loopLen=%d vol=%d\n",
                    SDL_GetTicks(), voiceIdx, vg, le - ls, (voll + volr) / 2); }
    }
    s_voices[voiceIdx].active = 1;
    s_voices[voiceIdx].vabId = vabId;
    s_voices[voiceIdx].prog = prog;
    s_voices[voiceIdx].ownSeq = -1;
}

void SsVoKeyOn(int vabId, int prog, unsigned short pitch, unsigned short vol) {
    /* `pitch` is a direct fixed-point pitch step (4096 == 1.0x); tone 0 of the program. */
    for (int i = 0; i < MAX_VOICES; i++) {
        if (!s_voices[i].active) {
            Tone *t; int vg;
            if (vabId < 0 || vabId >= MAX_VAB || !s_vabs[vabId].inUse) return;
            if (prog < 0 || prog >= s_vabs[vabId].numPrograms) return;
            t = &s_vabs[vabId].tones[prog][0];
            vg = t->vag;
            if (vg < 0 || vg >= MAX_VAG || !s_vabs[vabId].vagPcm[vg]) return;
            {
                int ls = s_vabs[vabId].vagLoopS[vg], le = s_vabs[vabId].vagLoopE[vg];
                int plen2 = s_vabs[vabId].vagLen[vg];
                /* truncate at the hold-loop start -- see PlayOnVoice's note (DC pulse = the pop) */
                if (le - ls < SFX_HOLD_LOOP_MAX) { if (ls > 0) plen2 = ls; ls = 0; le = 0; }
                /* Same key-on chain as PlayOnVoice (VabHdr.mvol x ProgAtr.mvol x VagAtr.vol, no
                 * per-channel stage for SFX), then the square law. */
                float g = (float)vol / 127.0f;
                g *= (float)s_vabs[vabId].masterVol / 127.0f;
                g *= (float)s_vabs[vabId].progVol[prog] / 127.0f;
                g *= (float)t->vol / 127.0f;
                PC_SpuKeyOn(i, s_vabs[vabId].vagPcm[vg], plen2, ls, le,
                            (unsigned int)pitch,
                            ApplyVolumeLaw(g) * SpuSeqGain(),
                            ApplyVolumeLaw(g) * SpuSeqGain(), /* centred */
                            t->adsr1, t->adsr2, (t->mode & 0x04) != 0);
                PC_SpuSetVoiceDebugId(i, vg, (int)prog, -1); /* SsVoKeyOn: raw pitch, no MIDI note */
            }
            s_voices[i].active = 1;
            s_voices[i].vabId = (short)vabId;
            s_voices[i].prog = (short)prog;
            s_voices[i].ownSeq = -1;
            return;
        }
    }
}

void SsVoKeyOff(int vabId, int prog) {
    for (int i = 0; i < MAX_VOICES; i++) {
        if (s_voices[i].active && s_voices[i].vabId == vabId && s_voices[i].prog == prog) {
            PC_SpuKeyOff(i);
            s_voices[i].active = 0;
        }
    }
}

short SsUtKeyOnV(short voice, short vabId, short prog, short tone, short note, short fine, short voll, short volr) {
    PlayOnVoice(voice, vabId, prog, tone, note, fine, voll, volr);
    return voice;
}

short SsUtKeyOffV(short voice) {
    if (voice >= 0 && voice < MAX_VOICES) {
        if (SfxLog()) { extern unsigned int SDL_GetTicks(void);
            fprintf(SfxLog(), "t=%6u keyoff v=%d\n", SDL_GetTicks(), voice); }
        PC_SpuKeyOff(voice);
        s_voices[voice].active = 0;
    }
    return voice;
}

/* ---- sequencer (PS1 SEQ / MIDI-like): parses the "pQES" header + event stream and keys notes
 * on the software SPU at real-time tempo, through the VAB named by SsSeqOpen's second argument.
 * See docs/pc-port/subsystems/spu.md, "PsyQ libsnd and the SEQ sequencer". */

#define MAX_SEQ         4
#define SEQ_CHANNELS    16
#define SEQ_VOICE_FIRST 0    /* SEQ uses voices [0,SEQ_VOICE_COUNT); SsUtKeyOnV SFX use 20-23 */
#define SEQ_VOICE_COUNT 20
#define SEQ_MAX_BYTES   0x40000 /* safety cap on a song blob; real end is the 0x2F meta */

short g_seqLastVabOpened = -1;   /* set by SsVabOpenHeadSticky; the SEQ->VAB binding */

typedef struct {
    int   inUse, playing;
    const unsigned char *base;   /* first event (after 15-byte header)      */
    const unsigned char *pos;    /* next byte to read                       */
    const unsigned char *end;    /* hard bound (base + SEQ_MAX_BYTES)        */
    int   ppqn;                  /* ticks per quarter note                  */
    unsigned int tempo;          /* microseconds per quarter note           */
    short vabId;                 /* instrument bank                         */
    int   volL, volR;            /* SsSeqSetVol (0..127)                    */
    int   loopInfinite, repeatLeft;
    double tickBudget;           /* accumulated musical ticks not yet spent */
    int  ticksToNext;           /* delta before the upcoming event         */
    unsigned char running;       /* MIDI running status                     */
    unsigned char chProg[SEQ_CHANNELS];
    unsigned char chVol[SEQ_CHANNELS];
    unsigned char chPan[SEQ_CHANNELS];  /* CC 0x0A, 0..127 (64 = centre)                */
    signed char   noteVoice[SEQ_CHANNELS][128]; /* voice holding (ch,note), or -1 */
} Seq;

static Seq s_seqs[MAX_SEQ];
static int s_seqVoiceRR = 0;
static int s_seqOns = 0, s_seqOffs = 0;   /* diag: note-on / note-off balance */

/* --- SEQ diagnostic sink: opt-in via VH_SEQ_LOG (off for end users) --- */
static FILE *SeqLog(void) {
    static FILE *f = NULL; static int tried = 0;
    if (!tried) { tried = 1; if (getenv("VH_SEQ_LOG") != NULL) f = fopen("vh_seq_log.txt", "w"); }
    return f;
}
static int s_seqDbgNotes = 0;

/* MIDI variable-length quantity (delta-times / meta lengths). */
static int SeqReadVLQ(const unsigned char **pp, const unsigned char *end) {
    int v = 0; int i;
    for (i = 0; i < 4 && *pp < end; i++) {
        unsigned char b = *(*pp)++;
        v = (v << 7) | (b & 0x7f);
        if (!(b & 0x80)) break;
    }
    return v;
}

/* Allocate a SEQ voice: prefer an idle one, else steal round-robin. */
static int SeqAllocVoice(void) {
    int i;
    for (i = 0; i < SEQ_VOICE_COUNT; i++) {
        int idx = SEQ_VOICE_FIRST + i;
        if (!s_voices[idx].active) return idx;
    }
    { int idx = SEQ_VOICE_FIRST + s_seqVoiceRR;
      s_seqVoiceRR = (s_seqVoiceRR + 1) % SEQ_VOICE_COUNT;
      return idx; }
}

/* Select the tone (and its program) for `note`: a key-range match inside the requested program
 * first, else the nearest key anywhere in the bank (for a multisample tiled across programs).
 * Returns the tone index, owning program in *outProg. See docs/pc-port/subsystems/spu.md, "Tone selection". */
static int SeqPickTone(const Vab *vab, int reqProg, int note, int *outProg) {
    int t, p;
    /* 1) exact range match inside the requested program (standard multi-program VAB) */
    if (reqProg >= 0 && reqProg < vab->numPrograms) {
        for (t = 0; t < TONES_PER_PROGRAM; t++) {
            const Tone *tn = &vab->tones[reqProg][t];
            if (tn->vag > 0 && note >= tn->min && note <= tn->max) { *outProg = reqProg; return t; }
        }
    }
    /* 2) global search: exact key, else the nearest key overall (tiled single multisample) */
    { int bestP = -1, bestT = -1, bestD = 1 << 30;
      for (p = 0; p < vab->numPrograms; p++) {
          for (t = 0; t < TONES_PER_PROGRAM; t++) {
              const Tone *tn = &vab->tones[p][t];
              if (tn->vag <= 0) continue;
              if (note >= tn->min && note <= tn->max) { *outProg = p; return t; }
              int key = tn->min, d = key > note ? key - note : note - key;
              if (d < bestD) { bestD = d; bestP = p; bestT = t; }
          }
      }
      *outProg = bestP; return bestT; }
}

static void SeqNoteOff(Seq *s, int ch, int note) {
    int si = (int)(s - s_seqs);
    int v = s->noteVoice[ch][note];
    if (v >= 0) {
        Voice *vc = &s_voices[v];
        /* only release if this voice still holds THIS note (wasn't stolen) */
        if (vc->ownSeq == si && vc->ownCh == ch && vc->ownNote == note) {
            PC_SpuKeyOff(v);   /* enter ADSR release; the tail rings out in pc_spu */
            s_seqOffs++;
        }
        s->noteVoice[ch][note] = -1;
    }
}

static void SeqNoteOn(Seq *s, int ch, int note, int vel) {
    if (s->vabId < 0 || s->vabId >= MAX_VAB || !s_vabs[s->vabId].inUse) return;
    Vab *vab = &s_vabs[s->vabId];
    int selProg = -1;
    int tone = SeqPickTone(vab, s->chProg[ch], note, &selProg);
    if (tone < 0 || selProg < 0) return;
    SeqNoteOff(s, ch, note);                 /* retrigger: drop any prior instance */
    int voice = SeqAllocVoice();
    /* Stealing an in-use SEQ voice: drop the previous owner's back-reference so that note's
     * later note-off cannot release this reassigned voice. */
    if (s_voices[voice].ownSeq >= 0) {
        Voice *vc = &s_voices[voice];
        Seq *o = &s_seqs[vc->ownSeq];
        if (vc->ownCh >= 0 && vc->ownCh < SEQ_CHANNELS && o->noteVoice[vc->ownCh][vc->ownNote] == voice)
            o->noteVoice[vc->ownCh][vc->ownNote] = -1;
    }
    int gain = (vel * s->chVol[ch]) / 127;   /* velocity x channel volume */
    /* x ProgAtr.mvol, then x VagAtr.vol -- PsyQ's two /127 stages in its order (0x800d7514, then
     * 0x800d7560). progVol keeps the bass (prog 0, mvol 127) above the rest of the bank (85..104). */
    gain = (gain * vab->masterVol) / 127;                 /* VabHdr.mvol  (0x800d6d98) */
    gain = (gain * vab->progVol[selProg]) / 127;
    gain = (gain * vab->tones[selProg][tone].vol) / 127;
    /* Per-voice L/R: three successive pan stages (tone, program, note), each attenuating one side.
     * progPan is 64 (centre) throughout this game's banks. */
    float gL, gR;
    { float tpL, tpR, ppL, ppR, cpL, cpR;
      PanFactors(vab->tones[selProg][tone].pan, &tpL, &tpR);
      PanFactors(vab->progPan[selProg], &ppL, &ppR);
      PanFactors(s->chPan[ch], &cpL, &cpR);
      gL = (float)((gain * s->volL) / 127) / 127.0f * tpL * ppL * cpL;
      gR = (float)((gain * s->volR) / 127) / 127.0f * tpR * ppR * cpR;
      /* PsyQ squares AFTER panning, so the pan factors are squared too. */
      gL = ApplyVolumeLaw(gL); gR = ApplyVolumeLaw(gR);
      /* VH_SPU_SOLOPROG=N plays only program N, VH_SPU_MUTEPROG=N silences it: isolate one
       * instrument by ear (spectral attribution alone misidentifies instruments). */
      { static int solo = -2, mute = -2;
        if (solo == -2) { const char *e = getenv("VH_SPU_SOLOPROG"); solo = e ? atoi(e) : -1;
                          e = getenv("VH_SPU_MUTEPROG");            mute = e ? atoi(e) : -1; }
        if ((solo >= 0 && selProg != solo) || (mute >= 0 && selProg == mute)) gL = gR = 0.0f; } }
    if (s_seqDbgNotes < 96) { FILE *lg = SeqLog(); if (lg) {
        Tone *tn = &vab->tones[selProg][tone];
        fprintf(lg, "[note] vab=%d ch=%d reqProg=%d note=%d vel=%d -> selProg=%d tone=%d vag=%d centre=%d key=%d voice=%d gain=%d\n",
                s->vabId, ch, s->chProg[ch], note, vel, selProg, tone, tn->vag, tn->centre, tn->min, voice, gain);
        fflush(lg); } s_seqDbgNotes++; }
    /* Software SPU: step the decoded VAG directly. VxPitch = unity(0x1000) scaled by the
     * centre-relative semitone ratio -- the faithful pitch. The tone's ADSR + STUDIO_C reverb
     * are applied in pc_spu.c. */
    { Tone *tn = &vab->tones[selProg][tone];
      int vg = tn->vag;
      if (vg >= 0 && vg < MAX_VAG && vab->vagPcm[vg]) {
          unsigned int step = (unsigned int)(PitchRatio(tn->centre, tn->shift, note, 0) * 4096.0f);
          PC_SpuKeyOn(voice, vab->vagPcm[vg], vab->vagLen[vg],
                      vab->vagLoopS[vg], vab->vagLoopE[vg], step,
                      SeqMuted() ? 0.0f : gL * SpuSeqGain(),
                      SeqMuted() ? 0.0f : gR * SpuSeqGain(), tn->adsr1, tn->adsr2,
                      (tn->mode & 0x04) != 0);
          PC_SpuSetVoiceDebugId(voice, vg, selProg, note);
      }
      s_voices[voice].active = 1; }  /* busy: keep the allocator from reusing it */
    s_voices[voice].ownSeq = (int)(s - s_seqs);
    s_voices[voice].ownCh = (short)ch;
    s_voices[voice].ownNote = (short)note;
    s->noteVoice[ch][note] = (signed char)voice;
    s_seqOns++;
}

/* Execute the event at s->pos. Returns 0 on end-of-track / malformed, else 1. */
static int SeqProcessEvent(Seq *s) {
    if (s->pos >= s->end) return 0;
    unsigned char status = *s->pos;
    if (status & 0x80) { s->pos++; s->running = status; }
    else               { status = s->running; }   /* running status */
    int ch = status & 0x0f;
    switch (status & 0xf0) {
    case 0x80: { int n = *s->pos++, v = *s->pos++; (void)v; SeqNoteOff(s, ch, n); break; }
    case 0x90: { int n = *s->pos++, v = *s->pos++;
                 if (v) SeqNoteOn(s, ch, n, v); else SeqNoteOff(s, ch, n); break; }
    case 0xa0: s->pos += 2; break;                 /* poly aftertouch (ignore)   */
    case 0xb0: { int c = *s->pos++, v = *s->pos++; /* control change             */
                 if (c == 0x07 || c == 0x0b) s->chVol[ch] = (unsigned char)v;
                 else if (c == 0x0a) s->chPan[ch] = (unsigned char)v;  /* pan */
                 else if (c == 0x7b || c == 0x78) { int n; for (n = 0; n < 128; n++) SeqNoteOff(s, ch, n); }
                 break; }
    case 0xc0: s->chProg[ch] = *s->pos++; break;   /* program change             */
    case 0xd0: s->pos += 1; break;                 /* channel pressure (ignore)  */
    case 0xe0: s->pos += 2; break;                 /* pitch bend (ignored)       */
    case 0xf0:
        if (status == 0xff) {                      /* meta event                 */
            /* PS1 SEQ meta events carry no SMF-style length byte: a tempo event is `FF 51 tt tt tt`
             * (3 raw bytes, us/quarter), not `FF 51 03 ...`. Reading the first tempo byte as a
             * length skips into a note-on and desyncs the whole stream. */
            unsigned char meta = *s->pos++;
            if (meta == 0x2f) return 0;            /* end of track               */
            if (meta == 0x51) {                    /* set tempo: exactly 3 bytes */
                s->tempo = (s->pos[0] << 16) | (s->pos[1] << 8) | s->pos[2];
                if (s->tempo == 0) s->tempo = 500000;
                s->pos += 3;
                break;
            }
            return 0;                              /* other meta: PS1 SEQ music uses only 0x51/0x2F -> stop safe */
        } else {
            return 0;                              /* unhandled F0-FE: stop safe */
        }
        break;
    default: return 0;
    }
    return (s->pos <= s->end);
}

short SsSeqOpen(unsigned int *seqData, short vabId) {
    /* PsyQ's 2nd arg is the VAB id the sequence plays through (the game passes 1 = SD_SEQ, the
     * music bank), not a mode. Binding to the last-opened VAB instead picks the map's SFX bank
     * during a battle -- the wrong instruments. */
    const unsigned char *d = (const unsigned char *)seqData;
    if (!d || !(d[0] == 'p' && d[1] == 'Q' && d[2] == 'E' && d[3] == 'S')) return -1;
    int slot = -1, i;
    for (i = 0; i < MAX_SEQ; i++) if (!s_seqs[i].inUse) { slot = i; break; }
    if (slot < 0) return -1;
    Seq *s = &s_seqs[slot];
    memset(s, 0, sizeof(*s));
    s->inUse = 1;
    /* header: magic(4) version(4) ppqn(2 BE) tempo(3 BE) rhythm(2); events follow */
    s->ppqn  = (d[8] << 8) | d[9];              if (s->ppqn == 0)  s->ppqn = 480;
    s->tempo = (d[10] << 16) | (d[11] << 8) | d[12]; if (s->tempo == 0) s->tempo = 500000;
    s->base = d + 15;
    s->end  = d + SEQ_MAX_BYTES;
    s->vabId = (vabId >= 0 && vabId < MAX_VAB && s_vabs[vabId].inUse) ? vabId : g_seqLastVabOpened;
    s->volL = s->volR = 127;
    { int c, n; for (c = 0; c < SEQ_CHANNELS; c++) { s->chVol[c] = 127; s->chProg[c] = 0;
        s->chPan[c] = 64;   /* centre until the SEQ sends CC 0x0A */
        for (n = 0; n < 128; n++) s->noteVoice[c][n] = -1; } }
    { FILE *lg = SeqLog(); if (lg) {
        fprintf(lg, "[open] slot=%d ppqn=%d tempo=%u boundVab=%d\n", slot, s->ppqn, s->tempo, s->vabId);
        for (i = 0; i < MAX_VAB; i++) if (s_vabs[i].inUse)
            fprintf(lg, "   vab[%d] inUse numPrograms=%d numVag=%d\n", i, s_vabs[i].numPrograms, s_vabs[i].numVag);
        if (getenv("VH_SEQ_LOG")) { int hb; fprintf(lg, "   hexdump d[0..191] (15-byte header, events from d[15]):\n");
          for (hb = 0; hb < 192; hb += 16) {
              int hc; fprintf(lg, "   %04x:", hb);
              for (hc = 0; hc < 16; hc++) fprintf(lg, " %02x", d[hb + hc]);
              fprintf(lg, "\n"); } }
        fflush(lg); } }
    s_seqDbgNotes = 0;
    return (short)slot;
}

static void SeqReleaseOrphanVoices(short num);   /* stuck-note safety net; defined with SsSeqStop */

void SsSeqPlay(short num, char playMode, short repeatCount) {
    if (num < 0 || num >= MAX_SEQ || !s_seqs[num].inUse) return;
    Seq *s = &s_seqs[num];
    if (playMode != SSPLAY_PLAY) {
        /* Pause/stop mode: freeze the sequencer and release held notes, else a sustained voice
         * rings forever. Defensive: the game only passes SSPLAY_PLAY, and play restarts from the
         * top, so there is no resume-from-pause state to keep. */
        s->playing = 0;
        { int c, n; for (c = 0; c < SEQ_CHANNELS; c++) for (n = 0; n < 128; n++) SeqNoteOff(s, c, n); }
        SeqReleaseOrphanVoices(num);
        return;
    }
    s->loopInfinite = (repeatCount == SSPLAY_INFINITY); /* 0 == loop forever */
    s->repeatLeft = repeatCount;
    s->pos = s->base;
    s->running = 0;
    s->tickBudget = 0;
    s->ticksToNext = SeqReadVLQ(&s->pos, s->end);       /* first delta */
    s->playing = 1;
}

/* Force-release every voice still owned by sequence `num`, tracked in noteVoice[][] or not (a
 * voice orphaned by a steal chain otherwise sustains after the sequence stops).
 * See docs/pc-port/subsystems/spu.md, "Voice ownership and the stuck-note reaper". */
static void SeqReleaseOrphanVoices(short num) {
    int vi, released = 0;
    for (vi = 0; vi < MAX_VOICES; vi++) {
        if (s_voices[vi].active && s_voices[vi].ownSeq == num) {
            PC_SpuKeyOff(vi);   /* idempotent: no-op if already releasing */
            released++;
        }
    }
    if (released) { FILE *lf = SeqLog(); if (lf) { fprintf(lf, "seq %d: force-released %d orphaned voice(s)\n", num, released); fflush(lf); } }
}

void SsSeqStop(short num) {
    if (num < 0 || num >= MAX_SEQ || !s_seqs[num].inUse) return;
    Seq *s = &s_seqs[num];
    s->playing = 0;
    { int c, n; for (c = 0; c < SEQ_CHANNELS; c++) for (n = 0; n < 128; n++) SeqNoteOff(s, c, n); }
    SeqReleaseOrphanVoices(num);   /* stuck-note safety net (see above) */
    { FILE *lf = SeqLog(); if (lf) { fprintf(lf, "SsSeqStop(%d)\n", num); fflush(lf); } }
}

void SsSeqSetVol(short num, short lVol, short rVol) {
    if (num < 0 || num >= MAX_SEQ || !s_seqs[num].inUse) return;
    s_seqs[num].volL = lVol; s_seqs[num].volR = rVol; /* applied on subsequent note-ons */
}

void SsSeqClose(short num) {
    if (num < 0 || num >= MAX_SEQ || !s_seqs[num].inUse) return;
    SsSeqStop(num);
    s_seqs[num].inUse = 0;
}

/* End-of-track handling shared by the real 0x2F end and the runaway-delta watchdog: releases held
 * notes, then loops or stops (returns 1 if it looped). releaseOnLoop drops held notes on a loop
 * too -- on for the watchdog (the held chord is stale), off for a clean 0x2F seam. */
static int SeqEndOfTrack(Seq *s, int releaseOnLoop) {
    if (s->loopInfinite || s->repeatLeft > 1) {
        if (!s->loopInfinite) s->repeatLeft--;
        if (releaseOnLoop) { int c, n; for (c = 0; c < SEQ_CHANNELS; c++) for (n = 0; n < 128; n++) SeqNoteOff(s, c, n); }
        s->pos = s->base; s->running = 0; s->tickBudget = 0;
        s->ticksToNext = SeqReadVLQ(&s->pos, s->end);
        return 1;
    }
    s->playing = 0;
    { int c, n; for (c = 0; c < SEQ_CHANNELS; c++) for (n = 0; n < 128; n++) SeqNoteOff(s, c, n); }
    return 0;
}

/* Advance every playing sequence by dtUsec of musical time (tempo-scaled). */
static void SeqAdvanceByUsec(double dtUsec) {
    int si;
    for (si = 0; si < MAX_SEQ; si++) {
        Seq *s = &s_seqs[si];
        if (!s->inUse || !s->playing) continue;
        s->tickBudget += (double)s->ppqn * dtUsec / (double)s->tempo;
        /* Runaway inter-event delta: s->end is a fixed 0x40000 cap, not the true length, so a
         * parser that runs past real data reads a garbage VLQ that tickBudget can never reach.
         * Treat it as end-of-track. See docs/pc-port/subsystems/spu.md, "Runaway-delta watchdog". */
        int maxDelta = (int)s->ppqn * 32;   /* 32 quarter-notes; far beyond any real gap */
        int guard = 0;
        while (guard++ < 20000) {
            if (s->ticksToNext > maxDelta)      {
                { FILE *lf = SeqLog(); if (lf) { fprintf(lf,
                    "[watchdog] seq %d runaway delta=%d (pos-base=%d) -> end-of-track\n",
                    si, s->ticksToNext, (int)(s->pos - s->base)); fflush(lf); } }
                if (SeqEndOfTrack(s, 1)) continue; else break; }
            if (s->tickBudget < (double)s->ticksToNext) {              /* wait for musical time */
                /* diag: a wait over 8 quarter-notes is a stall candidate; log tempo+delta,
                 * rate-limited ~1/s per seq. Normal music never trips it. */
                if (s->ticksToNext > s->ppqn * 8) {
                    extern unsigned int SDL_GetTicks(void);
                    static unsigned int lastLog[MAX_SEQ];
                    unsigned int now = SDL_GetTicks();
                    if (now - lastLog[si] > 1000) { lastLog[si] = now; FILE *lf = SeqLog();
                        if (lf) { fprintf(lf, "[stall?] seq=%d tempo=%u ppqn=%d ticksToNext=%d tickBudget=%.1f posBase=%d\n",
                                  si, s->tempo, s->ppqn, s->ticksToNext, s->tickBudget, (int)(s->pos - s->base)); fflush(lf); } }
                }
                break;
            }
            s->tickBudget -= (double)s->ticksToNext;
            if (!SeqProcessEvent(s))            { if (SeqEndOfTrack(s, 0)) continue; else break; }
            s->ticksToNext = SeqReadVLQ(&s->pos, s->end);              /* delta before next event */
        }
    }
}

/* Free pool slots whose software-SPU note has finished, and run the stuck-note reaper: a held SEQ
 * voice whose owning sequence has dropped its back-reference can never get a note-off, so release
 * it. See docs/pc-port/subsystems/spu.md, "Voice ownership and the stuck-note reaper". */
static void SwSpuSyncActive(void) {
    int vi;
    for (vi = 0; vi < MAX_VOICES; vi++) {
        if (s_voices[vi].active && !PC_SpuVoiceActive(vi)) {
            s_voices[vi].active = 0;
            s_voices[vi].ownSeq = -1;
            continue;
        }
        if (s_voices[vi].active && s_voices[vi].ownSeq >= 0 && !PC_SpuVoiceReleasing(vi)) {
            Voice *vc = &s_voices[vi];
            Seq *o = &s_seqs[vc->ownSeq];
            int refd = o->inUse && vc->ownCh >= 0 && vc->ownCh < SEQ_CHANNELS
                       && vc->ownNote >= 0 && vc->ownNote < 128
                       && o->noteVoice[vc->ownCh][vc->ownNote] == vi;
            if (!refd) {
                PC_SpuKeyOff(vi);
                { FILE *lf = SeqLog(); if (lf) { fprintf(lf,
                    "[reaper] force-released orphan voice=%d ownSeq=%d ch=%d note=%d\n",
                    vi, vc->ownSeq, vc->ownCh, vc->ownNote); fflush(lf); } }
            }
        }
    }
}

/* Audio-clock entry point (called from pc_spu.c's render loop): advance the
 * sequencer by exactly n rendered output samples, so note timing is locked to
 * the audio stream rather than the jittery video-frame clock. */
void PC_SeqAdvanceSamples(int n) {
    SwSpuSyncActive();                                 /* reclaim finished voices first */
    SeqAdvanceByUsec((double)n / 44100.0 * 1000000.0);
}

/* Called once per VSync (libetc.c). The sequencer itself is advanced from pc_spu's
 * render loop (audio-clock, PC_SeqAdvanceSamples); here we only pump the sink. */
void PC_SeqTick(void) {
    int activeSeq = 0, vi;

    PC_SpuSetMaster(s_masterVolL, s_masterVolR);
    PC_SpuSetReverb(s_reverbOn, s_reverbDepth);        /* SPU reverb (STUDIO_C) */
    PC_SpuService();                                   /* drives PC_SeqAdvanceSamples per block */
    for (vi = 0; vi < MAX_VOICES; vi++) if (PC_SpuVoiceActive(vi)) activeSeq++;

    /* diag: density + note-on/off balance + master vol + wall-clock, ~6x/sec. A jumping timestamp
     * during a silence means the game thread blocked; mvol 0 means an intentional mute; activeSeq 0
     * with a seq still 'playing' means the SEQ stopped. */
    { static int tk = 0, verbose = -1; FILE *lg = SeqLog();
      if (verbose < 0) verbose = getenv("VH_SEQ_LOG") ? 1 : 0;   /* gate the ~6x/sec spam; low-volume [note]/[reaper]/SsSeqStop stay on */
      if (lg && verbose && (++tk % 10) == 0) {
          extern unsigned int SDL_GetTicks(void);
          extern short gSeqSetIdx; extern unsigned char gSeqCurrentID;
          int anyPlaying = 0, si; for (si = 0; si < MAX_SEQ; si++) if (s_seqs[si].inUse && s_seqs[si].playing) anyPlaying = 1;
          fprintf(lg, "[voices] t=%ums activeSeq=%d seqPlaying=%d ons=%d offs=%d mvol=%d/%d "
                  "reverbOn=%d depth=%.2f | seqSetIdx=%d seqCurrentID=%u\n",
                  SDL_GetTicks(), activeSeq, anyPlaying, s_seqOns, s_seqOffs,
                  (int)s_masterVolL, (int)s_masterVolR, s_reverbOn, s_reverbDepth,
                  (int)gSeqSetIdx, (unsigned)gSeqCurrentID);
          /* Per-voice roster: pinpoint a stuck note. A voice that stays "HELD" (not
           * releasing) across many dumps is the culprit; ownSeq<0 => stuck SFX,
           * ownSeq>=0 => note held by a (likely stalled) sequence. */
          { int vj; for (vj = 0; vj < MAX_VOICES; vj++)
              if (PC_SpuVoiceActive(vj))
                  fprintf(lg, "    voice=%d ownSeq=%d ch=%d note=%d %s\n", vj,
                          s_voices[vj].ownSeq, (int)s_voices[vj].ownCh, (int)s_voices[vj].ownNote,
                          PC_SpuVoiceReleasing(vj) ? "releasing" : "HELD"); }
          fflush(lg); } }
}
