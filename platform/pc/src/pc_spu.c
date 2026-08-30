/*
 * Software SPU voice renderer (Phase 2.1, milestone M1).
 *
 * Replaces the OpenAL-per-voice music/SFX backend with a sample-accurate PS1
 * SPU: each voice is stepped through its decoded SPU-ADPCM (VAG) PCM at the SPU
 * pitch rate, interpolated, gained, and the up-to-32 voices are summed into one
 * stereo 44100 Hz stream fed to a single OpenAL streaming source (same no-thread
 * per-VSync top-up pattern as pc_xa.c). This removes the dependence on OpenAL's
 * per-source AL_PITCH/AL_GAIN and (later, M5) the EFX reverb extension.
 *
 * Adapted from the working per-sample SPU core in ctr-native/platform/
 * native_audio.c (sibling native PS1 port, already a trusted reference here),
 * cross-checked against psx-spx soundprocessingunitspu.md.
 *
 * M1 = pitch step + LINEAR interpolation + flat gain (short click-avoidance
 * ramps), no ADSR/Gaussian/reverb yet. Those land in M3/M4/M5.
 */
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

#define SPU_VOICES        32          /* matches libsnd.c MAX_VOICES (voice indices)          */
#define SPU_RATE          44100
#define SPU_BLOCK_FRAMES  512         /* ~11.6 ms per block (sequencer advance granularity)   */
#define SPU_NUM_BUFFERS   12          /* ~140 ms pool: fewer underruns on game-thread hitches  */
#define SPU_UNITY_STEP    0x1000      /* VxPitch 0x1000 = 1.0x (per psx-spx)                  */
#define SPU_MAX_STEP      0x4000      /* clamp to 176.4 kHz                                   */
#define ADSR_MAX          0x7fff
#define ADSR_STEP_BIT     0x8000u

enum { ADSR_OFF, ADSR_ATTACK, ADSR_DECAY, ADSR_SUSTAIN, ADSR_RELEASE };

typedef struct {
    const short *pcm;                 /* decoded PCM (owned by libsnd's VAB)                  */
    int          len;                 /* sample count                                        */
    int          loopS, loopE;        /* sustain loop [loopS,loopE), or loopE<=loopS = none   */
    int          active;
    int          revOn;               /* VagAtr.mode bit2 -- this voice feeds the reverb send.
                                       * Hardware sets EON per voice; the music bank's tones all
                                       * carry mode=4 and EVERY SFX bank's carry mode=0, so SFX
                                       * are dry. Sending the whole mix (what we did until
                                       * 2026-07-21) gave menu beeps a 1.2 s reverb tail. */
    int          looped;              /* 1 once the sustain loop has wrapped at least once;
                                       * makes the Gaussian history fetch loop-aware (see
                                       * SpuGetSample -- hardware keeps a running 4-sample
                                       * shift register, so its history never sees pre-loop
                                       * data after a wrap). */
    unsigned long long phase;         /* 20.12 fixed-point sample position                   */
    unsigned int step;                /* VxPitch (0x1000 = unity)                            */
    int          dbgVag, dbgProg, dbgNote;  /* identity for the fidelity trace (exchange/57) */
    float        gainL, gainR;        /* per-voice L/R note volume, 0..1 (velocity*chan*seq*
                                       * vol*pan). Hardware has independent VolL/VolR per voice
                                       * (octoshock spu.cpp reads them per-voice); previously a
                                       * single mono gain was summed into both channels, i.e. the
                                       * whole mix rendered dead-centre and the VAB tone pan /
                                       * SEQ channel pan were ignored. See exchange/57. */
    /* Sample-accurate ADSR (M3), ported from ctr-native native_audio.c / psx-spx. */
    int          adsrLevel;           /* 0..0x7fff current envelope level                     */
    unsigned int adsrCounter;         /* 15-bit fractional rate counter                       */
    int          adsrPhase;           /* enum ADSR_*                                          */
    /* decoded from adsr1/adsr2 at key-on */
    int arShift, arStep, aExp;        /* attack  shift/step/exponential                       */
    int drShift;                      /* decay   shift (always exp decrease)                  */
    int slTarget;                     /* sustain target level = min(0x7fff,(SL+1)<<11)        */
    int srShift, srStep, sExp, sDec;  /* sustain shift/step/exp/decreasing                    */
    int rrShift, rExp;                /* release shift/exponential (always decrease)          */
} SpuVoice;

static SpuVoice s_v[SPU_VOICES];
static float    s_masterL = 1.0f, s_masterR = 1.0f;

static int Clamp16(int x) { return x < -32768 ? -32768 : (x > 32767 ? 32767 : x); }

