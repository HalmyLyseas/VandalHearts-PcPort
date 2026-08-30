/* PC backend for the PS1 GTE: one software model of the coprocessor's registers and opcodes
 * (psx-spx formulas, saturation rules, UNR divide), entered both by the raw gte_* macros of
 * inline_gte.h and by the PsyQ/libgte.h SDK wrappers. See docs/pc-port/subsystems/gte.md. */
#include <math.h>
#include <string.h>
#include <stdlib.h>
#include <stdio.h>
#include <stdint.h>

#include "PsyQ/libgte.h"

#ifndef M_PI
#define M_PI 3.14159265358979323846
#endif

/* ---- saturation helpers ------------------------------------------------ */

static int Sat16(int v, int lm) {
    int lo = lm ? 0 : -32768;
    if (v > 32767) return 32767;
    if (v < lo) return lo;
    return v;
}

static unsigned int SatU16(int v) {
    if (v < 0) return 0;
    if (v > 65535) return 65535;
    return (unsigned int)v;
}

/* ---- GTE register state (single hardware instance) --------------------- */

typedef struct { short vx, vy, vz, pad; } PcSVector;
typedef struct { int vx, vy, vz, pad; } PcVector;
typedef struct { unsigned char r, g, b, c; } PcColor;

static struct {
    /* control registers */
    MATRIX rt;        /* RT11..RT33 */
    int   tr[3];      /* TRX,TRY,TRZ */
    MATRIX light;      /* L11..L33 */
    int   bk[3];       /* RBK,GBK,BBK */
    MATRIX colorMat;   /* LR1..LB3 */
    int   fc[3];       /* RFC,GFC,BFC */
    int   ofx, ofy;
    short  h;
    short  dqa;
    int   dqb;
    short  zsf3, zsf4;

    /* PushMatrix/PopMatrix software stack (the GTE has none). The game nests pushes -- a camera
     * push with per-object pushes inside -- so a single slot is silently clobbered; 16 is a scene-
     * graph bound, not a hardware number. See docs/pc-port/subsystems/gte.md, "Gotchas / notes". */
    MATRIX savedRt[16];
    int   savedTr[16][3];
    int    savedDepth;

    /* scratch for gte_ldopv1's "D1,D2,D3" (RT diagonal misused as a vector,
     * see PC_GTE_LoadOPV1) -- kept separate from savedRt/savedTr above so
     * OP0 can never collide with real PushMatrix/PopMatrix state. */
    int opvD[3];

    /* data registers */
    PcSVector v0, v1, v2;
    PcColor   rgbc;
    unsigned short otz;
    short ir0, ir1, ir2, ir3;
    short sxy0[2], sxy1[2], sxy2[2]; /* 3-stage FIFO, [x,y] */
    unsigned short sz0, sz1, sz2, sz3; /* 4-stage FIFO */
    PcColor rgbFifo0, rgbFifo1, rgbFifo2;
    int mac0, mac1, mac2, mac3;
    unsigned int flag;
} g;

/* ---- UNR hardware division (psx-spx "GTE Division Inaccuracy") --------- */

static unsigned short s_unrTable[257];
static int s_unrTableInit = 0;

static void BuildUnrTable(void) {
    int i;
    for (i = 0; i <= 0x100; i++) {
        int v = 0x40000L / (i + 0x100) + 1;
        v = v / 2 - 0x101;
        if (v < 0) v = 0;
        s_unrTable[i] = (unsigned short)v;
    }
    s_unrTableInit = 1;
}

static int CountLeadingZeroes16(unsigned int sz) {
    int z = 0;
    if (sz == 0) return 16;
    while (!(sz & 0x8000)) {
        sz <<= 1;
        z++;
    }
    return z;
}

/* Returns the UNR division result n = H/SZ3 (scaled), sets FLAG bits on overflow. */
static unsigned int UnrDivide(unsigned short H, unsigned short SZ3) {
    int z;
    unsigned int n, d, u;

    if (!s_unrTableInit) BuildUnrTable();

    if (SZ3 == 0 || H >= (unsigned int)SZ3 * 2) {
        g.flag |= (1UL << 17) | (1UL << 31);
        return 0x1FFFF;
    }

    z = CountLeadingZeroes16(SZ3);
    n = ((unsigned int)H) << z;
    d = ((unsigned int)SZ3) << z;
    u = s_unrTable[(d - 0x7FC0) >> 7] + 0x101;
    d = (0x2000080UL - (d * u)) >> 8;
    d = (0x80UL + (d * u)) >> 8;
    /* 64-bit product: n*d exceeds 32 bits whenever the quotient exceeds 0x10000 (SZ3 < H), and a
     * wrapped n projects all near geometry onto (OFX,OFY). The GTE divides at full width.
     * See docs/pc-port/subsystems/gte.md, "Two fixed-point bugs worth knowing". */
    n = (unsigned int)((((unsigned long long)n * d) + 0x8000) >> 16);
    if (n > 0x1FFFF) n = 0x1FFFF;
    return n;
}

