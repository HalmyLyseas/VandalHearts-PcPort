/*
 * PC backend for the GTE (Geometry Transformation Engine).
 *
 * A single software model of the PS1 GTE coprocessor's register state and
 * opcodes, built from the public Nocash PSX Specifications hardware
 * reference (psx-spx.github.io/docs/geometrytransformationenginegte.md)
 * -- register numbering, RTPS/RTPT/NCLIP/AVSZ4/OP/NCCS formulas, saturation
 * rules, and the exact Unsigned Newton-Raphson (UNR) hardware division
 * algorithm (including its 257-entry lookup table, generated here from the
 * documented formula rather than hand-transcribed) are all taken from there,
 * not guessed.
 *
 * Two entry points share this one core, matching how the real SDK's
 * high-level RotTrans and SetRotMatrix functions are themselves presumably
 * built on top of the same raw coprocessor opcodes core/graphics.c calls
 * directly:
 *   - PC_GTE_ functions, called by the gte_ macros in
 *     platform/pc/include/inline_gte.h (core/graphics.c only -- the one file
 *     needing real coprocessor-level fidelity).
 *   - The high-level PsyQ/libgte.h SDK wrappers below (RotTrans, SetRotMatrix,
 *     PushMatrix, rcos/rsin, ...), used by the other 37 GTE-touching files,
 *     which only need geometrically correct results.
 *
 * SDK-library-level routines that aren't raw GTE opcodes (rcos, rsin, ratan2,
 * SquareRoot0, SquareRoot12, csqrt, RotMatrix's Euler axis order) have no
 * hardware formula to cite -- psx-spx documents the coprocessor, not Sony's
 * SDK library internals. These are reasoned standard-math implementations,
 * flagged as such here and in exchange 08-phase-c-gte-backend.md, same as
 * this project's prior flagged approximations (Audio's ADSR and pitch table,
 * Kernel's RCNT1 tick rate).
 */
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

    /* push/pop matrix stack. Originally a single saved slot -- wrong: real
     * PS1-era rendering code nests PushMatrix()/PopMatrix() calls (e.g. a
     * per-frame camera push, with individual objects/sprites pushing again
     * around their own local transform before popping back to the camera's),
     * and a single slot silently gets clobbered by the second push instead
     * of erroring, corrupting whichever matrix the first, still-unpopped
     * push was trying to preserve. Found via a real symptom: unit sprites'
     * computed screen coordinates (RenderUnitSprite, src/core/object.c) started
     * sane but degraded into wild garbage (thousands of pixels off-screen)
     * over successive frames of the same battle scene, despite stable
     * per-call input data -- exactly the signature of a matrix stack losing
     * saved state under nesting. 16 is a generous bound for this era's
     * scene-graph depth, not a hardware-mandated number (the GTE itself has
     * no native stack; PushMatrix/PopMatrix are SDK-level software only). */
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
    /* MUST be a 64-bit product: on the -m32 build `unsigned int` is 32-bit, and n*d overflows it
     * whenever the division result exceeds 0x10000 (i.e. whenever SZ3 < H) -- the wrapped garbage
     * gave those (nearer-than-H) vertices a tiny `n`, projecting them onto OFX/OFY and collapsing
     * all near geometry to the screen centre. The GTE does this division in hardware without a
     * C-width limit; do the multiply in 64-bit so it's correct at any `unsigned int` width. */
    n = (unsigned int)((((unsigned long long)n * d) + 0x8000) >> 16);
    if (n > 0x1FFFF) n = 0x1FFFF;
    return n;
}

