/*
 * PC-backend replacement for the PSX SDK's libcd.h CD-ROM interface.
 *
 * Clean-room reimplementation: signatures, struct layouts, and command/mode
 * constants are functional facts (the CD-ROM command protocol and BCD MSF
 * addressing are public, standard conventions, not creative expression) --
 * no text from Sony's original header. Scope covers only what the game's
 * source actually calls (per exchange/02-phase-c-interface-contract.md).
 *
 * The 4 DecDCT* (MDEC video decompression) functions are declared but not
 * yet implemented -- FMV playback is deferred, see libcd.c.
 */
#ifndef PLATFORM_PC_PSYQ_LIBCD_H
#define PLATFORM_PC_PSYQ_LIBCD_H

#include "sys/types.h"

typedef struct {
    unsigned char minute; /* BCD */
    unsigned char second; /* BCD */
    unsigned char sector; /* BCD */
    unsigned char track;
} CdlLOC;

typedef struct {
    unsigned char val0, val1, val2, val3;
} CdlATV;

typedef struct {
    unsigned char file; /* file ID (always 1) */
    unsigned char chan; /* channel ID */
    unsigned short pad;
} CdlFILTER;

/* STR (streaming movie) sector header -- referenced by cd.c's deferred
 * Movie_* / MDEC playback path (see the DecDCT* deferral note above); the
 * struct itself is still needed just for cd.c to compile and pack/unpack
 * frame headers, even though frame decode isn't implemented. */
typedef struct {
    u_short id;
    u_short type;
    u_short secCount;
    u_short nSectors;
    u_long  frameCount;
    u_long  frameSize;
    u_short width;
    u_short height;
    u_long  dummy1;
    u_long  dummy2;
    CdlLOC  loc;
} StHEADER;

#define CdlNop        0x01
#define CdlSetloc     0x02
#define CdlReadN      0x06
#define CdlSetfilter  0x0d
#define CdlSetmode    0x0e
#define CdlPause      0x09
#define CdlReset      0x0a
#define CdlSeekL      0x15

#define CdlComplete       0x02
#define CdlStatShellOpen  0x10
#define CdlStatSeek       0x40

#define CdlModeSF     0x08
#define CdlModeRT     0x40
#define CdlModeSpeed  0x80

int CdInit(void);
int CdControl(u_char com, u_char *param, u_char *result);
int CdControlB(u_char com, u_char *param, u_char *result);
int CdSync(int mode, u_char *result);
int CdRead(int sectors, u_long *buf, int mode);
int CdRead2(long mode);
int CdReadSync(int mode, u_char *result);
u_long CdReadyCallback(void (*func)());
CdlLOC *CdIntToPos(int i, CdlLOC *p);
int CdMix(CdlATV *vol);

void DecDCTReset(int mode);
int DecDCTvlc(u_long *bs, u_long *buf);
void DecDCTin(u_long *buf, int mode);
void DecDCTout(u_long *buf, int size);

/* STR (streaming movie) ring-buffer API -- same deferral as DecDCT* above
 * (FMV/movie playback, not regular data loading). DecDCToutCallback isn't
 * declared in any real header either (same "undeclared, relies on old
 * GCC's implicit-declaration leniency" pattern already found for Kernel's
 * GetRCnt/OpenEvent) -- signature inferred from its call sites
 * (cd.c passes NULL or a function pointer, matching the
 * CdReadyCallback/DrawSyncCallback callback-setter convention). */
u_long DecDCToutCallback(void (*func)());
void StSetRing(u_long *ring_addr, u_long ring_size);
void StUnSetRing(void);
u_long StFreeRing(u_long *base);
void StSetStream(u_long mode, u_long start_frame, u_long end_frame, int (*func1)(), int (*func2)());
u_long StGetNext(u_long **addr, u_long **header);
void StCdInterrupt(void);

#endif
