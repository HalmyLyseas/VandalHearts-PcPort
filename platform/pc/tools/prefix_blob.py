#!/usr/bin/env python3
"""Prefix-rename a partial-linked region core for the unified binary.

Renames every DEFINED external symbol in a relocatable object with a region prefix
(us_/jp_) so two complete region cores can coexist in one executable. Includes weak and
COMMON symbols (an unrenamed tentative definition would silently MERGE across the two
blobs at the final link).

MinGW/COFF `.refptr.X` auto-import thunks are renamed too -- symbol AND section -- to
`.refptr.<prefix>X` / `.rdata$.refptr.<prefix>X`. They were originally skipped on the
assumption that "each blob's copies are section-local"; that is FALSE at the final link:
the thunks live in COMDAT sections keyed by their bare name, so the linker keeps ONE copy
for both blobs -- whichever blob's slot survives, BOTH blobs then dereference it. Seen
live in the unified-binary build: the surviving `.refptr.gGraphicBuffers` pointed at
us_gGraphicBuffers, so the JAPANESE game wrote the US blob's ordering table through the
thunk while reading its own -- a split-brain OT that cycled, hanging DrawOTag on the
first title-transition frame (Windows-only: ELF has no refptr thunks). Renaming both the
symbol and the section gives each blob its own slot, relocated to its own renamed global.
Truly-shared externals (libc/libav) get duplicate 8-byte slots per blob -- harmless.

Usage: prefix_blob.py <in.o> <out.o> <prefix> [--nm=nm] [--objcopy=objcopy] [--objdump=objdump]
"""
import subprocess
import sys
import tempfile
import os

REFPTR_SECT = ".rdata$.refptr."


def main() -> int:
    args = [a for a in sys.argv[1:] if not a.startswith("--")]
    opts = dict(a[2:].split("=", 1) for a in sys.argv[1:] if a.startswith("--"))
    if len(args) != 3:
        print(__doc__, file=sys.stderr)
        return 2
    inobj, outobj, prefix = args
    nm = opts.get("nm", "nm")
    objcopy = opts.get("objcopy", "objcopy")
    objdump = opts.get("objdump", "objdump")

    out = subprocess.run([nm, "-g", "--defined-only", inobj],
                         capture_output=True, text=True, check=True).stdout
    lines = []
    nrefptr = 0
    for line in out.splitlines():
        parts = line.split()
        if len(parts) < 3:
            continue
        sym = parts[-1]
        if sym.startswith(".refptr."):
            # .refptr.X -> .refptr.<prefix>X (see the header comment: COMDAT dedup hazard)
            lines.append(f"{sym} .refptr.{prefix}{sym[len('.refptr.'):]}")
            nrefptr += 1
            continue
        lines.append(f"{sym} {prefix}{sym}")
    if not lines:
        print(f"prefix_blob: no defined symbols found in {inobj}?", file=sys.stderr)
        return 1

    # COFF only: rename the COMDAT sections that carry the thunks, so the final link cannot
    # dedup them across blobs by section name either. ELF inputs have no such sections.
    rename_args = []
    sect = subprocess.run([objdump, "-h", inobj], capture_output=True, text=True, check=True).stdout
    seen = set()
    for line in sect.splitlines():
        parts = line.split()
        if len(parts) >= 2 and parts[1].startswith(REFPTR_SECT) and parts[1] not in seen:
            seen.add(parts[1])
            new = REFPTR_SECT + prefix + parts[1][len(REFPTR_SECT):]
            rename_args.append(f"--rename-section={parts[1]}={new}")

    with tempfile.NamedTemporaryFile("w", suffix=".map", delete=False) as f:
        f.write("\n".join(lines) + "\n")
        mappath = f.name
    try:
        subprocess.run([objcopy, f"--redefine-syms={mappath}"] + rename_args + [inobj, outobj],
                       check=True)
    finally:
        os.unlink(mappath)
    print(f"prefix_blob: {len(lines)} symbols -> {prefix}* in {outobj} "
          f"({nrefptr} refptr thunks + {len(rename_args)} thunk sections re-keyed)")
    return 0


if __name__ == "__main__":
    sys.exit(main())
