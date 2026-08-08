# Local macOS app packaging

This recipe builds a local `.app`; it is not a binary release recipe. The PC executable contains
data generated from the user's copy of the game and BIOS/PsyQ font input, so neither the executable,
the resulting app, nor those inputs may be committed, attached to a public release, or uploaded by
automation. Only these packaging sources and dependency/input hashes belong in the public repo.

## Universal 2 build

Use the official SDL 2 framework. SDL2 2.32.10's DMG has SHA-256
`4a7ac31640d70214e848f994be8a12849c0f97918a7e6c2e27a40036166d1a7f`; its framework contains both
`arm64` and `x86_64`. Mount it, then configure a dependency-minimal Universal 2 build:

```sh
cmake -S platform/pc -B platform/pc/build-macos-universal \
  -DCMAKE_BUILD_TYPE=Release \
  '-DCMAKE_OSX_ARCHITECTURES=arm64;x86_64' \
  -DSDL2_DIR=/Volumes/SDL2/SDL2.framework/Resources/CMake \
  -DCMAKE_FRAMEWORK_PATH=/Volumes/SDL2 \
  -DVH_WEBP=OFF -DVH_HDVIDEO=OFF \
  -DVH_PSX_EXE=/path/to/your/SLUS_004.47 \
  -DVH_KROM_SOURCE=/path/to/your/PS1-BIOS-or-KROMDAT.BIN
cmake --build platform/pc/build-macos-universal -j
lipo -info platform/pc/build-macos-universal/vandalhearts_pc
```

The HD options can be enabled only when WebP and all FFmpeg libraries are also available for every
requested architecture. HD pack files remain external in all cases.

## Create and sign the local app

```sh
platform/pc/packaging/macos/make-app.sh \
  platform/pc/build-macos-universal/vandalhearts_pc \
  /Volumes/SDL2/SDL2.framework
```

The default is an ad-hoc signature. For a local Developer ID identity, add
`--identity 'Developer ID Application: Name (TEAMID)'`. CMake writes `VH_BUILDINFO.txt` beside the
executable with the exact source commit, input hashes, compiler, options, architectures, and binary
hash; the app recipe embeds that manifest. The script does not notarize or upload.
It refuses to overwrite an existing app and scans the bundle for common game, BIOS, HD-pack, and
save formats before signing. Output goes under the already-ignored `platform/pc/dist/` directory.

At runtime, drag a raw `.bin` onto the app, place it in a `game` directory beside the app, or put it
in `~/Library/Application Support/Vandal Hearts/game/`. Configuration and saves also live in that
Application Support directory, outside the signed bundle.

For a tagged local validation build, pass `--tag vX.Y.Z`. This runs `check-release.sh`, which rejects
unsafe tag names and proves that the tag, local/pushed branch, build manifest, binary, and optional app
all refer to the same clean source commit. This addresses the otherwise easy-to-miss edge case where a
release asset is built from one checkout while a release tool creates its tag from another commit.
