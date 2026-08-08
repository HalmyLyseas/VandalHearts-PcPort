#!/bin/bash
# Finder-facing launcher. Game data, configuration, HD packs, and saves remain outside the signed
# app so the public package recipe never redistributes or modifies copyrighted inputs.
set -euo pipefail

macos_dir="$(cd "$(dirname "$0")" && pwd)"
contents_dir="$(cd "$macos_dir/.." && pwd)"
app_dir="$(cd "$contents_dir/.." && pwd)"
distribution_dir="$(cd "$app_dir/.." && pwd)"
support_dir="${HOME}/Library/Application Support/Vandal Hearts"

mkdir -p "$support_dir/game" "$support_dir/saves"
if [[ ! -f "$support_dir/vandalhearts.ini" ]]; then
    cp "$contents_dir/Resources/vandalhearts.ini" "$support_dir/vandalhearts.ini"
fi

export VH_DEPLOY_DIR="$support_dir"

# A .bin dropped onto the app (or opened with it) wins. Ignore LaunchServices' own -psn argument.
for argument in "$@"; do
    case "$argument" in
        *.bin|*.BIN)
            if [[ -f "$argument" ]]; then
                export VH_DISC_IMAGE="$argument"
                break
            fi
            ;;
    esac
done

# Otherwise discover a user-supplied raw disc image. Nothing is copied into the app bundle.
if [[ -z "${VH_DISC_IMAGE:-}" ]]; then
    for search_dir in "$support_dir/game" "$distribution_dir/game" "$distribution_dir"; do
        for candidate in "$search_dir"/*.bin; do
            if [[ -f "$candidate" ]]; then
                export VH_DISC_IMAGE="$candidate"
                break 2
            fi
        done
    done
fi

exec "$macos_dir/vandalhearts_pc"
