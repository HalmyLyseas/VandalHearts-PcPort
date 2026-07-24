# Kernel, events and timing (libapi / libetc)

The PS1 BIOS and the PsyQ `libapi`/`libetc` layer give the game everything that isn't a hardware
coprocessor: the interrupt-driven **event system**, the free-running **root counters** (hardware
timers), the **VSync** frame clock, **pad** input, and the **memory-card** file services. The game
reaches all of it by name — `OpenEvent`, `TestEvent`, `GetRCnt`, `VSync`, `PadRead`, `FileOpen`, … —
so on PC those names resolve to two portable C files instead of Sony's asm:

- [`platform/pc/src/libkernel.c`](../../../platform/pc/src/libkernel.c) — events, root counters, the
  memory-card/save file layer, `Krom2RawAdd` (BIOS kanji glyph lookup) and the BIOS `rand()`.
- [`platform/pc/src/libetc.c`](../../../platform/pc/src/libetc.c) — `VSync()` frame pacing and
  `PadRead()` input, both backed by SDL2.

Two clean-room headers define the interface with the exact signatures and constants the byte-exact
decompile recovered: [`PsyQ/kernel.h`](../../../platform/pc/include/PsyQ/kernel.h) (`OpenEvent`,
`EvSpIOE`, `HwCARD`, `RCntCNT1`, …) and [`PsyQ/libetc.h`](../../../platform/pc/include/PsyQ/libetc.h)
(`VSync`, `PadRead`, the `PADxxx` bit layout).

## The event system

On real hardware a BIOS *event* is a slot describing "when interrupt source *class* raises condition
*spec*, do *mode*". Code arms an event with `OpenEvent(class, spec, mode, handler)` + `EnableEvent`,
then either polls it with `TestEvent` or blocks on `WaitEvent`. Vandal Hearts uses events for exactly
one thing — memory-card completion — and only ever calls `OpenEvent` / `EnableEvent` / `TestEvent`
(see [`src/card.c`](../../../src/card.c)); it never calls `DeliverEvent` or `WaitEvent`, so the
backend implements only the three functions the game actually uses.

The model in `libkernel.c` is deliberately minimal: a fixed `Event[16]` table (`MAX_EVENTS`), each
entry holding `{class, spec, enabled, signaled}`.

- `OpenEvent` allocates the next slot and records `class`/`spec`; `mode` and the handler are ignored
  (nothing here delivers to a callback).
- `EnableEvent` flips `enabled`.
- `TestEvent` returns 1 exactly once per signal: if the event is enabled *and* signaled it clears the
  flag and returns 1, else 0 — the auto-clear matching the BIOS's "consume on read".

