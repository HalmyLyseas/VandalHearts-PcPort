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
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

#include "PsyQ/libgpu.h"
#include "pc_platform.h"

#define VRAM_W 1024
#define VRAM_H 512

static unsigned short s_vram[VRAM_H][VRAM_W];

static DRAWENV s_drawEnv;
static DISPENV s_dispEnv;
static int s_drawModeAbr = 0;   /* last SetDrawMode-configured semi-trans mode */
static int s_drawModeTPage = 0; /* last SetDrawMode-configured tpage -- SPRT/TILE
                                  * have no tpage field of their own on real hw
                                  * (unlike POLY_FT4), they use this instead */

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

static void PutPixel(int x, int y, unsigned short c, int abr, int semiTrans) {
    if (x < 0 || x >= VRAM_W || y < 0 || y >= VRAM_H) return;
    if (semiTrans) {
        int r, g, b, br, bg, bb;
        UnpackColor(c, &r, &g, &b);
        UnpackColor(s_vram[y][x], &br, &bg, &bb);
        switch (abr) {
        case 0: r = (br + r) / 2; g = (bg + g) / 2; b = (bb + b) / 2; break;
        case 1: r = br + r; g = bg + g; b = bb + b; break;
        case 2: r = br - r; g = bg - g; b = bb - b; break;
        case 3: r = br + r / 4; g = bg + g / 4; b = bb + b / 4; break;
        }
        s_vram[y][x] = PackColor(r, g, b);
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

static void FillTriangle(RVert a, RVert b, RVert c, int r, int g, int bcol,
                          int textured, int tpage, int clut, int semiTrans, int abr) {
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
                int u = (int)(w0 * a.u + w1 * b.u + w2 * c.u);
                int v = (int)(w0 * a.v + w1 * b.v + w2 * c.v);
                unsigned short texel = SampleTexture(tpage, clut, u, v);
                int tr, tg, tb;
                if (texel == 0) continue; /* real hw: 0000h texel = transparent */
                UnpackColor(texel, &tr, &tg, &tb);
                tr = (tr * r) / 128; tg = (tg * g) / 128; tb = (tb * bcol) / 128;
                PutPixel(x, y, PackColor(tr, tg, tb), abr, semiTrans && (texel & 0x8000));
            } else {
                PutPixel(x, y, PackColor(r, g, bcol), abr, semiTrans);
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
            PutPixel(x, y, c, 0, 0);
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
    return env;
}

void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw) {
    p->tag = 0;
    p->tpage = (u_long)tpage;
    setcode(p, PC_GPU_PRIM_DR_MODE);
    (void)dfe; (void)dtd; (void)tw;
}

/* ---- VRAM transfers ------------------------------------------------------ */

int LoadImage(RECT *rect, u_long *p) {
    unsigned short *src = (unsigned short *)p;
    int x, y;
    for (y = 0; y < rect->h; y++)
        for (x = 0; x < rect->w; x++)
            if (rect->y + y < VRAM_H && rect->x + x < VRAM_W)
                s_vram[rect->y + y][rect->x + x] = src[y * rect->w + x];
    return 0;
}

int StoreImage(RECT *rect, u_long *p) {
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
            if (sx < VRAM_W && sy < VRAM_H && dx < VRAM_W && dy < VRAM_H)
                s_vram[dy][dx] = s_vram[sy][sx];
        }
    return 0;
}

int ClearImage(RECT *rect, u_char r, u_char g, u_char b) {
    FillRectRaw(rect->x, rect->y, rect->w, rect->h, r, g, b);
    return 0;
}

/* ---- Ordering Table (see the header comment on the `tag` representation) */

u_long *ClearOTag(u_long *ot, int n) {
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
    /* Bit 0 of the stored address marks "this link's target is a bare OT
     * bucket, not a real primitive" (safe: real primitive structs are at
     * least 4-byte aligned, so AddPrim/setaddr never produces an odd
     * address). DrawOTag checks this bit before dispatching each node so
     * it can skip drawing bucket-chain slots instead of misreading their
     * raw bytes as a primitive's code/fields -- real hardware gets the
     * same distinction for free via the tag word's separate `len` field
     * (len==0 means "just a link"), which direct pointer storage has no
     * room for. See the libgpu.h file-header comment for why tag holds a
     * direct pointer at all (only valid for this -m32 build). */
    for (i = 0; i < n - 1; i++) slots[i] = ((u32)(size_t)&slots[i + 1]) | 1u;
    slots[n - 1] = 0;
    return ot;
}

