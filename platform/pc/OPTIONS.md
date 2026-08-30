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
| `VH_DISC_IMAGE` | auto-detected (a `game/` folder or a `*.bin` next to the exe/AppImage; dev fallback `../../../external/{game,alt}/`) | Path to your game disc `.bin`; overrides the scan. Any supported release: USA `SLUS-00447`, Asia `SCPS-45183`, Japan `SLPM-86007`. |
| `VH_REGION` | `auto` (US/Asia preferred) | Which game to boot when discs of both regions are installed: `us` or `jp` (2.0). Written by the overlay's **DISC** row; applies at launch. |
| `VH_DISC_ID` | unset | Exact disc within the US family when both dumps are installed (`SLUS-00447` vs `SCPS-45183`). Written by the **DISC** row alongside `VH_REGION`. |
| `VH_SCALE` | `2` | Integer window scale (clamped 1–8). Aspect-preserved. Also settable live in the in-game options overlay (SELECT+START). |
| `VH_FULLSCREEN` | `0` | Start in fullscreen-desktop (`1`) vs windowed (`0`). Aspect preserved (letterboxed). Also toggleable in the options overlay. |
| `VH_ACCURATE` | `1` | PSX-accurate software rasterizer — a fixed-point integer DDA (coverage + UV at the exact GPU pixel position), ordered dithering (gated on the GPU dither-enable bit), 5-bit blend. ~99.8–99.99% pixel-exact vs a DuckStation VRAM capture. On by default. `0` = legacy renderer (softer, no dithering); advanced users only. |
| `VH_INTERNAL_SCALE` | `1` | **Internal-resolution supersampling** (1.5/G2). `1` = off (native render). `2`–`4` render the software GPU at that multiple: each primitive is rasterized a second time into an Nx-larger buffer (geometry ×N, native texture sampled per hi-res pixel), for crisper terrain/edges without re-authoring any art. Needs `VH_ACCURATE=1` (the default). Cost is roughly N² × the native rasterizer (dual-pass), but the hi-res pass is **multithreaded** (see `VH_RASTER_THREADS`), so on a multicore CPU `2`/`3`/`4` all hold the 30 fps cap and battle fast-forward stays effective at `4`×. Also settable live in the options overlay ("INTERNAL RES", X1/X2/X3/X4) and persisted. **"Crust-free" tile sampling** is built in (no knob): the hi-res pass biases the UV sample +0.5 texel on perspective world polys so tile edges land on the bright cell interior instead of the dark border "crust" texel — this is exactly what the reference renderer does, and it removes the faint dark grid that finer sampling would otherwise draw along terrain/lava/water tile seams (and the compass "dotted lines"), with no softening. Axis-aligned 2D UI/text/sprites are auto-detected and keep plain centre-sampling, so glyph/border columns never shift. The decision is per-quad, so a quad's two triangles always agree (no diagonal seam). Hi-res pass only — native `s_vram` stays byte-exact. |
| `VH_RASTER_THREADS` | auto | Worker-thread count for the internal-resolution rasterizer (1.5/P1). Default = online CPU count (auto-detected via SDL, capped at 32). Set a number only to **cap** it (e.g. `4` to leave cores free). Ignored at `VH_INTERNAL_SCALE=1` (nothing to parallelize) or for a tiny display list (falls back to single-threaded). Bands write disjoint hi-res scanlines → lock-free; output is bit-identical to single-threaded (harness-validated). |
| `VH_HDPACK` | auto | **HD background pack** on/off (1.6; per-region since 2.0). Set automatically when a valid pack for the running game is found under `hdpacks/<game-id>/` beside the executable (`SLUS-00447` for USA/Asia, `SLPM-86007` for Japan; the pre-2.0 flat `hdpacks/` still works as a US fallback); toggle live in the overlay ("HD PACK", greyed with no pack) and persisted here. Enabling bumps `INTERNAL RES`→≥2 and (windowed) `WINDOW SCALE`→≥2 so the HD detail is visible; dropping either to 1 disables it. Only 8bpp backgrounds are replaced — pixel-art portraits/sprites/UI stay native. `VH_HD_PACK=<dir>` (underscore) overrides the auto-detect with an explicit `backgrounds/` folder. HD images decode on a background thread (a scene shows native texels for the first frame or two behind the fade-in); `VH_HD_SYNC=1` forces the old inline decode for A/B. See [../../docs/hd-pack.md](../../docs/hd-pack.md). |
| `VH_VERBOSE` | `0` | Per-event backend console chatter: `[lang]` table/section apply lines and per-read dialogue substitutions, `[HD] REPLACED` per background, `[HDvideo]` per movie. Default OFF (post-1.7 console hygiene, same spirit as the av_log errors-only rule): the console keeps the one-time boot summary (config echo, pack identities, `[HD] pack detected`) and EVERY warning/refusal -- only the recurring progress lines are gated. Set `1` when debugging a pack or reporting an issue. |
| `VH_LANG` | unset | **Language pack** selection (1.7): the folder name under `<deploy>/langpacks/` whose `manifest.json` passes the game/format gate. Set from the overlay ("LANGUAGE" — a pending change is marked `*` and applies at the next launch; packs apply at boot only, no live swap) and persisted here. US-disc feature: greyed on the Japanese game (a pack can still be queued there while a US disc switch is pending in the DISC row). A pack's localized backgrounds render at every `VH_INTERNAL_SCALE` — at 1× the engine keeps a native-size shadow pass alive just for the pack (HD-pack backgrounds still need ≥ 2×). `VH_LANGPACK=<dir>` points straight at a pack folder (dev override, skips the picklist). See [../../docs/language-packs.md](../../docs/language-packs.md). |
| `VH_CAM_INVERT_X` | `0` | Invert the right-stick camera *horizontal* axis (rotate direction). `1` = inverted. Also toggleable in the in-game options overlay (SELECT+START). |
| `VH_CAM_INVERT_Y` | `1` | Invert the right-stick camera *vertical* axis (raise/lower angle). **Ships inverted** — the modern twin-stick convention (push up = tilt the view down); set `0` for a normal vertical axis. Also toggleable in the in-game options overlay. |
| `VH_SEQ_MUTE` | off | `1` = mute only the SEQ **music**, keep VAG SFX + XA audible (useful for isolating sound effects). |
| `VH_FAST_BOOT` | `1` | **Fast boot** (2.0.0): the fresh-process boot load (the black-window stretch before the intro logo) runs the CD-timing model 16× scaled — on a console those seconds hid behind the BIOS animation. Everything from the first movie onward (title, New Game, area loads, battles) keeps the **hardware-exact** load pacing untouched. `0` = full hardware timing from the very first read (validation A/B). |