Nothing is signaled by a real interrupt, so the backend raises events itself through an internal
`SignalCardEvent(spec)` that marks every `HwCARD`/`SwCARD` slot with a matching `spec`. **Signalling
is not optional plumbing — it is load-bearing.** `card.c`'s `Card_WaitForSwCardEvent()` /
`Card_WaitForHwCardEvent()` are `while (1) { if (TestEvent(...)) return; }` busy-loops that only exit
when a card event fires. On hardware the async card BIOS calls (`_card_info`, `_card_clear`,
`_card_async_load_directory`) complete later and raise a `SwCARD`/`HwCARD` interrupt that signals the
event; the port's virtual card is synchronous, so those three stubs raise `EvSpIOE` (I/O-End —
success, value `0x0004`, *not* an error) inline before returning. Without that call the wait-loops
spin forever and the window freezes the instant New Game / Load Game touches the card. See
[Memory card and saves](#memory-card-and-saves).

## Root counters and VSync timing

**Root counters (`GetRCnt`/`ResetRCnt`).** The game reads one PS1 root counter, `RCntCNT1` (Timer 1
in HBlank mode) — a free-running position-within-the-frame clock. The backend approximates its rate
as `RCNT1_HZ = 15734.0` (the nominal-NTSC scanline rate, `53,693,181.818 / 3413 ≈ 15,732.7 Hz`,
within 0.008% — the rate was never the problem). `GetRCnt` returns `elapsed_seconds * RCNT1_HZ` from
a `CLOCK_MONOTONIC` timestamp taken at the last `ResetRCnt`; `ResetRCnt` restamps. This is honest
wall-clock, not cycle-accurate — see [Frame pacing](#frame-pacing-and-timing-fidelity) for the AI
throttle layered on top, and [Gotchas](#gotchas--notes) for the slow-host sprite-decoder caveat.

**VSync.** [`libetc.c`](../../../platform/pc/src/libetc.c)'s `VSync(mode)` is the frame clock. The
mode rule is faithful to the BIOS:

```c
if (mode < 0) return s_vblankCount;      /* query: report the running vblank count, no wait */
int waits = (mode == 0) ? 1 : mode;      /* mode 0 => 1 vblank; mode N => N vblanks */
```

So `VSync(0)` waits one vblank and `VSync(2)` waits **two** — i.e. a 30 FPS cadence. That is
authentic, not a bug: battle scenes set `gState.vsyncMode = 2` and legitimately run at 30 FPS (one
logic tick = two hardware vblanks), while menus/field use `VSync(0)` at 60. Any real-hardware frame
comparison during battle must divide BizHawk's raw vblank counts by 2 first.

The wait itself paces against a **fractional 59.94 Hz accumulator** rather than a fixed integer
delay. `FRAME_MS_F = 1000.0 / 59.94 ≈ 16.683 ms`; each vblank adds `FRAME_MS_F` to a `double`
deadline `s_nextVBlankMs` and `SDL_Delay`s the integer millisecond gap to it. Because the deadline
carries the sub-millisecond remainder forward, some frames sleep 16 ms and some 17, and the average
lands on the true NTSC rate. The earlier `FRAME_MS = 1000/60` truncated to 16 ms → 62.5 Hz → the demo
ran ~4% fast (the uniform "everything a touch shorter than hardware" drift). If a frame runs long
(slow host / stall) the deadline resyncs to *now* instead of racing to catch up, so a hitch doesn't
make the game briefly sprint. After the wait, `VSync()` also pumps the XA-music and SEQ-music tickers
(`PC_CdXaUpdate` / `PC_SeqTick`), which is why audio streaming is frame-locked to the display clock.

## Frame pacing and timing fidelity

The port runs PS1 game logic on a host that is orders of magnitude faster, which distorts one
specific thing: the enemy-AI turn evaluation. `src/ai.c`'s `IsLagging()` is `GetRCnt(RCntCNT1) > 450`
— a real-elapsed-CPU-time budget checked from inside seven AI state machines
(`Objf570/400/401/402/403/404/589_AI_TBD`). On hardware the AI's grid computation is slow enough to
trip that budget repeatedly, so the work spreads across ~110 vblanks; on a modern host it never
trips, collapsing the same work into ~16 and — because a steady ~0.5 `rand()`/frame background
consumer runs on both platforms — dropping ~46 `rand()` advances per AI turn, which eventually
desyncs the shared RNG stream and flips discrete block/counter outcomes.

The fix keeps `GetRCnt` honest for every *other* caller and applies a synthetic per-visit tick cost
only to AI callers, identified by `__builtin_return_address(0)` against the compiled address ranges
of those eight functions (a PIE-correct `&Func` range table — the previous link-time-address model
never matched under the PIE binary). Each AI function accrues a calibrated `s_aiThrottleTicks[]` cost
per `GetRCnt` visit instead of wall-clock, immune to host speed by construction; the sprite decoder,
debug `FntPrint`, and everything else fall through to the plain wall-clock path unchanged. Costs were
calibrated in *game-updates* (30 Hz) against BizHawk traces.

This work is **largely settled and banked at sub-1%**: with the throttle plus the 59.94 Hz pacing
fix, the demo tracks hardware to ~0.35% over the full run and the RNG holds lockstep for the first
several turns. A residual per-class difference remains (ranged/caster undershoot melee by a few
frames), left as an optional per-checkpoint refinement — it is not a blocker.

> **Caveat for timing comparisons.** The demo scripts *turn order*, not *outcomes*: block/counter
> chance and target selection are still RNG-driven, so two runs can diverge in actual gameplay. Only
> compare runs that produced the same outcomes; a block-result difference is an outcome divergence,
> not a tick bug.

## Pad input

`PadRead(id)` in `libetc.c` returns the standard PS1 digital-pad word, with the `PADxxx` bit layout
from `libetc.h` (`PADLup = 1<<12`, `PADRdown = 1<<6`, `PADstart = 1<<11`, …). Two input sources are
OR'd together each call:

- **Keyboard** (SDL scancodes): arrow keys → D-pad; `W/A/S/D` → Triangle/Square/Cross/Circle;
  `Q/E` → L1/R1; `Return` → Start; `Space` → Select.
- **Game controller** (any SDL-recognised pad, opened lazily): D-pad and left stick → D-pad; face
  buttons by position (A/B/X/Y → Cross/Circle/Square/Triangle); shoulders → L1/R1; analog triggers →
  L2/R2; Start/Back → Start/Select. As a QoL extra the right stick maps to the camera shoulder
  buttons (rotate/zoom) for a twin-stick feel.

`PadRead(0)` packs both controller ports into one 32-bit value (port 0 low 16 bits, port 1 high 16 —
the game reads player 2 via `PadRead(...) >> 0x10`); only port 0 is mapped, so the high half stays
zero. `PadInit` is a no-op (SDL is already up); L3/R3 are unmapped because the game reads a digital
pad.

## Memory card and saves

Saves are **real files**, not a simulated card image. The virtual card is always present, and
card-path strings of the form `bu00:BASLUS-00447VH` are mapped to `saves/<name>` by stripping the
`device:` prefix (`LocalPath` / `StripDevicePrefix`). `InitCard` `mkdir`s the `saves/` directory;
`FileOpen/Read/Write/Seek/Close` are thin `fopen`/`fread`/`fwrite`/`fseek` wrappers over a small open
handle table, and directory enumeration (`firstfile`/`nextfile`) walks `saves/` with `opendir`,
reporting each entry's real size. `card.c`'s own block/capacity accounting is driven entirely by
those real file sizes, so this layer only supplies honest existence/size/read/write — no PS1 card
filesystem format is emulated. Saving in-game calls `Card_CreateFile` + `Card_WriteFileListing`,
which writes `saves/BASLUS-00447VH`; loading checksum-verifies and reads it back. Nothing needs to be
pre-created.

The async-completion **event signalling** described in [The event system](#the-event-system) is what
makes New Game / Load Game work at all: the three async card stubs raise `EvSpIOE`, driving
`Card_CheckState()` to "card ready" so the file ops proceed instead of the wait-loop spinning
forever. `StartCard` similarly raises `EvSpNEW` ("a card is present").

**Caveats.**

- `SAVE_DIR` (`"saves"`) is **relative to the launch cwd** — run the game from a consistent directory
  or saves appear to come and go.
- A garbage or stale file under `saves/` will (correctly) checksum-fail and surface as "CANNOT READ
  MEMORY CARD"; that is the game rejecting a bad save, not a backend fault. Delete the stray file.
- The in-battle save serializes `UnitStatus`, whose in-memory size *is* pointer-width-sensitive
  (120 bytes at ILP32, 136 at LP64 — it embeds two live runtime pointers, so LP64 opens an 8-byte
  hole plus tail padding). Left naive, a 64-bit build would write a differently-sized blob than a
  32-bit one and the two couldn't share saves. That is **fixed** (`PC_PORT`-gated
  `Pc_PackInBattleSave`/`Pc_UnpackInBattleSave` in `src/card.c`): the on-disk record is always the
  **fixed 120-byte PSX layout**, packed field-by-field around the pointer hole, with the checksum over
  that fixed size — so **in-battle saves are architecture-agnostic**, cross-loadable between the 32-
  and 64-bit builds (regular saves and the file listing contain no `UnitStatus` and were never
  affected). See [`../../memory-safety.md`](../../memory-safety.md) for the width-bug *class* and how
  it was found (`sizeof` diff between the 64- and 32-bit builds — the detection method, which stays
  valuable even though this instance is resolved).

## Gotchas / notes

- **`VSync(2)` is 30 FPS on purpose.** Don't "fix" it to 60 — battle logic ticks once per two
  vblanks by design. Convert BizHawk vblank counts by `/2` before comparing battle timing.
- **`GetRCnt` is shared, and scaling the primitive is a trap.** A single global multiplier on
  `RCntCNT1` was tried and reverted: `IsLagging()` needs it slowed, but `src/graphics.c`'s
  incremental sprite decoder gates its per-frame decode budget on the same counter and gets *starved*
  by the same scale (corrupted geometry, infinite demo loop). The per-caller synthetic model exists
  precisely so these two consumers never share a fudge factor.
- **Slow-host sprite-decoder starvation.** The decoder gates on `GetRCnt(RCntCNT1) <= 470` during
  enemy turns; on hardware a frame is 16.7 ms so the counter can't reach 470, but a slow host
  (e.g. a 12 FPS 32-bit ASan build) passes 470 mid-frame, gating decode off every frame and hanging
  the enemy turn. The opt-in `VH_RCNT1_NORMALIZE=1` makes `RCntCNT1` frame-relative (scales elapsed
  time by nominal-frame / previous-frame duration, clamped to ≤ 1.0), so the counter sweeps the same
  0..~262 range per frame regardless of host speed. **Off by default** — it must not perturb the
  calibrated AI-timing work.
- **`rand()` is the PS1 BIOS LCG, not glibc.** `libkernel.c` reimplements BIOS A(2Fh):
  `seed = seed*0x41C64E6D + 0x3039; return (seed>>16) & 0x7FFF`, seeded to **0** (the game never
  calls `srand`; the hardware default is 0, confirmed empirically). Using glibc's `rand()` produces
  a deterministic but *different* stream, which scrambles per-demo unit team assignments and breaks
  the AI — the algorithm has to match exactly.
- **`Krom2RawAdd` returns `void *`, not `s32`.** It hands back a pointer into the extracted BIOS
  kanji glyph table (or `-1` on an unmapped code). It was once typed `s32` "to match the 32-bit
  pointer width", which truncated under LP64 and crashed the first battle menu; `void *` is correct
  at both widths and needs no `src/` change.
