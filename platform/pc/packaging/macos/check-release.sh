#!/bin/bash
# Verify that a locally built macOS artifact, source tag, and pushed commit all describe one source.
# This closes the edge case where an artifact is built locally but a release tool creates its tag from
# a different remote/default-branch commit. It does not upload or publish anything.
set -euo pipefail

usage() {
    echo "usage: $0 <tag> <vandalhearts_pc> <VH_BUILDINFO.txt> [Vandal Hearts.app]" >&2
    exit 2
}

[[ $# -ge 3 && $# -le 4 ]] || usage
tag=$1
binary=$2
buildinfo=$3
app=${4:-}
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
repo="$(cd "$script_dir/../../../.." && pwd)"

# Git forbids slashes in this policy even though Git itself permits them in tag names: the same tag may
# later become a staging-directory component. Tag annotation messages are irrelevant and unrestricted.
if [[ ! "$tag" =~ ^v?[0-9]+([.][0-9]+){1,3}([._-][0-9A-Za-z]+)*$ ]]; then
    echo "error: unsafe/non-version tag '$tag' (expected e.g. v1.7.0 or v1.7.0-rc.1)" >&2
    exit 1
fi
[[ -x "$binary" ]] || { echo "error: binary not executable: $binary" >&2; exit 1; }
[[ -f "$buildinfo" ]] || { echo "error: build manifest missing: $buildinfo" >&2; exit 1; }

head_commit=$(git -C "$repo" rev-parse HEAD)
tag_commit=$(git -C "$repo" rev-parse "refs/tags/$tag^{commit}" 2>/dev/null) || {
    echo "error: local tag does not exist: $tag" >&2; exit 1;
}
[[ "$tag_commit" == "$head_commit" ]] || {
    echo "error: tag $tag points to $tag_commit, but this checkout is $head_commit" >&2; exit 1;
}
[[ -z "$(git -C "$repo" status --porcelain --untracked-files=all)" ]] || {
    echo "error: tracked worktree changes make artifact provenance ambiguous" >&2; exit 1;
}

upstream_ref=$(git -C "$repo" rev-parse --abbrev-ref --symbolic-full-name '@{upstream}' 2>/dev/null) || {
    echo "error: current branch has no upstream tracking ref" >&2; exit 1;
}
upstream_commit=$(git -C "$repo" rev-parse "$upstream_ref^{commit}")
[[ "$upstream_commit" == "$head_commit" ]] || {
    echo "error: local HEAD is not the pushed $upstream_ref commit" >&2; exit 1;
}
remote_name=${upstream_ref%%/*}
remote_tag=$(git -C "$repo" ls-remote --tags "$remote_name" "refs/tags/$tag^{}" "refs/tags/$tag" |
    awk 'NR == 1 { value=$1 } /\^\{\}$/ { value=$1 } END { print value }')
[[ "$remote_tag" == "$head_commit" ]] || {
    echo "error: pushed tag $tag is missing or resolves to '${remote_tag:-nothing}', not $head_commit" >&2
    exit 1
}

manifest_commit=$(sed -n 's/^source_commit=//p' "$buildinfo")
manifest_dirty=$(sed -n 's/^source_dirty=//p' "$buildinfo")
manifest_binary=$(sed -n 's/^binary_sha256=//p' "$buildinfo")
actual_binary=$(shasum -a 256 "$binary" | awk '{print $1}')
[[ "$manifest_commit" == "$tag_commit" ]] || { echo "error: artifact was built from $manifest_commit, not $tag_commit" >&2; exit 1; }
[[ "$manifest_dirty" == 0 ]] || { echo "error: artifact was built from a dirty source tree" >&2; exit 1; }
[[ "$manifest_binary" == "$actual_binary" ]] || { echo "error: binary differs from its build manifest" >&2; exit 1; }

"$repo/platform/pc/tools/check_build_parity.sh"
bash -n "$script_dir"/*.sh

if [[ -n "$app" ]]; then
    [[ -d "$app/Contents" ]] || { echo "error: app not found: $app" >&2; exit 1; }
    if find "$app/Contents" -type f \( \
        -iname '*.bin' -o -iname '*.cue' -o -iname '*.chd' -o -iname '*.iso' -o \
        -iname '*.img' -o -iname '*.pbp' -o -iname '*.mcd' -o -iname '*.sav' -o \
        -iname '*.hdi' -o -iname '*.webp' -o -iname '*.mp4' -o -iname '*.hevc' -o \
        -iname '*.str' -o -iname 'SLUS_*' -o -iname 'SCPH*' -o -iname 'KROMDAT*' \) \
        -print -quit | grep -q .; then
        echo "error: app contains game, BIOS, save, or HD-pack data" >&2
        exit 1
    fi
    codesign --verify --deep --strict --verbose=2 "$app"
    (cd "$app/Contents" && shasum -a 256 -c Resources/SHA256SUMS.txt)
fi

echo "macOS provenance check PASS: artifact, tag, local HEAD, and pushed refs all use $head_commit"
