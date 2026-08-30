/* PC backend for the PSX GPU: the PsyQ libgpu API surface, VRAM transfers into the 1 MB BGR555
 * buffer, the ordering-table token bridge and the DrawOTag walker. Rasterization lives in
 * pc_raster.c. See docs/pc-port/subsystems/gpu.md. */
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "PsyQ/libgpu.h"
#include "pc_lang.h"       /* PC_LangPatchFwdUpload: pack glyphs stamped into the F_WD sheet upload */
#include "pc_platform.h"
#include "pc_gpu_internal.h"   /* seams with the extracted subsystem TUs (pc_gpu_trace.c, ...) */

#define VRAM_W 1024
#define VRAM_H 512

unsigned s_drawFrame = 0; /* incremented per DrawOTag; ties prim log to VRAM dumps + libgte VH_GTE_LOG */

static DRAWENV s_drawEnv;
static DISPENV s_dispEnv;
static int s_drawModeAbr = 0;   /* last SetDrawMode-configured semi-trans mode */
static int s_drawModeDither = 0;/* GP0(E1h).9 dither-enable, from DRAWENV.dtd / SetDrawMode / DR_MODE.
                                 * The battle field runs with dtd=0 (states/game_setup.c), so terrain/UI
                                 * must NOT be dithered; only dtd=1 scenes (some effects) are. */
static int s_drawModeTPage = 0; /* last SetDrawMode-configured tpage -- SPRT/TILE have no tpage
                                  * field of their own on real hardware (unlike POLY_FT4) */

/* Texture window state (GP0(E2h)): set by SetDrawMode's `tw` RECT via DR_MODE, applied per texel in
 * SampleTexture, and persistent across primitives and frames like real hardware. Mask/Offset are in
 * 8-pixel steps; mask 0 = full page. See docs/pc-port/subsystems/gpu.md, "Texture window (GP0 E2h)". */
static int s_twMaskX = 0, s_twMaskY = 0, s_twOffX = 0, s_twOffY = 0;

/* ---- GetTPage / GetClut (psx-spx "Texpage Attribute" / "Clut Attribute") */

u_short GetTPage(int tp, int abr, int x, int y) {
    return (u_short)(((x / 64) & 0xF) | (((y / 256) & 1) << 4) |
                      ((abr & 3) << 5) | ((tp & 3) << 7));
}

u_short GetClut(int x, int y) {
    return (u_short)(((x / 16) & 0x3F) | ((y & 0x1FF) << 6));
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
    /* PsyQ's SetDefDrawEnv also sets the draw offset to the clip origin. The game's two page-flip
     * buffers differ only in clip.y (16 vs 272, states/game_setup.c); without the offset every
     * primitive lands at its raw y and one buffer never receives a pixel (alternating black frames). */
    env->ofs[0] = (short)x;
    env->ofs[1] = (short)y;
    return env;
}

DISPENV *GetDispEnv(DISPENV *env) { *env = s_dispEnv; return env; }

DISPENV *PutDispEnv(DISPENV *env) {
    /* Only records the DISPENV; presentation happens at the end of DrawOTag. Every call site pairs
     * PutDispEnv with an immediate DrawOTag and this backend rasterizes synchronously, so presenting
     * here would show the slot's previous contents. See docs/pc-port/subsystems/gpu.md, "Presentation". */
    s_dispEnv = *env;
    return env;
}

DRAWENV *PutDrawEnv(DRAWENV *env) {
    TrcWrite('E', NULL, 0, env, (u32)sizeof(*env));
    s_drawEnv = *env;
    s_drawModeDither = env->dtd ? 1 : 0;   /* GP0(E1h).9 -- persistent dither-enable state */
    return env;
}

