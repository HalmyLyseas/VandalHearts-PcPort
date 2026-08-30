# Startup & bootstrap

On real hardware there is no startup step: the disc is in the drive and the TV is already showing VRAM
at power-on, so nothing in the game calls an "initialise the console" API. The PC port has to do that
work *before* the game's own `main()` runs, and with no changes to any `src/` file. It uses
`__attribute__((constructor))` functions in `platform/pc/src/pc_bootstrap.c` — the same mechanism the
generated data-segment initialisers already use — so everything here runs before `main()` for free.

## Constructor order

Three constructors run, earliest first (constructor priority controls the order):

1. **`PC_LoadIniConfig`** (priority 101 — runs first). Reads `vandalhearts.ini` next to the executable
   so configuration is in place before anything reads it.
2. **`PC_ReservePsxRam`** (default priority). Reserves the fixed PSX RAM ranges.
3. **`PC_Bootstrap`** (default priority). Makes read-only data writable, installs the fault handler,
   mounts and validates the disc, and opens the window.

## `vandalhearts.ini` loading

`PC_LoadIniConfig` finds the executable's own directory (via `PC_GetExePath`, which is per-OS —
`/proc/self/exe`, `GetModuleFileNameA`, `_NSGetExecutablePath`) and reads `vandalhearts.ini` from
there. It parses plain `KEY=VALUE` lines, ignoring `[section]` headers and `;`/`#` comments, and for
each `VH_*` key pushes it into the environment **only if not already set** — so the precedence is
*environment variable › INI › built-in default*. It must run first because later steps read
`VH_NULL_FIXUP`, `VH_DISC_IMAGE`, `VH_SCALE`, etc. See [../configuration.md](../configuration.md).

## Reserving the PSX RAM ranges

`PC_ReservePsxRam` maps the exact PSX addresses as real, writable memory so hard-coded scratch-buffer
literals in `src/` are valid again:

- **`0x80000000`, 2 MB** — the KUSEG main RAM.
- **`0x1f800000`, 1 KB** — the Scratchpad ("fast RAM").

`mmap(MAP_FIXED_NOREPLACE)` on Linux, `VirtualAlloc` at the fixed address on Windows. Apple Silicon's
Mach-O layout reserves the low 4 GB, so macOS uses host-backed work buffers instead. Failure is
non-fatal (it only matters once a code path actually touches that literal). See
[../memory-safety.md](../memory-safety.md) for the full rationale.

