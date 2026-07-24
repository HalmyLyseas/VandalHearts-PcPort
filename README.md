# Vandal Hearts — Native PC Port

A native PC port of the US PlayStation 1 release of **Vandal Hearts** (`SLUS_004.47`), built on a
byte-exact matching decompilation of the game.

> **Non-commercial fan preservation project. Not affiliated with Konami or Sony. You must supply
> your own legally-owned copy of the game.** See [DISCLAIMER](DISCLAIMER) and *Legal* below.

The project has two stages:

1. **Matching decompilation (complete).** Every non-PsyQ function is decompiled to C that rebuilds
   the original `SLUS_004.47` byte-for-byte (`md5 596bb082a2de5f1fe977dd3d7e160b03`, verified by
   `make check`). This is the foundation the port is built on.
2. **De-consolization → native PC port (in progress).** Each PlayStation hardware interface — GPU
   packet submission, GTE matrix math, CD-ROM / XA audio, SPU, MDEC video, pad input — is replaced
   with a portable equivalent (SDL2 + OpenGL + OpenAL), so the game boots and runs on a modern
   desktop from its own data.

## Status

The port **runs the full game** end-to-end from the original data — intro FMVs, cutscenes, tactical
battles, world map, party management, dialogue, shops, and save/load — validated by a full
playthrough.

- **A/V fidelity: complete**, validated against real hardware (BizHawk): GTE/perspective and
  terrain/sprite rendering; a sample-accurate software SPU driving the SEQ music and VAG sound
  effects; CD-XA streamed audio; MDEC/STR video; and PS1 Shift-JIS/kanji text.
- **64-bit** is the default build; the port is memory-safe (runs unprivileged, no root/setcap) and
  has passed an AddressSanitizer out-of-bounds sweep across the game.
- **In progress:** cross-platform support (Windows via MinGW-w64, then macOS/Apple Silicon), with a
  CMake build alongside the Makefile.

## Building & running

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

Point `VH_DISC_IMAGE` at your extracted disc `.bin` (it defaults to a sibling `game/` folder).
Runtime options (window scale, audio, diagnostics) and build flags are in
[`platform/pc/OPTIONS.md`](platform/pc/OPTIONS.md).

Byte-exact decomp check: `make check` rebuilds `SLUS_004.47` and md5-compares it to the original.

## Documentation

Full developer and user documentation is in **[`docs/`](docs/)**:

- [Architecture](docs/architecture.md) — the two-layer design and how the port avoids breaking the match
- [Building](docs/building.md) · [Configuration & running](docs/configuration.md)
- [PC-port internals](docs/pc-port/overview.md) and per-subsystem deep-dives
  ([GPU](docs/pc-port/subsystems/gpu.md), [GTE](docs/pc-port/subsystems/gte.md),
  [SPU](docs/pc-port/subsystems/spu.md), [CD/XA](docs/pc-port/subsystems/cd-xa.md),
  [MDEC](docs/pc-port/subsystems/mdec.md), [Kernel](docs/pc-port/subsystems/kernel.md))
- [Memory safety & the 64-bit port](docs/memory-safety.md) · [Cross-platform (Windows/macOS)](docs/cross-platform.md)

## Repository layout

- `src/`, `include/` — the matching decompilation (C source and project headers).
- `platform/pc/` — the native PC port: PSX subsystem backends (`src/lib*.c`, `pc_*.c`), the build
  system (Makefile, CMake), data-generation tooling (`tools/`), and clean-room PsyQ headers.
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
- **Not distributed:** the game executable, a disc image, Sony BIOS/SDK data (including the PS1
  kanji font), Konami's in-game text, or any extracted assets. Supply your own game copy.
- **Present in the repo:** the reverse-engineered decompilation of the game code (`src/`), and a
  small number of functional game-data tables (unit stats, animation/sprite pointer tables)
  reconstructed from the binary — included for interoperability and preservation, © Konami, not
  claimed as this project's own.

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