/* Reference-matching EQ tilt -- now HARDWIRED OFF (s_analogOn = 0); the code is kept but unreached.
 * CORRECTION (2026-07-17): this was originally believed to model the PS1 *analog output stage*,
 * but the octoshock reference is RAW DIGITAL -- spu.cpp sums voices+reverb, applies main volume,
 * clamps, and scales x0.75, with NO analog/lowpass filter anywhere. So this is NOT analog modeling;
 * it's a small spectral-tilt correction. Once the square volume law landed it OVER-corrects (2.20 dB
 * -> 4.27 dB mean error vs the reference), so it stays off; the proper fix lives in the raw mix itself
 * (interpolation aliasing / ADSR-attack brightness / reverb balance). See memory spu_m1_progress. */
static int   s_analogOn = -1;
static float s_bassK = 2.0f, s_bassA, s_trebK = 0.6f, s_trebA;
static float s_bassLP[2], s_trebLP[2];
static float EnvF(const char *k, float d){ const char *e=getenv(k); return e?(float)atof(e):d; }
static void AnalogInit(void) {
    if (s_analogOn >= 0) return;
    /* DEFAULT FLIPPED TO OFF, 2026-07-20. This curve was fitted to compensate for the missing
     * SQUARE LAW (see exchange/57). With the square law implemented the compensation OVER-corrects:
     * measured mean|error| vs the octoshock reference over 30 Hz-12 kHz is
     *     raw mix 2.20 dB   vs   raw mix + this filter 4.27 dB
     * i.e. enabling it is now WORSE than the pre-fix build (3.21 dB). Now hardwired OFF -- the square
     * law makes it an over-correction; the code path is kept (fed by s_bassK/s_trebK) but unreached. */
    s_analogOn = 0;
    s_bassK = EnvF("VH_SPU_BASS", 2.0f);           /* sub-bass boost amount (fit: ~4->1.4dB) */
    s_trebK = EnvF("VH_SPU_TREB", 0.6f);           /* treble softening 0..1                   */
    s_bassA = 1.0f - (float)exp(-2.0 * 3.14159265 * EnvF("VH_SPU_BASSFC", 70.0f)  / SPU_RATE);
    s_trebA = 1.0f - (float)exp(-2.0 * 3.14159265 * EnvF("VH_SPU_TREBFC", 4500.0f)/ SPU_RATE);
}
static float AnalogCh(float x, int c) {
    float b, y;
    s_bassLP[c] += s_bassA * (x - s_bassLP[c]);    /* lowpassed sub-bass  */
    b = x + s_bassK * s_bassLP[c];                 /* + boost             */
    s_trebLP[c] += s_trebA * (b - s_trebLP[c]);    /* lowpassed (darker)  */
    y = (1.0f - s_trebK) * b + s_trebK * s_trebLP[c];
    return y;
}

/* ---- OpenAL streaming sink (mirrors pc_xa.c) ---- */
static ALuint s_source = 0;
static ALuint s_bufFree[SPU_NUM_BUFFERS];
static int    s_bufFreeCount = 0;
static int    s_alReady = 0;
static int    s_started = 0;

void PC_SpuSetMaster(int l127, int r127) {
    s_masterL = (l127 < 0 ? 0 : l127) / 127.0f;
    s_masterR = (r127 < 0 ? 0 : r127) / 127.0f;
}

/* Decode the PS1 ADSR1/ADSR2 words into per-phase shift/step/mode fields
 * (psx-spx "Volume and ADSR Generator"; layout matches ctr-native). */
static void SpuDecodeAdsr(SpuVoice *v, unsigned short adsr1, unsigned short adsr2) {
    int ar = (adsr1 >> 8) & 0x7f;
    int sr = (adsr2 >> 6) & 0x7f;
    int sl = adsr1 & 0xf;
    v->arShift = (ar >> 2) & 0x1f; v->arStep = ar & 3; v->aExp = (adsr1 & 0x8000) != 0;
    v->drShift = (adsr1 >> 4) & 0xf;
    v->slTarget = ((sl + 1) << 11); if (v->slTarget > ADSR_MAX) v->slTarget = ADSR_MAX;
    v->srShift = (sr >> 2) & 0x1f; v->srStep = sr & 3;
    v->sExp = (adsr2 & 0x8000) != 0; v->sDec = (adsr2 & 0x4000) != 0;
    v->rrShift = adsr2 & 0x1f; v->rExp = (adsr2 & 0x20) != 0;
}

void PC_SpuKeyOn(int voice, const short *pcm, int len, int loopS, int loopE,
                 unsigned int step, float gainL, float gainR,
                 unsigned short adsr1, unsigned short adsr2, int revOn) {
    SpuVoice *v;
    if (voice < 0 || voice >= SPU_VOICES || !pcm || len <= 0) return;
    if (step < 1) step = 1;
    if (step > SPU_MAX_STEP) step = SPU_MAX_STEP;
    v = &s_v[voice];
    v->pcm = pcm; v->len = len;
    v->loopS = loopS; v->loopE = loopE;
    v->phase = 0;
    v->looped = 0;               /* history starts cleared, as on a hardware key-on */
    v->revOn = revOn;            /* VagAtr.mode bit2 -- hardware's per-voice EON */
    v->step = step;
    v->gainL = gainL < 0.0f ? 0.0f : gainL;
    v->gainR = gainR < 0.0f ? 0.0f : gainR;
    SpuDecodeAdsr(v, adsr1, adsr2);
    v->adsrLevel = 0;
    v->adsrCounter = 0;
    v->adsrPhase = ADSR_ATTACK;
    v->active = 1;
}

