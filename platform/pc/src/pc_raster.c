/* pc_raster.c -- the software GPU: framebuffers (native VRAM + G2 hi-res) and rasterization.
 * Extracted verbatim from libgpu.c, which keeps the PsyQ API surface + the OT walker and calls in
 * here per primitive (seams: pc_gpu_internal.h). Contents: BGR555 pixel helpers, the G1 PSX-accurate
 * fixed-point DDA (VH_ACCURATE, default) + the legacy barycentric fallback, texture sampling with
 * the GP0(E2h) texture window, G2 internal-resolution supersampling (VH_INTERNAL_SCALE) with the
 * per-frame hi-res display list, and the P1 persistent worker pool that rasterizes it in bands.
 * PERF: the whole per-pixel hot path (dda_span -> SampleTexture -> PutPixel -> blend) lives in this
 * single TU on the owned static s_vram/s_hires buffers, so the split changes no inlining. */
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>     /* P1 step 2b: band-parallel hi-res rasterization (winpthreads on MinGW) */

#include "PsyQ/libgpu.h"
#include "pc_platform.h"
#include "pc_gpu_internal.h"

#define VRAM_W 1024
#define VRAM_H 512

static unsigned short s_vram[VRAM_H][VRAM_W];

/* ---- G2: internal-resolution supersampling (VH_INTERNAL_SCALE) --------------------------------
 * Each primitive is rasterized a SECOND time into an S-times-larger colour buffer (s_hires) with its
 * geometry scaled by S but UVs left native, so the DDA samples the native-res texture once PER HI-RES
 * PIXEL -- coverage and texture both at S x the density. s_vram stays native and byte-identical (all
 * readback / blend / effect behaviour unchanged); present() blits s_hires when S>1. Bulk VRAM writes
 * that bypass the rasterizer (ClearImage/LoadImage/MoveImage) are mirrored in as SxS nearest blocks.
 * Backend-only, default OFF (S=1); VH_INTERNAL_SCALE=N or the options overlay ("INTERNAL RES") set it,
 * live and persisted. `g_vhInternalScale` is the overlay-facing setting (see pc_platform.h). */
#define HIRES_MAXSCALE 4
int g_vhInternalScale = -1;              /* 1 (off) .. HIRES_MAXSCALE; -1 = unresolved (env not read yet) */
static unsigned short *s_hires = NULL;   /* allocated at HIRES_MAXSCALE so the scale can change live */

static int InternalScale(void) {
    if (g_vhInternalScale < 0) {
        const char *v = getenv("VH_INTERNAL_SCALE");
        int s = v ? atoi(v) : 1;
        if (s < 1) s = 1;
        if (s > HIRES_MAXSCALE) s = HIRES_MAXSCALE;
        g_vhInternalScale = s;
    }
    return g_vhInternalScale;
}

void HiresEnsure(void) {
    /* Allocate at the MAX scale once, so the overlay can raise/lower the scale live without reallocating
     * (each frame uses the current scale's stride within this buffer). */
    if (!s_hires && InternalScale() > 1)
        s_hires = (unsigned short *)calloc((size_t)VRAM_W * HIRES_MAXSCALE * VRAM_H * HIRES_MAXSCALE,
                                           sizeof(unsigned short));
}

/* Overlay/live setter (pc_platform.h). Clamp, (re)allocate if needed, and clear so a scale change
 * doesn't briefly show the previous scale's stride as garbage. */
void PC_GpuSetInternalScale(int s) {
    if (s < 1) s = 1;
    if (s > HIRES_MAXSCALE) s = HIRES_MAXSCALE;
    if (s == g_vhInternalScale) return;
    g_vhInternalScale = s;
    if (s > 1) {
        HiresEnsure();
        if (s_hires)
            memset(s_hires, 0, (size_t)VRAM_W * HIRES_MAXSCALE * VRAM_H * HIRES_MAXSCALE * sizeof(unsigned short));
    }
}

/* Framebuffer-byte + scale accessors for the extracted TUs (pc_gpu_internal.h) -- the trace
 * harness hashes the raw buffers for its regression signature without owning them. */
const void *PC_GpuVramBytes(size_t *n) { *n = sizeof(s_vram); return s_vram; }
unsigned short (*PC_GpuVram(void))[VRAM_W] { return s_vram; }
const void *PC_GpuHiresBytes(size_t *n) {
    int S = InternalScale();
    if (!s_hires || S <= 1) { *n = 0; return NULL; }
    *n = (size_t)VRAM_W * S * VRAM_H * S * 2;
    return s_hires;
}
int PC_GpuGetInternalScale(void) { return InternalScale(); }

/* Mirror a native VRAM rect into s_hires as SxS nearest blocks (bulk writes bypass the rasterizer). */
void HiresMirrorRect(int x0, int y0, int w, int h) {
    int S, x, y, sx, sy, W;
    if (InternalScale() <= 1) return;
    HiresEnsure();
    if (!s_hires) return;
    S = g_vhInternalScale; W = VRAM_W * S;
    for (y = 0; y < h; y++) {
        int vy = y0 + y;
        if (vy < 0 || vy >= VRAM_H) continue;
        for (x = 0; x < w; x++) {
            int vx = x0 + x;
            unsigned short c;
            if (vx < 0 || vx >= VRAM_W) continue;
            c = s_vram[vy][vx];
            for (sy = 0; sy < S; sy++)
                for (sx = 0; sx < S; sx++)
                    s_hires[(size_t)(vy * S + sy) * W + (vx * S + sx)] = c;
        }
    }
}

/* ---- pixel format helpers (BGR555, bit15 = mask/semi-transparency source) */

static unsigned short PackColor(int r, int g, int b) {
    if (r < 0) r = 0;
    if (r > 255) r = 255;
    if (g < 0) g = 0;
    if (g > 255) g = 255;
    if (b < 0) b = 0;
    if (b > 255) b = 255;
    return (unsigned short)(((b >> 3) << 10) | ((g >> 3) << 5) | (r >> 3));
}

void UnpackColor(unsigned short c, int *r, int *g, int *b) {   /* non-static: pc_hdpack.c decodes dumps with it */
    *r = (c & 0x1F) << 3;
    *g = ((c >> 5) & 0x1F) << 3;
    *b = ((c >> 10) & 0x1F) << 3;
}

/* PSX GPU ordered-dither matrix (GP0(E1h).9): a 4x4 signed offset added to 24-bit R/G/B before the
 * 24->15-bit truncation, breaking hard 5-bit steps into a fine stipple. Used by the accurate DDA's
 * PackColorG1Front (gated on the real dither-enable bit). */
static const signed char DITHER4[4][4] = {
    { -4,  0, -3,  1 },
    {  2, -2,  3, -1 },
    { -3,  1, -4,  0 },
    {  3, -1,  2, -2 },
};
/* Legacy-renderer pack (VH_ACCURATE=0): the legacy barycentric path packs straight, without dither.
 * (The old VH_DITHER prototype dither on this path was superseded by the accurate G1 dither and removed.) */
