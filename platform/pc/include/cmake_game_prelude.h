/* CMake-build-only force-include prelude for the game sources (../../src/*.c).
 *
 * It combines the two things the Makefile passes on the command line -- `-D'asm(x)='` and
 * `-include pc_forward_decls.h` -- into ONE header, because CMake per-source COMPILE_OPTIONS both
 * (a) drop the function-like `asm(x)=` macro and (b) de-duplicate repeated `-include` flags, so the
 * two-flag command-line form cannot be reproduced. A single `-include cmake_game_prelude.h` sidesteps
 * both. See platform/pc/CMakeLists.txt.
 *
 * `#define asm(x)` neutralises the two `register T v asm("reg")` MIPS register-binding hints
 * (fx_057370.c, split_0a2ce0.c); real `__asm__(...)` inline-asm is untouched. */
#ifndef asm
#define asm(x)
#endif

#include "pc_forward_decls.h"
