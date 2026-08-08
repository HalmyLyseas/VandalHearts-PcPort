/*
 * CD-XA ADPCM streaming audio backend.
 *
 * Scope (validated 2026-07-15 against real hardware via exchange/45-xa-audio-track-map.lua):
 * the game uses XA for the intro-movie audio and for streamed sound effects -- notably the big
 * elemental spell SFX (Hurricane/Salamander/Avalanche/Plasma-Wave, XA files 17/18/19/22/23/33).
 * It does NOT use XA for background music: the loading/battle BGM is SEQ (sequenced music ->
 * SPU voices; battle_eval.c PlayBattleBGM -> AUDIO_CMD_PLAY_SEQ), a separate subsystem. There
 * are zero looping XA tracks in the game's demo, so this layer only ever streams finite one-shots.
 *
 * The game (src/audio.c) requests XA via PerformAudioCommand(PLAY_XA/PREPARE_XA), which drives a
 * CdlSetmode(RT)/CdlSetfilter(file,chan)/CdlSeekL state machine. On real hardware the CD
 * controller decodes the interleaved XA-ADPCM sectors and mixes them into the SPU's CD input at
 * a serial volume. Here we mirror that: libcd.c reads raw 2352-byte sectors from the .bin at the
 * stream cursor and hands each to PC_XaSubmitSector(); this module decodes the ones matching the
 * active {file,channel,audio} filter and streams the PCM through a dedicated OpenAL source.
 * OpenAL owns the realtime mixing/thread, so the feed is a cheap per-VSync top-up (PC_XaService)
 * with no locking -- see the ctr-native comparison (they hand-mix the SPU and need a callback
 * thread; we offload to OpenAL and don't).
 *
 * CD-XA sector layout (psx-spx cdromformat.md + the standard sound-group format):
 *   subheader[+16..]: file, channel, submode (bit2=0x04 audio), coding-info.
 *   coding-info: bit0-1 mono/stereo (1=stereo), bit2 rate (0=37800,1=18900),
 *                bit4-5 bits/sample (0=4bit,1=8bit).
 *   user data (offset +24): 2304 bytes = 18 sound groups x 128 bytes.
 *   each group: 16 param bytes + 112 data bytes = 28 data-words x 4 bytes.
 *     4-bit: 8 blocks/group (block b -> param group[4+b]; nibble = byte[16+i*4+(b>>1)] hi/lo).
 *     8-bit: 4 blocks/group (block b -> param group[4+b]; sample = byte[16+i*4+b]).
 *   per-block: shift=param&0xF, filter=(param>>4); s = (raw>>shift)+(old*f0+older*f1+32>>6).
 *   Filter pair table is the same public PS1 ADPCM set libsnd.c already uses.
 *   ADPCM prev-sample state is continuous across sectors (per channel) -> kept in module state.
 */
#include <stdio.h>
#include <string.h>
#if defined(__APPLE__)
#include <OpenAL/al.h>
#include <OpenAL/alc.h>
#else
#include <AL/al.h>
#include <AL/alc.h>
#endif

typedef unsigned char u8;
typedef short s16;

/* Same public PS1 ADPCM coefficient pair table as libsnd.c's SPU-ADPCM decoder. */
static const int XA_F0[5] = {0, 60, 115, 98, 122};
static const int XA_F1[5] = {0, 0, -52, -55, -60};

#define XA_SUBHEADER_OFS   16          /* file,chan,submode,coding (+ redundant copy)   */
#define XA_USERDATA_OFS    24          /* start of the 2304-byte audio payload          */
#define XA_GROUPS          18
#define XA_MAX_FRAMES      4032        /* mono 4-bit worst case: 18*8*28                */
#define XA_NUM_BUFFERS     24          /* OpenAL streaming buffer pool (~1.3s @ stereo) */

/* ---- decode state (persists across sectors within a stream) ---- */
static int s_prev1[2], s_prev2[2];     /* [channel] ADPCM history                       */

/* ---- OpenAL streaming ---- */
static ALuint s_source = 0;
static ALuint s_bufFree[XA_NUM_BUFFERS];
static int    s_bufFreeCount = 0;
static int    s_alReady = 0;
static float  s_gain = 0.8f;    /* default until the game sets serial volume (SsSetSerialVol) */
static int    s_streaming = 0;

static int clamp16(int v) { return v < -32768 ? -32768 : (v > 32767 ? 32767 : v); }

/* Decode one 28-sample block of a sound group into interleaved out[] (stride = channels),
 * carrying the per-channel ADPCM history. */
