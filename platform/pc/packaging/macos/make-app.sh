#!/bin/bash
# Create and sign a LOCAL macOS app from a user-built executable. This script intentionally has no
# game/BIOS/HD-pack input and refuses suspicious asset files before signing.
set -euo pipefail

usage() {
    echo "usage: $0 [--identity <codesign identity>] <vandalhearts_pc> <SDL2.framework> [output-dir]" >&2
    exit 2
}

identity="-"
if [[ "${1:-}" == "--identity" ]]; then
    [[ $# -ge 4 ]] || usage
    identity="$2"
    shift 2
fi
[[ $# -ge 2 && $# -le 3 ]] || usage

binary="$(cd "$(dirname "$1")" && pwd)/$(basename "$1")"
sdl_framework="$(cd "$(dirname "$2")" && pwd)/$(basename "$2")"
script_dir="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
pc_dir="$(cd "$script_dir/../.." && pwd)"
output_dir="${3:-$pc_dir/dist/macos}"
app="$output_dir/Vandal Hearts.app"

[[ -x "$binary" ]] || { echo "error: executable not found: $binary" >&2; exit 2; }
[[ -f "$sdl_framework/SDL2" ]] || { echo "error: SDL2.framework is incomplete: $sdl_framework" >&2; exit 2; }
if ! otool -L "$binary" | grep -q '@rpath/SDL2.framework/'; then
    echo "error: executable was not linked against the relocatable SDL2.framework" >&2
    exit 2
fi
if [[ -e "$app" ]]; then
    echo "error: output already exists; move it aside first: $app" >&2
    exit 2
fi

mkdir -p "$app/Contents/MacOS" "$app/Contents/Resources" "$app/Contents/Frameworks"
cp "$binary" "$app/Contents/MacOS/vandalhearts_pc"
cp "$script_dir/launch.sh" "$app/Contents/MacOS/Vandal Hearts Launcher"
cp "$script_dir/Info.plist" "$app/Contents/Info.plist"
cp "$pc_dir/vandalhearts.ini" "$app/Contents/Resources/vandalhearts.ini"
ditto "$sdl_framework" "$app/Contents/Frameworks/SDL2.framework"
chmod 755 "$app/Contents/MacOS/Vandal Hearts Launcher" "$app/Contents/MacOS/vandalhearts_pc"

# Add only the app-local framework search path. The public script never copies Homebrew libraries.
if ! otool -l "$app/Contents/MacOS/vandalhearts_pc" | grep -A2 LC_RPATH | grep -q '@executable_path/../Frameworks'; then
    install_name_tool -add_rpath '@executable_path/../Frameworks' "$app/Contents/MacOS/vandalhearts_pc"
fi

# This list is deliberately conservative. If support for another external asset type is added,
# teach the launcher where to find it outside Contents; do not weaken this bundle guard.
if find "$app/Contents" -type f \( \
    -iname '*.bin' -o -iname '*.cue' -o -iname '*.chd' -o -iname '*.iso' -o \
    -iname '*.img' -o -iname '*.pbp' -o -iname '*.mcd' -o -iname '*.sav' -o \
    -iname '*.hdi' -o -iname '*.webp' -o -iname '*.mp4' -o -iname '*.hevc' -o \
    -iname '*.str' -o -iname 'SLUS_*' -o -iname 'SCPH*' -o -iname 'KROMDAT*' \) \
    -print -quit | grep -q .; then
    echo "error: refusing to sign an app containing game, BIOS, save, or HD-pack data" >&2
    exit 1
fi

sign_args=(--force --sign "$identity")
if [[ "$identity" == "-" ]]; then
    sign_args+=(--timestamp=none)
else
    sign_args+=(--options runtime --timestamp)
fi
codesign "${sign_args[@]}" "$app/Contents/Frameworks/SDL2.framework"
codesign "${sign_args[@]}" "$app/Contents/MacOS/vandalhearts_pc"

(
    cd "$app/Contents"
    shasum -a 256 \
        'MacOS/Vandal Hearts Launcher' MacOS/vandalhearts_pc \
        Frameworks/SDL2.framework/SDL2 Info.plist Resources/vandalhearts.ini \
        > Resources/SHA256SUMS.txt
)
codesign "${sign_args[@]}" "$app"
codesign --verify --deep --strict --verbose=2 "$app"

echo "Created local app: $app"
echo "Game data stays external: drag a .bin onto the app, place it beside the app,"
echo "or put it in ~/Library/Application Support/Vandal Hearts/game/."
