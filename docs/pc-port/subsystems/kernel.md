# Kernel, events and timing (libapi / libetc)

The PS1 BIOS and the PsyQ `libapi`/`libetc` layer give the game everything that isn't a hardware
coprocessor: the interrupt-driven **event system**, the free-running **root counters** (hardware
timers), the **VSync** frame clock, **pad** input, and the **memory-card** file services. The game
reaches all of it by name — `OpenEvent`, `TestEvent`, `GetRCnt`, `VSync`, `PadRead`, `FileOpen`, … —
so on PC those names resolve to a few portable C files instead of Sony's asm:

- [`platform/pc/src/libkernel.c`](../../../platform/pc/src/libkernel.c) — events, root counters (and
  the AI throttle layered on `RCntCNT1`), the memory-card/save file layer, `Krom2RawAdd` (BIOS kanji
  glyph lookup) and the BIOS `rand()`.
- [`platform/pc/src/libetc.c`](../../../platform/pc/src/libetc.c) — `VSync()` frame pacing and
  `PadRead()` input, both backed by SDL2. Two companions split out of it, wired through the
  file-private seams in [`pc_etc_internal.h`](../../../platform/pc/src/pc_etc_internal.h):
  [`pc_battle_speed.c`](../../../platform/pc/src/pc_battle_speed.c) (the battle fast-forward state +
  gate) and [`pc_diag.c`](../../../platform/pc/src/pc_diag.c) (every `VH_*`-gated diagnostic
  logger/meter plus the `VH_SMOKE` boot harness, driven from `VSync()` via small hooks).
- [`platform/pc/src/libsn.c`](../../../platform/pc/src/libsn.c) — `PCcreat`/`PClseek`/`PCwrite`/
  `PCclose`, the devkit's "write a file on the connected host PC" link, which
  `states/game_setup.c` calls. They are real local file I/O (`open`/`lseek`/`write`/`close`), a
  faithful port of the workflow rather than stubs. `pollhost`/`PSYQpause` are never called.

Three clean-room headers define the interface with the exact signatures and constants the byte-exact
decompile recovered: [`PsyQ/kernel.h`](../../../platform/pc/include/PsyQ/kernel.h) (`OpenEvent`,
`EvSpIOE`, `HwCARD`, `RCntCNT1`, …), [`PsyQ/libetc.h`](../../../platform/pc/include/PsyQ/libetc.h)
(`VSync`, `PadRead`, the `PADxxx` bit layout) and [`PsyQ/libsn.h`](../../../platform/pc/include/PsyQ/libsn.h).
Several of these functions (`GetRCnt`, `ResetRCnt`, `OpenEvent`, `EnableEvent`, `Krom2RawAdd`, the
card file I/O) are declared in **no** real PsyQ header either — the retail build reaches them through
old GCC's implicit-declaration leniency, and `core/card.c` carries its own local `extern s32 ...`
declarations. The PC headers therefore declare them with `s32`/`u32` (not `int`) so they agree with
those local externs, and rely on `include/common.h` pulling in `types.h` first, as the game does.

## The event system

On real hardware a BIOS *event* is a slot describing "when interrupt source *class* raises condition
*spec*, do *mode*". Code arms an event with `OpenEvent(class, spec, mode, handler)` + `EnableEvent`,
then either polls it with `TestEvent` or blocks on `WaitEvent`. Vandal Hearts uses events for exactly
one thing — memory-card completion — and only ever calls `OpenEvent` / `EnableEvent` / `TestEvent`
(see [`src/core/card.c`](../../../src/core/card.c)); it never calls `DeliverEvent` or `WaitEvent`, so the
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
is not optional plumbing — it is load-bearing.** `core/card.c`'s `Card_WaitForSwCardEvent()` /
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
as `RCNT1_HZ = 15734.0`: the nominal-NTSC scanline rate is `53,693,181.818 Hz / 3413 video cycles per
scanline ≈ 15,732.7 Hz` (psx-spx timers + GPU chapters; BizHawk's octoshock `GPUClockRatio` /
`TIMER_ClockHRetrace` agree), and the constant lands within 0.008% of it. `GetRCnt` returns
`elapsed_seconds * RCNT1_HZ` from a `CLOCK_MONOTONIC` timestamp taken at the last `ResetRCnt`;
`ResetRCnt` restamps. This is honest wall-clock, not cycle-accurate. The game resets the counter
exactly once per frame, at the **end** of `UpdateEngine()` (`src/core/engine.c`) after object
execution and the sprite decoder have run — so on hardware the counter is a genuine
whole-frame-elapsed clock, and any code reading it mid-frame sees the cost of everything that ran
before it. Two consumers read it with very different needs:

