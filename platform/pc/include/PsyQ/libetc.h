/* PC-backend replacement for the PSX SDK's libetc.h pad/vsync interface: a clean-room
 * reimplementation. Only the function signatures and controller bit values are reproduced
 * (functional facts dictated by the PS1 controller protocol, not copyrightable expression). */
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
unsigned int PadRead(int id);
int VSync(int mode);
int ResetCallback(void);

#endif
