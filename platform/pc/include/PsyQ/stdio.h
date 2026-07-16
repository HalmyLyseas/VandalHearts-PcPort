/*
 * PC-backend replacement for the PSX SDK's own minimal stdio.h (needed
 * because the real matching-decomp build is -nostdinc, so Sony shipped a
 * tiny stdio.h of its own just for BUFSIZ, EOF, the SEEK_ constants, and
 * size_t). The PC build isn't -nostdinc -- a real, full stdio
 * implementation is already available, so this just forwards to it.
 */
#ifndef PLATFORM_PC_PSYQ_STDIO_H
#define PLATFORM_PC_PSYQ_STDIO_H

#include <stdio.h>

#endif
