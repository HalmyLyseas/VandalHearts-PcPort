/* Regression fixtures for the three libsnd.c/pc_spu.c audio-lifecycle-and-bounds fixes
 * (SsVabClose voice stop, VAB size-table body bound, SEQ parser bound). Compiled and run
 * under AddressSanitizer by audio_bounds.sh; not part of the real game build.
 *
 * This links only src/libsnd.c + src/pc_spu.c + src/libspu.c + this file (see
 * src/test_libsnd.c's header comment for the sibling harness's linking approach). It does
 * NOT link core/cd.c or the generated data segment, so every extern libsnd.c reaches for in
 * those TUs (gCdFiles, gVabLoader, gSeqData, PC_GenSize_gSeqData, SDL_GetTicks) is stubbed
 * below with the same layout libsnd.c expects.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "cd_files.h"     /* CdFileInfo -- real layout, so our gCdFiles stub matches libsnd.c's use */
#include "PsyQ/libsnd.h"

/* ---- stubs for externs libsnd.c normally gets from core/cd.c and the generated data segment ---- */
CdFileInfo gCdFiles[712];
struct {
    int state, vabId, headerCdf, bodyCdf, bodyTransferResult;
} gVabLoader;
unsigned char gSeqData[256];
const unsigned int PC_GenSize_gSeqData = sizeof(gSeqData);
unsigned int SDL_GetTicks(void) { return 0; }
short gSeqSetIdx = 0;
unsigned char gSeqCurrentID = 0;
void PC_XaSetVolume(int l, int r) { (void)l; (void)r; }

/* ---- pc_spu.c entry points libsnd.c calls directly (declared, not exported via a header) ---- */
extern int  PC_SpuVoiceActive(int voice);
extern void PC_SpuService(void);
extern void PC_SeqAdvanceSamples(int n);

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "  FAIL: %s\n", msg); g_fail = 1; } \
} while (0)

static void PutU16LE(unsigned char *p, unsigned short v) { p[0] = v & 0xff; p[1] = (v >> 8) & 0xff; }

/* Minimal one-program/one-tone VAB header: program 0 owns tone 0, which points at VAG index 1
 * (index 0 is the reserved slot) with size-table entry `vagSizeUnits` (8-byte SPU units).
 * `hdrLen` must be at least 2592 + 4: program table + one tone block + a 2-entry size table. */
static void BuildVabHeader(unsigned char *hdr, int hdrLen, unsigned short vagSizeUnits) {
    unsigned char *prog0, *tone0, *sizeTable;
    memset(hdr, 0, (size_t)hdrLen);
    memcpy(hdr, "pBAV", 4);
    PutU16LE(hdr + 18, 1);      /* numPrograms */
    PutU16LE(hdr + 22, 1);      /* numVag: valid indices are [0,1] */
    hdr[24] = 127;              /* VabHdr.mvol */
    prog0 = hdr + 32;
    prog0[0] = 1;                /* nTone: program 0 owns one tone block */
    prog0[1] = 127;               /* ProgAtr.mvol */
    prog0[4] = 64;                /* ProgAtr.mpan (centre) */
    tone0 = hdr + 32 + 128 * 16;  /* fixed 128-slot program table, then the tone table */
    tone0[1] = 0;                  /* mode: dry (no reverb send) */
    tone0[2] = 127;                /* vol */
    tone0[3] = 64;                  /* pan (centre) */
    tone0[4] = (unsigned char)60;   /* centre note */
    tone0[5] = 0;                    /* shift */
    tone0[6] = 0; tone0[7] = 127;     /* key range: whole keyboard */
    PutU16LE(tone0 + 16, 0);           /* adsr1 */
    PutU16LE(tone0 + 18, 0);            /* adsr2 */
    PutU16LE(tone0 + 22, 1);             /* vag index */
    sizeTable = tone0 + 16 * 32;          /* one tone block (16 slots) -- see nBlocks in the header parser */
    PutU16LE(sizeTable + 0, 0);            /* index 0: reserved/dummy */
    PutU16LE(sizeTable + 2, vagSizeUnits); /* index 1: our one real VAG */
}

/* Fixture 1 (finding 2.1): a voice keyed on from a bank must be hard-stopped before
 * SsVabClose frees its PCM -- else PC_SpuService renders a dangling short* (UAF). */