Why these two ranges and nothing else: `src/` contains eight main-RAM literals (all anonymous scratch
space such as `src/core/cd.c`'s `gSoundSets` buffers, none aliasing a named symbol) plus the
Scratchpad use in `src/maps/unpack.c` (`pCache = (u8 *)0x1f800000`, indexed up to `cacheOfs & 0x3ff`,
i.e. the full 1 KB). Neither mapping has to coexist with the port's own globals at matching offsets —
it only has to be valid memory. `MAP_FIXED_NOREPLACE` (rather than `MAP_FIXED`) makes the mapping
fail loudly instead of silently clobbering whatever the PIE loader put there. Under AddressSanitizer
the region is explicitly unpoisoned: ASan's shadow does not know about a raw `mmap` outside its
allocator, so every write into the range would otherwise report as an "unknown-crash". Note that address 0 is deliberately
**not** mapped. The native Linux i386 fault handler can log and emulate its supported access forms;
macOS, Windows, and the default Linux x86-64 build instead depend on source-level guards and
terminate after diagnostics on an unknown low-address access.

## Making read-only data writable

`PC_Bootstrap` first calls `PC_MakeRodataWritable`, which makes the executable's read-only data
segments writable up front, so the game's in-place string-literal writes never fault. Per-OS:
`dl_iterate_phdr` + `mprotect` (Linux), a PE-section walk + `VirtualProtect` (Windows), and a dyld
section walk + `mprotect` (macOS). See [../cross-platform.md](../cross-platform.md).

## The fault handler and crash diagnostics

On POSIX, a `SIGSEGV`/`SIGBUS` handler (`sigaction` with `SA_SIGINFO`) provides crash diagnostics. Two
additional recovery paths exist on native Linux x86 hosts:

- a **low-address read** (`< 2 MB`) is emulated — decode the faulting instruction, zero the destination
  register, step over it — mirroring what reading from PSX RAM's start would have returned;
- a **read-only-data write** makes the page writable and retries the store.

Each emulated i386 site is logged once to `vh_null_reads.log`, so it can receive a proper source-level
fix. Low-read instruction emulation is implemented only for native Linux i386. It is **not
implemented** on macOS, Windows, or the default Linux x86-64 build; those targets rely on `PC_PORT`
guards and normalized host data, and an unknown low read remains a real crash. The read-only-data
retry works on native Linux x86-32/x86-64, while all three supported OSes also perform a proactive
startup remap. Windows compiles out the POSIX handler entirely.

### Handler mechanics

- **Installation.** `sigaction` with `SA_SIGINFO | SA_NODEFER | SA_ONSTACK` on a dedicated 64 KB
  `sigaltstack`. Without the alternate stack a stack overflow (or a wild jump with a garbage `RSP`)
  cannot even enter the handler — the kernel has nowhere to push the signal frame — and the process
  dies with no dump and no core.
- **Re-entry guard.** `s_crashHandlerActive` is set on entry; a fault *inside* the handler (typically
  `backtrace()` walking a corrupted stack) writes a one-line breadcrumb with `write(2)` and `_exit`s
  instead of recursing. Everything on the crash path is async-signal-safe (`write`,
  `backtrace_symbols_fd`, a hand-rolled hex printer) — no `printf`.
- **Low-address decode (i386 only).** `VhDecodeMemAccess` covers what `gcc -O0` emits for
  struct-field loads/stores: `mov` / `movzx` / `movsx` (opcodes `8A/8B/88/89/C6/C7`, `0F B6/B7/BE/BF`)
  with the `0x66` operand-size prefix and full ModRM/SIB/disp addressing. A load zeroes the
  destination register (8-bit forms honour the AH/CH/DH/BH high-byte encoding); a store is discarded;
  `EIP` is advanced past the instruction. An undecodable form falls through to the normal crash dump
  with a message naming the decoder, so it can be extended. The 2 MB threshold is
  `PSX_NULL_MIRROR_SIZE` — the size of PSX main RAM, which KUSEG address 0 mirrors on hardware.
- **Read-only-data retry (x86-32 and x86-64).** `PC_IsWriteFault` reads bit 1 of the x86 page-fault
  error code (`REG_ERR`, identical layout on both widths). This path is deliberately *not* inside
  the `__i386__` gate: the 64-bit build still needs it as a safety net for any literal write the
  startup remap missed. `PC_MakePageWritable` dedups by page, which also prevents an infinite retry
  loop when `mprotect` succeeds but the store still faults.

## Mounting and validating the disc

1. The disc path comes from `VH_DISC_IMAGE`, or from `DefaultDiscPath()` which auto-detects a `*.bin`
   in a `game/` folder inside the deploy directory, then directly in it, then the dev-layout fallback
   (`external/game/` three levels up from `platform/pc/build*`). Each candidate is validated by
   reading 8 bytes at *this region's* boot LBA (`VH_REGION_BOOT_LBA`: 23 US/Asia, 15200 JP; sector
   data starts at byte 24 of a 2352-byte raw sector) and comparing against `PS-X EXE`. Multi-track
   dumps hold several `.bin` files and only the data track carries the header, and a folder holding
   both regions' dumps auto-picks the build's own disc.
2. `PC_CdMount` opens it; `PC_CdDiscSignatureOk` checks for the `PS-X EXE` boot signature at the
   region's boot LBA (see [subsystems/cd-xa.md](subsystems/cd-xa.md)).
3. **Truncation guard.** A raw `.bin` is a whole number of 2352-byte sectors; an interrupted copy or
   download almost never is. `PC_CdImageBytes() % 2352 != 0` is reported as an incomplete image rather
   than left to become a silent boot hang (the loader retrying garbage forever).