/* Tag a voice with the sample/program/note it is playing, so the fidelity trace can identify
 * WHICH VAG the bass notes use (exchange/57). Called right after PC_SpuKeyOn by libsnd. */
void PC_SpuSetVoiceDebugId(int voice, int vag, int prog, int note) {
    if (voice < 0 || voice >= SPU_VOICES) return;
    s_v[voice].dbgVag = vag; s_v[voice].dbgProg = prog; s_v[voice].dbgNote = note;
}

void PC_SpuKeyOff(int voice) {
    if (voice < 0 || voice >= SPU_VOICES) return;
    if (s_v[voice].active) s_v[voice].adsrPhase = ADSR_RELEASE;
}

/* One 44100Hz envelope step (psx-spx "Envelope Operation"; phaseNegative is always
 * false for normal ADSR, so it's dropped). Updates v->adsrLevel in place. */
static void SpuAdsrRunStep(SpuVoice *v, int shift, int stepv, int exponential, int decreasing,
                           int rateAllOnes) {
    int adsrStep;
    unsigned int inc, counter;
    if (rateAllOnes) return;
    adsrStep = 7 - stepv;
    if (decreasing) adsrStep = ~adsrStep;
    if (shift < 11) adsrStep <<= (11 - shift);
    inc = ADSR_STEP_BIT;
    if (shift > 11) { int s = shift - 11; inc = (s < 31) ? (ADSR_STEP_BIT >> s) : 0; }
    if (exponential && !decreasing && v->adsrLevel > 0x6000) {
        if (shift < 10) adsrStep >>= 2;
        else if (shift >= 11) inc >>= 2;
        else { adsrStep >>= 1; inc >>= 1; }
    } else if (exponential && decreasing) {
        adsrStep = (int)(((long long)adsrStep * v->adsrLevel) / 0x8000);
        if (v->adsrPhase == ADSR_RELEASE && adsrStep == 0 && v->adsrLevel > 0) adsrStep = -1;
    }
    if (inc == 0) inc = 1;
    counter = v->adsrCounter + inc;
    v->adsrCounter = counter & (ADSR_STEP_BIT - 1);
    if ((counter & ADSR_STEP_BIT) == 0) return;      /* not time to step yet */
    v->adsrLevel += adsrStep;
    if (!decreasing) { if (v->adsrLevel > ADSR_MAX) v->adsrLevel = ADSR_MAX;
                       else if (v->adsrLevel < 0) v->adsrLevel = 0; }
    else if (v->adsrLevel < 0) v->adsrLevel = 0;
}

/* Advance the phase machine by one output sample. Returns 0 when the voice
 * has finished (release reached zero) -> caller deactivates. */
static int SpuAdsrAdvance(SpuVoice *v) {
    switch (v->adsrPhase) {
    case ADSR_ATTACK: {
        int allOnes = ((v->arStep & 3) | (v->arShift << 2)) == 0x7f;
        SpuAdsrRunStep(v, v->arShift, v->arStep, v->aExp, 0, allOnes);
        if (v->adsrLevel >= ADSR_MAX) { v->adsrLevel = ADSR_MAX; v->adsrPhase = ADSR_DECAY; }
        break; }
    case ADSR_DECAY:
        if (v->adsrLevel <= v->slTarget) { v->adsrLevel = v->slTarget; v->adsrPhase = ADSR_SUSTAIN; break; }
        SpuAdsrRunStep(v, v->drShift, 0, 1, 1, 0);
        if (v->adsrLevel <= v->slTarget) { v->adsrLevel = v->slTarget; v->adsrPhase = ADSR_SUSTAIN; }
        break;
    case ADSR_SUSTAIN: {
        int allOnes = ((v->srStep & 3) | (v->srShift << 2)) == 0x7f;
        SpuAdsrRunStep(v, v->srShift, v->srStep, v->sExp, v->sDec, allOnes);
        /* A sustain-DECREASE that has faded to 0 is silent forever (a looping SFX with no
         * key-off relies on exactly this). Free the voice so it doesn't linger active,
         * hogging the pool and blocking new SFX/music -- the SFX log showed 90 looping
         * key-ons vs 14 key-offs, i.e. most SFX end via ADSR, not key-off. */
        if (v->sDec && v->adsrLevel <= 0) return 0;
        break; }
    case ADSR_RELEASE:
        if (v->adsrLevel <= 0) return 0;
        SpuAdsrRunStep(v, v->rrShift, 0, v->rExp, 1, v->rrShift == 0x1f);
        if (v->adsrLevel <= 0) return 0;
        break;
    default: return 0;
    }
    return 1;
}

