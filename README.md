# Vandal Hearts — Native PC Port

A native PC port of the US PlayStation 1 release of **Vandal Hearts** (`SLUS_004.47`), built on a
byte-exact matching decompilation of the game.

> **Non-commercial fan preservation project. Not affiliated with Konami or Sony. You must supply
> your own legally-owned copy of the game.** See [DISCLAIMER](DISCLAIMER) and *Legal* below.

The project has three stages, **all completed**:

1. **Matching decompilation.** Every non-PsyQ function is decompiled to C that rebuilds the original
   `SLUS_004.47` byte-for-byte (`md5 596bb082a2de5f1fe977dd3d7e160b03`, verified by `make check`).
   This is the foundation the port is built on, and it is still enforced on every change.
2. **De-consolization → native PC port.** Each PlayStation hardware interface — GPU packet
   submission, GTE matrix math, CD-ROM / XA audio, SPU, MDEC video, pad input — is replaced with a
   portable equivalent (SDL2 + Metal/OpenGL + OpenAL), so the game boots and runs on a modern desktop from
   its own data.
3. **New features.** A PC-only layer of additions on top of the faithful port: reworked controls with
   an at-a-glance enemy threat overlay, an in-game options and save-management overlay, an opt-in
   **Tactical Mode** rebalance that offers a fresh way to play, a higher-fidelity graphics track (an
   accurate software rasterizer with optional internal-resolution supersampling), and an optional
   **HD pack** for backgrounds and movies. The faithful retail experience stays the default; anything
   that changes gameplay is opt-in. See [docs/gameplay-additions.md](docs/gameplay-additions.md),
   [docs/tactical-mode.md](docs/tactical-mode.md) and [docs/hd-pack.md](docs/hd-pack.md).

## Status

The port **runs the full game** end-to-end from the original data — intro FMVs, cutscenes, tactical
battles, world map, party management, dialogue, shops, and save/load — validated by full
playthroughs on both Windows and Linux, including the endgame and credits.

- **A/V fidelity: complete**, validated against real hardware (BizHawk): GTE/perspective and
  terrain/sprite rendering; a sample-accurate software SPU driving the SEQ music and VAG sound
  effects; CD-XA streamed audio; MDEC/STR video; and PS1 Shift-JIS/kanji text.
- **Graphics fidelity (v1.5):** the software renderer is a PSX-accurate integer rasterizer (exact GPU
  coverage + texture sampling, dithering, 5-bit blend; ~99.8–99.99% pixel-exact vs a reference-emulator
  capture), with optional **internal-resolution supersampling** (1–4×) on a multithreaded high-res pass
  for a sharper image with no re-authored art. The faithful look is the default.
- **Optional HD pack (v1.6):** an opt-in layer that replaces the pre-rendered backgrounds and the FMV
  movies with higher-resolution art (`.webp` + HEVC), auto-detected beside the executable and toggled by
  the **HD PACK** option. The source tree and base build ship no art — a pack is either built offline from
  your own disc or downloaded as an optional 1.6 release asset; the base build is unchanged without one.
  See [docs/hd-pack.md](docs/hd-pack.md).
- **64-bit** is the default build. The port is memory-safe — it runs unprivileged (no root, no
  `setcap`) and has passed both an AddressSanitizer out-of-bounds sweep and a UBSan pass across the
  game, which together fixed seven real out-of-bounds bugs latent in the retail game.
- **Platforms: Windows and Linux** (fully supported, full-playthrough validated), plus a
  **community-supported, experimental macOS build**, all from a single source tree. The Windows
  `.exe` is cross-compiled from Linux with MinGW-w64; Linux ships as an AppImage. The native macOS
  CMake build (a community contribution — the maintainer has no Apple hardware) has been tested on
  Apple Silicon through the first battle and its surrounding cutscenes, world map, towns, shops,
  saves, Tactical Mode, fast-forward and HD pack; a dependency-minimal Universal 2 build has passed
  boot-to-title in both native arm64 and x86_64/Rosetta modes, and source-only local `.app` packaging
  is available. Full-playthrough and notarisation validation remain to be done. See
  [docs/cross-platform.md](docs/cross-platform.md).

## Where to start