void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw) {
    p->tag = 0;
    p->tpage = (unsigned int)tpage;
    setcode(p, PC_GPU_PRIM_DR_MODE);
    /* Pack dtd (bit 22) and the texture window (bit 23 = present; Mask = (256-size)>>3, Offset = pos>>3,
     * five bits each) into the unused r0/g0/b0 bytes so DrawOTag applies them in OT order, as hardware
     * threads GP0(E2h) through the packet stream. NULL tw leaves the window unchanged, matching PsyQ. */
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

/* ---- VRAM transfers ------------------------------------------------------
 * The TrcInit/TrcWrite calls feed the GPU trace record/replay regression harness (pc_gpu_trace.c;
 * seams in pc_gpu_internal.h). */

int LoadImage(RECT *rect, unsigned int *p) {
    unsigned short *src = (unsigned short *)p;
    unsigned short (*vram)[VRAM_W] = PC_GpuVram();
    int x, y;
    TrcInit();
    /* Language pack (pc_lang_font.c): when this upload is the F_WD glyph sheet, the pack's glyphs are
     * stamped into the SOURCE buffer first, so VRAM, the hi-res mirror, the trace and the HD hash all see
     * one image and every re-upload (LoadFWD runs per scene) re-applies it. No-op without a pack. */
    PC_LangPatchFwdUpload(rect->x, rect->y, rect->w, rect->h, src);
    if (rect->w > 0 && rect->h > 0) TrcWrite('L', rect, 8, src, (u32)(rect->w * rect->h * 2));
    for (y = 0; y < rect->h; y++)
        for (x = 0; x < rect->w; x++)
            if (rect->y + y < VRAM_H && rect->x + x < VRAM_W)
                vram[rect->y + y][rect->x + x] = src[y * rect->w + x];
    HiresMirrorRect(rect->x, rect->y, rect->w, rect->h);   /* keep the hi-res FB in sync (backgrounds) */
    if (!TrcReplaying()) HdPack_OnLoad(rect, src);         /* HD pack: hash + replace/dump (env-gated) */
    return 0;
}

int StoreImage(RECT *rect, unsigned int *p) {
    unsigned short *dst = (unsigned short *)p;
    unsigned short (*vram)[VRAM_W] = PC_GpuVram();
    int x, y;
    for (y = 0; y < rect->h; y++)
        for (x = 0; x < rect->w; x++)
            dst[y * rect->w + x] = (rect->y + y < VRAM_H && rect->x + x < VRAM_W)
                                        ? vram[rect->y + y][rect->x + x] : 0;
    return 0;
}

int MoveImage(RECT *rect, int x, int y) {
    unsigned short (*vram)[VRAM_W] = PC_GpuVram();
    int i, j;
    { int xy[2]; xy[0] = x; xy[1] = y; TrcWrite('M', rect, 8, xy, 8); }
    for (j = 0; j < rect->h; j++)
        for (i = 0; i < rect->w; i++) {
            int sx = rect->x + i, sy = rect->y + j, dx = x + i, dy = y + j;
            /* Guard BOTH bounds: a negative coord (from a garbage RECT) passes "< VRAM_W" and would
             * fault on s_vram. PSX masks off-VRAM coords into the framebuffer rather than faulting;
             * skipping the pixel is the safe equivalent. */
            if (sx >= 0 && sy >= 0 && dx >= 0 && dy >= 0 &&
                sx < VRAM_W && sy < VRAM_H && dx < VRAM_W && dy < VRAM_H)
                vram[dy][dx] = vram[sy][sx];
        }
    HiresMirrorRect(x, y, rect->w, rect->h);   /* keep the hi-res FB in sync (VRAM->VRAM blit) */
    return 0;
}

int ClearImage(RECT *rect, u_char r, u_char g, u_char b) {
    { unsigned char rgb[3]; rgb[0] = r; rgb[1] = g; rgb[2] = b; TrcWrite('C', rect, 8, rgb, 3); }
    FillRectRaw(rect->x, rect->y, rect->w, rect->h, r, g, b);
    return 0;
}

/* ---- Ordering Table: the token bridge. An OT slot is a 4-byte u32 (Graphics.ot, include/graphics.h)
 * and cannot hold a host pointer, so a slot/tag stores a 1-based token into this per-frame registry
 * (0 = end of chain). See docs/pc-port/subsystems/gpu.md, "The ordering table and the link-token bridge". */
#define PC_OT_MAX_TOKENS 65536

static void *s_otPtr[PC_OT_MAX_TOKENS];
static unsigned char s_otIsBucket[PC_OT_MAX_TOKENS];
static u32 s_otTokens;      /* highest token minted this frame; 0 = none */
static int s_otOverflowed;  /* latched, so the warning prints once per frame */

void PC_OtResetTokens(void) { s_otTokens = 0; s_otOverflowed = 0; }   /* non-static: the trace replay mints its own chains (pc_gpu_internal.h) */

/* Mint a token for `p`. On overflow returns 0 (end of chain), which drops the tail of that bucket
 * rather than corrupting memory, and warns once per frame: silently losing primitives would look
 * like a subtle rendering bug. */
u32 PC_OtMint(void *p, int isBucket) {
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

/* ---- Ordering Table API */

unsigned int *ClearOTag(unsigned int *ot, int n) {
    /* `ot` is a u32[n] array (Graphics.ot); index through a u32* so each slot write is 4 bytes.
     * Thread the whole array into ONE chain (ot[0]->ot[1]->...->ot[n-1]->0): the game inserts at
     * depth-sorted buckets but always walks from ot[0], so every bucket must sit on that path. */
    u32 *slots = (u32 *)ot;
    int i;
    if (n <= 0) return ot;
    /* Frame boundary: ClearOTag is always the head of the ClearOTag -> AddPrim* -> DrawOTag cycle
     * (every call site in src/ pairs it with a DrawOTag), so this is where the token registry resets. */
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

/* VRAM snapshot probe: writes the full 1024x512 VRAM as a binary PPM (BGR555 -> RGB888). Triggers:
 * `kill -USR2 <pid>` snaps the current frame (the handler only sets a flag; fopen is not signal-safe),
 * or VH_VRAM_DUMP=N every N frames, capped by VH_VRAM_DUMP_MAX (0 = unlimited), into VH_VRAM_DUMP_DIR. */
static volatile sig_atomic_t s_vramDumpReq = 0;
static volatile sig_atomic_t s_tileLogReq = 0;     /* SIGUSR2 also latches a one-frame prim-list dump */
static unsigned s_tileLogFrame = 0;                /* the frame SIGUSR2 latched (VH_TILELOG) */
static const char *s_vramDumpDir = NULL;

/* Debug (VH_TILELOG=1 + `kill -USR2`): on the latched frame, log every primitive in draw order whose
 * bounding box overlaps a small region (VH_TILELOG_BOX="x0,y0,x1,y1"), so the prim that paints a given
 * pixel is identifiable. SIGUSR2 is POSIX-only; on Windows s_tileLogReq stays 0 and this never fires. */
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
static void PC_VramDumpSignal(int sig) { (void)sig; s_vramDumpReq = 1; g_vhHiresDumpReq = 1; s_tileLogReq = 1; }
#endif

static void PC_WriteVramPpm(int idx) {
    char path[512];
    FILE *f;
    int x, y;
    if (s_vramDumpDir && s_vramDumpDir[0])
        snprintf(path, sizeof(path), "%s/vh_vram_%05d_f%06u.ppm", s_vramDumpDir, idx, s_drawFrame);
    else
        snprintf(path, sizeof(path), "vh_vram_%05d_f%06u.ppm", idx, s_drawFrame);
    f = fopen(path, "wb");
    if (!f) return;
    fprintf(f, "P6\n%d %d\n255\n", VRAM_W, VRAM_H);
    { unsigned short (*vram)[VRAM_W] = PC_GpuVram();
    for (y = 0; y < VRAM_H; y++)
        for (x = 0; x < VRAM_W; x++) {
            int r, g, b;
            UnpackColor(vram[y][x], &r, &g, &b);
            fputc(r, f); fputc(g, f); fputc(b, f);
        }
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
    /* VH_GPU_PRIM_LOG=1: dump anomalous primitives (SetSemiTrans 0x80 bit set, or a code that decodes
     * to an unrecognized type), e.g. effect polys that call SetSemiTrans without SetPolyFT4. Env read once. */
    static int s_primLog = -1;
    u32 nextTok = ((P_TAG *)p)->tag;
    unsigned long walkSteps = 0;   /* fail-soft cycle guard -- see the cap check at the loop tail */
    int hiScale = PC_GpuGetInternalScale(); /* >1 => also rasterize each prim into the hi-res buffer at Sx */
    static int s_rtTime = -1; static clock_t s_rtAccum = 0; static unsigned s_rtFrames = 0; clock_t s_rtStart = 0;
    TrcInit();
    s_drawFrame++;
    if (s_tileLogReq) { s_tileLogFrame = s_drawFrame; s_tileLogReq = 0; }
    HiresEnsure();                           /* self-gated: allocates when scale > 1 OR a langpack
                                              * ships localized backgrounds (shadow pass at 1x) */
    HiresFrameReset();                       /* reset the per-frame hi-res display list */
    if (s_rtTime < 0) s_rtTime = getenv("VH_RASTER_TIME") ? 1 : 0;
    if (s_rtTime) s_rtStart = clock();
    if (s_primLog < 0) s_primLog = getenv("VH_GPU_PRIM_LOG") ? 1 : 0;
    while (nextTok) {
        int isBucket = 0;
        void *cur = PC_OtResolve(nextTok, &isBucket);
        u32 rawTagOfCur;
        if (cur == NULL) {
            /* A non-zero link that does not resolve means a RAW POINTER was written into a tag instead
             * of a minted token (only AddPrim/ClearOTag may link). Breaking here silently drops every
             * primitive further down the chain, so warn once rather than fail quietly. */
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

            TrcPrim(type, cur);                    /* trace record: raw struct bytes, walk order */

            if (s_primLog) {
                /* Log every semi-transparent prim + any unrecognized type, with blend mode (abr) and
                 * UVs -- so a wrong effect instance and a correct one both appear and can be diffed.
                 * (u0/v0/tpage are only meaningful for textured types; harmless raw dump otherwise.) */
                if (semi || type < 1 || type > 5) {
                    unsigned char code = getcode((P_TAG *)cur);
                    POLY_FT4 *q = (POLY_FT4 *)cur;
                    int abr = ((unsigned)q->tpage >> 5) & 3;
                    int tp = ((unsigned)q->tpage >> 7) & 3;
                    /* STP scan: which of the 16 CLUT entries carry bit15 (per-pixel semi-transparency).
                     * A textured semi poly only blends where the texel's STP is set; mask 0 = draws opaque.
                     * Reads raw VRAM (bit15 intact), unlike the RGB PPM dump. */
                    int clutX = ((unsigned)q->clut & 0x3f) * 16;
                    int clutY = ((unsigned)q->clut >> 6) & 0x1ff;
                    unsigned stp = 0; int i;
                    unsigned short (*vram)[VRAM_W] = PC_GpuVram();
                    for (i = 0; i < 16; i++)
                        if (vram[clutY][clutX + i] & 0x8000) stp |= (1u << i);
                    fprintf(stderr,
                        "[gpuprim] f=%u code=0x%02x type=%d semi=%d abr=%d tp=%d tpage=0x%04x clut=0x%04x "
                        "uv0=(%u,%u) xy=(%d,%d)-(%d,%d) rgb=(%d,%d,%d) clutStp=0x%04x tw=mask(%d,%d)off(%d,%d)\n",
                        s_drawFrame, code, type, semi, abr, tp, (unsigned)q->tpage, (unsigned)q->clut,
                        q->u0, q->v0, q->x0, q->y0, q->x2, q->y2, q->r0, q->g0, q->b0, stp,
                        s_twMaskX, s_twMaskY, s_twOffX, s_twOffY);
                }
            }

            /* Build the per-pass render contexts from the current walk state: native (target 0) always,
             * hi-res (target 1, geometry x hiScale) when supersampling is on. Snapshotting into a ctx is
             * what lets the hi-res pass be split across per-band worker threads afterwards. */
            RenderCtx rcn, rch;
            int hires = HiresActive();
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
        /* Fail-soft cycle guard: a cyclic OT link would spin this walk forever (Windows then freezes
         * the process as unresponsive). A legitimate frame is a few thousand prims plus the bucket
         * table, so one million steps only happens on a cycle: dump a tail, drop the frame, keep running. */
        if (++walkSteps > 1000000UL) {
            static int cycleWarned = 0;
            if (!cycleWarned) {
                int d;
                cycleWarned = 1;
                fprintf(stderr, "[libgpu] OT walk exceeded 1M steps -- CYCLIC ordering table; "
                                "dropping the rest of the frame. Chain tail:\n");
                for (d = 0; d < 12 && nextTok; d++) {
                    int b = 0;
                    void *c2 = PC_OtResolve(nextTok, &b);
                    if (!c2) { fprintf(stderr, "  tok=%u UNRESOLVED\n", nextTok); break; }
                    fprintf(stderr, "  tok=%u %s addr=%p type=0x%02x code=0x%02x\n", nextTok,
                            b ? "bucket" : "prim", c2,
                            b ? 0 : PC_GPU_PRIM_TYPE((P_TAG *)c2), ((P_TAG *)c2)->code);
                    nextTok = ((P_TAG *)c2)->tag;
                }
            }
            break;
        }
    }

    TrcFrameEnd();   /* trace record: 'Z' frame delimiter (+ closes the file at the frame cap) */

    /* Rasterize the deferred hi-res display list (the native pass drew inline above), fanned out
     * across per-band worker threads. */
    if (HiresActive())
        HiresRasterizeThreaded(s_drawEnv.clip.y, s_drawEnv.clip.h);

    /* Present now, after this frame's rasterization, using the DISPENV the most recent PutDispEnv
     * recorded: every call site pairs PutDispEnv with an immediate DrawOTag. See
     * docs/pc-port/subsystems/gpu.md, "Presentation". */
    if (s_rtTime) { s_rtAccum += clock() - s_rtStart;
        if (++s_rtFrames >= 120) { fprintf(stderr, "[raster] scale=%d  mean %.0f us/frame CPU (%u frames)\n",
            hiScale, (double)s_rtAccum / CLOCKS_PER_SEC * 1e6 / s_rtFrames, s_rtFrames); s_rtAccum = 0; s_rtFrames = 0; } }
    PC_UpdateCamOsd(); /* refresh the debug pose label to match this frame (VH_CAM_OSD) */
    PC_MaybeDumpVram(); /* VH_VRAM_DUMP=N: snapshot VRAM to PPM for texture/CLUT triage */
    if (HiresActive()) {                   /* present the supersampled display region */
        HiresPresent(s_dispEnv.disp.x, s_dispEnv.disp.y, s_dispEnv.disp.w, s_dispEnv.disp.h);
    } else {
        unsigned short (*vram)[VRAM_W] = PC_GpuVram();
        PC_GpuPresent(&vram[0][0], VRAM_W, VRAM_H,
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

/* ---- TIM parsing: ReadTIM only parses a TIM header; paddr/caddr point into the caller's file
 * buffer, and the caller uploads it. See docs/pc-port/subsystems/gpu.md, "Primitives and
 * rasterisation". */

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

/* ---- deferred (debug bitmap-font printer, not the game's own core/text.c) ---- */

int FntPrint(const char *fmt, ...) { (void)fmt; return 0; }
unsigned int *FntFlush(int id) { (void)id; return NULL; }
