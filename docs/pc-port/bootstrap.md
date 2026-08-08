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
[../memory-safety.md](../memory-safety.md) for the full rationale. Note that address 0 is deliberately
**not** mapped — leaving it unmapped is what lets the fault handler log each transient NULL read.

## Making read-only data writable

`PC_Bootstrap` first calls `PC_MakeRodataWritable`, which makes the executable's read-only data
segments writable up front, so the game's in-place string-literal writes never fault. Per-OS:
`dl_iterate_phdr` + `mprotect` (Linux), a PE-section walk + `VirtualProtect` (Windows), and a dyld
section walk + `mprotect` (macOS). See [../cross-platform.md](../cross-platform.md).

## The fault handler (POSIX net)

On POSIX, a `SIGSEGV`/`SIGBUS` handler (`sigaction` with `SA_SIGINFO`) is installed as a safety net for
anything the startup passes miss:

- a **low-address read** (`< 2 MB`) is emulated — decode the faulting instruction, zero the destination
  register, step over it — mirroring what reading from PSX RAM's start would have returned;
- a **read-only-data write** makes the page writable and retries the store.

Each distinct site is logged once to `vh_null_reads.log`, so unguarded spots surface for a proper
source-level fix. On Windows this handler is compiled out entirely — the 64-bit build absorbs NULL
reads with `PC_PORT` source guards and needs no signal machinery. The instruction-decode NULL fixup is
x86-specific; the read-only-data retry works on any x86 (32- and 64-bit).

## Mounting and validating the disc

1. The disc path comes from `VH_DISC_IMAGE`, or from `DefaultDiscPath()` which auto-detects a `*.bin`
   in a `game/` folder next to the executable, then beside it, then the dev-layout fallback.
2. `PC_CdMount` opens it; `PC_CdDiscSignatureOk` checks for *Vandal Hearts (USA)*'s `PS-X EXE` boot
   signature at sector 23 (see [subsystems/cd-xa.md](subsystems/cd-xa.md)).
3. If the file can't be opened, or the signature is missing (wrong game/region, or a `.cue`/`.iso`),
   `PC_FatalDiscError` prints a clear message — **and pops a `MessageBox` on Windows** for double-click
   users — then exits, rather than booting into a blank window.

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