static unsigned short PackColorDither(int r, int g, int b, int x, int y) {
    (void)x; (void)y;
    return PackColor(r, g, b);   /* PackColor clamps to [0,255] then truncates to 5-bit */
}

/* G1 (1.5) PSX-accurate rasterization: UV round-to-nearest, front-colour dither, 5-bit blargg blend.
 * Validated against a DuckStation VRAM oracle (platform/pc/tools/raster_harness): 27.17 -> 4.96 mean
 * abs-diff, 96.4% pixel-exact. **Default ON** (the preferred, hardware-faithful look) -- set
 * VH_ACCURATE=0 (vandalhearts.ini or env) to fall back to the legacy renderer (advanced users only).
 * Runtime gate (not #ifdef) so the harness can A/B one binary: pass VH_ACCURATE=0 for the legacy path. */
static int AccurateEnabled(void) {
    static int e = -1;
    if (e < 0) {
        const char *v = getenv("VH_ACCURATE");
        e = (v && v[0] == '0') ? 0 : 1;   /* default ON; only an explicit VH_ACCURATE=0 disables */
    }
    return e;
}

/* Rule 4 (gotcha #4): the GPU dithers the FRONT colour in the 24->15 truncation (BEFORE any
 * semi-transparency blend), on modulated/untextured polygons only, matrix indexed [y&3][x&3].
 * Same DITHER4 offsets as DuckStation's DITHER_MATRIX; PackColor's clamp-then->>3 reproduces the
 * hardware dither LUT (clamp((v+off)>>3,0,31)). The blend result itself is written un-dithered. */
static unsigned short PackColorG1Front(int r, int g, int b, int x, int y) {
    int d = DITHER4[y & 3][x & 3];
    return PackColor(r + d, g + d, b + d);
}

/* Rule 5 (gotcha #5): the GPU's semi-transparency is blargg's parallel 5-bit BGR555 bit-math on
 * the packed front/back halfwords (per-channel 5-bit add/sub with saturation), not the 8-bit
 * per-channel blend we used to unpack/blend/repack. Ported verbatim from DuckStation's ShadePixel.
 * `fg` carries bit15 set (this is a blending pixel); for untextured polys bit15 is cleared after. */
static unsigned short BlendG1(unsigned fg, unsigned bg, int abr, int textured) {
    unsigned color = 0;
    switch (abr) {
    case 0: /* 0.5*B + 0.5*F */
        bg |= 0x8000u;
        color = ((fg + bg) - ((fg ^ bg) & 0x0421u)) >> 1;
        break;
    case 1: { /* B + F */
        unsigned sum, carry;
        bg &= ~0x8000u;
        sum = fg + bg;
        carry = (sum - ((fg ^ bg) & 0x8421u)) & 0x8420u;
        color = (sum - carry) | (carry - (carry >> 5));
        break;
    }
    case 2: { /* B - F */
        unsigned diff, borrow;
        bg |= 0x8000u; fg &= ~0x8000u;
        diff = bg - fg + 0x108420u;
        borrow = (diff - ((bg ^ fg) & 0x108420u)) & 0x108420u;
        color = (diff - borrow) & (borrow - (borrow >> 5));
        break;
    }
    case 3: { /* B + 0.25*F */
        unsigned sum, carry;
        bg &= ~0x8000u;
        fg = ((fg >> 2) & 0x1CE7u) | 0x8000u;
        sum = fg + bg;
        carry = (sum - ((fg ^ bg) & 0x8421u)) & 0x8420u;
        color = (sum - carry) | (carry - (carry >> 5));
        break;
    }
    }
    if (!textured) color &= ~0x8000u;
    return (unsigned short)(color & 0xFFFFu);
}


/* Blend/write one colour into a target pixel (native s_vram or s_hires). x,y are target-space, used
 * only for the dither matrix. */
static void WritePixel(unsigned short *px, int x, int y, unsigned short c, int abr, int semiTrans, int textured) {
    if (semiTrans) {
        if (AccurateEnabled()) {
            *px = BlendG1((unsigned)c | 0x8000u, *px, abr, textured);
        } else {
            int r, g, b, br, bg, bb;
            UnpackColor(c, &r, &g, &b);
            UnpackColor(*px, &br, &bg, &bb);
            switch (abr) {
            case 0: r = (br + r) / 2; g = (bg + g) / 2; b = (bb + b) / 2; break;
            case 1: r = br + r; g = bg + g; b = bb + b; break;
            case 2: r = br - r; g = bg - g; b = bb - b; break;
            case 3: r = br + r / 4; g = bg + g / 4; b = bb + b / 4; break;
            }
            *px = PackColorDither(r, g, b, x, y);
        }
    } else {
        *px = c;
    }
}

static void PutPixel(const RenderCtx *rc, int x, int y, unsigned short c, int abr, int semiTrans, int textured) {
    if (rc->target) {                      /* G2: write the supersampled target (coords already scaled) */
        int W = VRAM_W * rc->scale, H = VRAM_H * rc->scale;
        if (x < 0 || x >= W || y < 0 || y >= H) return;
        WritePixel(&s_hires[(size_t)y * W + x], x, y, c, abr, semiTrans, textured);
        return;
    }
    if (x < 0 || x >= VRAM_W || y < 0 || y >= VRAM_H) return;
    WritePixel(&s_vram[y][x], x, y, c, abr, semiTrans, textured);
}

void TPageOrigin(int tpage, int *x, int *y, int *tp) {   /* non-static: pc_hdpack.c maps UV bboxes to VRAM */
    *x = (tpage & 0xF) * 64;
    *y = ((tpage >> 4) & 1) * 256;
    *tp = (tpage >> 7) & 3;
}

static unsigned short SampleTexture(const RenderCtx *rc, int tpage, int clut, int u, int v) {
    int tpX, tpY, tp;
    int clutX = (clut & 0x3F) * 16;
    int clutY = (clut >> 6) & 0x1FF;
    unsigned short raw;

    /* Apply the active texture window (GP0(E2h)); mask 0 leaves u/v untouched. */
    u = (u & ~(rc->twMaskX * 8)) | ((rc->twOffX & rc->twMaskX) * 8);
    v = (v & ~(rc->twMaskY * 8)) | ((rc->twOffY & rc->twMaskY) * 8);

    TPageOrigin(tpage, &tpX, &tpY, &tp);

    if (tp == 0) { /* 4bpp indexed */
        unsigned short hw = s_vram[tpY + v][tpX + u / 4];
        int index = (hw >> ((u % 4) * 4)) & 0xF;
        raw = s_vram[clutY][clutX + index];
    } else if (tp == 1) { /* 8bpp indexed */
        unsigned short hw = s_vram[tpY + v][tpX + u / 2];
        int index = (u % 2) ? ((hw >> 8) & 0xFF) : (hw & 0xFF);
        raw = s_vram[clutY][clutX + index];
    } else { /* 15bpp direct */
        raw = s_vram[tpY + v][tpX + u];
    }
    return raw;
}

