/* PC-backend replacement for the PSX SDK's minimal memory.h (the matching build is -nostdinc).
 * The host libc provides memcpy/memmove/memcmp/memset, so this forwards to it. */
#ifndef PLATFORM_PC_PSYQ_MEMORY_H
#define PLATFORM_PC_PSYQ_MEMORY_H

#include <string.h>

#endif
