#!/usr/bin/env bash
# fault_retry.sh -- regression test for src/pc_bootstrap.c's PC_MakePageWritable /
# PC_AddrInMainImage: the SIGSEGV handler's read-only-write retry must succeed for the main
# executable's own image, and must NOT retry (i.e. crash for real) for a write outside it.

# Compiles fault_retry_test.c, which #includes src/pc_bootstrap.c directly (its crash-handling
# pieces are `static`) with stubs for the CD/GPU externs it never calls. Linux x86/x86_64 only,
# like the code under test.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
BIN="$(mktemp -u)"

echo "fault_retry: compiling..."
if ! cc -g -O0 -DPC_PORT -Wall -Wextra -Wno-unused-parameter \
       -I"$PC_DIR/include" -I"$PC_DIR/src" "$HERE/fault_retry_test.c" -o "$BIN" 2>&1; then
    echo "fault_retry: build failed -- skipping (see tools/regress/README.md)" >&2
    exit 2
fi

fail=0

echo "fault_retry: main-image write..."
out="$("$BIN" main-image 2>&1)"; rc=$?
if [ "$rc" -eq 0 ] && printf '%s' "$out" | grep -q "main-image write survived"; then
    echo "fault_retry: PASS -- main-image write was retried and continued"
else
    echo "fault_retry: FAIL -- main-image write did not survive (rc=$rc)" >&2
    printf '%s\n' "$out" | sed 's/^/    /' >&2
    fail=1
fi

echo "fault_retry: outside-main-image write..."
out="$("$BIN" outside 2>&1)"; rc=$?
# A real SIGSEGV kills the process with signal 11 -- bash reports that as exit code 128+11.
if [ "$rc" -eq 139 ] && printf '%s' "$out" | grep -q "CRASH: fatal signal"; then
    echo "fault_retry: PASS -- outside-main-image write was correctly refused and crashed"
else
    echo "fault_retry: FAIL -- outside-main-image write was NOT refused (rc=$rc, expected 139/SIGSEGV)" >&2
    printf '%s\n' "$out" | sed 's/^/    /' >&2
    fail=1
fi

rm -f "$BIN"
if [ "$fail" -eq 0 ]; then
    echo "fault_retry: PASS"
    exit 0
else
    echo "fault_retry: FAIL"
    exit 1
fi
