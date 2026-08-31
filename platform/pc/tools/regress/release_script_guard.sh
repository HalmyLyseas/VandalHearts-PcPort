#!/usr/bin/env bash
# release_script_guard.sh -- fast regression for make-release.sh + smoke_boot.sh guard logic; no build, no network.
# Exercises the tag/path/symlink guards via VH_RELEASE_DRY_RUN=1 (stops make-release.sh right after its correctness gates, before any build) and re-runs the hdpack packaging block in isolation (extracted verbatim from the script -- it only needs `zip`, no build) on tiny fixture directories.
# Also runs smoke_boot.sh against /bin/true and the real exe.
set -euo pipefail
HERE="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
PC_DIR="$(cd "$HERE/../.." && pwd)"
RELEASE="$PC_DIR/packaging/make-release.sh"
SMOKE="$PC_DIR/tools/regress/smoke_boot.sh"
FAIL=0

pass() { printf '  PASS: %s\n' "$*"; }
fail() { printf '  FAIL: %s\n' "$*" >&2; FAIL=1; }

TMP="$(mktemp -d)"
trap 'rm -rf "$TMP"' EXIT

echo "== bash -n =="
bash -n "$RELEASE" && pass "make-release.sh syntax" || fail "make-release.sh syntax"
bash -n "$SMOKE" && pass "smoke_boot.sh syntax" || fail "smoke_boot.sh syntax"

echo "== case 1: bad tag refused before any rm -rf =="
DIST="$PC_DIR/dist/release"
# Prove no directory gets created/touched for a path-traversal tag: snapshot dist/release (or its
# absence) before, run with a hostile tag, then confirm nothing changed.
BEFORE="absent"; [ -d "$DIST" ] && BEFORE="$(find "$DIST" -maxdepth 1 | sort)"
if VH_RELEASE_DRY_RUN=1 "$RELEASE" '../../x' --no-publish >"$TMP/bad_tag.log" 2>&1; then
    fail "bad tag '../../x' was accepted (exit 0)"
else
    rc=$?
    AFTER="absent"; [ -d "$DIST" ] && AFTER="$(find "$DIST" -maxdepth 1 | sort)"
    if [ "$rc" -ne 0 ] && [ "$BEFORE" = "$AFTER" ]; then
        pass "bad tag rejected (exit $rc), dist/release untouched"
    else
        fail "bad tag rejected but dist/release changed (before='$BEFORE' after='$AFTER')"
    fi
fi

echo "== case 2: good tag + dry run passes the guards =="
if VH_RELEASE_DRY_RUN=1 "$RELEASE" v9.9.9-test --no-publish >"$TMP/good_tag.log" 2>&1; then
    pass "v9.9.9-test --no-publish (dry run) exits 0 after the guards"
else
    fail "v9.9.9-test --no-publish (dry run) should have passed the guards: $(tail -5 "$TMP/good_tag.log")"
fi
rm -rf "$PC_DIR/dist/release/v9.9.9-test"

# These call the real script in publish mode, but VH_RELEASE_DRY_RUN stops after input and
# correctness guards: no build, gh invocation, or network access is possible. Publishing must
# carry both supported HD packs even for a one-platform artifact release.
run_publish_dry() {   # run_publish_dry <log> [make-release flags...]
    local out="$1"
    shift
    VH_RELEASE_DRY_RUN=1 "$RELEASE" v9.9.8-hdguard "$@" >"$out" 2>&1
}

echo "== case 2a: publish requires an explicit --hdpack even when the environment has one =="
mkdir -p "$TMP/valid-env/SLUS-00447" "$TMP/valid-env/SLPM-86007"
echo '{}' > "$TMP/valid-env/SLUS-00447/manifest.json"
echo '{}' > "$TMP/valid-env/SLPM-86007/manifest.json"
if VH_HDPACK_DIR="$TMP/valid-env" run_publish_dry "$TMP/no_explicit_hdpack.log" --windows-only; then
    fail "publish accepted VH_HDPACK_DIR without explicit --hdpack"
else
    grep -q 'publishing requires --hdpack=' "$TMP/no_explicit_hdpack.log" \
        && pass "publish rejects missing explicit --hdpack" \
        || fail "missing --hdpack was rejected for the wrong reason: $(tail -5 "$TMP/no_explicit_hdpack.log")"
