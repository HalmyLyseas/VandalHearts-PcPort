/*
 * PC-backend replacement for the PSX SDK's kernel.h BIOS interface.
 *
 * Clean-room reimplementation (struct layouts and constants are public,
 * standard PS1 BIOS facts, not Sony's header text). Scope covers what the
 * game's source actually calls (per exchange/02-phase-c-interface-contract.md)
 * plus the memory-card file-I/O and timer/event functions the real game
 * uses via old-GCC implicit declaration (2026-07-10 finding: the real
 * kernel.h doesn't declare GetRCnt/ResetRCnt/OpenEvent/EnableEvent either
 * -- old K&R-permissive C allowed calling them undeclared; modern C
 * doesn't, so these need real declarations here that the original never
 * needed).
 */
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
    long attr;
    long size;
    struct DIRENTRY *next;
    long head;
    char system[4];
};

/* s32/u32 (not long) to exactly match card.c's own local `extern s32
 * TestEvent(s32);` declaration -- s32 is `int` in this project's
 * types.h, not `long` (which is 64-bit on this target), so a `long`
 * signature here would conflict. Relies on include/common.h always
 * pulling in "types.h" before any PsyQ header, same as the real project. */
s32 OpenEvent(s32 class_, s32 spec, s32 mode, s32 (*func)());
s32 EnableEvent(s32 event);
s32 TestEvent(s32 event);
u32 GetRCnt(s32 which);
u32 ResetRCnt(s32 which);

/* BIOS Kanji-ROM raw font data lookup. Not declared in any real header
 * either (same "undeclared anywhere, relies on old GCC's implicit-
 * declaration leniency" pattern as OpenEvent/GetRCnt above) -- signature
 * inferred from its 2 call sites (text.c, window.c), both of which cast
 * the result to a pointer and compare it against -1 on failure. The US
 * release ships no Kanji ROM data, so real hardware would also always
 * fail this lookup -- returning -1 unconditionally is the *correct*
 * behavior here, not a placeholder. */
s32 Krom2RawAdd(s32 sjisCode);

#endif
