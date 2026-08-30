#!/usr/bin/env python3
"""Verify that every `See docs/<x>.md, "<heading>"` pointer in a tracked source file names a
Markdown file that exists and a heading that file actually contains. Exit 1 on any miss."""
import os
import re
import subprocess
import sys

SRC_EXT = (".c", ".h", ".py", ".sh", ".ini", ".txt", ".cmake")
SRC_NAMES = ("Makefile", "CMakeLists.txt", ".gitignore")
POINTER = re.compile(r'((?:docs|platform)/[\w./-]+\.md)(?:[,:]?\s*(?:section\s*)?["“]([^"”]+)["”])?')


def headings(root, path, cache):
    if path not in cache:
        try:
            with open(os.path.join(root, path), encoding="utf-8") as fh:
                cache[path] = [re.sub(r"\s+", " ", h.strip("# \n")).lower()
                               for h in fh if h.startswith("#")]
        except FileNotFoundError:
            cache[path] = None
    return cache[path]


def main():
    root = subprocess.run(["git", "rev-parse", "--show-toplevel"], capture_output=True,
                          text=True, check=True).stdout.strip()
    files = subprocess.run(["git", "ls-files"], cwd=root, capture_output=True,
                           text=True, check=True).stdout.split()
    cache, bad = {}, 0
    for rel in files:
        if not rel.endswith(SRC_EXT) and os.path.basename(rel) not in SRC_NAMES:
            continue
        try:
            with open(os.path.join(root, rel), encoding="utf-8", errors="replace") as fh:
                lines = fh.read().split("\n")
        except (IsADirectoryError, FileNotFoundError):
            continue
        for i, line in enumerate(lines, 1):
            for m in POINTER.finditer(line):
                path, section = m.group(1), m.group(2)
                hs = headings(root, path, cache)
                if hs is None:
                    print(f"{rel}:{i}: missing doc {path}")
                    bad += 1
                elif section:
                    want = section.strip().lower()
                    if not any(want in h or h in want for h in hs):
                        print(f'{rel}:{i}: no heading like "{section}" in {path}')
                        bad += 1
    if bad:
        print(f"{bad} unresolved doc pointer(s)", file=sys.stderr)
    return 1 if bad else 0


if __name__ == "__main__":
    sys.exit(main())
