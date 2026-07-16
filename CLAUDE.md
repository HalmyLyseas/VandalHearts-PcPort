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
   `platform/pc/build/vandalhearts_pc` (`make link`). Current work (2026-07-13) is a
   rendering-correctness pass, comparing our output against real hardware (BizHawk) at matched
   camera poses: several GTE-backend bugs have been found and fixed this way (`SetGeomOffset`
   not shifting the projection centre `<<16`; `zsf3/zsf4` never set, flattening terrain depth),
   with a few known rendering differences still open (far-terrain `otz` overflowing the OT →
   black patches; some sprites horizontally flipped). See the `phase-c-pc-port` skill and
   `exchange/00-progress-checkpoint.md` for current status.
   **Important**: the current build is intentionally **32-bit** (`-m32`), not 64-bit — a
   deliberate debugging baseline (a real chunk of decompiled source does raw pointer-relative
   struct access that breaks under 64-bit's wider pointers), not the final target. 64-bit is
   still required eventually (macOS dropped 32-bit entirely in 2019) — see the skill's
   32-bit callout before changing this. `OpenDriver2/PsyCross` was evaluated and rejected as
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

## Stage 2 (PC port) status (2026-07-13)

- The native PC port lives under `platform/pc/` (all 6 PSX subsystem backends, the data-segment
  generator, SDL2/OpenGL windowing). `make link` builds `platform/pc/build/vandalhearts_pc`;
  it runs the demo battle and renders battle scenes with unit sprites.
- Active work is **rendering correctness vs. real hardware**, driven by matched-pose BizHawk
  comparison (see the `phase-c-pc-port` skill's "Render-debugging tooling" section). Recurring
  bug class this session: **GTE setup values left unshifted/unset in `platform/pc/src/libgte.c`**
  (`SetGeomOffset` stored screen offsets raw instead of `<<16`; `zsf3/zsf4` were never assigned)
  — silent until unit sprites actually render, then they translate/mis-depth the whole scene.
- Debug-only, gated flags/tooling added (matching build untouched): `NO_FADE=1` / `NO_LOADING=1`
  make-flags, `VH_CAM_OSD=1` (on-screen camera-pose overlay), `SPRITE_LOG=1` + `VH_SPRITE_LOG`
  (per-sprite cull/projection/GTE-state CSV), `exchange/30-disable-fade-bizhawk.lua`.
- **Byte-exact re-verified 2026-07-13** (MD5 `596bb082a2de5f1fe977dd3d7e160b03`) — and doing so
  caught a **real regression**: a prior stage-2 PC-port fix (widening `sFontGlyphBitmaps` from
  `[128][9]` to `[129][9]` in `src/text.c` to kill a start-menu glyph artifact) had been applied
  *unconditionally*, adding 12 bytes to `.data` and silently breaking the match since ~07-11. Now
  fixed: the widening is gated behind `#ifdef PERMUTER` (defined only by the platform/pc build),
  so the matching build keeps `[128]` and the PC build keeps `[129]`. **Rule this established:**
  any stage-2 change to decompiled `src/*.c` that alters code size or data layout MUST be gated
  (`#ifdef PERMUTER` for behavioural/layout changes; `#ifdef PC_DEBUG_*` for debug-only hooks) and
  re-verified with `make check`, or it breaks stage-1 invisibly.
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
