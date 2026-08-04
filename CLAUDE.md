# Vandal Hearts Decompilation → Native PC Port — Project Context

## What this is

A matching decompilation of the US PS1 release of Vandal Hearts (`SLUS_004.47`), carried through
to a **working, packaged, cross-platform native PC port**. The decomp uses the standard PSX-decomp
toolchain: [splat](https://github.com/ethteck/splat) for disassembly/extraction,
[maspsx](https://github.com/mkst/maspsx) to post-process GCC 2.x asm for the PSX assembler quirks,
an old GCC 2.x frontend (`cc1_v263`/`cc1_v257`, prebuilt by
[decompals/old-gcc](https://github.com/decompals/old-gcc)) for compilation, and a
`mips-suse-linux-*` binutils cross-toolchain for assembling/linking. The match is verified by
`md5sum`-comparing the rebuilt `SLUS_004.47` against the original.

## Project stages — read this before making architectural calls

The objective was always a **native PC port**, not just a matching rebuild. Two foundational stages
are **complete**; a third (gameplay/QoL) is **underway** (first release shipped).

1. **Stage 1 — matching decomp (✅ done 2026-07-10).** The C source is bit-identical to the
   original: `make check` produces a byte-exact `SLUS_004.47` (`md5 596bb082a2de5f1fe977dd3d7e160b03`,
   confirmed by both `md5sum` and `cmp`). This is the foundation, and it is **still enforced on
   every change** — see *Byte-exact discipline* below.
2. **Stage 2 — de-consolization / native PC port (✅ complete 2026-07-24).** Every PSX hardware
   interface — GPU packet submission (`libgpu`), GTE matrix math (`libgte`), CD-ROM/XA audio
   (`libcd`/`libpress`), SPU, MDEC video, pad input — is replaced with a portable equivalent
   (SDL2 + OpenGL + OpenAL, chosen after evaluating Vulkan). The **full game runs end-to-end** from
   the real disc, validated by full playthroughs on **Windows and Linux** including the endgame and
   credits. Details below and in [`docs/`](docs/).
3. **Stage 3 — gameplay/QoL (v1.1–v1.6 shipped; main additions complete, now maintenance).** PC-only
   quality-of-life, balance and graphics work, all `platform/pc/` or `#ifdef PC_FEAT`-gated. **v1.1.0 released 2026-07-25** (bidirectional
   shoulder-button ally-cycle; enemy threat overlay; a **SELECT+START** in-game options overlay,
   right-stick axis invert — Y inverted by default — plus deploy-relative saves). **v1.2.0 released
   2026-07-25** (video: window scale/fullscreen in the overlay; **save management** — unlimited
   whole-card backups via `saves/.archive/`, restore/delete with a "back up first" safe default, a
   `(*)` active-card marker, and a Start-to-inspect detail view of each backup's 3 slots). The overlay
   is now a small menu system (`platform/pc/src/pc_overlay.c` + `pc_saves.c`; screens MAIN/SAVES/
   CONFIRM/DETAIL). **v1.3.0 + v1.3.1 released** — **Tactical Mode**, a large opt-in rebalance
   (per-chapter level cap, Trials that reward gold + XP, class reworks, a reined-in Vandalier, clarified
   item text), validated across a full playthrough; normal mode stays byte-for-byte retail. Player guide
   [`docs/tactical-mode.md`](docs/tactical-mode.md), [`docs/known_issues.md`](docs/known_issues.md),
   roadmap [`docs/roadmap.md`](docs/roadmap.md). Note: the "zero `src/` edits" ideal was the
   *balance-package* rule — other Stage-3 features need gated `src/` hooks. **v1.4.0 released 2026-07-31**
   — a QoL pass: battle fast-forward (2×), controller-aware overlay labels (Xbox/PlayStation), magic-
   resistance-aware Tactical AI, finer camera elevation (incl. keyboard R/F). **v1.5.0 released** — a
   higher-fidelity graphics track. Three pieces: (1) the **PSX-accurate software rasterizer**
   (`VH_ACCURATE`, default-on) — a fixed-point integer DDA matching the PS1 GPU's exact coverage + texture
   UVs, ordered dithering (gated on the GPU dither bit), 5-bit blend, ~99.8–99.99% pixel-exact vs a
   DuckStation VRAM capture (legacy renderer kept as `VH_ACCURATE=0`, an INI-only softer alternate); (2)
   **internal-resolution supersampling** (`VH_INTERNAL_SCALE` 1–4×, off by default) layered on that DDA —
   sharper 3D with no re-authored assets, with built-in *crust-free* tile sampling (biases the finer hi-res
   sample onto tile interiors like the reference renderer, so no dark tile-seam grid; 2D UI/text auto-kept
   pixel-aligned); (3) a **multithreaded rasterizer** (`VH_RASTER_THREADS`, auto) so 4× holds 30 fps and
   battle fast-forward stays effective. The tile-seam grid, compass "dotted lines" and parked water-shimmer
   were all resolved by crust-free sampling. Guiding principle: *sharpen without reinterpreting* (no
   upscaled video, no redrawn sprites, no camera-feel changes). **v1.6.0 released 2026-08-02** — an
   **optional HD pack** (the base build ships no art and is unchanged without one). Upscaled `.webp`
   backgrounds replace the 320×240 pre-rendered art via a content hash of each `LoadImage` VRAM upload
   (sampled in the 1.5 hi-res pass); the intro/ending FMVs can be swapped for HD **H.264/HEVC** re-encodes
   (a `libav` decoder in `platform/pc/src/pc_hdvideo.c`) while the game keeps its original XA audio + frame
   timing. New deps **libwebp** + **libav** are default-on (`NO_WEBP`/`NO_HDVIDEO` opt-outs); the Windows
   build links a **minimal static libav** so it ships no ffmpeg DLLs. Hand-drawn pixel art (portraits,
   sprites, UI, fonts) deliberately stays native. **The pack is copyright-derived and is a RELEASE ASSET,
   not committed** — an optional download or self-built from your own disc (disclosed in `NOTICE`/
   `DISCLAIMER`). This marks the natural end of the main Stage-3 additions; further work is expected to be
   maintenance — minor adjustments and reported bugs. **v1.6.1 released 2026-08-04** — the first
   maintenance release: a frame-pacing fix so battle fast-forward holds the full 2× at every graphics
   setting, HD backgrounds decoded on a background thread (no scene-load dips), HD-pack **manifest v2**
   (FMVs declared, verified at startup, self-explaining HD PACK row; the v1.6.0 pack download is
   withdrawn — the v2 pack is the only supported one), a **Player Manual (PDF)** built per release
   (`platform/pc/tools/build-manual.sh`), reorganized player docs, and a developer regression harness
   (`platform/pc/tools/regress/`) + release checklist ([`docs/releasing.md`](docs/releasing.md)).
   Post-1.6.1 the two largest backend files were split into single-purpose units for maintainability
   (pure code motion, output verified bit-identical against the shipped release binary): `libgpu.c` →
   + `pc_raster.c`/`pc_hdpack.c`/`pc_gpu_trace.c`, `libetc.c` → + `pc_diag.c`/`pc_battle_speed.c`
   (file maps in [`docs/pc-port/subsystems/gpu.md`](docs/pc-port/subsystems/gpu.md) and
   [`kernel.md`](docs/pc-port/subsystems/kernel.md)). **Active development concluded 2026-08-04**:
   the project is in maintenance — fixes for reported issues, no new feature tracks planned.

**Do not "clean up" or restructure the decompiled `src/`/`include/` toward port concerns.** Stage 1's
job is byte-exact matching, not readability or portability; all port-side changes live behind gates
(below) or under `platform/pc/`. Keep the two concerns separate in commits and discussion.

## Where things live

- **The port** is under [`platform/pc/`](platform/pc/): the six PSX subsystem backends
  (`src/lib*.c`, `pc_*.c`), the SDL2/OpenGL/OpenAL windowing+audio, the data-segment generator
  (`tools/`), both build systems, release packaging (`packaging/`), and clean-room PsyQ headers.
- **Canonical documentation** is committed under [`docs/`](docs/) — architecture, building,
  configuration, per-subsystem deep-dives, memory-safety, cross-platform/packaging, and
  [`docs/width-bugs.md`](docs/width-bugs.md). **Prefer these for durable reference.** The
  [`README.md`](README.md) is the end-user + contributor entry point.
- **On-demand build knowledge** is in the tracked skills [`.claude/skills/decomp-build`](.claude/skills/decomp-build/SKILL.md)
  (full toolchain/environment recipe) and [`.claude/skills/phase-c-pc-port`](.claude/skills/phase-c-pc-port/SKILL.md)
  (port architecture + the local psx-spx hardware reference).
- **`exchange/` is gitignored local scratch** (investigation notes, extracted-asset staging) — it is
  **not part of the repo** and will not exist in a clone. Do not cite it as a durable reference; put
  anything that should survive into `docs/` or a skill.

## Stage 1 status (matching decomp)

- **`make check` is byte-exact** (`596bb082a2de5f1fe977dd3d7e160b03`), reproduced from a from-scratch
  environment. All application code is decompiled and matches: **1184 typed functions across 70
  `src/*.c` files**. PsyQ SDK library functions are intentionally left as raw asm — Sony's SDK isn't
  the decomp's target (standard practice).
- Two small regions of `src/text.c` are byte-exact placeholders rather than proper decompiles
  (`D_800151C8[888]`, `D_80122FB0`..`D_80123090`), both marked `TODO`. Neither is referenced by any
  code yet — **don't delete them as "dead code," they hold specific bytes.**

## Stage 2 status (native PC port) — COMPLETE

- **A/V fidelity complete**, validated against real hardware (BizHawk): GTE/perspective, terrain and
  unit-sprite depth/occlusion; a **sample-accurate software SPU** driving SEQ music + VAG SFX
  (measured **1.33 dB mean** error vs the octoshock reference); CD-XA streamed audio; MDEC/STR video;
  PS1 Shift-JIS/kanji text. *Lesson for future audio work:* every sound bug turned out to be in the
  **PsyQ `libsnd` reimplementation** (missing `ProgAtr.mvol`, pan divisor 63-not-64, the square
  volume law, VAB tone-block packing), a layer neither psx-spx nor octoshock covers — not our SPU
  hardware emulation. Treat "signed off" as "no known issue," not "verified."
- **64-bit is the default build** (`make link`). The port was deliberately 32-bit for a long time as
  a debugging baseline — decompiled source assumes the PS1's 32-bit-pointer layout, and at 64-bit
  that shifts struct fields *silently* rather than failing loudly. All cases are fixed and validated
  at both widths; the 32-bit build remains as an A/B reference
  (`make link M32=-m32 BUILD_DIR=build32`). The **width-bug class** (truncated copies, union
  aliasing, struct-layout drift) is invisible to both ASan and UBSan — found only by building,
  running, and diffing widths. Full catalogue + detector table in
  [`docs/width-bugs.md`](docs/width-bugs.md); **read it before touching struct layouts or the
  `Object` union.**
- **Memory-safe:** runs unprivileged (no root, no `setcap`) via a portable SIGSEGV fault handler in
  `pc_bootstrap.c`. Passed an **AddressSanitizer OOB sweep** and a **UBSan pass** across the game,
  fixing **seven real out-of-bounds bugs latent in the retail game** (e.g. `gWindowDisplayX/Y` was
  corrupting live XA audio state). Tooling: `make asan32` + `run_asan.sh` (ASan must be 32-bit — its
  64-bit shadow collides with the `0x80000000` PSX RAM arena), `make ubsan` (works at 64-bit),
  `tools/struct_width_diff.sh`. *Method lesson (bit us 3×): a mis-sized-array report is not
  automatically a too-small declaration — prove which index is out of range against the byte-exact
  binary before widening.*
- **Cross-platform (Windows + Linux) from one source tree.** The Windows `.exe` is cross-compiled
  from Linux via a MinGW-w64 CMake toolchain and validated end-to-end on real hardware; it ships as
  a self-contained zip (exe + 6 DLLs + `vandalhearts.ini`). Linux ships as an **AppImage** built in a
  pinned Debian 12 container (the build box sets the glibc floor — 2.34, broadly portable). Both have
  drop-in disc auto-detect, a fatal wrong-disc guard, and `vandalhearts.ini` config. **macOS is
  scaffolded but not pursued.** Full recipe in [`docs/cross-platform.md`](docs/cross-platform.md).
- **Repo:** **public** GitHub `HalmyLyseas/VandalHearts-PcPort` (`origin`); `upstream` =
  `github.com/shao113/vh` (the original decomp, reference only). `upstream-master` is the pristine
  byte-exact base. Commit identity is the GitHub noreply address (no PII). Since it's public, keep
  committed files self-contained — reference only tracked paths, never `exchange/` or local notes.
- **Releases:** **v1.0.0 through v1.6.1 published** (each: Windows zip + Linux AppImage; since v1.6 also
  an optional `hdpack.zip` and a Player Manual PDF as release assets). Built and published *locally* by
  `platform/pc/packaging/make-release.sh <tag> [--hdpack=<dir>]` — never CI, because the data-segment
  generator needs the byte-exact `SLUS_004.47` + `KROMDAT.BIN` at build time (copyrighted, can't live on
  runners). Always stage-build BOTH platforms (`--no-publish`) before publishing — it has caught
  Windows-only breaks every graphics release. Release binaries embed a portion of game-derived data; this
  and the optional HD pack (upscaled derivative art) are disclosed in `NOTICE`/`DISCLAIMER`.

## Byte-exact discipline — the one rule that must not break

**The matching build must stay byte-exact. Re-run `make check` after ANY change under `src/` or
`include/`** (target MD5 `596bb082a2de5f1fe977dd3d7e160b03`). The matching build defines **none** of
the gates below, so every port-side edit to shared source must sit behind one:

- `#ifdef PERMUTER` — PC-build behavioural/layout changes (the PC Makefile defines it globally for
  game source). E.g. `src/text.c`'s `sFontGlyphBitmaps[129][9]` (PC) vs `[128][9]` (matching).
- `#ifdef PC_PORT` — portability/64-bit correctness guards (per-site NULL-deref guards, etc.).
- `#ifdef PC_PORT_LP64` — 64-bit-host-only struct-layout fixes (e.g. `Object_719`/`_675` in
  `include/object.h`, where a leading pointer's 4→8-byte growth shifts aliased fields).
- `#ifdef PC_FEAT` — **(Stage 3)** PC-only gameplay/QoL additions (e.g. the bidirectional ally-cycle
  in `battle_0201b8.c`). Distinct from `PC_PORT` (correctness) so gameplay changes are greppable alone.
- `#ifdef PC_DEBUG_*` — per-file debug/instrumentation hooks, keyed to Makefile flags.

`grep -rnE "PERMUTER|PC_PORT|PC_FEAT|PC_DEBUG" src/ include/` finds them all. **History lesson:** an
*unconditional* `src/text.c` widening once silently broke the match for ~2 days — gate first, then
`make check`.

## Repo layout

- `src/*.c` — the decompiled C source, one file per logical unit. `split_XXXXXX.c` / `fx_XXXXXX.c` /
  `battle_XXXXXX.c` names come from the unit's start VRAM address where no semantic name is assigned
  yet — rename opportunistically, but check `symbol_addrs.txt` / the yaml first so splat's segment
  mapping doesn't break.
- `include/*.h` — project headers. `include/PsyQ/` holds the **real Sony PsyQ v3.3 SDK headers** and
  is **gitignored** (proprietary, local-build-only) — do not confuse with the **clean-room** PsyQ
  headers under `platform/pc/include/PsyQ/`, which are tracked and safe.
- `SLUS_004.47.yaml`, `symbol_addrs.txt` — splat config and the authoritative address→symbol map.
- `Makefile` — top-level decomp build (`check`, etc.). Port build is `platform/pc/Makefile` (and
  `platform/pc/CMakeLists.txt`).
- `asm/`, `build/`, `assets/` — generated by splat against a real game copy; all gitignored.

## Build environment

Every dependency has been sourced and the build is verified byte-exact, but a fresh checkout has
none of it installed — the [`decomp-build`](.claude/skills/decomp-build/SKILL.md) skill has the full
recipe (headers, toolchain, base game files, exact PATH/env overrides; several tools live in sibling
folders or use non-default package names). Building the **port** needs far less: a Linux host with
SDL2/OpenAL/OpenGL dev libraries and Python 3 — see [`docs/building.md`](docs/building.md). Nothing
game-derived (the disc, `SLUS_004.47`, Sony BIOS/SDK data, in-game text) is committed; the build
reconstructs what it needs from the user's own copy.

## Working conventions

- Keep this file a concise, accurate, always-loaded overview. It is **committed and public** —
  reference only tracked files (`docs/`, `.claude/skills/`, real source paths), never `exchange/` or
  external notes that a cloner won't have. Put deep-dive detail in `docs/` or a skill.
- Update `docs/` and this file at milestones rather than writing new top-level summary files.
