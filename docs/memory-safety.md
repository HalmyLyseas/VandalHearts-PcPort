# Memory safety & the 64-bit port

The PlayStation has **no memory protection**. Its 2 MB of RAM is flat and always writable; address 0
is real RAM, not a trap; and there is no distinction between read-only and writable data. Byte-exact
game code relies on all of this, in ways that are perfectly safe on the console but fault instantly on
a modern host. Making the port survive — first at 32-bit, then at 64-bit, then unprivileged, then under
sanitizers — is the subject of this page. It corresponds to Stage 2.2 (memory safety) and Stage 2.3
(64-bit) of the roadmap.

## The three console-isms that fault on a host

1. **Transient NULL / low-address reads.** Some decompiled code dereferences a pointer for a frame or
   two before it's assigned — e.g. reading `unitSprite->x1` while `unitSprite` is briefly
   `0x00000000` right after an object is created. On PSX that reads a few harmless bytes from the start
   of RAM; on a host, address 0 is an unmapped page → `SIGSEGV`. The same class shows up as a
   transient-NULL *write*: `src/ui/window.c`'s `Objf004_005_408_Window` teardown can have either of two
   local sprite pointers be NULL, but only one was guarded before two `functionIndex` writes — both are
   now guarded, matching a live sprite's teardown when neither is NULL. A related variant is a
   negative-index read: retail can read `gUnits[-1]` when the first highlighted target is a
   convoy/non-unit slot (3 sites in `src/ui/supplies.c`'s party-inventory, purchase and depot-transfer
   screens; 4 in `jp/src/ui/supplies.c`) — harmless garbage on PSX, a wild read on a host — guarded by
   skipping the read while the index is still `-1`. Guarding a **write** through a transient NULL is
   also behavior-exact, not just crash-safe: on PS1 a store through a null pointer is silently
   discarded, so a `PC_PORT`-gated `if (ptr)` before a write (e.g. `src/spells/hit_effects.c`'s
   `obj_s0->functionIndex = OBJF_NULL` teardown) reproduces retail's own silent-discard semantics
   rather than changing behavior.
2. **In-place mutation of string literals.** Code writes into what the compiler placed in read-only
   data (e.g. `ShowExpDialog` writing the EXP digits into a `"You got     "` literal). Fine on PSX
   (all RAM is writable); a write fault on a host.
3. **Hard-coded RAM addresses.** A few already-decompiled functions bake literal PSX addresses in as
   scratch buffers (`(void *)0x80140878`, `pCache = (u8 *)0x1f800000`), matching the real 2 MB memory
   map. Those numeric values aren't valid addresses in the port's process at all.

## How the port handles them

All of this lives in `platform/pc/src/pc_bootstrap.c`, run before `main()` via constructors. See
[pc-port/bootstrap.md](pc-port/bootstrap.md) for the startup sequence; the memory-safety-relevant
pieces are:

- **Reserve the real PSX RAM ranges.** On Linux and Windows the port maps the exact PSX addresses as real,
  writable memory — the 2 MB KUSEG RAM at `0x80000000` and the 1 KB Scratchpad at `0x1f800000` — so
  every hard-coded literal is a valid buffer again, exactly as on hardware. `mmap(MAP_FIXED)` on
  Linux, `VirtualAlloc` at the fixed address on Windows. Apple Silicon cannot map those low addresses,
  so the macOS port uses host-backed storage for the remaining scratch/work buffers. The two regions
  don't always agree on which route to take: `src/maps/unpack.c`'s map-file decompressor keeps its
  1 KB dictionary ring at the hardware Scratchpad address and relies on the mapping, exactly like the
  rest of this list — but the US tree additionally carries a `PC_PORT` gate that swaps in a static host
  buffer of the same size instead, a small extra step toward not depending on the low-address mapping
  at all, taken opportunistically rather than everywhere at once.
- **Make read-only data writable at startup.** Rather than trap every literal write, the port makes
  the executable's read-only data segments writable once, up front. This is per-OS: `dl_iterate_phdr`
  + `mprotect` on Linux; a PE-section walk + `VirtualProtect` on Windows; and a dyld section walk +
  `mprotect` on macOS (see [cross-platform.md](cross-platform.md)).
- **Platform-specific fault diagnostics and recovery.** The POSIX `SIGSEGV`/`SIGBUS` handler reports
  crashes. On native Linux i386 it can additionally emulate supported low-address reads as zero and
  step over them; on native Linux x86 it can retry a read-only-data write after changing protection.
  Each emulated i386 site is logged once to `vh_null_reads.log`, so it surfaces for a proper
  source-level fix instead of crashing.
  In a healthy run this file never appears. The low-read instruction fixup is currently implemented
  only for Linux i386; it is **not implemented** on macOS, Windows, or the default Linux x86-64 build.
  This distinction is load-bearing. The PC string-table constructor now normalizes its 12 retail NULL
  entries and entry-100 sentinel to a stable empty string, removing that known class without a signal
  handler. Other unguarded low-pointer paths still need explicit source guards on every target that
  lacks the i386 fixup.

