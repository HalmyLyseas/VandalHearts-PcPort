#!/bin/sh
# Runs the AddressSanitizer build (make asan32) with the options this port needs.
# See docs/memory-safety.md, "Running the ASan build".

# The 32-bit build is required: at 64-bit ASan's shadow memory collides with the PSX RAM
# arena. See docs/memory-safety.md, "AddressSanitizer — 32-bit only".

# The binary must run from platform/pc/ -- it resolves the disc image and saves/ relative to cwd.

cd "$(dirname "$0")" || exit 1

if [ ! -x build_asan32/vandalhearts_pc ]; then
    echo "build_asan32/vandalhearts_pc not found -- run 'make asan32' first." >&2
    exit 1
fi

# Archive the previous run's reports rather than deleting them -- a sweep's whole value is
# the accumulated report set, so even a short unrelated run must not discard earlier findings.
if ls asan.log.* >/dev/null 2>&1; then
    mkdir -p asan_runs
    stamp=$(date +%Y%m%d-%H%M%S)
    for f in asan.log.*; do mv "$f" "asan_runs/${stamp}-${f}"; done
    echo "archived previous reports to asan_runs/${stamp}-*"
fi

# VH_SCALE is display-only and cannot affect what ASan reports (default 4; override with
# `VH_SCALE=2 ./run_asan.sh`). VH_RCNT1_NORMALIZE compensates for a sanitizer-induced slow-host
# stall, not a game bug. See docs/memory-safety.md, "Running the ASan build".
ASAN_OPTIONS="handle_segv=0:halt_on_error=0:detect_leaks=0:log_path=asan.log:print_stacktrace=1:strict_string_checks=0" \
VH_SCALE="${VH_SCALE:-4}" \
VH_RCNT1_NORMALIZE="${VH_RCNT1_NORMALIZE:-1}" \
    ./build_asan32/vandalhearts_pc "$@"
status=$?

echo
if ls asan.log.* >/dev/null 2>&1; then
    echo "=== ASAN findings ==="
    grep -h "^==.*ERROR: AddressSanitizer" asan.log.* | sort | uniq -c | sort -rn
    echo
    echo "Full reports: $(ls asan.log.*)"
else
    echo "No ASAN findings."
fi
exit $status
