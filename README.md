# Vandal Hearts — Native PC Port

A native PC port of the US PlayStation 1 release of **Vandal Hearts** (`SLUS_004.47`), built on a
byte-exact matching decompilation of the game.

The project has two stages:

1. **Matching decompilation (complete).** Every non-PsyQ function is decompiled to C that rebuilds
   the original `SLUS_004.47` byte-for-byte (`md5 596bb082a2de5f1fe977dd3d7e160b03`, verified by
   `make check`). This is the foundation the port is built on — see *Acknowledgements* below.
2. **De-consolization → native PC port (in progress).** Each PlayStation hardware interface — GPU
   packet submission, GTE fixed-point matrix math, CD-ROM / XA audio, SPU, pad input — is replaced
   with a portable equivalent (SDL2 + OpenGL + OpenAL), so the game boots and runs from its own data
   on a modern desktop.

## Status

The port **runs the full game loop** from the original game data: intro FMVs, scripted cutscenes,
tactical battles, the world map, party management, dialogue, and save/load.

**Audio & video fidelity: complete** — validated against real hardware (BizHawk) at matched poses:

- **Graphics** — GTE / perspective math, terrain and unit-sprite rendering, depth and occlusion.
- **Music** — a sample-accurate software SPU (per-voice pitch, 4-point Gaussian interpolation, ADSR
  envelopes, STUDIO_C reverb, optional analog-output coloration) driving the game's SEQ sequencer
  and VAG sound effects.
- **CD-XA streaming** — FMV audio and streamed spell sound effects, timed to match hardware.
- **Video & text** — MDEC / STR full-motion video, and PS1 BIOS kanji / Shift-JIS glyph rendering.

The current build is intentionally **32-bit** (`-m32`) as a debugging baseline; 64-bit and
cross-platform support are on the roadmap.

## Roadmap (remaining Stage-2 work)

- **Memory-safety / NULL hardening** — retire the "harmless-on-PSX" workarounds (writable-`.rodata`
  remap; the low-page mapping / `CAP_SYS_RAWIO` that a benign NULL-deref currently needs) by fixing
  their root causes, and reconstruct the remaining data-generation-zeroed pointer globals.
- **64-bit** — make the decompiled code pointer-width-agnostic and build as `-m64` (required for
  current macOS).
- **Cross-platform** — Windows and macOS builds.

## Building & running

> **You must supply your own legally-owned copy of the game.** No copyrighted game code or data is
> distributed here — the base executable, disc image, and proprietary Sony SDK headers are all
> git-ignored and must be sourced from your own copy.

Requirements: a Linux host with SDL2, OpenAL, and OpenGL development libraries and a 32-bit
toolchain (`-m32`); plus, for the matching-decomp verification, the PSY-Q 3.3 toolchain. The full
environment recipe (headers, toolchain, base files, exact commands) lives in
`.claude/skills/decomp-build/`.

- **Native PC build:** `cd platform/pc && make link` produces `platform/pc/build/vandalhearts_pc`.
  Point `VH_DISC_IMAGE` at your extracted disc `.bin` (defaults to a sibling `game/` folder).
- **Byte-exact decomp check:** `make check` rebuilds `SLUS_004.47` and md5-compares it to the
  original.

The build runs as a normal unprivileged process — no `setcap`/root needed (a portable fault handler
takes the place of the old privileged NULL-page mapping). Runtime options (window scale, audio,
diagnostics) and build flags are documented in [`platform/pc/OPTIONS.md`](platform/pc/OPTIONS.md).

## Repository layout

- `src/`, `include/` — the matching decompilation (C source and project headers).
- `platform/pc/` — the native PC port: PSX subsystem backends (`src/lib*.c`, `pc_*.c`), the
  data-segment generator (`tools/`), SDL2/OpenGL windowing, audio, and backend unit tests.
- `SLUS_004.47.yaml`, `symbol_addrs.txt` — the splat configuration and the authoritative
  address → symbol map.

## External Files

Not included in this repository — supply from your own copy:

SLUS_004.47
`596bb082a2de5f1fe977dd3d7e160b03`

ASPSX.EXE (ASPSX v2.21 from PSYQ v3.3 / DTL-S2190)
`e3ae8aea2623b916f89384bf70f55487`

LIB34.ZIP
`d25fd757e944a05369e8fb003a007dd2`

## Acknowledgements

Built on the Vandal Hearts matching decompilation by **shao113**
(https://github.com/shao113/vh) — the byte-exact C foundation this port stands on.

Bits and pieces of various decompilation projects were used to get the decompilation started,
including assembler macros, header files, makefiles, splat configurations, and general references:

https://github.com/mkst/esa

https://github.com/mkst/ctr

https://github.com/Drahsid/ffvii

https://github.com/pmret/papermario

## Tools

https://github.com/ethteck/splat

https://github.com/simonlindholm/asm-differ

https://github.com/lab313ru/ghidra_psx_ldr

https://github.com/NationalSecurityAgency/ghidra

https://github.com/grumpycoders/pcsx-redux