fi

echo "== case 2b: publish rejects a source missing either supported pack =="
mkdir -p "$TMP/us-only/SLUS-00447"
echo '{}' > "$TMP/us-only/SLUS-00447/manifest.json"
if run_publish_dry "$TMP/missing_jp.log" --linux-only --hdpack="$TMP/us-only"; then
    fail "publish accepted a HD-pack root missing SLPM-86007"
else
    grep -q 'exactly the SLUS-00447 and SLPM-86007' "$TMP/missing_jp.log" \
        && pass "publish rejects a root missing SLPM-86007 (also under --linux-only)" \
        || fail "missing SLPM-86007 was rejected for the wrong reason: $(tail -5 "$TMP/missing_jp.log")"
fi
mkdir -p "$TMP/jp-only/SLPM-86007"
echo '{}' > "$TMP/jp-only/SLPM-86007/manifest.json"
if run_publish_dry "$TMP/missing_us.log" --windows-only --hdpack="$TMP/jp-only"; then
    fail "publish accepted a HD-pack root missing SLUS-00447"
else
    grep -q 'exactly the SLUS-00447 and SLPM-86007' "$TMP/missing_us.log" \
        && pass "publish rejects a root missing SLUS-00447 (also under --windows-only)" \
        || fail "missing SLUS-00447 was rejected for the wrong reason: $(tail -5 "$TMP/missing_us.log")"
fi