/* Projection diagnostic ring: the last 8 TransformOne results, read back via PC_GteProjEntry. */
static struct { int sx, sy, ir1, ir2, ir3, sz3, n; } s_projRing[8];
static int s_projRingHead;

/* ---- core transform (shared by RTPS/RTPT and the high-level wrappers) -- */

/* Rotate+translate one vector by the resident RT/TR registers (sf=1), push
 * the SZ/SXY FIFOs, and compute the perspective-projected screen X/Y and
 * IR0 depth-cue value. Matches the RTPS formula exactly. */
static void TransformOne(short vx, short vy, short vz) {
    int mac1, mac2, mac3;
    unsigned int n;

    mac1 = (int)(((int64_t)g.tr[0] * 4096 + g.rt.m[0][0] * vx + g.rt.m[0][1] * vy + g.rt.m[0][2] * vz) >> 12);
    mac2 = (int)(((int64_t)g.tr[1] * 4096 + g.rt.m[1][0] * vx + g.rt.m[1][1] * vy + g.rt.m[1][2] * vz) >> 12);
    mac3 = (int)(((int64_t)g.tr[2] * 4096 + g.rt.m[2][0] * vx + g.rt.m[2][1] * vy + g.rt.m[2][2] * vz) >> 12);

    g.mac1 = mac1; g.mac2 = mac2; g.mac3 = mac3;
    g.ir1 = (short)Sat16(mac1, 0);
    g.ir2 = (short)Sat16(mac2, 0);
    g.ir3 = (short)Sat16(mac3, 0);

    g.sz0 = g.sz1; g.sz1 = g.sz2; g.sz2 = g.sz3;
    g.sz3 = (unsigned short)SatU16(mac3);

    n = UnrDivide((unsigned short)g.h, g.sz3);

    /* psx-spx: MAC0 = (((H*20000h/SZ3)+1)/2)*IR1+OFX is a SIGNED multiply. IR1/IR2/DQA are
     * signed 16-bit (negative for any point left of/above the projection centre); an unsigned
     * widen turns -166 into 65370 and throws the vertex tens of thousands of pixels off-screen. */
    g.mac0 = (int)n * (int)g.ir1 + g.ofx;
    {
        int sx = g.mac0 >> 16;
        int sy;
        g.mac0 = (int)n * (int)g.ir2 + g.ofy;
        sy = g.mac0 >> 16;

        /* Hardware saturates SX2/SY2 to -0x400..+0x3FF (psx-spx RTPS, FLAG.14/13). Near-plane
         * geometry (n = H/SZ3 grows as SZ3 -> 0) projects to huge coords that clamp to the screen
         * edge; a bare (short) cast would wrap them (+40000 -> -25536) and tear quads. */
        if (sx < -0x400) { sx = -0x400; g.flag |= 1u << 14; }
        else if (sx > 0x3FF) { sx = 0x3FF; g.flag |= 1u << 14; }
        if (sy < -0x400) { sy = -0x400; g.flag |= 1u << 13; }
        else if (sy > 0x3FF) { sy = 0x3FF; g.flag |= 1u << 13; }

        g.sxy0[0] = g.sxy1[0]; g.sxy0[1] = g.sxy1[1];
        g.sxy1[0] = g.sxy2[0]; g.sxy1[1] = g.sxy2[1];
        g.sxy2[0] = (short)sx; g.sxy2[1] = (short)sy;

        /* Record result + inputs so pc_diag.c can dump a tile's 4 corners and break the spread
         * SX-OFX = H*IR1/SZ3 down term by term. */
        s_projRing[s_projRingHead & 7].sx = sx;
        s_projRing[s_projRingHead & 7].sy = sy;
        s_projRing[s_projRingHead & 7].ir1 = g.ir1;
        s_projRing[s_projRingHead & 7].ir2 = g.ir2;
        s_projRing[s_projRingHead & 7].ir3 = g.ir3;
        s_projRing[s_projRingHead & 7].sz3 = g.sz3;
        s_projRing[s_projRingHead & 7].n = (int)n;
        s_projRingHead++;
    }

    g.mac0 = (int)n * (int)g.dqa + g.dqb;
    g.ir0 = (short)Sat16(g.mac0 >> 12, 1);
}