/* ---- triangle rasterizer (flat or textured, affine UV, no perspective) -- */


/* ===================== VH_DDA (experimental): fixed-point integer triangle DDA =====================
 * Faithful port of DuckStation's Mednafen rasterizer (external/duckstation gpu_sw_rasterizer.inl:
 * DrawTriangle / DrawTrianglePart / DrawSpan / UVStepper). Unlike our barycentric FillTriangle -- which
 * samples COVERAGE at the pixel centre (x+0.5,y+0.5) but UV at the corner (x,y) -- the DDA evaluates
 * BOTH at the pixel's INTEGER position in 64-bit fixed point (edges) + 24-frac-bit UV, so there is no
 * centre/corner mismatch: the source of our tile-edge seams AND the opaque-UV extrapolation (fuzzy
 * stones / fragmented water). Per-pixel shading (texture sample, modulate, dither, blend) stays ours.
 * This IS the VH_ACCURATE (default) rasterizer -- validated 99.99% (cutscene) / 99.84% (battle 6-1)
 * pixel-exact vs DuckStation's VRAM oracle, multi-scene + in-game. VH_ACCURATE=0 falls back to the
 * legacy barycentric FillTriangle below (centre-sample, floor UV, no dither). */
#define DDA_ASHIFT 12
#define DDA_APOST  12
typedef struct { unsigned u, v; } DdaUV;
typedef struct { unsigned dudx, dvdx, dudy, dvdy; } DdaUVStep;
typedef struct { int start_y, end_y; long long start_x[2], step_x[2]; int upside_down; } DdaPart;
typedef struct {
    int r, g, bcol, textured, tpage, clut, semiTrans, abr;
    int clipL, clipR, clipT, clipB;   /* inclusive drawing area */
    DdaUVStep step;
    const RenderCtx *rc;              /* per-pass state: dither, texture window, target/scale (P1) */
    /* HD-pack replace (1.6): resolved ONCE per triangle in FillTriangleDDA. hdPx==NULL -> no replacement,
     * per-pixel path is unchanged. Scale is precomputed to a fixed-point multiply (no per-pixel divide). */
    const unsigned short *hdPx; int hdW, hdH;       /* HD image, PRE-PACKED to 16-bit texels (0=transparent) + dims */
    int hdTpX, hdTpY, hdPpw, hdRx, hdRy, hdRw, hdRh; /* hoisted VRAM-word mapping constants + region rect */
    long long hdSx, hdSy;                            /* (hdW<<HD_SC)/nativeW , (hdH<<HD_SC)/nativeH */
} DdaCtx;
#define HD_SC 16

static long long dda_makefp(int x)  { return ((long long)x << 32) + ((1LL << 32) - (1 << 11)); }
static long long dda_makestep(int dx, int dy) {
    long long bias = (dx < 0) ? -(long long)(dy - 1) : ((dx > 0) ? (long long)(dy - 1) : 0);
    return (((long long)dx << 32) + bias) / dy;
}
static int dda_unfp(long long xfp) { return (int)((unsigned long long)xfp >> 32); }
static void dda_uv_init(DdaUV *s, int us, int vs) {
    s->u = (((unsigned)us << DDA_ASHIFT) + (1u << (DDA_ASHIFT - 1))) << DDA_APOST;
    s->v = (((unsigned)vs << DDA_ASHIFT) + (1u << (DDA_ASHIFT - 1))) << DDA_APOST;
}
static int dda_getu(const DdaUV *s) { return (int)((s->u >> (DDA_ASHIFT + DDA_APOST)) & 0xFF); }
static int dda_getv(const DdaUV *s) { return (int)((s->v >> (DDA_ASHIFT + DDA_APOST)) & 0xFF); }
static void dda_stepx_n(DdaUV *s, const DdaUVStep *st, int n) {
    s->u += (unsigned)((int)st->dudx * n); s->v += (unsigned)((int)st->dvdx * n);
}
static void dda_stepy_n(DdaUV *s, const DdaUVStep *st, int n) {
    s->u += (unsigned)((int)st->dudy * n); s->v += (unsigned)((int)st->dvdy * n);
}

/* HD asset-replacement (1.6 HD pack) lives in pc_hdpack.c -- LoadImage and the DDA below are its
 * hooks (HdPack_OnLoad / HdFindTriRegion / HdMaybeDump; seams in pc_gpu_internal.h). */

static void dda_span(const DdaCtx *cx, int y, int x_start, int x_bound, DdaUV uv) {
    int width = x_bound - x_start;         /* fill [x_start, x_bound): left-inclusive, right-exclusive */
    int current_x = x_start;
    if (current_x < cx->clipL) { int d = cx->clipL - current_x; x_start += d; current_x += d; width -= d; }
    if (current_x + width > cx->clipR + 1) width = cx->clipR + 1 - current_x;
    if (width <= 0) return;
    if (cx->textured) dda_stepx_n(&uv, &cx->step, x_start);   /* seed UV to the span's start x */
    {
    do {
        if (cx->textured) {
            int tr, tg, tb, su = dda_getu(&uv), sv = dda_getv(&uv), draw = 1;
            unsigned short texel;
            int wx = cx->hdPx ? cx->hdTpX + su / cx->hdPpw : 0, wy = cx->hdPx ? cx->hdTpY + sv : 0;
            if (cx->hdPx &&                                  /* HD replace, and this pixel is inside the region */
                wx >= cx->hdRx && wx < cx->hdRx + cx->hdRw && wy >= cx->hdRy && wy < cx->hdRy + cx->hdRh) {
                /* region resolved per-triangle; precomputed scale -> multiply, no per-pixel divide */
                int shift = DDA_ASHIFT + DDA_APOST;
                int px = (wx - cx->hdRx) * cx->hdPpw + (su % cx->hdPpw), py = wy - cx->hdRy;
                long long nxf = ((long long)px << shift) + (uv.u & ((1u << shift) - 1));
                long long nyf = ((long long)py << shift) + (uv.v & ((1u << shift) - 1));
                long long hx = (nxf * cx->hdSx) >> (shift + HD_SC);
                long long hy = (nyf * cx->hdSy) >> (shift + HD_SC);
                unsigned short t;
                if (hx < 0) { hx = 0; } if (hx >= cx->hdW) { hx = cx->hdW - 1; }
                if (hy < 0) { hy = 0; } if (hy >= cx->hdH) { hy = cx->hdH - 1; }
                t = cx->hdPx[hy * cx->hdW + hx];             /* pre-packed 16-bit texel; 0 = transparent */
                if (t == 0) draw = 0;
                else texel = t;
            } else { texel = SampleTexture(cx->rc, cx->tpage, cx->clut, su, sv); draw = (texel != 0); }
            if (draw) {                                      /* 0000h = transparent (skip, still step) */
                UnpackColor(texel, &tr, &tg, &tb);
                tr = (tr * cx->r) / 128; tg = (tg * cx->g) / 128; tb = (tb * cx->bcol) / 128;
                PutPixel(cx->rc, current_x, y,
                         (AccurateEnabled() && cx->rc->dither) ? PackColorG1Front(tr, tg, tb, current_x, y)
                                                               : PackColor(tr, tg, tb),
                         cx->abr, cx->semiTrans && (texel & 0x8000), 1);
            }
        } else {
            PutPixel(cx->rc, current_x, y,
                     (AccurateEnabled() && cx->rc->dither) ? PackColorG1Front(cx->r, cx->g, cx->bcol, current_x, y)
                                                           : PackColor(cx->r, cx->g, cx->bcol),
                     cx->abr, cx->semiTrans, 0);
        }
        current_x++;
        if (cx->textured) { uv.u += cx->step.dudx; uv.v += cx->step.dvdx; }
    } while (--width > 0);
    }
}

