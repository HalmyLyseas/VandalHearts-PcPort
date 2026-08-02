# CMake toolchain file: cross-compile the native PC port to a 64-bit Windows .exe from Linux,
# using the MinGW-w64 GCC cross-toolchain (Stage 2.4).
#
#   cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake
#   cmake --build build_win
#
# Produces build_win/vandalhearts_pc.exe. SDL2 + OpenAL for the w64-mingw32 target must be present
# in the sysroot below (Arch/CachyOS: `paru -S mingw-w64-sdl2 mingw-w64-openal`; OpenGL's import lib
# ships with the toolchain). The runtime DLLs are copied next to the .exe by CMakeLists.
#
# Win64 is LLP64 (`long` is 32-bit) vs Linux LP64 (`long` is 64-bit). Harmless here: Stage 2.3
# mapped PSX `long`->`int` throughout the PC layer, so no struct depends on `long`'s width.

set(CMAKE_SYSTEM_NAME Windows)
set(CMAKE_SYSTEM_PROCESSOR x86_64)

set(MINGW_TARGET x86_64-w64-mingw32)
set(CMAKE_C_COMPILER   ${MINGW_TARGET}-gcc)
set(CMAKE_CXX_COMPILER ${MINGW_TARGET}-g++)
set(CMAKE_RC_COMPILER  ${MINGW_TARGET}-windres)

# The toolchain's sysroot: where mingw headers/import-libs (and the AUR SDL2/OpenAL) live.
set(CMAKE_FIND_ROOT_PATH /usr/${MINGW_TARGET})

# 1.6: an extra prefix for a locally-built STATIC libav (the HD-video decoder) can be searched too.
# With MODE ONLY below, find_library/find_path only look under CMAKE_FIND_ROOT_PATH, so a host
# CMAKE_PREFIX_PATH is ignored -- the prefix must be appended here. Set it with -DVH_MINGW_FFMPEG=<prefix>
# or the VH_MINGW_FFMPEG environment variable (make-release.sh does the latter).
if(VH_MINGW_FFMPEG)
    list(APPEND CMAKE_FIND_ROOT_PATH "${VH_MINGW_FFMPEG}")
elseif(DEFINED ENV{VH_MINGW_FFMPEG})
    list(APPEND CMAKE_FIND_ROOT_PATH "$ENV{VH_MINGW_FFMPEG}")
endif()

# Search host paths for programs (python3, etc.) but only the sysroot for headers/libs/packages,
# so we never accidentally link a host .so or pick up a Linux SDL2Config.cmake.
set(CMAKE_FIND_ROOT_PATH_MODE_PROGRAM NEVER)
set(CMAKE_FIND_ROOT_PATH_MODE_LIBRARY ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_INCLUDE ONLY)
set(CMAKE_FIND_ROOT_PATH_MODE_PACKAGE ONLY)
