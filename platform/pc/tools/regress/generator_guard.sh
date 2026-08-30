#!/usr/bin/env bash
# generator_guard.sh -- fast (no build needed) regression test for build_data_segment.py's
# PSX_EXE guard: a wrong-region or truncated executable must be refused, and
# VH_ALLOW_UNVERIFIED_EXE=1 must skip only the md5 check, never the size check.

# Uses `python3 build_data_segment.py --validate-only`, which runs just ValidatePsxExe() -- no
# linking, probing, or byte extraction -- so this needs neither a prior `make link` nor the
# mipsel toolchain. Needs the US and JP retail executables (same as any generator run).
set -uo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
GEN="$PC_DIR/tools/build_data_segment.py"
PROJECT_ROOT="$(cd "$PC_DIR/../.." && pwd)"
US_EXE="$PROJECT_ROOT/SLUS_004.47"
JP_EXE="$PROJECT_ROOT/jp/SLPM_860.07"
[ -f "$US_EXE" ] || { echo "generator_guard: no $US_EXE (need your own disc's executable)" >&2; exit 2; }
[ -f "$JP_EXE" ] || { echo "generator_guard: no $JP_EXE (need your own disc's executable)" >&2; exit 2; }

WORK="$(mktemp -d)"
trap 'rm -rf "$WORK"' EXIT
fail=0

# check DESC EXPECT_RC GREP_PATTERN -- ENV... : runs the validator with ENV set, asserts the exit
# code matches EXPECT_RC (0 or nonzero) and that GREP_PATTERN appears in its combined output.
check() {
    local desc="$1" expect_rc="$2" pattern="$3"; shift 3
    local out rc
    out="$(cd "$PC_DIR" && env "$@" python3 "$GEN" --validate-only 2>&1)"
    rc=$?
    if [ "$expect_rc" = "0" ]; then ok=$([ "$rc" -eq 0 ] && echo 1 || echo 0)
    else ok=$([ "$rc" -ne 0 ] && echo 1 || echo 0); fi
    if [ "$ok" = "1" ] && printf '%s' "$out" | grep -qF "$pattern"; then
        echo "generator_guard: PASS -- $desc"
    else
        echo "generator_guard: FAIL -- $desc (rc=$rc, expected ${expect_rc}; wanted pattern: $pattern)" >&2
        echo "$out" | sed 's/^/    /' >&2
        fail=1
    fi
}

# 1. A JP executable presented to a US-region check must fail with the hash message.
check "wrong-region exe fails on hash mismatch" nonzero "does not match the known US retail" \
    VH_REGION=us VH_PSX_EXE="$JP_EXE"

# 2. A truncated copy of the US executable must fail with the size message.
TRUNC="$WORK/truncated.exe"
head -c 100000 "$US_EXE" > "$TRUNC"
check "truncated exe fails on size check" nonzero "looks truncated" \
    VH_PSX_EXE="$TRUNC"

# 3. VH_ALLOW_UNVERIFIED_EXE=1 skips only the hash check: the truncated copy must still fail.
check "VH_ALLOW_UNVERIFIED_EXE=1 does not skip the size check" nonzero "looks truncated" \
    VH_PSX_EXE="$TRUNC" VH_ALLOW_UNVERIFIED_EXE=1

# 4. VH_ALLOW_UNVERIFIED_EXE=1 on a byte-patched (but full-size) copy must warn and proceed.
PATCHED="$WORK/patched.exe"
cp "$US_EXE" "$PATCHED"
python3 - "$PATCHED" <<'PYEOF'
import sys
path = sys.argv[1]
with open(path, 'r+b') as f:
    f.seek(0x900)                # past the 0x800 header: a body byte, not the header itself
    b = f.read(1)
    f.seek(0x900)
    f.write(bytes([b[0] ^ 0xFF]))
PYEOF
check "VH_ALLOW_UNVERIFIED_EXE=1 warns and proceeds on a patched exe" 0 "WARNING" \
    VH_PSX_EXE="$PATCHED" VH_ALLOW_UNVERIFIED_EXE=1

# 5. The real, unmodified US and JP executables must both still pass outright.
check "the real US exe still validates" 0 "PSX_EXE OK" \
    VH_PSX_EXE="$US_EXE"
check "the real JP exe still validates" 0 "PSX_EXE OK" \
    VH_REGION=jp VH_PSX_EXE="$JP_EXE"

if [ "$fail" -eq 0 ]; then
    echo "generator_guard: PASS (all checks)"
    exit 0
else
    echo "generator_guard: FAIL" >&2
    exit 1
fi
