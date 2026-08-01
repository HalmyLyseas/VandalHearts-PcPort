/*
 * PC backend for the GPU (rendering) subsystem.
 *
 * A real 1MB VRAM buffer (512 lines x 1024 halfwords, BGR555 -- the actual
 * PS1 hardware pixel format, per psx-spx's "VRAM Overview/Addressing"), a
 * real Ordering-Table walk (AddPrim/ClearOTag/DrawOTag), and a software
 * rasterizer for the 4 primitive types the game actually uses (POLY_F4,
 * POLY_FT4, SPRT, TILE), plus real TIM texture-file parsing (format per
 * psx-spx's cdromfileformats.md -- a public, well-documented Sony SDK file
 * format, not proprietary expression). SDL2+OpenGL is used only for the
 * final step: blitting the finished VRAM contents to a window each frame --
 * matching the project's SDL2/OpenGL decision (exchange's
 * decision_phase_c_graphics_api note) and the "OpenLara-style" translation
 * the interface contract doc recommends (OT -> per-frame primitive list ->
 * rasterize, not a 1:1 GPU command re-submission).
 *
 * Texture sampling is intentionally nearest-neighbor / affine (no bilinear,
 * no perspective correction) -- this matches real PS1 GPU behavior exactly,
 * not a shortcut. A texel value of 0x0000 is treated as fully transparent
 * (skip the pixel), a well-documented real hardware quirk independent of
 * the semi-transparency flag.
 *
 * GetTPage/GetClut bit-packing and the 4bpp/8bpp CLUT-indexed texture
 * addressing formulas are taken directly from psx-spx's "Texpage Attribute"/
 * "Clut Attribute"/"VRAM Overview" sections, not guessed.
 */
#include <math.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <pthread.h>     /* P1 step 2b: band-parallel hi-res rasterization (winpthreads on MinGW) */

#include "PsyQ/libgpu.h"
#include "pc_platform.h"

#define VRAM_W 1024
#define VRAM_H 512

static unsigned short s_vram[VRAM_H][VRAM_W];
unsigned s_drawFrame = 0; /* incremented per DrawOTag; ties prim log to VRAM dumps + libgte VH_GTE_LOG */

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

