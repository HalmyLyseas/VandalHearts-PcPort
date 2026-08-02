#!/usr/bin/env bash
# Build a MINIMAL, STATIC libav for the Windows (x86_64-w64-mingw32) target — just what the
# port's HD-FMV decoder (pc_hdvideo.c) needs: H.264 decode + mov/mp4 demux + swscale rescale.
#
# Why: distro/MSYS2 ffmpeg is a SHARED "kitchen-sink" build — its avcodec DLL alone imports 35+
# external codec DLLs (x264/x265/aom/vpx/dav1d/jxl/cairo/glib/gnutls/...), 60-100 MB to bundle.
# This build enables ONLY the decode path, static, with no external libs, so the libs link
# straight into vandalhearts_pc.exe (~+2-4 MB) and NO ffmpeg DLLs need shipping.
#
# Output: $PREFIX/{lib/libav*.a,libswscale.a, include/libav*, include/libswscale}
# Point the port's CMake at it:  cmake ... -DCMAKE_PREFIX_PATH="$PREFIX"
# Static libav on MinGW also needs -lbcrypt at final link (avutil uses BCryptGenRandom); the
# CMakeLists adds it under VH_HDVIDEO when cross-compiling.
set -euo pipefail

CROSS=${CROSS:-x86_64-w64-mingw32-}
FFVER=${FFVER:-n7.1}                       # any recent release tag; matches the 8.x soname era
PREFIX=${PREFIX:-$PWD/ffmpeg-mingw-static}
SRC=${SRC:-/tmp/ffmpeg-src}

command -v "${CROSS}gcc" >/dev/null || { echo "need ${CROSS}gcc on PATH"; exit 1; }

if [ ! -d "$SRC/.git" ]; then
  git clone --depth 1 --branch "$FFVER" https://github.com/FFmpeg/FFmpeg.git "$SRC"
fi
cd "$SRC"

# x86 SIMD needs nasm/yasm. If absent, fall back to a C-only ("crippled") build -- fine for our
# light 15fps decode, but installing nasm (e.g. pacman -S nasm) gives faster SIMD for the release.
ASM_FLAG=
if ! command -v nasm >/dev/null && ! command -v yasm >/dev/null; then
  echo "WARN: no nasm/yasm -> building C-only (--disable-x86asm); install nasm for SIMD."
  ASM_FLAG=--disable-x86asm
fi

./configure ${ASM_FLAG:+$ASM_FLAG} \
  --prefix="$PREFIX" \
  --arch=x86_64 --target-os=mingw32 --cross-prefix="$CROSS" --enable-cross-compile \
  --enable-static --disable-shared --enable-pic \
  --disable-programs --disable-doc --disable-network --disable-autodetect \
  --disable-avdevice --disable-avfilter --disable-postproc --disable-swresample \
  --disable-everything \
  --enable-avformat --enable-avcodec --enable-swscale --enable-avutil \
  --enable-decoder=h264 --enable-parser=h264 \
  --enable-decoder=hevc --enable-parser=hevc \
  --enable-demuxer=mov --enable-protocol=file \
  --disable-zlib --disable-bzlib --disable-lzma --disable-iconv --disable-sdl2 --disable-schannel

make -j"$(nproc)"
make install
echo
echo "== done: static libav in $PREFIX =="
ls -sh "$PREFIX"/lib/lib{avformat,avcodec,avutil,swscale}.a 2>/dev/null || true
echo "configure the port with:  cmake -S . -B build_win \\"
echo "    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake -DCMAKE_PREFIX_PATH=$PREFIX"
