/*
 * PC-backend replacement for the PSX SDK's own minimal strings.h (needed
 * because the real matching-decomp build is -nostdinc). Declares
 * strcpy/strcat/strlen/etc -- a real host libc already provides these
 * correctly, so this just forwards to it.
 */
#ifndef PLATFORM_PC_PSYQ_STRINGS_H
#define PLATFORM_PC_PSYQ_STRINGS_H

#include <string.h>

#endif
