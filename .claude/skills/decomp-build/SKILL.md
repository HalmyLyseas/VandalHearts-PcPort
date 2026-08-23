---
name: decomp-build
description: How the Vandal Hearts matching-decomp build system works — toolchain, dependencies, build/verify steps, and current known blockers. Use when setting up the build environment, running make extract/check, or diagnosing why the build doesn't produce a matching SLUS_004.47.
---

# Decomp build system

## Pipeline

`make extract` → `make check` (default target). Roughly:

1. `tools/sortSymbols.py` sorts `symbol_addrs.txt` against `undefined_additional.txt` —
   **in practice this step is currently skippable**, since the script is missing and
   `symbol_addrs.txt` as committed works fine on its own (confirmed 2026-07-10 by running
   `splat` directly instead of via `make extract`).
2. `splat split SLUS_004.47.yaml` disassembles the original `SLUS_004.47` binary into
   `asm/`, `asm/data/`, and `assets/` based on the segment layout in the yaml + symbol map,
   producing `.s` files for un-decompiled regions and stub `.c` files matched against
   `src/*.c` where decompiled source already exists.
3. Each `src/*.c` is preprocessed (`cpp -P`) → compiled with an **old GCC 2.x frontend**
   (`cc1_v263_decompals` by default, `cc1_v257_decompals` for a couple of special-cased
   files) → post-processed by `maspsx` (fixes up asm for PSX assembler quirks, e.g.
   div expansion) → assembled with `mips-suse-linux-as -EL`.
4. Raw `.s` files (un-decompiled/library regions) assemble directly.
5. Everything links with `mips-suse-linux-ld -EL` against a splat-generated linker script
   (`SLUS_004.47.ld`) plus auto-generated undefined-symbol files.
6. `objcopy -O binary` produces `build/SLUS_004.47`.
7. `make check` runs `md5sum` on the original and rebuilt binary — **this is the only
   correctness signal**; if it doesn't match, something in the pipeline is wrong (wrong
   compiler flags, wrong symbol address, an actual mismatch in the decompiled C, etc.).

Per-file overrides exist in the Makefile for a handful of files that need different
optimization/GP flags than the global defaults (`core/audio.c` uses `cc1_v257` + `-O2` + `-G0`;
`core/glyphs.c`, `ui/supplies.c`, `world/dojo.c`, `world/map.c` use `-G0`). If you add per-file special
casing, follow this pattern rather than changing global flags.

## The Japanese tree (`jp/`) builds the same way — with three differences

`jp/` is a complete second matching decomp (SLPM-86007, 88/88 TUs byte-exact) with its own
Makefile, yaml, symbol map and verification target:

    cd jp && PATH="$PWD/external/toolchain/bin:$PATH" make check < /dev/null
    # both md5 lines must read 53849277b08184863bd45f10925995a6

1. **PATH**: the JP tree ships its own slim toolchain at `jp/external/toolchain/bin`
   (`cc1_v263_decompals` etc. — same compilers as the US build, prebuilt); nothing else from
   the US environment recipe is needed.
2. **`< /dev/null` is mandatory FOR BOTH TREES** (maspsx blocks on non-EOF stdin — a
   backgrounded US `make check` without it slept 21 min, 2026-08-22) and the usual rules apply:
   never pipe make through grep, never two makes over one build dir.
3. Its `.inc` regeneration quirk: editing `assets/*.inc` does NOT invalidate `build/%.i` —
   touch the including `.c`. From-clean builds work (the build rules mkdir their own output
   dirs).

Gated PC edits to `jp/src`/`jp/include` follow the same PERMUTER/PC_PORT discipline as the US
tree, verified against the JP md5 above.

## Toolchain / dependency inventory

**Local dependency layout:** all local, non-committed build dependencies live under **`vh/external/`**
(gitignored): `external/maspsx`, `external/old-gcc`, `external/toolchain` (cc1 symlinks),
`external/psy-q` (the PsyQ SDK header source), and `external/game` (disc images + `SLUS_004.47`).
**`external/references`** lists the upstream URLs to re-obtain any of them. Paths below are relative to
the `vh/` repo root (where you run `make`).