The payoff of Stage 2.2 was removing every privileged crutch: earlier versions mapped page 0 (needing
`CAP_SYS_RAWIO` / `setcap`) and parsed `/proc`. The current build **runs unprivileged, no root, no
setcap**. Startup remapping removes the read-only-data-write faults, but does not replace Linux's
low-read instruction fixup on macOS or Windows.

## Going 64-bit (Stage 2.3)

The port was deliberately **32-bit for most of its life**. Decompiled source assumes the PS1's
32-bit-pointer layout in places, and at 64-bit those assumptions shift struct fields *silently*
instead of failing loudly — so `-m32` removed a whole class of confusion while the real rendering and
audio bugs were being found. Once those were fixed, the port was moved to 64-bit by default and
validated at both widths. The 32-bit build remains available as an A/B reference
(`make link M32=-m32 BUILD_DIR=build32`).

### `long` is the trap: LP64 vs LLP64

PSX `long` is **32-bit**. Linux (LP64) `long` is 64-bit; Windows (LLP64) `long` is 32-bit. Any struct
whose layout depended on `long` being pointer-sized would differ between platforms. The fix was to map
PSX `long` → `int` throughout the PC-owned PsyQ layer, so no struct depends on `long`'s width. This is
also why the Windows (LLP64) build "just works" once the Linux 64-bit build does.

### The width bugs — a class of their own

The transition surfaced ~10 distinct width-bug classes, and the important pattern is that **most were
invisible to static review *and* to both sanitizers** — they were found by building and running, by a
visual 32-vs-64 A/B, or by diffing `sizeof` between the widths. A representative sample:

| Bug | Symptom | Fix |
|---|---|---|
| A `long *` GTE out-param wrote 8 bytes into a 4-byte `src/` local | stack smash at 64-bit | map PSX `long`→`int` in the PC PsyQ layer |
| raw byte offsets into the `Object` union (`d.bytes[4]`) | wrong field read → SIGSEGV | `PC_PORT`-gated typed field access |
| union members aliasing `Object_Sprite.coords` | misaligned read → white blobs | gated alignment padding |
| `CopyObject` copied a hardcoded 96 bytes (32-bit `sizeof(Object)`) | level-up sprite animated but didn't lift | copy the real `sizeof(Object)` |
| the GPU OT link stored a truncated host pointer | corrupt ordering table | the token bridge ([subsystems/gpu.md](pc-port/subsystems/gpu.md)) |

The complete catalogue — all ~10 classes, how each was found (and why sanitizers missed most of them),
and the transferable audit lessons — is in **[width-bugs.md](width-bugs.md)**. Two lessons worth
carrying here:

- **A mis-sized-array *report* is not automatically a too-small *declaration*.** Before widening any
  array, prove which index is actually out of range against the byte-exact binary — sometimes the
  index is simply *wrong* (a real state bug), not the array too small.
- **Keep the 32-bit build as the oracle** (`make link M32=-m32 BUILD_DIR=build32`). The fastest
  localiser for the silent-value class is a side-by-side 32-vs-64 comparison.

## Finding what static review can't: the sweeps

### AddressSanitizer — 32-bit only

ASAN redzones globals and heap objects and faults at the offending instruction instead of one crash
per play session. It **must be 32-bit here**: at 64-bit its shadow memory (around `0x7fff8000`)
collides with the `0x80000000` PSX-RAM arena the port reserves. `make asan32` + `./run_asan.sh`.

A full-game AddressSanitizer playthrough (chapters 1/4/6 + the final battle + credits) found **7 real
out-of-bounds bugs** that static audit could not — the class where an index is simply wrong and only
the consequence changes with pointer width. All were fixed `PERMUTER`-gated and byte-exact (e.g.
`gClutIds[124]→128` — which clobbered adjacent state in the retail game too; `gWindowDisplayX/Y[16]→70`
— which was overwriting live XA audio state; several travel-cost / AI-grid tables). The full list is
in the commit history (search `PERMUTER` array-widenings) and `docs/width-bugs.md`.

`gClutIds`'s overrun has a concrete mechanism: `SetupGfx` (`src/states/game_setup.c`) runs a
`for i<8 { for k<16 }` loop — 128 `s16` writes — against the 124-entry array, overrunning 4 entries
into `s_cdSyncStatus` and its padding. It is benign in the retail game only by luck: `src/core/audio.c`
always reassigns `s_cdSyncStatus = CdSync(1, buf)` immediately before every read of it, so the
clobbered value is never observed. The widening is `PERMUTER`- (not `PC_PORT`-) gated because the
data-segment generator's `sizeof()` probe also compiles with `-DPERMUTER` and must agree with the game
code about the array's size — the same discipline as `sFontGlyphBitmaps[129]` vs `[128]` in
`src/core/text.c`.

