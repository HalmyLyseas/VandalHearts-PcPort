#!/usr/bin/env bash
# quit_confirm.sh -- regression for Escape's quit confirmation (src/pc_overlay.c CONF_QUIT).

# Compiles quit_confirm_test.c (see its header comment) and runs four fixtures against
# pc_overlay.c's real state machine: (a) Escape on a closed overlay opens the CONFIRM screen on
# CONF_QUIT with NO as the safe default; (b) the Back button closes the overlay entirely, not MAIN.

# (c) Escape on an already-open overlay acts as Back; (d) selecting YES, QUIT and confirming
# reaches the quit action (observed via a hook, so the harness process never exits).
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
if ! cc -fno-builtin-csqrt -m64 -std=gnu89 -DPERMUTER -I"$STAGE_DIR" -Iinclude \
       "$HERE/quit_confirm_test.c" -o "$BIN" 2>&1; then
    echo "quit_confirm: build failed" >&2
    exit 1
fi

echo "quit_confirm: running..."
LOG="$(mktemp)"
rc=0
( cd "$PC_DIR" && "$BIN" ) > "$LOG" 2>&1 || rc=$?
cat "$LOG"

fail=0
if [ "$rc" -ne 0 ]; then
    echo "quit_confirm: test binary exited $rc (a fixture assertion failed)" >&2
    fail=1
fi
for n in a b c d; do
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
