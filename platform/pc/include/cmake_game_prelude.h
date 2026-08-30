/* CMake-build-only force-include prelude for the game sources: one header standing in for the
 * Makefile's `-D'asm(x)='` + `-include pc_forward_decls.h`, since CMake per-source COMPILE_OPTIONS
 * drop function-like macros and de-duplicate repeated -include flags. See CMakeLists.txt. */
#ifndef asm
/* Neutralises the two `register T v asm("reg")` MIPS register-binding hints (spells/support_magic.c,
 * maps/unpack.c); real `__asm__(...)` inline asm is untouched. */
#define asm(x)
#endif

#include "pc_forward_decls.h"
#if defined(__APPLE__)
#include "common.h"
#include "graphics.h"
#include "object.h"
#include "units.h"
#include "field.h"
#include "battle.h"
#include "window.h"
#include "audio.h"
#include "state.h"
#include "apple_void_forward_decls.h"
#endif