static void XaDecodeBlock(const u8 *group, int blk, int is8bit, int ch, s16 *out, int stride) {
    /* sound-parameter byte for unit `blk`: units 0-3 -> params[4..7], units 4-7 -> params[12..15]
     * (the 4-bit sound group's redundant-copy layout; verified vs octoshock cdc.cpp). */
    u8 param = group[4 | (blk & 3) | ((blk & 4) << 1)];
    int shift = param & 0x0F;
    int filter = (param >> 4) & 0x0F;
    if (filter > 4) filter = 4;         /* guard malformed data                          */
    if (shift > 12) shift = 9;          /* hardware: shift>12 behaves as 9               */
    int f0 = XA_F0[filter], f1 = XA_F1[filter];
    int p1 = s_prev1[ch], p2 = s_prev2[ch];
    int i;
    for (i = 0; i < 28; i++) {
        int raw;
        if (is8bit) {
            raw = (int)(signed char)group[16 + i * 4 + blk] << 8;
        } else {
            u8 b = group[16 + i * 4 + (blk >> 1)];
            int nib = (blk & 1) ? (b >> 4) : (b & 0x0F);
            raw = (int)(s16)(nib << 12);          /* sign-extend 4-bit at top            */
        }
        int s = (raw >> shift) + ((p1 * f0 + p2 * f1 + 32) >> 6);
        s = clamp16(s);
        p2 = p1; p1 = s;
        out[i * stride] = (s16)s;
    }
    s_prev1[ch] = p1; s_prev2[ch] = p2;
}

/* Decode a full XA-audio sector's payload -> interleaved 16-bit PCM.
 * Returns frames-per-channel; sets *stereo,*rate. out must hold XA_MAX_FRAMES*2 shorts. */
static int XaDecodeSector(const u8 *userdata, u8 coding, s16 *out, int *stereo, int *rate) {
    int st = (coding & 0x03) == 1;
    int r  = (coding & 0x04) ? 18900 : 37800;
    int is8bit = (coding & 0x30) == 0x10;
    int chans = st ? 2 : 1;
    int blocks = is8bit ? 4 : 8;                  /* blocks per group                    */
    int runsPerCh = blocks / chans;               /* 28-sample runs per channel per group */
    int framesPerGroup = runsPerCh * 28;
    int g, blk;
    for (g = 0; g < XA_GROUPS; g++) {
        const u8 *group = userdata + g * 128;
        for (blk = 0; blk < blocks; blk++) {
            int ch = st ? (blk & 1) : 0;
            int run = st ? (blk >> 1) : blk;       /* which 28-run within this channel    */
            s16 *dst = out + (g * framesPerGroup + run * 28) * chans + ch;
            XaDecodeBlock(group, blk, is8bit, ch, dst, chans);
        }
    }
    *stereo = st; *rate = r;
    return XA_GROUPS * framesPerGroup;             /* frames per channel                  */
}

/* ---- OpenAL streaming plumbing ---- */

static void XaEnsureAl(void) {
    if (s_alReady) return;
    /* SsInit()/libsnd made an OpenAL context current already; create our source in it. */
    if (!alcGetCurrentContext()) return;           /* audio not up yet; try again later   */
    alGenSources(1, &s_source);
    alGenBuffers(XA_NUM_BUFFERS, s_bufFree);
    s_bufFreeCount = XA_NUM_BUFFERS;
    alSourcef(s_source, AL_GAIN, s_gain);
    s_alReady = 1;
}

/* Recycle any OpenAL buffers the source has finished playing.
 * IMPORTANT: only while the source is actually PLAYING. On a STOPPED/INITIAL source
 * AL_BUFFERS_PROCESSED is unreliable -- a source that already stopped (e.g. the previous
 * stream underran to its end) reports even freshly-queued buffers as "processed", so
 * reclaiming here would instantly drain the queue we're trying to build up and the source
 * could never be (re)started. Deferring reclaim until playback resumes is safe: those
 * buffers are still queued and get reclaimed on the next PLAYING pass. Intentional teardown
 * (PC_XaReset) does its own explicit unqueue after alSourceStop. */
static void XaRecycle(void) {
    if (!s_alReady) return;
    ALint state = 0;
    alGetSourcei(s_source, AL_SOURCE_STATE, &state);
    if (state != AL_PLAYING) return;
    ALint processed = 0;
    alGetSourcei(s_source, AL_BUFFERS_PROCESSED, &processed);
    while (processed-- > 0 && s_bufFreeCount < XA_NUM_BUFFERS) {
        ALuint b = 0;
        alSourceUnqueueBuffers(s_source, 1, &b);
        s_bufFree[s_bufFreeCount++] = b;
    }
}

/* -------- public API (called by libcd.c) -------- */

