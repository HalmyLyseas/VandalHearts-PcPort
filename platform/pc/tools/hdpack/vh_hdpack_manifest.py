#!/usr/bin/env python3
"""
vh_hdpack_manifest.py -- write hdpacks/manifest.json for an installed HD pack.

The engine auto-detects <deploy>/hdpacks/ and validates manifest.json's "game" id against the build
(SLUS-00447) before enabling the HD PACK option. This emits that manifest from a backgrounds/ folder
of <hash>.webp files.

    ./vh_hdpack_manifest.py --pack /path/to/hdpacks --game SLUS-00447 --version 1
    (expects <pack>/backgrounds/<hash>.webp ; writes <pack>/manifest.json)
"""
import os, glob, json, argparse


def main():
    ap = argparse.ArgumentParser(description="Generate hdpacks/manifest.json")
    ap.add_argument("--pack", required=True, help="the hdpacks/ folder (contains backgrounds/)")
    ap.add_argument("--game", default="SLUS-00447", help="disc/build id the engine validates against")
    ap.add_argument("--version", type=int, default=2, help="pack version (2 = current: backgrounds + videos)")
    a = ap.parse_args()
    bgdir = os.path.join(a.pack, "backgrounds")
    viddir = os.path.join(a.pack, "videos")
    hashes = sorted(os.path.splitext(os.path.basename(f))[0]
                    for f in glob.glob(os.path.join(bgdir, "*.webp")) + glob.glob(os.path.join(bgdir, "*.hdi")))
    sectors = sorted(os.path.splitext(os.path.basename(f))[0]
                     for f in glob.glob(os.path.join(viddir, "*.mp4")))
    manifest = {"game": a.game, "packVersion": a.version, "count": len(hashes), "hashes": hashes,
                "videos": len(sectors), "sectors": sectors}
    out = os.path.join(a.pack, "manifest.json")
    with open(out, "w") as f:
        json.dump(manifest, f, indent=1)
    print(f"{out}: game={a.game} v{a.version} count={len(hashes)} videos={len(sectors)}")


if __name__ == "__main__":
    main()
