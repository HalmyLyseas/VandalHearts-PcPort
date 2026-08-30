/* PC-backend replacement for the PSX SDK's sys/file.h open-flag constants. Clean-room: core/card.c
 * only uses these symbolically (never serialized or compared against a constant), so this defines
 * its own consistent bit assignment rather than reproducing Sony's. */
#ifndef PLATFORM_PC_PSYQ_SYS_FILE_H
#define PLATFORM_PC_PSYQ_SYS_FILE_H

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   0x03
#define O_CREAT  0x04

#endif