/* Projection diagnostic ring: last 8 TransformOne results (terrain-collapse investigation). */
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

    /* psx-spx: "MAC0=(((H*20000h/SZ3)+1)/2)*IR1+OFX" -- a SIGNED multiply. IR1/
     * IR2/DQA are signed 16-bit interpolation registers (legitimately negative
     * for any point left of/above the projection center -- routine, not an
     * edge case), while n (the division result) is always non-negative. Casting
     * ir1/ir2/dqa to (unsigned short) before the multiply, as this used to do,
     * reinterprets a negative value as a huge positive one (e.g. -166 becomes
     * 65370), producing wildly wrong screen coordinates -- found via a real,
     * bit-exact-reproduced bug: RenderUnitSprite's (src/core/object.c) computed
     * quad corners matched real hardware whenever every ir1/ir2 for that
     * sprite happened to be positive, and went to tens-of-thousands-off-screen
     * garbage the moment any of them went negative, which is routine (facing
     * left of the camera's projection center), not rare -- exactly matching
     * the alternating sane/garbage pattern observed. Verified against a
     * captured real (buggy) run: replacing the unsigned cast with a plain
     * signed widen reproduces the actual captured output bit-for-bit. */
    g.mac0 = (int)n * (int)g.ir1 + g.ofx;
    {
        int sx = g.mac0 >> 16;
        int sy;
        g.mac0 = (int)n * (int)g.ir2 + g.ofy;
        sy = g.mac0 >> 16;

        /* Real GTE SATURATES SX2/SY2 to -0x400..+0x3FF (psx-spx: RTPS "ScrX/ScrY FIFO
         * -400h..+3FFh", FLAG.14/13). Geometry near the near-clip plane (n = H/SZ3 blows up as
         * SZ3->0) or far off the projection centre projects to huge screen coords that hardware
         * clamps to the screen edge; without this clamp our (short) cast WRAPS them (e.g. +40000
         * -> -25536), tearing projected quads across the frame -- most visibly a vertical seam at
         * the projection centre where IR1 flips sign. UI uses direct coords (no projection) so it's
         * unaffected, matching the symptom. Set the saturation flags too, as hardware does. */
        if (sx < -0x400) { sx = -0x400; g.flag |= 1u << 14; }
        else if (sx > 0x3FF) { sx = 0x3FF; g.flag |= 1u << 14; }
        if (sy < -0x400) { sy = -0x400; g.flag |= 1u << 13; }
        else if (sy > 0x3FF) { sy = 0x3FF; g.flag |= 1u << 13; }

        g.sxy0[0] = g.sxy1[0]; g.sxy0[1] = g.sxy1[1];
        g.sxy1[0] = g.sxy2[0]; g.sxy1[1] = g.sxy2[1];
        g.sxy2[0] = (short)sx; g.sxy2[1] = (short)sy;

        /* Projection diagnostic ring (terrain-collapse investigation): record what each vertex
         * projected to + the inputs, so the terrain path can dump a tile's 4 corners and we can
         * compute the exact spread = SX-OFX = H*IR1/SZ3 and see which term is deficient. */
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
    /* Real hardware ctc2's 3 words into control regs 0/2/4 (RT11/RT13/RT22),
     * "misusing" the RT matrix diagonal as a vector (see psx-spx OP command).
     * We model the functional effect (D1,D2,D3 feeding into OP0) directly
     * rather than the bit-packing, since nothing else reads the RT matrix
     * between this and gte_op0(). */
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
    /* Real hardware's gte_stotz (include/PsyQ/inline.h) is a raw SWC2 (Store Word from
     * Coprocessor 2) -- always a full 32-bit store, never 16-bit, regardless of OTZ's
     * value range. The PS1 GTE's OTZ register (cop2r7) is a saturated 16-bit unsigned
     * value zero-extended into its full 32-bit register, so real hardware always leaves
     * the upper 16 bits of the destination at zero. Writing only 16 bits here left the
     * caller's full-width `int otz` variable (src/core/graphics.c's RenderMapTile and friends)
     * with its upper 16 bits as whatever uninitialized stack garbage preceded the call --
     * usually harmless (otz normally small), but a real, reported SIGSEGV once garbage
     * upper bits made otz large enough to push `ot + OT_SIZE - otz` (AddPrim's target)
     * wildly out of bounds. */
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
    /* GTE control-register defaults, taken byte-for-byte from the REAL PsyQ InitGeom disassembled
     * out of SLUS_004.47 (0x800d04a8: `li t0,imm; ctc2 t0,$reg`) -- NOT guessed. The whole set was
     * previously either left 0 or estimated from the psx-spx "normally 1/3, 1/4" note, which is
     * wrong for this library:
     *   $29 ZSF3 = 0x155,  $30 ZSF4 = 0x100   (Z-average scale factors; OTZ = ZSF * sum(SZ) >> 12)
     *   $26 H    = 1000    (overridden by SetGeomScreen(0x200) before rendering, kept for fidelity)
     *   $27 DQA  = -4194,  $28 DQB = 0x1400000 (depth-cue interpolation -> IR0)
     * The earlier 0x555/0x400 guess (feedback-16) was exactly 4x too large, so terrain OTZ (via
     * gte_avsz4) came out ~4x inflated (nearest tile ~472 instead of ~118), pushing essentially all
     * terrain past core/graphics.c's distance-darkening black threshold (otz>=406) -- the "black terrain"
     * symptom. 0x100 is the ground-truth value; verified by disassembly, matches real hardware. */
    g.zsf3 = 0x0155;
    g.zsf4 = 0x0100;
    g.h    = 1000;
    g.dqa  = -4194;
    g.dqb  = 0x1400000;
}

