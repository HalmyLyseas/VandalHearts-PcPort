# Known Issues

A short, honest list of the defects and limitations we're already aware of — so you can check here
before filing a report. If you hit something that **isn't** listed, please do report it.

Some background on scope: the game's original logic is reproduced byte-for-byte, and the normal
(non-Tactical) mode is unaffected by the gameplay work. The items below are almost entirely in the
**port's rendering/backend layer** (Stage 2 audio/visual fidelity) and therefore apply to both modes,
unless noted otherwise.

## Graphics

### A casting "ray" effect reads slightly denser than on hardware

- **What you'll see:** the blue energy "zip" casting effect — most noticeable in a **Chapter 2 cutscene**,
  and on some casts (e.g. Thunder Flash). Compared against real hardware **at native resolution**, the
  port's version reads a touch **denser / more saturated**, and slightly less cleanly organised into the
  spiral, than the original. It is timing/RNG-driven, so its exact appearance varies from run to run.
- **Status:** **minor cosmetic difference — non-blocking.** This was previously described as a stark
  "blocky additive cloud vs. thin translucent rays" difference — but that comparison turned out to be
  partly an **artifact of the emulator's upscaling/filtering** (a smoothed, modern-looking reference).
  Against a faithful **native, unfiltered** capture the two are close — both are pixelated translucent
  blue streaks — and only a subtle density/coherence gap remains. An earlier, more visible bug (these
  effects dropping their texture entirely) was **fixed in 1.3.1**.
- **Root cause (identified):** every *structural* cause matches the original — primitive type, geometry,
  texture UVs, colour palette, per-pixel transparency, blend mode, texture windowing. The effect is 16
  overlapping **additive** quads per frame, each sampling a thin, mostly-transparent streak texture that is
  **minified ~5× vertically** on screen. Point-sampling a minified thin-streak texture is phase-sensitive:
  our software rasterizer's texel selection differs slightly from the PS1 GPU's (its exact fixed-point
  texture stepping and thin-polygon coverage), so it catches a marginally denser set of texels, and 16
  additive layers accumulate that into the visible density gap. It affects both modes and is purely visual.
- **Fix:** a PS1-accurate software-rasterizer pass (exact texture stepping + coverage) is the anchor feature
  of the planned higher-fidelity graphics release — see the [roadmap](roadmap.md). Non-blocking until then.

## Not bugs — by design

- **Avalanche debris (Tactical Mode).** The Tactical ice re-skin re-textures only the main boulder; the
  tumbling debris behind it is drawn from fixed rock sprites and stays rocky by design (a full snow
  effect would need new artwork). See [tactical-mode.md](tactical-mode.md#avalanche--an-ice-re-skin).
- **macOS.** Only **Windows** and **Linux** are supported. macOS is scaffolded in the build but not
  built or tested — see [cross-platform.md](cross-platform.md).

---

*Reporting anything else:* open an issue with the location (chapter/battle), what you expected vs. what
you saw, and a screenshot if it's visual. For rendering issues, a comparison against a screenshot from
original hardware or an accurate emulator is especially helpful.