void AddPrim(void *ot, void *p) {
    setaddr(p, getaddr(ot));
    setaddr(ot, p);
}

void DrawOTag(u_long *p) {
    u32 rawNext = ((P_TAG *)p)->tag;
    while (rawNext) {
        int isBucket = (int)(rawNext & 1u);
        void *cur = (void *)(size_t)(rawNext & ~1u);
        u32 rawTagOfCur = ((P_TAG *)cur)->tag;

        if (!isBucket) {
            int type = PC_GPU_PRIM_TYPE((P_TAG *)cur);
            int semi = PC_GPU_IS_SEMI((P_TAG *)cur) ? 1 : 0;

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
                s_drawModeAbr = (int)((m->tpage >> 5) & 3);
                s_drawModeTPage = (int)m->tpage;
                break;
            }
            default:
                break;
            }
        }
        rawNext = rawTagOfCur;
    }

    /* Present now, after this frame's rasterization is done -- see the
     * comment on PutDispEnv for why. Uses whichever DISPENV the most
     * recent PutDispEnv call recorded, matching every real call site's
     * PutDispEnv-then-DrawOTag pairing. */
    PC_UpdateCamOsd(); /* refresh the debug pose label to match this frame (VH_CAM_OSD) */
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
 * Format per exchange/09-phase-c-gpu-backend.md (psx-spx cdromfileformats.md
 * "TIM Format"): 4-byte magic (10h), 4-byte mode (bit3 = has-CLUT, bits0-2 =
 * pixel type), then a CLUT section (if present) and a pixel section, each
 * shaped { u_long size; u_long destCoord (YyyyXxxxh); u_long whWord
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
 * with its paddr/caddr pointing at dummy zeroed static u_long placeholders --
 * every caller's LoadImage() then read pixel data starting from the address
 * of a single zeroed local variable, walking off into whatever adjacent
 * stack/data-segment bytes followed it (visible on screen as garbage that
 * looked suspiciously like host pointer values -- because it was). */

static u_long *s_openTimBase;

int OpenTIM(u_long *addr) {
    s_openTimBase = addr;
    return 0;
}

static u_long *ParseTimSection(u_long *sec, RECT *outRect, u_long **outData) {
    u_long destCoord = sec[1];
    u_long whWord = sec[2];
    RECT r;
    r.x = (short)(destCoord & 0xFFFF);
    r.y = (short)(destCoord >> 16);
    r.w = (short)(whWord & 0xFFFF);
    r.h = (short)(whWord >> 16);
    if (outRect) *outRect = r;
    if (outData) *outData = &sec[3];
    return sec + (sec[0] / sizeof(u_long));
}

TIM_IMAGE *ReadTIM(TIM_IMAGE *t) {
    u_long *cursor = s_openTimBase;
    u_long mode;

    cursor++; /* skip the 4-byte magic+version+reserved word */
    mode = *cursor++;
    t->mode = mode;

    if (mode & 8) {
        static RECT s_clutRect;
        static u_long *s_clutAddr;
        cursor = ParseTimSection(cursor, &s_clutRect, &s_clutAddr);
        t->crect = &s_clutRect;
        t->caddr = s_clutAddr;
    } else {
        t->crect = NULL;
        t->caddr = NULL;
    }

    {
        static RECT s_pixRect;
        static u_long *s_pixAddr;
        ParseTimSection(cursor, &s_pixRect, &s_pixAddr);
        t->prect = &s_pixRect;
        t->paddr = s_pixAddr;
    }

    return t;
}

/* ---- deferred (debug bitmap-font printer, not the game's own text.c) ---- */

int FntPrint(const char *fmt, ...) { (void)fmt; return 0; }
u_long *FntFlush(int id) { (void)id; return NULL; }
