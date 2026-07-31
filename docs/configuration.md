# Configuration & running

Once built (see [building.md](building.md)), the port needs one thing to run: **your own copy of the
game disc**, as a raw `.bin` image. Everything else has sensible defaults.

## Supplying the disc image

The game reads its data (maps, sprites, audio, FMVs) from a raw 2352-byte/sector `.bin` dump of
*Vandal Hearts (USA)*. You do **not** need to set anything if you put it where the port looks. On
startup it searches, in order:

1. a **`game/` folder next to the executable** containing any `*.bin`;
2. a `*.bin` sitting **directly beside the executable**;
3. (dev builds only) the repo's `game/` folder a few levels up.

So the zero-config layout is:

```
vandalhearts_pc(.exe)
game/
  Vandal Hearts (USA).bin
```

To point somewhere else, set `VH_DISC_IMAGE` (environment variable or `vandalhearts.ini`) to the full
path. That override wins over auto-detect.

### Wrong / missing disc is fatal

The port validates the image at mount: *Vandal Hearts (USA)*'s boot executable `SLUS_004.47` carries a
`PS-X EXE` signature at a fixed sector, and the port checks for it. If the file is missing, or is a
different game / region / a `.cue`/`.iso` instead of a raw `.bin`, it **exits with a clear message**
(and a message box on Windows, for double-click users) rather than booting into a blank window. This is
a cheap signature check, not a full hash — any genuine *Vandal Hearts (USA)* dump passes. See
[pc-port/subsystems/cd-xa.md](pc-port/subsystems/cd-xa.md) for the mechanism.

## The `vandalhearts.ini` config file

A `vandalhearts.ini` is shipped next to the binary. It's the friendly way to set options without
environment variables — edit it and uncomment what you want. Format is plain `KEY=VALUE`; `[section]`
headers and `;`/`#` comments are ignored (sections are just for readability). Example:

```ini
[game]
VH_DISC_IMAGE=game/Vandal Hearts (USA).bin
VH_SCALE=3

[debug]
;VH_SEQ_LOG=1        ; uncomment to enable a diagnostic log
```

**Precedence is environment variable › `vandalhearts.ini` › built-in default.** A real environment
variable always wins, so scripts and power users can override the file; the file only fills in what
isn't already set. Applied keys are echoed at startup (`PC_Config: KEY=VALUE (from vandalhearts.ini)`).

## Common options

| Key | Default | Effect |
|---|---|---|
| `VH_DISC_IMAGE` | auto-detect (see above) | Full path to the game disc `.bin`. |
| `VH_SCALE` | `2` | Integer window scale of the native 320×240 (2 = 640×480). Upscaled nearest-neighbour, so pixel art stays crisp. |
| `VH_ACCURATE` | `1` (on) | PSX-accurate software rasterizer — round-to-nearest texture sampling, ordered dithering, 5-bit semi-transparency blend. The intended, hardware-faithful look. Set `0` for the legacy renderer (no dithering, slightly softer); advanced users only. |

The everyday knobs are few by design. There are also **compatibility** and **audio-tuning** keys whose
defaults reproduce real-hardware behaviour and shouldn't normally be touched — they're documented in
the shipped `vandalhearts.ini` and in [`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md).

## Diagnostics and logging

All diagnostic logging is **off by default** — end users get a clean run with no stray files. Each
diagnostic is opt-in via its own `VH_*` flag (set in the environment or the `[debug]` section of the
INI); enabling one writes a `vh_*.csv` / `.txt` / `.log` next to the binary. The full list is in
[`platform/pc/OPTIONS.md`](../platform/pc/OPTIONS.md) and the INI's `[debug]` section — e.g.
`VH_SEQ_LOG` (SEQ music trace), `VH_CAM_LOG` / `VH_RAND_LOG` (per-frame camera / RNG watches),
`VH_UI_LOG`, `VH_XA_LOG`. Some deeper probes also need a build flag to compile the hook in; those are
marked in `OPTIONS.md`.

One log is always allowed to appear because its mere presence is a signal: `vh_null_reads.log` is
written only if the port hits a NULL-region read or a read-only-data write the startup remap missed —
i.e. never, in a healthy run. See [memory-safety.md](memory-safety.md).

## Saves

Save files are written as real files under a `saves/` folder next to the working directory. They're
created automatically on first save. See
[pc-port/subsystems/kernel.md](pc-port/subsystems/kernel.md) for the memory-card model and a caveat
about in-battle-save struct layout across build widths.
