/*
 * PC-backend replacement for the PSX SDK's libpress.h (MDEC/LZS
 * decompression). The real header re-declares the same DecDCT* symbols
 * already declared in libcd.h (both get included together by cd.c) plus a
 * few extra ones -- forward to libcd.h for the shared ones rather than
 * duplicating, and add the rest here. All deferred (FMV/MDEC playback is
 * out of scope, same as libcd.h's DecDCT* -- see exchange/05-phase-c-cd-backend.md).
 */
#ifndef PLATFORM_PC_PSYQ_LIBPRESS_H
#define PLATFORM_PC_PSYQ_LIBPRESS_H

#include "libcd.h"

int DecDCTBufSize(unsigned int *bs);
int DecDCTinSync(int mode);
int DecDCToutSync(int mode);

#endif
