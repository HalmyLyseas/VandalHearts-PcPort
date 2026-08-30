/* PC-backend replacement for the PSX SDK's minimal strings.h (the matching build is -nostdinc).
 * The host libc provides strcpy/strcat/strlen/etc, so this forwards to it. */
#ifndef PLATFORM_PC_PSYQ_STRINGS_H
#define PLATFORM_PC_PSYQ_STRINGS_H

#include <string.h>

#endif
