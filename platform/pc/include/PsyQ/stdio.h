/* PC-backend replacement for the PSX SDK's minimal stdio.h (the matching build is -nostdinc, so Sony
 * ships a tiny one for BUFSIZ, EOF, the SEEK_ constants and size_t). The PC build has a full host
 * stdio, so this forwards to it. */
#ifndef PLATFORM_PC_PSYQ_STDIO_H
#define PLATFORM_PC_PSYQ_STDIO_H

#include <stdio.h>

#endif