- `src/battle/ai.c`'s `IsLagging()` (`GetRCnt(RCntCNT1) > 450`), the enemy-AI turn-evaluation
  budget, which **needs slowing** on a fast host — see [The AI throttle](#the-ai-throttle-a-synthetic-rcntcnt1-for-islagging-callers).
- `src/core/graphics.c`'s incremental unit-sprite decoder, which gates decode work off entirely once
  `GetRCnt(RCntCNT1) > 470` during an enemy turn and bounds each decode burst with
  `while (GetRCnt(RCntCNT1) <= maxTicks)`. It runs every frame, is nowhere near its budget on a
  modern CPU, and is **starved** by any global slow-down of the counter (corrupted, half-drawn
  geometry and a demo that never completes, because `gDecodingSprites` never clears).

That conflict is why the primitive is never scaled: one multiplier cannot serve both, so the AI gets
a per-caller synthetic counter and every other reader keeps plain wall-clock.

### Frame-relative `RCntCNT1` (`VH_RCNT1_NORMALIZE=1`, off by default)

The sprite decoder's `> 470` gate assumes a 16.7 ms frame: 470 ticks at `RCNT1_HZ` is ~30 ms, so on
hardware the counter only reaches ~262 by end of frame and the gate can never trip. A slow host
breaks that assumption — at ~12 FPS (an 83 ms frame, what a 32-bit ASan build gets in battle) the
counter passes 470 thirty milliseconds in, the decoder is gated off every frame during an enemy
turn, `gDecodingSprites` never clears, and the enemy turn never begins (the camera pans forever
while the render loop stays alive). The symptom is the same starvation as a synthetic multiplier,
reached through real slowness.

With `VH_RCNT1_NORMALIZE=1`, `ResetRCnt(RCntCNT1)` measures the gap since the previous reset — which
*is* the previous frame's duration, since the game resets once per frame — and `GetRCnt` scales
elapsed time by `nominal_frame / previous_frame` (`RCNT1_NOMINAL_FRAME_SEC = 1/59.94`). The counter
then sweeps the same 0..~262 range per frame regardless of host speed: frame-relative, which is what
the hardware counter physically is. The scale is clamped to ≤ 1.0 (it only ever compensates for a
host *slower* than 60 FPS; at or above target it is a no-op) and to ≥ `RCNT1_MIN_SCALE = 0.05` (a
20× cap so a pathological stall stays bounded). The counter must still advance monotonically within
a frame — the decode burst loop above would never terminate if the counter were pinned to a
constant. It stays **off by default** so the calibrated AI timing is undisturbed.

### VSync

[`libetc.c`](../../../platform/pc/src/libetc.c)'s `VSync(mode)` is the frame clock. The mode rule is
faithful to the BIOS:

```c
if (mode < 0) return s_vblankCount;      /* query: report the running vblank count, no wait */
int waits = (mode == 0) ? 1 : mode;      /* mode 0 => 1 vblank; mode N => N vblanks */
```

So `VSync(0)` waits one vblank and `VSync(2)` waits **two** — i.e. a 30 FPS cadence. That is
authentic, not a bug: battle scenes set `gState.vsyncMode = 2` and legitimately run at 30 FPS (one
logic tick = two hardware vblanks), while menus/field use `VSync(0)` at 60. Any real-hardware frame
comparison during battle must divide BizHawk's raw vblank counts by 2 first.

The wait paces against a **fractional 59.94 Hz accumulator**, not a fixed integer delay.
`FRAME_MS_F = 1000.0 / 59.94 ≈ 16.683 ms`; `SDL_Delay` only takes whole milliseconds, so a `double`
deadline `s_nextVBlankMs` carries the sub-millisecond remainder forward — some frames sleep 16 ms and
some 17, and the average lands on the true NTSC rate. (An integer `1000/60 = 16 ms` paces at 62.5 Hz,
~4% fast — a uniform "everything a touch shorter than hardware" drift across a whole demo.)

Each call advances the deadline by **one consolidated budget**, `frameMs * waits`, where `frameMs`
is `FRAME_MS_F` divided by the [battle fast-forward](#battle-fast-forward) factor (1 outside
battle). The single deadline matters: a per-vblank sub-wait loop that resyncs on each miss and then
sleeps the next sub-wait in full charges `work + sub-wait` per frame instead of `max(work, frame)` —
at `VSync(2)` under 2× fast-forward (two 8.3 ms sub-waits) that burns a constant ~8 ms of idle every
frame and caps ~53 fps with a 90+ fps work ceiling. With one deadline per call, sub-frame overshoot
is absorbed by the remaining budget; a miss keeps the deadline, so the catch-up is bounded to one
frame with no racing; and only when the call is **more than a whole budget late** — a genuine
hitch/stall — is the deadline resynced to *now*, so a stall never makes the game briefly sprint.

Around the wait, `VSync()` is also the port's once-per-tick hook point:

- **Boot, on the first call:** `PC_LangBoot()` (language pack) then `PC_BalanceBoot()` (Tactical-mode
  config default). The first `VSync` is the earliest point after the data-segment constructors, and
  the order is fixed: `pc_balance.c` lazily snapshots "pristine" tables the first time it touches
  them, so the pack must apply first — toggling Tactical off then restores the player's language,
  not retail English, on any entry both layers touch.
- **Before the wait:** `PC_GpuPumpEvents()`, so the compositor's responsiveness ping (and
  close/Ctrl+C) is answered even inside the game's own `VSync` spin loops — the hardware-exact boot
  load spins here for several seconds with nothing presented, which otherwise trips desktop
  "not responding" dialogs. Every wait loop in the game funnels through `VSync`, so one call covers
  them all.
