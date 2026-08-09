# Known Issues

A short, honest list of the defects and limitations we're already aware of — so you can check here
(and [troubleshooting.md](troubleshooting.md)) before filing a report. If you hit something that **isn't** listed, please do report it.

Some background on scope: the game's original logic is reproduced byte-for-byte, and the normal
(non-Tactical) mode is unaffected by the gameplay work. The items below are almost entirely in the
**port's rendering/backend layer** (Stage 2 audio/visual fidelity) and therefore apply to both modes,
unless noted otherwise.

## Graphics

### ~~A casting "ray" effect reads slightly denser than on hardware~~ — RESOLVED

- **Resolved by the PS1-accurate software rasterizer (the `VH_ACCURATE` default).** The rasterizer now
  uses a fixed-point integer DDA that evaluates coverage *and* texture UVs at the exact pixel position the
  PS1 GPU does — the density gap came from our old rasterizer point-sampling the minified thin-streak
  texture a marginally different way than the console. The renderer is validated **99.99% (the Chapter-2
  cutscene) / 99.84% (a battle field) pixel-exact vs a DuckStation VRAM capture**, so the casting effects
  now select the same texels as hardware. (A separate, more visible bug where the effect briefly showed a
  solid garbage blob — an LP64 struct-aliasing OOB — was fixed at the same time; see
  [width-bugs.md](width-bugs.md) #3c.) Set `VH_ACCURATE=0` for the legacy renderer if desired.

## Data reconstruction (port layer)

### Rare blank text or static minor effects — class audited, believed closed

- The port reconstructs the game's statically-initialized data tables from your own game copy at build
  time. Pointer-typed tables need individual reconstruction; past instances (blank spell/item
  descriptions, frozen arrows/effect shadows, invisible cutscene extras) were all fixed in earlier
  releases. A **systematic audit of every remaining pointer-typed global (2026-08-03)** checked each
  against its initializer in the original executable: all with real initializers are covered by
  existing reconstructions, and the rest are zero on the PlayStation too (assigned at runtime). No
  uncovered case remains. If you nonetheless see **blank description text, a missing minor visual, or
  a small effect that doesn't animate**, please report it with the location — it would be this class,
  and fixes are quick once located.

## Performance

### ~~Brief frame dips when a scene loads with the HD pack~~ — RESOLVED in v1.6.1

- **Fixed by moving the HD background decode to a background thread (v1.6.1).** Entering a new screen
  no longer dips the frame rate; the scene shows native texels for the one or two frames (behind the
  scene fade) it takes the decode to land, then switches to the HD image. Validated at the worst case
  (fullscreen, internal resolution ×4): no dip. If you still see scene-entry dips on v1.6.1 or later,
  please report it with your CPU model and settings.

## Not bugs — by design

- **Avalanche debris (Tactical Mode).** The Tactical ice re-skin re-textures only the main boulder; the
  tumbling debris behind it is drawn from fixed rock sprites and stays rocky by design (a full snow
  effect would need new artwork). See [tactical-mode.md](tactical-mode.md#avalanche--an-ice-re-skin).
- **HD PACK toggle timing (v1.6).** Turning HD **on** mid-scene takes effect from the next
  screen/background load (each background is identified as the game uploads it); turning it **off** is
  immediate. See [hd-pack.md](hd-pack.md).
- **Language changes need a restart (v1.7).** Selecting a language pack (the overlay's LANGUAGE
  row) marks the change with `*` and applies it **at the next launch** — there is no live language
  switch, by design: a pack rewrites structures the game builds once at boot. If the game still
  shows English after selecting a pack, restart it. See
  [language-packs.md](language-packs.md#installing-and-selecting-a-pack).
- **macOS validation.** The native Apple Silicon build is tested through the first battle and its
  surrounding game systems, but has not yet had a complete playthrough or Intel Mac validation. See
  [cross-platform.md](cross-platform.md).
- **macOS low/NULL reads can still crash.** The Linux i386 `SIGSEGV` instruction fixup is not
  implemented on macOS. Known `gStringTable` NULL/sentinel entries are normalized to an empty string
  by the PC constructor, and most other known transient reads have `PC_PORT` source guards. Any
  remaining unguarded low-pointer path will still terminate the macOS build instead of being emulated
  as a zero read. See [memory-safety.md](memory-safety.md).

---

*Reporting anything else:* open an issue with the location (chapter/battle), what you expected vs. what
you saw, and a screenshot if it's visual. For rendering issues, a comparison against a screenshot from
original hardware or an accurate emulator is especially helpful.