void PC_SpuStopAll(void) {
    int i;
    for (i = 0; i < SPU_VOICES; i++) { s_v[i].active = 0; s_v[i].adsrPhase = ADSR_OFF; }
}

/* A VAB bank is about to free its decoded PCM (SsVabClose); hard-stop every voice still
 * pointing at it, including ones in RELEASE -- a releasing voice keeps reading v->pcm each
 * render (see SpuGetSample), so only stopping outright (not key-off) is safe here. */
void PC_SpuStopVoicesUsing(const short *pcm) {
    int i;
    if (!pcm) return;
    for (i = 0; i < SPU_VOICES; i++) {
        if (s_v[i].pcm == pcm) { s_v[i].active = 0; s_v[i].adsrPhase = ADSR_OFF; }
    }
}

/* So libsnd's voice allocator (which keys off s_voices[].active) can track when a
 * software-SPU note has finished on its own (one-shot end or release). */
int PC_SpuVoiceActive(int voice) {
    if (voice < 0 || voice >= SPU_VOICES) return 0;
    return s_v[voice].active;
}

/* True if the voice is no longer held (release tail or off) -- i.e. its note-off
 * already happened and it will fade out on its own. Lets the SEQ layer tell a
 * stuck (still-held) orphan from a normally-releasing voice. */
int PC_SpuVoiceReleasing(int voice) {
    if (voice < 0 || voice >= SPU_VOICES) return 1;
    return s_v[voice].adsrPhase == ADSR_RELEASE || s_v[voice].adsrPhase == ADSR_OFF;
}

/* PS1 SPU 4-point Gaussian interpolation table (psx-spx; values verbatim from ctr-native). */
static const short s_gauss[512] = {
    -1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,-1,
    0,0,0,0,0,0,0,1,1,1,1,2,2,2,3,3,
    3,4,4,5,5,6,7,7,8,9,9,10,11,12,13,14,
    15,16,17,18,19,21,22,24,25,27,28,30,32,33,35,37,
    39,41,44,46,48,51,53,56,58,61,64,67,70,73,77,80,
    84,87,91,95,99,103,107,111,116,120,125,130,135,140,145,150,
    156,161,167,173,179,186,192,199,205,212,219,227,234,242,250,257,
    266,274,283,291,300,309,319,328,338,348,358,369,379,390,401,412,
    424,436,448,460,473,485,498,512,525,539,553,567,582,597,612,627,
    643,659,675,692,708,726,743,761,779,797,816,835,854,874,894,914,
    935,956,977,999,1020,1043,1066,1089,1112,1136,1160,1184,1209,1234,1260,1286,
    1312,1339,1366,1394,1422,1450,1479,1508,1537,1567,1598,1628,1660,1691,1723,1756,
    1789,1822,1856,1890,1924,1959,1995,2031,2067,2104,2141,2179,2217,2256,2295,2334,
    2374,2415,2456,2497,2539,2582,2624,2668,2712,2756,2801,2846,2892,2938,2985,3032,
    3079,3128,3176,3225,3275,3325,3376,3427,3479,3531,3584,3637,3691,3745,3799,3855,
    3910,3967,4023,4081,4138,4197,4255,4315,4374,4435,4495,4557,4619,4681,4744,4807,
    4871,4935,5000,5065,5131,5197,5264,5332,5399,5468,5536,5606,5676,5746,5817,5888,
    5959,6032,6104,6177,6251,6325,6400,6475,6550,6626,6702,6779,6856,6934,7012,7091,
    7170,7249,7329,7409,7490,7571,7653,7735,7817,7900,7983,8066,8150,8234,8319,8404,
    8489,8575,8661,8748,8834,8922,9009,9097,9185,9273,9362,9451,9541,9630,9720,9811,
    9901,9992,10083,10174,10266,10358,10450,10542,10635,10727,10820,10913,11007,11100,11194,11288,
    11382,11476,11571,11665,11760,11855,11950,12045,12140,12236,12331,12427,12522,12618,12714,12809,
    12905,13001,13097,13193,13289,13385,13481,13577,13673,13769,13865,13961,14056,14152,14248,14343,
    14439,14534,14630,14725,14820,14915,15010,15104,15199,15293,15387,15481,15575,15669,15762,15855,
    15948,16041,16133,16226,16317,16409,16500,16592,16682,16773,16863,16953,17042,17131,17220,17308,
    17396,17484,17571,17658,17744,17830,17916,18001,18086,18170,18254,18337,18420,18502,18584,18665,
    18746,18826,18905,18985,19063,19141,19219,19295,19372,19447,19522,19597,19671,19744,19816,19888,
    19959,20030,20100,20169,20238,20306,20373,20439,20505,20570,20634,20698,20760,20822,20884,20944,
    21004,21063,21121,21178,21235,21290,21345,21399,21452,21505,21556,21607,21657,21706,21754,21801,
    21848,21893,21938,21982,22025,22066,22107,22148,22187,22225,22262,22299,22334,22369,22402,22435,
    22467,22498,22527,22556,22584,22611,22637,22662,22686,22709,22731,22752,22772,22791,22809,22826,
    22842,22857,22872,22885,22897,22908,22918,22927,22935,22942,22948,22953,22957,22960,22962,22963 };

