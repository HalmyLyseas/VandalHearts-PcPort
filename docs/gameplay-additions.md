# Gameplay additions (PC port)

The port adds an optional layer of features on top of the faithful game. This page describes each
feature as it works **today**; per-release history lives in the [CHANGELOG](../CHANGELOG.md).

Two rules hold everywhere:

- **The faithful experience is the default.** Nothing here changes how the original game plays
  unless you turn it on.
- **Anything that changes gameplay is opt-in** (Tactical Mode). Everything else is presentation,
  controls, or convenience.

Since v2.0 every feature on this page works on **all supported discs** — USA, Asia and Japan —
except language packs, which target the US game's text engine (noted below).

## Startup & loading *(v2.0)*

In-game loads run at **hardware-exact pacing** — the port simulates the PS1 CD drive's seek and
transfer timing, validated against real-hardware captures — so battles, towns and scene changes
feel exactly like the console. The one exception is deliberate: the **initial boot load** (the
stretch before the intro logo, which a real console hides behind its BIOS boot animation) runs
accelerated, taking the fresh launch from ~7 s to ~2 s. `VH_FAST_BOOT=0` restores full hardware
timing there too.

---

## Controls & camera

**Intent: modern controller comfort without changing how the game feels.**

- **Twin-stick camera** — the right stick rotates the battle camera and raises/lowers the view
  angle, with finer elevation steps than the original's fixed positions. The vertical axis ships
  inverted (push up = tilt down, the modern convention); both axes can be flipped in the options
  overlay. Keyboard: `Q`/`E` rotate, `R`/`F` elevate. Elevation has five rest points at pitch
  11.25° / 28.125° / 45° / 61.875° / 78.75° — the clamped 11.25°–78.75° range split into four equal
  16.875° intervals, every stop on the game's 1/4096-turn angle grid, with a clean 45° in the
  middle. Retail's original single-axis mask trick only yields power-of-two spacing (too few,
  nearly redundant stops), so the finer elevation uses an explicit modulo interval instead; pitch is
  not quadrant-coupled the way yaw is, and every scripted camera move (cutscenes, spell FX) drives
  pitch by raw values with its own save/restore, so the added rest points stay isolated to player
  control. The rotation speed evenly divides the interval, so a single tap lands exactly on a stop.
- **Shoulder unit-cycle** — `L1`/`R1` jump the cursor to the previous/next ally, in both
  directions (the original only cycled forward). Keyboard: `[` / `]`.
- **Controller-aware button labels** — in-game button prompts can follow your pad: Xbox letters or
  PlayStation symbols. Set it in the options overlay (`BUTTON LABELS`).

| Xbox labels | PlayStation labels |
|---|---|
| ![Save-management footer showing Xbox A/B/X/Y letters](images/ButtonsLabels-XBOX.png) | ![The same footer showing PlayStation symbols](images/ButtonsLabels-Playstation.png) |

The full scheme — every binding, pad and keyboard — is in [controls.md](controls.md).

## The options overlay

**Intent: change settings in-game, without config files or restarts.**

Press **SELECT + START** during play to open the overlay. Every setting applies live and persists
to `vandalhearts.ini`:

![The options overlay: Tactical Mode, HD pack, resolution, camera, labels, disc, language, saves](images/OverlayMenu-Main.png)

- **TACTICAL MODE** — the opt-in rebalance (see below). Changeable only at the title screen: a
  run's mode is fixed.
- **HD PACK** — the optional HD backgrounds/movies layer (see below). When a pack can't be used,
  this row says why (`NO PACK` / `OUTDATED PACK` / `WRONG GAME`).
- **INTERNAL RES / WINDOW SCALE / FULLSCREEN** — display settings (see *Graphics*).
- **CAMERA X/Y-AXIS, BUTTON LABELS** — controls settings (above).
- **DISC** *(v2.0)* — with more than one supported disc installed (USA / Asia / Japan), pick which
  game to run. Not live: the pending disc is marked `*` and boots at the next launch. Greyed with
  a single disc.
- **LANGUAGE** — pick an installed language pack (see below). Also not live: a pending change is
  marked `*` and applies at the next launch. Greyed on the Japanese game (packs target the US text
  engine) — but with a US disc switch pending you can already queue a pack for the restart.
- **SAVE MANAGEMENT / RETURN TO TITLE** — below.

### Save management

**Intent: never lose progress to the original's single save card.**

Saves are ordinary files in a `saves/` folder next to the game — no memory-card images to manage.
`SAVE MANAGEMENT` in the overlay adds unlimited whole-card backups:

![The save browser: timestamped backups and the active-card marker](images/SaveManagement-Browse.png)

Press Start on a backup to inspect its three slots — chapter, section, level, playtime — before
touching anything:

![The save-detail view showing each slot's progress](images/SaveManagement-Inspect.png)

Restoring defaults to the safe path: **back up the current card first**, then restore. Destructive
prompts are shown in red:

![The restore confirmation: back up then restore is the default](images/SaveManagement-Restore.png)

### Return to title

**Intent: leave a run without quitting the program.**

`RETURN TO TITLE` jumps back to the title menu from anywhere, with a red confirmation first —
unsaved progress is lost, and the prompt says so:

![The return-to-title confirmation](images/ReturnToTitle.png)

### Overlay internals

*Maintainer notes.* The overlay is `platform/pc/src/pc_overlay.c` / `pc_overlay.h` (model and
state machine); the pad filter in `libetc.c` forwards one button edge at a time to
`PC_OverlayInput()`, and the renderer in `pc_gpu_window.c` paints the current screen from the
accessor functions. Four screens: MAIN (the settings list), SAVES (the archive browser), CONFIRM
(a two- or three-option prompt) and DETAIL (one archive's three slots).

- **No "Close" item.** The SELECT+START chord is the only way to show or hide the overlay: a
  face-button close would leak the still-held press to the game underneath.
- **MAIN is a data-driven table** (`s_items[]`): each row is a toggle, a choice or an action with
  an optional `apply` callback, an ini section/key to persist to, and two predicates —
  `disabled` (greyed, visual only) and `locked` (read-only). Window Scale and Fullscreen are
  greyed to show which display mode is inactive but stay interactive, because toggling the
  greyed one is how you switch; Tactical Mode (off the title screen), Return to Title (at the
  title), HD Pack (no valid pack) and a single-disc DISC row are greyed *and* locked.
- **Cross-row rules.** Changing the window scale drops fullscreen so the new scale is visible.
  HD renders only at INTERNAL RES > 1 and, windowed, is only visible at WINDOW SCALE > 1, so
  turning HD PACK on bumps both to at least 2, and dropping either back to 1 turns it off again
  (`reconcileHdPack`, suppressed while the bump itself is in progress). When the pack is unusable
  the HD row's value is the reason (`NO PACK` / `OUTDATED PACK` / `WRONG GAME`).
- **Restart-bound picklists.** DISC lists one entry per release id found by the unified launcher
  (the `VH_DISC_ID_US/ASIA/JP` environment published by `pc_region_main.c`; SLUS-00447 and
  SCPS-45183 share a master but are distinct discs) and persists `VH_REGION` + `VH_DISC_ID`; the
  running binary *is* its region, so the change applies at the next launch and `*` marks the
  pending restart against the `VH_DISC_BOOTED` baseline. LANGUAGE works the same way on `VH_LANG`,
  never auto-selects a pack, and is greyed whenever the selected disc is the Japanese game (its
  core links `pc_lang_stub.c`, which also reports zero packs). Both rows' ceilings are set at open
  time to the number of entries found, so the generic clamp/cycle logic treats them like any
  other choice.
- **The overlay's text is English and not translatable** — packs cover the game's content, and
  the 5×7 OSD font is caps-only Latin, so a non-Latin pack could not render here anyway. The one
  piece of pack text shown, the manifest name, is folded from Latin-1 accents to base letters.
- **Return to title is deferred.** The confirm only sets a request flag; `PC_ApplyReturnToTitle()`
  performs the jump at the top of the main loop (`main.c`, before `UpdateState`), the one point
  where no game code is mid-frame. The overlay runs from the pad path, which also fires inside
  nested VSync wait loops (`LoadCdFile` spins VSync mid-`LoadEvent`); flipping `gState` there
  lets the live loader's tail write it straight back and race the title re-init until the object
  pool overflows. The jump replicates the game-over teardown (`battle/evaluators.c`): abort any
  movie stream (not object-driven, so it needs an explicit stop), drop the held final frame
  (`CdlPause` keeps the last decoded frame on screen until the movie flow's `ClearScreen`, which
  this path never reaches — without the drop the title menu runs invisibly under the frozen
  frame), close subtitles, stop all audio, clear `gIsEnemyTurn` / `gState.inEvent` /
  `gPlayerControlSuppressed`, set `gState.primary = STATE_TITLE_SCREEN`, and re-sync the balance
  patch. The title's own object reset then clears every leftover object before any can run.

### Save-file internals

*Maintainer notes.* `platform/pc/src/pc_saves.c` is pure file I/O; the overlay owns the UI and the
confirmations.

- **One card per region.** The game keeps a single memory-card file, `bu00:<id>` in `core/card.c`
  — `BASLUS-00447VH` on the US game, `BISLPM-86007VH` on the Japanese game — mapped to
  `saves/<id>` (Tactical: `saves_tactical/`). Archives live in the hidden `saves/.archive/` folder,
  dot-prefixed so the game's own `firstfile("bu00:*")` enumeration skips them, and are named
  `<id>.<YYYYMMDD-HHMMSS>` from the PC clock (a `-NNN` suffix disambiguates same-second backups).
  Every archive name, listing filter and restore check carries the region id, so both regions'
  cards and backups coexist in one folder with no cross-region restore possible.
- **Every archive is a real card.** Backups are byte-identical copies, so any of them is still a
  valid memory-card file. Copies are durable: written to a sibling temporary, fsynced, then
  atomically renamed over the destination (`MoveFileEx` with write-through on Windows).
- **Validation before restore or inspect.** A card must start with the magic `'S','C',0x12,0x02`;
  the 128-byte listing block follows the header and is CRC32-checked; each occupied slot's
  0x300-byte record at `header + (slot+1) × 0x400` is CRC-checked too. The header is 384 bytes on
  the US card and 512 on the Japanese one (an appended third icon frame), and the icon-frame type
  byte reads 0x12 on both, so the layout is probed — the listing CRC is tried at 384, then 512 —
  which keeps the parser region-blind for a hand-moved file.
- **Listing layout.** `checksum[0..3]`, `slotOccupied[4..7]`, then three 40-byte captions at
  offset 8 — all byte arrays, so the parse is width-independent. The in-battle "continue" record
  has no caption and is naturally excluded. Captions are uppercased for the caps-only OSD font;
  Japanese captions are full-width Shift-JIS (`１章１節　Ｌ５　０：１０`) and fold to ASCII — digits,
  letters, space and colon to their ASCII forms, 章 to `-`, 節 dropped — giving `1-1 L5 0:10`. A
  byte-wise filter would shred those pairs, since Shift-JIS trail bytes fall in the ASCII range.
- **Ordering and the active marker.** Archives list newest first by modification time (stored as
  `long long`: `long` is 32-bit on Windows while `st_mtime` is 64-bit), with the filename as the
  tie-breaker. The `(*)` active marker means the archive is byte-identical to the current card.

## Battle quality of life

### Enemy threat overlay

**Intent: see at a glance what the enemy can reach, so turns need less counting.**

Toggle the overlay in battle to paint the enemies' combined danger zone:

![The threat overlay painting the enemies' combined attack reach in red](images/ThreatOverlay-EnemyPhysicalAttackReach.png)

With one of your units selected, its movement range overlays the threat: blue = safe movement,
orange = reachable but threatened, purple = enemy threat beyond your reach:

![A selected unit's movement range over the threat: blue safe, orange contested](images/ThreatOverlay-PlayerMovement.png)

Targeting keeps the distinction visible while you pick a destination or victim:

![Targeting with the threat overlay active](images/ThreatOverlay-Targeting.png)

Spell AoE previews stay visually distinct from the threat colors, so area placement and danger
never blur together:

![A spell's area preview clearly distinct from the threat overlay](images/ThreatOverlay-AOEvisibility.png)

### Battle fast-forward

**Intent: keep tactical battles, skip the waiting.**

Hold nothing, tap once: **R2** doubles the battle speed; **L2** returns to normal (keyboard `.`
and `,`). A `BATTLE SPEED X2` readout shows while it's active:

![A battle running at double speed with the BATTLE SPEED X2 readout](images/BattleSpeed-x2.png)

Only the pacing changes — AI decisions and outcomes are identical, so fast-forward is never a
gameplay change. It applies in every battle type, and holds the full 2× even at the maximum
internal resolution.

## Tactical Mode *(opt-in rebalance)*

**Intent: a fresh challenge for players who know the game — without touching the original.**

Tactical Mode is a separate, opt-in way to play: a per-chapter level cap, Trials that reward gold
and XP, reworked classes, a reined-in Vandalier, and restored/clarified content. Normal mode stays
byte-for-byte the retail game, and each mode keeps its own save folder.

One example of its smaller fixes — items that never explained themselves now do:

![The shop showing an item's restored effect description](images/TacticalOnly-ItemDescriptions.png)

…and the Avalanche map's re-skin turns its boulder to ice, matching the scene:

![The Avalanche battle with the Tactical ice re-skin](images/TacticalOnly-AvalancheReskin.png)

The full design — every class change, spell list, and the reasoning — is in
[tactical-mode.md](tactical-mode.md).

## Graphics

**Intent: sharpen the original image without reinterpreting the art.**

The renderer is a PSX-accurate software rasterizer by default: pixel coverage, texture sampling,
dithering and blending match the console's GPU to measured ~99.8–99.99 % pixel-exactness. On top
of it, `INTERNAL RES` renders the 3D at **1× to 4×** the native resolution — the same art, sampled
more finely, with no re-authored assets. Set it live in the overlay.

*Native (×1):*

![A battle at native internal resolution](images/InternalResolution-x1.png)

*Supersampled (×4):*

![The same view at 4x internal resolution](images/InternalResolution-x4.png)

The high-resolution pass is multithreaded, so ×4 holds the frame cap — including during battle
fast-forward. Configuration detail: [configuration.md](configuration.md).

## HD pack

**Intent: real HD for the pre-rendered art — backgrounds and movies — while hand-drawn pixel art
stays untouched.**

An optional pack replaces the 320×240 pre-rendered backgrounds and the FMV movies with
high-resolution versions. The source tree and base build ship no HD art: install the pack from the
release page, or build one from your own disc. Portraits, sprites, UI and fonts are deliberately
kept native — smoothing pixel art would clash with the crisp UI.

*Without the pack:*

![A pre-rendered background at native resolution](images/HDPack-Off.png)

*With the pack:*

![The same background from the HD pack](images/HDPack-On.png)

Movies get the same treatment — the game keeps its original timing and audio; only the picture is
swapped:

*Native (MDEC):*

![The intro FMV at native MDEC resolution](images/features-1.6-Videos-01.png)

*HD pack:*

![The same intro frame from the HD re-encode](images/features-1.6-Videos-02.png)

Installing, building, and every option: [hd-pack.md](hd-pack.md).

## Language packs

**Intent: a community-driven effort — let more people enjoy the game, in their own language.**

An optional pack displays the game's text in another language: dialogue, menus, item and spell
names and descriptions, battle messages, save captions, **subtitles for the story videos**
*(1.7.1)* — even backgrounds with text baked into the art, such as the title card. The base build
ships no translated text; packs are built by the community with the repository's authoring
toolchain, from a player's own disc. Non-Latin scripts work with drawn glyph sheets (Cyrillic and
Greek are proven in game), and everything renders at every graphics setting.

Select an installed pack with the overlay's `LANGUAGE` row — it applies at the next launch. A pack
is a *diff*: entries a Latin-script pack has not translated yet simply stay English, so a
work-in-progress translation is already playable. Packs are a **US-disc feature** (they are built
on the US game's text engine); the Japanese game plays with its own original Japanese text.

*Greek demo — translated dialogue (note the `;`, the Greek question mark):*

![In-game Greek dialogue](../platform/pc/tools/langpack/images-quickstart/quickstart-10-dialog.png)

*A localized title background:*

![A localized Greek title background](../platform/pc/tools/langpack/images-quickstart/quickstart-12-backgrounds.png)

*Subtitled story video:*

![Greek movie subtitles over the opening video](../platform/pc/tools/langpack/images-quickstart/quickstart-13-videos.png)

Installing and what a pack covers: [language-packs.md](language-packs.md). Making one: the
[hands-on quickstart](../platform/pc/tools/langpack/quickstart.md) and the
[toolchain reference](../platform/pc/tools/langpack/README.md).

---

## The debug menu *(v2.0, advanced)*

**Intent: restore the developers' own tool, for the curious.**

The Japanese release shipped with KCET's development debug menu still in the code — battle-map
warp, a scene selector covering all 95 story events plus world-map destinations and towns, a unit
viewer. The US release stripped most of it (and what remained could never render: its menu text
was drawn through an incompatible text path, blank even on real hardware). The port restores the
full menu **on every supported disc** — on the US/Asia game with the scene lists translated:

*The original Japanese menu, and the same menu on the US game:*

![The KCET debug menu on the Japanese game](images/features-2.0.0-DebugMenu-01.png)

![The same debug menu on the US game, translated](images/features-2.0.0-DebugMenu-02.png)

It is off by default and stays a power-user feature: launch with `VH_DEBUG_MENU=1` and idle
~1.5 s at the title screen to open it. Warping skips the setup a scene normally gets from the
story flow, so some destinations load with placeholder state or look wrong — that is authentic
dev-tool behavior. **Saves made from warped states are unsupported, and bug reports are only
actionable from a normal boot.** Details in
[`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md#debug-menu-advanced).

The US restoration renders its header through the full-width SJIS text path rather than the ASCII
one — beyond matching the Japanese menu's look, this sidesteps `DrawText`'s bare-`U`/`D` control
codes, which ate letters on an earlier ASCII-literal attempt at the same header. The US menu also
skips the Japanese hub's state 8 (an unreachable settings preview that no menu choice ever routes
to in either release) — a reader looking for a missing case in the restored menu isn't missing
anything.

Because the debug menu makes warp paths reachable that a normal playthrough never exercises, two
safety guards exist purely to keep a warp from crashing: unit setup bails out if the object pool is
exhausted, which a warp straight into a battle scene can hit by skipping the normal roster/object-pool
reset; and battle-unit setup bails out if the target map number is out of range or its initial-state
tables have no row for that map, for the same reason. Both are unreachable outside a debug-menu warp.

---

## Planned

See the [roadmap](roadmap.md).