static void dda_part(const DdaCtx *cx, const DdaPart *tp, DdaUV origin) {
    long long left_x = tp->start_x[0], right_x = tp->start_x[1];
    long long lstep = tp->step_x[0], rstep = tp->step_x[1];
    int y = tp->start_y, end_y = tp->end_y;
    DdaUV luv = origin;
    if (tp->upside_down) {
        if (y <= end_y) return;
        if (cx->textured) dda_stepy_n(&luv, &cx->step, y);
        do {
            y--; left_x -= lstep; right_x -= rstep;
            if (y < cx->clipT) break;
            if (cx->textured) { luv.u -= cx->step.dudy; luv.v -= cx->step.dvdy; }
            if (y > cx->clipB) continue;
            dda_span(cx, y, dda_unfp(left_x), dda_unfp(right_x), luv);
        } while (y > end_y);
    } else {
        if (y >= end_y) return;
        if (cx->textured) dda_stepy_n(&luv, &cx->step, y);
        do {
            if (y > cx->clipB) break;
            if (y >= cx->clipT) dda_span(cx, y, dda_unfp(left_x), dda_unfp(right_x), luv);
            y++; left_x += lstep; right_x += rstep;
            if (cx->textured) { luv.u += cx->step.dudy; luv.v += cx->step.dvdy; }
        } while (y < end_y);
    }
}

