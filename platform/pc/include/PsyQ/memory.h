/*
 * PC-backend replacement for the PSX SDK's own minimal memory.h (needed
 * because the real matching-decomp build is -nostdinc). Declares
 * memcpy/memmove/memcmp/memset -- a real host libc already provides these
 * correctly, so this just forwards to it.
 */
#ifndef PLATFORM_PC_PSYQ_MEMORY_H
#define PLATFORM_PC_PSYQ_MEMORY_H

#include <string.h>

#endif
