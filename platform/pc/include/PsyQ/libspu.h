/*
 * PC-backend replacement for the PSX SDK's libspu.h low-level SPU register
 * interface. Clean-room reimplementation (see libsnd.h for the general
 * rationale). On this backend most of these are deliberate no-ops: SPU RAM
 * management doesn't apply when OpenAL owns its own buffers -- see
 * libspu.c for what each one actually does here and why.
 */
#ifndef PLATFORM_PC_PSYQ_LIBSPU_H
#define PLATFORM_PC_PSYQ_LIBSPU_H

#include "sys/types.h"

#define SPU_OFF 0
#define SPU_ON  1
#define SPU_REV_MODE_STUDIO_C 4
/* Individual per-channel bits aren't used directly by this project's call
 * sites (only the all-channels mask is) -- see exchange/02 for the full
 * per-channel bit list if a future file needs it. */
#define SPU_ALLCH 0x00ffffff

typedef void (*SpuIRQCallbackProc)(void);

int SpuSetTransferMode(int mode);
int SpuSetIRQ(int on_off);
SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback);
void SpuSetKey(int on_off, unsigned int voiceBit);
int SpuMallocWithStartAddr(unsigned int addr, int size);
int SpuClearReverbWorkArea(int mode);

#endif
