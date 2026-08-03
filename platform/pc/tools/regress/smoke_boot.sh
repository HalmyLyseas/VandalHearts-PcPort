#!/usr/bin/env bash
# smoke_boot.sh [path-to-exe] -- headless boot-to-title smoke test (~20-30s).
#
# Boots the real game with VH_SMOKE=1: the port exits 0 the moment the title screen is reached,
# which proves the whole boot chain end-to-end -- data-segment constructors, disc auto-detect +
# mount, the MDEC logo movie, SPU/XA init, the kanji font, and the rasterizer. Exit 1 = the boot
# stalled (the failure mode an empty/broken data segment or a bad disc path produces). Runs
# window-less (SDL dummy video driver; the present path no-ops without a GL context) and with
# OpenAL's null backend so no audio device is needed -- safe on a headless box or in a container.
#
# Needs the same runtime data a normal run does: your own disc image (auto-detected, or set
# VH_DISC_IMAGE). Run after any change that could affect boot: data-segment/generator changes,
# backend init order, disc handling, movie/MDEC, SPU init.
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
