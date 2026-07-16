/*
 * PC backend for PsyQ/libsn.h's PCcreat/PClseek/PCwrite/PCclose -- real
 * local file I/O, standing in for the original "write to the connected
 * dev PC over the debug link" workflow. See libsn.h for why this is a
 * real implementation rather than a stub.
 */
#include <fcntl.h>
#include <unistd.h>

#include "PsyQ/libsn.h"

int PCcreat(char *name, int perms) {
    (void)perms;
    return open(name, O_WRONLY | O_CREAT | O_TRUNC, 0644);
}

int PClseek(int fd, int offset, int mode) {
    int whence = SEEK_SET;
    if (mode == 1) whence = SEEK_CUR;
    else if (mode == 2) whence = SEEK_END;
    return (int)lseek(fd, offset, whence);
}

int PCwrite(int fd, char *buff, int len) {
    return (int)write(fd, buff, (size_t)len);
}

int PCclose(int fd) {
    return close(fd);
}