| You want to… | Go to |
|---|---|
| **Play the game** | Grab the [latest release](https://github.com/HalmyLyseas/VandalHearts-PcPort/releases/latest) — the **Player Manual** (PDF) ships with it and covers setup, controls, and every feature ([source](docs/manual/manual.md)). Quick start below. |
| **Something's wrong** | [Troubleshooting](docs/troubleshooting.md) · [Known issues](docs/known_issues.md) · then open an issue. |
| **Translate the game** | [Language packs](docs/language-packs.md) — what they cover and how they install — then the authoring toolchain: [hands-on quickstart](platform/pc/tools/langpack/quickstart.md) · [full reference](platform/pc/tools/langpack/README.md). |
| **Contribute / fix a bug** | [Architecture](docs/architecture.md) first — the two-layer design and the one unbreakable rule — then [CONTRIBUTING.md](CONTRIBUTING.md). |
| **Understand how it works** | The [documentation index](docs/README.md): internals, subsystem deep-dives, decoded game mechanics. |

## Quick start

You supply the game; the port supplies everything else. **You need your own legally-owned copy of
Vandal Hearts (USA), dumped as a raw `.bin` disc image** — nothing game-derived is distributed here.

| Platform | Package | Requirements |
|---|---|---|
| **Windows** | `.zip` — `vandalhearts_pc.exe`, 8 runtime DLLs, `vandalhearts.ini` | Windows 10/11 |
| **Linux** | `VandalHearts-x86_64.AppImage` + `vandalhearts.ini` | glibc ≥ 2.34 (Debian 12+, Ubuntu 22.04+, Fedora 35+, RHEL 9, Arch); FUSE2 |
| **macOS** *(community-supported, experimental)* | Build from source; local `.app` recipe | macOS 11+; Universal 2 base build or Apple Silicon HD build |

Put your disc image in a `game/` folder next to the executable and launch — the disc is
auto-detected and verified, settings live in the in-game overlay (**SELECT + START**) and
`vandalhearts.ini`, and saves are plain files in `saves/`. Details: the Player Manual,
[configuration.md](docs/configuration.md), and [`platform/pc/OPTIONS.md`](platform/pc/OPTIONS.md).

## Building from source

> **No game executable, disc image, Sony BIOS/SDK data, or in-game text is distributed here.** They
> are git-ignored and must come from your own legally-owned copy; the build reconstructs what it
> needs locally. (The repo does contain the decompiled game code and a few functional game-data
> tables — see *Legal* and [NOTICE](NOTICE).)

```sh
cd platform/pc && make link       # the native port  -> build/vandalhearts_pc   (needs SDL2/OpenAL/GL/libwebp/libav, Python 3)
make check                        # (repo root) byte-exact decomp verification -> MD5 must match
```

The full picture — CMake, the Windows cross-compile, the AppImage container, sanitizers, and the
matching-decomp toolchain — is in [building.md](docs/building.md) and
[cross-platform.md](docs/cross-platform.md). **Any change under `src/` or `include/` must keep
`make check` passing** ([architecture.md](docs/architecture.md) has the gating rules).

## Documentation

Full developer and user documentation is in **[`docs/`](docs/)**. Contributions and bug reports are
welcome — see **[CONTRIBUTING.md](CONTRIBUTING.md)** for the ground rules (above all: the matching
build must stay byte-exact).

- [Known issues](docs/known_issues.md) — current defects and limitations we're already aware of
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

**The repository root *is* the matching-decompilation layer**, kept in the standard
[splat](https://github.com/ethteck/splat)-decomp shape — `Makefile` (`make check`),
`SLUS_004.47.yaml`, and the symbol map at top level, exactly where anyone from the decomp scene
expects them (and where the upstream decompilation keeps them). The port lives entirely under
`platform/pc/`.

- `src/`, `include/` — the matching decompilation (C source and project headers).
- `SLUS_004.47.yaml`, `undefined_additional.txt` — splat configuration and linker symbol input.
- `symbol_addrs.txt` — the address → symbol map. **Shared ground truth, not decomp-only**: the
  port's data-segment generator reads it on every port build.
- `platform/pc/` — the native PC port: PSX subsystem backends (`src/lib*.c`, `pc_*.c`), the build
  system (Makefile, CMake, `cmake/` toolchain files), data-generation and sanitizer tooling
  (`tools/`, `run_*san.sh`), release packaging (`packaging/`), and clean-room PsyQ headers.
- `docs/` — developer & user documentation.

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