- **After the wait:** the diagnostic CSV rows (`pc_diag.c`), the XA-music and SEQ-music tickers
  (`PC_CdXaUpdate` / `PC_SeqTick` — which is why audio streaming is frame-locked to the display
  clock), arming the battle-gated GPU-trace recorder from the same in-battle state set the
  fast-forward gate uses, the `VH_SMOKE` exit check and the `VH_FPS_LOG` meter.

## Frame pacing and timing fidelity

The port runs PS1 game logic on a host that is orders of magnitude faster, which distorts one
specific thing: the enemy-AI turn evaluation. `src/battle/ai.c`'s `IsLagging()` is `GetRCnt(RCntCNT1) > 450`
— a real-elapsed-CPU-time budget checked from inside seven AI state machines
(`Objf570_AI_ChooseAction`, `Objf400/401/402/403/404`, `Objf589_AI_MoveToEscapePoint`). On hardware
the AI's grid computation is slow enough to trip that budget repeatedly, so the work spreads across
~110 vblanks; on a modern host it never trips, collapsing the same work into ~16 and — because a
steady ~0.5 `rand()`/frame background consumer runs on both platforms — dropping ~46 `rand()`
advances per AI turn, which eventually desyncs the shared RNG stream and flips discrete
block/counter outcomes.

### The AI throttle: a synthetic `RCntCNT1` for `IsLagging()` callers

Reconstructing "real elapsed time" synthetically is intractable: because the counter is reset at the
end of the frame, a faithful model would have to cost `RenderField()`, every active object and the
sprite decoder — everything that runs before the AI — not just the AI functions. Instead the
throttle decouples the AI's view of the counter from host speed entirely (the idea behind
ctr-native's `GetRCnt`, which advances fixed ticks per vblank; `IsLagging()` needs sub-frame
resolution, so this keys on call count instead):

- **Caller identification.** `GetRCnt` reads `__builtin_return_address(0)` — supplied by the call
  instruction itself, no frame-pointer walking — and matches it against a table of
  `{&Func, size}` ranges covering `IsLagging` and the seven AI functions (`s_aiThrottleRanges[]`).
  The start of each range is a runtime `&Func`, so `retAddr - start` cancels the ASLR base and the
  match is PIE-correct; a table of literal link-time addresses never matches in a PIE binary. The
  *sizes* come from `nm -S build/src/ai.o` and go stale if any of those eight functions changes
  compiled size — regenerate with
  `nm -S --size-sort platform/pc/build/src/ai.o | grep -iE "objf(570|4|589)|islagging"`.
  `IsLagging()` is a real, non-inlined function whose only callers are those seven state machines
  (the straight-line case-boundary checks), so "called via `IsLagging()`" already implies "AI" and
  its ~24 sites share one bucket; the ~10 direct `GetRCnt(RCntCNT1)` calls in the per-tile scanning
  loops are attributed by the enclosing function's range.