> **Audio is calibration-fixed (hardwired, no knobs).** The software SPU matches real-hardware output at
> the authentic values, so there are no tuning env vars — the settings are fixed in code: master trim
> `1.012` (RMS-matched to the octoshock reference within 0.01 dB); PsyQ's square volume law
> `VolL=L*L/16383` (`0x800d6d8c`) always on (the reason the mix has the right dynamic range); the legacy
> EQ tilt off (it over-corrects once the square law is present, 2.20 → 4.27 dB mean error).

## Compatibility

The game runs **cap-less** (no `setcap`, no elevated rights): the portable NULL-read / rodata-write fault
handler in `pc_bootstrap.c` catches transient PSX NULL/low-address accesses, emulates them (read 0 /
discard store), logs each site, and steps over — so no page-0 mapping is needed. This is the sole path;
the old privileged low-page-mapping fallback (and its `setcap` targets) was retired.

## Debug menu (advanced)

| Variable | Default | Effect |
|---|---|---|
| `VH_DEBUG_MENU` | off | `1` = enable KCET's original development debug menu (both regions, 2.0.0). Idle ~1.5 s at the **title screen** and it opens: battle-map warp, scene selector (all 95 story events, world-map destinations, towns), unit viewer, GAME START. The menu is the Japanese dev team's own tool; the US game shipped it stripped — this port restores it, with the scene lists translated. |

**Use at your own risk.** Warping skips the setup a scene normally gets from the story flow:
some destinations load with placeholder state, look wrong, or drop you somewhere unplayable —
that is authentic dev-tool behavior, not a port bug. Two hard rules: **saves made from a warped
state are unsupported** (they can encode states unreachable in normal play), and bug reports
are only actionable from a normal boot (no `VH_DEBUG_MENU`).

