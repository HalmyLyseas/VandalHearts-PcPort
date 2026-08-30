#!/usr/bin/env bash
# pack_input.sh -- ASan regression for the pack-input-hygiene fixes in src/pc_movie_subs.c
# (VHCUES cue geometry validation) and src/pc_hdpack.c (the HD image decode budget).

# Compiles pack_input_test.c (see its header comment) and runs five fixtures: (a/b) a bad and
# a good VHCUES cue, (c) an oversize .hdi header rejected before allocating, (d/e) a big and a
# small WebP -- budgeted then round-tripped via WebPDecodeRGBAInto.

# Fixtures c/d prove "no huge decode" by elapsed time, not by pairing ASan with a low ulimit -v
# -- ASan's shadow memory needs a large virtual address reservation up front, which a tight
# ulimit -v would break independently of this fix.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
BIN="$PC_DIR/build/pack_input_test"

if ! command -v cwebp >/dev/null 2>&1; then
    echo "pack_input: no cwebp on PATH -- skipping (see tools/regress/README.md)" >&2
    exit 2
fi
if ! pkg-config --exists libwebp 2>/dev/null; then
    echo "pack_input: no libwebp dev package (pkg-config libwebp) -- skipping" >&2
    exit 2
fi
WEBP_CFLAGS="$(pkg-config --cflags libwebp)"
WEBP_LIBS="$(pkg-config --libs libwebp)"

echo "pack_input: compiling..."
mkdir -p "$PC_DIR/build"
if ! cc -fsanitize=address -fno-omit-frame-pointer -g -O0 -DPC_PORT -DVH_HD_WEBP \
       -Iinclude $WEBP_CFLAGS \
       "$HERE/pack_input_test.c" \
       $WEBP_LIBS -lpthread -o "$BIN" 2>&1; then
    echo "pack_input: build failed" >&2
    exit 1
fi

echo "pack_input: running..."
LOG="$(mktemp)"
rc=0
( cd "$PC_DIR" && env ASAN_OPTIONS=detect_leaks=0,allocator_may_return_null=1 "$BIN" ) > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "pack_input: test binary exited $rc (ASan finding or a fixture assertion failed)" >&2
    fail=1
fi
for n in a b c d e; do
    grep -q "^fixture $n: PASS$" "$LOG" || { echo "pack_input: fixture $n did not report PASS" >&2; fail=1; }
done
grep -q "cue geometry out of bounds -- file rejected" "$LOG" || {
    echo "pack_input: fixture a's rejection message never printed" >&2; fail=1; }

rm -f "$LOG" "$BIN"
if [ "$fail" -eq 0 ]; then
    echo "pack_input: PASS"
    exit 0
else
    echo "pack_input: FAIL"
    exit 1
fi
