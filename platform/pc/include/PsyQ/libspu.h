/* PC-backend replacement for the PSX SDK's libspu.h low-level SPU interface (clean-room: names and
 * constants only). Most entry points are deliberate no-ops here, since SPU RAM management does not
 * apply when OpenAL owns its buffers -- see libspu.c for what each one does. */
#ifndef PLATFORM_PC_PSYQ_LIBSPU_H
#define PLATFORM_PC_PSYQ_LIBSPU_H

#include "sys/types.h"

#define SPU_OFF 0
#define SPU_ON  1
#define SPU_REV_MODE_STUDIO_C 4
/* Only the all-channels mask is used by this project's call sites; the per-channel
 * bits are bit N = voice N (psx-spx). */
#define SPU_ALLCH 0x00ffffff

typedef void (*SpuIRQCallbackProc)(void);

int SpuSetTransferMode(int mode);
int SpuSetIRQ(int on_off);
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback);
void SpuSetKey(int on_off, unsigned int voiceBit);
int SpuMallocWithStartAddr(unsigned int addr, int size);
int SpuClearReverbWorkArea(int mode);

#endif
