/*
 * PC-backend replacement for the PSX SDK's libsnd.h sequencer/VAB interface.
 *
 * Clean-room reimplementation: only signatures and mode constants (public,
 * standard facts, not Sony's header text) -- no text from Sony's original
 * header. Scope covers only what the game's source actually calls (per
 * exchange/02-phase-c-interface-contract.md).
 *
 * SsSeqOpen/Play/Stop/Close/SetVol (full sequence/song playback -- a real
 * MIDI-like interpreter for a proprietary format) are declared but not yet
 * implemented; see libsnd.c and the checkpoint doc's Audio step file for
 * why this is deferred as a separate, substantial piece of work.
 */
#ifndef PLATFORM_PC_PSYQ_LIBSND_H
#define PLATFORM_PC_PSYQ_LIBSND_H

#include "sys/types.h"

#define SS_SEQ_TABSIZ 172

#define SS_SOFF  0
#define SS_SON   1
#define SS_MIX   0
#define SS_REV   1
#define SS_SERIAL_A 0
#define SS_TICK60   1
#define SSPLAY_PLAY      1
#define SSPLAY_INFINITY  0

void SsInit(void);
void SsQuit(void);
void SsStart(void);
void SsEnd(void);
void SsSetTickMode(long mode);
void SsSetTableSize(char *table, short maxVab, short maxSeq);
char SsSetReservedVoice(char n);
void SsSetMono(void);
void SsSetStereo(void);
void SsSetMVol(short lVol, short rVol);
void SsSetSerialAttr(char port, char attr, char mode);
void SsSetSerialVol(char port, short lVol, short rVol);

short SsVabOpenHeadSticky(unsigned char *vabHead, short vabId, unsigned long dummy);
short SsVabTransBodyPartly(unsigned char *vabBody, unsigned long size, short vabId);
short SsVabTransCompleted(short flag);
void SsVabClose(short vabId);

void SsVoKeyOn(long vabId, long prog, unsigned short pitch, unsigned short vol);
void SsVoKeyOff(long vabId, long prog);
short SsUtKeyOnV(short voice, short vabId, short prog, short tone, short note, short fine, short voll, short volr);
short SsUtKeyOffV(short voice);

void SsUtReverbOn(void);
short SsUtSetReverbType(short type);
void SsUtSetReverbDepth(short depth1, short depth2);

short SsSeqOpen(unsigned long *seqData, short mode);
void SsSeqPlay(short seqAccessNum, char playMode, short repeatCount);
void SsSeqStop(short seqAccessNum);
void SsSeqSetVol(short seqAccessNum, short lVol, short rVol);
void SsSeqClose(short seqAccessNum);

#endif
