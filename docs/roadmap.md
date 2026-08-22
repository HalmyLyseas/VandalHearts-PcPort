# Roadmap

Vandal Hearts — PC Port is delivered in stages. The two foundational stages are complete; **Stage 3**
is an optional layer of gameplay and quality-of-life enhancements built on top of the faithful port.

> These are **plans and intentions, not commitments or dates.** This is a non-commercial hobby
> preservation project — priorities may shift, and items may change or be dropped. The faithful
> beginning-to-end experience is always preserved; Stage 3 additions are optional and, where they
> change gameplay, opt-in.

## Done

- **Stage 1 — Matching decompilation.** The C source rebuilds the original executable byte-for-byte.
- **Stage 2 — Native PC port.** The full game runs end-to-end on Windows and Linux from your own disc,
  with faithful audio and video, packaged as one-file downloads.

### 1.0 — Faithful port + first additions ✅
- Complete beginning-to-end experience on Windows and Linux
- Skip intro / movies with START
- Camera rotation and elevation mapped to the right analog stick

### 1.1 — Controls ✅ (released 2026-07-25)
- Cycle through your units with the shoulder buttons (both directions), freeing the Square button
- **Enemy threat overlay** — press Square to see the combined movement-and-attack reach of *all*
  enemy units at once, so you can plan positioning without inspecting them one at a time
- An in-game options overlay (**Select + Start**), with per-axis right-stick invert as its first
  settings (the vertical axis ships inverted by default — the modern twin-stick convention)
- Saves and config now live next to the executable / AppImage, independent of where you launch from
- A documented control scheme and a dedicated page for the PC-port additions

### 1.2 — User experience ✅
- **Save management** in the overlay: unlimited whole-card backups (working around the three-slot
  limit), restore any backup over the current card (with a "back up first" safe default), delete, and
  a `(*)` marker for the backup matching your current card — each backup stays a real-hardware-valid
  save. Press **Start** on a backup to inspect its three slots' chapter / level / playtime.
- **Window scale** (X1–X8) and **fullscreen** from the overlay, applied live and saved to the config.

### 1.3 — Tactical Mode + fixes ✅ (released)
For **all players**: fixes two crashes latent since 1.2 (item transfer, certain cutscenes) and polishes
the options overlay and the enemy-threat overlay. It also adds a **Return to Title** overlay
convenience. And it introduces **Tactical Mode** — a large, **opt-in** rebalance for a more varied
tactical experience (per-chapter level cap, Trials that scale to your chapter and reward gold + XP,
class reworks, a reined-in Vandalier, and clarified item descriptions). Normal mode stays
byte-for-byte the original, and Tactical saves are kept separate. Full player guide:
**[tactical-mode.md](tactical-mode.md)**; per-release detail in the [changelog](../CHANGELOG.md).

> **Tactical Mode is opt-in and validated** across a full playthrough. It stays a living design, so a few
> balance numbers may still be fine-tuned in later point releases. This only affects the opt-in mode —
> the normal experience is unaffected. The **1.3.1** point release folded in the full-playthrough balance
> tuning and endgame fixes.

### 1.4 — Quality of life ✅ (released 2026-07-31)

1.4 is a quality-of-life pass. Most of it — the fast-forward, the controller-matched prompts, the camera
tweak — is convenience an emulator or launcher could offer too; the one change that genuinely needs the
game's source is the **smarter Tactical AI**, which reaches into the enemy's own targeting logic. No new
artwork required:

- **Faster battles** — a battle-only **2×** fast-forward on the triggers for quicker play, with outcomes
  **identical** to normal speed.
- **Controller-aware prompts** — the port's own overlay button hints show **Xbox** letters or
  **PlayStation** symbols, switchable in the overlay (defaults to Xbox).
- **Smarter Tactical AI** — enemy spellcasters weigh magic resistance when choosing targets, so
  positioning and defensive buffs (like Mystic Energy / Perfect Guard) matter on defense too. **Tactical
  Mode only.**
- **Finer camera elevation** — an extra, evenly-spaced up/down camera angle (including a level 45°) for
  reading maps with stepped terrain.

### 1.5 — Graphics fidelity ✅ (released 2026-08-01)

A **graphics fidelity** track, kept deliberately conservative — the goal is to *sharpen without
reinterpreting* the game. Three pieces:

- **A more hardware-accurate software renderer — DONE (`VH_ACCURATE`, on by default).** A fixed-point
  integer DDA that evaluates coverage *and* texture UVs at the exact pixel position the PS1 GPU does, plus
  ordered dithering (gated on the GPU dither-enable bit) and 5-bit semi-transparency blend. Validated
  ~99.8–99.99% pixel-exact against a DuckStation VRAM capture across effect and battle scenes; the
  casting-ray difference that motivated it is resolved. `VH_ACCURATE=0` keeps the legacy renderer.
- **Higher internal rendering resolution — DONE (`VH_INTERNAL_SCALE`, off by default).** Draws the 3D at
  1× (native), 2×, 3×, or 4× the sample rate for crisper terrain and edges — an optional supersampling
  layer *on top of* the accurate DDA, which samples the same textures on a denser grid. **No assets are
  re-authored:** the game's own sprites, textures, videos and UI are untouched. Set live in the options
  overlay ("INTERNAL RES") and persisted. **Crust-free tile sampling** is built in: on perspective world
  polys the finer grid biases the sample onto the tile interior (like the reference renderer) instead of
  each tile's dark texture-cell "crust," which would otherwise show as a faint dark grid along terrain/lava/
  water seams — this also resolved the old compass "dotted lines" and the parked water-shimmer note. 2D
  UI/text is auto-detected and kept pixel-aligned, so the bias never disturbs menus or glyphs.
