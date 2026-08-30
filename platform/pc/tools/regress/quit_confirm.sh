#!/usr/bin/env bash
# quit_confirm.sh -- regression for Escape's quit confirmation (src/pc_overlay.c CONF_QUIT).

# Compiles quit_confirm_test.c under ASan (see its header comment) and runs six fixtures: (a) safe
# CONF_QUIT default; (b) first Escape sizing; (c/d) Back behavior; (e) observed YES quit; (f) title
# flow exits immediately while story movies retain confirmation.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
BIN="$PC_DIR/build/quit_confirm_test"

# pc_overlay.c compiles through the staged real game headers (state.h, battle.h, ...); reuse
# the same STAGE_DIR a prior `make link`/`make stage` produced rather than re-running it here.
STAGE_DIR="$PC_DIR/build/include_stage"
if [ ! -d "$STAGE_DIR" ]; then
    echo "quit_confirm: no $STAGE_DIR -- run 'make stage' (or 'make link') first" >&2
    exit 2
fi

echo "quit_confirm: compiling..."
mkdir -p "$PC_DIR/build"
if ! cc -fno-builtin-csqrt -m64 -std=gnu89 -DPERMUTER -fsanitize=address -fno-omit-frame-pointer -I"$STAGE_DIR" -I"$PC_DIR/include" \
       "$HERE/quit_confirm_test.c" -o "$BIN" 2>&1; then
    echo "quit_confirm: build failed" >&2
    exit 1
fi

echo "quit_confirm: running..."
LOG="$(mktemp)"
rc=0
( cd "$PC_DIR" && ASAN_OPTIONS=detect_leaks=0 "$BIN" ) > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "quit_confirm: test binary exited $rc (a fixture assertion failed)" >&2
    fail=1
fi
for n in a b c d e f; do
    grep -q "^fixture $n: PASS$" "$LOG" || { echo "quit_confirm: fixture $n did not report PASS" >&2; fail=1; }
done

rm -f "$LOG" "$BIN"
if [ "$fail" -eq 0 ]; then
    echo "quit_confirm: PASS"
    exit 0
else
    echo "quit_confirm: FAIL"
    exit 1
fi
