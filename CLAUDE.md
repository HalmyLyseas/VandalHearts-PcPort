# Vandal Hearts Decompilation — Project Context

## What this is

A matching decompilation of the US PS1 release of Vandal Hearts (`SLUS_004.47`), using the
standard PSX-decomp toolchain: [splat](https://github.com/ethteck/splat) for
disassembly/extraction, [maspsx](https://github.com/mkst/maspsx) to post-process GCC 2.x
asm for the PSX assembler quirks, an old GCC 2.x frontend (`cc1_v263`/`cc1_v257`, prebuilt
by [decompals/old-gcc](https://github.com/decompals/old-gcc)) for compilation, and a
`mips-suse-linux-*` binutils cross-toolchain for assembling/linking. The build is verified by
`md5sum`-comparing the rebuilt `SLUS_004.47` against the original.

## End goal — read this before making architectural calls

**The actual objective is a native PC port**, not just a matching PS1 rebuild. The matching
decomp is stage 1 of a two-stage project:

1. **Stage 1 (✅ done 2026-07-10): matching decomp.** Prove the C source is bit-identical
   to the original binary. `make check` produces a byte-exact `SLUS_004.47` (`md5sum` and
   `cmp` both confirm). See "Current status" below.
2. **Stage 2 (in progress, the active phase): de-consolization.** Replace every PSX
   hardware call — GPU packet submission (`libgpu`), GTE fixed-point matrix math (`libgte`),
   CD-ROM/XA audio streaming (`libcd`/`libpress`), SPU audio, pad input — with a modern
   equivalent (SDL2/OpenGL, decided after evaluating Vulkan). All 6 swappable-interface
   subsystems are done, all 70 `src/*.c` files compile, the link succeeds, **and the game runs
   real demo battles and renders battle scenes with real unit sprites** —
   `platform/pc/build/vandalhearts_pc` (`make link`). As of 2026-07-17 the **full core game loop
   runs end-to-end** from the real disc and **A/V fidelity is complete (user-signed-off)**; the
   earlier rendering-correctness bugs (perspective collapse, sprite occlusion, terrain `otz`) are
   all fixed. The project has **forked** to a private repo (Codeberg) and moved into the
   de-consolization phases: 2.1 music ✅, 2.2 memory-safety ✅ (cap-less portable fault handler),
   **2.3 64-bit ✅ DONE (2026-07-21)** + **AddressSanitizer OOB sweep ✅ DONE (2026-07-23), 7 more OOB
   bugs fixed (`exchange/58`)** + **2.4 cross-platform ✅ (Windows + Linux, 2026-07-23; macOS deferred)**.
   Stage 2 is essentially complete. **Canonical developer/user docs now live in committed [`docs/`](docs/)**
   (architecture, building, configuration, per-subsystem deep-dives, memory-safety, cross-platform) —
   prefer those for durable reference; `exchange/` remains gitignored investigation scratch. See also the
   `phase-c-pc-port` skill, `exchange/52-stage2-roadmap.md`, and `exchange/00-progress-checkpoint.md`.
   **Build width (updated 2026-07-21): the default is now 64-bit** (`make link`). The port was
   deliberately **32-bit** for a long time as a debugging baseline — decompiled source assumes the
   PS1's 32-bit-pointer layout in places, and at 64-bit that shifts struct fields *silently*
   instead of failing loudly. Phase 2.3 fixed every case and validated the demo battle at both
   widths, so `-m32` is no longer the default. The 32-bit build remains available as an A/B
   reference: `make link M32=-m32 BUILD_DIR=build32`.
   The five width bugs found, all worth knowing before touching this area (details in
   `exchange/52-stage2-roadmap.md`): PSX `long` is 32-bit but LP64's is 64 (a `long *` GTE
   out-param wrote 8 bytes into a 4-byte local — stack smash); raw byte offsets into the Object
   union (`d.bytes[4]`); union members deliberately aliasing `Object_Sprite.coords`; the GPU OT
   link storing a truncated host pointer (now a token bridge); and the data-segment generator
   probing `sizeof()` at a hardcoded `-m32`. **All but the last are invisible to static audit —
   they were found by building and running.** `OpenDriver2/PsyCross` was evaluated and rejected as
   a drop-in — kept as a reference only.

Do not "clean up" or restructure decompiled code toward stage-2 concerns unless explicitly
working on stage 2 — stage 1's job is byte-exact matching, not readability or portability.
Keep the two concerns separated in commits and discussion.

## Current status (last verified 2026-07-10)

- **`make check` produces a byte-exact match** — verified via both `md5sum` and `cmp` against
  the user's own legally-owned copy of the game. This is the first time this has actually been
  reproduced from a from-scratch environment; the prior claim (commit `a9e53de`, 2024-09-17,
  *"Finish matching all non-PsyQ functions!"*) is now independently confirmed accurate, not
  just asserted.
- All application code is decompiled and matches: **1184 typed functions across 70
  `src/*.c` files**. PsyQ SDK library functions are intentionally left as raw, un-decompiled
  asm — this is standard practice; Sony's proprietary SDK isn't the target of the decomp.
- Two small regions of `src/text.c` are byte-exact placeholders rather than proper decompiles
  (`D_800151C8[888]`, `D_80122FB0`..`D_80123090`) — both marked `TODO` in the source. Neither
  is referenced by any code yet; a real decompile would identify the actual consumer and
  replace them with typed structures. See `exchange/00-progress-checkpoint.md`'s Phase B
  section for the full story — don't delete these as "dead code," they hold specific bytes.
- Full environment-setup recipe (headers, toolchain, base game files, exact build commands)
  is documented in the `decomp-build` skill (`.claude/skills/decomp-build/SKILL.md`) — use it
  rather than re-deriving from scratch if the build environment needs to be rebuilt.

## Stage 2 (PC port) status (2026-07-17)

- The native PC port lives under `platform/pc/` (all 6 PSX subsystem backends, the data-segment
  generator, SDL2/OpenGL/OpenAL windowing+audio). `make link` builds
  `platform/pc/build/vandalhearts_pc`. The **full core game loop runs end-to-end** from the real
  disc (intro FMVs, cutscenes, scripted + story battles, world map, dialogue, shops, save/load).
- **A/V fidelity complete + user-signed-off (2026-07-16; music re-opened and closed properly
  2026-07-21):** graphics (GTE/perspective, terrain, unit-sprite depth/occlusion) and sound
  (sample-accurate software SPU + SEQ music, CD-XA, MDEC video, SJIS/kanji text) all validated
  against real hardware (BizHawk). The 2026-07-13/14 rendering bugs (perspective collapse, sprite
  occlusion, terrain otz) are all fixed.
  **Caution for future sessions:** the 07-16 *sound* sign-off was by ear and turned out to be
  premature — four real bugs remained (missing `ProgAtr.mvol`, pan divisor 63-not-64, PsyQ's
  **square volume law**, and VAB **tone-block packing** which had five programs playing the wrong
  instrument's samples). All fixed 07-20/21; measured error vs the octoshock reference is now
  **1.33 dB mean** (was 3.21). Treat "signed off" as "no known issue", not "verified" — and see
  `exchange/57`'s structural note: our SPU *hardware* emulation was never the problem, every bug
  was in the **PsyQ `libsnd` reimplementation**, a layer neither psx-spx nor octoshock covers.
- **Forked** to a private repo (Codeberg `halmyrach/VandalHearts-PcPort`, origin); `upstream-master`
  = the pristine byte-exact base. Commits allowed now. See memory `reference_codeberg_push`.
- **Stage 2 (de-consolization) is essentially COMPLETE**, sequenced per `exchange/52-stage2-roadmap.md`:
  2.0 fork ✅ · 2.1 music ✅ · **2.2 memory-safety ✅** (removed the setcap/zero-page + `/proc`
  rodata crutches — a portable SIGSEGV fault handler in `pc_bootstrap.c` emulates transient PSX
  NULL-reads + in-place `.rodata` writes, so the build runs **cap-less, no root**) ·
  **2.3 64-bit ✅ DONE (2026-07-21)** (NULL guards, GPU token bridge, `long`→`int` width fix, union-offset fixes; default is now `-m64`) ·
  **2.4 cross-platform ✅ (Windows + Linux, 2026-07-23)** — a MinGW-w64 `.exe` is cross-compiled from
  Linux (CMake toolchain file) and **validated end-to-end on real Windows** (full demo playthrough);
  self-contained DLL package + drop-in disc auto-detect + `vandalhearts.ini` config + fatal wrong-disc
  check. **macOS/Apple Silicon DEPRIORITIZED** (can't cleanly cross-compile from Linux; `__APPLE__`
  scaffolding left in place for an on-device finish — see memory `stage2_4_crossplatform`).
- **Stage-2.3 out-of-bounds sweep ✅ COMPLETE (2026-07-23), `exchange/58-asan-sweep.md`.** After the
  `-m64` flip, an AddressSanitizer playthrough (chapters 1/4/6 + final battle + credits) found **7 real
  OOB bugs** static audit could not — a class where an index is simply *wrong* and only the consequence
  changes with pointer width. All fixed `PERMUTER`-gated + byte-exact: `gClutIds[124]→128` (clobbered
  `s_cdSyncStatus`, in retail too), `gWindowDisplayX/Y[16]→70` (was overwriting **live XA audio state**),
  `gUnitAnimSets[144]→301`, `gStringTable[100]→101`, travel-cost tables `[14][20]→[20][20]`,
  `D_8017DF50[27]→[29][64]` (AI-grid write). **Tooling now in `platform/pc/`:** `make asan32` +
  `./run_asan.sh` (must be 32-bit — 64-bit ASAN's shadow collides with the `0x80000000` arena);
  `make ubsan` (bounds-checking that *does* work at 64-bit); `tools/struct_width_diff.sh` (the width-bug
  class neither sanitizer sees). This also retires the "re-run the 6-chapter NULL sweep at 64-bit" 2.4
  to-do. **Method lesson (bit me 3×): a mis-sized-array *report* is not automatically a too-small
  *declaration* — prove which index is out of range against the byte-exact binary before widening.**
- **Gating conventions for `src/*.c` edits** (the matching build must stay byte-exact — **re-run
  `make check` after any `src/` change**, MD5 `596bb082a2de5f1fe977dd3d7e160b03`): the matching
  build defines **none** of these, so each keeps its `src/` edit out of stage 1 —
  - `#ifdef PC_DEBUG_*` — per-file debug/instrumentation hooks, keyed to Makefile flags.
  - `#ifdef PERMUTER` — PC-build behavioural/layout changes (PC Makefile defines it globally for
    all game source). E.g. `src/text.c`'s `sFontGlyphBitmaps[129][9]` (PC) vs `[128][9]` (matching).
  - `#ifdef PC_PORT` — **(NEW, Stage 2.3)** portability/64-bit correctness guards (e.g. per-site
    NULL-deref guards replacing the x86-32 fault decoder). Also PC-build-only.
  - `grep -rnE "PC_DEBUG|PERMUTER|PC_PORT" src/` finds them all. **History lesson:** an
    *unconditional* `src/text.c` widening once silently broke the match for ~2 days — gate, then
    `make check`.
- **Debug-only gates in decompiled source** (`grep -rn PC_DEBUG src/`): `src/screen_effects.c`,
  `src/cd.c`, `src/object.c` carry `#ifdef PC_DEBUG_*` blocks; they compile out of the matching
  build and are included in the re-verified byte-exact result above. If matching ever regresses,
  check recent `src/` layout/size changes first (see the `phase-c-pc-port` skill's "Gated
  instrumentation / stage-2-safe edits" note).

## Repo layout

- `src/*.c` — decompiled C source, one file per logical unit (roughly one per original
  object file boundary). Filenames like `split_XXXXXX.c`, `fx_XXXXXX.c`,
  `map_effects_XXXXXX.c`, `battle_XXXXXX.c` are named by their start VRAM address because a
  clearer semantic name hasn't been assigned yet — rename opportunistically as understanding
  improves, but check `symbol_addrs.txt` / the yaml before renaming to avoid breaking splat's
  segment mapping.
- `include/*.h` — project headers (`common.h`, `graphics.h`, `audio.h`, `object.h`, `state.h`,
  etc.). `include/PsyQ/` (libgpu.h, libgte.h, libcd.h, libpress.h, …) is populated locally
  (real Sony PsyQ v3.3 SDK headers) but **gitignored** — proprietary, local-build-use-only by
  explicit user decision; see the `decomp-build` skill before re-sourcing these.
- `SLUS_004.47.yaml` — splat config: segment/section layout, symbol paths, compiler settings.
- `symbol_addrs.txt` — the authoritative address→symbol map (1735 lines) that both splat and
  the linker rely on.
- `Makefile` — build orchestration (`extract`, `check`/build, `clean`). See targets and
  toolchain variables at the top of the file for exact versions/flags expected.
- `tools/old/` — legacy scripts (dosbox-based assembly, PsyQ obj/lnk parsing, data extraction)
  from an earlier iteration of the project. Kept for reference; not part of the current
  Makefile-driven flow. `tools/maspsx/` is populated locally (symlink to a sibling clone),
  gitignored — see the `decomp-build` skill.
- `asm/`, `build/`, `assets/` appear after `python3 -m splat split SLUS_004.47.yaml` runs
  against a real copy of the game (`make extract` itself is currently broken — it
  unconditionally shells out to a still-missing `sortSymbols.py`; run `splat` directly
  instead, per the `decomp-build` skill). All three are gitignored.

## Known blockers to building

**None as of 2026-07-10** — every dependency has been sourced, wired up, and the resulting
build is verified byte-exact. Don't assume this list stays current forever (a fresh container
won't have any of this installed) — the `decomp-build` skill has the full recipe to redo it,
including exact PATH/env-var overrides needed because several tools live outside `vh/`
(sibling folders under `vandalHearts_decomp/`) or use non-default package names in this
environment. `README.md`'s setup section is still just a placeholder ("TODO") — the real,
proven steps exist in the skill but haven't been transcribed into the README yet.

## Working conventions

- `exchange/` (inside this repo, but **gitignored — never git-tracked**) is the
  staging/communication layer for anything that shouldn't enter this repo's git history:
  sourced headers pending vetting, toolchain notes, research, extracted-asset scratch space.
  It lives inside the repo purely for convenience (one tree to work in); it is not part of
  the project's committed content.
- `exchange/00-progress-checkpoint.md` is the living top-level progress tracker across
  both stages — update it at milestones instead of writing new summary docs. Step-specific
  detail docs live alongside it as `NN-<topic>.md`.
- `context.txt` (also gitignored) is the original task-framing note from the user — kept for
  reference, not project documentation in its own right.
- `.claude/skills/` in this repo holds deeper, on-demand knowledge for continuing this
  project (build-system internals, later: PC-port architecture) — keep it updated as work
  progresses rather than letting this file grow unbounded. This file (`CLAUDE.md`) should
  stay a concise always-loaded overview; put deep-dive detail in a skill instead.
