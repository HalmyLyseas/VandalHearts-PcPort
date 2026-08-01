# PC port — runtime options & build flags

Two kinds of knobs:
- **Environment variables** (`VH_*`) — set at launch, no rebuild: `VH_SCALE=3 ./build/vandalhearts_pc`
- **Make flags** — compile-time, passed to `make link`: `make link AI_LOG=1`

Some diagnostics need *both* (a make flag to compile the hook in, then an env var to turn it on at
runtime). Those are marked "**build:** `FLAG=1`" below. All debug logs are written to the current
directory (`vh_*.csv` / `.txt` / `.log`, all gitignored).

## Everyday options

| Variable | Default | Effect |
|---|---|---|
| `VH_DISC_IMAGE` | auto-detected (a `game/` folder or a `*.bin` next to the exe/AppImage; dev fallback `../../../external/game/Vandal Hearts (USA).bin`) | Path to your game disc `.bin`; overrides auto-detect. |
| `VH_SCALE` | `2` | Integer window scale (clamped 1–8). Aspect-preserved. Also settable live in the in-game options overlay (SELECT+START). |
| `VH_FULLSCREEN` | `0` | Start in fullscreen-desktop (`1`) vs windowed (`0`). Aspect preserved (letterboxed). Also toggleable in the options overlay. |
| `VH_ACCURATE` | `1` | PSX-accurate software rasterizer — a fixed-point integer DDA (coverage + UV at the exact GPU pixel position), ordered dithering (gated on the GPU dither-enable bit), 5-bit blend. ~99.8–99.99% pixel-exact vs a DuckStation VRAM capture. On by default. `0` = legacy renderer (softer, no dithering); advanced users only. |
| `VH_INTERNAL_SCALE` | `1` | **Internal-resolution supersampling** (1.5/G2). `1` = off (native render). `2`–`4` render the software GPU at that multiple: each primitive is rasterized a second time into an Nx-larger buffer (geometry ×N, native texture sampled per hi-res pixel at the hi-res pixel centre), for crisper terrain/edges without re-authoring any art. Needs `VH_ACCURATE=1` (the default). Cost is roughly N² × the native rasterizer (dual-pass), but the hi-res pass is **multithreaded** (see `VH_RASTER_THREADS`), so on a multicore CPU `2`/`3`/`4` all hold the 30 fps cap and battle fast-forward stays effective at `4`×. Also settable live in the options overlay ("INTERNAL RES", X1/X2/X3/X4) and persisted. |
| `VH_RASTER_THREADS` | auto | Worker-thread count for the internal-resolution rasterizer (1.5/P1). Default = online CPU count (auto-detected via SDL, capped at 32). Set a number only to **cap** it (e.g. `4` to leave cores free). Ignored at `VH_INTERNAL_SCALE=1` (nothing to parallelize) or for a tiny display list (falls back to single-threaded). Bands write disjoint hi-res scanlines → lock-free; output is bit-identical to single-threaded (harness-validated). |
| `VH_HIRES_INSET` | `1` | **Tile-edge inset** (1.5/G2), in texels. At internal res > 1, the finer sampling would otherwise land on the dark "crust" texel at each tile's texture-cell border — native samples the tile interiors, hi-res the borders — producing a faint dark grid along terrain/lava/water tile seams (and the compass "dotted lines"). The inset clamps the hi-res sample to the tile interior (`[uMin+n, uMax-n]`), matching native and removing the grid; interior stays sharp (unlike a blur filter). `0` disables (raw grid returns). Hi-res pass only (`rc->target`) so native is byte-exact; auto-tied to `VH_INTERNAL_SCALE>1`. Root-caused texel-exact (native bright-lava vs hi-res `CLUT[0]` gray at seams) and offline-validated on the real texture. **Minor cosmetic:** small high-contrast glyphs (compass E/W/S/N) fatten ~1px, since their edge texel is content rather than crust — no clean way to distinguish the two, so accepted as a documented trade for the whole-scene grid fix. |
| `VH_CAM_INVERT_X` | `0` | Invert the right-stick camera *horizontal* axis (rotate direction). `1` = inverted. Also toggleable in the in-game options overlay (SELECT+START). |
| `VH_CAM_INVERT_Y` | `1` | Invert the right-stick camera *vertical* axis (raise/lower angle). **Ships inverted** — the modern twin-stick convention (push up = tilt the view down); set `0` for a normal vertical axis. Also toggleable in the in-game options overlay. |
| `VH_SEQ_MUTE` | off | `1` = mute only the SEQ **music**, keep VAG SFX + XA audible (useful for isolating sound effects). |

> **Audio is calibration-fixed** — the software SPU matches real-hardware output at the authentic
> values, so the tuning env vars are **not exposed** in `vandalhearts.ini` and are *not* user knobs.
> They remain in code for developer A-B only: `VH_SPU_GAIN` (`1.012`, master trim RMS-matched to the
> octoshock reference within 0.01 dB), `VH_SPU_SQUARE` (on = PsyQ's square volume law `VolL=L*L/16383`
> at `0x800d6d8c` — the whole reason the mix has the right dynamic range; `0` is the old linear A-B and
> drops gain to `0.24`), `VH_SPU_ANALOG` (off = legacy EQ tilt that over-corrects once the square law
> is present: 2.20 dB → 4.27 dB mean error). See `exchange/57` for the derivation.

