#!/usr/bin/env python3
"""
vh_hdpack_videos.py -- assemble the HD FMV pack (hdpacks/videos/).

The engine keys each replacement movie by the movie's start SECTOR (== its base LBA at playback), so the
files must be named <sector-hex>.mp4. This maps the STR-named HD movies (LOGO_USA, TITLE_WS, ...) to those
sector names, using the game's own movie table (MOV_* enum -> movies[].startingSector).

    ./vh_hdpack_videos.py --videos <DIR of <NAME>.STR.mp4> --out hdpacks
    (writes hdpacks/videos/<sector>.mp4)

The HD movies are H.264 .mp4 at the source aspect (4:3), upscaled from the disc STR frames, at the SAME
frame rate (~14.99 fps) with AT LEAST the game's frame count for that movie (else the last frame freezes
for the remainder -- avoid `ffmpeg -shortest`, which trims video to the slightly-shorter audio). Audio in
the mp4 is ignored (the game plays its own XA), so `-an` may be used to trim size.
"""
import os, glob, shutil, argparse

# MOV_* enum (include/cd_files.h) index -> movies[].startingSector (src/split_037758.c), by STR name.
NAME_TO_SECTOR = {
    "LOGO_USA": 0x1fded, "TITLE_WS": 0x20385, "1BU_WS": 0x21618, "EPI1_WS": 0x34172,
    "2BU_WS": 0x27a08, "3BU_WS": 0x2919f, "4BU_WS": 0x2e115, "5BU_WS": 0x30604,
    "6BU_WS": 0x32773, "END1_WS": 0x36c20, "END2_WS": 0x3a68c, "EPI2_WS": 0x3488f,
    "EPI3_WS": 0x34fac, "EPI4_WS": 0x356c9, "EPI5_WS": 0x35de6, "EPI6_WS": 0x36503,
}


def main():
    ap = argparse.ArgumentParser(description="Assemble hdpacks/videos/ from STR-named HD movies")
    ap.add_argument("--videos", required=True, help="folder of <NAME>.STR.mp4 HD movies")
    ap.add_argument("--out", required=True, help="the hdpacks/ folder (videos/ is created inside)")
    ap.add_argument("--link", action="store_true", help="symlink instead of copy (for local testing)")
    a = ap.parse_args()
    vdir = os.path.join(a.out, "videos")
    os.makedirs(vdir, exist_ok=True)
    made = missing = 0
    for name, sector in sorted(NAME_TO_SECTOR.items(), key=lambda kv: kv[1]):
        src = os.path.join(a.videos, f"{name}.STR.mp4")
        if not os.path.exists(src):
            src2 = glob.glob(os.path.join(a.videos, f"{name}*.mp4"))   # tolerate slight name variants
            if src2:
                src = src2[0]
            else:
                print(f"  {sector:x}  {name}  (no mp4)"); missing += 1; continue
        dst = os.path.join(vdir, f"{sector:x}.mp4")
        if os.path.islink(dst) or os.path.exists(dst):
            os.remove(dst)
        if a.link:
            os.symlink(os.path.abspath(src), dst)
        else:
            shutil.copyfile(src, dst)
        print(f"  {sector:x}.mp4 <- {os.path.basename(src)}"); made += 1
    print(f"\n{made} movies -> {vdir}  ({missing} missing)")


if __name__ == "__main__":
    main()