/* Read a past TransformOne's projection (back=0 most recent .. back=7). Used by the terrain
 * projection diagnostic to recover a tile's 4 corners (v0,v1,v2 from RTPT then v3 from RTPS =
 * back 3,2,1,0). */
void PC_GteProjEntry(int back, int *sx, int *sy, int *ir1, int *ir2, int *ir3, int *sz3, int *nout) {
    int i = (s_projRingHead - 1 - back) & 7;
    if (sx) *sx = s_projRing[i].sx;
    if (sy) *sy = s_projRing[i].sy;
    if (ir1) *ir1 = s_projRing[i].ir1;
    if (ir2) *ir2 = s_projRing[i].ir2;
    if (ir3) *ir3 = s_projRing[i].ir3;
    if (sz3) *sz3 = s_projRing[i].sz3;
    if (nout) *nout = s_projRing[i].n;
}

/* ---- raw coprocessor primitives (called from inline_gte.h macros) ------ */

void PC_GTE_LoadV0(void *v) {
    PcSVector *p = (PcSVector *)v;
    g.v0 = *p;
}

void PC_GTE_LoadV3(void *v0, void *v1, void *v2) {
    g.v0 = *(PcSVector *)v0;
    g.v1 = *(PcSVector *)v1;
    g.v2 = *(PcSVector *)v2;
}

void PC_GTE_LoadRGB(void *rgbc) {
    g.rgbc = *(PcColor *)rgbc;
}

void PC_GTE_LoadOPV1(void *v) {
    /* Hardware ctc2's the 3 words into control regs 0/2/4 (RT11/RT13/RT22), using the RT diagonal
     * as a vector (psx-spx OP). Model the functional effect (D1..D3 feeding OP0) instead of the
     * bit-packing: nothing reads RT between this and gte_op0(). */
    int *src = (int *)v;
    g.opvD[0] = src[0];
    g.opvD[1] = src[1];
    g.opvD[2] = src[2];
}

void PC_GTE_LoadOPV2(void *v) {
    int *src = (int *)v;
    g.ir1 = (short)src[0];
    g.ir2 = (short)src[1];
    g.ir3 = (short)src[2];
}

void PC_GTE_RTPS(void) {
    g.flag = 0;
    TransformOne(g.v0.vx, g.v0.vy, g.v0.vz);
}

void PC_GTE_RTPT(void) {
    g.flag = 0;
    TransformOne(g.v0.vx, g.v0.vy, g.v0.vz);
    TransformOne(g.v1.vx, g.v1.vy, g.v1.vz);
    TransformOne(g.v2.vx, g.v2.vy, g.v2.vz);
}

void PC_GTE_NCLIP(void) {
    int sx0 = g.sxy0[0], sy0 = g.sxy0[1];
    int sx1 = g.sxy1[0], sy1 = g.sxy1[1];
    int sx2 = g.sxy2[0], sy2 = g.sxy2[1];
    g.mac0 = sx0 * sy1 + sx1 * sy2 + sx2 * sy0 - sx0 * sy2 - sx1 * sy0 - sx2 * sy1;
}

void PC_GTE_AVSZ4(void) {
    g.mac0 = (int)g.zsf4 * (int)(g.sz0 + g.sz1 + g.sz2 + g.sz3);
    g.otz = (unsigned short)SatU16(g.mac0 >> 12);
}

void PC_GTE_OP0(void) {
    int d1 = g.opvD[0], d2 = g.opvD[1], d3 = g.opvD[2];
    int ir1 = g.ir1, ir2 = g.ir2, ir3 = g.ir3;

    g.mac1 = ir3 * d2 - ir2 * d3;
    g.mac2 = ir1 * d3 - ir3 * d1;
    g.mac3 = ir2 * d1 - ir1 * d2;
    g.ir1 = (short)Sat16(g.mac1, 0);
    g.ir2 = (short)Sat16(g.mac2, 0);
    g.ir3 = (short)Sat16(g.mac3, 0);
}

