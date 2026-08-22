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
  overlay. Keyboard: `Q`/`E` rotate, `R`/`F` elevate.
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

---

## Planned

See the [roadmap](roadmap.md).