/* Loop-aware PCM fetch. Out-of-range reads 0 (a keyed-on voice starts with a cleared history).
 *
 * The loop-awareness is REAL now -- it previously wasn't, despite the comment. The 4-point
 * Gaussian needs the three samples that PRECEDED idx *in playback order*. Back-indexing gives
 * that everywhere except immediately after a sustain-loop wrap: at idx==loopS the true history
 * is loopE-1..loopE-3 (the end of the loop we just came from), but a raw idx-1..idx-3 reads
 * loopS-1..loopS-3 -- i.e. the pre-loop ATTACK data, which is unrelated. Hardware never has this
 * problem because it keeps a running 4-sample shift register rather than indexing backwards.
 * The result was a 3-sample discontinuity on EVERY wrap of EVERY sustained voice; see
 * exchange/57 (it is the last surviving candidate for the +6 dB 3-6 kHz excess in the raw mix). */
static int SpuGetSample(const SpuVoice *v, int idx) {
    if (v->looped && v->loopE > v->loopS && idx < v->loopS)
        idx = v->loopE - (v->loopS - idx);   /* wrap the history back into the loop tail */
    if (idx < 0 || idx >= v->len) return 0;
    return v->pcm[idx];
}

/* Read one interpolated sample from a voice (4-point Gaussian; psx-spx). The
 * gauss index is bits 4..11 of the 12-frac-bit pitch counter. */
static float SpuVoiceSample(const SpuVoice *v) {
    int idx = (int)(v->phase >> 12);
    int gi  = (int)((v->phase >> 4) & 0xff);
    int s3 = SpuGetSample(v, idx - 3);   /* oldest */
    int s2 = SpuGetSample(v, idx - 2);   /* older  */
    int s1 = SpuGetSample(v, idx - 1);   /* old    */
    int s0 = SpuGetSample(v, idx);       /* new    */
    int out = (s_gauss[0xff - gi] * s3) >> 15;
    out    += (s_gauss[0x1ff - gi] * s2) >> 15;
    out    += (s_gauss[0x100 + gi] * s1) >> 15;
    out    += (s_gauss[gi]        * s0) >> 15;
    return (float)Clamp16(out);
}

/* Advance a voice's phase by one output sample; handle loop / end. */
static void SpuVoiceAdvance(SpuVoice *v) {
    v->phase += v->step;
    if (v->loopE > v->loopS && v->loopS >= 0) {
        unsigned long long loopEndPh = (unsigned long long)v->loopE << 12;
        unsigned long long loopLenPh = (unsigned long long)(v->loopE - v->loopS) << 12;
        while (v->phase >= loopEndPh) { v->phase -= loopLenPh; v->looped = 1; }
    } else if ((int)(v->phase >> 12) >= v->len) {
        v->active = 0;               /* one-shot finished */
    }
}

/* ---- SPU reverb (M5) --------------------------------------------------------
 * Faithful PS1 SPU reverb (the comb/all-pass network over a work buffer, run at
 * 22050 Hz with a 39-tap FIR resampling the 44100 in/out), ported from
 * ctr-native native_audio.c. The game selects SPU_REV_MODE_STUDIO_C, so only
 * that preset is embedded. Replaces the OpenAL EFX approximation for the SW
 * path. All SEQ music voices are reverb-enabled (as the OpenAL path routed
 * them), so the whole SW mix is the reverb send. */
#define REV_FIR_TAPS 39
#define REV_SAMPLES  0x37F0            /* STUDIO_C size: 0x6FE0 bytes / 2 */

/* register indices (preset order, matches psx-spx / ctr-native) */
enum { R_DAPF1, R_DAPF2, R_VIIR, R_VCOMB1, R_VCOMB2, R_VCOMB3, R_VCOMB4, R_VWALL,
       R_VAPF1, R_VAPF2, R_MLSAME, R_MRSAME, R_MLCOMB1, R_MRCOMB1, R_MLCOMB2, R_MRCOMB2,
       R_DLSAME, R_DRSAME, R_MLDIFF, R_MRDIFF, R_MLCOMB3, R_MRCOMB3, R_MLCOMB4, R_MRCOMB4,
       R_DLDIFF, R_DRDIFF, R_MLAPF1, R_MRAPF1, R_MLAPF2, R_MRAPF2, R_VLIN, R_VRIN, R_REGS };