## Diagnostics — logging

| Variable | Build flag | Output | Logs |
|---|---|---|---|
| `VH_AI_LOG` | `AI_LOG=1` | `vh_ai_log.txt` | AI spell-target scoring: every candidate a caster evaluated + term breakdown (level / HP / `gAdvantage` type-matchup / terrain). Highest score = the pick. |
| `VH_XA_LOG` | — | `vh_xa_log.txt` | CD-XA seek/read events (`CdlSeekL`/`CdlReadN`, LBA, same-track replays). |
| `VH_XA_CSV` | — | `vh_xa_ours.csv` | Per-frame XA state (xaId, duration, stream/queue/source state) for diffing vs the BizHawk track map. |
| `VH_SEQ_LOG` | — | `vh_seq_log.txt` | SEQ music diagnostics. `=1` adds a ~6×/sec voice-census dump (active-voice count, note-on/off balance, master vol, timestamp). Low-volume lines — first-96 `[note]` events, stuck-note-reaper `[reaper]` events, `SsSeqStop` — are always logged (file auto-created), so a hung-note recurrence is captured without the flag. |
| `VH_SPRITE_LOG` | `SPRITE_LOG=1` | `vh_sprite_fate.csv` | Per-unit-sprite cull/projection/GTE state. |
| `VH_TERRAIN_LOG` | `TERRAIN_LOG=1` | `vh_terrain_otz_pc.csv` | Per-frame terrain `otz` stats + black-tile counts. |
| `VH_FPS_LOG` | — | stderr | Per-second `[FPS]` meter (VSync calls/sec). Off by default — one line/sec is console noise in the logs bug reports paste. |
| `VH_FRAME_TIME` | — | stderr | Per-frame budget split, mean over 120 frames: `work` (logic + GTE + raster + present) vs `idle` (pacing sleep), tagged with VSync mode + battle speed, plus the work-only fps ceiling. First stop for any "can't hold N fps" report. |
| `VH_PRESENT_TIME` | — | stderr | Present-path phase timing (555→888 convert / UI+GL submit / swap), mean over 120 frames. |
| `VH_SMOKE` | — | stderr + exit code | Boot smoke mode (`tools/regress/smoke_boot.sh`): exit 0 the moment the title screen is reached, exit 1 on timeout. Auto-holds START through the intro movies. `VH_SMOKE_LINGER=N` keeps running N frames after the title (for trace recording). |
| `VH_GPU_RECORD` | — | `<file>` | Record a GPU trace (every VRAM upload + every primitive drawn) for the raster regression harness; `VH_GPU_RECORD_FRAMES=N` frames (default 400); `VH_GPU_RECORD_BATTLE=1` holds recording until an active battle (a frame budget captures pure battle, not the boot lead-in). Replay with `VH_GPU_REPLAY=<file>` (deterministic, prints a VRAM signature; `VH_GPU_REPLAY_VERBOSE=1` per-frame). See `tools/regress/raster_check.sh`. |
| `VH_CAM_OSD` | — | on-screen | Camera-pose overlay (position/rotation/zoom) drawn in the window. |
| NULL-read fixup handler | — | `vh_null_reads.log` | Auto: each NULL-region read / rodata-write the handler fixed up (see `make crash-trace`). |

Advanced GTE/GPU render probes (developer, mostly gated by `SPRITE_LOG=1`): `VH_MTX_LOG`,
`VH_OBJPRIM4_LOG`, `VH_SPRITE_QUAD_LOG`, `VH_TERRAINPROJ_LOG`.

## Diagnostics — audio isolation

For chasing a single instrument in the music mix — spectral attribution alone can misidentify
the culprit; soloing an isolated program settles it in one run.

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
| `make unified` | Build the all-regions release binary (both game cores + disc-dispatch launcher) → `build-uni/`. |
| `make link` | Build the single-region dev binary (US default; `REGION=jp` for the Japanese core). |
| `make check-shared` | Verify the game files shared between the US and JP decomp trees are identical (modulo PC gates). |
| `make crash-trace` | Resolve each site in `vh_null_reads.log` to `file:line` (nm + addr2line). |