## Compatibility

| Variable | Default | Effect |
|---|---|---|
| `VH_NULL_FIXUP` | on | The portable NULL-read / rodata-write fault handler that lets the game run **cap-less**. Set `0` only to fall back to the legacy privileged zero-page mapping (then `make setcap` is required, and un-guarded NULL reads hard-crash). Leave unset for normal use. |

> With the fixup handler on (default), you do **not** need `setcap`. `make setcap` / `make link-cap`
> are only for the `VH_NULL_FIXUP=0` fallback.

## Diagnostics — logging

| Variable | Build flag | Output | Logs |
|---|---|---|---|
| `VH_AI_LOG` | `AI_LOG=1` | `vh_ai_log.txt` | AI spell-target scoring: every candidate a caster evaluated + term breakdown (level / HP / `gAdvantage` type-matchup / terrain). Highest score = the pick. |
| `VH_XA_LOG` | — | `vh_xa_log.txt` | CD-XA seek/read events (`CdlSeekL`/`CdlReadN`, LBA, same-track replays). |
| `VH_XA_CSV` | — | `vh_xa_ours.csv` | Per-frame XA state (xaId, duration, stream/queue/source state) for diffing vs the BizHawk track map. |
| `VH_SEQ_LOG` | — | `vh_seq_log.txt` | SEQ music diagnostics. `=1` adds a ~6×/sec voice-census dump (active-voice count, note-on/off balance, master vol, timestamp). Low-volume lines — first-96 `[note]` events, stuck-note-reaper `[reaper]` events, `SsSeqStop` — are always logged (file auto-created), so a hung-note recurrence is captured without the flag. |
| `VH_SPRITE_LOG` | `SPRITE_LOG=1` | `vh_sprite_fate.csv` | Per-unit-sprite cull/projection/GTE state. |
| `VH_TERRAIN_LOG` | `TERRAIN_LOG=1` | `vh_terrain_otz_pc.csv` | Per-frame terrain `otz` stats + black-tile counts. |
| `VH_CAM_OSD` | — | on-screen | Camera-pose overlay (position/rotation/zoom) drawn in the window. |
| `VH_NULL_FIXUP` handler | — | `vh_null_reads.log` | Auto: each NULL-region read / rodata-write the handler fixed up (see `make crash-trace`). |

Advanced GTE/GPU render probes (developer, mostly gated by `SPRITE_LOG=1`): `VH_MTX_LOG`,
`VH_OBJPRIM4_LOG`, `VH_SPRITE_QUAD_LOG`, `VH_TERRAINPROJ_LOG`.

## Diagnostics — audio isolation

For chasing a single instrument in the music mix. Spectral attribution misidentified the culprit
twice before these existed (`exchange/57`); soloing settles it in one run.

| Variable | Effect |
|---|---|
| `VH_SPU_SOLOPROG=N` | Play **only** VAB program `N` in the SEQ music. Everything else silent. |
| `VH_SPU_MUTEPROG=N` | Silence VAB program `N`, keep the rest. Usually the faster test — if the offending sound vanishes, that's your instrument. |
| `VH_SPU_DUMPVAG=1` | On VAB load, dump every decoded sample as `vh_vag_<vab>_<n>.wav` (44.1 kHz mono) plus `vh_vag_manifest.csv` (len / loopS / loopE). This is the PCM we *actually* play — use it instead of parsing the VAB body out of the disc image, which does not give the same offsets. |
| `VH_SPU_TRACE=1` | Per-voice census to `vh_spu_voices_ours.csv` / `vh_spu_globals_ours.csv` (vag, prog, note, step, gains, ADSR level/phase, loop points). |
| `VH_SPU_REVDEPTH` / `VH_SPU_REVOFF` | Override / disable the SPU reverb send, for isolating reverb from the dry mix. |

## Build flags (`make link <FLAG>=1`)

| Flag | Effect |
|---|---|
| `AI_LOG=1` | Compile the AI target-scoring hook (then enable with `VH_AI_LOG`). |
| `SPRITE_LOG=1` | Compile the sprite-fate hook (then `VH_SPRITE_LOG`). |
| `TERRAIN_LOG=1` | Compile the terrain-otz hook (then `VH_TERRAIN_LOG`). |
| `NO_FADE=1` | Disable the screen fade overlay (study builds). |
| `NO_LOADING=1` | Skip the "Now Loading" screen. |

Debug hooks compiled into `src/` are gated behind `PC_DEBUG_*` and are **only** added by these flags,
so the matching decomp build (`make check`) never sees them and stays byte-exact.

## Helper targets

| Target | Purpose |
|---|---|
| `make link` | Build the PC binary (cap-less; no setcap needed). |
| `make crash-trace` | Resolve each site in `vh_null_reads.log` to `file:line` (nm + addr2line). |
| `make setcap` / `make link-cap` | Legacy only — for the `VH_NULL_FIXUP=0` fallback. |
