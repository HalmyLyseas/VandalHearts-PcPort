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

/* ---- provided by pc_gpu_trace.c (record/replay regression harness) ---- */
void TrcInit(void);                /* arm recording if VH_GPU_RECORD asks for it (no-op otherwise) */
void TrcWrite(char op, const void *a, u32 na, const void *b, u32 nb);   /* no-op unless recording */
void TrcPrim(int type, const void *prim);   /* record one walked primitive (checks state + size) */
void TrcFrameEnd(void);            /* 'Z' frame delimiter + closes the file at the frame cap */
int  TrcReplaying(void);           /* 1 while PC_GpuReplayTrace is feeding ops back through the API */

#endif /* PC_GPU_INTERNAL_H */
