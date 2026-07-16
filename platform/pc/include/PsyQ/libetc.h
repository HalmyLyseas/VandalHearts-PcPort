/*
 * PC-backend replacement for the PSX SDK's libetc.h pad/vsync interface.
 *
 * Clean-room reimplementation: only the function signatures and controller
 * bit values are reproduced here (these are functional facts dictated by
 * the PS1 controller protocol and this project's own call sites, not
 * copyrightable expression) -- no text from Sony's original header. Scope
 * is intentionally limited to what the game's source and include/common.h
 * actually use (per exchange/02-phase-c-interface-contract.md); extend if
 * a future file needs more of the real libetc.h surface.
 */
#ifndef PLATFORM_PC_PSYQ_LIBETC_H
#define PLATFORM_PC_PSYQ_LIBETC_H

#include "sys/types.h"

/* Standard PS1 digital-pad button bitmasks (one 16-bit field per
 * controller port, packed low/high into PadRead()'s 32-bit result). */
#define PADLup     (1<<12)
#define PADLdown   (1<<14)
#define PADLleft   (1<<15)
#define PADLright  (1<<13)
#define PADRup     (1<< 4)
#define PADRdown   (1<< 6)
#define PADRleft   (1<< 7)
#define PADRright  (1<< 5)
#define PADL1      (1<< 2)
#define PADL2      (1<< 0)
#define PADR1      (1<< 3)
#define PADR2      (1<< 1)
#define PADstart   (1<<11)
#define PADselect  (1<< 8)

void PadInit(int mode);
u_long PadRead(int id);
int VSync(int mode);
int ResetCallback(void);

#endif
