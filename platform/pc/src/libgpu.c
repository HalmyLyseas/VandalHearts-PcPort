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

#include "PsyQ/libgpu.h"
#include "pc_platform.h"

#define VRAM_W 1024
#define VRAM_H 512

static unsigned short s_vram[VRAM_H][VRAM_W];
unsigned s_drawFrame = 0; /* incremented per DrawOTag; ties prim log to VRAM dumps + libgte VH_GTE_LOG */

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

/* PSX GPU ordered dither (GP0(E1h).9). The hardware adds this 4x4 signed offset to the 24-bit R/G/B
 * *after* texture-blend + semi-transparency and *before* the 24->15-bit truncation, breaking hard 5-bit
 * steps into a fine stipple that reads as a smooth gradient. Our backend never did this, so blended
 * gradients (e.g. the additive casting-ray effect) show hard "blocky" steps where hardware looks smooth.
 * Experimental / opt-in via VH_DITHER=1 while we evaluate it (known-issues casting-ray residual). */
static const signed char DITHER4[4][4] = {
    { -4,  0, -3,  1 },
    {  2, -2,  3, -1 },
    { -3,  1, -4,  0 },
    {  3, -1,  2, -2 },
};
static int DitherEnabled(void) {
    static int e = -1;
    if (e < 0) e = getenv("VH_DITHER") ? 1 : 0;
    return e;
}
static unsigned short PackColorDither(int r, int g, int b, int x, int y) {
    if (DitherEnabled()) {
        int d = DITHER4[y & 3][x & 3];
        r += d; g += d; b += d;
    }
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

static int imin4(int a, int b, int c, int d) {
    int m = a; if (b < m) m = b; if (c < m) m = c; if (d < m) m = d; return m;
}
static int imax4(int a, int b, int c, int d) {
    int m = a; if (b > m) m = b; if (c > m) m = c; if (d > m) m = d; return m;
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

static void PutPixel(int x, int y, unsigned short c, int abr, int semiTrans, int textured) {
    if (x < 0 || x >= VRAM_W || y < 0 || y >= VRAM_H) return;
    if (semiTrans) {
        if (AccurateEnabled()) {
            s_vram[y][x] = BlendG1((unsigned)c | 0x8000u, s_vram[y][x], abr, textured);
        } else {
            int r, g, b, br, bg, bb;
            UnpackColor(c, &r, &g, &b);
            UnpackColor(s_vram[y][x], &br, &bg, &bb);
            switch (abr) {
            case 0: r = (br + r) / 2; g = (bg + g) / 2; b = (bb + b) / 2; break;
            case 1: r = br + r; g = bg + g; b = bb + b; break;
            case 2: r = br - r; g = bg - g; b = bb - b; break;
            case 3: r = br + r / 4; g = bg + g / 4; b = bb + b / 4; break;
            }
            s_vram[y][x] = PackColorDither(r, g, b, x, y);
        }
    } else {
        s_vram[y][x] = c;
    }
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

static unsigned short SampleTexture(int tpage, int clut, int u, int v) {
    int tpX, tpY, tp;
    int clutX = (clut & 0x3F) * 16;
    int clutY = (clut >> 6) & 0x1FF;
    unsigned short raw;

    /* Apply the active texture window (GP0(E2h)); mask 0 leaves u/v untouched. */
    u = (u & ~(s_twMaskX * 8)) | ((s_twOffX & s_twMaskX) * 8);
    v = (v & ~(s_twMaskY * 8)) | ((s_twOffY & s_twMaskY) * 8);

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
 * Opt-in via VH_DDA=1 while validated on the harness; folds into VH_ACCURATE once proven multi-scene. */
static int DdaEnabled(void) {
    static int e = -1;
    if (e < 0) { const char *v = getenv("VH_DDA"); e = (v && v[0] != '0') ? 1 : 0; }
    return e;
}
#define DDA_ASHIFT 12
#define DDA_APOST  12
typedef struct { unsigned u, v; } DdaUV;
typedef struct { unsigned dudx, dvdx, dudy, dvdy; } DdaUVStep;
typedef struct { int start_y, end_y; long long start_x[2], step_x[2]; int upside_down; } DdaPart;
typedef struct {
    int r, g, bcol, textured, tpage, clut, semiTrans, abr;
    int clipL, clipR, clipT, clipB;   /* inclusive drawing area */
    DdaUVStep step;
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
    do {
        if (cx->textured) {
            unsigned short texel = SampleTexture(cx->tpage, cx->clut, dda_getu(&uv), dda_getv(&uv));
            if (texel != 0) {                                /* 0000h = transparent (skip, still step) */
                int tr, tg, tb; UnpackColor(texel, &tr, &tg, &tb);
                tr = (tr * cx->r) / 128; tg = (tg * cx->g) / 128; tb = (tb * cx->bcol) / 128;
                PutPixel(current_x, y,
                         (AccurateEnabled() && s_drawModeDither) ? PackColorG1Front(tr, tg, tb, current_x, y)
                                                                 : PackColor(tr, tg, tb),
                         cx->abr, cx->semiTrans && (texel & 0x8000), 1);
            }
        } else {
            PutPixel(current_x, y,
                     (AccurateEnabled() && s_drawModeDither) ? PackColorG1Front(cx->r, cx->g, cx->bcol, current_x, y)
                                                             : PackColor(cx->r, cx->g, cx->bcol),
                     cx->abr, cx->semiTrans, 0);
        }
        current_x++;
        if (cx->textured) { uv.u += cx->step.dudx; uv.v += cx->step.dvdx; }
    } while (--width > 0);
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

static void FillTriangleDDA(RVert ra, RVert rb, RVert rc, int r, int g, int bcol,
                            int textured, int tpage, int clut, int semiTrans, int abr) {
    int ox = s_drawEnv.ofs[0], oy = s_drawEnv.ofs[1];
    int vx[3], vy[3], vu[3], vv[3], i;
    RVert rv[3]; rv[0] = ra; rv[1] = rb; rv[2] = rc;
    for (i = 0; i < 3; i++) {
        vx[i] = (int)lround(rv[i].x) + ox; vy[i] = (int)lround(rv[i].y) + oy;
        vu[i] = (int)lround(rv[i].u);      vv[i] = (int)lround(rv[i].v);
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
    cx.clipL = s_drawEnv.clip.x; cx.clipR = s_drawEnv.clip.x + s_drawEnv.clip.w - 1;
    cx.clipT = s_drawEnv.clip.y; cx.clipB = s_drawEnv.clip.y + s_drawEnv.clip.h - 1;
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
    } else {
        cx.step.dudx = cx.step.dvdx = cx.step.dudy = cx.step.dvdy = 0;
    }
    dda_part(&cx, &parts[0], origin);
    dda_part(&cx, &parts[1], origin);
#undef DDA_SWAP
}

static void FillTriangle(RVert a, RVert b, RVert c, int r, int g, int bcol,
                          int textured, int tpage, int clut, int semiTrans, int abr) {
    if (DdaEnabled()) { FillTriangleDDA(a, b, c, r, g, bcol, textured, tpage, clut, semiTrans, abr); return; }
    int ox = s_drawEnv.ofs[0], oy = s_drawEnv.ofs[1];
    int minX, maxX, minY, maxY;
    int x, y;
    double area;

    a.x += ox; a.y += oy;
    b.x += ox; b.y += oy;
    c.x += ox; c.y += oy;

    minX = (int)floor(a.x < b.x ? (a.x < c.x ? a.x : c.x) : (b.x < c.x ? b.x : c.x));
    maxX = (int)ceil(a.x > b.x ? (a.x > c.x ? a.x : c.x) : (b.x > c.x ? b.x : c.x));
    minY = (int)floor(a.y < b.y ? (a.y < c.y ? a.y : c.y) : (b.y < c.y ? b.y : c.y));
    maxY = (int)ceil(a.y > b.y ? (a.y > c.y ? a.y : c.y) : (b.y > c.y ? b.y : c.y));

    if (minX < s_drawEnv.clip.x) minX = s_drawEnv.clip.x;
    if (minY < s_drawEnv.clip.y) minY = s_drawEnv.clip.y;
    if (maxX > s_drawEnv.clip.x + s_drawEnv.clip.w) maxX = s_drawEnv.clip.x + s_drawEnv.clip.w;
    if (maxY > s_drawEnv.clip.y + s_drawEnv.clip.h) maxY = s_drawEnv.clip.y + s_drawEnv.clip.h;

    area = (b.x - a.x) * (c.y - a.y) - (b.y - a.y) * (c.x - a.x);
    if (area == 0) return;

    for (y = minY; y < maxY; y++) {
        for (x = minX; x < maxX; x++) {
            double px = x + 0.5, py = y + 0.5;
            double w0 = ((b.x - px) * (c.y - py) - (b.y - py) * (c.x - px)) / area;
            double w1 = ((c.x - px) * (a.y - py) - (c.y - py) * (a.x - px)) / area;
            double w2 = 1.0 - w0 - w1;
            if (w0 < 0 || w1 < 0 || w2 < 0) continue;

            if (textured) {
                int u, v;
                /* Rule 2 (gotcha #2), scoped to semi-transparent effects only. The corner-round UV
                 * (round-to-nearest via the pixel's INTEGER corner + a +0.5-texel bias -- the GPU's
                 * DDA seed) is the casting-ray density fix. But our coverage is CENTRE-sampled
                 * (x+0.5,y+0.5) while this UV is corner-sampled, so on an OPAQUE tile edge a pixel
                 * whose centre is inside but corner is outside the triangle gets extrapolated
                 * corner-weights -> UV past the tile -> a wrong/neighbour texel = fuzzy stones,
                 * fragmented water mortar, compass bleed (user-validated 2026-07-31). Thin additive
                 * effect quads hide that; abutting opaque tiles do not. So opaque textured polys use
                 * the centre-consistent UV below (same as the legacy path -- no extrapolation).
                 * Trade-off accepted: this is NOT VRAM-faithful to DuckStation for opaque terrain
                 * (DuckStation corner-rounds everything; the one-frame cutscene metric drops to ~54%),
                 * but it renders the battle field correctly, which is what the default must do. A full
                 * fixed-point integer DDA (coverage + UV both at the integer position, no float
                 * extrapolation) is the only way to be both faithful and clean -- deferred. */
                if (AccurateEnabled() && semiTrans) {
                    double cw0 = ((b.x - x) * (c.y - y) - (b.y - y) * (c.x - x)) / area;
                    double cw1 = ((c.x - x) * (a.y - y) - (c.y - y) * (a.x - x)) / area;
                    double cw2 = 1.0 - cw0 - cw1;
                    u = (int)floor(cw0 * a.u + cw1 * b.u + cw2 * c.u + 0.5) & 0xFF;
                    v = (int)floor(cw0 * a.v + cw1 * b.v + cw2 * c.v + 0.5) & 0xFF;
                } else {
                    u = (int)(w0 * a.u + w1 * b.u + w2 * c.u);
                    v = (int)(w0 * a.v + w1 * b.v + w2 * c.v);
                }
                unsigned short texel = SampleTexture(tpage, clut, u, v);
                int tr, tg, tb;
                if (texel == 0) continue; /* real hw: 0000h texel = transparent */
                UnpackColor(texel, &tr, &tg, &tb);
                tr = (tr * r) / 128; tg = (tg * g) / 128; tb = (tb * bcol) / 128;
                PutPixel(x, y,
                         (AccurateEnabled() && s_drawModeDither) ? PackColorG1Front(tr, tg, tb, x, y)
                                                                 : PackColor(tr, tg, tb),
                         abr, semiTrans && (texel & 0x8000), 1);
            } else {
                PutPixel(x, y,
                         (AccurateEnabled() && s_drawModeDither) ? PackColorG1Front(r, g, bcol, x, y)
                                                                 : PackColor(r, g, bcol),
                         abr, semiTrans, 0);
            }
        }
    }
}

static void FillQuad(RVert v0, RVert v1, RVert v2, RVert v3, int r, int g, int b,
                      int textured, int tpage, int clut, int semiTrans, int abr) {
    /* psx-spx: "Quads are internally processed as two triangles, the first
     * consisting of vertices 1,2,3, and the second of vertices 2,3,4." */
    FillTriangle(v0, v1, v2, r, g, b, textured, tpage, clut, semiTrans, abr);
    FillTriangle(v1, v2, v3, r, g, b, textured, tpage, clut, semiTrans, abr);
}

/* TILE's rect fill, subject to the current drawing offset/clip (it's a
 * regular render primitive, walked from the OT like any other). */
static void FillRect(int x0, int y0, int w, int h, int r, int g, int b) {
    int ox = s_drawEnv.ofs[0], oy = s_drawEnv.ofs[1];
    int x, y;
    int minX = x0 + ox, minY = y0 + oy, maxX = minX + w, maxY = minY + h;
    unsigned short c = PackColor(r, g, b);

    if (minX < s_drawEnv.clip.x) minX = s_drawEnv.clip.x;
    if (minY < s_drawEnv.clip.y) minY = s_drawEnv.clip.y;
    if (maxX > s_drawEnv.clip.x + s_drawEnv.clip.w) maxX = s_drawEnv.clip.x + s_drawEnv.clip.w;
    if (maxY > s_drawEnv.clip.y + s_drawEnv.clip.h) maxY = s_drawEnv.clip.y + s_drawEnv.clip.h;

    for (y = minY; y < maxY; y++)
        for (x = minX; x < maxX; x++)
            PutPixel(x, y, c, 0, 0, 0);
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
static const char *s_vramDumpDir = NULL;
#ifdef SIGUSR2   /* on-demand VRAM dump via `kill -USR2 <pid>` -- POSIX only; MinGW/Windows has no SIGUSR2 */
static void PC_VramDumpSignal(int sig) { (void)sig; s_vramDumpReq = 1; }
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

void DrawOTag(unsigned int *p) {
    /* VH_GPU_PRIM_LOG=1: dump "anomalous" primitives (our SetSemiTrans 0x80 bit set, or a code that
     * decodes to an unrecognized type) -- the fx SetSemiTrans-without-SetPolyFT4 effect polys we hunt.
     * Env checked once. */
    static int s_primLog = -1;
    u32 nextTok = ((P_TAG *)p)->tag;
    s_drawFrame++;
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

            /* VH_FXALL=1: comprehensive casting-effect region log. Every handled primitive type
             * (F4/FT4/SPRT/TILE), semi OR opaque, whose screen bbox lies in the effect area --
             * correctly decoding each type's own geometry/colour (unlike VH_GPU_PRIM_LOG, which
             * always reads a POLY_FT4). Finds the blue-blob primitive that VH_RAYLOG (semi-FT4
             * only) misses -- the port's logged FT4 quads are the magenta sparkles, not the blob. */
            {
                static int s_fxAll = -1;
                if (s_fxAll < 0) s_fxAll = getenv("VH_FXALL") ? 1 : 0;
                if (s_fxAll) {
                    int bx0 = 0, by0 = 0, bx1 = -1, by1 = -1, r = 0, g = 0, b = 0, tpg = -1, cl = -1;
                    const char *tn = 0;
                    if (type == PC_GPU_PRIM_POLY_F4) { POLY_F4 *q = (POLY_F4 *)cur; tn = "F4";
                        bx0 = imin4(q->x0,q->x1,q->x2,q->x3); bx1 = imax4(q->x0,q->x1,q->x2,q->x3);
                        by0 = imin4(q->y0,q->y1,q->y2,q->y3); by1 = imax4(q->y0,q->y1,q->y2,q->y3);
                        r = q->r0; g = q->g0; b = q->b0;
                    } else if (type == PC_GPU_PRIM_POLY_FT4) { POLY_FT4 *q = (POLY_FT4 *)cur; tn = "FT4";
                        bx0 = imin4(q->x0,q->x1,q->x2,q->x3); bx1 = imax4(q->x0,q->x1,q->x2,q->x3);
                        by0 = imin4(q->y0,q->y1,q->y2,q->y3); by1 = imax4(q->y0,q->y1,q->y2,q->y3);
                        r = q->r0; g = q->g0; b = q->b0; tpg = q->tpage; cl = q->clut;
                    } else if (type == PC_GPU_PRIM_SPRT) { SPRT *s = (SPRT *)cur; tn = "SPRT";
                        bx0 = s->x0; by0 = s->y0; bx1 = s->x0 + s->w; by1 = s->y0 + s->h;
                        r = s->r0; g = s->g0; b = s->b0; cl = s->clut;
                    } else if (type == PC_GPU_PRIM_TILE) { TILE *t = (TILE *)cur; tn = "TILE";
                        bx0 = t->x0; by0 = t->y0; bx1 = t->x0 + t->w; by1 = t->y0 + t->h;
                        r = t->r0; g = t->g0; b = t->b0;
                    }
                    if (tn && bx1 >= 60 && bx0 <= 270 && by1 >= 90 && by0 <= 230)
                        fprintf(stderr, "[fxall] f=%u %-4s code=0x%02x semi=%d bbox=(%d,%d)-(%d,%d) %dx%d "
                                "rgb=(%d,%d,%d) tpage=0x%04x clut=0x%04x\n",
                                s_drawFrame, tn, getcode((P_TAG *)cur), semi, bx0, by0, bx1, by1,
                                bx1 - bx0, by1 - by0, r, g, b, tpg & 0xffff, cl & 0xffff);
                }
            }

            /* VH_RAYLOG=1: targeted casting-ray diagnostic (bugreport-02 follow-up). For each semi
             * POLY_FT4 quad, print full corners + the coverage/sampling ratio: |screen area| (shoelace)
             * vs |texel area| (shoelace over UVs). ratio>1 => MAGNIFY (few texels stretched over many
             * pixels -> streaks); ratio<1 => MINIFY (texels dropped -> sparkle). Directly tests whether
             * our blue over-fills vs hardware. Small output: only fires on the effect's own quads. */
            {
                static int s_rayLog = -1;
                if (s_rayLog < 0) s_rayLog = getenv("VH_RAYLOG") ? 1 : 0;
                if (s_rayLog && semi && type == PC_GPU_PRIM_POLY_FT4) {
                    POLY_FT4 *q = (POLY_FT4 *)cur;
                    /* PSX FT4 verts are Z-order (TL,TR,BL,BR); quad area = 0.5*|diag(3-0) x diag(2-1)|. */
                    double sa = 0.5 * fabs((double)(q->x3 - q->x0) * (q->y2 - q->y1)
                                         - (double)(q->y3 - q->y0) * (q->x2 - q->x1));
                    double ta = 0.5 * fabs((double)(q->u3 - q->u0) * (q->v2 - q->v1)
                                         - (double)(q->v3 - q->v0) * (q->u2 - q->u1));
                    fprintf(stderr,
                        "[raylog] f=%u tpage=0x%04x clut=0x%04x abr=%d rgb=(%d,%d,%d) "
                        "uv=(%u,%u)(%u,%u)(%u,%u)(%u,%u) xy=(%d,%d)(%d,%d)(%d,%d)(%d,%d) "
                        "screenArea=%.1f texelArea=%.1f ratio=%.2f\n",
                        s_drawFrame, (unsigned)q->tpage, (unsigned)q->clut,
                        ((unsigned)q->tpage >> 5) & 3, q->r0, q->g0, q->b0,
                        q->u0, q->v0, q->u1, q->v1, q->u2, q->v2, q->u3, q->v3,
                        q->x0, q->y0, q->x1, q->y1, q->x2, q->y2, q->x3, q->y3,
                        sa, ta, ta > 0.0 ? sa / ta : 0.0);
                }
            }

            switch (type) {
            case PC_GPU_PRIM_POLY_F4: {
                POLY_F4 *q = (POLY_F4 *)cur;
                RVert a = { q->x0, q->y0, 0, 0 }, b = { q->x1, q->y1, 0, 0 };
                RVert c = { q->x2, q->y2, 0, 0 }, d = { q->x3, q->y3, 0, 0 };
                FillQuad(a, b, c, d, q->r0, q->g0, q->b0, 0, 0, 0, semi, s_drawModeAbr);
                break;
            }
            case PC_GPU_PRIM_POLY_FT4: {
                POLY_FT4 *q = (POLY_FT4 *)cur;
                RVert a = { q->x0, q->y0, q->u0, q->v0 }, b = { q->x1, q->y1, q->u1, q->v1 };
                RVert c = { q->x2, q->y2, q->u2, q->v2 }, d = { q->x3, q->y3, q->u3, q->v3 };
                FillQuad(a, b, c, d, q->r0, q->g0, q->b0, 1, q->tpage, q->clut, semi, (q->tpage >> 5) & 3);
                break;
            }
            case PC_GPU_PRIM_SPRT: {
                SPRT *s = (SPRT *)cur;
                RVert a = { s->x0, s->y0, s->u0, s->v0 };
                RVert b = { s->x0 + s->w, s->y0, s->u0 + s->w, s->v0 };
                RVert c = { s->x0, s->y0 + s->h, s->u0, s->v0 + s->h };
                RVert d = { s->x0 + s->w, s->y0 + s->h, s->u0 + s->w, s->v0 + s->h };
                FillQuad(a, b, c, d, s->r0, s->g0, s->b0, 1, s_drawModeTPage, s->clut, semi, s_drawModeAbr);
                break;
            }
            case PC_GPU_PRIM_TILE: {
                TILE *t = (TILE *)cur;
                FillRect(t->x0, t->y0, t->w, t->h, t->r0, t->g0, t->b0);
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

    /* Present now, after this frame's rasterization is done -- see the
     * comment on PutDispEnv for why. Uses whichever DISPENV the most
     * recent PutDispEnv call recorded, matching every real call site's
     * PutDispEnv-then-DrawOTag pairing. */
    PC_UpdateCamOsd(); /* refresh the debug pose label to match this frame (VH_CAM_OSD) */
    PC_MaybeDumpVram(); /* VH_VRAM_DUMP=N: snapshot VRAM to PPM for texture/CLUT triage */
    PC_GpuPresent(&s_vram[0][0], VRAM_W, VRAM_H,
                  s_dispEnv.disp.x, s_dispEnv.disp.y, s_dispEnv.disp.w, s_dispEnv.disp.h);
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