/* The GTE OFX/OFY registers are 1.15.16 fixed-point (TransformOne adds them in that scale then
 * >>16), so the screen-pixel arguments must be shifted <<16. Storing them raw collapsed the
 * projection centre to (0,0), translating all 3D-projected content up-left by (ofx,ofy) -- which
 * pushed unit sprites off-screen while leaving the (larger) terrain partly visible. See
 * exchange/feedback-15.md / the sprite-fate GTE-state log. */
void SetGeomOffset(int ofx, int ofy) { g.ofx = ofx << 16; g.ofy = ofy << 16; }
void SetGeomScreen(int h) { g.h = (short)h; }

/* Debug (feedback-15 follow-up): expose the projection state used by the last TransformOne, so
 * the sprite log can tell whether missing units are mis-projected by a wrong geom offset
 * (ofx/ofy/h -- the "vector translation" theory) or by the leaked facing matrix (rt = scaled
 * identity {4096,.,0;...;0,.,4096} vs a rotated camera matrix). */
void PC_GteDebugState(int *ofx, int *ofy, int *h, int *rt00, int *rt02, int *rt22,
                      int *trx, int *trz) {
    *ofx = g.ofx; *ofy = g.ofy; *h = g.h;
    *rt00 = g.rt.m[0][0]; *rt02 = g.rt.m[0][2]; *rt22 = g.rt.m[2][2];
    *trx = g.tr[0]; *trz = g.tr[2];
}

/* g.otz = last AVSZ4 result (terrain OTZ); zsf4 = the Z-scale factor. Used to verify the
 * zsf4 fix -- terrain OTZ should now land in the sprite SZ3 range, not 0/5. */
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

/* Euler angle order (Rz * Ry * Rx applied to column vectors) is an SDK
 * library convention, not a raw GTE opcode -- psx-spx doesn't cover it.
 * Reasoned choice, matching the most common PsyQ-era convention; flagged
 * pending verification once real rendering output can be eyeballed. */
MATRIX *RotMatrix(SVECTOR *r, MATRIX *m) {
    double rx = r->vx * (2.0 * M_PI / 4096.0);
    double ry = r->vy * (2.0 * M_PI / 4096.0);
    double rz = r->vz * (2.0 * M_PI / 4096.0);
    double sx = sin(rx), cx = cos(rx);
    double sy = sin(ry), cy = cos(ry);
    double sz = sin(rz), cz = cos(rz);
    double R[3][3];

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
    /* Real PsyQ RotTransPers returns SZ3 >> 2 (÷4), NOT raw SZ3. Disassembled from the byte-exact
     * SLUS_004.47 @ 0x800d0178:  mfc2 v0,$19 (SZ3) ; jr ra ; sra v0,v0,0x2 (delay slot). This puts
     * the returned otz on the SAME scale as the terrain's AVSZ path (gte_stotz = zsf4·ΣSZ>>12 =
     * sz3/4 with zsf4=0x100), so RotTransPers-drawn sprites interleave with terrain in the OT instead
     * of sitting ~4x too deep and being painted over. We previously returned raw g.sz3, which made
     * the demo-opening units render behind the terrain (Thread 3 occlusion); the VH_SPRITE_OTZ_DIV=4
     * validation hack was accidentally replicating this exact >>2. Callers use the return ONLY as the
     * OT index; screen X/Y (*sxy) is stored above and unaffected. */
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
    /* VH_GTE_LOG=1: for each AddObjPrim4 quad, dump the input SVECTOR height (vy) of the bottom
     * (v0) and top (v2) verts vs the projected screen Y. bugreport-02: the thunder-ray cylinder
     * wall has Δvy≈256 (0x800>>3) between bottom/top and should give a tall on-screen wall; if the
     * output Δsy stays ~0 while a comparable Δsx is large, the GTE is collapsing the height. Screen
     * coords are PackSXY(x=lo16, y=hi16), both signed shorts. Env checked once. */
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

/* SquareRoot0/SquareRoot12/csqrt are SDK library routines, not raw GTE
 * opcodes -- no hardware formula exists to cite. "0"/"12" name the assumed
 * fixed-point scale of the input/output (unscaled vs. Q12); implemented via
 * standard sqrt(), flagged as a reasoned choice like RotMatrix's axis order. */
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