void PC_GTE_NCCS(void) {
    int m1, m2, m3;

    m1 = (g.light.m[0][0] * g.v0.vx + g.light.m[0][1] * g.v0.vy + g.light.m[0][2] * g.v0.vz) >> 12;
    m2 = (g.light.m[1][0] * g.v0.vx + g.light.m[1][1] * g.v0.vy + g.light.m[1][2] * g.v0.vz) >> 12;
    m3 = (g.light.m[2][0] * g.v0.vx + g.light.m[2][1] * g.v0.vy + g.light.m[2][2] * g.v0.vz) >> 12;
    g.ir1 = (short)Sat16(m1, 0); g.ir2 = (short)Sat16(m2, 0); g.ir3 = (short)Sat16(m3, 0);

    m1 = (int)(((int64_t)g.bk[0] * 4096 + g.colorMat.m[0][0] * g.ir1 + g.colorMat.m[0][1] * g.ir2 + g.colorMat.m[0][2] * g.ir3) >> 12);
    m2 = (int)(((int64_t)g.bk[1] * 4096 + g.colorMat.m[1][0] * g.ir1 + g.colorMat.m[1][1] * g.ir2 + g.colorMat.m[1][2] * g.ir3) >> 12);
    m3 = (int)(((int64_t)g.bk[2] * 4096 + g.colorMat.m[2][0] * g.ir1 + g.colorMat.m[2][1] * g.ir2 + g.colorMat.m[2][2] * g.ir3) >> 12);
    g.ir1 = (short)Sat16(m1, 0); g.ir2 = (short)Sat16(m2, 0); g.ir3 = (short)Sat16(m3, 0);

    m1 = ((int)g.rgbc.r * g.ir1) << 4;
    m2 = ((int)g.rgbc.g * g.ir2) << 4;
    m3 = ((int)g.rgbc.b * g.ir3) << 4;
    m1 >>= 12; m2 >>= 12; m3 >>= 12;

    g.rgbFifo0 = g.rgbFifo1;
    g.rgbFifo1 = g.rgbFifo2;
    {
        int r = m1 / 16, gg = m2 / 16, b = m3 / 16;
        if (r < 0) r = 0;
        if (r > 255) r = 255;
        if (gg < 0) gg = 0;
        if (gg > 255) gg = 255;
        if (b < 0) b = 0;
        if (b > 255) b = 255;
        g.rgbFifo2.r = (unsigned char)r;
        g.rgbFifo2.g = (unsigned char)gg;
        g.rgbFifo2.b = (unsigned char)b;
        g.rgbFifo2.c = g.rgbc.c;
    }
    g.mac1 = m1; g.mac2 = m2; g.mac3 = m3;
    g.ir1 = (short)Sat16(m1, 0); g.ir2 = (short)Sat16(m2, 0); g.ir3 = (short)Sat16(m3, 0);
}

void PC_GTE_StoreSXY(void *out) {
    short *o = (short *)out;
    o[0] = g.sxy2[0]; o[1] = g.sxy2[1];
}

void PC_GTE_StoreSXY3(void *out0, void *out1, void *out2) {
    ((short *)out0)[0] = g.sxy0[0]; ((short *)out0)[1] = g.sxy0[1];
    ((short *)out1)[0] = g.sxy1[0]; ((short *)out1)[1] = g.sxy1[1];
    ((short *)out2)[0] = g.sxy2[0]; ((short *)out2)[1] = g.sxy2[1];
}

void PC_GTE_StoreOTZ(void *out) {
    /* gte_stotz is a raw SWC2: always a full 32-bit store. OTZ (cop2r7) is a saturated u16
     * zero-extended into its register, so hardware clears the top half of the caller's `int otz`;
     * a 16-bit store leaves stack garbage there and can push AddPrim's OT index out of bounds. */
    *(unsigned int *)out = (unsigned int)g.otz;
}

void PC_GTE_StoreOPZ(void *out) {
    *(int *)out = g.mac0;
}

void PC_GTE_StoreLVNL(void *out) {
    int *o = (int *)out;
    o[0] = g.mac1; o[1] = g.mac2; o[2] = g.mac3;
}

void PC_GTE_StoreRGB(void *out) {
    *(PcColor *)out = g.rgbFifo2;
}

/* ---- high-level SDK wrappers (PsyQ/libgte.h) ---------------------------- */

