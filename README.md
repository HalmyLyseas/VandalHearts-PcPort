# Vandal Hearts — Native PC Port

A native PC port of the US PlayStation 1 release of **Vandal Hearts** (`SLUS_004.47`), built on a
byte-exact matching decompilation of the game.

> **Non-commercial fan preservation project. Not affiliated with Konami or Sony. You must supply
> your own legally-owned copy of the game.** See [DISCLAIMER](DISCLAIMER) and *Legal* below.

The project has two stages, **both complete**:

1. **Matching decompilation.** Every non-PsyQ function is decompiled to C that rebuilds the original
   `SLUS_004.47` byte-for-byte (`md5 596bb082a2de5f1fe977dd3d7e160b03`, verified by `make check`).
   This is the foundation the port is built on, and it is still enforced on every change.
2. **De-consolization → native PC port.** Each PlayStation hardware interface — GPU packet
   submission, GTE matrix math, CD-ROM / XA audio, SPU, MDEC video, pad input — is replaced with a
   portable equivalent (SDL2 + OpenGL + OpenAL), so the game boots and runs on a modern desktop from
   its own data.

## Status

The port **runs the full game** end-to-end from the original data — intro FMVs, cutscenes, tactical
battles, world map, party management, dialogue, shops, and save/load — validated by full
playthroughs on both Windows and Linux, including the endgame and credits.

- **A/V fidelity: complete**, validated against real hardware (BizHawk): GTE/perspective and
  terrain/sprite rendering; a sample-accurate software SPU driving the SEQ music and VAG sound
  effects; CD-XA streamed audio; MDEC/STR video; and PS1 Shift-JIS/kanji text.
- **64-bit** is the default build. The port is memory-safe — it runs unprivileged (no root, no
  `setcap`) and has passed both an AddressSanitizer out-of-bounds sweep and a UBSan pass across the
  game, which together fixed seven real out-of-bounds bugs latent in the retail game.
- **Platforms: Windows and Linux**, from a single source tree. The Windows `.exe` is cross-compiled
  from Linux with MinGW-w64; Linux ships as an AppImage. A CMake build sits alongside the Makefile.
  macOS is scaffolded but not pursued — see [docs/cross-platform.md](docs/cross-platform.md).

## Playing the game

You supply the game; the port supplies everything else. **You need your own legally-owned copy of
Vandal Hearts (USA), dumped as a raw `.bin` disc image** — nothing game-derived is distributed here.

A release is self-contained and needs no dependency hunting:

| Platform | Package | Requirements |
|---|---|---|
| **Windows** | `.zip` — `vandalhearts_pc.exe`, 6 runtime DLLs, `vandalhearts.ini` | Windows 10/11 |
| **Linux** | `VandalHearts-x86_64.AppImage` + `vandalhearts.ini` | glibc ≥ 2.34 (Debian 12+, Ubuntu 22.04+, Fedora 35+, RHEL 9, Arch); FUSE2 to run the AppImage |

**Setup is drop-in:** put your disc image in a `game/` folder next to the executable (or a bare
`*.bin` beside it) and launch. No configuration, no environment variables — the disc is
auto-detected, and its boot signature is verified, so mounting the wrong disc fails with a clear
message instead of booting into a blank window.

**To configure**, edit `vandalhearts.ini` next to the executable (on Linux, next to the
`.AppImage`): window scale, audio, and compatibility options, all commented out at their defaults.
The same file and keys work on every platform; environment variables still override it. Full option
reference in [`platform/pc/OPTIONS.md`](platform/pc/OPTIONS.md) and
[docs/configuration.md](docs/configuration.md).

Saves are ordinary files in a `saves/` folder next to the executable. They use a fixed on-disk
layout, so they are architecture-agnostic and cross-loadable between the 32- and 64-bit builds.

## Building from source

> **No game executable, disc image, Sony BIOS/SDK data, or in-game text is distributed here.** They
> are git-ignored and must come from your own legally-owned copy; the build reconstructs what it
> needs locally. (The repo does contain the decompiled game code and a few functional game-data
> tables — see *Legal* and [NOTICE](NOTICE).)

Requirements: a Linux host with SDL2, OpenAL, and OpenGL development libraries; Python 3; and, for
the matching-decomp verification, the PSY-Q toolchain. A 32-bit toolchain is optional (only for the
`-m32` reference build). The full environment recipe lives in `.claude/skills/decomp-build/`.

Two interchangeable build systems for the native port (both produce the same binary):

```sh
cd platform/pc

# Makefile
make link                       # -> platform/pc/build/vandalhearts_pc  (64-bit)
make link M32=-m32 BUILD_DIR=build32   # 32-bit reference build

# CMake
cmake -S . -B build_cmake && cmake --build build_cmake   # -> build_cmake/vandalhearts_pc
```

Windows is **cross-compiled from Linux** (no Windows machine needed) via a CMake toolchain file:

```sh
cmake -S . -B build_win -DCMAKE_TOOLCHAIN_FILE=cmake/toolchain-mingw-w64.cmake
cmake --build build_win         # -> build_win/vandalhearts_pc.exe + bundled DLLs
```