echo "== case 2c: publish rejects an extra per-game manifest =="
mkdir -p "$TMP/three-packs/SLUS-00447" "$TMP/three-packs/SLPM-86007" "$TMP/three-packs/OTHER-00000"
for d in "$TMP/three-packs"/*; do echo '{}' > "$d/manifest.json"; done
if run_publish_dry "$TMP/extra_pack.log" --hdpack="$TMP/three-packs"; then
    fail "publish accepted an unsupported third HD-pack manifest"
else
    grep -q 'exactly the SLUS-00447 and SLPM-86007' "$TMP/extra_pack.log" \
        && pass "publish rejects an extra per-game manifest" \
        || fail "extra manifest was rejected for the wrong reason: $(tail -5 "$TMP/extra_pack.log")"
fi

echo "== case 2d: a valid pair is accepted in publish mode without builds or network =="
if run_publish_dry "$TMP/valid_pair.log" --windows-only --hdpack="$TMP/valid-env"; then
    if grep -q 'dry run: guards passed, stopping before any build' "$TMP/valid_pair.log" \
       && ! grep -q 'Publishing GitHub release\|gh release create' "$TMP/valid_pair.log"; then
        pass "valid explicit pair passes publish preflight under --windows-only without build/network"
    else
        fail "valid pair did not stop at the dry-run boundary: $(tail -5 "$TMP/valid_pair.log")"
    fi
else
    fail "valid explicit HD-pack pair should pass publish preflight: $(tail -5 "$TMP/valid_pair.log")"
fi
rm -rf "$PC_DIR/dist/release/v9.9.8-hdguard"

# Pull the hdpack packaging block (two functions + the dispatch if) verbatim out of the real
# script, so cases 3 and 4 exercise the actual packaging logic, not a reimplementation of it.
HDPACK_BLOCK="$(sed -n '/^HDPACK_ROWS=()/,/^fi$/p' "$RELEASE")"
[ -n "$HDPACK_BLOCK" ] || fail "could not extract the hdpack block from make-release.sh (script layout changed?)"

run_hdpack_block() {   # run_hdpack_block <TAG> <STAGE-dir> <HDPACK_SRC-dir>
    ( TAG="$1" STAGE="$2" HDPACK_SRC="$3"
      log()  { :; }
      die()  { printf 'die: %s\n' "$*" >&2; exit 1; }
      eval "$HDPACK_BLOCK"
      printf '%s\n' "${HDPACK_ROWS[@]+"${HDPACK_ROWS[@]}"}" )
}

echo "== case 3: symlink escaping the hdpack dir is refused =="
mkdir -p "$TMP/case3/SLUS-00447" "$TMP/case3_outside"
echo '{}' > "$TMP/case3/SLUS-00447/manifest.json"
touch "$TMP/case3_outside/secret.txt"
ln -s "$TMP/case3_outside/secret.txt" "$TMP/case3/SLUS-00447/leak"
if OUT="$(run_hdpack_block v1.0.0 "$TMP/case3_stage" "$TMP/case3" 2>&1)"; then
    fail "escaping symlink was NOT refused: $OUT"
else
    case "$OUT" in
        *"points outside the pack"*) pass "escaping symlink refused: $(printf '%s' "$OUT" | grep 'points outside')" ;;
        *) fail "refused for the wrong reason: $OUT" ;;
    esac
fi

echo "== case 4: per-game hdpack tree names one zip per game folder =="
mkdir -p "$TMP/case4/SLUS-00447" "$TMP/case4/SLPM-86007" "$TMP/case4_stage"
echo '{}' > "$TMP/case4/SLUS-00447/manifest.json"
echo '{}' > "$TMP/case4/SLPM-86007/manifest.json"
ROWS="$(run_hdpack_block v2.0.0 "$TMP/case4_stage" "$TMP/case4")"
EXP_US="VandalHearts-v2.0.0-hdpack-SLUS-00447.zip"
EXP_JP="VandalHearts-v2.0.0-hdpack-SLPM-86007.zip"
if [ -f "$TMP/case4_stage/$EXP_US" ] && [ -f "$TMP/case4_stage/$EXP_JP" ]; then
    pass "per-game layout produced $EXP_US and $EXP_JP"
else
    fail "expected $EXP_US and $EXP_JP in $TMP/case4_stage, found: $(ls "$TMP/case4_stage" 2>&1)"
fi
if printf '%s' "$ROWS" | grep -q "$EXP_US" && printf '%s' "$ROWS" | grep -q "$EXP_JP"; then
    pass "notes-table rows list exactly the two zips produced"
else
    fail "notes-table rows do not list both zips: $ROWS"
fi
NZIPS=$(ls "$TMP/case4_stage"/VandalHearts-*.zip 2>/dev/null | wc -l)
[ "$NZIPS" -eq 2 ] && pass "exactly 2 zips produced (no extra/legacy zip)" \
                    || fail "expected exactly 2 zips, found $NZIPS"

echo "== case 4b: legacy flat layout still works, with a deprecation warning =="
mkdir -p "$TMP/case4b" "$TMP/case4b_stage"
echo '{}' > "$TMP/case4b/manifest.json"
OUT4B="$( ( TAG=v2.0.0 STAGE="$TMP/case4b_stage" HDPACK_SRC="$TMP/case4b"
            log()  { printf 'log: %s\n' "$*"; }
            die()  { printf 'die: %s\n' "$*" >&2; exit 1; }
            eval "$HDPACK_BLOCK" ) 2>&1 )"
if [ -f "$TMP/case4b_stage/VandalHearts-v2.0.0-hdpack.zip" ]; then
    pass "legacy flat layout still produces VandalHearts-v2.0.0-hdpack.zip"
else
    fail "legacy flat layout did not produce the expected zip: $OUT4B"
fi
printf '%s' "$OUT4B" | grep -qi "deprecated" \
    && pass "legacy flat layout logs a deprecation warning" \
    || fail "no deprecation warning logged for the legacy flat layout"

echo "== case 5: smoke_boot.sh against a non-game binary fails =="
if "$SMOKE" /bin/true >"$TMP/smoke_true.log" 2>&1; then
    fail "smoke_boot.sh /bin/true unexpectedly passed"
else
    rc=$?
    [ "$rc" -ne 0 ] && pass "smoke_boot.sh /bin/true fails (exit $rc)" || fail "smoke_boot.sh /bin/true exit 0"
fi

echo "== case 6: smoke_boot.sh against the real built exe passes =="
REAL_EXE="$PC_DIR/build/vandalhearts_pc"
if [ -x "$REAL_EXE" ]; then
    if "$SMOKE" "$REAL_EXE" >"$TMP/smoke_real.log" 2>&1; then
        pass "smoke_boot.sh passes against $REAL_EXE"
    else
        fail "smoke_boot.sh failed against the real exe: $(tail -5 "$TMP/smoke_real.log")"
    fi
else
    fail "no built exe at $REAL_EXE (build first: make link OPT=1)"
fi

echo
if [ "$FAIL" -eq 0 ]; then
    echo "release_script_guard: ALL PASS"
else
    echo "release_script_guard: FAILURES ABOVE" >&2
fi
exit "$FAIL"