| Dependency | Purpose | Status |
|---|---|---|
| `splat` (Python pkg) | disassembly/extraction (`make extract`) | **done** — system package `python-splat64`, Arch `extra/` (upstream `ethteck/splat`). **Re-verified 2026-08-23 at 0.50.0** (spimdisasm 1.42.4): a full `python3 -m splat split SLUS_004.47.yaml` runs end-to-end with zero schema errors, and the rebuild from the regenerated `asm/` is still byte-exact. Previously recorded working at 0.41.0. |
| `tools/sortSymbols.py` (+ its `symbols.csv` input) | sorts `symbol_addrs.txt` before splat runs (`SORT_SYM` step) | **missing from the repo** (a bespoke script + personal spreadsheet the original author never committed). It is **skippable**: `symbol_addrs.txt` as committed is already sorted sufficiently — run `splat` directly instead of via `make extract`, which unconditionally invokes the missing script. |
| `tools/maspsx/maspsx.py` | post-process GCC output for PSX `as` | **present** — a `mkst/maspsx` clone at `external/maspsx/`; `vh/tools/maspsx` symlinks to it (`-> ../external/maspsx`), which is how the Makefile (`tools/maspsx`) reaches it. |
| `cc1_v263_decompals`, `cc1_v257_decompals` | old GCC 2.x compiler frontends | **present** — prebuilt `cc1` binaries from `decompals/old-gcc`'s Releases (tag `0.17`: `gcc-2.6.3-psx.tar.gz`, `gcc-2.5.7-psx.tar.gz`; no Docker build needed) under `external/old-gcc/build-gcc-{2.6.3,2.5.7}-psx/`, symlinked into `external/toolchain/bin/` under the exact names the Makefile expects. **Must be on `PATH`** when running `make` — see "Wiring up cc1" below. |
| `{as,ld,objcopy}` for MIPS r3000 | cross binutils | **done** — `mipsel-linux-gnu-binutils`; use `make CROSS=mipsel-linux-gnu- ...` instead of editing the Makefile's hardcoded `mips-suse-linux-` prefix. See "Toolchain vendor triple" below. **On Arch this is AUR-only, not a repo package** (`yay -S mipsel-linux-gnu-binutils`) — the 2026-07-10 note calling it a "system package" was wrong, or the package has since left the repos. Verified byte-exact at **2.46.1** and again at **2.47-1** (2026-08-23). `mipsel-linux-gnu-binutils-minimal` (2.45) is the AUR fallback if a version ever misbehaves. |
| `python3.11` | required interpreter version (pinned in Makefile) | **not installed** (re-checked 2026-08-23) — only `python3.14` is present, which is what `python-splat64` targets. Use `make PYTHON=python3 ...` rather than editing the Makefile. |
| `include/PsyQ/*.h` (libgpu.h, libgte.h, libcd.h, libpress.h, …) | headers the app code includes directly | **user-supplied**, gitignored — real Sony PsyQ **v3.3** SDK `INCLUDE/` headers at `vh/include/PsyQ/` (lowercase). Proprietary, never committed. Validate a candidate set by compiling `src/core/graphics.c` through the full pipeline. A modern hosted header set does NOT work. See "PsyQ SDK headers" below. |
| `SLUS_004.47` (original exe), `LIB34.ZIP` (PsyQ v3.4 lib objects), full ISO | base game files, needed for extraction + asset/audio data | **`SLUS_004.47` done 2026-07-10** — extracted from the user's own CHD via `chdman`+`bchunk`+`7z`, MD5-verified exact match. See "Getting the base game files" below. `LIB34.ZIP` not needed by the current pipeline (see there); disc images + `SLUS_004.47` kept in `external/game/` for later asset/audio extraction if needed. |

Before trusting this table, re-verify — it reflects a single point-in-time check, not a
guarantee these stay missing.

## Re-establishing the host after an OS reinstall (Arch) — 2026-08-23

Done once for real (CachyOS → Omarchy). **Everything the repo needs that isn't a system package
lives in `external/` and `include/PsyQ/` — if `$HOME` survives, so does all of it**, and the only
work is reinstalling packages. Nothing in the repo, and no hand-made artifact (`assets/*.inc`,
the `include/assets` and `tools/maspsx` symlinks, `external/toolchain/bin` cc1 symlinks), needed
recreating. Budget ~15 min, most of it AUR compile time.

```sh
sudo pacman -S --needed base-devel python-splat64        # matching decomp
yay -S mipsel-linux-gnu-binutils                         # AUR — the only real blocker
```

