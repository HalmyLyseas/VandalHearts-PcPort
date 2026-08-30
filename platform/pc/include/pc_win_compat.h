#ifndef PC_WIN_COMPAT_H
#define PC_WIN_COMPAT_H
/* Windows (MinGW-w64) shims, force-included ahead of every translation unit on the Windows build
 * only (CMakeLists `if(WIN32) add_compile_options(-include ...)`): MinGW's <sys/types.h> lacks the
 * BSD u_char/u_short/u_int/u_long aliases the clean-room PsyQ headers use for packet fields. */
#if defined(_WIN32)
/* u_long is `unsigned long`: 32-bit under Win64's LLP64 model, the same width as the PSX `long`, so
 * packet/struct layouts stay correct. */
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

#endif /* PC_WIN_COMPAT_H */