- **Per-visit cost.** An AI caller never sees wall-clock. Each visit adds a per-function constant
  from `s_aiThrottleTicks[]` to a synthetic accumulator (`s_aiSyntheticTicks`, zeroed by
  `ResetRCnt` like the real counter) and returns that; the function yields after
  `ceil(450 / cost)` visits per update. Every other caller (the sprite decoder, the debug
  `FntPrint`) falls through to the wall-clock path untouched, so the two consumers never share a
  fudge factor.
- **Calibration.** Costs are flat per function and derived from measured visit counts against
  BizHawk frame targets: `cost_per_visit = 450 ticks × HW_target_updates / measured_visits_per_scan`.
  Targets are in **game updates** (30 Hz, `VSync(2)`) — the AI state machine steps once per update,
  so raw BizHawk vblank counts are halved first. The unmodelled pre-AI frame work is absorbed into
  the constant, and because both visits and target frames scale with map size, a flat per-function
  constant is approximately map-invariant. The current values: `IsLagging` and the `Objf570`
  orchestrator cost 5 (case-boundary checks must never gate); the `Objf401` threat map costs 285
  (about one visit per update, matching its 8-update hardware target); the `Objf403` move-scoring
  and `Objf402` caster-evaluation costs (30 / 42) come from a linear fit of measured-vs-target
  updates on the calibration demo's melee, ranged and caster turns; `Objf400/404/589` are not
  exercised by that demo and take the `Objf403` value. `g_aiVisitCount[]` (cumulative visits per
  range, always incremented, exported into the `VH_AI_LOG` chain CSV) is the measurement side of the
  formula: diff it across an AI phase to get `visits_per_scan`.

With the throttle plus the 59.94 Hz pacing, a scripted demo tracks hardware to ~0.35% over the full
run and the RNG holds lockstep for the first several turns. A residual per-class difference remains
(ranged/caster undershoot melee by a few percent); the refinement, if ever needed, is per-checkpoint
costs anchored to `&Func + delta` rather than one flat cost per function.

> **Caveat for timing comparisons.** The demo scripts *turn order*, not *outcomes*: block/counter
> chance and target selection are still RNG-driven, so two runs can diverge in actual gameplay. Only
> compare runs that produced the same outcomes; a block-result difference is an outcome divergence,
> not a tick bug.

### Battle fast-forward