static void HiresEnsure(void) {
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

/* Mirror a native VRAM rect into s_hires as SxS nearest blocks (bulk writes bypass the rasterizer). */
static void HiresMirrorRect(int x0, int y0, int w, int h) {
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

static DRAWENV s_drawEnv;
static DISPENV s_dispEnv;
static int s_drawModeAbr = 0;   /* last SetDrawMode-configured semi-trans mode */
static int s_drawModeDither = 0;/* GP0(E1h).9 dither-enable, from DRAWENV.dtd / SetDrawMode / DR_MODE.
                                 * The game runs the battle field with dtd=0 (split_0496f8.c:1083) so
                                 * terrain/UI must NOT be dithered; only dtd=1 scenes (e.g. some fx) are.
                                 * DuckStation gates dither on this exact bit -- honouring it stops us
                                 * over-dithering (fuzzy walls, fragmented water-tile edges). */
static int s_drawModeTPage = 0; /* last SetDrawMode-configured tpage -- SPRT/TILE
                                  * have no tpage field of their own on real hw
                                  * (unlike POLY_FT4), they use this instead */

/* Current GPU texture window (persistent state, GP0(E2h)), set by SetDrawMode's
 * `tw` RECT and applied per-texel in SampleTexture. Mask/Offset are in 8-pixel
 * steps (0..31 each); mask 0 = no windowing (full 256x256 page). Persists across
 * primitives/frames exactly like real hardware -- callers that want full-page
 * sampling re-arm it with a full-page window (see Map15 ocean, which brackets
 * its 32x32-windowed chunks with a w=0 reset). See psx-spx GPU GP0(E2h):
 *   Texcoord = (Texcoord AND NOT(Mask*8)) OR ((Offset AND Mask)*8)
 * Used e.g. by src/map_effects_0861c8.c Objf299_Map15_Ocean, which tiles a
 * single 32x32 water tile (MoveImage'd to the 576,256 page) across the sea via a
 * 32x32 window -- without this the chunks' 0..255 UVs sampled the mostly-empty
 * page (purple rectangles + noise bands during the sailing intro; bugreport-02). */
static int s_twMaskX = 0, s_twMaskY = 0, s_twOffX = 0, s_twOffY = 0;

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

static void UnpackColor(unsigned short c, int *r, int *g, int *b) {
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

/* Per-pass render context (Stage-3 1.5/P1 re-entrancy). The mutable rasterizer state the pixel/texture
 * path reads: clip band, draw offset, dither-enable, texture window, and the target (native s_vram vs the
 * Sx hi-res s_hires). Passed EXPLICITLY through the fill path instead of read from globals, so the hi-res
 * pass can later be split into per-band worker threads, each with its own ctx. The OT walk (DrawOTag)
 * still tracks the DR_MODE state in globals and snapshots it into a ctx per draw. */
typedef struct {
    int clipX, clipY, clipW, clipH;   /* drawing area, native units (y/h become the band under threading) */
    int ofsX, ofsY;                   /* draw offset, native units */
    int dither;                       /* dither-enable (GP0 E1h.9) */
    int twMaskX, twMaskY, twOffX, twOffY;  /* texture window (GP0 E2h) */
    int target;                       /* 0 = native s_vram, 1 = hi-res s_hires */
    int scale;                        /* geometry x scale when target==1 (else effectively 1) */
} RenderCtx;

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

/* ---- GetTPage / GetClut (psx-spx "Texpage Attribute" / "Clut Attribute") */

u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)(((x / 64) & 0xF) | (((y / 256) & 1) << 4) |
                      ((abr & 3) << 5) | ((tp & 3) << 7));
}

u_short GetClut(int x, int y) {
    return (u_short)(((x / 16) & 0x3F) | ((y & 0x1FF) << 6));
}

static void TPageOrigin(int tpage, int *x, int *y, int *tp) {
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

typedef struct { double x, y, u, v; } RVert;

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
} DdaCtx;

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
            int tr, tg, tb, su = dda_getu(&uv), sv = dda_getv(&uv);
            unsigned short texel;
            texel = SampleTexture(cx->rc, cx->tpage, cx->clut, su, sv);
            if (texel != 0) {                                /* 0000h = transparent (skip, still step) */
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
    RVert rv[3]; rv[0] = ra; rv[1] = rb; rv[2] = rvc;
    for (i = 0; i < 3; i++) {
        vx[i] = (int)lround(rv[i].x) * S + ox; vy[i] = (int)lround(rv[i].y) * S + oy;
        vu[i] = (int)lround(rv[i].u);          vv[i] = (int)lround(rv[i].v);
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

static void FillQuad(const RenderCtx *rc, RVert v0, RVert v1, RVert v2, RVert v3, int r, int g, int b,
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
static void FillRect(const RenderCtx *rc, int x0, int y0, int w, int h, int r, int g, int b) {
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
static void FillRectRaw(int x0, int y0, int w, int h, int r, int g, int b) {
    int x, y;
    unsigned short c = PackColor(r, g, b);
    for (y = y0; y < y0 + h && y < VRAM_H; y++)
        for (x = x0; x < x0 + w && x < VRAM_W; x++)
            if (x >= 0 && y >= 0) s_vram[y][x] = c;
    HiresMirrorRect(x0, y0, w, h);   /* G2: keep hires FB in sync (ClearImage / quick fill) */
}

/* ---- environment / init ------------------------------------------------- */

int ResetGraph(int mode) {
    (void)mode;
    memset(&s_drawEnv, 0, sizeof(s_drawEnv));
    memset(&s_dispEnv, 0, sizeof(s_dispEnv));
    return 0;
}

int SetGraphDebug(int level) { (void)level; return 0; }
void SetDispMask(int mask) { (void)mask; }
int DrawSync(int mode) { (void)mode; return 0; } /* rasterization is synchronous -- nothing to wait for */

DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h) {
    memset(env, 0, sizeof(*env));
    setRECT(&env->disp, x, y, w, h);
    setRECT(&env->screen, 0, 0, w, h);
    return env;
}

DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h) {
    memset(env, 0, sizeof(*env));
    setRECT(&env->clip, x, y, w, h);
    /* Real SetDefDrawEnv always sets the draw offset to match the clip
     * position -- without it (as this function did before), every
     * primitive draws at its raw, buffer-relative y0 (e.g. 0..240)
     * regardless of which of the two page-flip buffers is current, since
     * both buffers' clip.y differ (16 vs 272 -- see split_0496f8.c's
     * SetDefDrawEnv calls) but neither's ofs did anything to shift the
     * actual draw position to match. FillQuad/FillRect's clip-clamping
     * then makes this look inconsistent rather than uniformly wrong: one
     * buffer's clip.y (16) partially overlaps the unshifted 0..240 draw
     * range (clamped to 16..240, so some content survives), while the
     * other's clip.y (272) doesn't overlap it at all (clamped minY=272 >
     * maxY=240, an inverted/empty range -- zero pixels ever drawn). Found
     * via gdb: real content was consistently present at VRAM y=16 and
     * consistently absent at y=272 regardless of which buffer was
     * "current," which only makes sense if draws always land at the same
     * absolute position rather than being offset per buffer -- confirmed
     * as a real, reported flicker (title screen alternating between
     * correct and solid-black every other frame), not a hypothetical. */
    env->ofs[0] = (short)x;
    env->ofs[1] = (short)y;
    return env;
}

DISPENV *GetDispEnv(DISPENV *env) { *env = s_dispEnv; return env; }

DISPENV *PutDispEnv(DISPENV *env) {
    /* Presenting here (as an earlier version of this function did) doesn't
     * match how every real call site actually uses this API: all three
     * (engine.c, cd.c, split_0496f8.c) call PutDispEnv immediately followed
     * by DrawOTag -- on real hardware that's fine, since PutDispEnv just
     * arms the CRT's next scanout and the GPU's DMA (kicked off by
     * DrawOTag) finishes well before that scanout actually happens. This
     * backend's DrawOTag rasterizes synchronously, so presenting here
     * showed s_vram's content from BEFORE this frame's DrawOTag ran --
     * i.e., whichever content this same double-buffer slot held from its
     * *previous* draw, one full buffer-swap cycle stale every single
     * frame. Visible as flicker between correctly- and stale-looking
     * frames, reported after a real build+visual check, not a
     * hypothetical. Deferred to the end of DrawOTag instead -- see there. */
    s_dispEnv = *env;
    return env;
}

DRAWENV *PutDrawEnv(DRAWENV *env) {
    s_drawEnv = *env;
    s_drawModeDither = env->dtd ? 1 : 0;   /* GP0(E1h).9 -- persistent dither-enable state */
    return env;
}

void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw) {
    p->tag = 0;
    p->tpage = (unsigned int)tpage;
    setcode(p, PC_GPU_PRIM_DR_MODE);
    /* Carry the texture window (if any) in this prim's otherwise-unused r0/g0/b0
     * bytes so DrawOTag can apply it in OT order (real hw threads GP0(E2h) in
     * code[1]). Bit 23 flags "window present"; NULL tw means "leave the window
     * unchanged", matching PsyQ SetDrawMode(...,NULL). RECT.w/h are the window
     * size in pixels -> Mask = (256-size)>>3 (psx-spx tiling table: 32px -> 0x1c
     * -> wrap u&31); RECT.x/y are the window offset in pixels -> Offset = >>3. */
    /* bit 22 = dither-enable (dtd), always carried; bit 23 = texture-window present (below). */
    {
        u32 packed = dtd ? 0x400000u : 0;
        if (tw) {
            u32 mx = ((u32)(256 - tw->w) >> 3) & 0x1f;
            u32 my = ((u32)(256 - tw->h) >> 3) & 0x1f;
            u32 ox = ((u32)tw->x >> 3) & 0x1f;
            u32 oy = ((u32)tw->y >> 3) & 0x1f;
            packed |= 0x800000u | mx | (my << 5) | (ox << 10) | (oy << 15);
        }
        p->r0 = (u_char)(packed & 0xff);
        p->g0 = (u_char)((packed >> 8) & 0xff);
        p->b0 = (u_char)((packed >> 16) & 0xff);
    }
    (void)dfe;
}

/* ---- VRAM transfers ------------------------------------------------------ */

int LoadImage(RECT *rect, unsigned int *p) {
    unsigned short *src = (unsigned short *)p;
    int x, y;
    for (y = 0; y < rect->h; y++)
        for (x = 0; x < rect->w; x++)
            if (rect->y + y < VRAM_H && rect->x + x < VRAM_W)
                s_vram[rect->y + y][rect->x + x] = src[y * rect->w + x];
    HiresMirrorRect(rect->x, rect->y, rect->w, rect->h);   /* G2: keep hires FB in sync (backgrounds) */
    return 0;
}

int StoreImage(RECT *rect, unsigned int *p) {
    unsigned short *dst = (unsigned short *)p;
    int x, y;
    for (y = 0; y < rect->h; y++)
        for (x = 0; x < rect->w; x++)
            dst[y * rect->w + x] = (rect->y + y < VRAM_H && rect->x + x < VRAM_W)
                                        ? s_vram[rect->y + y][rect->x + x] : 0;
    return 0;
}

int MoveImage(RECT *rect, int x, int y) {
    int i, j;
    for (j = 0; j < rect->h; j++)
        for (i = 0; i < rect->w; i++) {
            int sx = rect->x + i, sy = rect->y + j, dx = x + i, dy = y + j;
            /* Guard BOTH bounds: a negative coord (from a garbage RECT) passes "< VRAM_W" and would
             * fault on s_vram. PSX masks off-VRAM coords into the framebuffer rather than faulting;
             * skipping the pixel is the safe equivalent. */
            if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 &&
                sx < VRAM_W && sy < VRAM_H && dx < VRAM_W && dy < VRAM_H)
                s_vram[dy][dx] = s_vram[sy][sx];
        }
    HiresMirrorRect(x, y, rect->w, rect->h);   /* G2: keep hires FB in sync (VRAM->VRAM blit) */
    return 0;
}

int ClearImage(RECT *rect, u_char r, u_char g, u_char b) {
    FillRectRaw(rect->x, rect->y, rect->w, rect->h, r, g, b);
    return 0;
}

/* ---- Ordering Table: the TOKEN BRIDGE (Stage 2.3, 2026-07-21) -------------
 *
 * An OT slot is 4 bytes and cannot be widened: `Graphics.ot` is `u32 ot[OT_SIZE]`
 * in include/graphics.h, a real decompiled struct. The previous implementation
 * stored a HOST POINTER truncated to 32 bits, which works only under -m32 and
 * was the single thing pinning the build to a 32-bit target.
 *
 * Instead of an address, a slot/tag now holds a **token**: a 1-based index into
 * a per-frame registry that owns the real (host-width) pointer. Tokens are 32
 * bits by construction, so the representation is pointer-width independent and
 * the same code is correct under -m32 and -m64.
 *
 *   token 0        = end of chain (what real hardware encodes as a NULL/terminator)
 *   token 1..N     = registry index; the entry carries the pointer AND whether the
 *                    target is a bare OT bucket rather than a drawable primitive
 *
 * This also RETIRES the old "bit 0 of the stored address means bucket" hack. That
 * trick stole an alignment bit because a raw pointer had nowhere to put the
 * distinction; real hardware gets it free from the tag word's separate `len`
 * field (len==0 => just a link). A registry entry has room for it properly.
 *
 * Lifetime: the registry resets in ClearOTag, which is the head of every frame's
 * ClearOTag -> AddPrim* -> DrawOTag cycle. Tokens are therefore valid for exactly
 * one frame, which is all the OT itself is.
 *
 * Note the surface this replaced was tiny: src/ touches the link ONLY through
 * AddPrim/addPrim (44 sites, 0 uses of setaddr/getaddr/termPrim/nextPrim), so
 * this needed no decompiled-source edits at all. */
#define PC_OT_MAX_TOKENS 65536

static void *s_otPtr[PC_OT_MAX_TOKENS];
static unsigned char s_otIsBucket[PC_OT_MAX_TOKENS];
static u32 s_otTokens;      /* highest token minted this frame; 0 = none */
static int s_otOverflowed;  /* latched, so the warning prints once per frame */

static void PC_OtResetTokens(void) { s_otTokens = 0; s_otOverflowed = 0; }

/* Mint a token for `p`. Returns 0 (= end of chain) on overflow, which drops the
 * tail of that bucket rather than corrupting memory -- and says so loudly, since
 * silently losing primitives is exactly the kind of failure that would otherwise
 * look like a subtle rendering bug. */
static u32 PC_OtMint(void *p, int isBucket) {
    if (s_otTokens + 1 >= PC_OT_MAX_TOKENS) {
        if (!s_otOverflowed) {
            fprintf(stderr, "[libgpu] OT token registry full (%d/frame) -- primitives dropped. "
                            "Raise PC_OT_MAX_TOKENS.\n", PC_OT_MAX_TOKENS);
            s_otOverflowed = 1;
        }
        return 0;
    }
    s_otTokens++;
    s_otPtr[s_otTokens] = p;
    s_otIsBucket[s_otTokens] = (unsigned char)isBucket;
    return s_otTokens;
}

static void *PC_OtResolve(u32 tok, int *isBucket) {
    if (tok == 0 || tok > s_otTokens) return NULL;   /* 0 = terminator; > = stale/garbage */
    *isBucket = s_otIsBucket[tok];
    return s_otPtr[tok];
}

/* ---- Ordering Table (see the header comment on the `tag` representation) */

unsigned int *ClearOTag(unsigned int *ot, int n) {
    /* ot really points at a u32[n] array (Graphics.ot in include/graphics.h)
     * -- see the libgpu.h file-header comment. Index through a u32* so each
     * slot write is 4 bytes, not 8.
     *
     * Real ClearOTag threads the whole array into ONE chain
     * (ot[0]->ot[1]->...->ot[n-1]->NULL) rather than zeroing each slot
     * independently: game code calls AddPrim at depth-sorted buckets other
     * than index 0 (ot[OT_SIZE-1], ot[2], ot+OT_SIZE-otz, ...) but always
     * calls DrawOTag(gGraphicsPtr->ot) -- i.e. starting the walk at ot[0].
     * Zeroing every slot left ot[0] permanently NULL regardless of what got
     * added elsewhere, so DrawOTag's walk terminated instantly every frame
     * -- confirmed via gdb (cur == NULL on every single DrawOTag call,
     * despite primitives genuinely being registered elsewhere in the same
     * table), a real, permanent black screen. Threading ot[0] through every
     * bucket up to the ot[n-1]->NULL terminator means AddPrim's insertions
     * at any bucket sit on the path DrawOTag actually walks. */
    u32 *slots = (u32 *)ot;
    int i;
    if (n <= 0) return ot;
    /* Frame boundary: ClearOTag is always the head of the
     * ClearOTag -> AddPrim* -> DrawOTag cycle (3 call sites in src/, each
     * paired with a DrawOTag), so this is where the token registry resets. */
    PC_OtResetTokens();
    for (i = 0; i < n - 1; i++) slots[i] = PC_OtMint(&slots[i + 1], 1);
    slots[n - 1] = 0;   /* token 0 == end of chain */
    return ot;
}

void AddPrim(void *ot, void *p) {
    /* `ot` points at a u32 OT bucket slot; `p` at a primitive struct whose
     * first word is its tag. Both hold TOKENS, never addresses -- see
     * PC_OtMint. Classic list insert at the head of this bucket. */
    u32 head = ((P_TAG *)ot)->tag;
    ((P_TAG *)p)->tag = head;
    ((P_TAG *)ot)->tag = PC_OtMint(p, 0);
}

/* VRAM snapshot probe: write the full 1024x512 VRAM as a binary PPM (BGR555 ->
 * RGB888) so a graphics bug can be inspected as an image -- texture pages, CLUT
 * rows, staging. The blue effect samples tpage 0x0036 (page origin 384,256) via
 * CLUT 0x7d28 (VRAM 640,500); the dump shows whether that region holds valid
 * texture/palette or garbage. Two triggers:
 *   - On demand (preferred for a long lead-in): `kill -USR2 <pid>` snaps the
 *     current frame. Unlimited; the signal handler just sets a flag (the write
 *     itself happens here, in the frame loop, since fopen/fwrite aren't
 *     async-signal-safe).
 *   - Periodic: VH_VRAM_DUMP=N dumps every N frames, capped at VH_VRAM_DUMP_MAX
 *     files (default 2000; set to 0 = UNLIMITED for a brute-force every-frame capture).
 *     VH_VRAM_DUMP_DIR=<path> writes the .ppm files there (default: cwd). */
static volatile sig_atomic_t s_vramDumpReq = 0;
static volatile sig_atomic_t s_hiresDumpReq = 0;   /* same SIGUSR2 also dumps the hires buffer (matched pair) */
static volatile sig_atomic_t s_tileLogReq = 0;     /* SIGUSR2 also latches a one-frame prim-list dump */
static unsigned s_tileLogFrame = 0;                /* the frame SIGUSR2 latched (VH_TILELOG) */
static const char *s_vramDumpDir = NULL;

/* DEBUG (VH_TILELOG=1 + `kill -USR2`): dump EVERY primitive in DRAW ORDER whose bounding box overlaps a
 * small region, on the SIGUSR2 frame -- so the prim that paints the seam pixels is identifiable (it's the
 * last one over a seam pixel). Region env-overridable via VH_TILELOG_BOX="x0,y0,x1,y1". Portable (the
 * SIGUSR2 trigger is POSIX-only; on Windows s_tileLogReq just stays 0 and this never fires). */
static void PrimLog(const char *kind, int n, int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                    int tpage, int clut, int semi, int abr, int r, int g, int b) {
    static int s_pl = -1; static int bx0 = 130, by0 = 95, bx1 = 178, by1 = 140;
    if (s_pl < 0) { const char *e = getenv("VH_TILELOG"); s_pl = e ? 1 : 0;
        const char *bb = getenv("VH_TILELOG_BOX"); if (bb) sscanf(bb, "%d,%d,%d,%d", &bx0, &by0, &bx1, &by1); }
    if (!s_pl || !s_tileLogFrame || s_drawFrame != s_tileLogFrame) return;
    { int xs[4] = {x0,x1,x2,x3}, ys[4] = {y0,y1,y2,y3}, i, mnx=99999, mxx=-99999, mny=99999, mxy=-99999;
      for (i = 0; i < n; i++) { if (xs[i]<mnx)mnx=xs[i]; if (xs[i]>mxx)mxx=xs[i]; if (ys[i]<mny)mny=ys[i]; if (ys[i]>mxy)mxy=ys[i]; }
      if (mxx < bx0 || mnx > bx1 || mxy < by0 || mny > by1) return;   /* no bbox overlap with region */
    }
    { static unsigned ord = 0, ordFrame = 0;
      if (ordFrame != s_drawFrame) { ord = 0; ordFrame = s_drawFrame; }
      fprintf(stderr, "[prim] ord=%u %-4s tpage=0x%04x clut=0x%04x semi=%d abr=%d rgb=(%d,%d,%d) "
              "xy=(%d,%d)(%d,%d)(%d,%d)(%d,%d)\n", ord++, kind, (unsigned)tpage, (unsigned)clut,
              semi, abr, r, g, b, x0, y0, x1, y1, x2, y2, x3, y3);
    }
}
#ifdef SIGUSR2   /* on-demand dump via `kill -USR2 <pid>` -- POSIX only; MinGW/Windows has no SIGUSR2.
                  * One signal writes a MATCHED native (vh_vram_900xx) + hires (vh_hires_900xx) pair,
                  * sharing the same frame number in the filename, for native-vs-hires comparison. */
static void PC_VramDumpSignal(int sig) { (void)sig; s_vramDumpReq = 1; s_hiresDumpReq = 1; s_tileLogReq = 1; }
#endif

static void PC_WriteVramPpm(int idx) {
    char path[512];
    FILE *f;
    int x, y;
    if (s_vramDumpDir && s_vramDumpDir[0])
        sprintf(path, "%.480s/vh_vram_%05d_f%06u.ppm", s_vramDumpDir, idx, s_drawFrame);
    else
        sprintf(path, "vh_vram_%05d_f%06u.ppm", idx, s_drawFrame);
    f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", VRAM_W, VRAM_H);
    for (y = 0; y < VRAM_H; y++)
        for (x = 0; x < VRAM_W; x++) {
            int r, g, b;
            UnpackColor(s_vram[y][x], &r, &g, &b);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
    fclose(f);
    fprintf(stderr, "[vramdump] wrote %s\n", path);
}

/* Write the presented hires display region to a PPM (idx picks the filename range). */
static void PC_WriteHiresPpm(int idx, int x, int y, int w, int h) {
    char path[512]; FILE *f; int px, py, W = VRAM_W * g_vhInternalScale;
    if (!s_hires) return;
    if (s_vramDumpDir && s_vramDumpDir[0])
        sprintf(path, "%.470s/vh_hires_%05d_f%06u.ppm", s_vramDumpDir, idx, s_drawFrame);
    else sprintf(path, "vh_hires_%05d_f%06u.ppm", idx, s_drawFrame);
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
    if (s_hiresDumpReq) {   /* on-demand: same 900xx range + frame number as the native dump */
        static int s_onDemand = 90000;
        s_hiresDumpReq = 0;
        PC_WriteHiresPpm(s_onDemand++, x, y, w, h);
    }
    if (s_interval == 0 || !s_hires) return;
    if (s_dumped >= s_max) return;
    if ((s_frame++ % s_interval) != 0) return;
    PC_WriteHiresPpm(s_dumped++, x, y, w, h);
}

static void PC_MaybeDumpVram(void) {
    static int s_init = 0;
    static int s_interval = 0;
    static int s_max = 2000;
    static int s_frame = 0;
    static int s_dumped = 0;
    if (!s_init) {
        const char *e = getenv("VH_VRAM_DUMP");
        const char *m = getenv("VH_VRAM_DUMP_MAX");
        s_interval = (e && atoi(e) > 0) ? atoi(e) : 0;
        if (m) s_max = atoi(m);            /* <= 0 => UNLIMITED (brute-force every-frame capture) */
        s_vramDumpDir = getenv("VH_VRAM_DUMP_DIR");
#ifdef SIGUSR2
        signal(SIGUSR2, PC_VramDumpSignal); /* on-demand trigger, always armed (POSIX only) */
#endif
        if (s_interval > 0)
            fprintf(stderr, "[vramdump] armed: every %d frame(s), max=%s, dir=%s "
                    "(~1.5 MB/frame)\n", s_interval, (s_max > 0) ? "capped" : "UNLIMITED",
                    (s_vramDumpDir && s_vramDumpDir[0]) ? s_vramDumpDir : "cwd");
        s_init = 1;
    }
    if (s_vramDumpReq) { /* on-demand: unlimited, distinct 900xx index range */
        static int s_onDemand = 90000;
        s_vramDumpReq = 0;
        PC_WriteVramPpm(s_onDemand++);
    }
    if (s_interval == 0) return;
    if (s_max > 0 && s_dumped >= s_max) return;   /* s_max <= 0 = unlimited */
    if ((s_frame++ % s_interval) != 0) return;
    PC_WriteVramPpm(s_dumped++);
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
static void HiresAppendQuad(const RenderCtx *rch, RVert a, RVert b, RVert c, RVert d,
                            int r, int g, int bcol, int textured, int tpage, int clut, int semi, int abr) {
    HiresPrim *hp = HiresPrimNext(); if (!hp) return;
    hp->kind = 0; hp->v[0] = a; hp->v[1] = b; hp->v[2] = c; hp->v[3] = d;
    hp->r = r; hp->g = g; hp->b = bcol; hp->textured = textured;
    hp->tpage = tpage; hp->clut = clut; hp->semiTrans = semi; hp->abr = abr; hp->rc = *rch;
}
static void HiresAppendRect(const RenderCtx *rch, int x, int y, int w, int h, int r, int g, int b) {
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
static void *HiresBandWorker(void *arg) { HiresBand *b = (HiresBand *)arg; HiresRasterizeBand(b->clipY, b->clipH); return NULL; }

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

static void HiresRasterizeThreaded(int clipY, int clipH) {
    int nth = HiresThreadCount();
    if (nth <= 1 || s_hprimCount < 64 || clipH < nth * 2) { HiresRasterizeBand(clipY, clipH); return; }
    if (nth > clipH) nth = clipH;
    {
        pthread_t th[HIRES_MAX_THREADS];
        HiresBand band[HIRES_MAX_THREADS];
        int per = clipH / nth, rem = clipH % nth, y = clipY, i, spawned = 0;
        (void)AccurateEnabled();                     /* warm the lazy cache before the parallel section */
        for (i = 0; i < nth; i++) {                   /* contiguous bands tile [clipY, clipY+clipH) exactly */
            int h = per + (i < rem ? 1 : 0);
            band[i].clipY = y; band[i].clipH = h; y += h;
        }
        for (i = 0; i < nth - 1; i++) {               /* spawn all but the last band... */
            if (pthread_create(&th[spawned], NULL, HiresBandWorker, &band[i]) == 0) spawned++;
            else HiresRasterizeBand(band[i].clipY, band[i].clipH);   /* spawn failed -> inline */
        }
        HiresRasterizeBand(band[nth - 1].clipY, band[nth - 1].clipH);   /* ...run the last on this thread */
        for (i = 0; i < spawned; i++) pthread_join(th[i], NULL);
    }
}

void DrawOTag(unsigned int *p) {
    /* VH_GPU_PRIM_LOG=1: dump "anomalous" primitives (our SetSemiTrans 0x80 bit set, or a code that
     * decodes to an unrecognized type) -- the fx SetSemiTrans-without-SetPolyFT4 effect polys we hunt.
     * Env checked once. */
    static int s_primLog = -1;
    u32 nextTok = ((P_TAG *)p)->tag;
    int hiScale = InternalScale();          /* G2: >1 => also rasterize each prim into s_hires at Sx */
    static int s_rtTime = -1; static clock_t s_rtAccum = 0; static unsigned s_rtFrames = 0; clock_t s_rtStart = 0;
    s_drawFrame++;
    if (s_tileLogReq) { s_tileLogFrame = s_drawFrame; s_tileLogReq = 0; }
    if (hiScale > 1) HiresEnsure();
    s_hprimCount = 0;                        /* P1: reset the per-frame hi-res display list */
    if (s_rtTime < 0) s_rtTime = getenv("VH_RASTER_TIME") ? 1 : 0;
    if (s_rtTime) s_rtStart = clock();
    if (s_primLog < 0) s_primLog = getenv("VH_GPU_PRIM_LOG") ? 1 : 0;
    while (nextTok) {
        int isBucket = 0;
        void *cur = PC_OtResolve(nextTok, &isBucket);
        u32 rawTagOfCur;
        if (cur == NULL) {
            /* A non-zero link that does not resolve means something wrote a RAW POINTER into a
             * tag instead of minting a token (the `addPrim` macro did exactly this until
             * 2026-07-21). Breaking here silently drops every primitive further down the chain --
             * which presented as "the compass, logo and all textboxes vanished while the 3D
             * looked perfect", and cost a full debug cycle to find. Never fail quietly again. */
            static int warned = 0;
            if (!warned) {
                fprintf(stderr, "[libgpu] OT walk aborted: link %u is not a valid token "
                                "(raw pointer written to a tag?). Everything after this point in "
                                "the ordering table is being DROPPED.\n", nextTok);
                warned = 1;
            }
            break;
        }
        rawTagOfCur = ((P_TAG *)cur)->tag;

        if (!isBucket) {
            int type = PC_GPU_PRIM_TYPE((P_TAG *)cur);
            int semi = PC_GPU_IS_SEMI((P_TAG *)cur) ? 1 : 0;

            if (s_primLog) {
                /* Log every semi-transparent prim + any unrecognized type, with blend mode (abr) and
                 * UVs -- so a wrong effect instance and a correct one both appear and can be diffed.
                 * (u0/v0/tpage are only meaningful for textured types; harmless raw dump otherwise.) */
                if (semi || type < 1 || type > 5) {
                    unsigned char code = getcode((P_TAG *)cur);
                    POLY_FT4 *q = (POLY_FT4 *)cur;
                    int abr = ((unsigned)q->tpage >> 5) & 3;
                    int tp = ((unsigned)q->tpage >> 7) & 3;
                    /* STP scan: which of the 16 CLUT entries carry bit15 (per-pixel
                     * semi-transparency). A textured semi poly only blends where the
                     * texel's STP is set -- if this mask is 0 the effect draws opaque.
                     * Reads raw s_vram (bit15 intact), unlike the RGB PPM dump. */
                    int clutX = ((unsigned)q->clut & 0x3f) * 16;
                    int clutY = ((unsigned)q->clut >> 6) & 0x1ff;
                    unsigned stp = 0; int i;
                    for (i = 0; i < 16; i++)
                        if (s_vram[clutY][clutX + i] & 0x8000) stp |= (1u << i);
                    fprintf(stderr,
                        "[gpuprim] f=%u code=0x%02x type=%d semi=%d abr=%d tp=%d tpage=0x%04x clut=0x%04x "
                        "uv0=(%u,%u) xy=(%d,%d)-(%d,%d) rgb=(%d,%d,%d) clutStp=0x%04x tw=mask(%d,%d)off(%d,%d)\n",
                        s_drawFrame, code, type, semi, abr, tp, (unsigned)q->tpage, (unsigned)q->clut,
                        q->u0, q->v0, q->x0, q->y0, q->x2, q->y2, q->r0, q->g0, q->b0, stp,
                        s_twMaskX, s_twMaskY, s_twOffX, s_twOffY);
                }
            }

            /* Build the per-pass render contexts from the current walk state (P1 re-entrancy): native
             * (target 0) always, hi-res (target 1, geometry ×hiScale) when supersampling is on. The
             * dual-write draws both. Snapshotting into a ctx here is what lets the hi-res pass later be
             * split into per-band worker threads. */
            RenderCtx rcn, rch;
            int hires = (hiScale > 1 && s_hires);
            rcn.clipX = s_drawEnv.clip.x; rcn.clipY = s_drawEnv.clip.y;
            rcn.clipW = s_drawEnv.clip.w; rcn.clipH = s_drawEnv.clip.h;
            rcn.ofsX = s_drawEnv.ofs[0];  rcn.ofsY = s_drawEnv.ofs[1];
            rcn.dither = s_drawModeDither;
            rcn.twMaskX = s_twMaskX; rcn.twMaskY = s_twMaskY; rcn.twOffX = s_twOffX; rcn.twOffY = s_twOffY;
            rcn.target = 0; rcn.scale = 1;
            rch = rcn; rch.target = 1; rch.scale = hiScale;

            switch (type) {
            case PC_GPU_PRIM_POLY_F4: {
                POLY_F4 *q = (POLY_F4 *)cur;
                RVert a = { q->x0, q->y0, 0, 0 }, b = { q->x1, q->y1, 0, 0 };
                RVert c = { q->x2, q->y2, 0, 0 }, d = { q->x3, q->y3, 0, 0 };
                PrimLog("F4", 4, q->x0,q->y0, q->x1,q->y1, q->x2,q->y2, q->x3,q->y3,
                        0, 0, semi, s_drawModeAbr, q->r0, q->g0, q->b0);
                FillQuad(&rcn, a, b, c, d, q->r0, q->g0, q->b0, 0, 0, 0, semi, s_drawModeAbr);
                if (hires) HiresAppendQuad(&rch, a, b, c, d, q->r0, q->g0, q->b0, 0, 0, 0, semi, s_drawModeAbr);
                break;
            }
            case PC_GPU_PRIM_POLY_FT4: {
                POLY_FT4 *q = (POLY_FT4 *)cur;
                RVert a = { q->x0, q->y0, q->u0, q->v0 }, b = { q->x1, q->y1, q->u1, q->v1 };
                RVert c = { q->x2, q->y2, q->u2, q->v2 }, d = { q->x3, q->y3, q->u3, q->v3 };
                PrimLog("FT4", 4, q->x0,q->y0, q->x1,q->y1, q->x2,q->y2, q->x3,q->y3,
                        q->tpage, q->clut, semi, (q->tpage>>5)&3, q->r0, q->g0, q->b0);
                FillQuad(&rcn, a, b, c, d, q->r0, q->g0, q->b0, 1, q->tpage, q->clut, semi, (q->tpage >> 5) & 3);
                if (hires) HiresAppendQuad(&rch, a, b, c, d, q->r0, q->g0, q->b0, 1, q->tpage, q->clut, semi, (q->tpage >> 5) & 3);
                break;
            }
            case PC_GPU_PRIM_SPRT: {
                SPRT *s = (SPRT *)cur;
                RVert a = { s->x0, s->y0, s->u0, s->v0 };
                RVert b = { s->x0 + s->w, s->y0, s->u0 + s->w, s->v0 };
                RVert c = { s->x0, s->y0 + s->h, s->u0, s->v0 + s->h };
                RVert d = { s->x0 + s->w, s->y0 + s->h, s->u0 + s->w, s->v0 + s->h };
                PrimLog("SPRT", 4, s->x0,s->y0, s->x0+s->w,s->y0, s->x0,s->y0+s->h, s->x0+s->w,s->y0+s->h,
                        s_drawModeTPage, s->clut, semi, s_drawModeAbr, s->r0, s->g0, s->b0);
                FillQuad(&rcn, a, b, c, d, s->r0, s->g0, s->b0, 1, s_drawModeTPage, s->clut, semi, s_drawModeAbr);
                if (hires) HiresAppendQuad(&rch, a, b, c, d, s->r0, s->g0, s->b0, 1, s_drawModeTPage, s->clut, semi, s_drawModeAbr);
                break;
            }
            case PC_GPU_PRIM_TILE: {
                TILE *t = (TILE *)cur;
                PrimLog("TILE", 4, t->x0,t->y0, t->x0+t->w,t->y0, t->x0,t->y0+t->h, t->x0+t->w,t->y0+t->h,
                        0, 0, 0, 0, t->r0, t->g0, t->b0);
                FillRect(&rcn, t->x0, t->y0, t->w, t->h, t->r0, t->g0, t->b0);
                if (hires) HiresAppendRect(&rch, t->x0, t->y0, t->w, t->h, t->r0, t->g0, t->b0);
                break;
            }
            case PC_GPU_PRIM_DR_MODE: {
                DR_MODE *m = (DR_MODE *)cur;
                u32 tw = (u32)m->r0 | ((u32)m->g0 << 8) | ((u32)m->b0 << 16);
                s_drawModeAbr = (int)((m->tpage >> 5) & 3);
                s_drawModeTPage = (int)m->tpage;
                s_drawModeDither = (int)((tw >> 22) & 1);  /* dtd carried in bit 22 */
                if (tw & 0x800000u) { /* texture window present -> update state */
                    s_twMaskX = tw & 0x1f;
                    s_twMaskY = (tw >> 5) & 0x1f;
                    s_twOffX = (tw >> 10) & 0x1f;
                    s_twOffY = (tw >> 15) & 0x1f;
                }
                break;
            }
            default:
                break;
            }
        }
        nextTok = rawTagOfCur;
    }


    /* P1: rasterize the deferred hi-res display list (the native pass drew inline above), fanned out
     * across per-band worker threads (step 2b). */
    if (hiScale > 1 && s_hires && s_hprimCount)
        HiresRasterizeThreaded(s_drawEnv.clip.y, s_drawEnv.clip.h);

    /* Present now, after this frame's rasterization is done -- see the
     * comment on PutDispEnv for why. Uses whichever DISPENV the most
     * recent PutDispEnv call recorded, matching every real call site's
     * PutDispEnv-then-DrawOTag pairing. */
    if (s_rtTime) { s_rtAccum += clock() - s_rtStart;
        if (++s_rtFrames >= 120) { fprintf(stderr, "[raster] scale=%d  mean %.0f us/frame CPU (%u frames)\n",
            hiScale, (double)s_rtAccum / CLOCKS_PER_SEC * 1e6 / s_rtFrames, s_rtFrames); s_rtAccum = 0; s_rtFrames = 0; } }
    PC_UpdateCamOsd(); /* refresh the debug pose label to match this frame (VH_CAM_OSD) */
    PC_MaybeDumpVram(); /* VH_VRAM_DUMP=N: snapshot VRAM to PPM for texture/CLUT triage */
    if (hiScale > 1 && s_hires) {          /* G2: present the supersampled display region */
        int S = hiScale, W = VRAM_W * S;
        int dx = s_dispEnv.disp.x * S, dy = s_dispEnv.disp.y * S;
        int dw = s_dispEnv.disp.w * S, dh = s_dispEnv.disp.h * S;
        /* Edge-clamp: the last presented hi-res column/row is a dead zone -- no primitive's scaled span
         * quite reaches it, so it retains stale previous-frame content (a flickery 1px strip at the
         * right/bottom edge). It is the 2nd sub-pixel of the same native edge pixel as its neighbour, so
         * replicate the neighbour (correct-within-pixel, cheap). */
        {
            int i, lastx = dx + dw - 1, lasty = dy + dh - 1;
            for (i = dy; i < dy + dh; i++) s_hires[(size_t)i * W + lastx] = s_hires[(size_t)i * W + lastx - 1];
            for (i = dx; i < dx + dw; i++) s_hires[(size_t)lasty * W + i] = s_hires[(size_t)(lasty - 1) * W + i];
        }
        PC_MaybeDumpHires(s_dispEnv.disp.x * S, s_dispEnv.disp.y * S, s_dispEnv.disp.w * S, s_dispEnv.disp.h * S);
        PC_GpuPresent(s_hires, VRAM_W * S, VRAM_H * S,
                      s_dispEnv.disp.x * S, s_dispEnv.disp.y * S,
                      s_dispEnv.disp.w * S, s_dispEnv.disp.h * S);
    } else {
        PC_GpuPresent(&s_vram[0][0], VRAM_W, VRAM_H,
                      s_dispEnv.disp.x, s_dispEnv.disp.y, s_dispEnv.disp.w, s_dispEnv.disp.h);
    }
}

/* ---- primitive initializers ---------------------------------------------- */

void SetSemiTrans(void *p, int abe) {
    if (abe) setcode((P_TAG *)p, (unsigned char)(getcode((P_TAG *)p) | 0x80));
    else setcode((P_TAG *)p, (unsigned char)(getcode((P_TAG *)p) & ~0x80));
}

void SetPolyF4(POLY_F4 *p) { p->tag = 0; setcode(p, PC_GPU_PRIM_POLY_F4); }
void SetPolyFT4(POLY_FT4 *p) { p->tag = 0; setcode(p, PC_GPU_PRIM_POLY_FT4); }
void SetSprt(SPRT *p) { p->tag = 0; setcode(p, PC_GPU_PRIM_SPRT); }
void SetTile(TILE *p) { p->tag = 0; setcode(p, PC_GPU_PRIM_TILE); }

/* ---- TIM texture file parsing --------------------------------------------
 * Format per psx-spx cdromfileformats.md
 * "TIM Format": 4-byte magic (10h), 4-byte mode (bit3 = has-CLUT, bits0-2 =
 * pixel type), then a CLUT section (if present) and a pixel section, each
 * shaped { unsigned int size; unsigned int destCoord (YyyyXxxxh); unsigned int whWord
 * (YsizXsizh); pixel data }.
 *
 * Real ReadTIM does NOT upload to VRAM itself -- it only parses the header
 * and hands back prect/crect (the TIM's own embedded target rect, purely
 * informational) and paddr/caddr, genuine pointers to the pixel/CLUT bytes
 * still sitting in the file buffer. The caller decides whether/where to
 * LoadImage() them -- confirmed by two real, different usage patterns:
 * screen_effects.c's LoadFullscreenImage() calls LoadImage(&timRect,
 * tim.paddr) with its OWN hardcoded rect (ignoring the TIM's embedded one),
 * and dojo.c does POINTER ARITHMETIC on tim.paddr (`tim.paddr + 0x1c20`) to
 * slice one TIM file into several separate LoadImage() calls. An earlier
 * version of this function uploaded to the TIM's embedded rect internally
 * with its paddr/caddr pointing at dummy zeroed static unsigned int placeholders --
 * every caller's LoadImage() then read pixel data starting from the address
 * of a single zeroed local variable, walking off into whatever adjacent
 * stack/data-segment bytes followed it (visible on screen as garbage that
 * looked suspiciously like host pointer values -- because it was). */

static unsigned int *s_openTimBase;

int OpenTIM(unsigned int *addr) {
    s_openTimBase = addr;
    return 0;
}

static unsigned int *ParseTimSection(unsigned int *sec, RECT *outRect, unsigned int **outData) {
    unsigned int destCoord = sec[1];
    unsigned int whWord = sec[2];
    RECT r;
    r.x = (short)(destCoord & 0xFFFF);
    r.y = (short)(destCoord >> 16);
    r.w = (short)(whWord & 0xFFFF);
    r.h = (short)(whWord >> 16);
    if (outRect) *outRect = r;
    if (outData) *outData = &sec[3];
    return sec + (sec[0] / sizeof(unsigned int));
}

TIM_IMAGE *ReadTIM(TIM_IMAGE *t) {
    unsigned int *cursor = s_openTimBase;
    unsigned int mode;

    cursor++; /* skip the 4-byte magic+version+reserved word */
    mode = *cursor++;
    t->mode = mode;

    if (mode & 8) {
        static RECT s_clutRect;
        static unsigned int *s_clutAddr;
        cursor = ParseTimSection(cursor, &s_clutRect, &s_clutAddr);
        t->crect = &s_clutRect;
        t->caddr = s_clutAddr;
    } else {
        t->crect = NULL;
        t->caddr = NULL;
    }

    {
        static RECT s_pixRect;
        static unsigned int *s_pixAddr;
        ParseTimSection(cursor, &s_pixRect, &s_pixAddr);
        t->prect = &s_pixRect;
        t->paddr = s_pixAddr;
    }

    return t;
}

/* ---- deferred (debug bitmap-font printer, not the game's own text.c) ---- */

int FntPrint(const char *fmt, ...) { (void)fmt; return 0; }
unsigned int *FntFlush(int id) { (void)id; return NULL; }
