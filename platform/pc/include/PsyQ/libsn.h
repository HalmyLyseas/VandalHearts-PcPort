/*
 * PC-backend replacement for the PSX SDK's libsn.h (debug-stub/host comms).
 * pollhost()/PSYQpause() (raw MIPS breakpoint-trap macros on real hardware)
 * are confirmed never called anywhere -- but PCcreat/PClseek/PCwrite/
 * PCclose (real dev-workflow functions: write debug data to the PC the
 * devkit's debug link was connected to) ARE called, by game_setup.c.
 * Implemented as real local file I/O below -- writing to a real local file
 * is a faithful port of "write to the connected dev PC" instead of a
 * hollow stub, and costs nothing extra to implement for real.
 */
#ifndef PLATFORM_PC_PSYQ_LIBSN_H
#define PLATFORM_PC_PSYQ_LIBSN_H

int PCcreat(char *name, int perms);
int PClseek(int fd, int offset, int mode);
int PCwrite(int fd, char *buff, int len);
int PCclose(int fd);

#endif