[`pc_battle_speed.c`](../../../platform/pc/src/pc_battle_speed.c) implements the player-facing
fast-forward ([gameplay-additions.md](../../gameplay-additions.md#battle-fast-forward)) as a
**whole-tick speed multiplier**: `VSync()` divides its per-tick idle wait by the factor, so the game
runs N *complete* `UpdateEngine()` ticks in the wall-clock of one. Only the idle time between ticks
is compressed — never a fractional or skipped tick — so AI and RNG stay byte-identical and outcomes
never change, which is why it applies in both Normal and Tactical mode.

- **Gate.** `PC_InActiveBattle()` is true for exactly `core/main.c`'s `State_Battle()` dispatch set
  — `STATE_3`, `STATE_LOAD_IN_BATTLE_SAVE` (23), `STATE_27`, `STATE_30`, `STATE_31` — every primary
  state that runs a real-time battle tick (`STATE_30` is the ordinary map-entered story battle; a
  gate missing it makes R2 a silent no-op in normal play). Outside that set `PC_BattleSpeedGet()`
  returns 1, and `PC_BattleSpeedReset()` (called once per `VSync`) drops the speed back to 1× so it
  never carries into the next battle or an overworld save. The same predicate arms the battle-gated
  GPU-trace recorder.
- **Input.** The physical L2/R2 triggers (keyboard `,`/`.`) are routed to the game's unused "pad 2"
  high-word L2/R2 bits in `PadRead` — the same decouple the ally-cycle uses for L1/R1 — so the right
  stick keeps camera pitch on the low word and no in-game control is stolen (the game only reads
  pad-2 L2/R2 in three debug-only object handlers). `PC_BattleSpeedInput()` edge-detects those bits:
  R2 steps faster, L2 slower, clamped to 1..`VH_BATTLE_SPEED_MAX`, and ignored while the options
  overlay is open.
- **Why 2× only.** The host renders a full battle frame in ~8 ms (~120 fps ceiling), so 2× is
  reached with every tick still rendered; 3× would need frame-skipping the software rasterizer
  (strobing) for little gain.

## Pad input

`PadRead(id)` in `libetc.c` returns the standard PS1 digital-pad word, with the `PADxxx` bit layout
from `libetc.h` (`PADLup = 1<<12`, `PADRdown = 1<<6`, `PADstart = 1<<11`, …). Two input sources are
OR'd together each call:

- **Keyboard** (SDL scancodes): arrow keys → D-pad; `W/A/S/D` → Triangle/Square/Cross/Circle;
  `Q/E` → L1/R1 (camera rotate); `R/F` → low-word R2/L2 (camera elevation); `[`/`]` → pad-2 L1/R1
  (ally cycle); `,`/`.` → pad-2 L2/R2 (fast-forward); `Return` → Start; `Space` → Select. The full
  layout is in [controls.md](../../controls.md).
- **Game controller** (any SDL-recognised pad, opened lazily and re-opened after a hot-plug): D-pad
  and left stick → D-pad; face buttons by position (A/B/X/Y → Cross/Circle/Square/Triangle);
  physical shoulders → **pad-2** L1/R1 (ally cycle) and analog triggers → **pad-2** L2/R2
  (fast-forward); Start/Back → Start/Select. The right stick maps to the low-word camera buttons
  (horizontal → L1/R1 rotate, vertical → L2/R2 elevation) for a twin-stick feel, with per-axis
  invert (`g_camInvertX/Y`, from `VH_CAM_INVERT_X/Y` / the ini's `[camera]` section; Y is inverted by
  default, the modern twin-stick convention). `g_btnLabels` (`VH_BUTTON_LABELS`, default Xbox)
  only chooses the port's own overlay footer glyphs; the game's prompts are untouched.

`PadRead(0)` packs both controller ports into one 32-bit value (port 0 low 16 bits, port 1 high 16 —
the game reads player 2 via `PadRead(...) >> 0x10`). The port deliberately uses that otherwise-unused
high word as a second, independent set of shoulder bits (`gPad2State`), which is what lets L1/R1 mean
"cycle" while the stick still rotates the view. `PadInit` is a no-op (SDL is already up); L3/R3 are
unmapped because the game reads a digital pad.

**Options-overlay filter.** The raw word passes through `PC_OverlayFilterPad()` before the game sees
it, so the SELECT+START overlay works in every context (battle, world map, movies) with no `src/`
change. The filter must be **idempotent across repeated same-frame calls** — the game reads
`PadRead(0)` twice per battle frame (`gPadState`, `gPad2State`) and busy-waits on it in
`core/engine.c` — so it keeps no multi-frame edge buffering, only: a chord *latch* (toggle once per
press, stable when re-read); stateless masking of START+SELECT from the low word while SELECT is
held, so the movie/battle START-skip never mistakes the chord for a skip (START alone is untouched;
low-word SELECT has no non-debug game use); a zero pad to the game while the overlay is open (it
keeps rendering but idles); and a release-tracked *swallow* mask — every button still held at the
instant the overlay closes is hidden from the game until physically released, otherwise it would
read as a fresh press (Circle would pop the battle menu).

## Memory card and saves

Saves are **real files**, not a simulated card image. The virtual card is always present, and
card-path strings of the form `bu00:BASLUS-00447VH` are mapped to `<saves>/<name>` by stripping the
`device:` prefix (`LocalPath` / `StripDevicePrefix`). `InitCard` creates the saves directory;
`FileOpen/Read/Write/Seek/Close` are thin `fopen`/`fread`/`fwrite`/`fseek` wrappers over a small open
handle table, and directory enumeration (`firstfile`/`nextfile`) walks the folder with `opendir`,
reporting each entry's real size. `core/card.c`'s own block/capacity accounting is driven entirely by
those real file sizes, so this layer only supplies honest existence/size/read/write — no PS1 card
filesystem format is emulated. Saving in-game calls `Card_CreateFile` + `Card_WriteFileListing`,
which writes `<saves>/BASLUS-00447VH`; loading checksum-verifies and reads it back. Nothing needs to be
pre-created.

**Where the folder is.** `SaveDir()` resolves the location once per mode and caches it. It prefers a
folder **next to the executable / AppImage** (`PC_GetDeployDir()`), the same predictable place as the
disc, so saves do not depend on the working directory the game is launched from (a double-clicked
AppImage otherwise scatters them into `$HOME`). Resolution order:

1. `<deploy>/saves` already exists → use it;
2. a cwd-relative `saves` exists → use it (an established launch-from-folder layout is not orphaned);
3. create `<deploy>/saves` → use it (the default for a fresh install);
4. the deploy dir is unusable or read-only → fall back to cwd-relative `saves`.

Tactical Mode keeps its saves apart: the folder name is `saves_tactical` while `gTacticalMode` is
set, and `SaveDir()` re-resolves when the mode changes (only ever at the title menu, so no save
operation is in flight). `PC_SaveDir()` exposes the resolved path so the save-management backend
(`pc_saves.c`) archives and restores from exactly the folder the game reads. Two robustness details:
a creating `FileOpen` (`O_CREAT`) re-creates the folder if it has gone missing mid-session, so an
in-battle save cannot fail on a deleted directory; and `nextfile` reports **regular files only** — a
real card cannot hold directories, and a stray subfolder returned as a card file makes
`Card_CountFreeBlocks` read garbage as its header block count ("no free blocks" on a near-empty
card).

The async-completion **event signalling** described in [The event system](#the-event-system) is what
makes New Game / Load Game work at all: the three async card stubs raise `EvSpIOE`, driving
`Card_CheckState()` to "card ready" so the file ops proceed instead of the wait-loop spinning
forever. `StartCard` similarly raises `EvSpNEW` ("a card is present"). `FormatDevice` is a
deliberate no-op: nothing exercises it, and speculatively deleting real save data is worse than
doing nothing.

**Caveats.**

- A garbage or stale file under `saves/` will (correctly) checksum-fail and surface as "CANNOT READ
  MEMORY CARD"; that is the game rejecting a bad save, not a backend fault. Delete the stray file.
- The in-battle save serializes `UnitStatus`, whose in-memory size *is* pointer-width-sensitive
  (120 bytes at ILP32, 136 at LP64 — it embeds two live runtime pointers, so LP64 opens an 8-byte
  hole plus tail padding). Left naive, a 64-bit build would write a differently-sized blob than a
  32-bit one and the two couldn't share saves. The `PC_PORT`-gated
  `Pc_PackInBattleSave`/`Pc_UnpackInBattleSave` in `src/core/card.c` keep the on-disk record at the
  **fixed 120-byte PSX layout**, packed field-by-field around the pointer hole, with the checksum over
  that fixed size — so **in-battle saves are architecture-agnostic**, cross-loadable between the 32-
  and 64-bit builds (regular saves and the file listing contain no `UnitStatus`). See
  [`../../memory-safety.md`](../../memory-safety.md) for the width-bug *class* and the `sizeof` diff
  between the 64- and 32-bit builds that detects it.

## Kanji glyph lookup (`Krom2RawAdd`)

`Krom2RawAdd(sjisCode)` emulates BIOS call B(51h): it returns a pointer to the 30-byte glyph bitmap
for a full-width Shift-JIS code, or `-1` when the code is unmapped — exactly what `core/text.c`'s
`DrawSjisGlyph()` expects. Every `DrawSjisText()` (the TURN counter, the PLAYER/ENEMY TURN banner,
item names, gold, party lists) draws through it, so a stubbed lookup renders nothing. The glyph data
is `pc_kanji_charset2[]` in the generated `pc_kanji_font.c` (from the PsyQ `KROMDAT.BIN`, see
[data-segment.md](../data-segment.md)); its extent is region-dependent, hence the unsized extern.

The BIOS packs its character sets **compacted** — undefined kuten positions are skipped — so the
index is not a simple offset (psx-spx kernel chapter, "BIOS Character Sets"):

- **US build** (`sjis_to_krom_glyph`, non-JP): the 209-glyph subset the game draws. Kuten row 1
  (`0x8140..0x817C`) is linear — the recovered anchors (space = 0, period = 4, plus = 59, minus = 60)
  all satisfy `index == sjis - 0x8140`, proving no skips inside that span, and the whole row is
  served so the language-pack subtitle renderer (`pc_lang.c`, `AsciiToWideSjis`) gets its
  punctuation from the built-in font. Then `#` = 65, digits `0x824F..` from 147, `A-Z` `0x8260..` from
  157, `a-z` `0x8281..` from 183.
- **JP build**: the full ROM layout, because the JP game draws its entire repertoire (kana, level-1
  kanji, punctuation) through this call. *Charset 2* (glyphs 0..523) is SJIS `0x8140..0x84BE` = JIS X
  0208 kuten rows 1–8, compacted by a per-row table of defined spans whose sizes sum to exactly 524
  (the ROM's charset-2 extent) and whose rows 1+2 sum to 147 — the US digit anchor — so the
  compaction rule is confirmed twice. *Charset 3* (glyphs 524..3488) is SJIS `0x889F..0x9872` = kuten
  rows 16–47, all 2965 JIS level-1 kanji, dense (31 full 94-cell rows + 51 in row 47). Bitmap
  anchors against `KROMDAT`: index 147 is fullwidth `0`, 210 hiragana *a*, 293 katakana *a*, 524 the
  kanji 亜 (16-01). Everything else returns `-1`, as the BIOS does; the JP game then falls back to
  its own `gCustomGlyphs` (`core/text.c`) for the ~27 level-2 codes it needs, as on hardware.

A language pack (`pc_lang_font.c`) is consulted first: pack-assigned 2-byte codes (`0x8440+`, a range
the retail map never answers) resolve to pack-supplied 16×15 glyphs, so accented item names ride the
existing `DrawSjisGlyph` path, anti-aliasing included; without a pack the hook returns NULL and the
retail lookup runs unchanged.

## Gotchas / notes

- **`VSync(2)` is 30 FPS on purpose.** Don't "fix" it to 60 — battle logic ticks once per two
  vblanks by design. Convert BizHawk vblank counts by `/2` before comparing battle timing.
- **`GetRCnt` is shared, and scaling the primitive is a trap.** `IsLagging()` needs it slowed, but
  `src/core/graphics.c`'s incremental sprite decoder gates its per-frame decode budget on the same
  counter and is *starved* by the same scale (corrupted geometry, infinite demo loop). The
  per-caller synthetic model exists precisely so these two consumers never share a fudge factor —
  see [Root counters](#root-counters-and-vsync-timing).
- **Slow-host sprite-decoder starvation** is the same failure reached through real slowness; the
  opt-in fix is [`VH_RCNT1_NORMALIZE`](#frame-relative-rcntcnt1-vh_rcnt1_normalize1-off-by-default).
- **`rand()` is the PS1 BIOS LCG, not glibc.** `libkernel.c` reimplements BIOS A(2Fh):
  `seed = seed*0x41C64E6D + 0x3039; return (seed>>16) & 0x7FFF` (psx-spx kernel chapter), seeded to
  **0**: the game never calls `srand`, and real hardware's live seed (RAM `0x80009010`, identified
  by its LCG signature in a BizHawk trace) starts at `0x00000000` with a first transition to
  `0x00003039 = 0*0x41C64E6D + 0x3039`. Using glibc's `rand()` produces a deterministic but
  *different* stream, which scrambles per-demo unit team assignments (`SetupBattleUnit` randomizes
  them) and leaves the AI with no valid target — the algorithm has to match exactly.
  `GetRandSeedForDebug()` exposes the seed for the `VH_RAND_LOG` trace without changing its linkage.
- **`Krom2RawAdd` returns `void *`, not `s32`.** Both callers immediately dereference the result
  (`u8 *p = Krom2RawAdd(...)`); an `s32` return truncates the address under LP64 and crashes the
  first battle menu. `void *` is correct at both widths, needs no `src/` change, and keeps the
  callers' `== -1` sentinel test working.
- **The JP debug menu hook lives in `pc_diag.c`.** `VH_DEBUG_MENU=1` turns the title screen into the
  retail debug-menu hub: once the title has been visible for 90 idle frames (`secondary == 2`,
  `state3` counting — so window assets and the backdrop are loaded), `PC_DiagFrameEntry` flips
  `gState.primary` to `STATE_LOAD_DEBUG_MENU`. Both regions carry the translated hub; see
  [`platform/pc/OPTIONS.md`](../../../platform/pc/OPTIONS.md).
