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

/* STR (streaming movie) sector header -- referenced by core/cd.c's deferred
 * Movie_* / MDEC playback path (see the DecDCT* deferral note above); the
 * struct itself is still needed just for core/cd.c to compile and pack/unpack
 * frame headers, even though frame decode isn't implemented. */
typedef struct {
    u_short id;
    u_short type;
    u_short secCount;
    u_short nSectors;
    unsigned int  frameCount;
    unsigned int  frameSize;
    u_short width;
    u_short height;
    unsigned int  dummy1;
    unsigned int  dummy2;
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
int CdRead(int sectors, unsigned int *buf, int mode);
int CdRead2(int mode);
int CdReadSync(int mode, u_char *result);
unsigned int CdReadyCallback(void (*func)());
CdlLOC *CdIntToPos(int i, CdlLOC *p);
int CdMix(CdlATV *vol);

void DecDCTReset(int mode);
int DecDCTvlc(unsigned int *bs, unsigned int *buf);
void DecDCTin(unsigned int *buf, int mode);
void DecDCTout(unsigned int *buf, int size);

/* STR (streaming movie) ring-buffer API -- same deferral as DecDCT* above
 * (FMV/movie playback, not regular data loading). DecDCToutCallback isn't
 * declared in any real header either (same "undeclared, relies on old
 * GCC's implicit-declaration leniency" pattern already found for Kernel's
 * GetRCnt/OpenEvent) -- signature inferred from its call sites
 * (core/cd.c passes NULL or a function pointer, matching the
 * CdReadyCallback/DrawSyncCallback callback-setter convention). */
unsigned int DecDCToutCallback(void (*func)());
void StSetRing(unsigned int *ring_addr, unsigned int ring_size);
void StUnSetRing(void);
unsigned int StFreeRing(unsigned int *base);
void StSetStream(unsigned int mode, unsigned int start_frame, unsigned int end_frame, int (*func1)(), int (*func2)());
unsigned int StGetNext(unsigned int **addr, unsigned int **header);
void StCdInterrupt(void);

#endif