void InitGeom(void) {
    memset(&g, 0, sizeof(g));
    g.rt.m[0][0] = g.rt.m[1][1] = g.rt.m[2][2] = ONE;
    g.light.m[0][0] = g.light.m[1][1] = g.light.m[2][2] = ONE;
    g.colorMat.m[0][0] = g.colorMat.m[1][1] = g.colorMat.m[2][2] = ONE;
    /* Control-register defaults from the PsyQ InitGeom disassembled out of SLUS_004.47 (0x800d04a8:
     * `li t0,imm; ctc2 t0,$reg`): OTZ = ZSF * sum(SZ) >> 12, IR0 from DQA/DQB, H is overridden by
     * SetGeomScreen. See docs/pc-port/subsystems/gte.md, "Recovering the real constants". */
    g.zsf3 = 0x0155;
    g.zsf4 = 0x0100;
    g.h    = 1000;
    g.dqa  = -4194;
    g.dqb  = 0x1400000;
}

/* OFX/OFY are 1.15.16 fixed-point (TransformOne adds them at that scale before the >>16), so the
 * pixel arguments are shifted <<16; stored raw they put the projection centre at (0,0). */
void SetGeomOffset(int ofx, int ofy) { g.ofx = ofx << 16; g.ofy = ofy << 16; }
void SetGeomScreen(int h) { g.h = (short)h; }

/* Diagnostic: the projection state used by the last TransformOne, read by pc_diag.c's sprite log
 * to tell a wrong geom offset (ofx/ofy/h) from a stale facing matrix (rt = scaled identity
 * instead of the rotated camera matrix). */
void PC_GteDebugState(int *ofx, int *ofy, int *h, int *rt00, int *rt02, int *rt22,
                      int *trx, int *trz) {
    *ofx = g.ofx; *ofy = g.ofy; *h = g.h;
    *rt00 = g.rt.m[0][0]; *rt02 = g.rt.m[0][2]; *rt22 = g.rt.m[2][2];
    *trx = g.tr[0]; *trz = g.tr[2];
}

/* Diagnostic accessors for pc_diag.c: the last AVSZ4 result (terrain OTZ) and the ZSF4 scale. */
int PC_GteLastOtz(void) { return g.otz; }
int PC_GteZsf4(void) { return g.zsf4; }

void SetRotMatrix(MATRIX *m) { g.rt = *m; }
void SetTransMatrix(MATRIX *m) { g.tr[0] = m->t[0]; g.tr[1] = m->t[1]; g.tr[2] = m->t[2]; }
void SetLightMatrix(MATRIX *m) { g.light = *m; }
void SetColorMatrix(MATRIX *m) { g.colorMat = *m; }
void SetBackColor(int r, int gg, int b) { g.bk[0] = r; g.bk[1] = gg; g.bk[2] = b; }

void PushMatrix(void) {
    if (g.savedDepth >= (int)(sizeof(g.savedRt) / sizeof(g.savedRt[0]))) return;
    g.savedRt[g.savedDepth] = g.rt;
    g.savedTr[g.savedDepth][0] = g.tr[0];
    g.savedTr[g.savedDepth][1] = g.tr[1];
    g.savedTr[g.savedDepth][2] = g.tr[2];
    g.savedDepth++;
}

void PopMatrix(void) {
    if (g.savedDepth <= 0) return;
    g.savedDepth--;
    g.rt = g.savedRt[g.savedDepth];
    g.tr[0] = g.savedTr[g.savedDepth][0];
    g.tr[1] = g.savedTr[g.savedDepth][1];
    g.tr[2] = g.savedTr[g.savedDepth][2];
}

/* Euler order Rz*Ry*Rx on column vectors matches PsyQ's RotMatrix (0x800d0aa8) term for term.
 * PsyQ computes it in integer over a packed sin/cos table; the double path here differs from it
 * by up to 15/4096. See docs/pc-port/subsystems/gte.md, "RotMatrix: float vs PsyQ integer path". */
static int s_gteTbl[4096];
/* Measurement switches, all off by default: VH_GTE_EXACT=<table> (integer path),
 * VH_GTE_EXACT_CHECK=<table> (float path, report the disagreement), VH_GTE_AB=<table> +
 * VH_GTE_AB_USE=float|exact (equal-cost A/B). The table is game data (tools/gen_gte_table.py). */
static int s_gteTblOk = -1;
static int s_gteExact = -1;
static int s_gteCheck = -1;
static int s_gteAB = -1;      /* equal-cost A/B: compute BOTH every call */
static int s_gteABUse;        /* 0 = keep float, 1 = keep exact */
static int s_chkWorst;
static long s_chkCalls;