4. If the file can't be opened, or the signature is missing (wrong game/region, or a `.cue`/`.iso`),
   `PC_FatalDiscError` prints a clear message — **and pops a message box** (Win32 `MessageBox`, or
   SDL's dialog elsewhere, which works before `SDL_Init` and no-ops under the headless dummy driver)
   for double-click users with no console — then exits, rather than booting into a blank window. A
   missing signature is first classified with `PC_CdDiscRelease` (which probes both regions' layouts)
   so a valid disc of the *other* region gets a message naming what was supplied.
5. The mounted release is named on the console (USA `SLUS-00447`, Asia `SCPS-45183`, Japan
   `SLPM-86007`). USA and Asia share one master and both run on the US core; a disc that boots with
   this region's signature but is not a recognized release still boots, with a warning.

Only after a valid mount does `PC_GpuInit` open the SDL2 window and host presentation backend
(Metal on macOS, OpenGL elsewhere).

## Crash diagnostics (POSIX)

Two more diagnostics exist on POSIX, both off the running path:

- **`SIGUSR1`** (`kill -USR1 <pid>`): dumps the current call stack + game state to `stderr` without a
  debugger — for diagnosing a freeze (a state stall) in a running process.
- **On a real crash** (`SIGSEGV`/`SIGBUS` that isn't a handled NULL/rodata case): dump the backtrace +
  `gState` before dying, so the window doesn't just vanish. The Linux build links `-rdynamic` so the
  backtrace carries symbol names; `make crash-trace` maps the addresses back to `file:line`.

These are developer aids and, like the fault handler, are compiled out on Windows.

## Writing a setting back to `vandalhearts.ini`

`PC_SaveIniConfig(section, key, value)` is the overlay's persistence path: it rewrites exactly one
key's line and keeps the rest of the file byte-for-byte (comments, blank lines, section order, and any
inline comment on the key's own line). Two passes over the file keep the three cases unambiguous:

1. **Key already present** (active or commented out, in any section) — replace that line in place,
   preserving its inline comment. Highest priority, so a stray value is never duplicated.
2. **Key absent but its `[section]` exists** — insert the line at the end of that section (before
   the next header, or at EOF), so it joins the section rather than spawning a new one.
3. **Neither** — append a fresh `[section]` header and the line; this is also the no-file case.

A key line is recognised as an optional `;`/`#` marker, optional whitespace, the key, optional
whitespace, then `=`. Sections are cosmetic for the loader (keys are the `VH_*` names directly); the
section argument only decides where a new line lands. The rewrite goes through a `.tmp` file and
`rename`; on Windows the old file is removed first because MinGW's `rename` won't clobber a target.

## The unified binary

`make unified` (and the CMake/MinGW equivalent) produces one executable carrying **both** region
cores as prefix-renamed blobs (`us_*` / `jp_*`). Each blob is that region's complete,
individually-validated object set — game code, generated/reconstructed data, and region-compiled
backends — partial-linked and then symbol-renamed, so only one of them ever executes per process.
`platform/pc/src/pc_region_main.c` is the thin shared layer and the binary's real `main()`.

Under `VH_UNIFIED` the three bootstrap constructors in `pc_bootstrap.c` are compiled out and
`pc_region_main.c` calls the pieces explicitly, in the same relative order the constructor priorities
give in a single-region build:

1. **GPU-trace replay hook** (`VH_GPU_REPLAY`, the regression harness) — before anything else; no disc,
   no region, exits when done.
2. **Once-per-process, region-neutral init** via the US blob's copies: `us_PC_LoadIniConfig` (so
   `VH_REGION` / `VH_DISC_IMAGE` from the ini are visible to the scan), then `us_PC_ReservePsxRam`.
   The generated data-init constructors (the `generated_data` memcpys and pointer-table fixups) still
   run in *both* blobs at load; they touch only their own renamed globals.
3. **Disc discovery and classification.** `VH_DISC_IMAGE` pins the path but is still classified;
   otherwise the launcher scans `game/` inside the deploy directory, the deploy directory itself, and
   the dev layouts `external/game` and `external/alt`. The first disc found per release id is kept.
4. **Dispatch**: `<region>_PC_BootstrapRegion(path)` then `<region>_main()`.

### Classifying a disc

The launcher reads the 14-byte memory-card id string embedded in each region's boot executable. The
boot exe is pinned at the region's boot LBA in contiguous ISO sectors, so a VRAM address inside it maps
to an image offset as `(vram - 0x80010000) + 0x800` (the 2 KB PS-X EXE header), then to a raw byte
offset via `lba * 2352 + 24 + (offset % 2048)`:

| Disc | Boot LBA | Card id VRAM | Card id |
|---|---|---|---|
| USA `SLUS-00447` | 23 | `0x800f5551` | `BASLUS-00447VH` |
| Asia `SCPS-45183` | 23 | `0x800f5551` | `BISCPS-45183VH` |
| Japan `SLPM-86007` | 15200 | `0x800f76a9` | `BISLPM-86007VH` |

The same math lives in `libcd.c`'s `PC_CdDiscRelease`; the launcher carries its own copy because the
blobs' functions are renamed per region and this scan runs before either region is chosen.

### Region selection

`VH_REGION=auto|us|jp` (environment or `vandalhearts.ini`). `auto` boots whichever region's disc is
present; with both present the US/Asia disc wins and a hint naming the override is printed. Within the
US family, `VH_DISC_ID` (`SLUS-00447` or `SCPS-45183`) picks the exact disc; otherwise SLUS is the
default. SLUS and SCPS are the same master and core but *distinct* discs, so the inventory keeps a
separate slot for each. The launcher publishes the inventory to the blob about to run through
launcher-owned environment variables (always overwritten, never user config): `VH_DISC_ID_US`,
`VH_DISC_ID_ASIA`, `VH_DISC_ID_JP` name each disc found, and `VH_DISC_BOOTED` names the one chosen —
the in-game options overlay's **DISC** row lists them and persists a switch as `VH_REGION` +
`VH_DISC_ID` (see [../configuration.md](../configuration.md)).

### Region identity of a single build

`platform/pc/include/pc_platform.h` centralises everything region-derived so no card id, pack id or
boot LBA is written twice. The build defines `VH_REGION_JP` for the JP core; the header then selects
`VH_REGION_NAME`, `VH_ACTIVE_CARD_NAME` (the memory-card file name), `VH_HD_GAME_ID` (the HD-pack
subfolder), `VH_REGION_BOOT_LBA` and `VH_REGION_DISC`. The JP values are read from the byte-exact
`SLPM_860.07` (card id at VRAM `0x800f76a9`, boot exe at LBA 15200).

## Force-included headers

The game sources compile unmodified, so a few host-side headers are forced in ahead of every
translation unit rather than included by the sources:

- **`pc_forward_decls.h`** (`-include` from the Makefile) declares the handful of functions that are
  called before their definition in the same unit *and* return a pointer. Under `-std=gnu89` an
  undeclared call is implicitly `int`; on a 64-bit host that truncates the returned address the
  moment a caller stores it (`Krom2RawAdd`, `FindUnitByNameIdx`, `FindUnitSpriteByNameIdx` — the only
  three pointer-returning cases among ~170 implicitly-declared calls). The old GCC 2.x PSX toolchain
  never needed the prototype; modern GCC makes the later mismatched definition a hard
  "conflicting types" error. See [../width-bugs.md](../width-bugs.md) for the class.
- **`cmake_game_prelude.h`** is the CMake build's equivalent of the Makefile's two flags
  `-D'asm(x)='` and `-include pc_forward_decls.h`: CMake per-source `COMPILE_OPTIONS` drop a
  function-like macro and de-duplicate repeated `-include` flags, so one prelude header carries both.
  `#define asm(x)` neutralises the two `register T v asm("reg")` MIPS register-binding hints
  (`spells/support_magic.c`, `maps/unpack.c`); real `__asm__(...)` blocks are untouched. The macOS
  build additionally pre-includes the core headers plus `apple_void_forward_decls.h`.
- **`pc_win_compat.h`** (Windows build only, from `CMakeLists.txt`) supplies the BSD
  `u_char`/`u_short`/`u_int`/`u_long` aliases that glibc defines under `__USE_MISC` and MinGW's
  `<sys/types.h>` lacks; the clean-room PsyQ headers use them for register and packet fields. `u_long`
  is `unsigned long`, 32-bit under Win64's LLP64 model — the same width as the PSX `long`, so packet
  layouts hold.
- **`types.h`** mirrors the project's own `include/types.h` (`s8`..`u64`, `f32`, `f64`) for the
  clean-room PsyQ headers, but takes `int8_t`..`uint64_t` from `<stdint.h>` instead of retyping them:
  the PC build is not `-nostdinc`, and a local `typedef char int8_t` is a distinct type from the host's
  `signed char` — a hard redeclaration conflict wherever SDL2/OpenAL/glibc headers are also visible.
- **`PsyQ/stdio.h`, `strings.h`, `memory.h`, `sys/file.h`** stand in for the SDK's minimal
  `-nostdinc` headers and forward to the host libc. `sys/file.h` defines its own `O_RDONLY`..`O_CREAT`
  bit values: the game only uses them symbolically (`core/card.c`), never serialised or compared to a
  constant, so Sony's values are neither needed nor reproduced.

### `region-jp/` headers

The JP core compiles from the US `src/` tree wherever the two regions' units are byte-identical after
stripping the PC gates (`make check-shared`). A header that differs in content between regions gets a
`platform/pc/include/region-jp/` variant: the US header (which carries the `PC_PORT`/`PC_FEAT` gate
blocks the gate-free `jp/include/` copy lacks) with the genuine JP differences applied. When the US
header changes, the variant is re-merged by hand; `field.h` is deliberately *not* in the shared list,
so `check-shared` only catches drift indirectly.

`region-jp/field.h` carries one content difference — `gTerrainBonus` is `u16` in JP, `s16` in US —
plus everything the US header has:

- **`GRID_COLOR_PURPLE` / `GRID_COLOR_ORANGE`** (`PC_FEAT`, threat overlay) are macros rather than
  `GridColor` enum members so the matching build's enum is byte-for-byte unchanged.
- **`gTravelAscentCost` / `gTravelDescentCost` outer dimension.** Hardware dimensions are `[14][20]`,
  indexed `[stepType][elevationDiff]` in `battle/path_grids.c`. `PopulateMovementGrid` reads each
  neighbour's terrain and elevation *unconditionally* and only afterwards rejects boundary tiles via
  `terrain >= 0`, so every move-flood reaching the playable-area edge reads the off-map neighbour's
  elevation (a `diff` of ~126), computes `gTravelAscentCost[stepType][126]`, and discards it. On
  hardware the four travel tables are contiguous (`Ascent 0x800fc110 | Descent 0x800fc228 |
  gGfxSubTextures 0x800fc340`), so the overread lands in real `Descent` bytes (almost always 255). In
  the PC build they are independent globals separated by an 8-byte alignment gap, so the same overread
  returns *different* bytes — 112 `(stepType, diff)` combinations flip 255→0 — and it is a genuine
  out-of-bounds access under ASan. The `PERMUTER`-gated fix widens the **outer** dimension to
  `[20][20]` (400 bytes): the data-segment generator extracts `sizeof` bytes from each symbol's real
  VRAM address, so the extra rows hold the genuine contiguous `Ascent+Descent(+gGfxSubTextures)`
  image and every overread is byte-identical to hardware. 400 bytes covers the worst case (stepType
  13, `s8`-max diff 127 → linear index 387). The inner dimension (stride 20) is never changed — the
  address math `base + stepType*20 + diff` depends on it. Every `gUnitInfo[].step` is 0..13, so
  stepType itself never overruns. The `PC_DEBUG_PATH_STEP` probe remains as a tripwire for any larger
  diff.
