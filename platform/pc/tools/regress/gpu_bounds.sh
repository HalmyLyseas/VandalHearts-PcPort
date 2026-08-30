#!/usr/bin/env bash
# gpu_bounds.sh -- ASan regression for the LoadImage/StoreImage VRAM-bound and
# ParseTimSection/ReadTIM plausibility-bound fixes in src/libgpu.c.
#
# Compiles ONLY src/libgpu.c + gpu_bounds_test.c (no pc_raster.c/pc_gpu_trace.c/pc_hdpack.c/
# pc_gpu_window.c -- gpu_bounds_test.c stubs the few externs libgpu.c reaches across those
# split-out TUs, per pc_gpu_internal.h) under AddressSanitizer, then runs three fixtures:
#   1. LoadImage(rect.x = -1) and StoreImage(rect.y = -1) -- must not touch memory before vram[].
#   2. a TIM blob with a CLUT section sec[0] = 0xFFFFFFF0 -- ReadTIM must reject it (prect NULL
#      or zero-size) without parsing off into the buffer.
#   3. a valid minimal TIM -- must still parse to the expected embedded rect.
# Any real out-of-bounds access aborts the process under ASan; this script also checks each
# fixture's own PASS line.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
BIN="$PC_DIR/build/gpu_bounds_test"

echo "gpu_bounds: compiling..."
cc -fsanitize=address -fno-omit-frame-pointer -g -O0 -DPC_PORT -DPERMUTER -fno-builtin-csqrt \
   -I"$PC_DIR/include" -I"$PC_DIR/src" \
   "$PC_DIR/src/libgpu.c" "$HERE/gpu_bounds_test.c" \
   -lm -o "$BIN"

echo "gpu_bounds: running..."
LOG="$(mktemp)"
rc=0
env ASAN_OPTIONS=detect_leaks=0 "$BIN" > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "gpu_bounds: test binary exited $rc (ASan finding or a fixture assertion failed)" >&2
    fail=1
fi
for n in 1 2 3; do
    grep -q "^fixture $n: PASS$" "$LOG" || { echo "gpu_bounds: fixture $n did not report PASS" >&2; fail=1; }
done

rm -f "$LOG"
if [ "$fail" -eq 0 ]; then
    echo "gpu_bounds: PASS"
    exit 0
else
    echo "gpu_bounds: FAIL"
    exit 1
fi
