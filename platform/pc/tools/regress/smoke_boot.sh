#!/usr/bin/env bash
# smoke_boot.sh [path-to-exe] -- headless boot-to-title smoke test (~20-30s). VH_SMOKE=1 makes the port
# exit 0 the moment the title screen is reached, proving the whole boot chain; exit 1 = the boot stalled.
# Needs your own disc image (auto-detected, or VH_DISC_IMAGE). Details in README.md.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXE="${1:-$HERE/../../build/vandalhearts_pc}"
[ -x "$EXE" ] || { echo "smoke: no executable at $EXE (build first: make link OPT=1)" >&2; exit 2; }

echo "smoke: booting $(basename "$EXE") headless..."
if env VH_SMOKE=1 SDL_VIDEODRIVER=dummy ALSOFT_DRIVERS=null \
       VH_FULLSCREEN=0 VH_HDPACK=0 \
       "$EXE" > /tmp/vh_smoke_$$.log 2>&1; then
    grep -E "^SMOKE:" /tmp/vh_smoke_$$.log || true
    rm -f /tmp/vh_smoke_$$.log
    echo "smoke: PASS"
else
    rc=$?
    echo "smoke: FAIL (exit $rc) -- last log lines:" >&2
    tail -15 /tmp/vh_smoke_$$.log >&2
    rm -f /tmp/vh_smoke_$$.log
    exit "$rc"
fi
