# Configuration & running

Once built (see [building.md](building.md)), the port needs one thing to run: **your own copy of the
game disc**, as a raw `.bin` image. Everything else has sensible defaults.

## Supplying the disc image

The game reads its data (maps, sprites, audio, FMVs) from a raw 2352-byte/sector `.bin` dump of
*Vandal Hearts (USA)* (`SLUS-00447`) — or, equivalently, the *Vandal Hearts (Asia)* release
(`SCPS-45183`); see [Supported releases](#supported-releases) below. You do **not** need to set
anything if you put it where the port looks. On startup it searches, in order:

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
a cheap signature check, not a full hash — any genuine *Vandal Hearts (USA)* or *(Asia)* dump passes.
See [pc-port/subsystems/cd-xa.md](pc-port/subsystems/cd-xa.md) for the mechanism.

### Supported releases

The port **is** the recompiled US game executable, and it reads disc data by fixed sector number
(not by filename). So it runs a disc only if that disc has the same game code and the same file
layout as the US release. Two releases qualify:

| release | serial | status | why |
|---|---|---|---|
| Vandal Hearts (USA) | `SLUS-00447` | ✅ supported | the reference release |
| Vandal Hearts (Asia) | `SCPS-45183` | ✅ supported | **byte-identical** to USA except the 14-byte memory-card save id; same disc master, same layout |
| Vandal Hearts (Europe) | `SLES-00204` | ❌ not supported | different executable (multi-language En/Fr/De build), different disc |
| Vandal Hearts (Japan) | `SLPM-86007` | ❌ not supported | different executable (kanji/SJIS build), different disc |

The port names the mounted release in its console log (`mounted disc image '…' [Asia (SCPS-45183)]`).
An unrecognized *Vandal Hearts* boot disc — most likely the Europe or Japan build — still passes the
signature gate and boots, but the port warns that it does not reproduce that build and behavior will
be wrong. Supporting Europe/Japan would each require a separate decompilation; note the
[language packs](language-packs.md) already provide non-English text on the supported engine.

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
| `VH_ACCURATE` | `1` (on) | PSX-accurate software rasterizer — a fixed-point integer DDA that evaluates coverage and texture UVs at the exact pixel position the PS1 GPU does, plus ordered dithering (honouring the GPU dither-enable bit) and 5-bit semi-transparency blend. Validated ~99.8–99.99% pixel-exact vs a DuckStation VRAM capture. The intended, hardware-faithful look. Set `0` for the legacy renderer (softer edges, no dithering); advanced users only. |
| `VH_INTERNAL_SCALE` | `1` (off) | Internal render resolution for the 3D — supersample at `1`× (native), `2`×, `3`×, or `4`×, for a sharper image with no change to the art, sprites, or camera (it samples the same textures on a denser grid). `2`× is essentially free; `3`×/`4`× are heavier (the rasterizer is multithreaded — see `VH_RASTER_THREADS`). Tile-seam handling ("crust-free" sampling) is built in — the denser grid samples tile interiors like the reference renderer, so no dark grid appears along terrain/lava/water seams, while 2D UI/text stays pixel-aligned. Also settable live in the options overlay (**SELECT + START**), which writes your choice back to the INI. |
| `VH_RASTER_THREADS` | auto (one per core) | Advanced: worker-thread count for the internal-resolution rasterizer. Default is automatic (online CPU count, capped). Set a number only to cap it — e.g. `4` to leave cores free for other work. Ignored at `1`× internal resolution (nothing to parallelize). |
| `VH_HDPACK` | auto (on when a valid pack is found) | Enable the optional HD pack (backgrounds + FMV movies). Set automatically when a valid `hdpacks/` is auto-detected beside the executable; toggle it live in the options overlay ("HD PACK"), which persists your choice here. Needs internal resolution > 1 (and, windowed, window scale > 1) to be visible — enabling it bumps those. See [hd-pack.md](hd-pack.md). `VH_HD_PACK=<dir>` (note the underscore) instead points at an explicit `backgrounds/` folder. |
| `VH_LANG` | *(unset)* | Language pack selection — the folder name under `langpacks/` beside the executable (e.g. `fr-fantrad`). Normally set from the options overlay's **LANGUAGE** row (System category), which persists here; a change applies at the next launch. Unset or empty = the original English. `VH_LANGPACK=<dir>` (developer override) points straight at a pack folder. See [language-packs.md](language-packs.md). |

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
