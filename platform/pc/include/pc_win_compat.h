#ifndef PC_WIN_COMPAT_H
#define PC_WIN_COMPAT_H
/*
 * Windows (MinGW-w64) compatibility shims, force-included ahead of every translation unit on the
 * Windows build ONLY (see CMakeLists `if(WIN32) add_compile_options(-include ...)`). Linux/macOS
 * never see this file, so their builds are unchanged.
 *
 * MinGW's <sys/types.h> does not define the BSD short type aliases (u_char/u_short/u_int/u_long)
 * that glibc supplies under __USE_MISC and that the clean-room PsyQ headers (PsyQ/libgpu.h,
 * libcd.h, libspu.h, libetc.h) use for register/packet fields. Define them here so those headers
 * compile verbatim. Widths: u_long is `unsigned long`, which is 32-bit under Win64's LLP64 model --
 * the same width as the PSX's `long`, so packet/struct layouts stay correct.
 */
#if defined(_WIN32)
typedef unsigned char  u_char;
typedef unsigned short u_short;
typedef unsigned int   u_int;
typedef unsigned long  u_long;
#endif

#endif /* PC_WIN_COMPAT_H */
