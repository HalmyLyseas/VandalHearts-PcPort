/* CMake-build-only force-include prelude for the decompiled game C sources.
 *
 * It combines the two things the Makefile passes on the command line -- `-D'asm(x)='` and
 * `-include pc_forward_decls.h` -- into ONE header, because CMake per-source COMPILE_OPTIONS both
 * (a) drop the function-like `asm(x)=` macro and (b) de-duplicate repeated `-include` flags, so the
 * two-flag command-line form cannot be reproduced. A single `-include cmake_game_prelude.h` sidesteps
 * both. See platform/pc/CMakeLists.txt.
 *
 * `#define asm(x)` neutralises the two `register T v asm("reg")` MIPS register-binding hints
 * (spells/support_magic.c, maps/unpack.c); real `__asm__(...)` inline-asm is untouched. */
#ifndef asm
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
