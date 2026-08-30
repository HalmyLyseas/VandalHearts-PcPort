#!/usr/bin/env bash
# Builds a minimal, static libav for the port's HD-FMV decoder (H.264/HEVC decode + mov/mp4
# demux + swscale only, no external codec libs). TARGET=mingw (default, Windows cross) or
# TARGET=native (the build host). See docs/cross-platform.md, "Toolchain".

# Output: $PREFIX/{lib/libav*.a,libswscale.a, include/libav*, include/libswscale}. Static
# libav on MinGW also needs -lbcrypt at final link (avutil uses BCryptGenRandom); CMakeLists
# adds it under VH_HDVIDEO when cross-compiling.
set -euo pipefail

TARGET=${TARGET:-mingw}
FFVER=${FFVER:-n7.1}                       # any recent release tag; matches the 8.x soname era
SRC=${SRC:-/tmp/ffmpeg-src}
if [ "$TARGET" = native ]; then
  CROSS=${CROSS:-}
  PREFIX=${PREFIX:-$PWD/ffmpeg-linux-static}
  TARGET_FLAGS=""
else
  CROSS=${CROSS:-x86_64-w64-mingw32-}
  PREFIX=${PREFIX:-$PWD/ffmpeg-mingw-static}
  TARGET_FLAGS="--arch=x86_64 --target-os=mingw32 --cross-prefix=$CROSS --enable-cross-compile"
fi

command -v "${CROSS}gcc" >/dev/null || { echo "need ${CROSS}gcc on PATH"; exit 1; }

if [ ! -d "$SRC" ] || [ ! -f "$SRC/configure" ]; then
  command -v git >/dev/null || { echo "no ffmpeg source at $SRC and no git to fetch it -- clone on the host first:";                                  echo "  git clone --depth 1 --branch $FFVER https://github.com/FFmpeg/FFmpeg.git $SRC"; exit 1; }
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
  $TARGET_FLAGS \
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