static int GteLoadTable(const char *path) {
    FILE *f;
    if (s_gteTblOk == 1) return 1;      /* loaded once, shared by both switches */
    if (!path || !*path) return 0;      /* NOT latched: a miss here must not poison the other switch */
    f = fopen(path, "rb");
    if (!f) {
        fprintf(stderr, "PC_Gte: cannot open sin/cos table '%s' -- staying on the float path\n", path);
        return 0;
    }
    if (fread(s_gteTbl, 4, 4096, f) == 4096) s_gteTblOk = 1;
    else fprintf(stderr, "PC_Gte: sin/cos table '%s' is short -- staying on the float path\n", path);
    fclose(f);
    return s_gteTblOk;
}

/* Transcription of PsyQ RotMatrix (0x800d0aa8). `sra` floors and the negation happens before it,
 * so negated terms are floor(-x/4096); P and Q are re-quantised to Q12 before the second multiply
 * (a full-width product shifted once does not match); m[0][2] is sy raw from the table. */
static void RotMatrixExact(SVECTOR *r, MATRIX *m) {
    int sx, cx, sy, nsy, cy, sz, cz, t, P, Q;
    if (r->vx >= 0) { t = s_gteTbl[r->vx & 0xfff];    sx =  (short)(t & 0xffff); cx = t >> 16; }
    else            { t = s_gteTbl[(-r->vx) & 0xfff]; sx = -(short)(t & 0xffff); cx = t >> 16; }
    if (r->vy >= 0) { t = s_gteTbl[r->vy & 0xfff];    sy =  (short)(t & 0xffff); nsy = -sy; cy = t >> 16; }
    else            { t = s_gteTbl[(-r->vy) & 0xfff]; nsy = (short)(t & 0xffff); sy = -nsy; cy = t >> 16; }
    if (r->vz >= 0) { t = s_gteTbl[r->vz & 0xfff];    sz =  (short)(t & 0xffff); cz = t >> 16; }
    else            { t = s_gteTbl[(-r->vz) & 0xfff]; sz = -(short)(t & 0xffff); cz = t >> 16; }
    P = (nsy * cz) >> 12;
    Q = (nsy * sz) >> 12;
    m->m[0][0] = (short)((cz * cy) >> 12);
    m->m[0][1] = (short)((-(sz * cy)) >> 12);
    m->m[0][2] = (short)sy;
    m->m[1][0] = (short)(((sz * cx) >> 12) - ((P * sx) >> 12));
    m->m[1][1] = (short)(((cz * cx) >> 12) + ((Q * sx) >> 12));
    m->m[1][2] = (short)((-(cy * sx)) >> 12);
    m->m[2][0] = (short)(((sz * sx) >> 12) + ((P * cx) >> 12));
    m->m[2][1] = (short)(((cz * sx) >> 12) - ((Q * cx) >> 12));
    m->m[2][2] = (short)((cx * cy) >> 12);
}

static void RotMatrixCheck(SVECTOR *r, const MATRIX *flt) {
    MATRIX ex; int i, j, d;
    RotMatrixExact(r, &ex);
    s_chkCalls++;
    for (i = 0; i < 3; i++) for (j = 0; j < 3; j++) {
        d = flt->m[i][j] - ex.m[i][j];
        if (d < 0) d = -d;
        if (d > s_chkWorst) {
            s_chkWorst = d;
            fprintf(stderr, "PC_Gte[check] new worst |float-exact| = %d at m[%d][%d], "
                            "angles (%d,%d,%d), call %ld\n", d, i, j, r->vx, r->vy, r->vz, s_chkCalls);
        }
    }
    if ((s_chkCalls % 20000) == 0)
        fprintf(stderr, "PC_Gte[check] %ld calls, worst |float-exact| so far = %d (of 4096)\n",
                s_chkCalls, s_chkWorst);
}

