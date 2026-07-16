/*
 * PC-backend replacement for the PSX SDK's sys/file.h open-flag constants.
 * Clean-room: the real header's underlying FREAD/FWRITE/FCREAT bit values
 * aren't reproduced anywhere in this project's tree (not needed -- these
 * flags are never serialized or compared against a hardcoded value, only
 * used symbolically by card.c), so this defines its own consistent bit
 * assignment rather than guessing Sony's.
 */
#ifndef PLATFORM_PC_PSYQ_SYS_FILE_H
#define PLATFORM_PC_PSYQ_SYS_FILE_H

#define O_RDONLY 0x01
#define O_WRONLY 0x02
#define O_RDWR   0x03
#define O_CREAT  0x04

#endif
