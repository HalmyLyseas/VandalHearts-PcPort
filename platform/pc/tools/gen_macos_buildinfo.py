#!/usr/bin/env python3
"""Write a reproducibility/provenance manifest beside a local macOS build."""
import argparse
import hashlib
import os
import platform
import subprocess
import tempfile


def run(args, cwd=None):
    return subprocess.check_output(args, cwd=cwd, text=True, stderr=subprocess.STDOUT).strip()


def sha256(path):
    h = hashlib.sha256()
    with open(path, "rb") as source:
        for block in iter(lambda: source.read(1024 * 1024), b""):
            h.update(block)
    return h.hexdigest()


def main():
    parser = argparse.ArgumentParser()
    parser.add_argument("--repo", required=True)
    parser.add_argument("--binary", required=True)
    parser.add_argument("--psx", required=True)
    parser.add_argument("--krom", required=True)
    parser.add_argument("--output", required=True)
    parser.add_argument("--compiler", required=True)
    parser.add_argument("--architectures", default="native")
    parser.add_argument("--build-type", default="")
    parser.add_argument("--webp", default="")
    parser.add_argument("--hdvideo", default="")
    args = parser.parse_args()

    repo = os.path.abspath(args.repo)
    binary = os.path.abspath(args.binary)
    commit = run(["git", "rev-parse", "HEAD"], repo)
    dirty = bool(run(["git", "status", "--porcelain", "--untracked-files=all"], repo))
    try:
        binary_arches = run(["lipo", "-archs", binary])
    except (OSError, subprocess.CalledProcessError):
        binary_arches = platform.machine()
    compiler_version = run([args.compiler, "--version"]).splitlines()[0]
    cmake_version = run(["cmake", "--version"]).splitlines()[0]

    values = [
        ("format", "vandal-hearts-macos-buildinfo-1"),
        ("source_commit", commit),
        ("source_dirty", "1" if dirty else "0"),
        ("binary_sha256", sha256(binary)),
        ("binary_architectures", binary_arches.replace(";", ",")),
        ("requested_architectures", args.architectures or "native"),
        ("build_type", args.build_type or "unspecified"),
        ("webp", args.webp),
        ("hdvideo", args.hdvideo),
        ("psx_exe_sha256", sha256(args.psx)),
        ("krom_source_sha256", sha256(args.krom)),
        ("compiler", compiler_version),
        ("cmake", cmake_version),
        ("host", "%s-%s" % (platform.system(), platform.machine())),
    ]
    output = os.path.abspath(args.output)
    os.makedirs(os.path.dirname(output), exist_ok=True)
    fd, temporary = tempfile.mkstemp(prefix=".VH_BUILDINFO.", dir=os.path.dirname(output), text=True)
    try:
        with os.fdopen(fd, "w") as out:
            for key, value in values:
                out.write("%s=%s\n" % (key, value.replace("\n", " ")))
            out.flush()
            os.fsync(out.fileno())
        os.replace(temporary, output)
    except Exception:
        try:
            os.unlink(temporary)
        except OSError:
            pass
        raise
    print("wrote macOS build provenance: %s" % output)


if __name__ == "__main__":
    main()
