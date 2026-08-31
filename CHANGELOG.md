# Changelog

Notable changes to the **Vandal Hearts PC port**. This tracks the port layer (Stage 3 gameplay/QoL and
packaging); the underlying decompilation stays byte-for-byte faithful to the retail game, and the normal
mode is unaffected by any of it. Format loosely follows [Keep a Changelog](https://keepachangelog.com/).

## [2.1.0] — Maintenance: robustness fixes

A maintenance release: two small conveniences, and a large batch of robustness fixes from an
independent code review of the port. As always, normal mode is unaffected — still byte-for-byte
the retail game.

### Added

- **A message box when no disc is found.** Launching by double-click with no `game/` folder set
  up used to open a window and close it again with no explanation. It now shows a dialog
  explaining what it looked for and where to put your disc `.bin`, on top of the existing console
  message.

  ![The missing-disc dialog explains which releases are supported and where to put the disc image](https://raw.githubusercontent.com/HalmyLyseas/VandalHearts-PcPort/master/docs/images/features-2.1.0-Missing-Bin-File.png)

- **Escape asks before quitting.** Once a game is underway, pressing Escape now opens a
  confirmation ("QUIT THE GAME?") with **NO** selected by default, so an accidental press can't
  lose unsaved progress — the same confirm the overlay's RETURN TO TITLE uses. At the title screen
  and during its logo/title-attract videos, Escape quits immediately because no run is active. The
  window's close button also still quits immediately, as before.

  ![The in-game quit confirmation warns that unsaved progress will be lost and selects No by default](https://raw.githubusercontent.com/HalmyLyseas/VandalHearts-PcPort/master/docs/images/features-2.1.0-Exit-Confirmation.png)

### Fixed

Eighteen issues found by an independent code review of the port. All but the first need a damaged
or deliberately modified disc image or add-on file to reach:

- **A rare audio crackle/crash on scene change** — a sound bank could be freed while a fading-out
  sound effect was still playing from it. Voices using a bank are now stopped before it's freed.
- **Windows and Linux now compute identical geometry for large camera/object translations** — a
  32-bit-vs-64-bit arithmetic difference in the GTE math (invisible in ordinary play, where the
  values involved stay small) meant the two platforms could diverge in principle. Both now use
  the same wide arithmetic.
- **Corrupt or hand-modified game data can no longer crash the port** — out-of-bounds reads and
  unbounded parsing were closed across TIM/VRAM image loading, VAB/SEQ sound-bank loading, HD
  video (a mid-file resolution/format change), an oversized HD image, and hand-edited subtitle
  cue files. All require a damaged or deliberately modified disc image or add-on file — an
  unmodified disc was never affected.
- **A genuine crash now fails loudly instead of being silently "repaired."** A safety net meant
  for one specific startup case had grown to catch stray memory writes anywhere, turning some
  real bugs into silent corruption instead of a clean crash. It's now scoped back to its
  original job.

### Changed

- **Headless launches now explain themselves in the log** *(advanced)* — if the game can't open a
  window, the log names the reason instead of just noting that it's running without one.
- **An optional exact-integer camera-math path** *(advanced, off by default)* — `VH_GTE_EXACT`
  switches the GTE's rotation-matrix math to a bit-for-bit transcription of the original PlayStation
  integer routine, for comparing against the port's default floating-point path. See
  [OPTIONS.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/OPTIONS.md).

### Developer

- Comment hygiene enforced tree-wide (`make check-comments`); the data-segment generator now
  refuses a wrong-region or truncated game executable instead of silently mis-building; ten new
  regression harnesses under `platform/pc/tools/regress/`; release-script hardening (a bad
  version tag is refused before anything is deleted, the shared-region identity check is now
  enforced, per-game HD-pack packaging, a Windows-only publish no longer references a missing
  file).

### Compatibility

- Saves, `vandalhearts.ini` and language packs all keep working untouched.
- **HD packs are unchanged since 2.0.0** — keep the one you already have, or download it from the
  [2.0.0 release page](https://github.com/HalmyLyseas/VandalHearts-PcPort/releases/tag/v2.0.0).

## [2.0.0] — One executable, three regions

A region barrier falls. This release adds a **second complete matching decompilation** — the
Japanese release, `SLPM-86007`, rebuilt byte-for-byte like the US one — and ships **one
executable that runs the USA, Asia and Japan discs**, auto-detecting whichever you own. The
Japanese game is a first-class citizen: the full PC feature set, its own HD pack, its own
faithful Japanese text. As always, normal mode remains byte-for-byte the retail game — now for
two retail games.

### Added

- **Japan and Asia disc support.** Drop any supported disc — USA (`SLUS-00447`), Asia
  (`SCPS-45183`) or Japan (`SLPM-86007`) — in the `game/` folder and play; several at once is
  fine. The Asia release (byte-identical to USA bar the save id) has quietly worked for a while —
  this is its first *supported* release. The Japanese release is a different build of the game
  entirely, and runs on its own decompiled code. Setup:
  [configuration.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/configuration.md#supported-releases).
- **The DISC option.** With more than one disc installed, a new overlay row lists each by its
  release id; pick one and it boots at the next launch (marked `*` until then, like LANGUAGE).
- **Full feature parity on the Japanese game**: Tactical Mode, the options overlay, save
  management, battle fast-forward, threat overlay, unit-cycle, finer camera elevation, internal
  resolution, HD pack support. Tactical Mode's reworked text stays faithful to the Japanese disc —
  its clarified item descriptions reuse the disc's own Japanese spell lines, and adjusted spell
  info lines are the retail Japanese text with only the numbers updated. **Language packs remain a**
  **US-disc feature by design** (they are built on the US game's text engine); the LANGUAGE row
  explains itself — greyed on the Japanese game, and even usable there to queue a pack when a
  switch to a US disc is pending.
- **A Japanese HD pack.** Backgrounds and story videos for the Japanese game (its movies carry
  the original burned-in Japanese subtitles), downloadable as a release asset like the US pack.
- **The developers' own debug menu** *(advanced)*. KCET's development menu — battle warp, a
  selector for all 95 story events, world-map and town warps, a unit viewer — survived in the
  Japanese release; the US release stripped most of it, and what remained could never render even
  on real hardware. Restored on **every** disc, translated on US/Asia. Off by default; launch
  with `VH_DEBUG_MENU=1` and idle at the title screen. Warped scenes can load in odd states —
  that's authentic dev-tool behavior — and saves made from them are unsupported. Details:
  [gameplay-additions.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/gameplay-additions.md#the-debug-menu-v20-advanced).
- **Fast boot.** The fresh-launch load before the intro logo (a black screen a real console hides
  behind its BIOS animation) now runs accelerated — launch-to-logo drops from ~7 s to under 2 s.
  Everything from the logo onward keeps the hardware-exact load pacing. `VH_FAST_BOOT=0` restores
  full hardware timing.

### Changed

- **Load pacing is now hardware-exact on the US game too.** The CD-timing model gained the
  drive's per-read start cost, tuned against real-hardware captures of both regions; US loads
  were previously ~5% faster than a console. If a load feels marginally longer than 1.7.1, it is
  now *exactly* as long as the real thing.
- **HD packs are per-game** — `hdpacks/SLUS-00447/` and `hdpacks/SLPM-86007/` side by side, the
  right one auto-detected for the running disc (the Asia disc uses the US pack — same master).
  Already have the 1.x HD pack? No need to re-download — create `hdpacks/SLUS-00447/` and move
  the previous contents of `hdpacks/` into it (the old flat layout also still works as-is).
- **Tactical Mode: Mystic Energy's info line now reads "Protect Magic"** (was "DEF,AT Up") —
  matching the spell's Tactical redesign, which is protective (DEF + magic resistance) and no
  longer raises ATK. Display only; Normal mode untouched.

### Fixed

- **RETURN TO TITLE is now safe from anywhere.** Pressed during a loading scene it could crash;
  pressed during a video it froze the frame while audio played on. The jump now tears down
  in-flight loads and movie streams cleanly, from any state.
- **The startup "not responding" dialog is gone.** During long loads the window answered no
  desktop events, so window managers (notably GNOME) sometimes flagged the game as stuck and the
  close button appeared dead. Loads now stay responsive — closing the window works even
  mid-load. Latent since 1.0.0.
- **Stray files in `saves/` no longer break saving.** A folder or foreign file in the saves
  directory was treated as save-card content and could produce a bogus "no free blocks" error.
  Latent since 1.1.0.
- **Windows, Tactical Mode: crash on a reworked spell's info line.** Moving the spell-list
  cursor onto a Tactical-reworked spell (Spread Force, Thunder Ball, the retuned support
  spells) could crash — a 32-bit truncation specific to Windows builds (Linux was never
  affected). Latent since Tactical Mode shipped in 1.3.0; found in 2.0.0's Windows
  validation and fixed, with a full audit of the port for the same defect class
  ([width-bugs.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/width-bugs.md)).
- Internal hardening from the two-region port work: object-pool overflow and a corrupt GPU
  ordering table now fail soft instead of crashing or hanging, and a reconstructed animation
  table's safety padding is sized from the shipped game data instead of an estimate.

### Compatibility

- **Existing installs upgrade in place**: saves, `vandalhearts.ini` and the flat 1.x `hdpacks/`
  layout all keep working untouched.
- **Saves are per-region** (matching the real consoles' different memory-card formats): the
  US/Asia and Japanese games keep separate save files, and the DISC switch does not carry
  progress across.
- New `vandalhearts.ini` keys: `VH_REGION`, `VH_DISC_ID` (written by the DISC row) and
  `VH_FAST_BOOT`. Full reference:
  [OPTIONS.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/OPTIONS.md).

## [1.7.1] — Language packs: the story videos, subtitled

The translation framework's one remaining gap is closed: the burned-in English narration of the
**story videos** (the six chapter intros and both endings) can now be subtitled by a language pack.
As always, the base build is unchanged without a pack — English players keep the original videos
exactly as they were.

### Added

- **Movie subtitles.** A pack can translate the narration of all eight story videos: the renderer
  covers the burned-in text and draws the translation in the game's large font, at the original
  narration's own on-screen size — identically with and without the HD pack, at every INTERNAL RES
  setting. Cue timings were validated frame-by-frame against the retail videos, so a translator
  only writes text. Subtitles are a **per-line diff**: an untranslated line keeps its burned-in
  English, so a partially subtitled pack is still a working pack. The ending's credit roll stays
  English by design (names and universally understood roles). Player guide:
  [language-packs.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/language-packs.md#subtitled-story-videos).
- **Toolchain: a fifth export.** `lang_export_cues.py` installs the per-video subtitle templates
  into the working set; each cue carries the English reference next to the translator's field.
  Subtitles may be written in **natural mixed case** even in a capitals-only script pack — the
  builder folds them correctly. The template tool counts subtitle letters in its art report, and
  the build verifies every subtitle character has a glyph before a pack ships.
  [Quickstart](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/tools/langpack/quickstart.md)
  and
  [reference](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/tools/langpack/README.md)
  updated, with new captures.

### Changed

- **Save-management hardening** *(from the project's first community code contribution — thanks
  to Christopher Ball)*. Backups and restores are now **atomic**: a crash or full disk mid-restore
  can no longer corrupt the active save card. Every backup is **checksum-validated** (the game's
  own save-card CRC) before a restore or inspection, so a damaged backup is refused with a clear
  message instead of being copied over your saves. The SAVES overlay gained a **status line**
  ("BACKUP CREATED", "RESTORE FAILED", …), and the 64-backup list limit is gone.
- **Disc auto-discovery improvements** *(same contribution)*. Candidate `.bin` files are verified
  by their boot signature, so a folder with a **multi-track dump** now finds the data track
  instead of failing on the first file, and discovery is case-insensitive.

### Compatibility

- Packs built before 1.7.1 keep working (they simply have no subtitles); packs built with the
  1.7.1 toolchain load on older builds too — the subtitle data is skipped there. To *gain*
  subtitles, rebuild the pack with the current toolchain and fill the new `strings/cues/` files.

## [1.7.0] — Language packs: play in your language

The port can now display the game in another language, loaded from an optional **language pack**
beside the executable. The base build still ships no game text and behaves identically without a
pack; normal mode remains byte-for-byte the retail game. Packs are a community effort — this
release ships the engine and the complete authoring toolchain, not translations.

### Added

- **Language packs.** Install a pack under `langpacks/<name>/` and pick it with the new
  **LANGUAGE** option (SELECT+START overlay, System) — the one overlay setting that is not live:
  the choice is marked `*` and **applies at the next launch**. A pack covers everything the game
  draws: story dialogue, menus, battle messages, item/spell names and descriptions, character,
  class and terrain names, the text hardcoded in the game's code, Tactical Mode's text, and the
  save-slot captions (translated at display time, so save files stay language-neutral and
  portable). A pack is a *diff* — untranslated entries simply stay English. Player guide:
  [language-packs.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/language-packs.md).
- **Non-Latin scripts.** Accented Latin (French, Italian, Portuguese, German, Spanish…) renders
  with no extra art — letterforms are composed from the game's own font. Scripts the game has
  never drawn ship their own glyph sheets, rasterised from GNU Unifont by the tooling and
  hand-tweakable: **Cyrillic and Greek are proven in game**. Packs can also opt into true
  **mixed-case** rendering (`--mixed-case`) instead of the retail ALL-CAPS folding.
- **Longer item names (pack format 2).** A pack can store item names as 16 characters instead of
  the retail 8, rendered through the small font across the shop, depot and battle screens — the
  difference between `MEGAHERB` and a real translation. A build that doesn't understand a pack's
  format refuses it loudly and keeps English — never garbled text.
- **Localized backgrounds.** A pack can replace backgrounds with text baked into the art (the
  title card, signposts) with translated versions — same content-hash mechanism as the HD pack,
  and a translated background takes priority over the HD one. They render at **every INTERNAL RES
  setting** (at 1× the art is downscaled to native resolution) and need nothing from the player —
  no HD pack required.
- **The authoring toolchain**
  ([hands-on quickstart](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/tools/langpack/quickstart.md) ·
  [reference](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/platform/pc/tools/langpack/README.md)):
  export the game's complete text from your own disc into a translator working set (every entry
  carries its English source and its display limit), translate, merge, validate against the
  engine's real limits (hard rules + on-screen fit lint), and build. Non-Latin packs derive and
  rasterise exactly the glyphs a translation needs. Game text is never committed to the
  repository — everything regenerates from the disc.

### Changed

- **The console is quiet by default.** Recurring per-event lines (per-table pack loads, per-scene
  HD replacements, per-movie decoder open/close) are now behind `VH_VERBOSE=1` (env or ini). The
  boot summary and **every warning** still always print — a refused or damaged pack says so loudly.
- The issue template asks about language packs, and the documentation covers them throughout (the
  Player Manual, configuration reference, feature guide, and a
  [translator entry point](https://github.com/HalmyLyseas/VandalHearts-PcPort#where-to-start) in
  the README).

## [1.6.2] — Corrupted disc images are detected instead of hanging

A small maintenance release, prompted by the first user bug report after release. No gameplay
changes; normal mode remains byte-for-byte the retail game.

### Fixed

- **A damaged disc image no longer hangs the game silently.** A `.bin` that was truncated
  (interrupted copy/download) or carried garbage sectors (bad rip) used to pass the quick boot
  check and then freeze the game — typically right after the `[HD] pack detected` console line,
  with no error at all. The port now validates the image three ways: the file size must be a whole
  number of raw CD sectors at startup, and every sector read during play is checked for the CD
  sync pattern and its own embedded sector address. A damaged image now stops with a clear message
  naming the exact broken sector (see
  [troubleshooting.md](https://github.com/HalmyLyseas/VandalHearts-PcPort/blob/master/docs/troubleshooting.md#the-game-wont-start)) instead of hanging. Intact
  images are unaffected — the checks are structural, so any valid dump still works, and their cost
  is unmeasurable (a 15-byte compare on data already being read).

## [1.6.1] — Point release: full-speed fast-forward, smoother HD, and a Player Manual

A maintenance and polish release. No gameplay changes; normal mode remains byte-for-byte the retail
game.

### Fixed (both modes)
- **Battle fast-forward now holds the full 2× at every graphics setting.** A frame-pacing flaw made
  battles at high `INTERNAL RES` sleep away part of each fast-forwarded frame (about 53 of the
  targeted 60 ticks/sec at ×4). The pacing now spends the whole frame budget; measured 60/60 with
  headroom on a worst-case battle scene.
- **HD pack scene loads no longer dip the frame rate.** HD backgrounds decode on a background
  thread; the brief 60→55 fps dip on entering a scene is gone (worst case validated at fullscreen
  ×4).

### Changed
- **The HD pack format is now v2 — re-download the pack.** The pack manifest now declares the FMVs
  too; the game verifies the pack's contents at startup (a missing movie is reported instead of
  silently playing the original), and the HD PACK option explains itself when a pack can't be used
  (`NO PACK` / `OUTDATED PACK` / `WRONG GAME`). The 1.6.0 pack download is withdrawn; this
  release's pack (same art, current manifest) is the supported one.
- **The Linux AppImage shrank from 65 MB to 14 MB** (the movie decoder is now built into the
  executable instead of bundling a system multimedia stack — same approach as Windows).

### Added
- **A Player Manual (PDF)** ships with each release: setup on Windows and Linux, controls, every
  feature illustrated, and a quick problem table.
- **Reorganized documentation**: the feature guide is now organized by feature with current
  screenshots, plus new [troubleshooting](docs/troubleshooting.md) and
  [performance](docs/performance.md) pages.
- Quality-of-life for diagnostics: the per-second FPS meter is now opt-in (`VH_FPS_LOG`, settable
  in the ini), joined by frame-budget and display-path meters (`VH_FRAME_TIME`,
  `VH_PRESENT_TIME`) — the first stops for any performance report.
- Developer-facing: an in-repo regression harness (a 7-second headless boot test and a byte-exact
  rasterizer golden-image check), a build-system parity guard, warning-clean builds on all
  targets, a release checklist, and contributor/issue templates.

## [1.6.0] — Optional HD pack: backgrounds + movies

An **optional** HD layer for the two pre-rendered parts of the game — the 320×240 backgrounds and the FMV
movies. Everything here is inert unless you install a pack: the **source tree and base build ship no HD
art** — a pack is either built from your own disc or downloaded as the optional asset on this release — it
is auto-detected in `hdpacks/` beside the executable, and without one the build renders exactly as before.
Hand-drawn pixel art (portraits, sprites, UI, fonts) stays native by design. Applies to both modes; normal
mode is still byte-for-byte the retail game.

### Added (both modes)
- **HD backgrounds** — the pre-rendered backgrounds are replaced with higher-resolution images at render
  time (no change to layout, palette or UI). Each is keyed by a content hash of the VRAM upload, so a pack
  maps 1:1 with no play-through. Enabled by the new **HD PACK** options row (auto-detected, defaults on,
  persisted as `VH_HDPACK`); enabling it raises the internal resolution so the detail is visible.
- **HD movies** — the intro/ending FMVs can be replaced with HD **H.264/HEVC** re-encodes, presented in
  place of the native MDEC video while the game keeps its original frame timing and XA audio in sync (only
  the picture is swapped).
- **Pack tooling** (`platform/pc/tools/hdpack/`) — build a complete pack offline from your own disc:
  background hashing + assembly, movie sector-mapping, a manifest generator, and an image finishing step.
- **New dependencies, on by default** — **libwebp** (`.webp` backgrounds, ~20 MB vs ~150 MB) and **libav /
  ffmpeg** (`.mp4` movies). Build without either via `NO_WEBP=1` / `NO_HDVIDEO=1` (Make) or `-DVH_WEBP=OFF`
  / `-DVH_HDVIDEO=OFF` (CMake); the Windows build links a minimal **static** libav, so it ships **no extra
  ffmpeg DLLs**.

### Fixed
- **Battle fast-forward** now works in every battle. It previously did nothing in a normal map-initiated
  battle (the speed gate recognised only loaded-save and dojo/trial battles, not the story-battle state);
  it is now tied to the game's own battle-tick dispatch, so R2 works everywhere a battle runs. *(This is a
  fix to the 1.4 feature and applies with or without an HD pack.)*
- **HD PACK toggle** — toggling HD on mid-scene no longer briefly shows the previous scene's HD background;
  a background uploaded while HD was off now correctly evicts the stale image. HD takes effect on the next
  background load; toggling off is immediate.

### Notes
- The **source tree commits no copyrighted art**. The optional HD pack — upscaled derivative art — is
  offered as a **separate release download** (`VandalHearts-<tag>-hdpack.zip`) that you can equally build
  yourself from your own disc; no ownership is claimed and it will be withdrawn on request (see
  [NOTICE](NOTICE) / [DISCLAIMER](DISCLAIMER)). Install/build instructions in
  [docs/hd-pack.md](docs/hd-pack.md); before/after shots in
  [docs/gameplay-additions.md](docs/gameplay-additions.md).

## [1.5.0] — Graphics fidelity: accurate rasterizer + internal-resolution supersampling

A graphics-fidelity release, kept deliberately conservative — *sharpen without reinterpreting*. The art,
sprites, videos, camera feel and gameplay are untouched; the same frames are rendered more precisely, and
optionally at a higher internal resolution. Everything here applies to both modes.

### Added (both modes)
- **Hardware-accurate software renderer** — a fixed-point integer rasterizer that evaluates coverage
  *and* texture UVs at the exact GPU pixel positions, with ordered dithering (gated on the GPU dither-
  enable bit) and a 5-bit transparency blend. ~99.8–99.99% pixel-exact vs a reference-emulator video-
  memory capture. On by default (`VH_ACCURATE`); a softer legacy renderer remains via `VH_ACCURATE=0`.
- **Internal-resolution supersampling** — render the 3D at **1× / 2× / 3× / 4×** for crisper terrain and
  edges with no re-authored art, set live in the options overlay (**INTERNAL RES**) and saved to
  `vandalhearts.ini` (`VH_INTERNAL_SCALE`). Built-in *crust-free* tile sampling removes the tile-seam grid
  while keeping 2D UI/text pixel-aligned.
- **Multithreaded rasterizer** — the higher-resolution pass is split across CPU cores (`VH_RASTER_THREADS`,
  automatic), so 4× still holds the 30 fps cap and battle fast-forward stays effective on a multicore CPU.

### Fixed (both modes)
- The Chapter 2 casting-ray effect that read denser than hardware at native resolution (flagged in 1.4) is
  resolved by the accurate rasterizer.

### Notes
- Cost scales ~N² with the factor: 2× is nearly free, 3×/4× are heavier. Normal mode stays byte-for-byte
  the original. Configuration: [docs/configuration.md](docs/configuration.md).

## [1.4.0] — Quality of life: fast-forward, controller labels, smarter AI, camera

A quality-of-life release. Everything here is either a convenience that applies to both modes or, for the
one AI change, a Tactical-only improvement. Normal mode stays byte-for-byte the original.

### Quality of life (both modes)
- **Battle fast-forward** — during a battle, tap **R2** (or the `.` key) to run at **2× speed**; **L2**
  (or `,`) returns to normal, with a small `BATTLE SPEED X2` readout while it's active. It only speeds up
  battles (menus, world map and cutscenes stay normal) and resets to 1× when the battle ends. It runs
  *whole* game steps closer together, so the AI, RNG and every outcome are **identical** to normal speed.
- **Controller-aware overlay labels** — the port's own overlay (save management / options) now shows
  button prompts that match your controller: **Xbox** letters (A / B / X / Y) or **PlayStation** symbols
  (□ ○ △ ✕), switchable in the overlay and saved to `vandalhearts.ini`. The game's own prompts are
  untouched.
- **Finer camera elevation** — the battle camera's up/down angle (right stick) gains a fifth,
  evenly-spaced stop **including a clean 45°**, for finding a readable angle on stepped terrain.

### Tactical Mode (opt-in)
- **Magic-aware enemy AI** — enemy spellcasters now weigh **magic resistance** when picking targets,
  preferring magic-weak units and avoiding resistant or buffed ones. Retail's AI ignores this entirely;
  the change lets the magic rebalance's resistances and defensive buffs actually matter to the enemy.

### Notes
- The Chapter 2 casting-ray effect still reads slightly denser than hardware at native resolution — now
  root-caused (see [known issues](docs/known_issues.md)); the fix is a higher-fidelity rasterizer pass
  planned for a later release.

## [1.3.1] — Tactical Mode tuning + fixes

The full campaign has now been playtested end to end; this release folds in the resulting balance tuning
and fixes, plus two rendering/stability fixes that apply to both modes. Normal mode remains byte-for-byte
the original.

### Fixed (both modes)
- **Spell & cutscene effects** — certain textured effects (energy rays, dissolve/fade planes, warp
  surfaces) were rendering as flat semi-transparent shapes with their texture dropped; they now show
  their proper textured appearance.
- Fixed a rare crash when the on-screen attack marker (miss / support / poison) appeared over certain
  map tiles — most likely on the long, narrow Trials corridor.

### Quality of life (both modes)
- **Enemy threat overlay** — reachable-and-threatened tiles now show in a distinct **orange** (they
  previously shared yellow with spell/attack targeting), so a spell's AoE preview stays readable even
  inside a danger zone. Overlay tints were also softened for easier reading over any terrain.

### Tactical Mode (opt-in)

**Progression**
- **Level-cap curve retuned** to **10 / 15 / 19 / 24 / 28 / 32** — a smoother, steady climb that keeps
  the party a step under the cap heading into each chapter's Trials and lands the endgame party right on
  the final boss's level. (Validated across a full playthrough.)
- **Trials of Toroah** — fixed the final Trial not awarding attack experience.

**Casters & spells**
- **Monk / Ninja rework** — the path now learns a genuine **Spread Force** and a retuned **Thunder
  Flash**, and *keeps* its inherited base spells through both promotions (they're no longer shed at
  Ninja), so the class stays flexible rather than losing utility.
- **Mystic Energy** reworked into an **area-of-effect party buff** — a defensive group screen that
  raises defense and grants **magic resistance** to every ally in the field, with a new icy-blue aura and
  a per-ally cast effect. Cost tuned so a second back-to-back cast needs an MP refill.
- **Thunder Ball** added to the mage kit — a long-range, small-area attack spell — and **Roman Fire**
  buffed (power 7 → 9) so it's a real pick alongside Phase Shift rather than strictly outclassed.
- **Perfect Guard** — cheaper (**12 MP**) and now also grants **magic resistance**, making it a
  single-unit "evasion + anti-magic" shield.
- **Healing Circle / Healing Wave** retuned into distinct roles — Circle is a cheap, efficient group
  top-up; Wave is the heavy group heal.
- Fixed **Monk / Ninja maximum MP** overshooting its intended value.

**Visuals**
- **Avalanche** re-skinned as an *ice* avalanche in Tactical (an icy boulder), matching the spell's
  intended theme.

**Changed since 1.3.0**
- The two "restored cut weapons" from 1.3.0 have been **removed** — they turned out to be unique enemy
  boss weapons, so they stay boss-exclusive. (Restored item description text is unchanged.)

## [1.3.0] — Tactical Mode + fixes

### Fixed (both modes)
- Fixed a crash that could occur when transferring items to or from the convoy/depot.
- Fixed a crash that could occur during certain cutscenes.

### Quality of life (both modes)
- **Return to Title** — a new options-overlay entry to jump straight back to the title screen from
  anywhere, without restarting the app or sitting through the intro. Confirms first (unsaved progress is
  lost).
- **Enemy threat overlay** — now consistent at every camera angle (it no longer leaks onto off-field
  tiles, nor drops from genuinely-threatened tiles, when you rotate), and its pulse is softer and easier
  on the eyes over large danger zones.
- **Save management** — destructive prompts (restore, delete) now show their question in red; the
  inspect action is relabeled **"Inspect file content"** for clarity.
- Fixed the **Fullscreen** option being un-toggleable in the overlay.

### New — Tactical Mode (opt-in, in testing)
An optional, fully-isolated rebalance for a more varied tactical experience — a per-chapter level cap
that ends experience-grinding, Trials of Toroah that scale to your chapter and reward gold + XP, class
reworks (Monk/Ninja, Guardsman/Dragoon mobility), a reined-in Vandalier, and restored cut content (item
descriptions, two cut weapons). **Off by default**; the normal mode stays byte-for-byte the original, and
Tactical saves are kept separate. See the [Tactical Mode guide](docs/tactical-mode.md).

> ⚠️ **In testing** — later chapters are still being validated in playtest; balance numbers may change in
> 1.3.x. Normal mode is unaffected.

## [1.2.0] — Video options + save management
- **Video** — window scale (X1–X8) and fullscreen from the in-game overlay, applied live.
- **Save management** — unlimited whole-card backups in the overlay: back up, restore (with a "back up
  first" safe default), delete, and inspect a backup's three slots. Each backup stays a
  real-hardware-valid card.

## [1.1.0] — Controls
- Bidirectional unit-cycle on the shoulder buttons (freeing the Square button).
- **Enemy threat overlay** — press Square to see every enemy's combined move-and-attack reach at once.
- In-game options overlay (**Select + Start**), with right-stick camera-axis invert.
- Saves and config now live next to the executable, independent of the launch directory.

## [1.0.0] — Faithful port
- Complete beginning-to-end experience on Windows and Linux, from your own disc.
- Skip intro / movies with Start; camera rotation and elevation on the right analog stick.