MATRIX *RotMatrix(SVECTOR *r, MATRIX *m) {
    /* gnu89: every declaration stays ahead of the first statement. */
    double rx, ry, rz, sx, cx, sy, cy, sz, cz;
    double R[3][3];

    if (s_gteExact < 0) s_gteExact = GteLoadTable(getenv("VH_GTE_EXACT"));
    if (s_gteExact) { RotMatrixExact(r, m); return m; }

    rx = r->vx * (2.0 * M_PI / 4096.0);
    ry = r->vy * (2.0 * M_PI / 4096.0);
    rz = r->vz * (2.0 * M_PI / 4096.0);
    sx = sin(rx); cx = cos(rx);
    sy = sin(ry); cy = cos(ry);
    sz = sin(rz); cz = cos(rz);

    R[0][0] = cy * cz;
    R[0][1] = -cy * sz;
    R[0][2] = sy;
    R[1][0] = sx * sy * cz + cx * sz;
    R[1][1] = -sx * sy * sz + cx * cz;
    R[1][2] = -sx * cy;
    R[2][0] = -cx * sy * cz + sx * sz;
    R[2][1] = cx * sy * sz + sx * cz;
    R[2][2] = cx * cy;

    {
        int i, j;
        for (i = 0; i < 3; i++)
            for (j = 0; j < 3; j++)
                m->m[i][j] = (short)(R[i][j] * ONE);
    }
    if (s_gteCheck < 0) s_gteCheck = GteLoadTable(getenv("VH_GTE_EXACT_CHECK"));
    if (s_gteCheck) RotMatrixCheck(r, m);

    if (s_gteAB < 0) {
        s_gteAB = GteLoadTable(getenv("VH_GTE_AB"));
        s_gteABUse = 0;
        if (s_gteAB) {
            const char *u = getenv("VH_GTE_AB_USE");
            s_gteABUse = (u && (*u == 'e' || *u == 'E')) ? 1 : 0;
            fprintf(stderr, "PC_Gte: equal-cost A/B active, keeping the %s matrix\n",
                    s_gteABUse ? "EXACT" : "FLOAT");
        }
    }
    if (s_gteAB) {
        MATRIX ex;
        RotMatrixExact(r, &ex);          /* ALWAYS computed, so both runs cost the same */
        /* copy only m[][] -- neither path writes the translation t[], and taking it from an
         * uninitialised local would clobber the caller's. */
        if (s_gteABUse) memcpy(m->m, ex.m, sizeof(m->m));
    }
    return m;
}

MATRIX *TransMatrix(MATRIX *m, VECTOR *v) {
    m->t[0] = v->vx; m->t[1] = v->vy; m->t[2] = v->vz;
    return m;
}

MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v) {
    int scale[3];
    int i, j;
    scale[0] = v->vx; scale[1] = v->vy; scale[2] = v->vz;
    for (i = 0; i < 3; i++)
        for (j = 0; j < 3; j++)
            m->m[i][j] = (short)((m->m[i][j] * scale[j]) >> 12);
    return m;
}

void RotTrans(SVECTOR *v0, VECTOR *v1, int *flag) {
    int mac1, mac2, mac3;
    g.flag = 0;
    mac1 = (int)(((int64_t)g.tr[0] * 4096 + g.rt.m[0][0] * v0->vx + g.rt.m[0][1] * v0->vy + g.rt.m[0][2] * v0->vz) >> 12);
    mac2 = (int)(((int64_t)g.tr[1] * 4096 + g.rt.m[1][0] * v0->vx + g.rt.m[1][1] * v0->vy + g.rt.m[1][2] * v0->vz) >> 12);
    mac3 = (int)(((int64_t)g.tr[2] * 4096 + g.rt.m[2][0] * v0->vx + g.rt.m[2][1] * v0->vy + g.rt.m[2][2] * v0->vz) >> 12);
    v1->vx = mac1; v1->vy = mac2; v1->vz = mac3;
    if (flag) *flag = (int)g.flag;
}

static int PackSXY(int x, int y) {
    return ((y & 0xFFFF) << 16) | (x & 0xFFFF);
}

int RotTransPers(SVECTOR *v0, int *sxy, int *p, int *flag) {
    g.flag = 0;
    PC_GTE_LoadV0(v0);
    TransformOne(g.v0.vx, g.v0.vy, g.v0.vz);
    if (sxy) *sxy = PackSXY(g.sxy2[0], g.sxy2[1]);
    if (p) *p = g.ir0;
    if (flag) *flag = (int)g.flag;
    /* PsyQ RotTransPers (0x800d0178: mfc2 v0,$19; jr ra; sra v0,v0,2) returns SZ3 >> 2, the same
     * scale as the AVSZ4 path (ZSF4*sum(SZ)>>12 = SZ3/4 with ZSF4=0x100), so sprites interleave
     * with terrain in the OT instead of sinking behind it. Callers use it only as the OT index. */
    return g.sz3 >> 2;
}

