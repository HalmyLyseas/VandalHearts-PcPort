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
| `VH_INTERNAL_SCALE` | `1` | **Internal-resolution supersampling** (1.5/G2). `1` = off (native render). `2`–`4` render the software GPU at that multiple: each primitive is rasterized a second time into an Nx-larger buffer (geometry ×N, native texture sampled per hi-res pixel), for crisper terrain/edges without re-authoring any art. Needs `VH_ACCURATE=1` (the default). Cost is roughly N² × the native rasterizer (dual-pass), but the hi-res pass is **multithreaded** (see `VH_RASTER_THREADS`), so on a multicore CPU `2`/`3`/`4` all hold the 30 fps cap and battle fast-forward stays effective at `4`×. Also settable live in the options overlay ("INTERNAL RES", X1/X2/X3/X4) and persisted. **"Crust-free" tile sampling** is built in (no knob): the hi-res pass biases the UV sample +0.5 texel on perspective world polys so tile edges land on the bright cell interior instead of the dark border "crust" texel — this is exactly what the reference renderer does, and it removes the faint dark grid that finer sampling would otherwise draw along terrain/lava/water tile seams (and the compass "dotted lines"), with no softening. Axis-aligned 2D UI/text/sprites are auto-detected and keep plain centre-sampling, so glyph/border columns never shift. The decision is per-quad, so a quad's two triangles always agree (no diagonal seam). Hi-res pass only — native `s_vram` stays byte-exact. |
| `VH_RASTER_THREADS` | auto | Worker-thread count for the internal-resolution rasterizer (1.5/P1). Default = online CPU count (auto-detected via SDL, capped at 32). Set a number only to **cap** it (e.g. `4` to leave cores free). Ignored at `VH_INTERNAL_SCALE=1` (nothing to parallelize) or for a tiny display list (falls back to single-threaded). Bands write disjoint hi-res scanlines → lock-free; output is bit-identical to single-threaded (harness-validated). |
| `VH_HDPACK` | auto | **HD background pack** on/off (1.6). Set automatically when a valid `hdpacks/` is auto-detected beside the executable (manifest game id must match `SLUS-00447`); toggle live in the overlay ("HD PACK", greyed with no pack) and persisted here. Enabling bumps `INTERNAL RES`→≥2 and (windowed) `WINDOW SCALE`→≥2 so the HD detail is visible; dropping either to 1 disables it. Only 8bpp backgrounds are replaced — pixel-art portraits/sprites/UI stay native. `VH_HD_PACK=<dir>` (underscore) overrides the auto-detect with an explicit `backgrounds/` folder. HD images decode on a background thread (a scene shows native texels for the first frame or two behind the fade-in); `VH_HD_SYNC=1` forces the old inline decode for A/B. See [../../docs/hd-pack.md](../../docs/hd-pack.md). |
| `VH_CAM_INVERT_X` | `0` | Invert the right-stick camera *horizontal* axis (rotate direction). `1` = inverted. Also toggleable in the in-game options overlay (SELECT+START). |
| `VH_CAM_INVERT_Y` | `1` | Invert the right-stick camera *vertical* axis (raise/lower angle). **Ships inverted** — the modern twin-stick convention (push up = tilt the view down); set `0` for a normal vertical axis. Also toggleable in the in-game options overlay. |
| `VH_SEQ_MUTE` | off | `1` = mute only the SEQ **music**, keep VAG SFX + XA audible (useful for isolating sound effects). |

> **Audio is calibration-fixed (hardwired, no knobs).** The software SPU matches real-hardware output at
> the authentic values, so there are no tuning env vars — the settings are fixed in code: master trim
> `1.012` (RMS-matched to the octoshock reference within 0.01 dB); PsyQ's square volume law
> `VolL=L*L/16383` (`0x800d6d8c`) always on (the reason the mix has the right dynamic range); the legacy
> EQ tilt off (it over-corrects once the square law is present, 2.20 → 4.27 dB mean error). Derivation:
> `exchange/57`.

## Compatibility

The game runs **cap-less** (no `setcap`, no elevated rights): the portable NULL-read / rodata-write fault
handler in `pc_bootstrap.c` catches transient PSX NULL/low-address accesses, emulates them (read 0 /
discard store), logs each site, and steps over — so no page-0 mapping is needed. This is the sole path;
the old privileged low-page-mapping fallback (and its `setcap` targets) was retired.

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
| NULL-read fixup handler | — | `vh_null_reads.log` | Auto: each NULL-region read / rodata-write the handler fixed up (see `make crash-trace`). |

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
