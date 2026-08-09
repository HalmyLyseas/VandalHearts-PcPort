# HD background pack

An optional feature (1.6): the port can replace the game's **pre-rendered backgrounds** and **FMV movies**
with higher-resolution versions at render time. The **source tree and base executable contain no HD art**,
and the game behaves identically without a pack. A pack is either **built from your own disc** (tools below)
or **downloaded as an optional asset on the 1.6 release** — the prebuilt pack is upscaled derivative art,
provided for convenience (see [NOTICE](../NOTICE)).

(A **language pack** can carry translated backgrounds through the same mechanism — those take
priority over the HD pack's and work at every internal resolution; see
[language-packs.md](language-packs.md).)

Backgrounds and movies are replaced because both are **continuous-tone** content that upscales cleanly.
The game's other 2D art — dialogue portraits, sprites, UI, fonts — is hand-drawn pixel art and is left at
its native resolution (the engine's integer-scaled nearest-neighbour), which keeps it consistent with the
crisp UI and text. Smoothing pixel art would clash with them.

## Installing a pack

Place the pack in a `hdpacks/` folder beside the executable (the same place the game finds its disc image
and `vandalhearts.ini`):

```
hdpacks/
  manifest.json
  backgrounds/
    <hash>.webp
  videos/            # optional
    <sector>.mp4
```

On launch the port auto-detects `hdpacks/`, validates `manifest.json`, and enables the **HD PACK** option
in the SELECT+START options overlay. `VH_HD_PACK=<dir>` overrides the auto-detect with an explicit
`backgrounds/`-style folder.

`manifest.json` records the disc/build id, pack version, and the pack's content (background hashes +
FMV start sectors). The engine requires **packVersion 2**. The startup console log reports the pack's
content (`75 backgrounds, 16 videos`); when the pack can't be used, the overlay's HD PACK row says why
(`NO PACK` / `OUTDATED PACK` / `WRONG GAME`).

```json
{ "game": "SLUS-00447", "packVersion": 2, "count": 75, "hashes": ["0c5035b9b009cde7", ...],
  "videos": 16, "sectors": ["1fded", ...] }
```

The `game` id must match the build (`SLUS-00447`, Vandal Hearts USA). A pack for a different version is
reported and left disabled rather than loaded, so it can't silently do nothing.

## The HD PACK option

The overlay row (between `TACTICAL MODE` and `INTERNAL RES`) is greyed and read-only when no valid pack is
installed. With a pack present it is toggleable and defaults to on; the choice persists in `vandalhearts.ini`
as `VH_HDPACK`.

![The options overlay with HD PACK ON and INTERNAL RES set to X4](images/OverlayMenu-Main.png)

HD detail is only visible when the game renders above the native 320×240:

- Enabling HD sets `INTERNAL RES` to at least ×2 (the hi-res pass renders the HD image). Windowed, it also
  sets `WINDOW SCALE` to at least ×2, so the hi-res buffer is not downsampled back to a 320-wide window;
  fullscreen leaves window scale alone (the display scales the buffer).
- Dropping `INTERNAL RES` to ×1 disables HD. Windowed, dropping `WINDOW SCALE` to ×1 also disables it.
- Higher values (×3, ×4) are accepted; for the sharpest result keep internal resolution ≥ window scale.

**When a toggle takes effect.** Each background's HD replacement is keyed by a content hash computed as the
game *uploads* it to VRAM, so toggling **HD PACK on takes effect on the next screen/background load** — the
current screen (already uploaded) stays native until it next reloads. Toggling **off is immediate** (the
current screen reverts to native at once). This is by design: enabling HD mid-scene would require re-hashing
whatever is currently in VRAM. Normal use — leaving it on — replaces every background from load, so the
distinction only shows when flipping the option during play.

Enabling HD applies from the next background load (the current on-screen background is not swapped live);
disabling it is immediate.

## Building a pack

Each background is identified by a hash of the exact bytes the game uploads to VRAM. Those bytes are
recoverable directly from the disc's background `.TIM` files: a disc file is stored as raw 2352-byte CD
sectors (24-byte header + 2048-byte data + trailing ECC), so concatenating the 2048-byte payloads
reconstructs the file, and the resulting 8bpp image data hashes to the identity the game requests. This
means a complete pack is buildable offline from the disc, with no play-through.

The build tools live in [`platform/pc/tools/hdpack/`](../platform/pc/tools/hdpack/) — see its `README.md`:

1. `vh_bg_restore.py` — finish each AI-upscaled background (recover the source's own high-frequency detail,
   lock hue/saturation to the original) so it is sharper without reinterpreting the art. Save as `.webp`.
2. `vh_tim_hashpack.py` — hash the disc TIMs and assemble `backgrounds/<hash>.webp` from your HD images.
3. `vh_hdpack_manifest.py` — write `manifest.json`.

`.webp` keeps the backgrounds around 20 MB (roughly 7× smaller than uncompressed); the HEVC movies add
about 310 MB, so a complete pack is ~330 MB. A raw `.hdi` format is also
accepted as a fallback for builds made without libwebp.

## Building the movies

Each FMV is keyed by its **start sector** (the identity the game plays it from), so the files are named
`videos/<sector>.mp4`. To produce them: extract the disc STR movies (jpsxdec) to frames + audio, upscale the
frames (an AI upscaler, then `vh_bg_restore.py` — the same finishing step as backgrounds), and re-encode to
an **HEVC (or H.264) `.mp4` at the source aspect (4:3) and frame rate (~14.99 fps)** — the engine decodes
both, but HEVC is ~40 % smaller at the same visual quality:

```sh
ffmpeg -framerate 14.99 -i processed/frame_%06d.png \
       -c:v libx265 -preset slow -crf 26 -g 300 -pix_fmt yuv420p -tag:v hvc1 -an NAME.STR.mp4
./vh_hdpack_videos.py --videos <DIR of NAME.STR.mp4> --out hdpacks   # -> videos/<sector>.mp4
```

Two requirements: the mp4 must have **at least the game's frame count** for that movie, or its last frame
freezes for the remainder (so never pass `-shortest`); and always encode **`-an`** — the game plays its own
original XA audio in sync, so the mp4's audio track is ignored dead weight (dropping it also saves space and
removes the usual DMCA soft-target). A long GOP (`-g 300`) is free: the engine only plays each movie forward
from frame 0 and never seeks. Movies present at full window resolution and are independent of `INTERNAL RES`.

## Dependencies

The port links **libwebp** (for `.webp` backgrounds) and **libav / ffmpeg** (`libavformat`, `libavcodec`,
`libavutil`, `libswscale` — for `.mp4` movies) by default, alongside SDL2 / OpenAL / OpenGL — see
[building.md](building.md). Build without them via `make link NO_WEBP=1 NO_HDVIDEO=1` /
`cmake -DVH_WEBP=OFF -DVH_HDVIDEO=OFF`; without libwebp the pack must use raw `.hdi` backgrounds, and
without libav the movies fall back to the native MDEC FMVs.
