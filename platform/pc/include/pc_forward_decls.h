/* Forward declarations for functions called before their definition in the same unit whose return
 * type is not `int` (gnu89's implicit int makes the later definition a "conflicting types" error).
 * Force-included via -include. See docs/pc-port/bootstrap.md, "Force-included headers". */
#ifndef PC_FORWARD_DECLS_H
#define PC_FORWARD_DECLS_H

/* -include forces this to the very start of the translation unit, before
 * common.h/types.h would normally run -- pull in s32/u32 ourselves. */
#include "types.h"

u32 *Movie_GetNextFrame(void);
s32 CountViewObstructions(s8 z, s8 x, s32 angle, u8 dir, s32 param_5);

/* Krom2RawAdd (BIOS B(51h), platform/pc/src/libkernel.c) returns a POINTER to a glyph bitmap, and
 * src/core/text.c + src/ui/window.c call it without PsyQ/kernel.h in scope: implicit `int` truncates
 * the address at -m64 (cltq after the call) and DrawSjisGlyph faults. Must be forced in here. */
void *Krom2RawAdd(s32 sjisCode);

/* Same hazard: of ~170 implicitly-declared calls across src/, these are the only other definitions
 * that RETURN A POINTER (5 call sites between them). Without a declaration the result is `int`,
 * truncated at -m64 the moment a caller stores it. */
struct UnitStatus *FindUnitByNameIdx(s16 nameIdx);
struct Object *FindUnitSpriteByNameIdx(u8 nameIdx);

#endif