- **Multithreaded rasterizer — DONE (`VH_RASTER_THREADS`, auto).** The software renderer's hi-res pass is
  split across CPU cores (disjoint scanline bands, lock-free, bit-identical output), so 4× internal
  resolution holds the 30 fps cap and battle fast-forward stays effective on a multicore machine.

Kept for the **graphics-track renderer** itself, but deliberately **out of scope:** redrawn or AI-upscaled
**sprites**, and camera changes like finer rotation — they'd re-author the hand-drawn pixel art or alter the
game's feel, which this project avoids. (Bundled "collector" extras such as artwork or manuals are also out
— that material is copyrighted.) The pre-rendered backgrounds and FMV movies are addressed separately by the
**optional** 1.6 HD pack below, which is user-supplied data rather than a change to the base build.

### 1.6 — HD pack: backgrounds + movies ✅ (released 2026-08-02)

An **optional** engine layer that replaces the 320×240 pre-rendered backgrounds *and* the FMV movies with
higher-resolution art, plus the tools to build a pack. Scoped to the pre-rendered layers only:

- **Backgrounds.** Pre-rendered, continuous-tone art that upscales cleanly; replaced at render time with no
  change to layout, palette or UI. Each is keyed by a content hash of its VRAM upload, so a pack maps 1:1
  with no play-through. Portraits, sprites, UI and fonts are hand-drawn pixel art and stay native.
- **Movies.** The intro/ending FMVs can be replaced with HD **H.264/HEVC** re-encodes, shown in place of the
  native MDEC video while the game keeps its original frame timing and XA audio in sync.
- **The source tree and base build ship no art.** A pack is either built from your own disc or downloaded
  as an optional 1.6 release asset (upscaled derivative art); it is auto-detected in `hdpacks/` beside the
  executable and toggled by the "HD PACK" options row. The base build is unchanged without one.
- **Buildable offline from the disc** (no play-through): `platform/pc/tools/hdpack/` assembles a complete
  pack — `.webp` backgrounds (~20 MB) + HEVC movies. New deps **libwebp** + **libav** are default-on and
  optional (`NO_WEBP` / `NO_HDVIDEO`); the Windows build links a **static** libav, so it ships no extra
  ffmpeg DLLs.

See [hd-pack.md](hd-pack.md).

### 1.6.1 + 1.6.2 — polish and robustness ✅ (released 2026-08-04/05)

- **1.6.1**: a frame-pacing fix so battle fast-forward holds the full 2× at every graphics setting,
  background-threaded HD scene loads, an HD-pack **manifest v2** that verifies the pack and explains
  itself when one can't be used, a **Player Manual (PDF)** shipped with each release, and reorganized
  player documentation (per-feature guide, [troubleshooting](troubleshooting.md),
  [performance](performance.md)).
- **1.6.2**: born from the first user bug report — a damaged disc image used to hang the game
  silently and now stops with a clear error naming the broken sector (see
  [troubleshooting.md](troubleshooting.md#the-game-wont-start)).

### 1.7 — Language packs ✅ (released 2026-08-09)

An *enabling* track rather than a gameplay one: the port can display the game's text in another
language, loaded from an optional community-built pack ([language-packs.md](language-packs.md)).
The authoring toolchain ([`platform/pc/tools/langpack/`](../platform/pc/tools/langpack/)) exports
the text from a player's own disc, validates a translation against the engine's real limits, and
builds the pack — including non-Latin scripts (Cyrillic and Greek proven in game) and localized
backgrounds. The base build still ships no game text and behaves identically without a pack.

### 1.7.1 — Video subtitles ✅

Closes the translation framework's one remaining gap: the burned-in narration of the eight story
videos (six chapter intros + both endings) becomes subtitle-able by a pack — validated timings,
the game's large font at the original narration's size, identical with and without the HD pack,
per-line fallback to the burned English. The toolchain gains a fifth export
(`lang_export_cues.py`) and the corresponding template/validation/build support.

Also ships the project's first community code contribution (thanks to Christopher Ball):
**save-management hardening** (atomic backup/restore, checksum-validated backups, an overlay
status line) and **smarter disc auto-discovery** (boot-signature check — multi-track dumps work).

### 2.0 — One executable, three regions ✅

The region barrier falls: a **second complete matching decompilation** — the Japanese
`SLPM-86007` build — joins the US one, and a unified executable carries both, auto-detecting
whichever supported disc you own (USA / Asia / Japan) with an in-game **DISC** switcher when
several are installed. The Japanese game runs with the **full PC feature set** (Tactical Mode,
overlay, HD pack support, fast-forward) and its own faithful Japanese text; a dedicated Japanese
**HD pack** ships alongside the US one (`hdpacks/<game-id>/`). Also in 2.0: **fast boot** (the
pre-logo load runs accelerated; in-game loads stay hardware-exact — and the US game's load pacing
is now exact too, closing a historical −5%), window-manager responsiveness during loads, and a
set of long-standing fixes (details in the [changelog](../CHANGELOG.md)).

## Beyond

The originally planned feature arc (Stages 1–3, **v1.0–v1.6**) is complete, and the project has
kept moving where it matters: **v1.7** answered the most-requested community feature, **v2.0**
brought the Japanese release and its own decompilation under the same roof, macOS support
arrived as the first community contribution, and fixes land as issues are reported
([known_issues.md](known_issues.md)). There is no fixed roadmap beyond the next release — direction
comes from what players and translators actually need.