int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                    int *sxy0, int *sxy1, int *sxy2, int *sxy3,
                    int *p, int *flag) {
    g.flag = 0;
    PC_GTE_LoadV3(v0, v1, v2);
    PC_GTE_RTPT();
    if (sxy0) *sxy0 = PackSXY(g.sxy0[0], g.sxy0[1]);
    if (sxy1) *sxy1 = PackSXY(g.sxy1[0], g.sxy1[1]);
    if (sxy2) *sxy2 = PackSXY(g.sxy2[0], g.sxy2[1]);

    PC_GTE_LoadV0(v3);
    TransformOne(g.v0.vx, g.v0.vy, g.v0.vz);
    if (sxy3) *sxy3 = PackSXY(g.sxy2[0], g.sxy2[1]);
    if (p) *p = g.ir0;
    if (flag) *flag = (int)g.flag;

    PC_GTE_AVSZ4();
    return g.otz;
}

int RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                  int *sxy0, int *sxy1, int *sxy2, int *sxy3,
                  int *p, int *flag) {
    int r = RotTransPers4(v0, v1, v2, v3, sxy0, sxy1, sxy2, sxy3, p, flag);
    /* VH_GTE_LOG=1: per AddObjPrim4 quad, dump the input vy of the bottom (v0) and top (v2) verts
     * against the projected screen Y; a tall wall whose dSy stays ~0 while dSx is large means the
     * GTE is collapsing its height. Screen coords are PackSXY(x=lo16, y=hi16), signed shorts. */
    {
        static int s_gteLog = -1;
        if (s_gteLog < 0) s_gteLog = getenv("VH_GTE_LOG") ? 1 : 0;
        if (s_gteLog) {
            extern unsigned s_drawFrame;
            int y0 = (short)((*sxy0 >> 16) & 0xFFFF), y2 = (short)((*sxy2 >> 16) & 0xFFFF);
            int x0 = (short)(*sxy0 & 0xFFFF),        x2 = (short)(*sxy2 & 0xFFFF);
            fprintf(stderr,
                "[gte4] f=%u inVxyz0=(%d,%d,%d) inVxyz2=(%d,%d,%d) out0=(%d,%d) out2=(%d,%d) "
                "dVy=%d dSy=%d dSx=%d\n",
                s_drawFrame, v0->vx, v0->vy, v0->vz, v2->vx, v2->vy, v2->vz,
                x0, y0, x2, y2, v2->vy - v0->vy, y2 - y0, x2 - x0);
        }
    }
    return r;
}

int VectorNormalS(VECTOR *v0, SVECTOR *v1) {
    double len = sqrt((double)v0->vx * v0->vx + (double)v0->vy * v0->vy + (double)v0->vz * v0->vz);
    if (len < 1.0) {
        v1->vx = v1->vy = v1->vz = 0;
        return 0;
    }
    v1->vx = (short)((v0->vx * (double)ONE) / len);
    v1->vy = (short)((v0->vy * (double)ONE) / len);
    v1->vz = (short)((v0->vz * (double)ONE) / len);
    return (int)len;
}

/* SquareRoot0/SquareRoot12/csqrt: "0"/"12" name the fixed-point scale of input and output
 * (unscaled vs Q12). PsyQ uses a coarse lookup table; true sqrt() here is the finer of the two.
 * See docs/pc-port/subsystems/gte.md, "SDK routines, verified against the binary". */
int SquareRoot0(int a) {
    if (a <= 0) return 0;
    return (int)sqrt((double)a);
}

int SquareRoot12(int a) {
    if (a <= 0) return 0;
    return (int)(sqrt((double)a / ONE) * ONE);
}

int csqrt(int a) {
    if (a <= 0) return 0;
    return (int)sqrt((double)a);
}

int rcos(int a) {
    double rad = (double)a * (2.0 * M_PI / 4096.0);
    return (int)(cos(rad) * ONE + (cos(rad) >= 0 ? 0.5 : -0.5));
}

int rsin(int a) {
    double rad = (double)a * (2.0 * M_PI / 4096.0);
    return (int)(sin(rad) * ONE + (sin(rad) >= 0 ? 0.5 : -0.5));
}

int ratan2(int y, int x) {
    double rad = atan2((double)y, (double)x);
    double units = rad * (4096.0 / (2.0 * M_PI));
    int a = (int)(units + (units >= 0 ? 0.5 : -0.5));
    while (a < 0) a += 4096;
    while (a >= 4096) a -= 4096;
    return a;
}
