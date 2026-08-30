/* Clean-room PC replacement for PsyQ libpress.h (MDEC decompression). The real header
 * re-declares the DecDCT* symbols from libcd.h (core/cd.c includes both); those are forwarded
 * to libcd.h and only the extra entry points are declared here. */
#ifndef PLATFORM_PC_PSYQ_LIBPRESS_H
#define PLATFORM_PC_PSYQ_LIBPRESS_H

#include "libcd.h"

int DecDCTBufSize(unsigned int *bs);
int DecDCTinSync(int mode);
int DecDCToutSync(int mode);

#endif
