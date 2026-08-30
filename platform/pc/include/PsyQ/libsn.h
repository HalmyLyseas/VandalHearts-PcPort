/* PC-backend replacement for the PSX SDK's libsn.h (debug-stub/host comms). pollhost()/PSYQpause()
 * are never called; PCcreat/PClseek/PCwrite/PCclose are, by states/game_setup.c, and are real local
 * file I/O below -- a faithful port of "write to the connected dev PC" rather than a stub. */
#ifndef PLATFORM_PC_PSYQ_LIBSN_H
#define PLATFORM_PC_PSYQ_LIBSN_H

int PCcreat(char *name, int perms);
int PClseek(int fd, int offset, int mode);
int PCwrite(int fd, char *buff, int len);
int PCclose(int fd);

#endif
