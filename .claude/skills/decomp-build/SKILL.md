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
optimization/GP flags than the global defaults (`audio.c` uses `cc1_v257` + `-O2` + `-G0`;
`glyphs.c`, `supplies.c`, `dojo.c`, `world_map.c` use `-G0`). If you add per-file special
casing, follow this pattern rather than changing global flags.

## Toolchain / dependency inventory

| Dependency | Purpose | Status as of 2026-07-10 |
|---|---|---|
| `splat` (Python pkg) | disassembly/extraction (`make extract`) | **done 2026-07-10** — system package `python-splat64` (0.41.0, upstream `ethteck/splat`) installed. Ran directly against `SLUS_004.47.yaml`: full config parsed with zero schema errors, got exactly to opening the (absent) base binary before failing — correct/expected failure point, confirms compatibility. |
| `tools/sortSymbols.py` (+ its `symbols.csv` input) | sorts `symbol_addrs.txt` before splat runs (`SORT_SYM` step) | **missing from repo, and not found anywhere else either** (checked `mkst/ctr`, `mkst/esa`, `sozud/psy-q-decomp` 2026-07-10 — none have it). Likely a bespoke script + personal spreadsheet the vh author never committed. Try `make extract` with the `SORT_SYM` step skipped/commented first — `symbol_addrs.txt` already exists and may be sufficient alone, making this possibly a non-blocker. |
| `tools/maspsx/maspsx.py` | post-process GCC output for PSX `as` | **done 2026-07-10** — user cloned `https://github.com/mkst/maspsx` to `vandalHearts_decomp/maspsx/`; symlinked `vh/tools/maspsx -> ../../maspsx`. Verified it runs through the symlink. |
| `cc1_v263_decompals`, `cc1_v257_decompals` | old GCC 2.x compiler frontends | **done 2026-07-10** — no Docker in this environment, so downloaded prebuilt release tarballs directly from `decompals/old-gcc`'s Releases page (tag `0.17`: `gcc-2.6.3-psx.tar.gz`, `gcc-2.5.7-psx.tar.gz`) rather than building via their Dockerfiles. Extracted `cc1` binaries into `vandalHearts_decomp/old-gcc/build-gcc-{2.6.3,2.5.7}-psx/`, verified executable, symlinked into `vandalHearts_decomp/toolchain/bin/` under the exact names the Makefile expects. **Must be on `PATH`** when running `make` — see "Wiring up cc1" below. |
| `{as,ld,objcopy}` for MIPS r3000 | cross binutils | **done 2026-07-10** — system `mipsel-linux-gnu-binutils` (2.46.1) works; use `make CROSS=mipsel-linux-gnu- ...` instead of editing the Makefile's hardcoded `mips-suse-linux-` prefix. See "Toolchain vendor triple" below. |
| `python3.11` | required interpreter version (pinned in Makefile) | **not installed** (found 2026-07-10) — only `python3.14` is present, which is what `python-splat64` targets. Use `make PYTHON=python3 ...` rather than editing the Makefile. |
| `include/PsyQ/*.h` (libgpu.h, libgte.h, libcd.h, libpress.h, …) | headers the app code includes directly | **done 2026-07-10** — real Sony PsyQ v3.3 SDK headers from `sozud/psy-q` (user's informed call given the provenance flag; see "Sourcing the PsyQ headers" below), populated into `vh/include/PsyQ/`, gitignored. Validated by actually compiling `src/graphics.c` through the full real pipeline — see below. `OpenDriver2/PsyCross` was tried first and empirically ruled out. |
| `SLUS_004.47` (original exe), `LIB34.ZIP` (PsyQ v3.4 lib objects), full ISO | base game files, needed for extraction + asset/audio data | **`SLUS_004.47` done 2026-07-10** — extracted from the user's own CHD via `chdman`+`bchunk`+`7z`, MD5-verified exact match. See "Getting the base game files" below. `LIB34.ZIP` not needed by the current pipeline (see there); full ISO/BIN kept in `vandalHearts_decomp/game/` for later asset/audio extraction (Phase D) if needed. |

Before trusting this table, re-verify — it reflects a single point-in-time check, not a
guarantee these stay missing.

## Toolchain vendor triple — `mips-suse-linux-` is not load-bearing

The Makefile hardcodes `CROSS := mips-suse-linux-`, but this vendor string isn't functionally
required — it's just whatever triple the project's original toolchain build happened to use.
Confirmed 2026-07-10: a stock `mipsel-linux-gnu-binutils` (2.46.1, e.g. from Arch/CachyOS
repos) assembles/links/objcopies correctly against the exact flags this Makefile passes
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
before suspecting the decompiled C.

Separately: `mipsel-linux-gnu-gcc`, if installed alongside the binutils package, is **not**
used by this build at all — the Makefile compiles with the specific prebuilt old GCC 2.x
frontends (`cc1_v263_decompals`/`cc1_v257_decompals`), not a modern distro GCC. Don't spend
time trying to make the modern `gcc` fit into the `CC` variable.

## Wiring up cc1 (done 2026-07-10)

The Makefile references `cc1_v263_decompals` (global default `CC`) and, separately,
`cc1_v257_decompals` as a **literal hardcoded string** in a target-specific override
(`build/src/audio.c.s: CC := cc1_v257_decompals`). That second one is NOT parameterized —
passing `make CC=...` on the command line only changes the *global default*, it cannot
redirect that one target-specific line, since it's a fixed string, not a variable reference.
So both names must resolve via `PATH` lookup exactly as spelled — there's no way to point
them elsewhere without editing the Makefile (which we're avoiding).

Fix: `vandalHearts_decomp/toolchain/bin/` contains symlinks
`cc1_v263_decompals -> ../../old-gcc/build-gcc-2.6.3-psx/cc1` and
`cc1_v257_decompals -> ../../old-gcc/build-gcc-2.5.7-psx/cc1`. Prepend that directory to
`PATH` before invoking `make`, e.g. from `vh/`:

```
PATH="$(cd .. && pwd)/toolchain/bin:$PATH" make PYTHON=python3 CROSS=mipsel-linux-gnu- ...
```

Both binaries are 32-bit x86 ELF, statically linked — verified they execute on this x86_64
host (kernel has ia32 support) and produce real compiler output (tested via direct invocation
and via a full `cpp -P` → `cc1` pipeline run against `src/graphics.c`, see below).

## Sourcing the PsyQ headers (resolved 2026-07-10)

Not written into the repo (proprietary Sony SDK, can't be redistributed) — and now also
excluded via `.gitignore` (`/include/PsyQ/`) even though populated locally, since the source
is legally uncertain and the user's explicit intent was local-only, non-distributed use. If
that `.gitignore` entry is ever missing, restore it before committing anything — don't let
this material end up in the repo's git history.

**Current state: `vh/include/PsyQ/` is populated with the real Sony PsyQ v3.3 SDK headers**,
sourced from `sozud/psy-q`'s `3.3/PSX/INCLUDE/` (see below for the full trail), and
**validated empirically** — see "Verifying a build works" below for the full pipeline test
that proved these work. If `include/PsyQ/` is ever missing/empty in a fresh session, this is
what needs restoring; the steps are: clone `sozud/psy-q`, copy `3.3/PSX/INCLUDE/*.H` →
`include/PsyQ/*.h` (lowercased) and `3.3/PSX/INCLUDE/SYS/*.H` → `include/PsyQ/sys/*.h`
(lowercased).

**Gotcha found repeatedly during Phase C work: these files are DOS-era text with CRLF line
endings** (`file include/PsyQ/libcd.h` reports "Non-ISO extended-ASCII text, with CRLF line
terminators"). Plain `grep -n "symbol" include/PsyQ/whatever.h` can silently report **zero
matches even when the symbol is genuinely present** — grep's binary-file heuristic trips on
these files and skips them without a warning. Always use `grep -a` against anything under
`include/PsyQ/`. This has caused real, time-costing false negatives twice: once concluding
`CdlModeStream` wasn't defined anywhere (it was, in `libcd.h`) during Phase B bisection, and
again concluding `struct DIRENTRY` wasn't defined anywhere (it was, in `kernel.h`) while
building the Phase C Kernel backend.

**Checked 2026-07-10: `mkst/ctr` and `mkst/esa` (credited in `README.md`) do NOT have a
usable set.** Actually inspected their trees (via GitHub's git-trees API, recursive) —
`mkst/ctr`'s `include/psy` (symlinked to `include/psyq4.3`) contains exactly **one** header
(`LIBSPU.H`), and its `tools/psyq/psyq4.x/` folders contain original DOS-era PsyQ SDK
*compiler executables* (ASPSX.EXE, CC1PSX.EXE, etc. — versions 4.0/4.1/4.3/4.6, wrong version
family anyway), not a C header set. Not a source for this.

**Two real candidates found 2026-07-10, neither vetted/vendored yet — stage in `exchange/`
first per the usual rule (license/version check before anything lands in `include/PsyQ/`):**

1. **`OpenDriver2/PsyCross`**, `include/psx/*.h` — **empirically ruled out 2026-07-10, do
   not retry without a different plan.** User cloned it; the full header set was staged into
   `vh/include/PsyQ/` and run through the *actual* pipeline: `cpp -P` with this project's
   real `CPP_FLAGS` (including `-nostdinc`), then the real `cc1_v263_decompals`, against
   `src/graphics.c`. Two independent, hard failures:
   - `common.h` (included by nearly every file) needs `sys/types.h`, `stdio.h`, `memory.h`,
     `libsnd.h`, `sys/file.h` — **PsyCross provides none of these**, because it assumes a
     modern hosted libc rather than this project's freestanding `-nostdinc` build.
   - Stubbing those just to see further, the real old `cc1` (2.6.3 — the actual version this
     game was built with) throws parse errors immediately on `uint`/`uintptr_t`/`ushort`
     (needs `stdint.h`, unavailable under `-nostdinc`) and on `PGXP_EmitCacheData`/
     `PGXP_GetIndex` — PsyCross's own precision-GTE extension, `#include`d directly inside
     `libgpu.h`/`libgte.h` unconditionally. This isn't stock PsyQ API surface at all, and the
     old compiler can't parse it regardless.
   Verdict: not a viable header source for stage-1 byte-exact matching, full stop — this
   isn't a "needs more patching" situation, it's a fundamentally different target
   (hosted/modern vs. freestanding/1990s). **PsyCross the framework is still a fine Phase C
   lead** (see the checkpoint doc) — only ruled out as a *header* source.
2. **`sozud/psy-q`** (a submodule of `sozud/psy-q-decomp`), path `3.3/PSX/INCLUDE/` — an
   exact match to the real Sony PsyQ **v3.3** SDK directory layout (BIN/DOC/INCLUDE/KANJI/
   LIB/SAMPLE), same version cited in this project's own `README.md`. Contains the literal
   original `LIBGPU.H`/`LIBGTE.H`/`LIBCD.H`/`LIBPRESS.H`/etc. — and, unlike PsyCross, this
   listing also includes `STDIO.H`, `MEMORY.H`, `LIBSND.H`, and a `SYS/` folder, i.e. the
   exact set PsyCross was missing. **Flagged as looking like an unlicensed, undocumented
   public mirror of Sony's proprietary SDK** (no README, no license, 0 stars, 1 watcher) —
   raised explicitly to the user 2026-07-10; **they made an informed decision to use it for
   local, non-distributed build use only** (explicitly not for redistribution/pushing
   anywhere). Cloned to `vandalHearts_decomp/psy-q/` (sibling to `vh/`, outside git). **This
   is now what's actually in `vh/include/PsyQ/`** — see the "resolved" note above. If sourcing
   this again in a future session, the same standard applies as the base ROM: this was a
   one-time informed call by the user given the specific circumstances, not a blanket green
   light — don't assume future similar decisions without asking again.

**Validation, not just copy-and-hope:** ran the actual pipeline —
`cpp -P` (real `CPP_FLAGS`, `-nostdinc`) → real `cc1_v263_decompals` → `maspsx` →
`mipsel-linux-gnu-as` — against `src/graphics.c` (heaviest GTE/GPU header usage in the
project, and the exact file that failed completely under PsyCross). Result: clean preprocess
(one harmless `#endif` warning), clean compile (17,953 lines of `.s`, only normal old-C89-era
warnings — implicit declarations / type mismatches from forward references, zero parse
errors), clean `maspsx` pass, valid final ELF object out of the assembler
(`MIPS-I, with debug_info`). Full mechanical pipeline confirmed working end-to-end on a real,
GTE/GPU-heavy file. Still not proof of *byte-exact* output — that needs `make check`'s
`md5sum`, which needs the base game files.

**Checked 2026-07-10: `lab313ru/ghidra_psx_ldr` (already credited in `README.md`'s Tools
list) is also NOT a header/toolchain source.** It's a Ghidra loader extension; its
`data/psyq` submodule (`lab313ru/psx_psyq_signatures`) is binary FLIRT-style signature data
for auto-identifying PsyQ functions across SDK versions 2.60–4.70 in a disassembled binary —
useful later for confirming the exact PsyQ SDK version and auto-labeling library functions
once `SLUS_004.47` is loaded into Ghidra, but no compilable headers, no old-GCC frontend, no
maspsx, no sortSymbols.py.

**Checked 2026-07-10: pcsx-redux (github.com/grumpycoders/pcsx-redux) is NOT a header
source.** Verified directly (fetched repo tree + `src/mips/psyq/README.md` +
`src/mips/psyq/include/` listing) — their `psyq/include/` folder contains exactly one file
(`inline_n.h`, a small compat shim), not the actual libgpu.h/libgte.h/libcd.h/libpress.h API
headers. What pcsx-redux *does* provide is `psyq-obj-parser`, a tool that converts original
PsyQ `.OBJ`/`.LIB` files (i.e. `LIB34.ZIP`, if/once obtained) into modern-linkable objects —
useful for library conversion, not for headers. `vh/tools/old/psyq-obj-parser/` is already a
copy of exactly this tool (its local README credits pcsx-redux), currently unused by the
current Makefile pipeline (which extracts PsyQ code as raw disassembly straight from the
original binary via splat rather than relinking `LIB34.ZIP`). Don't install the full
pcsx-redux app for this — the one relevant piece is already in the repo. Also confirmed
absent from pcsx-redux: `cc1_v263_decompals`/`cc1_v257_decompals`, `maspsx`,
`sortSymbols.py` — none of Phase A's other gaps are solved by it either.

## Getting the base game files (done 2026-07-10)

User's own `Vandal Hearts (USA).chd` in `vandalHearts_decomp/game/` (watch out — the first
file placed there was accidentally a different game's CHD; verify the volume label before
trusting a CHD is the right one). Extraction recipe:

```
cd vandalHearts_decomp/game
chdman extractcd -i "Vandal Hearts (USA).chd" -o "Vandal Hearts (USA).cue"   # -> .bin + .cue
bchunk -v "Vandal Hearts (USA).bin" "Vandal Hearts (USA).cue" "Vandal Hearts (USA)"  # -> standard 2048-byte-sector ISO
7z l "Vandal Hearts (USA)01.iso"        # sanity check: volume label should be SLUS_00447, publisher KONAMI
7z e "Vandal Hearts (USA)01.iso" -o. "SLUS_004.47" -r
md5sum SLUS_004.47   # must be 596bb082a2de5f1fe977dd3d7e160b03 (the hash README.md has always cited)
cp SLUS_004.47 ../../vh/SLUS_004.47
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
PATH="$(cd .. && pwd)/toolchain/bin:$PATH" make PYTHON=python3 CROSS=mipsel-linux-gnu- check
```

(The `PATH=`/`PYTHON=`/`CROSS=` bits are specific to this environment's installed package
names and where the cc1 binaries were extracted to — see "Wiring up cc1" and "Toolchain
vendor triple" above, and the dependency table. Re-check these are still needed/correct
before copying blindly, especially the `toolchain/bin` path. `make extract` itself still
fails because it unconditionally runs the still-missing `sortSymbols.py` first — invoke
`splat` directly instead, as above, until that script is sourced or the Makefile is changed.)

### The `assets/*.inc` gap (found and fixed 2026-07-10)

4 files (`map_effects_08f524.c`, `map_effects_092320.c`, `text.c` ×2) do
`#include "assets/<vram-addr>.inc"` for hand-written C initializer literals of specific data
(`MapTileModel` struct(s), a `u8 *gStringTable[100]` pointer table, a
`u8 sFontGlyphBitmaps[128][9]` byte array) that `splat`'s generic `.data`/`.bin` extraction
doesn't produce — same category of gap as the missing `sortSymbols.py`, bespoke tooling the
original author had locally and never committed.

Fixed by: confirming the exact struct layout empirically (traced `SVECTOR`'s definition by
grepping the *preprocessed* output of `graphics.c`, not by guessing from a header, since
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

### `splat` rewrites some tracked helper files — expected, not a bug

Running `splat split` modifies `include/include_asm.h` and `include/macro.inc` (both
tracked) and creates `include/gte_macros.inc` / `include/labels.inc` (new). This is normal:
splat generates/refreshes these assembler-compatibility glue files (the `glabel`/`dlabel`/
`INCLUDE_ASM` macro definitions, GTE instruction macros for GAS) to match whatever splat
version is currently running — confirmed by precedent, the project's own commit history shows
the "switch to maspsx + current splat" commit touching these exact same files. Since this
environment's `splat` (0.41.0) is newer than whatever was pinned when the repo was last
touched (2024-09), the regenerated versions differ from what's committed — that's expected,
not a mistake to revert. These files only affect assembler directive syntax (`.type`/`.size`/
`.ent`/`.end`, and which literal `.s` file gets `.include`d) — not actual instruction bytes —
so this shouldn't affect byte-exact matching. Leave them as splat regenerates them.

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

Full detail and rationale in `exchange/00-progress-checkpoint.md`'s Phase B section. Summary:

1. `text.c` was missing 888 bytes of `.rodata` (a string pool of world-map/job-class names +
   two pointer tables) — caused a uniform 888-byte shift through the entire rest of the file.
   **Placement is order-sensitive**: had to go at the very top of the file (right after
   `#include`s), not the bottom — unreferenced top-level globals get emitted before a later
   function's switch-statement jump table, regardless of which comes first/last textually
   relative to *functions*, but top-level declaration order among *themselves* still matters.
2. `src/cd.c`'s `Movie_Start`: `CdRead2(CdlModeStream | CdlModeSpeed | CdlModeRT)` computes
   `0x1e0` from the real, verified PsyQ macros, but the original binary uses `0x1c0` — fixed
   with a literal constant + comment; no clean macro combination produces `0x1c0`.
3. `text.c` was also missing 232 bytes of `.sdata` (more name strings). **This had to be 29
   separate `static u8 D_XXXXXXXX[8]` declarations, not one array** — this Makefile's `-G8`
   only routes objects ≤8 bytes to `.sdata` automatically; anything bigger silently lands in
   `.data` at the wrong address instead. Also must **not** be `const` (`.sdata` is writable
   small data; `const` data goes elsewhere).
4. The `assets/*.inc` files from finishing Phase A (see above) were *not* wrong — verified
   clean by the full bisection.

Both `text.c` additions are marked `TODO` in the source — byte-exact but not semantically
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
- `SLUS_004.47`'s correct MD5 is `596bb082a2de5f1fe977dd3d7e160b03` (matches `README.md`) —
  verified 2026-07-10 against the user's own CHD.
- If `src/text.c`'s two `D_` placeholder blocks (see above) are ever removed/"cleaned up" by
  a future refactor without replacing them with an equivalent byte-exact structure, that will
  reintroduce this exact mismatch — don't delete them as "unused code" without checking.
