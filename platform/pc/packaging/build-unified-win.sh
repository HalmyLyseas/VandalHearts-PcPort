#!/usr/bin/env bash
# build-unified-win.sh -- cross-compiles the unified Windows executable (both regions,
# runtime disc selection), mirroring `make unified` in three CMake stages.

#   1. VH_REGION=us  VH_UNIFIED_CORE=ON VH_BLOB=ON  -> build_win_uni_us/core_us.o
#   2. VH_REGION=jp  VH_UNIFIED_CORE=ON VH_BLOB=ON  -> build_win_uni_jp/core_jp.o
#   3. VH_UNIFIED_CORES="<both cores>"              -> build_win_uni/vandalhearts_pc.exe

# Same toolchain/static-libav conventions as make-release.sh's Windows leg. Clean builds
# always -- flag changes are not dependency-tracked between runs.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/.." && pwd)"
die() { echo "build-unified-win: $*" >&2; exit 1; }

command -v x86_64-w64-mingw32-gcc >/dev/null || die "MinGW-w64 toolchain not found"
FFPREFIX="${VH_MINGW_FFMPEG:-$PC_DIR/ffmpeg-mingw-static}"
[ -f "$FFPREFIX/lib/libavcodec.a" ] || die "static libav prefix missing ($FFPREFIX) -- run tools/build-ffmpeg-static.sh"

cd "$PC_DIR"
for R in us jp; do
    rm -rf "build_win_uni_$R"
    VH_MINGW_FFMPEG="$FFPREFIX" cmake -S . -B "build_win_uni_$R" \
        -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake \
        -DCMAKE_C_FLAGS=-O2 -DVH_MINGW_FFMPEG="$FFPREFIX" \
        -DVH_REGION=$R -DVH_UNIFIED_CORE=ON -DVH_BLOB=ON
    cmake --build "build_win_uni_$R" -j"$(nproc)"
    [ -f "build_win_uni_$R/core_$R.o" ] || die "region core $R was not produced"
done

rm -rf build_win_uni
VH_MINGW_FFMPEG="$FFPREFIX" cmake -S . -B build_win_uni \
    -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake \
    -DCMAKE_C_FLAGS=-O2 -DVH_MINGW_FFMPEG="$FFPREFIX" \
    -DVH_UNIFIED_CORES="$PC_DIR/build_win_uni_us/core_us.o;$PC_DIR/build_win_uni_jp/core_jp.o"
cmake --build build_win_uni -j"$(nproc)"
[ -f build_win_uni/vandalhearts_pc.exe ] || die "no unified .exe produced"
echo "build-unified-win: OK -> $PC_DIR/build_win_uni/vandalhearts_pc.exe"
