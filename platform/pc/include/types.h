/* Mirrors the project's own include/types.h (s8/u8/.../f32/f64) for the clean-room PsyQ headers.
 * Unlike the original it takes int8_t..uint64_t from <stdint.h>: the PC build is not -nostdinc, and
 * a local `typedef char int8_t` is a hard conflict with the host's `signed char` definition. */
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
