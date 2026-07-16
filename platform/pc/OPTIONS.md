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
| `VH_DISC_IMAGE` | `../../../../game/Vandal Hearts (USA).bin` (relative to the exe) | Path to your game disc `.bin`. |
| `VH_SCALE` | `2` | Integer window scale (clamped 1–8). Aspect-preserved. |
| `VH_SPU_GAIN` | `0.24` | Software-SPU master gain (float). Music/SFX loudness. |
| `VH_SPU_ANALOG` | on | PS1 analog-output coloration filter (bass/treble shaping). Set `0` to disable (flat digital). |
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
| `VH_SPRITE_LOG` | `SPRITE_LOG=1` | `vh_sprite_fate.csv` | Per-unit-sprite cull/projection/GTE state. |
| `VH_TERRAIN_LOG` | `TERRAIN_LOG=1` | `vh_terrain_otz_pc.csv` | Per-frame terrain `otz` stats + black-tile counts. |
| `VH_CAM_OSD` | — | on-screen | Camera-pose overlay (position/rotation/zoom) drawn in the window. |
| `VH_NULL_FIXUP` handler | — | `vh_null_reads.log` | Auto: each NULL-region read / rodata-write the handler fixed up (see `make crash-trace`). |

Advanced GTE/GPU render probes (developer, mostly gated by `SPRITE_LOG=1`): `VH_MTX_LOG`,
`VH_OBJPRIM4_LOG`, `VH_SPRITE_QUAD_LOG`, `VH_TERRAINPROJ_LOG`.

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
