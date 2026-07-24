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
| `VH_SCALE` | `2` | Integer window scale (clamped 1–8). Aspect-preserved. |
| `VH_SPU_GAIN` | `1.012` | Software-SPU master output trim (float). Music/SFX loudness. Calibrated so the rendered mix RMS-matches the octoshock reference to within 0.01 dB (peak −5.6 dBFS vs the reference's −5.9, i.e. the same headroom hardware leaves for SFX). Falls back to `0.24` if `VH_SPU_SQUARE=0`. |
| `VH_SPU_SQUARE` | on | PsyQ's **square volume law** — the final stage of the real key-on chain (`0x800d6d8c`): `VolL = L*L / 16383`, applied after panning, to music *and* SFX. Quadratic, so every dB of attenuation is doubled. Without it the mix renders at half the hardware's dynamic range in dB. `0` reverts to the old linear behaviour (and drops `VH_SPU_GAIN` to `0.24`) for A/B. See `exchange/57`. |
| `VH_SPU_ANALOG` | **off** | Legacy EQ tilt (sub-bass lift + treble roll-off). **Was on by default until 2026-07-20**, when it turned out to be compensating for the missing square law — with that implemented it now *over*-corrects (mean error vs the reference: raw mix 2.20 dB, with this filter 4.27 dB). Kept as a tilt knob; `1` re-enables. Fine-tune with `VH_SPU_BASS`/`VH_SPU_TREB`/`VH_SPU_BASSFC`/`VH_SPU_TREBFC`. |
| `VH_SEQ_MUTE` | off | `1` = mute only the SEQ **music**, keep VAG SFX + XA audible (useful for isolating sound effects). |

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
