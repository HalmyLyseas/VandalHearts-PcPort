/* pc_gpu_internal.h -- seams between libgpu.c (PsyQ GPU API + VRAM + OT walker) and the other GPU TUs
 * (pc_raster.c, pc_hdpack.c, pc_gpu_trace.c). NOT a public API: nothing outside platform/pc/src
 * includes this. */
#ifndef PC_GPU_INTERNAL_H
#define PC_GPU_INTERNAL_H

#include <stddef.h>
#include <signal.h>        /* sig_atomic_t (the SIGUSR2 hires-dump latch) */
#include "PsyQ/libgpu.h"   /* u32, P_TAG, PC_GPU_PRIM_* */

/* ---- provided by libgpu.c ---- */
/* OT token registry (see the "token bridge" comment in libgpu.c). The replay path mints its own
 * chains through the same registry the game uses. */
u32  PC_OtMint(void *p, int isBucket);
void PC_OtResetTokens(void);
/* Raw framebuffer bytes for hashing (regression signatures). Vram is always available; Hires
 * returns NULL when supersampling is off (scale 1) or the buffer was never allocated. */
const void *PC_GpuVramBytes(size_t *n);
const void *PC_GpuHiresBytes(size_t *n);
/* The resolved internal scale (reads VH_INTERNAL_SCALE on first use, like the rasterizer). */
int PC_GpuGetInternalScale(void);
/* The per-DrawOTag frame counter (libgpu.c) -- diagnostics stamp filenames/logs with it. */
extern unsigned s_drawFrame;

/* ---- provided by pc_raster.c (framebuffers + software rasterizer) ---- */
/* The live VRAM as its natural 2D shape (rows of 1024 halfwords) -- libgpu.c's bulk transfers
 * write through this; the HD dump path decodes texels + CLUTs straight out of it. */
#define PC_GPU_VRAM_W 1024
#define PC_GPU_VRAM_H 512
unsigned short (*PC_GpuVram(void))[PC_GPU_VRAM_W];
/* Texpage/pixel helpers shared across the GPU TUs. */
void TPageOrigin(int tpage, int *x, int *y, int *tp);
void UnpackColor(unsigned short c, int *r, int *g, int *b);
/* Per-pass render context: the mutable rasterizer state the pixel/texture path reads -- clip band, draw
 * offset, dither-enable, texture window, and the target (native VRAM vs the Sx hi-res buffer). DrawOTag
 * snapshots its DR_MODE globals into one per draw; the hi-res pass hands per-band copies to workers. */
typedef struct {
    int clipX, clipY, clipW, clipH;   /* drawing area, native units (y/h become the band under threading) */
    int ofsX, ofsY;                   /* draw offset, native units */
    int dither;                       /* dither-enable (GP0 E1h.9) */
    int twMaskX, twMaskY, twOffX, twOffY;  /* texture window (GP0 E2h) */
    int target;                       /* 0 = native VRAM, 1 = hi-res */
    int scale;                        /* geometry x scale when target==1 (else effectively 1) */
} RenderCtx;
typedef struct { double x, y, u, v; } RVert;
void FillQuad(const RenderCtx *rc, RVert v0, RVert v1, RVert v2, RVert v3, int r, int g, int b,
              int textured, int tpage, int clut, int semiTrans, int abr);
void FillRect(const RenderCtx *rc, int x0, int y0, int w, int h, int r, int g, int b);
void FillRectRaw(int x0, int y0, int w, int h, int r, int g, int b);   /* ClearImage: no offset/clip */
/* Hi-res pass (VH_INTERNAL_SCALE): buffer lifecycle, per-frame display list, banded thread pool. */
void HiresEnsure(void);
void HiresMirrorRect(int x0, int y0, int w, int h);   /* keep hi-res in sync with bulk VRAM writes */
void HiresFrameReset(void);
void HiresAppendQuad(const RenderCtx *rch, RVert a, RVert b, RVert c, RVert d,
                     int r, int g, int bcol, int textured, int tpage, int clut, int semi, int abr);
void HiresAppendRect(const RenderCtx *rch, int x, int y, int w, int h, int r, int g, int b);
void HiresRasterizeThreaded(int clipY, int clipH);
int  HiresActive(void);                                        /* scale > 1 and the buffer exists */
void HiresPresent(int dispX, int dispY, int dispW, int dispH); /* edge-clamp + dump hook + present */
extern volatile sig_atomic_t g_vhHiresDumpReq;   /* SIGUSR2 latch: dump the hires pair this frame */

/* ---- provided by pc_hdpack.c (HD pack: background replacement) ---- */
/* One replaced/dumped VRAM region. px is published by the async loader with a release store;
 * readers acquire-load it (NULL until the decode lands -> native texels draw). */
typedef struct { unsigned long long hash; int rx, ry, rw, rh; unsigned short *px; int w, h; int dumped; int live; const char *dir; } HdRegion;
void HdPack_OnLoad(const RECT *rect, const unsigned short *src);  /* LoadImage hook: hash/register/evict */
void HdMaybeDump(int tpage, int clut, int uMin, int uMax, int vMin, int vMax);   /* VH_HD_DUMP */
HdRegion *HdFindTriRegion(int tpage, int uMin, int uMax, int vMin, int vMax);    /* per-triangle resolve */
int  HdActive(void);           /* HD pack (or VH_HD_PACK override) live right now? */
int  HdAnyActive(void);        /* HD pack OR a langpack backgrounds/ source live */
const char *HdDumpDir(void);   /* VH_HD_DUMP dir, or NULL */
int  HdRegionCount(void);      /* registered regions (gates the per-triangle work) */
int  HdReplaceCount(void);     /* regions with a replacement (gates threading + resolve) */

/* ---- provided by pc_gpu_trace.c (record/replay regression harness) ---- */
void TrcInit(void);                /* arm recording if VH_GPU_RECORD asks for it (no-op otherwise) */
void TrcWrite(char op, const void *a, u32 na, const void *b, u32 nb);   /* no-op unless recording */
void TrcPrim(int type, const void *prim);   /* record one walked primitive (checks state + size) */
void TrcFrameEnd(void);            /* 'Z' frame delimiter + closes the file at the frame cap */
int  TrcReplaying(void);           /* 1 while PC_GpuReplayTrace is feeding ops back through the API */

#endif /* PC_GPU_INTERNAL_H */
