/* pc_gpu_internal.h -- seams between libgpu.c (PsyQ GPU API + VRAM + OT walker) and the subsystem
 * TUs extracted from it (pc_gpu_trace.c, ...). NOT a public API: nothing outside platform/pc/src
 * includes this. Everything here used to be file-static in libgpu.c; the split moved whole
 * subsystems out verbatim, and these are the few symbols that must cross the new TU boundaries. */
#ifndef PC_GPU_INTERNAL_H
#define PC_GPU_INTERNAL_H

#include <stddef.h>
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
/* The live VRAM as its natural 2D shape (rows of 1024 halfwords) -- the HD dump path decodes
 * texels + CLUTs straight out of it. */
#define PC_GPU_VRAM_W 1024
#define PC_GPU_VRAM_H 512
unsigned short (*PC_GpuVram(void))[PC_GPU_VRAM_W];
/* Texpage/pixel helpers shared across the split TUs (were file-static in libgpu.c). */
void TPageOrigin(int tpage, int *x, int *y, int *tp);
void UnpackColor(unsigned short c, int *r, int *g, int *b);

/* ---- provided by pc_hdpack.c (1.6 HD pack: background replacement) ---- */
/* One replaced/dumped VRAM region. px is published by the async loader with a release store;
 * readers acquire-load it (NULL until the decode lands -> native texels draw). */
typedef struct { unsigned long long hash; int rx, ry, rw, rh; unsigned short *px; int w, h; int dumped; int live; } HdRegion;
void HdPack_OnLoad(const RECT *rect, const unsigned short *src);  /* LoadImage hook: hash/register/evict */
void HdMaybeDump(int tpage, int clut, int uMin, int uMax, int vMin, int vMax);   /* VH_HD_DUMP */
HdRegion *HdFindTriRegion(int tpage, int uMin, int uMax, int vMin, int vMax);    /* per-triangle resolve */
int  HdActive(void);           /* pack (or VH_HD_PACK override) live right now? */
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
