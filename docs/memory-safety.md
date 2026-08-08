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
   of RAM; on a host, address 0 is an unmapped page → `SIGSEGV`.
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
  so the macOS port uses host-backed storage for the remaining scratch/work buffers.
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

ASAN's blind spot, worth knowing: a buffer that is a *slice* of a larger array
(`gCardFileBufferPtr = &gScratch1[64]`) has no redzone between the slice and the rest, so overruns
*within* the parent object are invisible to it.

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

### Struct-width diffing — what neither sanitizer sees

Serialized structures that contain pointers are a distinct hazard, and one bit here. The in-battle
save writes a `UnitStatus`, whose size changes between build widths (120 bytes at the PSX/`-m32`
layout, 136 at LP64, because it embeds two live pointers). Left naive, a save written by one build
couldn't be read by the other — and **ASAN cannot see this** (it's a layout difference, not an
out-of-bounds access). It was caught by diffing `sizeof` between the 32- and 64-bit builds
(`tools/struct_width_diff.sh`) rather than by a sanitizer, and **fixed**: the in-battle save is now
serialized to a fixed 120-byte PSX on-disk layout (`PC_PORT`-gated `Pc_PackInBattleSave` /
`Pc_UnpackInBattleSave` in `src/card.c`), so saves are architecture-agnostic and cross-loadable
between build widths. The `sizeof`-diff method stays in the toolkit for the *next* such struct. See
[pc-port/subsystems/kernel.md](pc-port/subsystems/kernel.md) for the save-format detail.

## Tooling summary

| Tool | What it catches | Constraint |
|---|---|---|
| `make asan32` + `run_asan.sh` | out-of-bounds on globals/heap | 32-bit only (shadow vs arena) |
| `make ubsan` / CMake `VH_SANITIZE` | bounds, null/UB when selected | works at 64-bit; needs runtime coverage |
| `tools/struct_width_diff.sh` | layout drift in serialized structs | the class sanitizers miss |
| `vh_null_reads.log` | NULL-region reads / rodata writes the remap missed | appears only on a real fault |