void PC_XaReset(void) {
    if (s_alReady) {
        /* Full teardown: stop marks every queued buffer processed, so reclaim them all
         * directly (XaRecycle now no-ops on a stopped source -- see its note). */
        alSourceStop(s_source);
        ALint queued = 0;
        alGetSourcei(s_source, AL_BUFFERS_QUEUED, &queued);
        while (queued-- > 0 && s_bufFreeCount < XA_NUM_BUFFERS) {
            ALuint b = 0;
            alSourceUnqueueBuffers(s_source, 1, &b);
            s_bufFree[s_bufFreeCount++] = b;
        }
    }
    s_prev1[0] = s_prev1[1] = s_prev2[0] = s_prev2[1] = 0;
    s_streaming = 0;
}

void PC_XaSetVolume(int left, int right) {
    /* CD serial volume 0..127 per side -> average -> OpenAL gain 0..1. */
    int v = (left + right) / 2;
    if (v < 0) { v = 0; } if (v > 127) { v = 127; }
    s_gain = (float)v / 127.0f;
    if (s_alReady) alSourcef(s_source, AL_GAIN, s_gain);
}

/* Feed one raw 2352-byte sector. Decodes+queues it iff it is an audio sector matching the
 * wanted {file,chan}. Returns 1 if audio was queued, else 0. */
int PC_XaSubmitSector(const u8 *raw, int wantFile, int wantChan) {
    u8 file = raw[XA_SUBHEADER_OFS + 0];
    u8 chan = raw[XA_SUBHEADER_OFS + 1];
    u8 submode = raw[XA_SUBHEADER_OFS + 2];
    u8 coding = raw[XA_SUBHEADER_OFS + 3];
    if (!(submode & 0x04)) return 0;               /* not an audio sector                 */
    if (wantFile >= 0 && file != (u8)wantFile) return 0;
    if (wantChan >= 0 && chan != (u8)wantChan) return 0;

    XaEnsureAl();
    if (!s_alReady) return 0;
    XaRecycle();
    if (s_bufFreeCount == 0) return 0;             /* queue full; drop (shouldn't happen)  */

    static s16 pcm[XA_MAX_FRAMES * 2];
    int stereo, rate;
    int frames = XaDecodeSector(raw + XA_USERDATA_OFS, coding, pcm, &stereo, &rate);
    if (frames <= 0) return 0;

    ALuint buf = s_bufFree[--s_bufFreeCount];
    alBufferData(buf, stereo ? AL_FORMAT_STEREO16 : AL_FORMAT_MONO16,
                 pcm, frames * (stereo ? 2 : 1) * (int)sizeof(s16), rate);
    alSourceQueueBuffers(s_source, 1, &buf);
    s_streaming = 1;
    return 1;
}

/* Buffers currently in-flight (queued + playing). The pump uses this for flow control:
 * only read/decode more sectors while this is below a target depth, so we never drop audio. */
int PC_XaQueuedBuffers(void) {
    if (!s_alReady) return 0;
    XaRecycle();
    return XA_NUM_BUFFERS - s_bufFreeCount;
}

/* Called by the pump when the track's interleaved data has genuinely ENDED (miss-limit / disc end),
 * as opposed to a momentary feed underrun. Stops PC_XaService from auto-restarting the source, so it
 * plays out whatever real buffers remain ONCE and then stays stopped -- instead of re-issuing
 * alSourcePlay on the drained source, which replays the last stale processed buffer over and over
 * (the ~6 short "afterhit" blips heard after each short spell-hit clip). This does NOT flush: the
 * clip's already-buffered tail finishes naturally; the buffers are reclaimed at the next PC_XaReset. */
void PC_XaEndStream(void) {
    s_streaming = 0;
}

/* Debug: OpenAL source state -> 0 stopped/initial, 1 playing, 2 paused. (XA runtime instrumentation
 * for diffing our actual audio output against the real-hardware xa-track-map CSV.) */
int PC_XaSourceState(void) {
    if (!s_alReady) return 0;
    ALint st = 0;
    alGetSourcei(s_source, AL_SOURCE_STATE, &st);
    return st == AL_PLAYING ? 1 : (st == AL_PAUSED ? 2 : 0);
}

/* VSync-driven service: recycle finished buffers and keep the source playing.
 * The source drops to AL_STOPPED whenever the queue drains (a finite stream ending, or a brief
 * feed gap); re-issuing alSourcePlay once buffers are queued again resumes it. */
void PC_XaService(void) {
    if (!s_alReady) return;
    XaRecycle();
    if (!s_streaming) return;
    ALint queued = 0, state = 0;
    alGetSourcei(s_source, AL_BUFFERS_QUEUED, &queued);
    alGetSourcei(s_source, AL_SOURCE_STATE, &state);
    if (queued > 0 && state != AL_PLAYING) {
        alSourcePlay(s_source);                    /* (re)start after a fill / underrun    */
    }
}
