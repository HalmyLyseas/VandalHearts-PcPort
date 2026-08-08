#!/usr/bin/env python3
"""Validate the exact US Vandal Hearts executable used by offset-based generators."""
import argparse
import hashlib
import os

EXPECTED_MD5 = "596bb082a2de5f1fe977dd3d7e160b03"
EXPECTED_SHA256 = "351daaf337cdfd4c00e8abde694f6d0dcbc8b3dcd5e69a411f6d30b8446dfbb4"


def digest(path, algorithm):
    h = hashlib.new(algorithm)
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("exe")
    args = parser.parse_args()
    path = os.path.abspath(args.exe)
    if not os.path.isfile(path):
        raise SystemExit("PS-X executable not found: %s" % path)
    md5 = digest(path, "md5")
    sha256 = digest(path, "sha256")
    if md5 != EXPECTED_MD5 or sha256 != EXPECTED_SHA256:
        raise SystemExit(
            "unsupported PS-X executable: %s\n"
            "  md5:    %s (expected %s)\n"
            "  sha256: %s (expected %s)"
            % (path, md5, EXPECTED_MD5, sha256, EXPECTED_SHA256)
        )
    print("validated SLUS_004.47: md5 %s; sha256 %s" % (md5, sha256))


if __name__ == "__main__":
    main()