static const short s_revFir[REV_FIR_TAPS] = {
    -0x0001,0x0000,0x0002,0x0000,-0x000A,0x0000,0x0023,0x0000,-0x0067,0x0000,0x010A,0x0000,-0x0268,
    0x0000,0x0534,0x0000,-0x0B90,0x0000,0x2806,0x4000,0x2806,0x0000,-0x0B90,0x0000,0x0534,0x0000,
    -0x0268,0x0000,0x010A,0x0000,-0x0067,0x0000,0x0023,0x0000,-0x000A,0x0000,0x0002,0x0000,-0x0001 };

static const short s_revStudioC[R_REGS] = {
    (short)0x00E3,(short)0x00A9,(short)0x6F60,(short)0x4FA8,(short)0xBCE0,(short)0x4510,(short)0xBEF0,(short)0xA680,
    (short)0x5680,(short)0x52C0,(short)0x0DFB,(short)0x0B58,(short)0x0D09,(short)0x0A3C,(short)0x0BD9,(short)0x0973,
    (short)0x0B59,(short)0x08DA,(short)0x08D9,(short)0x05E9,(short)0x07EC,(short)0x04B0,(short)0x06EF,(short)0x03D2,
    (short)0x05EA,(short)0x031D,(short)0x031C,(short)0x0238,(short)0x0154,(short)0x00AA,(short)0x8000,(short)0x8000 };

static struct {
    short buf[REV_SAMPLES];
    short inHistL[REV_FIR_TAPS], inHistR[REV_FIR_TAPS];
    short outHistL[REV_FIR_TAPS], outHistR[REV_FIR_TAPS];
    int cursor, inCur, outCur, phase;
} s_rev;
static int   s_revOn = 0;
static float s_revDepth = 0.0f;         /* 0..1 wet mix, from SsUtSetReverbDepth */

/* Wet-mix reverb, driven by the game via SsUtSetReverbDepth. (The old VH_SPU_REVDEPTH/REVOFF debug
 * overrides from the bass-deficit investigation, exchange/57, were removed once the mix was calibrated.) */
void PC_SpuSetReverb(int on, float depth) {
    s_revOn = on;
    s_revDepth = depth;
}

static int RevOff(short r){ return (int)((unsigned short)r) * 4; }
static int RevWrap(int i){ i %= REV_SAMPLES; if (i < 0) i += REV_SAMPLES; return i; }
static int RevRd(int reg, int d){ return s_rev.buf[RevWrap(s_rev.cursor + RevOff(s_revStudioC[reg]) + d)]; }
static int RevRdAt(int off){ return s_rev.buf[RevWrap(s_rev.cursor + off)]; }
static void RevWr(int reg, int v){ s_rev.buf[RevWrap(s_rev.cursor + RevOff(s_revStudioC[reg]))] = (short)Clamp16(v); }
static int RevMul(int s, short vol){ return (int)(((long long)s * (int)vol) / 0x8000); }
static int RevFir(const short *h, int cur){ long long sum = 0; int i;
    for (i = 0; i < REV_FIR_TAPS; i++) sum += (long long)s_revFir[i] * h[(cur + i) % REV_FIR_TAPS];
    return Clamp16((int)(sum / 0x8000)); }
static int RevReflect(int in, int fbReg, int wrReg){
    int prev = RevRd(wrReg, -1);
    int fb = RevMul(RevRd(fbReg, 0), s_revStudioC[R_VWALL]);
    int val = RevMul(in + fb - prev, s_revStudioC[R_VIIR]) + prev;
    return val; }
static int RevComb(int base){
    int v = RevMul(RevRd(base, 0), s_revStudioC[R_VCOMB1]);
    v += RevMul(RevRd(base + 2, 0), s_revStudioC[R_VCOMB2]);
    v += RevMul(RevRd(base + 8, 0), s_revStudioC[R_VCOMB3]);
    v += RevMul(RevRd(base + 10, 0), s_revStudioC[R_VCOMB4]);
    return v; }
static int RevApf(int in, int apfReg, int dReg, int volReg){
    int delta = RevOff(s_revStudioC[apfReg]) - RevOff(s_revStudioC[dReg]);
    int delayed = RevRdAt(delta);
    int stored = in - RevMul(delayed, s_revStudioC[volReg]);
    RevWr(apfReg, stored);
    delayed = RevRdAt(delta);
    return delayed + RevMul(stored, s_revStudioC[volReg]); }

