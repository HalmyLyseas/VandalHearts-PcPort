# Known Issues

A short, honest list of the defects and limitations we're already aware of — so you can check here
before filing a report. If you hit something that **isn't** listed, please do report it.

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

### Rare blank text or static minor effects are possible on untested paths

- The port reconstructs the game's statically-initialized data tables from your own game copy at build
  time. Pointer-typed tables need individual reconstruction, and while every instance found so far has
  been fixed (blank spell/item descriptions, frozen arrows/effect shadows, invisible cutscene extras —
  all resolved in past releases), a systematic audit of the remaining pointer-typed globals is planned.
  If you ever see **blank description text, a missing minor visual, or a small effect that doesn't
  animate**, please report it with the location — it is almost certainly this class, and fixes are
  quick once located.

## Performance

### Brief frame dips when a scene loads with the HD pack (v1.6)

- With an HD pack installed, entering a new screen decodes that scene's HD background (and opens the HD
  movie, for FMVs) on the render thread, which can dip the frame rate from 60 to ~55 for a moment on
  scene entry. Gameplay is unaffected. Moving the decode to a background thread is planned. Without an
  HD pack there is no dip.

## Not bugs — by design

- **Avalanche debris (Tactical Mode).** The Tactical ice re-skin re-textures only the main boulder; the
  tumbling debris behind it is drawn from fixed rock sprites and stays rocky by design (a full snow
  effect would need new artwork). See [tactical-mode.md](tactical-mode.md#avalanche--an-ice-re-skin).
- **HD PACK toggle timing (v1.6).** Turning HD **on** mid-scene takes effect from the next
  screen/background load (each background is identified as the game uploads it); turning it **off** is
  immediate. See [hd-pack.md](hd-pack.md).
- **macOS.** Only **Windows** and **Linux** are supported. macOS is scaffolded in the build but not
  built or tested — see [cross-platform.md](cross-platform.md).

---

*Reporting anything else:* open an issue with the location (chapter/battle), what you expected vs. what
you saw, and a screenshot if it's visual. For rendering issues, a comparison against a screenshot from
original hardware or an accurate emulator is especially helpful.