`gWindowDisplayX/Y` are indexed by raw `windowId`; `DrawWindow` is the only writer, called with ids up
to 68 (a secondary path also writes `windowId + 1`). This is harmless on real hardware — the two
arrays sit back-to-back with an 88-byte unclaimed gap after them, so ids up to 68 land in that gap and
nothing else lives there. In the PC build the two arrays are independently placed 32-byte globals, so
the same out-of-range index instead lands on the next globals in link order — live XA volume/control
state, corrupted every time a window is drawn. Widened to 70, the same discipline as `gClutIds`; no
retail aliasing is lost, because `DrawWindow` is never called with the ids (20–36) where hardware's X
and Y arrays physically overlap.

`gTravelAscentCost`/`gTravelDescentCost` (`src/battle/path_grids.c`), `[14][20]` indexed
`[stepType][diff]`, have two independent overrun causes: `stepType > 13` (a garbage or uninitialized
unit entry — a real state bug elsewhere) and `|diff| >= 20` (a genuinely steep elevation step —
one boss's map reshapes the terrain into a tall pyramid, so adjacent-tile elevation differences really
can exceed 19). On hardware the two tables sit contiguous with the next table, so the overrun reads
real data instead of faulting; on the port the same read runs off the end of the declared array. The
`PC_DEBUG_PATH_STEP` probes (`PcPathStepProbe` / `PcTravelDiffProbe`) distinguish which of the two
causes triggered a given overrun.

`gAiCastValueGrid[iz][ix]` (`src/battle/ai.c`) is indexed with an *absolute* map-Z coordinate, not a
0-based row; tall maps push that coordinate past the array's declared `[27][64]`, up to row 28.
Hardware is safe only because the retail global has more real memory behind it than its declared size
implies — the next global in link order happens to sit far enough away that the extra row lands in
slack, not live data. The PC data-segment generator emits exactly the declared size, so the same
write there corrupts whatever comes next; the fix widens the array to `[29][64]`, covering the full
row range, with no effect on codegen since only the inner dimension enters the address computation.

`jp/src/states/main_menu.c`'s `sEmptyFileCaption` sits alongside `gClutIds`/`gWindowDisplayX/Y` in this
worked-example set: it's widened from an implicit 28 bytes to 29 so the string's NUL terminator lands
inside the object instead of relying on the next symbol's leading zero byte. The US tree needs no
equivalent fix, since its "Empty" literal is already long enough that the implicit size covers the
terminator.

ASAN's blind spot, worth knowing: a buffer that is a *slice* of a larger array
(`gCardFileBufferPtr = &gScratch1[64]`) has no redzone between the slice and the rest, so overruns
*within* the parent object are invisible to it.

#### Running the ASan build

Prerequisites: a 32-bit userspace for the two runtime libraries the port links at `-m32` — on Arch
the multilib/AUR packages `lib32-sdl2` (or `lib32-sdl2-compat`) and `lib32-openal`; there is no
32-bit libwebp/libav, so build with `make asan32 NO_WEBP=1 NO_HDVIDEO=1`. Without them the link
fails with 32-bit `pkg-config` lookups for `sdl2`/`openal`.

`make asan32` compiles with `-fsanitize=address -fsanitize-recover=address -fno-omit-frame-pointer`
(recovery so one playthrough reports every distinct site instead of aborting at the first; frame
pointers because the gnu89 game source is full of small static functions whose frames otherwise
vanish from the traces). `run_asan.sh` launches it from `platform/pc/` (the binary resolves the disc
and `saves/` relative to cwd) with `ASAN_OPTIONS` the port genuinely needs:

| Option | Why |
|---|---|
| `handle_segv=0` | `pc_bootstrap.c` installs its own load-bearing `SIGSEGV` handler (rodata-write retry, low-read fixup). ASan's default handler would intercept those faults and report every legitimate literal write as "SEGV on unknown address". ASan's real findings come from redzone instrumentation, not from `SIGSEGV`, so nothing is lost. |
| `halt_on_error=0` | report and continue (needs `-fsanitize-recover=address`). |
| `detect_leaks=0` | the port never frees anything by design (it emulates a fixed PSX memory map), so LeakSanitizer's exit dump would be thousands of lines of expected noise. |
| `log_path=asan.log` | reports go to `asan.log.<pid>`, surviving the SDL window closing and not interleaving with the game's own logging. |