static void FillTriangleDDA(const RenderCtx *rc, RVert ra, RVert rb, RVert rvc, int r, int g, int bcol,
                            int textured, int tpage, int clut, int semiTrans, int abr, int flat2d) {
    /* G2: when rasterizing into the hires target, scale geometry (vertices/offset/clip) by S but keep
     * UVs native. The DDA's own UV-step math then advances the native texture at 1/S the rate across the
     * S-times-wider span, i.e. one native-texture sample per hi-res pixel -- coverage AND texture at Sx
     * density (this is the sharp, per-hires-pixel path; the native pass, rc->target=0, uses S=1). */
    int S = rc->target ? rc->scale : 1;
    int ox = rc->ofsX * S, oy = rc->ofsY * S;
    int vx[3], vy[3], vu[3], vv[3], i;
    HdRegion *hdReg = NULL;   /* 1.6 HD pack: replaced region this triangle samples (resolved below) */
    RVert rv[3]; rv[0] = ra; rv[1] = rb; rv[2] = rvc;
    for (i = 0; i < 3; i++) {
        vx[i] = (int)lround(rv[i].x) * S + ox; vy[i] = (int)lround(rv[i].y) * S + oy;
        vu[i] = (int)lround(rv[i].u);          vv[i] = (int)lround(rv[i].v);
    }
    if (textured && HdRegionCount()) {   /* 1.6 HD pack: per-triangle UV bbox -> dump reference and/or resolve replaced region */
        int uMn = vu[0], uMx = vu[0], vMn = vv[0], vMx = vv[0], k;
        for (k = 1; k < 3; k++) { if (vu[k]<uMn)uMn=vu[k]; if (vu[k]>uMx)uMx=vu[k]; if (vv[k]<vMn)vMn=vv[k]; if (vv[k]>vMx)vMx=vv[k]; }
        if (HdDumpDir())                    HdMaybeDump(tpage, clut, uMn, uMx, vMn, vMx);
        if (rc->target && HdReplaceCount() && HdActive()) hdReg = HdFindTriRegion(tpage, uMn, uMx, vMn, vMx);
        {   /* VH_HD_TRACE: log each UNIQUE (tpage,clut) that gets HD-replaced -> reveals which prims are
             * wrongly matched (battle overlays/effects) vs the real background, to pick a discriminator. */
            static int tr = -1; if (tr < 0) tr = getenv("VH_HD_TRACE") ? 1 : 0;
            if (hdReg && tr) {
                static unsigned seen[512]; static int nseen;
                unsigned key = ((unsigned)tpage << 16) | (unsigned)(clut & 0xFFFF);
                int j, dup = 0;
                for (j = 0; j < nseen; j++) if (seen[j] == key) { dup = 1; break; }
                if (!dup && nseen < 512) {
                    seen[nseen++] = key;
                    fprintf(stderr, "[HDtrace] tpage=%d clut=%d flat2d=%d semiTrans=%d abr=%d uv=[%d..%d,%d..%d] -> region(%d,%d,%d,%d) %016llx\n",
                            tpage, clut, flat2d, semiTrans, abr, uMn, uMx, vMn, vMx,
                            hdReg->rx, hdReg->ry, hdReg->rw, hdReg->rh, hdReg->hash);
                }
            }
        }
    }
    /* `flat2d` (from the whole quad, via FillQuad): the prim projects to an axis-aligned screen
     * RECTANGLE (UI window / text glyph / billboard sprite). It scopes the "crust for free" +0.5-texel
     * bias below -- applied to perspective tiles (lands sampling on the interior, off the dark border
     * crust) but NOT to axis-aligned unit-mapped UI (there it shifts the sample half a texel and
     * doubles/drops columns -- the 2D "vertical lines"). Decided per-QUAD so both triangles agree;
     * a per-triangle decision splits one quad's halves and leaves a diagonal seam. */
    /* G2 seam fix (hires pass only): a full 256-wide/tall texture's EXCLUSIVE right/bottom edge sits at
     * U/V==256, which the 32-bit UV fixed point can't hold (256<<24 overflows to 0), so the finer hires
     * sampling tips the last pixel to texel 0 instead of the real edge texel 255 -- a dark seam on
     * full-page background sprites. Clamp that exact edge to 255. Only single-page sprites (u0+w==256)
     * have a vertex precisely at 256; tiled sprites (U past 256) wrap correctly via the natural overflow
     * and never have a 256 vertex, so they're untouched. Native pass (S==1) never reaches U==256. */
    if (S > 1) {
        for (i = 0; i < 3; i++) {
            if (vu[i] == 256) vu[i] = 255;
            if (vv[i] == 256) vv[i] = 255;
        }
    }
#define DDA_SWAP(p, q) do { int t; \
    t=vx[p];vx[p]=vx[q];vx[q]=t; t=vy[p];vy[p]=vy[q];vy[q]=t; \
    t=vu[p];vu[p]=vu[q];vu[q]=t; t=vv[p];vv[p]=vv[q];vv[q]=t; } while (0)
    /* sort v0=top v1=mid v2=bottom by y, tracking which is the top-left vertex (`tl`) */
    unsigned tl;
    if (vx[1] <= vx[0]) tl = (vx[2] <= vx[1]) ? 4 : 2;
    else if (vx[2] < vx[0]) tl = 4; else tl = 1;
    if (vy[2] < vy[1]) { DDA_SWAP(1, 2); tl = ((tl >> 1) & 0x2) | ((tl << 1) & 0x4) | (tl & 0x1); }
    if (vy[1] < vy[0]) { DDA_SWAP(0, 1); tl = ((tl >> 1) & 0x1) | ((tl << 1) & 0x2) | (tl & 0x4); }
    if (vy[2] < vy[1]) { DDA_SWAP(1, 2); tl = ((tl >> 1) & 0x2) | ((tl << 1) & 0x4) | (tl & 0x1); }
    tl >>= 1;
    if (vy[0] == vy[2]) return;                        /* zero-height: nothing to fill */

    long long base_coord = dda_makefp(vx[0]);
    long long base_step  = dda_makestep(vx[2] - vx[0], vy[2] - vy[0]);
    long long bound_us   = (vy[1] == vy[0]) ? 0 : dda_makestep(vx[1] - vx[0], vy[1] - vy[0]);
    long long bound_ls   = (vy[2] == vy[1]) ? 0 : dda_makestep(vx[2] - vx[1], vy[2] - vy[1]);
    unsigned vo = (tl != 0) ? 1u : 0u;
    unsigned vp = (tl == 2) ? 3u : 0u;
    int right_facing = (vy[1] == vy[0]) ? (vx[1] > vx[0]) : (bound_us > base_step);
    unsigned rfi = right_facing ? 1u : 0u, ofi = right_facing ? 0u : 1u;

    long long det = (long long)(vx[1] - vx[0]) * (vy[2] - vy[1])
                  - (long long)(vx[2] - vx[1]) * (vy[1] - vy[0]);
    if (det == 0) return;

    DdaPart parts[2];
    DdaPart *tpo = &parts[vo], *tpp = &parts[vo ^ 1];
    tpo->start_y = vy[0 ^ vo]; tpo->end_y = vy[1 ^ vo];
    tpp->start_y = vy[1 ^ vp]; tpp->end_y = vy[2 ^ vp];
    tpo->start_x[rfi] = dda_makefp(vx[0 ^ vo]); tpo->step_x[rfi] = bound_us;
    tpo->start_x[ofi] = base_coord + (long long)(vy[vo] - vy[0]) * base_step; tpo->step_x[ofi] = base_step;
    tpo->upside_down = (int)vo;
    tpp->start_x[rfi] = dda_makefp(vx[1 ^ vp]); tpp->step_x[rfi] = bound_ls;
    tpp->start_x[ofi] = base_coord + (long long)(vy[1 ^ vp] - vy[0]) * base_step; tpp->step_x[ofi] = base_step;
    tpp->upside_down = (vp != 0) ? 1 : 0;

    DdaCtx cx;
    cx.r = r; cx.g = g; cx.bcol = bcol; cx.textured = textured;
    cx.tpage = tpage; cx.clut = clut; cx.semiTrans = semiTrans; cx.abr = abr;
    cx.clipL = rc->clipX * S; cx.clipR = (rc->clipX + rc->clipW) * S - 1;
    cx.clipT = rc->clipY * S; cx.clipB = (rc->clipY + rc->clipH) * S - 1;
    cx.rc = rc;
    cx.hdPx = NULL;
    if (hdReg) {   /* hoist the HD mapping constants + precompute the scale as a fixed-point multiply */
        int tpX, tpY, tp, ppw; TPageOrigin(tpage, &tpX, &tpY, &tp); ppw = (tp == 0) ? 4 : (tp == 1) ? 2 : 1;
        cx.hdPx = hdReg->px; cx.hdW = hdReg->w; cx.hdH = hdReg->h;
        cx.hdTpX = tpX; cx.hdTpY = tpY; cx.hdPpw = ppw;
        cx.hdRx = hdReg->rx; cx.hdRy = hdReg->ry; cx.hdRw = hdReg->rw; cx.hdRh = hdReg->rh;
        cx.hdSx = ((long long)hdReg->w << HD_SC) / (hdReg->rw * ppw);
        cx.hdSy = ((long long)hdReg->h << HD_SC) / hdReg->rh;
    }
    DdaUV origin; origin.u = origin.v = 0;
    if (textured) {
        /* ATTRIB_STEP(A,B) = (u32)(det(A,B)*(1<<12)/det) << 12, det(A,B)=(v1.A-v0.A)(v2.B-v1.B)-(v2.A-v1.A)(v1.B-v0.B) */
        long long d_uy = (long long)(vu[1]-vu[0])*(vy[2]-vy[1]) - (long long)(vu[2]-vu[1])*(vy[1]-vy[0]);
        long long d_vy = (long long)(vv[1]-vv[0])*(vy[2]-vy[1]) - (long long)(vv[2]-vv[1])*(vy[1]-vy[0]);
        long long d_xu = (long long)(vx[1]-vx[0])*(vu[2]-vu[1]) - (long long)(vx[2]-vx[1])*(vu[1]-vu[0]);
        long long d_xv = (long long)(vx[1]-vx[0])*(vv[2]-vv[1]) - (long long)(vx[2]-vx[1])*(vv[1]-vv[0]);
        cx.step.dudx = (unsigned)((d_uy * 4096) / det) << 12;
        cx.step.dvdx = (unsigned)((d_vy * 4096) / det) << 12;
        cx.step.dudy = (unsigned)((d_xu * 4096) / det) << 12;
        cx.step.dvdy = (unsigned)((d_xv * 4096) / det) << 12;
        dda_uv_init(&origin, vu[tl], vv[tl]);        /* seed at top-left vertex, then back to (0,0) */
        dda_stepx_n(&origin, &cx.step, -vx[tl]);
        dda_stepy_n(&origin, &cx.step, -vy[tl]);
        if (S > 1) {
            /* Sample at hi-res PIXEL centres, not native-pixel centres. dda_uv_init seeds +0.5 texel
             * (right for the native grid); at Sx with a ~1:1 texture that lands the sub-samples exactly
             * on texel boundaries (0.5,1.0,1.5,... at S=2), where fixed-point floor drops/duplicates
             * whole columns -- text/UI vertical strokes vanish or shift (the x2-vs-x4 phase difference).
             * Re-centre to the hi-res pixel: +0.5*(du/dx + du/dy) per axis. For MINIFIED textures
             * (terrain, >1 texel/pixel) this equals the +0.5 texel seed -> no change; for ~1:1 it becomes
             * +0.5/S texel, giving each source texel S evenly-spaced samples. Hi-res pass only. */
            /* "Crust for free": for perspective prims (!flat2d) we do NOT subtract the half-texel, which
             * shifts the hi-res sample +0.5 texel so tile edges land on the interior texel instead of the
             * dark border "crust" -- reproducing exactly what the legacy renderer does, removing the
             * tile-seam grid (and the compass "dotted lines") with NO softening, keeping the hardware
             * dither. Validated offline vs the real lava texture (gray-crust 598 -> 0, full detail kept).
             * Axis-aligned 2D UI/text (flat2d) keeps centre-sampling, so the bias never shifts glyph/
             * border columns (that shift was the 2D "vertical lines"). Decided per-quad in FillQuad so a
             * quad's two triangles always agree (a per-triangle split leaves a diagonal seam). */
            int half = flat2d ? (1 << (DDA_ASHIFT + DDA_APOST - 1)) : 0;
            origin.u += (unsigned)((((int)cx.step.dudx + (int)cx.step.dudy) / 2) - half);
            origin.v += (unsigned)((((int)cx.step.dvdx + (int)cx.step.dvdy) / 2) - half);
        }
    } else {
        cx.step.dudx = cx.step.dvdx = cx.step.dudy = cx.step.dvdy = 0;
    }
    dda_part(&cx, &parts[0], origin);
    dda_part(&cx, &parts[1], origin);
