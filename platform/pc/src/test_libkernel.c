/* Standalone proof-of-concept, not part of the game build: exercises the event system
 * (OpenEvent/EnableEvent/StartCard/TestEvent, matching core/card.c's flow), the timer, and a save-
 * file round trip through FileOpen/FileWrite/FileRead/FileClose plus firstfile/nextfile scanning. */
#include <stdio.h>
#include <string.h>
#include <unistd.h>
#include "PsyQ/kernel.h"
#include "PsyQ/sys/file.h"

/* Matching core/card.c's own approach: these aren't declared in kernel.h
 * (neither the real one nor ours) -- forward-declare locally. */
extern void InitCard(s32);
extern s32 StartCard(void);
extern s32 FileOpen(unsigned char *, s32);
extern s32 FileClose(s32);
extern s32 FileRead(s32, void *, s32);
extern s32 FileWrite(s32, void *, s32);
extern struct DIRENTRY *firstfile(unsigned char *, struct DIRENTRY *);
extern struct DIRENTRY *nextfile(struct DIRENTRY *);

int main(void) {
    /* ---- event system, mirroring core/card.c's StartCard()/init sequence ---- */
    s32 evNew = OpenEvent(SwCARD, EvSpNEW, EvMdNOINTR, NULL);
    s32 evError = OpenEvent(SwCARD, EvSpERROR, EvMdNOINTR, NULL);
    EnableEvent(evNew);
    EnableEvent(evError);

    if (TestEvent(evError) != 0) {
        fprintf(stderr, "FAIL: error event signaled before any error occurred\n");
        return 1;
    }
    StartCard(); /* should signal "card present" (EvSpNEW) */
    if (TestEvent(evNew) != 1) {
        fprintf(stderr, "FAIL: StartCard() did not signal the NEW-card event\n");
        return 1;
    }
    if (TestEvent(evNew) != 0) {
        fprintf(stderr, "FAIL: TestEvent did not auto-clear after a successful test\n");
        return 1;
    }
    printf("PASS: event system (OpenEvent/EnableEvent/StartCard/TestEvent) behaves correctly\n");

    /* ---- timer ---- */
    ResetRCnt(RCntCNT1);
    usleep(20000); /* ~20ms */
    u32 ticks = GetRCnt(RCntCNT1);
    printf("GetRCnt after ~20ms sleep: %u ticks (%.2fms at the assumed %gHz rate)\n",
           ticks, ticks / 15.734, 15734.0);
    if (ticks == 0) {
        fprintf(stderr, "FAIL: GetRCnt reported zero elapsed time\n");
        return 1;
    }

    /* ---- real save-file round trip ---- */
    InitCard(0);
    unsigned char writeBuf[64];
    memset(writeBuf, 0xAB, sizeof(writeBuf));
    strcpy((char *)writeBuf, "VandalHearts PC backend save test");

    s32 fd = FileOpen((unsigned char *)"bu00:BASLUS-00447VH", O_CREAT);
    if (fd < 0) {
        fprintf(stderr, "FAIL: FileOpen(O_CREAT) failed\n");
        return 1;
    }
    s32 written = FileWrite(fd, writeBuf, sizeof(writeBuf));
    FileClose(fd);
    if (written != sizeof(writeBuf)) {
        fprintf(stderr, "FAIL: FileWrite wrote %d bytes, expected %zu\n", written, sizeof(writeBuf));
        return 1;
    }

    fd = FileOpen((unsigned char *)"bu00:BASLUS-00447VH", O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "FAIL: FileOpen(O_RDONLY) failed on the file just written\n");
        return 1;
    }
    unsigned char readBuf[64];
    s32 readN = FileRead(fd, readBuf, sizeof(readBuf));
    FileClose(fd);
    if (readN != sizeof(readBuf) || memcmp(readBuf, writeBuf, sizeof(readBuf)) != 0) {
        fprintf(stderr, "FAIL: read-back data does not match what was written\n");
        return 1;
    }
    printf("PASS: real save-file round trip (FileOpen/Write/Close -> Open/Read/Close) byte-identical\n");

    /* ---- directory enumeration ---- */
    struct DIRENTRY entry;
    struct DIRENTRY *d = firstfile((unsigned char *)"bu00:*", &entry);
    int found = 0;
    while (d) {
        if (strcmp(d->name, "BASLUS-00447VH") == 0) {
            found = 1;
            printf("firstfile/nextfile found our save: name=%s size=%ld\n", d->name, d->size);
        }
        d = nextfile(&entry);
    }
    if (!found) {
        fprintf(stderr, "FAIL: directory scan did not find the save file we just wrote\n");
        return 1;
    }
    printf("PASS: firstfile/nextfile directory enumeration works\n");

    return 0;
}
