# HD background pack — build tools (1.6)

Tools for producing an **HD background pack** for the port's `HD PACK` option. The engine auto-detects a
pack in `hdpacks/` beside the executable and replaces each 320×240 background with a higher-resolution
image at render time (see `docs/`). The pack is data the user supplies — the port ships none of it.

## Layout the engine expects
```
hdpacks/
  manifest.json          # {"game":"SLUS-00447","packVersion":2,"count":C,"hashes":[...],"videos":V,"sectors":[...]}
  backgrounds/
    <hash>.webp          # one per background, keyed by the engine's upload hash
```

## Building a pack

Each background is identified by a hash of the exact bytes the game uploads to VRAM. `vh_tim_hashpack.py`
computes those hashes directly from the disc's background `.TIM` files (extracted with jpsxdec) and copies
your HD images into place under the right hash name — no play-through needed.

```sh
# 1. hashes + pack   (--tim: disc background TIMs;  --hd: your HD .webp images, one per TIM stem)
./vh_tim_hashpack.py --tim <DISC_TIMs> --hd <HD_WEBP_DIR> --out hdpacks/backgrounds

# 2. manifest        (validated by the engine against the build's game id, SLUS-00447)
./vh_hdpack_manifest.py --pack hdpacks --game SLUS-00447        # run AFTER the videos step below
```

Drop the resulting `hdpacks/` next to the executable; the `HD PACK` toggle then enables in the options
overlay. `VH_HD_PACK=<dir>` overrides the auto-detect with an explicit `backgrounds/`-style folder.

## Producing the HD images (`vh_bg_restore.py`)

Backgrounds are pre-rendered, continuous-tone art, so they upscale well with an AI upscaler.
`vh_bg_restore.py` is the finishing step: it recovers the source image's own high-frequency detail and
locks hue/saturation to the original, so the result is sharper without reinterpreting the art.

```sh
# ORIG = jpsxdec PNG of the native background; AI = an upscaler's output (e.g. Upscayl 4x)
./vh_bg_restore.py            # batch a folder
./vh_bg_restore.py ORIG.png AI.png OUT.png    # one image
```

Convert the finished PNGs to `.webp` for the pack (≈7× smaller; a full pack is ~20 MB vs ~150 MB).

## HD movies (FMVs)

The FMV movies (`videos/<sector>.mp4`) are the second half of the pack. Produce each from the disc:

1. Extract the STR movie with **jpsxdec** → an MJPEG `.avi` (video) + `.wav` (audio).
2. `ffmpeg` → PNG frames; upscale them (AI upscaler) + `vh_bg_restore.py` (same finish as backgrounds).
3. `ffmpeg` re-encode the finished frames → **HEVC `.mp4`** at the source aspect (4:3) and frame rate
   (~14.99 fps):
   ```sh
   ffmpeg -framerate 14.99 -i processed/frame_%06d.png \
          -c:v libx265 -preset slow -crf 26 -g 300 -pix_fmt yuv420p -tag:v hvc1 -an NAME.STR.mp4
   ```
   - **`-an`** — always drop audio. The game plays its own XA off the disc, so the mp4's track is dead
     weight; stripping it also removes the usual DMCA soft-target. (`-c:v libx264` also works — the engine
     decodes both — but HEVC crf 26 is ~40 % smaller at visually-transparent quality.)
   - Keep **all** frames: the mp4 must have **≥** the game's frame count for that movie or the last frame
     freezes for the remainder in-game. With `-an` there is no audio track for `-shortest` to trim
     against, so that footgun is gone — just never pass `-shortest`.
   - `-g 300` (a long GOP) is free here: the engine only ever plays each movie forward from frame 0 and
     never seeks, so frequent keyframes would only add size.
4. `vh_hdpack_videos.py --videos <DIR of NAME.STR.mp4> --out hdpacks` → renames each named movie to its
   `videos/<sector>.mp4` (using the game's `MOV_*` → start-sector map).

Deps: `numpy`, `pillow` (Python 3); jpsxdec + ffmpeg for the movie extraction/encode.