#undef DDA_SWAP
}

static void FillTriangle(const RenderCtx *rc, RVert a, RVert b, RVert c, int r, int g, int bcol,
                          int textured, int tpage, int clut, int semiTrans, int abr, int flat2d) {
    /* VH_ACCURATE (default) rasterizes via the fixed-point integer DDA -- coverage + UV at the integer
     * position, VRAM-faithful to DuckStation. The barycentric path below is only the VH_ACCURATE=0
     * legacy fallback (centre-sample coverage, floor UV, no dither). */
    if (AccurateEnabled()) { FillTriangleDDA(rc, a, b, c, r, g, bcol, textured, tpage, clut, semiTrans, abr, flat2d); return; }
    int S = rc->target ? rc->scale : 1;   /* G2: scale geometry by S, keep UVs native */
    int ox = rc->ofsX * S, oy = rc->ofsY * S;
    int minX, maxX, minY, maxY;
    int x, y;
    double area;

    a.x = a.x * S + ox; a.y = a.y * S + oy;
    b.x = b.x * S + ox; b.y = b.y * S + oy;
    c.x = c.x * S + ox; c.y = c.y * S + oy;

    minX = (int)floor(a.x < b.x ? (a.x < c.x ? a.x : c.x) : (b.x < c.x ? b.x : c.x));
    maxX = (int)ceil(a.x > b.x ? (a.x > c.x ? a.x : c.x) : (b.x > c.x ? b.x : c.x));
    minY = (int)floor(a.y < b.y ? (a.y < c.y ? a.y : c.y) : (b.y < c.y ? b.y : c.y));
    maxY = (int)ceil(a.y > b.y ? (a.y > c.y ? a.y : c.y) : (b.y > c.y ? b.y : c.y));

    if (minX < rc->clipX * S) minX = rc->clipX * S;
    if (minY < rc->clipY * S) minY = rc->clipY * S;
    if (maxX > (rc->clipX + rc->clipW) * S) maxX = (rc->clipX + rc->clipW) * S;
    if (maxY > (rc->clipY + rc->clipH) * S) maxY = (rc->clipY + rc->clipH) * S;

    area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (area == 0) return;

    for (y = minY; y < maxY; y++) {
        for (x = minX; x < maxX; x++) {
            double px = x + 0.5, py = y + 0.5;
            double w0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) / area;
            double w1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) / area;
            double w2 = 1.0 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            /* legacy VH_ACCURATE=0 path: centre-sampled floor UV, no dither, 8-bit blend (in PutPixel). */
            if (textured) {
                int u = (int)(w0 * a.u + w1 * b.u + w2 * c.u);
                int v = (int)(w0 * a.v + w1 * b.v + w2 * c.v);
                unsigned short texel = SampleTexture(rc, tpage, clut, u, v);
                int tr, tg, tb;
                if (texel == 0) continue; /* real hw: 0000h texel = transparent */
                UnpackColor(texel, &tr, &tg, &tb);
                tr = (tr * r) / 128; tg = (tg * g) / 128; tb = (tb * bcol) / 128;
                PutPixel(rc, x, y, PackColor(tr, tg, tb), abr, semiTrans && (texel & 0x8000), 1);
            } else {
                PutPixel(rc, x, y, PackColor(r, g, bcol), abr, semiTrans, 0);
            }
        }
    }
}

void FillQuad(const RenderCtx *rc, RVert v0, RVert v1, RVert v2, RVert v3, int r, int g, int b,
                      int textured, int tpage, int clut, int semiTrans, int abr) {
    /* psx-spx: "Quads are internally processed as two triangles, the first
     * consisting of vertices 1,2,3, and the second of vertices 2,3,4." */
    /* flat2d: does the whole quad project to an axis-aligned screen rectangle (UI/text/sprite)?
     * i.e. its 4 vertices span exactly two distinct X and two distinct Y (rounded) screen coords.
     * A perspective world tile is a diamond/parallelogram (>=3 distinct in one axis). Decided ONCE
     * here so both triangles share it -- see the "crust for free" bias in FillTriangleDDA. */
    int qx0 = (int)lround(v0.x), qx1 = (int)lround(v1.x), qx2 = (int)lround(v2.x), qx3 = (int)lround(v3.x);
    int qy0 = (int)lround(v0.y), qy1 = (int)lround(v1.y), qy2 = (int)lround(v2.y), qy3 = (int)lround(v3.y);
    int ndx = 1 + (qx1!=qx0) + ((qx2!=qx0)&&(qx2!=qx1)) + ((qx3!=qx0)&&(qx3!=qx1)&&(qx3!=qx2));
    int ndy = 1 + (qy1!=qy0) + ((qy2!=qy0)&&(qy2!=qy1)) + ((qy3!=qy0)&&(qy3!=qy1)&&(qy3!=qy2));
    int flat2d = (ndx == 2 && ndy == 2);
    FillTriangle(rc, v0, v1, v2, r, g, b, textured, tpage, clut, semiTrans, abr, flat2d);
    FillTriangle(rc, v1, v2, v3, r, g, b, textured, tpage, clut, semiTrans, abr, flat2d);
}

/* TILE's rect fill, subject to the current drawing offset/clip (it's a
 * regular render primitive, walked from the OT like any other). */
void FillRect(const RenderCtx *rc, int x0, int y0, int w, int h, int r, int g, int b) {
    int S = rc->target ? rc->scale : 1;   /* G2: scale geometry by S into the hires target */
    int ox = rc->ofsX * S, oy = rc->ofsY * S;
    int x, y;
    int minX = x0 * S + ox, minY = y0 * S + oy, maxX = minX + w * S, maxY = minY + h * S;
    unsigned short c = PackColor(r, g, b);

    if (minX < rc->clipX * S) minX = rc->clipX * S;
    if (minY < rc->clipY * S) minY = rc->clipY * S;
    if (maxX > (rc->clipX + rc->clipW) * S) maxX = (rc->clipX + rc->clipW) * S;
    if (maxY > (rc->clipY + rc->clipH) * S) maxY = (rc->clipY + rc->clipH) * S;

    for (y = minY; y < maxY; y++)
        for (x = minX; x < maxX; x++)
            PutPixel(rc, x, y, c, 0, 0, 0);
}