The script also sets `VH_RCNT1_NORMALIZE=1`. That is not a workaround for a game bug but for one the
sanitizer creates: `src/core/graphics.c` gates its incremental sprite decoder on
`GetRCnt(RCntCNT1) <= 470` during enemy turns — 470 HBlank ticks is ~30 ms, unreachable on hardware's
16.7 ms frames. Under ASan a battle runs at ~12 fps (83 ms frames), the gate trips every frame,
`gDecodingSprites` never clears, and the enemy turn never starts (the camera pans forever). The option
makes RCnt1 frame-relative so the budget survives a slow host (`libkernel.c`, `ResetRCnt`); it is off
in normal builds so the calibrated AI timing is untouched. `VH_SCALE` is display-only (window size and
blit upscale) and cannot affect what ASan reports. Previous runs' reports are archived under
`asan_runs/`, never deleted — a sweep's value is the accumulated set.

### UBSan — works at 64-bit

UBSan has no shadow memory, so it runs at the default 64-bit (`make ubsan`, or the CMake
`-DVH_SANITIZE="-fsanitize=bounds …"`). On macOS, add `null` while hunting unknown low-pointer paths:

```sh
cmake -S platform/pc -B platform/pc/build_macos_ubsan \
  -DVH_SANITIZE="-fsanitize=null,bounds -fno-omit-frame-pointer"
cmake --build platform/pc/build_macos_ubsan
```

This produces a source-level diagnostic before the ordinary crash for instrumentable NULL
dereferences. It cannot prove that every scene is safe: coverage still depends on the battles,
cutscenes, menus, and save states exercised. It is also the bounds-checking pass that ASAN's arena
collision rules out at 64-bit on Linux.

`run_ubsan.sh` needs far less special-casing than the ASan script: UBSan installs no `SIGSEGV`
handler (the port's own stays in charge), has no LeakSanitizer, and `-fsanitize=bounds` is inline
compares rather than shadow lookups, so the game runs at near-normal speed and `VH_RCNT1_NORMALIZE`
stays off. If an enemy turn ever hangs with the camera panning, that slow-host symptom is back — rerun
with `VH_RCNT1_NORMALIZE=1`. `UBSAN_OPTIONS=halt_on_error=0:print_stacktrace=1:log_path=ubsan.log`
reports every site and keeps the logs (archived under `ubsan_runs/`). Its coverage limit: `-fsanitize=bounds`
only instruments accesses whose array bound is statically visible in that translation unit; anything
reached through a pointer (`gCardFileBufferPtr[...]`, the `gScratch1` slices) is not checked — the same
blind spot ASan has, for a different reason. Struct-width diffing covers that class.

### Struct-width diffing — what neither sanitizer sees

Serialized structures that contain pointers are a distinct hazard, and one bit here. The in-battle
save writes a `UnitStatus`, whose size changes between build widths (120 bytes at the PSX/`-m32`
layout, 136 at LP64, because it embeds two live pointers). Left naive, a save written by one build
couldn't be read by the other — and **ASAN cannot see this** (it's a layout difference, not an
out-of-bounds access). It was caught by diffing `sizeof` between the 32- and 64-bit builds
(`tools/struct_width_diff.sh`) rather than by a sanitizer, and **fixed**: the in-battle save is now
serialized to a fixed 120-byte PSX on-disk layout (`PC_PORT`-gated `Pc_PackInBattleSave` /
`Pc_UnpackInBattleSave` in `src/core/card.c`), so saves are architecture-agnostic and cross-loadable
between build widths. The `sizeof`-diff method stays in the toolkit for the *next* such struct. See
[pc-port/subsystems/kernel.md](pc-port/subsystems/kernel.md) for the save-format detail.

`tools/struct_width_diff.sh` needs both builds present (`make link` → `build/`, `make link M32=-m32
BUILD_DIR=build32` → `build32/`), reads every `typedef struct {...} Name;` from `include/*.h`, and
asks gdb for `sizeof` in each binary. Reading the output: a size difference is **not** automatically a
bug — anything holding a pointer legitimately grows. It becomes a bug only where the struct meets a
fixed byte size: serialization to a save file, a `memcpy` with a literal length, a preallocated
buffer. Triage each hit by asking "is this ever written to a file or copied with a hardcoded size?".
`Object_*` variants are union members of `Object`, runtime-only and already handled by the union-alias
padding in `include/object.h`, so they are listed separately and expected to differ.

## Tooling summary

| Tool | What it catches | Constraint |
|---|---|---|
| `make asan32` + `run_asan.sh` | out-of-bounds on globals/heap | 32-bit only (shadow vs arena) |
| `make ubsan` / CMake `VH_SANITIZE` | bounds, null/UB when selected | works at 64-bit; needs runtime coverage |
| `tools/struct_width_diff.sh` | layout drift in serialized structs | the class sanitizers miss |
| `vh_null_reads.log` | NULL-region reads / rodata writes the remap missed | appears only on a real fault |
