#!/usr/bin/env bash
# audio_bounds.sh -- ASan regression for the SsVabClose/VAB-size-table/SEQ-parser bounds fixes in src/libsnd.c + src/pc_spu.c; compiles ONLY src/libsnd.c + src/pc_spu.c + src/libspu.c + audio_bounds_test.c (no core/cd.c, no generated data segment -- the test file stubs the few externs libsnd.c reaches for there) under AddressSanitizer.
# Runs three synthetic fixtures (see audio_bounds_test.c for what each builds); any real out-of-bounds access aborts the process under ASan.
# This script also checks each fixture's own PASS line and (fixture 2) that the bounds guard's own diagnostic fired.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
STAGE_DIR="$PC_DIR/build/include_stage"
BIN="$PC_DIR/build/audio_bounds_test"

if [ ! -d "$STAGE_DIR" ]; then
    echo "audio_bounds: no $STAGE_DIR -- run 'make link OPT=1' once first (stages headers)" >&2
    exit 2
fi

echo "audio_bounds: compiling..."
cc -fsanitize=address -fno-omit-frame-pointer -g -O0 -DPC_PORT -DPERMUTER -fno-builtin-csqrt \
   -Iinclude -I"$STAGE_DIR" -I/usr/include/AL \
   "$PC_DIR/src/libsnd.c" "$PC_DIR/src/pc_spu.c" "$PC_DIR/src/libspu.c" \
   "$HERE/audio_bounds_test.c" \
   -lopenal -lm -o "$BIN"

echo "audio_bounds: running..."
LOG="$(mktemp)"
rc=0
env ALSOFT_DRIVERS=null ASAN_OPTIONS=detect_leaks=0 "$BIN" > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "audio_bounds: test binary exited $rc (ASan finding or a fixture assertion failed)" >&2
    fail=1
fi
for n in 1 2 3; do
    grep -q "^fixture $n: PASS$" "$LOG" || { echo "audio_bounds: fixture $n did not report PASS" >&2; fail=1; }
done
grep -qi "out of bounds" "$LOG" || { echo "audio_bounds: fixture 2's bounds guard never fired" >&2; fail=1; }

rm -f "$LOG"
if [ "$fail" -eq 0 ]; then
    echo "audio_bounds: PASS"
    exit 0
else
    echo "audio_bounds: FAIL"
    exit 1
fi