/* ClearImage maps to the raw "Quick Rectangle Fill" GPU command, which
 * per psx-spx operates on an absolute VRAM rect -- unlike TILE, it is not
 * a render primitive and isn't affected by the drawing offset or clip. */
void FillRectRaw(int x0, int y0, int w, int h, int r, int g, int b) {
    int x, y;
    unsigned short c = PackColor(r, g, b);
    for (y = y0; y < y0 + h && y < VRAM_H; y++)
        for (x = x0; x < x0 + w && x < VRAM_W; x++)
            if (x >= 0 && y >= 0) s_vram[y][x] = c;
    HiresMirrorRect(x0, y0, w, h);   /* G2: keep hires FB in sync (ClearImage / quick fill) */
}

/* ---- P1 step 2: deferred hi-res pass via a per-frame display list -------------------------------
 * Instead of drawing each hi-res primitive inline during the OT walk, DrawOTag APPENDS it here (with a
 * self-contained RenderCtx snapshot), then rasterizes the whole list AFTER the single-threaded native
 * walk. Because the list is flat + read-only, it can be rasterized by N per-band worker threads (each
 * clipped to a scanline band => disjoint hi-res pixels, lock-free). Reused across frames (count reset). */
typedef struct {
    int kind;                 /* 0 = quad (F4/FT4/SPRT), 1 = rect (TILE) */
    RVert v[4];               /* quad vertices */
    int rx, ry, rw, rh;       /* rect (TILE) */
    int r, g, b, textured, tpage, clut, semiTrans, abr;
    RenderCtx rc;             /* per-prim snapshot: target=1, scale, clip (full), dither, texture window */
} HiresPrim;
static HiresPrim *s_hprims = NULL;
static int s_hprimCount = 0, s_hprimCap = 0;

static HiresPrim *HiresPrimNext(void) {
    if (s_hprimCount >= s_hprimCap) {
        int nc = s_hprimCap ? s_hprimCap * 2 : 8192;
        HiresPrim *np = (HiresPrim *)realloc(s_hprims, (size_t)nc * sizeof(HiresPrim));
        if (!np) return NULL;
        s_hprims = np; s_hprimCap = nc;
    }
    return &s_hprims[s_hprimCount++];
}
void HiresAppendQuad(const RenderCtx *rch, RVert a, RVert b, RVert c, RVert d,
                            int r, int g, int bcol, int textured, int tpage, int clut, int semi, int abr) {
    HiresPrim *hp = HiresPrimNext(); if (!hp) return;
    hp->kind = 0; hp->v[0] = a; hp->v[1] = b; hp->v[2] = c; hp->v[3] = d;
    hp->r = r; hp->g = g; hp->b = bcol; hp->textured = textured;
    hp->tpage = tpage; hp->clut = clut; hp->semiTrans = semi; hp->abr = abr; hp->rc = *rch;
}
void HiresAppendRect(const RenderCtx *rch, int x, int y, int w, int h, int r, int g, int b) {
    HiresPrim *hp = HiresPrimNext(); if (!hp) return;
    hp->kind = 1; hp->rx = x; hp->ry = y; hp->rw = w; hp->rh = h;
    hp->r = r; hp->g = g; hp->b = b; hp->rc = *rch;
}

/* Rasterize the appended hi-res prims into the scanline band [clipY, clipY+clipH) (native units; the
 * fill functions scale by rc.scale). OT order preserved; overriding only clipY/clipH keeps threads on
 * disjoint rows. */
static void HiresRasterizeBand(int clipY, int clipH) {
    int i;
    for (i = 0; i < s_hprimCount; i++) {
        HiresPrim *hp = &s_hprims[i];
        RenderCtx rc = hp->rc;
        int semi = hp->semiTrans;
        rc.clipY = clipY; rc.clipH = clipH;
        if (hp->kind == 0)
            FillQuad(&rc, hp->v[0], hp->v[1], hp->v[2], hp->v[3], hp->r, hp->g, hp->b,
                     hp->textured, hp->tpage, hp->clut, semi, hp->abr);
        else
            FillRect(&rc, hp->rx, hp->ry, hp->rw, hp->rh, hp->r, hp->g, hp->b);
    }
}

/* P1 step 2b: rasterize the hi-res list across N worker threads, each owning a scanline band of the
 * drawing area. Bands write DISJOINT hi-res rows -> lock-free; the list is read-only. VH_RASTER_THREADS
 * overrides the count (default = online CPUs, capped). Falls back to single-threaded for a tiny list or
 * one thread. All lazy statics in the fill path are already warm here (the native pass ran first). */
#define HIRES_MAX_THREADS 32
typedef struct { int clipY, clipH; } HiresBand;

static int HiresThreadCount(void) {
    static int n = -1;
    if (n < 0) {
        const char *e = getenv("VH_RASTER_THREADS");
        if (e && atoi(e) > 0) n = atoi(e);
        else { int c = PC_CpuCount(); n = (c > 1) ? c : 1; }   /* OS-agnostic (SDL under the hood), see pc_platform.h */
        if (n > HIRES_MAX_THREADS) n = HIRES_MAX_THREADS;
        if (n < 1) n = 1;
        fprintf(stderr, "[raster] hi-res worker threads: %d%s\n", n, e ? " (VH_RASTER_THREADS)" : " (auto)");
    }
    return n;
}

/* PERF (1.6): PERSISTENT worker pool. Threads are created ONCE and reused every hi-res frame, driven by
 * a generation counter -- no per-frame pthread_create/join (that overhead was a few % of a 60fps budget,
 * paid on every HD frame). Bands write disjoint hi-res rows -> lock-free rasterization; the mutex/condvars
 * only gate the start/finish handshake. Idle workers (band clipH==0) complete instantly. */
static struct {
    int inited, nworkers;
    pthread_t th[HIRES_MAX_THREADS];
    HiresBand band[HIRES_MAX_THREADS];
    pthread_mutex_t mtx;
    pthread_cond_t start_cv, done_cv;
    unsigned gen;   /* bumped per dispatch */
    int done;       /* workers finished this gen */
} s_hpool;

static void *HiresPoolWorker(void *arg) {
    int id = (int)(uintptr_t)arg; unsigned seen = 0;   /* LLP64-safe: Win64 long is 32-bit */
    for (;;) {
        pthread_mutex_lock(&s_hpool.mtx);
        while (s_hpool.gen == seen) pthread_cond_wait(&s_hpool.start_cv, &s_hpool.mtx);
        seen = s_hpool.gen;
        pthread_mutex_unlock(&s_hpool.mtx);
        HiresRasterizeBand(s_hpool.band[id].clipY, s_hpool.band[id].clipH);
        pthread_mutex_lock(&s_hpool.mtx);
        if (++s_hpool.done == s_hpool.nworkers) pthread_cond_signal(&s_hpool.done_cv);
        pthread_mutex_unlock(&s_hpool.mtx);
    }
    return NULL;
}

