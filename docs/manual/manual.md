---
title: "Vandal Hearts — PC Port"
subtitle: "Player Manual"
---

![](platform/pc/packaging/appimage/vandalhearts.svg){.cover-logo}

<p class="version">@VERSION@</p>

# About this port

This is a native PC version of *Vandal Hearts* (PlayStation, 1997), rebuilt from the original
game's code. It plays the original faithfully by default, and adds optional quality-of-life
features on top — all of them off, or purely cosmetic, unless you turn them on.

**You need your own game disc.** The port ships no game content: supply your own legally-owned
*Vandal Hearts (USA)* disc image. This is a non-commercial fan preservation project, not
affiliated with Konami or Sony.

# Setup

## Windows

1. Unzip the release (`VandalHearts-*-windows-x64.zip`) into one folder. Keep everything
   together — the game needs all 8 DLLs next to `vandalhearts_pc.exe`.
2. Put your disc image — a raw `.bin` dump of *Vandal Hearts (USA)* — into a `game` folder next
   to the `.exe` (or directly beside it).
3. Run `vandalhearts_pc.exe`. The disc is found automatically; a wrong or missing disc produces a
   clear message, not a blank window.

Works on a stock Windows 10 or 11 — no runtimes to install.

## Linux

1. Download `VandalHearts-*-linux-x86_64.AppImage` and `vandalhearts.ini`, keep them together.
2. Make it executable: `chmod +x VandalHearts-*.AppImage`. The AppImage runtime needs FUSE2
   (package `fuse2` / `libfuse2` on most distributions).
3. Put your disc `.bin` in a `game/` folder next to the AppImage, and run it.

## The HD pack (optional)

Download `VandalHearts-*-hdpack.zip` from the release page and unzip it so the `hdpacks/` folder
sits next to the executable (next to the AppImage on Linux). The game detects it at the next
launch and turns the **HD PACK** option on. Without the pack, nothing changes.

## Language packs (optional)

Community-made translations install the same way: place the pack folder under `langpacks/` next
to the executable (e.g. `langpacks/fr-fantrad/`), then pick it with the **LANGUAGE** option
in-game — **the pack applies at the next launch, so restart the game after choosing it**. Without
a pack the game is its original English.

## Settings

Almost everything is set in-game: press **SELECT + START** for the options overlay. Choices
persist to `vandalhearts.ini` next to the executable — a plain text file you can also edit
directly. Saves are ordinary files in a `saves/` folder; copy it to back up or move your progress.

# Controls

| Gamepad | Original | Action |
|---|---|---|
| D-Pad / Left stick | D-Pad | Move the cursor |
| A / Cross | Cross | Confirm |
| B / Circle | Circle | Cancel · hold to speed text |
| X / Square | Square | Unit list |
| Y / Triangle | Triangle | Menu / end turn |
| Right stick | — | Rotate the camera and raise/lower the view angle |
| L1 / R1 | L1–R2 | Cycle through your units (previous / next) |
| L2 / R2 | — | Battle fast-forward — R2 doubles the speed, L2 back to normal |
| START | Start | Confirm dialogs · skip movies |
| SELECT + START | — | Options overlay |

Keyboard: arrows move, `S` confirm, `D` cancel, `A` unit list, `W` menu, `Q`/`E` rotate,
`R`/`F` raise/lower the view, `[`/`]` cycle units, `.`/`,` fast-forward, Enter Start,
Space Select. Full reference: `docs/controls.md` in the repository.

# Features

All features work in both the normal game and Tactical Mode, except where marked.

## The options overlay

Press **SELECT + START** during play. Every setting applies immediately and is remembered.

![](../images/OverlayMenu-Main.png)

## Battle helpers {.page-break}

**Enemy threat overlay** — toggle an overlay that paints the enemies' combined reach; with a unit
selected, blue is safe movement and orange is reachable-but-threatened. Plan turns at a glance
instead of counting tiles.

![](../images/ThreatOverlay-PlayerMovement.png)

**Battle fast-forward** — tap **R2** in battle for 2× speed, **L2** for normal. Only the pacing
changes; outcomes are identical. Works in every battle, at every graphics setting.

## Save management {.page-break}

The overlay's **SAVE MANAGEMENT** keeps unlimited timestamped backups of your save card. Inspect
any backup's three slots before restoring; restoring backs up the current card first by default.
Destructive choices are shown in red.

![](../images/SaveManagement-Browse.png)

## Graphics {.page-break}

**INTERNAL RES** renders the 3D at up to 4× the original resolution — the same art, sampled more
finely. The renderer itself reproduces the PlayStation GPU's output pixel-for-pixel; supersampling
sharpens it without changing its character.

![](../images/InternalResolution-x4.png)

## HD pack {.page-break}

With the optional pack installed, the pre-rendered backgrounds and the movies play in true HD.
Hand-drawn pixel art — portraits, sprites, menus — deliberately stays original: crisp, not
smoothed.

![](../images/HDPack-On.png)

## Language packs {.page-break}

With a pack selected, the game's text — dialogue, menus, items, battle messages — displays in
that language, at any graphics setting. Some packs also translate backgrounds with baked-in text
(such as the title card). **Changing the language takes effect at the next launch**; the port's own
SELECT+START menu deliberately stays English. Packs are made by the community with the tools in
the repository — this project distributes no game text.

## Tactical Mode *(opt-in, changes gameplay)* {.page-break}

A second way to play for veterans: a per-chapter level cap, rewarding Trials, reworked classes,
and a reined-in ultimate class. Turn it on at the title screen; it keeps its own saves, and the
normal mode remains exactly the game as shipped. Full design: `docs/tactical-mode.md`.

# If something goes wrong {.no-break}

| Symptom | Fix |
|---|---|
| "Disc image not found" | put your `.bin` in the `game/` folder, or set `VH_DISC_IMAGE` in the ini |
| Error box at launch, then nothing (Windows) | a DLL is missing — re-extract the whole zip |
| AppImage won't start (Linux) | install FUSE2; `chmod +x` the file |
| HD PACK row greyed | its value says why: `NO PACK` / `OUTDATED PACK` / `WRONG GAME` |
| Slow or stuttering | lower `INTERNAL RES`; see `docs/performance.md` |

More detail: `docs/troubleshooting.md`. Bugs: the project's issue page — run from a terminal and
include the console output.
