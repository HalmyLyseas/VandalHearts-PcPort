#!/bin/sh
# Runs the UndefinedBehaviorSanitizer build (make ubsan) with the options this port needs.
# The 64-bit complement to run_asan.sh; needs far less special-casing (no SIGSEGV handler,
# no LeakSanitizer). See docs/memory-safety.md, "UBSan — works at 64-bit".

# UBSAN_OPTIONS: halt_on_error=0 (report and continue), print_stacktrace=1, log_path=ubsan.log.
# If an enemy turn hangs with the camera panning, re-run with VH_RCNT1_NORMALIZE=1.

# Run from platform/pc/ -- the binary resolves the disc image and saves/ relative to cwd.

cd "$(dirname "$0")" || exit 1

if [ ! -x build_ubsan/vandalhearts_pc ]; then
    echo "build_ubsan/vandalhearts_pc not found -- run 'make ubsan' first." >&2
    exit 1
fi

# Archive the previous run's reports, never delete them (a sweep's value is the accumulated set).
if ls ubsan.log.* >/dev/null 2>&1; then
    mkdir -p ubsan_runs
    stamp=$(date +%Y%m%d-%H%M%S)
    for f in ubsan.log.*; do mv "$f" "ubsan_runs/${stamp}-${f}"; done
    echo "archived previous reports to ubsan_runs/${stamp}-*"
fi

# VH_SCALE is display-only (nearest-neighbour blit upscale); it cannot affect what UBSan reports.
UBSAN_OPTIONS="halt_on_error=0:print_stacktrace=1:log_path=ubsan.log" \
VH_SCALE="${VH_SCALE:-4}" \
    ./build_ubsan/vandalhearts_pc "$@"
status=$?

echo
if ls ubsan.log.* >/dev/null 2>&1; then
    echo "=== UBSan findings (distinct sites) ==="
    grep -h "runtime error:" ubsan.log.* | sort | uniq -c | sort -rn
    echo
    echo "Full reports: $(ls ubsan.log.*)"
else
    echo "No UBSan findings."
fi
exit $status
