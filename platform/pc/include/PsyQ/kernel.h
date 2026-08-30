/* PC-backend replacement for the PSX SDK's kernel.h BIOS interface: a clean-room reimplementation
 * (struct layouts and constants are public PS1 BIOS facts, not Sony's header text). Covers memory-
 * card file I/O and timer/event functions the game calls via old-GCC implicit declaration. */
#ifndef PLATFORM_PC_PSYQ_KERNEL_H
#define PLATFORM_PC_PSYQ_KERNEL_H

#include "types.h"

#define HwCARD 0x11
#define SwCARD 0x01

#define EvSpIOE     0x0004
#define EvSpTIMOUT  0x0100
#define EvSpNEW     0x2000
#define EvSpERROR   0x8000
#define EvMdNOINTR  0x2000

#define RCntCNT1 0x01

struct DIRENTRY {
    char name[20];
    int attr;
    int size;
    struct DIRENTRY *next;
    int head;
    char system[4];
};

/* s32/u32 (not int) to match core/card.c's own local `extern s32 TestEvent(s32);` declaration --
 * s32 is a 32-bit int in this project's types.h, so a native int here would conflict at LP64.
 * Relies on include/common.h pulling in types.h first, as the game does. */
s32 OpenEvent(s32 class_, s32 spec, s32 mode, s32 (*func)());
s32 EnableEvent(s32 event);
s32 TestEvent(s32 event);
u32 GetRCnt(s32 which);
u32 ResetRCnt(s32 which);

/* BIOS Kanji-ROM raw font data lookup; not declared in any real header either, same pattern as
 * OpenEvent/GetRCnt above. Signature inferred from its call sites (core/text.c, ui/window.c), both
 * of which cast the result to a pointer and compare it against -1 on failure. */

/* Returns a POINTER, so the return type must be pointer-width, not s32 (which truncates the glyph
 * address at LP64 and crashes the first battle menu). See docs/pc-port/subsystems/kernel.md,
 * "Kanji glyph lookup (`Krom2RawAdd`)". */
void *Krom2RawAdd(s32 sjisCode);

#endif