/* Process one 44100Hz stereo sample; returns depth-scaled wet L/R. */
static void RevProcess(int sendL, int sendR, int *wetL, int *wetR) {
    int lin, rin, sameL, sameR, diffL, diffR, oL, oR;
    s_rev.inHistL[s_rev.inCur] = (short)Clamp16(sendL);
    s_rev.inHistR[s_rev.inCur] = (short)Clamp16(sendR);
    s_rev.inCur = (s_rev.inCur + 1) % REV_FIR_TAPS;
    s_rev.phase ^= 1;
    if (s_rev.phase == 0) {                 /* process at 22050Hz (every other sample) */
        lin = RevMul(RevFir(s_rev.inHistL, s_rev.inCur), s_revStudioC[R_VLIN]);
        rin = RevMul(RevFir(s_rev.inHistR, s_rev.inCur), s_revStudioC[R_VRIN]);
        sameL = RevReflect(lin, R_DLSAME, R_MLSAME);
        sameR = RevReflect(rin, R_DRSAME, R_MRSAME);
        diffL = RevReflect(lin, R_DRDIFF, R_MLDIFF);
        diffR = RevReflect(rin, R_DLDIFF, R_MRDIFF);
        RevWr(R_MLSAME, sameL); RevWr(R_MRSAME, sameR);
        RevWr(R_MLDIFF, diffL); RevWr(R_MRDIFF, diffR);
        oL = RevComb(R_MLCOMB1); oR = RevComb(R_MRCOMB1);
        oL = RevApf(oL, R_MLAPF1, R_DAPF1, R_VAPF1); oR = RevApf(oR, R_MRAPF1, R_DAPF1, R_VAPF1);
        oL = RevApf(oL, R_MLAPF2, R_DAPF2, R_VAPF2); oR = RevApf(oR, R_MRAPF2, R_DAPF2, R_VAPF2);
        s_rev.outHistL[s_rev.outCur] = (short)Clamp16(oL);
        s_rev.outHistR[s_rev.outCur] = (short)Clamp16(oR);
        s_rev.outCur = (s_rev.outCur + 1) % REV_FIR_TAPS;
        s_rev.cursor = RevWrap(s_rev.cursor + 1);
    } else {
        s_rev.outHistL[s_rev.outCur] = 0; s_rev.outHistR[s_rev.outCur] = 0;
        s_rev.outCur = (s_rev.outCur + 1) % REV_FIR_TAPS;
    }
    oL = Clamp16(RevFir(s_rev.outHistL, s_rev.outCur) * 2);   /* upsample back to 44100 */
    oR = Clamp16(RevFir(s_rev.outHistR, s_rev.outCur) * 2);
    *wetL = (int)(oL * s_revDepth);
    *wetR = (int)(oR * s_revDepth);
}

/* Per-voice trace mirroring exchange/58-spu-deep-trace.lua's hardware capture, so the two diff
 * field-for-field (bass-deficit investigation, exchange/57). Enable with VH_SPU_TRACE=1 ->
 * vh_spu_voices_ours.csv + vh_spu_globals_ours.csv. Sampled once per rendered block (not per
 * sample) to stay cheap; `frame` here is a block counter, so align to the hardware trace by
 * elapsed time, not by absolute frame number. */
static void SpuTrace(void) {
    static int en = -1;
    static FILE *fv = NULL, *fg = NULL;
    static long blk = 0;
    int i, nActive = 0;
    if (en < 0) en = getenv("VH_SPU_TRACE") ? 1 : 0;
    if (!en) return;
    if (!fv) {
        fv = fopen("vh_spu_voices_ours.csv", "w");
        fg = fopen("vh_spu_globals_ours.csv", "w");
        if (!fv || !fg) { en = 0; return; }
        fprintf(fv, "block,voice,vag,prog,note,step,sampleHz,gainL,gainR,adsrVol,adsrPhase,len,loopS,loopE,loops\n");
        fprintf(fg, "block,revOn,revDepth,masterL,masterR,nActive\n");
    }
    for (i = 0; i < SPU_VOICES; i++) {
        SpuVoice *v = &s_v[i];
        if (!v->active) continue;
        nActive++;
        /* sampleHz: step is 12-frac fixed point, 0x1000 = unity = the VAG's native rate.
         * Hardware's pitch register is the same scale, so these are directly comparable. */
        fprintf(fv, "%ld,%d,%d,%d,%d,%u,%.1f,%.4f,%.4f,%d,%d,%d,%d,%d,%d\n",
                blk, i, v->dbgVag, v->dbgProg, v->dbgNote,
                v->step, 44100.0 * (double)v->step / 4096.0,
                v->gainL, v->gainR, v->adsrLevel, v->adsrPhase,
                v->len, v->loopS, v->loopE, (v->loopE > v->loopS) ? 1 : 0);
    }
    fprintf(fg, "%ld,%d,%.4f,%.4f,%.4f,%d\n", blk, s_revOn, s_revDepth, s_masterL, s_masterR, nActive);
    fflush(fv); fflush(fg);
    blk++;
}