That is the *entire* dependency set for `make check` on both trees. Notes:

- **The cc1 binaries are 32-bit i386 static ELF**, so the host kernel needs ia32 emulation
  (`CONFIG_IA32_EMULATION`). Stock Arch/Omarchy kernels have it; verify with a bare
  `external/old-gcc/build-gcc-2.6.3-psx/cc1 </dev/null` — it should emit `.file 1 "stdin"`
  rather than "cannot execute". **No 32-bit userspace/multilib is needed** (they're static).
- **`splat` is not needed for `make check`** — only for `make extract`. `asm/` holds just 16
  data `.s` files (all 88 TUs are decompiled), so once extracted it rarely needs regenerating.
- **`chdman`/`bchunk`/`7z` are not needed** unless re-extracting from the CHD; `external/game/`
  already holds the disc `.bin` and the verified `SLUS_004.47`.
- **`make dirs` alone may not be enough** after wiping `build/` — belt-and-braces, run the
  `find src asm assets -type d | sed 's|^|build/|' | xargs mkdir -p` line from "Verifying a
  build works" as well. (Both were run together in the 2026-08-23 rebuild, so which one
  sufficed is untested.)
- Port + release host packages are a separate, larger set — see the `phase-c-pc-port` skill.

## Toolchain vendor triple — `mips-suse-linux-` is not load-bearing

The Makefile hardcodes `CROSS := mips-suse-linux-`, but this vendor string isn't functionally
required — it's just whatever triple the project's original toolchain build happened to use.
Confirmed 2026-07-10 and again 2026-08-23: a stock `mipsel-linux-gnu-binutils` (2.46.1, then
2.47-1 — AUR on Arch) assembles/links/objcopies correctly against the exact flags this Makefile passes
(`-EL -march=r3000 -mtune=r3000 -no-pad-sections -G0` for `as`; `--cref
--no-check-sections` for `ld`; `-O binary` for `objcopy`), verified with an end-to-end smoke
test. This makes sense because the final artifact is a raw binary via `objcopy -O binary` —
intermediate object format (this toolchain emits ELF) doesn't affect the output bytes, only
instruction encoding does, and `maspsx` already exists specifically to normalize old-GCC
assembly for a modern GNU assembler.

Don't edit the Makefile to rename the prefix — override it on the command line instead:
`make CROSS=mipsel-linux-gnu- extract` / `make CROSS=mipsel-linux-gnu- check`.

This is **not** proof of byte-exact output yet, only that the pipeline mechanically works.
That's only provable once `cc1_v263_decompals`/`cc1_v257_decompals`, `maspsx`, `splat`, the
PsyQ headers, and the base game files are all in place and `make check`'s `md5sum` passes. If
it ever doesn't match, a binutils version difference in macro/pseudo-instruction expansion
(not the vendor triple) would be a reasonable first suspect — try pinning an older binutils
before suspecting the decompiled C. **Two versions are now known-good end-to-end (2.46.1 and
2.47), so treat "binutils drifted" as a weaker hypothesis than it first appears** — this
toolchain has proven insensitive to the version across a major bump.

Separately: `mipsel-linux-gnu-gcc`, if installed alongside the binutils package, is **not**
used by this build at all — the Makefile compiles with the specific prebuilt old GCC 2.x
frontends (`cc1_v263_decompals`/`cc1_v257_decompals`), not a modern distro GCC. Don't spend
time trying to make the modern `gcc` fit into the `CC` variable.

## Wiring up cc1