static void HiresPoolInit(int nworkers) {
    int i;
    pthread_mutex_init(&s_hpool.mtx, NULL);
    pthread_cond_init(&s_hpool.start_cv, NULL);
    pthread_cond_init(&s_hpool.done_cv, NULL);
    s_hpool.nworkers = nworkers; s_hpool.gen = 0; s_hpool.done = 0;
    for (i = 0; i < nworkers; i++)
        if (pthread_create(&s_hpool.th[i], NULL, HiresPoolWorker, (void *)(uintptr_t)i) != 0) { s_hpool.nworkers = i; break; }
    s_hpool.inited = 1;
}

void HiresRasterizeThreaded(int clipY, int clipH) {
    int nth = HiresThreadCount();
    if (!s_hprimCount) return;   /* the walker used to skip the call for an empty list */
    /* Thread when the frame is heavy. Primitive COUNT is a poor proxy: a fullscreen HD-replaced
     * background is one primitive but millions of expensive per-pixel samples, so the count guard
     * (meant to skip trivial frames) would wrongly single-thread it -> also thread whenever an HD
     * replacement is loaded (1.6 HD pack). Cheap non-HD frames keep the single-thread fast path. */
    if (nth <= 1 || clipH < nth * 2 || (s_hprimCount < 64 && HdReplaceCount() == 0)) { HiresRasterizeBand(clipY, clipH); return; }
    (void)AccurateEnabled();                          /* warm the lazy cache before the parallel section */
    if (!s_hpool.inited) HiresPoolInit(nth - 1);      /* nth-1 workers; the main thread does one band too */
    {
        int W = s_hpool.nworkers, total = W + 1;      /* W workers + this thread */
        int per = clipH / total, rem = clipH % total, y = clipY, i, mainH;
        pthread_mutex_lock(&s_hpool.mtx);
        for (i = 0; i < W; i++) {                      /* contiguous bands; workers 0..W-1 then main = last */
            int h = per + (i < rem ? 1 : 0);
            s_hpool.band[i].clipY = y; s_hpool.band[i].clipH = h; y += h;
        }
        s_hpool.done = 0; s_hpool.gen++;              /* release workers */
        pthread_cond_broadcast(&s_hpool.start_cv);
        pthread_mutex_unlock(&s_hpool.mtx);
        mainH = per + (W < rem ? 1 : 0);
        HiresRasterizeBand(y, mainH);                 /* this thread's band, in parallel with the workers */
        pthread_mutex_lock(&s_hpool.mtx);
        while (s_hpool.done != W) pthread_cond_wait(&s_hpool.done_cv, &s_hpool.mtx);
        pthread_mutex_unlock(&s_hpool.mtx);
    }
}


/* SIGUSR2 (in libgpu.c) latches this alongside the native VRAM dump -- one signal writes a MATCHED
 * native + hires pair sharing the frame number. */
volatile sig_atomic_t g_vhHiresDumpReq = 0;

/* Write the presented hires display region to a PPM (idx picks the filename range). */
static void PC_WriteHiresPpm(int idx, int x, int y, int w, int h) {
    char path[512]; FILE *f; int px, py, W = VRAM_W * g_vhInternalScale;
    static const char *s_vramDumpDir; static int s_dirInit;
    if (!s_dirInit) { s_dirInit = 1; s_vramDumpDir = getenv("VH_VRAM_DUMP_DIR"); }
    if (!s_hires) return;
    if (s_vramDumpDir && s_vramDumpDir[0])
        snprintf(path, sizeof(path), "%s/vh_hires_%05d_f%06u.ppm", s_vramDumpDir, idx, s_drawFrame);
    else snprintf(path, sizeof(path), "vh_hires_%05d_f%06u.ppm", idx, s_drawFrame);
    f = fopen(path, "wb"); if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", w, h);
    for (py = 0; py < h; py++)
        for (px = 0; px < w; px++) {
            int r, g, b; UnpackColor(s_hires[(size_t)(y + py) * W + (x + px)], &r, &g, &b);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    fclose(f);
    fprintf(stderr, "[hiresdump] wrote %s\n", path);
}

/* Debug: dump the presented hires display region -- VH_HIRES_DUMP=N (every N frames) and/or the
 * on-demand SIGUSR2 trigger (matched with the native VRAM dump). */
static void PC_MaybeDumpHires(int x, int y, int w, int h) {
    static int s_init = 0, s_interval = 0, s_frame = 0, s_dumped = 0, s_max = 200;
    if (!s_init) {
        const char *e = getenv("VH_HIRES_DUMP");
        s_interval = (e && atoi(e) > 0) ? atoi(e) : 0;
        s_init = 1;
    }
    if (g_vhHiresDumpReq) {   /* on-demand: same 900xx range + frame number as the native dump */
        static int s_onDemand = 90000;
        g_vhHiresDumpReq = 0;
        PC_WriteHiresPpm(s_onDemand++, x, y, w, h);
    }
    if (s_interval == 0 || !s_hires) return;
    if (s_dumped >= s_max) return;
    if ((s_frame++ % s_interval) != 0) return;
    PC_WriteHiresPpm(s_dumped++, x, y, w, h);
}


/* ---- walker-facing wrappers (pc_gpu_internal.h) ------------------------------------------------ */

int HiresActive(void) { return InternalScale() > 1 && s_hires != NULL; }
void HiresFrameReset(void) { s_hprimCount = 0; }   /* P1: reset the per-frame hi-res display list */

/* Present the supersampled display region (was DrawOTag's hires-present branch; native units). */
void HiresPresent(int dispX, int dispY, int dispW, int dispH) {
    int S = InternalScale(), W = VRAM_W * S;
    int dx = dispX * S, dy = dispY * S, dw = dispW * S, dh = dispH * S;
    /* Edge-clamp: the last presented hi-res column/row is a dead zone -- no primitive's scaled span
     * quite reaches it, so it retains stale previous-frame content (a flickery 1px strip at the
     * right/bottom edge). It is the 2nd sub-pixel of the same native edge pixel as its neighbour, so
     * replicate the neighbour (correct-within-pixel, cheap). */
    {
        int i, lastx = dx + dw - 1, lasty = dy + dh - 1;
        for (i = dy; i < dy + dh; i++) s_hires[(size_t)i * W + lastx] = s_hires[(size_t)i * W + lastx - 1];
        for (i = dx; i < dx + dw; i++) s_hires[(size_t)lasty * W + i] = s_hires[(size_t)(lasty - 1) * W + i];
    }
    PC_MaybeDumpHires(dx, dy, dw, dh);
    PC_GpuPresent(s_hires, VRAM_W * S, VRAM_H * S, dx, dy, dw, dh);
}
