/* PC replacement for include/inline_gte.h (raw MIPS COP2 asm, used only by src/core/graphics.c):
 * -Iplatform/pc/include precedes -Iinclude, so graphics.c resolves here unchanged. Every macro
 * calls the libgte.c GTE core that also backs PsyQ/libgte.h. See docs/pc-port/subsystems/gte.md. */
#ifndef PLATFORM_PC_INLINE_GTE_H
#define PLATFORM_PC_INLINE_GTE_H

#include "types.h"

void PC_GTE_LoadV0(void *v);
void PC_GTE_LoadV3(void *v0, void *v1, void *v2);
void PC_GTE_LoadRGB(void *rgbc);
void PC_GTE_LoadOPV1(void *v);
void PC_GTE_LoadOPV2(void *v);
void PC_GTE_RTPS(void);
void PC_GTE_RTPT(void);
void PC_GTE_NCLIP(void);
void PC_GTE_AVSZ4(void);
void PC_GTE_OP0(void);
void PC_GTE_NCCS(void);
void PC_GTE_StoreSXY(void *out);
void PC_GTE_StoreSXY3(void *out0, void *out1, void *out2);
void PC_GTE_StoreOTZ(void *out);
void PC_GTE_StoreOPZ(void *out);
void PC_GTE_StoreLVNL(void *out);
void PC_GTE_StoreRGB(void *out);

#define gte_ldv0(r0)          PC_GTE_LoadV0(r0)
#define gte_ldv3(r0, r1, r2)  PC_GTE_LoadV3(r0, r1, r2)
#define gte_ldrgb(r0)         PC_GTE_LoadRGB(r0)
#define gte_ldopv1(r0)        PC_GTE_LoadOPV1(r0)
#define gte_ldopv2(r0)        PC_GTE_LoadOPV2(r0)
#define gte_rtps()            PC_GTE_RTPS()
#define gte_rtpt()            PC_GTE_RTPT()
#define gte_nclip()           PC_GTE_NCLIP()
#define gte_avsz4()           PC_GTE_AVSZ4()
#define gte_op0()             PC_GTE_OP0()
#define gte_nccs()            PC_GTE_NCCS()
#define gte_stsxy(r0)         PC_GTE_StoreSXY(r0)
#define gte_stsxy3(r0, r1, r2) PC_GTE_StoreSXY3(r0, r1, r2)
#define gte_stotz(r0)         PC_GTE_StoreOTZ(r0)
#define gte_stopz(r0)         PC_GTE_StoreOPZ(r0)
#define gte_stlvnl(r0)        PC_GTE_StoreLVNL(r0)
#define gte_strgb(r0)         PC_GTE_StoreRGB(r0)

#endif