/* Render `frames` stereo samples (interleaved s16) by summing active voices. */
static void SpuRenderBlock(short *out, int frames) {
    int f, i;
    AnalogInit();
    SpuTrace();
    for (f = 0; f < frames; f++) {
        float accL = 0.0f, accR = 0.0f;
        float revSendL = 0.0f, revSendR = 0.0f;  /* only mode-bit2 voices (see SpuVoice.revOn) */
        for (i = 0; i < SPU_VOICES; i++) {
            SpuVoice *v = &s_v[i];
            float s, env;
            if (!v->active) continue;
            s = SpuVoiceSample(v);
            /* sample-accurate ADSR envelope (M3): level 0..0x7fff scales the sample. */
            if (!SpuAdsrAdvance(v)) { v->active = 0; continue; }
            env = (float)v->adsrLevel / (float)ADSR_MAX;
            accL += s * env * v->gainL;
            accR += s * env * v->gainR;
            if (v->revOn) { revSendL += s * env * v->gainL; revSendR += s * env * v->gainR; }
            SpuVoiceAdvance(v);
        }
        {
            int dryL = (int)accL, dryR = (int)accR;
            int wetL = 0, wetR = 0;
            float oL, oR;
            /* Only voices whose tone sets VagAtr.mode bit2 feed the reverb -- matching hardware's
             * per-voice EON. Still run RevProcess every sample (even with a zero send) so the
             * tail flushes during silence instead of freezing. */
            if (s_revOn) RevProcess((int)revSendL, (int)revSendR, &wetL, &wetR);
            oL = (float)(dryL + wetL); oR = (float)(dryR + wetR);
            if (s_analogOn) { oL = AnalogCh(oL, 0); oR = AnalogCh(oR, 1); }
            out[f * 2 + 0] = (short)Clamp16((int)(oL * s_masterL));
            out[f * 2 + 1] = (short)Clamp16((int)(oR * s_masterR));
        }
    }
}

/* ---- sink plumbing (mirror pc_xa.c) ---- */
static void SpuEnsureAl(void) {
    if (s_alReady) return;
    if (!alcGetCurrentContext()) return;      /* SsInit not up yet; retry next service */
    alGenSources(1, &s_source);
    alGenBuffers(SPU_NUM_BUFFERS, s_bufFree);
    s_bufFreeCount = SPU_NUM_BUFFERS;
    s_alReady = 1;
}

static void SpuRecycle(void) {
    ALint processed = 0;
    if (!s_alReady) return;
    /* Reclaim every finished buffer -- crucially INCLUDING after an underrun (a game-thread stall,
     * e.g. an attack or a CD load, drains the queue and the source goes STOPPED with all buffers
     * processed). The old "skip when not PLAYING" guard, copied from pc_xa, permanently stalled a
     * continuous stream: without reclaiming, nothing could be re-rendered and the music went silent
     * forever ("cuts and never catches up"). A fresh/INITIAL source reports 0 processed, so this is
     * safe there too. */
    alGetSourcei(s_source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0 && s_bufFreeCount < SPU_NUM_BUFFERS) {
        ALuint b = 0;
        alSourceUnqueueBuffers(s_source, 1, &b);
        s_bufFree[s_bufFreeCount++] = b;
    }
}

/* Advance the SEQ sequencer by n output samples, interleaved with rendering, so
 * note on/off timing is locked to the audio sample clock instead of the video
 * frame clock (removes frame-jitter / loop-point hiccups). Defined in libsnd.c. */
extern void PC_SeqAdvanceSamples(int n);

/* Per-VSync top-up: keep the queue full, rendering silence when idle so the
 * stream never underruns. Called from PC_SeqTick when VH_SWSPU is active.
 * Each block is preceded by advancing the sequencer by exactly that block's
 * worth of samples, so the audio IS the master clock. */
void PC_SpuService(void) {
    static short block[SPU_BLOCK_FRAMES * 2];
    SpuEnsureAl();
    if (!s_alReady) return;
    SpuRecycle();
    while (s_bufFreeCount > 0) {
        ALuint buf = s_bufFree[--s_bufFreeCount];
        PC_SeqAdvanceSamples(SPU_BLOCK_FRAMES);
        SpuRenderBlock(block, SPU_BLOCK_FRAMES);
        alBufferData(buf, AL_FORMAT_STEREO16, block,
                     SPU_BLOCK_FRAMES * 2 * (int)sizeof(short), SPU_RATE);
        alSourceQueueBuffers(s_source, 1, &buf);
    }
    /* (Re)start whenever the source isn't playing but the queue is primed -- covers both the
     * initial start and recovery from an underrun (the queue was just refilled above). */
    {
        ALint state = 0, queued = 0;
        alGetSourcei(s_source, AL_SOURCE_STATE, &state);
        if (state != AL_PLAYING) {
            alGetSourcei(s_source, AL_BUFFERS_QUEUED, &queued);
            if (queued >= 2) { alSourcePlay(s_source); s_started = 1; }
        }
    }
}
