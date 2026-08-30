#!/usr/bin/env bash
# gte_width_check.sh -- PR C / finding 2.6: build + run gte_width_check.c natively, and cross-
# compile it with the MinGW-w64 toolchain (running it there too if `wine` is available).
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
SRC="$HERE/gte_width_check.c"
OUT_DIR="$(mktemp -d)"
trap 'rm -rf "$OUT_DIR"' EXIT

echo "gte_width_check: building native..."
cc -O2 -Wall -Wextra -o "$OUT_DIR/gte_width_check" "$SRC"
"$OUT_DIR/gte_width_check"

if command -v x86_64-w64-mingw32-gcc >/dev/null 2>&1; then
    echo "gte_width_check: building with x86_64-w64-mingw32-gcc..."
    x86_64-w64-mingw32-gcc -O2 -Wall -Wextra -o "$OUT_DIR/gte_width_check.exe" "$SRC"
    if command -v wine >/dev/null 2>&1; then
        echo "gte_width_check: running under wine..."
        wine "$OUT_DIR/gte_width_check.exe"
    else
        echo "gte_width_check: wine not found -- compiled only, not run"
    fi
else
    echo "gte_width_check: x86_64-w64-mingw32-gcc not found -- skipping MinGW build" >&2
fi

echo "gte_width_check: PASS"