The Makefile references `cc1_v263_decompals` (global default `CC`) and, separately,
`cc1_v257_decompals` as a **literal hardcoded string** in a target-specific override
(`build/src/core/audio.c.s: CC := cc1_v257_decompals`). That second one is NOT parameterized —
passing `make CC=...` on the command line only changes the *global default*, it cannot
redirect that one target-specific line, since it's a fixed string, not a variable reference.
So both names must resolve via `PATH` lookup exactly as spelled — there's no way to point
them elsewhere without editing the Makefile (which we're avoiding).

Fix: `external/toolchain/bin/` contains symlinks
`cc1_v263_decompals -> ../../old-gcc/build-gcc-2.6.3-psx/cc1` and
`cc1_v257_decompals -> ../../old-gcc/build-gcc-2.5.7-psx/cc1` (relative within `external/`). Prepend
that directory to `PATH` before invoking `make`, from `vh/`:

```
PATH="$(pwd)/external/toolchain/bin:$PATH" make PYTHON=python3 CROSS=mipsel-linux-gnu- ...

The proven US one-liner (2026-08-22, used ~10x during the debug-menu track):

    PATH="$PWD/external/toolchain/bin:$PATH" make check CROSS=mipsel-linux-gnu- PYTHON=python3 < /dev/null

If `build/` was ever cleaned, the Makefile does NOT recreate its directory skeleton — recreate
it first (`find src asm assets -type d | sed 's|^|build/|' | xargs mkdir -p`), or every stage
fails with "can't create build/...: No such file or directory".
```

Both binaries are 32-bit x86 ELF, statically linked — verified they execute on this x86_64
host (kernel has ia32 support) and produce real compiler output (tested via direct invocation
and via a full `cpp -P` → `cc1` pipeline run against `src/core/graphics.c`, see below).

## PsyQ SDK headers (required, user-supplied, never committed)

The build needs the real **Sony PsyQ v3.3** SDK `INCLUDE/` headers (`libgpu.h`, `libgte.h`,
`libcd.h`, `libpress.h`, `libsnd.h`, `stdio.h`, `memory.h`, a `sys/` folder, …) placed at
`vh/include/PsyQ/` (lowercase filenames). These are **proprietary Sony material**: they are
**gitignored (`/include/PsyQ/`) and must never enter the repo or be redistributed** — supply
your own legally-obtained copy. If that `.gitignore` entry is ever missing, restore it before
committing anything.

Do **not** substitute a modern, hosted PsyQ-style header set (e.g. a PSX homebrew SDK's
headers) — this was tried and hard-fails. This build is **freestanding (`-nostdinc`)** and
uses the **real old GCC 2.6.3 `cc1`**: hosted headers assume a modern libc (they don't provide
`common.h`'s `sys/types.h`/`stdio.h`/`memory.h`/`libsnd.h`), and the old `cc1` throws parse
errors on modern constructs (`uint`/`uintptr_t`, `stdint.h`, precision-GTE `#include`s baked
into `libgpu.h`/`libgte.h`). It is a fundamentally different target (hosted/modern vs.
freestanding/1990s), not a "needs more patching" situation. Use an actual v3.3 SDK header set.

**Gotcha — these are DOS-era text with CRLF line endings** (`file include/PsyQ/libcd.h` →
"…with CRLF line terminators"). Plain `grep -n "symbol" include/PsyQ/whatever.h` can silently
report **zero matches even when the symbol is present** — grep's binary-file heuristic skips
them without warning. **Always `grep -a`** against anything under `include/PsyQ/`. This caused
time-costing false negatives twice (`CdlModeStream` in `libcd.h`, `struct DIRENTRY` in
`kernel.h`).

**Validate the header set, don't copy-and-hope:** run the actual pipeline — `cpp -P` (real
`CPP_FLAGS`, `-nostdinc`) → real `cc1_v263_decompals` → `maspsx` → `mipsel-linux-gnu-as` —
against `src/core/graphics.c` (the heaviest GTE/GPU header user). A correct set preprocesses and
compiles cleanly (only normal old-C89 implicit-declaration/type warnings, zero parse errors)
to a valid MIPS-I ELF object. That proves the pipeline works mechanically; byte-exact output
is only proven by `make check`'s `md5sum`, which additionally needs the base game files.

**Not header sources (don't chase these):** `mkst/ctr`/`mkst/esa` (wrong PsyQ version family,
no usable header set); `lab313ru/ghidra_psx_ldr` (a Ghidra loader + FLIRT signature data —
useful for identifying PsyQ functions in a disassembly, not for compilable headers);
`pcsx-redux` (ships only a small `inline_n.h` shim). pcsx-redux's `psyq-obj-parser` *does*
convert original PsyQ `.OBJ`/`.LIB` objects to modern-linkable form — but this pipeline extracts
PsyQ code as raw disassembly via splat and never relinks `LIB34.ZIP`, so it isn't needed.

## Getting the base game files

Your own `Vandal Hearts (USA).chd` in `external/game/` (watch out — verify the volume label
before trusting a CHD is the right one). Extraction recipe, run from `vh/`:

```
cd external/game
chdman extractcd -i "Vandal Hearts (USA).chd" -o "Vandal Hearts (USA).cue"   # -> .bin + .cue
bchunk -v "Vandal Hearts (USA).bin" "Vandal Hearts (USA).cue" "Vandal Hearts (USA)"  # -> standard 2048-byte-sector ISO
7z l "Vandal Hearts (USA)01.iso"        # sanity check: volume label should be SLUS_00447, publisher KONAMI
7z e "Vandal Hearts (USA)01.iso" -o. "SLUS_004.47" -r
md5sum SLUS_004.47   # must be 596bb082a2de5f1fe977dd3d7e160b03 (the hash README.md cites)
cp SLUS_004.47 ../../SLUS_004.47   # -> vh/SLUS_004.47 (the build's base binary)
```

`chdman` and `bchunk` were both already present on this system (`bchunk` via the `bchunk`
Arch package — note: `which bchunk` misses it if it's not been resolved yet in-shell, check
`pacman -Ql bchunk` directly if `which` reports nothing suspicious). Default `bchunk` mode
(2048 bytes/sector from offset 24) was used, not `-p` PSX mode — sufficient for reading the
ISO9660 filesystem and pulling out `SLUS_004.47`, which is a Form-1 sector file. If XA
audio/streaming asset extraction is needed later (Phase D), `-p` PSX mode or a
PSX-XA-aware tool may be needed instead, since Form-2 sectors have a different data layout
that naive fixed 2048-byte extraction would corrupt.

## Verifying a build works

**Confirmed 2026-07-10 — `make check` produces a byte-exact match.** `md5sum` and `cmp` both
confirm `build/SLUS_004.47` is identical to the original. This is stage 1, fully done.

```
cd vh
python3 -m splat split SLUS_004.47.yaml    # run splat directly — sortSymbols.py/SORT_SYM is skippable, symbol_addrs.txt is sufficient alone
make dirs
PATH="$(pwd)/external/toolchain/bin:$PATH" make PYTHON=python3 CROSS=mipsel-linux-gnu- check
```

(The `PATH=`/`PYTHON=`/`CROSS=` bits are specific to this environment's installed package
names and where the cc1 binaries were extracted to — see "Wiring up cc1" and "Toolchain
vendor triple" above, and the dependency table. Re-check these are still needed/correct
before copying blindly, especially the `toolchain/bin` path. `make extract` itself still
fails because it unconditionally runs the still-missing `sortSymbols.py` first — invoke
`splat` directly instead, as above, until that script is sourced or the Makefile is changed.)

### ⚠ `make clean` DELETES the hand-made `.inc` files (bit us 2026-08-09)

Top-level `clean` is `rm -rf asm assets build` — and `assets/` holds the 4 hand-generated
`.inc` files described below, which `splat` does NOT regenerate. After a `make clean`, a
rebuild fails at `maps/map_32.c` on the missing `assets/801009bc.inc`. Recovery: re-run
`splat` (for `asm/`), then regenerate the 4 `.inc` files from the verified `SLUS_004.47` with
the recipe below (field-ordered flat initializers: MapTileModel = 88+18 s16 then 92 u8 per
struct ×4 at 0x801009bc / ×1 at 0x80100e9c; 100 `(u8 *)0x...` pointers at 0x8010102c;
128×9 raw u8 at 0x801012e4). Verified 2026-08-09: the regenerated set reproduces the
byte-exact match from clean.

### The `assets/*.inc` gap (found and fixed 2026-07-10)

4 files (`maps/map_32.c`, `maps/map_35.c`, `core/text.c` ×2) do
`#include "assets/<vram-addr>.inc"` for hand-written C initializer literals of specific data
(`MapTileModel` struct(s), a `u8 *gStringTable[100]` pointer table, a
`u8 sFontGlyphBitmaps[128][9]` byte array) that `splat`'s generic `.data`/`.bin` extraction
doesn't produce — same category of gap as the missing `sortSymbols.py`, bespoke tooling the
original author had locally and never committed.

Fixed by: confirming the exact struct layout empirically (traced `SVECTOR`'s definition by
grepping the *preprocessed* output of `core/graphics.c`, not by guessing from a header, since
`SVECTOR`/`VECTOR`/`MATRIX`/`CVECTOR` turned out to be defined via `common.h`'s own include
chain rather than `PsyQ/libgte.h` — `{ s16 vx, vy, vz, pad; }`, no padding), then reading raw
bytes directly from the verified `SLUS_004.47` at `file_offset = 0x800 + (vram - 0x80010000)`
(the file-offset/VRAM mapping the yaml's `main` segment already defines) and formatting them
as flat, brace-elided C initializer lists (valid C89, and simpler/safer than trying to
reproduce nested field-name braces exactly). Pointer-typed data (`gStringTable`) was emitted
as absolute VRAM address casts (`(u8 *)0x801230xx,`) — valid because PS1 games load at a
fixed address, so literal pointer constants baked into `.data` are exactly how the original
compiler would have handled this too.

**Also needed:** `include/assets -> ../assets` symlink. `#include "assets/X.inc"` from a file
in `src/` does not resolve to the top-level `assets/` via quote-relative-to-source-file rules
(that would look for `src/assets/...`), and the Makefile's `CPP_FLAGS` has no `-I.`/`-Iassets`
— only `-Iinclude`/`-Iinclude/PsyQ`. The symlink makes `assets/X.inc` resolve via the existing
`-Iinclude` path without touching the Makefile. Both the `.inc` files and this symlink are
gitignored (`.inc` files fall under the existing `/assets/` rule; the symlink has its own
entry) — they're derived from copyrighted game data, same category as everything else pulled
from the disc.

**Verified correct**: these 4 `.inc` files were *not* the source of the mismatch found during
bisection (see below) — they held up under the full byte-exact verification.

### `splat` may rewrite some tracked helper files — expected, not a bug

`splat split` generates/refreshes the assembler-compatibility glue files (the `glabel`/`dlabel`/
`INCLUDE_ASM` macro definitions, GTE instruction macros for GAS) to match whatever splat version
is running, and creates `include/gte_macros.inc` / `include/labels.inc` (both gitignored).

- **At 0.41.0** it also modified the *tracked* `include/include_asm.h` and `include/macro.inc`,
  leaving the tree dirty after every extract. That was expected, not a mistake to revert —
  precedent being the project's own "switch to maspsx + current splat" commit touching these
  same files.
- **At 0.50.0 it no longer does** (verified 2026-08-23): a full split leaves `git status` clean,
  including a byte-identical regeneration of the tracked `SLUS_004.47.ld`.

Either way these files only affect assembler directive syntax (`.type`/`.size`/`.ent`/`.end`,
and which literal `.s` file gets `.include`d) — not instruction bytes — so they don't affect
byte-exact matching. If a future splat dirties them again, leave them as splat regenerates them.

## Bisection methodology (used successfully 2026-07-10, reusable for any future mismatch)

`asm-differ` was never actually needed — plain binutils + a Python one-liner was enough. The
loop, repeated until `cmp` reports no differences:

1. `cmp SLUS_004.47 build/SLUS_004.47` → first differing byte *offset*.
2. Convert to VRAM: `vram = 0x80010000 + (file_offset - 0x800)` (holds throughout the `main`
   segment — confirm against the yaml if working in a different segment/overlay).
3. Find the enclosing symbol: search `symbol_addrs.txt` for the nearest function/data address
   ≤ the target (a short Python script sorting `type:func`/all entries by address and scanning
   for the bracketing pair is faster than grepping by hand once you're past the first hit).
4. If it's a function: disassemble both binaries at that exact function's address range and
   diff:
   ```
   mipsel-linux-gnu-objdump -D -b binary -m mips:3000 -EL --adjust-vma=0x8000f800 \
     --start-address=<vram_start> --stop-address=<vram_end> SLUS_004.47 > /tmp/orig.txt
   # same for build/SLUS_004.47 -> /tmp/build.txt
   diff /tmp/orig.txt /tmp/build.txt
   ```
   (`--adjust-vma=0x8000f800` = `0x80010000 - 0x800`, i.e. "pretend file byte 0 is this VRAM
   address" — lets objdump work directly on the whole raw `SLUS_004.47`/`build/SLUS_004.47`
   without needing to slice it first.)
5. If it's data: `xxd -s <file_offset> -l <length>` on both files and diff/compare directly.
6. **The symptom's location is not necessarily the root cause's location.** A wrong-sized
   region anywhere shifts every address *after* it — a jump table or function address 20KB
   away can be the *first* visible difference even though the real bug is much earlier. When
   a diff shows a large *uniform* address/offset shift (not a content difference), that's the
   signature of a size mismatch somewhere earlier — cross-check every yaml-named subsegment's
   *actual* linked address (from `build/SLUS_004.47.map`) against its yaml-declared address to
   binary-search for exactly where the drift starts, rather than disassembling blindly:
   ```python
   # parse yaml "- [fileoff, .rodata, name]  # 0xVRAM" entries and compare against
   # map file "  .rodata   0xADDR   0xSIZE  build/src/NAME.c.o" lines — first mismatch
   # is where the real problem is, even if that's not where cmp first complained.
   ```
7. Fix, rebuild (incremental — `make` only recompiles what changed), re-run from step 1.

### What was actually wrong (all four fixed 2026-07-10, `make check` now byte-exact)

The four mismatches that had to be fixed to reach byte-exact:

1. `core/text.c` was missing 888 bytes of `.rodata` (a string pool of world-map/job-class names +
   two pointer tables) — caused a uniform 888-byte shift through the entire rest of the file.
   **Placement is order-sensitive**: had to go at the very top of the file (right after
   `#include`s), not the bottom — unreferenced top-level globals get emitted before a later
   function's switch-statement jump table, regardless of which comes first/last textually
   relative to *functions*, but top-level declaration order among *themselves* still matters.
2. `src/core/cd.c`'s `Movie_Start`: `CdRead2(CdlModeStream | CdlModeSpeed | CdlModeRT)` computes
   `0x1e0` from the real, verified PsyQ macros, but the original binary uses `0x1c0` — fixed
   with a literal constant + comment; no clean macro combination produces `0x1c0`.
3. `core/text.c` was also missing 232 bytes of `.sdata` (more name strings). **This had to be 29
   separate `static u8 D_XXXXXXXX[8]` declarations, not one array** — this Makefile's `-G8`
   only routes objects ≤8 bytes to `.sdata` automatically; anything bigger silently lands in
   `.data` at the wrong address instead. Also must **not** be `const` (`.sdata` is writable
   small data; `const` data goes elsewhere).
4. The `assets/*.inc` files from finishing Phase A (see above) were *not* wrong — verified
   clean by the full bisection.

Both `core/text.c` additions are marked `TODO` in the source — byte-exact but not semantically
decompiled (nothing references them; their true consumer/structure is still unidentified).
Worth a real decompilation pass later; doesn't block anything else.

## Known-good reference points

- `symbol_addrs.txt` (1735 lines) is authoritative for addresses — trust it over guessing.
- **`make check` byte-exact matching was independently reproduced 2026-07-10** in a from-
  scratch environment (fresh clone + this skill's toolchain-sourcing steps), confirming
  commit `a9e53de`'s (2024-09-17) claim of completeness was accurate — not just a claim.
  If a *future* from-scratch build doesn't match, suspect a regression introduced since
  2026-07-10 (check `git diff`/recent changes to `src/`) or an environment/toolchain
  difference (binutils version, etc.) before suspecting the underlying source is wrong.
- **Reproduced again 2026-08-23 on a rebuilt host** (OS reinstalled CachyOS → Omarchy, both
  Arch-based) at commit `ce079c2`, with a *newer* toolchain than either prior run —
  binutils **2.47**, splat **0.50.0**, python **3.14.7** — after deleting `build/` in both
  trees (**not** `make clean`, which eats `assets/*.inc`). Both regions matched:
  US `596bb082a2de5f1fe977dd3d7e160b03`, JP `53849277b08184863bd45f10925995a6`. Only
  `external/` and the system packages had to be re-established; nothing in the repo changed.
  This is the third independent reproduction and the second distinct toolchain generation.
- `SLUS_004.47`'s correct MD5 is `596bb082a2de5f1fe977dd3d7e160b03` (matches `README.md`) —
  verified 2026-07-10 against the user's own CHD.
- If `src/core/text.c`'s two `D_` placeholder blocks (see above) are ever removed/"cleaned up" by
  a future refactor without replacing them with an equivalent byte-exact structure, that will
  reintroduce this exact mismatch — don't delete them as "unused code" without checking.