Linux releases are packaged as an AppImage, built inside a pinned Debian 12 container so the
artifact's glibc floor stays low — see [docs/cross-platform.md](docs/cross-platform.md) for the
full recipe:

```sh
packaging/appimage/build-appimage.sh build_deb/vandalhearts_pc   # -> dist/VandalHearts-x86_64.AppImage
```

The built binary finds your disc and `vandalhearts.ini` the same way a release does (see *Playing
the game*); `VH_DISC_IMAGE` still overrides. Runtime options and build flags are in
[`platform/pc/OPTIONS.md`](platform/pc/OPTIONS.md).

Byte-exact decomp check: `make check` rebuilds `SLUS_004.47` and md5-compares it to the original.
**Any change under `src/` or `include/` must keep this passing** — see
[docs/architecture.md](docs/architecture.md) for the gating conventions (`PERMUTER`, `PC_PORT`,
`PC_DEBUG_*`) that keep port work out of the matching build.

## Documentation

Full developer and user documentation is in **[`docs/`](docs/)**:

- [Architecture](docs/architecture.md) — the two-layer design and how the port avoids breaking the match
- [Building](docs/building.md) · [Configuration & running](docs/configuration.md)
- [PC-port internals](docs/pc-port/overview.md) and per-subsystem deep-dives
  ([GPU](docs/pc-port/subsystems/gpu.md), [GTE](docs/pc-port/subsystems/gte.md),
  [SPU](docs/pc-port/subsystems/spu.md), [CD/XA](docs/pc-port/subsystems/cd-xa.md),
  [MDEC](docs/pc-port/subsystems/mdec.md), [Kernel](docs/pc-port/subsystems/kernel.md))
- [Memory safety & the 64-bit port](docs/memory-safety.md) ·
  [Cross-platform & packaging](docs/cross-platform.md) — Windows cross-compile, Linux AppImage releases
- [Width bugs](docs/width-bugs.md) — the 32→64-bit bug class that **neither ASan nor UBSan can
  detect** (truncated copies, union aliasing, struct-layout drift), with a detector table. Read this
  before touching struct layouts or `Object` unions.

## Repository layout

- `src/`, `include/` — the matching decompilation (C source and project headers).
- `platform/pc/` — the native PC port: PSX subsystem backends (`src/lib*.c`, `pc_*.c`), the build
  system (Makefile, CMake, `cmake/` toolchain files), data-generation and sanitizer tooling
  (`tools/`, `run_*san.sh`), release packaging (`packaging/`), and clean-room PsyQ headers.
- `docs/` — developer & user documentation.
- `SLUS_004.47.yaml`, `symbol_addrs.txt` — the splat configuration and the address → symbol map.

Some data files are **generated at build time** from your own copy and are not committed (e.g. the
PS1 kanji font from `KROMDAT.BIN`, and the in-game description strings) — the same model as the
data segment. See [NOTICE](NOTICE).

## Legal

This is a non-commercial fan preservation and interoperability project, not affiliated with or
endorsed by Konami or Sony, not sold, and generating no profit.

- **Vandal Hearts and all related IP are © Konami.** The PlayStation, its BIOS, and the PsyQ SDK are
  © Sony. No ownership of either is claimed.
- **Not in this source repository:** the game executable, a disc image, Sony BIOS/SDK data
  (including the PS1 kanji font), Konami's in-game text, or any extracted assets. Supply your own
  game copy; the build reconstructs what it needs locally.
- **Present in the repo:** the reverse-engineered decompilation of the game code (`src/`), and a
  small number of functional game-data tables (unit stats, animation/sprite pointer tables)
  reconstructed from the binary — included for interoperability and preservation, © Konami, not
  claimed as this project's own.
- **Pre-built release binaries** *do* embed a portion of game-derived data (the executable's static
  data segment, a small BIOS-derived kanji font, and the reconstructed text/data tables), because a
  compiled binary bakes in what the source tree regenerates locally. This is only a fraction of the
  game — dialogue, maps, audio, and video load at runtime from the disc you supply, so a release
  binary does nothing without your own legally-owned copy. Full breakdown in [NOTICE](NOTICE).

Full detail in [DISCLAIMER](DISCLAIMER).

## License

The original PC-port code and tooling written for this project (principally `platform/pc/`) are
licensed **GPL-2.0** (see [LICENSE](LICENSE)). This does **not** extend to the decompiled game code
or the game-derived data, which are Konami's and are not the project's to license — see
[NOTICE](NOTICE) for exactly what the GPL-2.0 grant covers and what it does not.

## Credits

Built on the Vandal Hearts matching decompilation by **shao113**
(<https://github.com/shao113/vh>). Full acknowledgements — the decomp toolchain (splat, maspsx,
old-gcc), the psx-spx hardware reference, and BizHawk — are in [CREDITS](CREDITS).

## External files (supply from your own copy)

Not included in this repository:

| file | md5 |
|---|---|
| `SLUS_004.47` (base executable) | `596bb082a2de5f1fe977dd3d7e160b03` |
| `ASPSX.EXE` (ASPSX v2.21, PSYQ v3.3 / DTL-S2190) | `e3ae8aea2623b916f89384bf70f55487` |
| `LIB34.ZIP` | `d25fd757e944a05369e8fb003a007dd2` |
