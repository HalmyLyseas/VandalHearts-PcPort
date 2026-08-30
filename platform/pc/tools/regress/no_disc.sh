#!/usr/bin/env bash
# no_disc.sh -- headless regression for the unified launcher's missing-disc path: with no disc
# found it must exit 1 through us_PC_FatalDiscError (dialog text on stderr). Needs a `make unified`
# exe; copies it to a temp dir so the dev-repo external/{game,alt} fallback cannot resolve.
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
EXE="${1:-$HERE/../../build-uni/vandalhearts_pc}"
[ -x "$EXE" ] || { echo "no_disc: no executable at $EXE (build first: make unified)" >&2; exit 2; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
cp "$EXE" "$WORK/vandalhearts_pc"
[ -d "$WORK/game" ] && { echo "no_disc: unexpected game/ next to the copied exe" >&2; exit 2; }

fail=0

# check DESC PATTERN... -- ENV... : runs the copied exe headless with ENV set (VH_DISC_IMAGE
# always cleared first so the case controls it), asserts exit 1 and every PATTERN present on
# the combined stderr/stdout.
check() {
    local desc="$1"; shift
    local -a patterns=()
    while [ "$1" != "--" ]; do patterns+=("$1"); shift; done
    shift
    local out rc
    out="$(cd "$WORK" && env -u VH_DISC_IMAGE SDL_VIDEODRIVER=dummy ALSOFT_DRIVERS=null "$@" \
           ./vandalhearts_pc 2>&1)"
    rc=$?
    local ok=1
    [ "$rc" -eq 1 ] || ok=0
    local p
    for p in "${patterns[@]}"; do
        printf '%s' "$out" | grep -qF "$p" || ok=0
    done
    if [ "$ok" -eq 1 ]; then
        echo "no_disc: PASS -- $desc"
    else
        echo "no_disc: FAIL -- $desc (rc=$rc, expected 1; wanted patterns: ${patterns[*]})" >&2
        echo "$out" | sed 's/^/    /' >&2
        fail=1
    fi
}

# 1. No game/ beside the exe, no VH_DISC_IMAGE: the dialog's dual-channel text on stderr.
check "no disc found -- dialog text on stderr" \
    "no disc image found" "Disc path tried" --

# 2. VH_DISC_IMAGE set to a nonexistent file: still exit 1, plus the "not recognized" note.
check "VH_DISC_IMAGE unrecognized -- dialog notes it" \
    "no disc image found" "Disc path tried" "not a recognized" -- \
    VH_DISC_IMAGE=/nonexistent/x.bin

if [ "$fail" -eq 0 ]; then
    echo "no_disc: PASS (all checks)"
    exit 0
else
    echo "no_disc: FAIL" >&2
    exit 1
fi
