/*
 * Forward declarations for a small number of functions that are called
 * before their own definition within the same translation unit, where the
 * real definition's return type isn't "int" -- an implicit declaration
 * assumes int, and a later real definition with a genuinely different
 * (non-promotion-compatible) return type is a hard "conflicting types"
 * error under every C standard mode on modern GCC, unlike an ordinary
 * missing-prototype warning (which -std=gnu89 already tolerates, see
 * exchange/10-phase-c-real-compilation.md). The real old-GCC-2.x PSX
 * toolchain tolerates this without any prototype at all; this isn't part
 * of the PSX-API swap surface, just a compiler-strictness gap only
 * surfaced by attempting real host compilation. Force-included for every
 * translation unit via the PC Makefile's -include flag -- harmless
 * elsewhere (unused prototypes), so no per-file special-casing needed.
 */
#ifndef PC_FORWARD_DECLS_H
#define PC_FORWARD_DECLS_H

/* -include forces this to the very start of the translation unit, before
 * common.h/types.h would normally run -- pull in s32/u32 ourselves. */
#include "types.h"

u32 *Movie_GetNextFrame(void);
s32 func_800C4350(s8 z, s8 x, s32 angle, u8 dir, s32 param_5);

/* Stage 2.3: Krom2RawAdd (BIOS B(51h), implemented in platform/pc/src/libkernel.c) returns a
 * POINTER to a glyph bitmap. src/text.c and src/window.c call it with no declaration in scope --
 * neither includes PsyQ/kernel.h -- so gnu89's implicit rule made it `int Krom2RawAdd()`, i.e. a
 * 32-BIT return. At -m64 the compiler emitted `cltq` after the call, sign-extending a truncated
 * address, and DrawSjisGlyph SIGSEGV'd on the first battle menu. Fixing the header alone was NOT
 * enough precisely because those files never see it; the declaration has to be forced in here,
 * which is exactly what this file exists for. */
void *Krom2RawAdd(s32 sjisCode);

/* Same hazard, found by sweeping every implicitly-declared call across src/ and keeping only the
 * ones whose definition RETURNS A POINTER (3 of ~170 -- the rest return void/int and are
 * harmless). Without a declaration these return `int`, so at -m64 their result is truncated the
 * moment a caller stores it. FindUnitSpriteByNameIdx has 4 such call sites and
 * FindUnitByNameIdx 1, i.e. these were live crashes waiting for the right scene. */
struct UnitStatus *FindUnitByNameIdx(s16 nameIdx);
struct Object *FindUnitSpriteByNameIdx(u8 nameIdx);

#endif
