/*
 * Mirrors the real project's own include/types.h (s8/u8/.../s32/u32 etc.)
 * -- this is this project's own original type system, not PsyQ SDK
 * content, so reproducing it exactly here is not a clean-room concern.
 * Needed because platform/pc's PsyQ-compatible headers (e.g. kernel.h)
 * use these types too, matching how include/common.h always pulls in
 * "types.h" before any PsyQ header in the real build.
 *
 * Differs from the real project's include/types.h in one way: the real one
 * also (redundantly, but harmlessly under the PSX build's -nostdinc) defines
 * int8_t/int16_t/int32_t/int64_t/uint*_t itself. The PC build isn't
 * -nostdinc -- game/platform code that also pulls in real system headers
 * (SDL2, OpenAL, glibc internals) gets a genuine, hard conflict between our
 * own retyped int8_t (`char`) and the system's (`__int8_t`/`signed char`),
 * distinct types under C's redeclaration rules even though same size. Fixed
 * by including <stdint.h> for those and only defining the truly
 * project-specific s8/u8/.../f32/f64 names here.
 */
#ifndef TYPES_H
#define TYPES_H

#include <stdint.h>

typedef signed char s8;
typedef signed short s16;
typedef signed int s32;
typedef signed long long s64;
typedef unsigned char u8;
typedef unsigned short u16;
typedef unsigned int u32;
typedef unsigned long long u64;
typedef float f32;
typedef double f64;

#endif