static void Fixture1_UafOnClose(void) {
    unsigned char hdr[2600];
    unsigned char body[2048];
    short vabId;
    printf("fixture 1: SsVabClose stops live voices before freeing PCM\n");
    BuildVabHeader(hdr, sizeof(hdr), 8 /* 8*8 = 64 bytes = 4 ADPCM blocks */);
    memset(body, 0, sizeof(body)); /* all-zero ADPCM blocks decode to silence, no crash */
    gVabLoader.bodyCdf = 0;
    gCdFiles[0].startingSector = 0;
    gCdFiles[0].sectorCt = 1;      /* 1 sector = 2048 bytes = the whole synthetic body */
    gCdFiles[0].bufferPtr = NULL;
    vabId = SsVabOpenHeadSticky(hdr, 0, 0);
    CHECK(vabId == 0, "SsVabOpenHeadSticky(vab 0) failed");
    CHECK(SsVabTransBodyPartly(body, sizeof(body), vabId) == 1, "VAB body transfer did not complete");
    SsUtKeyOnV(0, vabId, 0, 0, 60, 0, 127, 127);
    CHECK(PC_SpuVoiceActive(0) != 0, "voice 0 did not key on");
    SsVabClose(vabId);
    CHECK(PC_SpuVoiceActive(0) == 0, "voice 0 still active after SsVabClose (would UAF on the next render)");
    PC_SpuService(); /* the render pass that used to touch freed PCM -- ASan is the real judge here */
    printf(g_fail ? "fixture 1: FAIL\n" : "fixture 1: PASS\n");
}

/* Fixture 2 (finding 2.2): a size-table entry the staged body can't back must stop that
 * bank's decode instead of reading past the staging buffer. */
static void Fixture2_OversizedVagSize(void) {
    unsigned char hdr[2600];
    unsigned char body[4096];
    short vabId;
    printf("fixture 2: oversized VAG size-table entry stops decode at the body bound\n");
    BuildVabHeader(hdr, sizeof(hdr), 0xffff); /* 0xffff*8 = 524280 bytes, way past the 4 KiB body */
    memset(body, 0, sizeof(body));
    gVabLoader.bodyCdf = 0;
    gCdFiles[0].startingSector = 0;
    gCdFiles[0].sectorCt = 2;      /* 2 sectors = 4096 bytes */
    gCdFiles[0].bufferPtr = NULL;
    vabId = SsVabOpenHeadSticky(hdr, 1, 0);
    CHECK(vabId == 1, "SsVabOpenHeadSticky(vab 1) failed");
    /* Must return without ASan reporting a heap-buffer-overflow read; the guard's own
     * fprintf (grepped for by audio_bounds.sh) is the confirmation it actually fired. */
    SsVabTransBodyPartly(body, sizeof(body), vabId);
    SsVabClose(vabId);
    printf(g_fail ? "fixture 2: FAIL\n" : "fixture 2: PASS\n");
}

/* Fixture 3 (finding 2.3): a SEQ blob living inside gSeqData with no FF 2F terminator must
 * stop at gSeqData's real (generator-reported) size, not walk SEQ_MAX_BYTES past it. */
static void Fixture3_SeqNoTerminator(void) {
    short seqSlot;
    int i;
    static const unsigned char unit[4] = { 0x01, 0x90, 0x3c, 0x40 }; /* delta=1, note-on ch0 */
    printf("fixture 3: SEQ with no FF 2F stays inside gSeqData's real bound\n");
    memcpy(gSeqData, "pQES", 4);        /* magic */
    memset(gSeqData + 4, 0, 4);         /* version (unused) */
    gSeqData[8] = 0x01; gSeqData[9] = 0xe0;              /* ppqn = 480, BE */
    gSeqData[10] = 0x07; gSeqData[11] = 0xa1; gSeqData[12] = 0x20; /* tempo = 500000us, BE */
    gSeqData[13] = 0; gSeqData[14] = 0;                   /* rhythm (unused) */
    for (i = 15; i < (int)sizeof(gSeqData); i++) gSeqData[i] = unit[(i - 15) % 4];
    seqSlot = SsSeqOpen((unsigned int *)gSeqData, -1); /* no VAB bound -- SeqNoteOn no-ops harmlessly */
    CHECK(seqSlot >= 0, "SsSeqOpen failed");
    SsSeqPlay(seqSlot, SSPLAY_PLAY, 1); /* single pass, no looping */
    /* One generous tick well past every delta in the blob (60 events * 1 tick each); the
     * watchdogs inside SeqAdvanceByUsec/SeqProcessEvent are what must keep this bounded, not
     * this loop count. ASan is the real judge: any OOB read aborts the process. */
    for (i = 0; i < 4; i++) PC_SeqAdvanceSamples(20000);
    SsSeqStop(seqSlot);
    SsSeqClose(seqSlot);
    printf(g_fail ? "fixture 3: FAIL\n" : "fixture 3: PASS\n");
}

int main(void) {
    SsInit();
    Fixture1_UafOnClose();
    Fixture2_OversizedVagSize();
    Fixture3_SeqNoTerminator();
    SsQuit();
    return g_fail;
}
