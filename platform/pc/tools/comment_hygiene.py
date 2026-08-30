#!/usr/bin/env python3
"""Comment-hygiene check for every tracked source file.

Two rules, both aimed at a developer reading the public tree without any
project history: no comment run longer than 3 consecutive lines, and no
project-log vocabulary (dates, local-only paths, stage/version markers,
past-tense narration) inside comments and Python docstrings. Markdown is prose
and only checked for local-only path references.

Usage: comment_hygiene.py [--root DIR] [FILE ...]
Exit 1 with one `file:line: message` per violation; silent on success.
"""
import argparse
import ast
import os
import re
import subprocess
import sys

MAX_RUN = 3

C_EXT = {".c", ".h", ".cpp", ".hpp", ".cc", ".m", ".mm", ".js", ".mjs"}
HASH_EXT = {".py", ".sh", ".cmake", ".yaml", ".yml", ".toml", ".txt", ".gitignore"}
HASH_NAMES = {"Makefile", "CMakeLists.txt", ".gitignore", ".gitattributes"}
INI_EXT = {".ini"}
SKIP_PREFIX = ("docs/images/", ".github/", "platform/pc/tools/langpack/images-quickstart/")
SKIP_EXT = {".png", ".jpg", ".jpeg", ".webp", ".gif", ".pdf", ".ico", ".json", ".bin"}

# Applied to comment text of code files only, never to code or string literals.
FORBIDDEN = [
    ("exchange/ reference", re.compile(r"exchange/")),
    ("scratchpad reference", re.compile(r"scratchpad")),
    ("home-directory path", re.compile(r"/home/|\bmcerna\b")),
    ("date", re.compile(r"\b20[0-9]{2}-[0-9]{2}-[0-9]{2}\b")),
    ("stage marker", re.compile(r"\bStage[- ][0-9]")),
    ("version marker", re.compile(r"\bv[0-9]+\.[0-9]+\.[0-9]+\b")),
    ("spec item id", re.compile(r"\b(GAP|Bug|Item|Finding) ?[0-9]+\b")),
    ("'the human'", re.compile(r"\bthe human\b")),
    ("'PM'", re.compile(r"\bPM\b")),
    ("model name", re.compile(r"\b(Fable|Sonnet|Opus|Claude|Codex)\b")),
    ("'review'", re.compile(r"\breview(ed|ing|s)?\b", re.I)),
    ("'repro'", re.compile(r"\brepro\b", re.I)),
    ("past-tense narration", re.compile(
        r"\b(previously|formerly|historical(ly)?|used to be|no longer)\b", re.I)),
]
# Markdown is documentation; only a pointer into a gitignored folder is wrong there.
FORBIDDEN_MD = [
    ("exchange/ pointer", re.compile(r"exchange/[A-Za-z0-9]")),
    ("scratchpad reference", re.compile(r"scratchpad")),
]


def classify(path):
    base = os.path.basename(path)
    ext = os.path.splitext(base)[1]
    if path.startswith(SKIP_PREFIX) or ext in SKIP_EXT:
        return None
    if ext == ".md":
        return "md"
    if ext in C_EXT:
        return "c"
    if base in HASH_NAMES or ext in HASH_EXT:
        return "hash"
    if ext in INI_EXT:
        return "ini"
    return None


def comment_mask_c(lines):
    """True per line when the whole line is comment (// or inside /* */)."""
    mask = [False] * len(lines)
    in_block = False
    for i, line in enumerate(lines):
        s = line.strip()
        if in_block:
            mask[i] = True
            if "*/" in s:
                in_block = False
                if s.split("*/", 1)[1].strip():
                    mask[i] = False
            continue
        if s.startswith("//"):
            mask[i] = True
        elif s.startswith("/*"):
            if "*/" in s:
                mask[i] = s.rsplit("*/", 1)[1].strip() == ""
            else:
                mask[i] = True
                in_block = True
    return mask


def docstring_mask(text, lines):
    """True per line inside a module/class/function docstring (Python only)."""
    mask = [False] * len(lines)
    try:
        tree = ast.parse(text)
    except SyntaxError:
        return mask
    for node in ast.walk(tree):
        if not isinstance(node, (ast.Module, ast.ClassDef, ast.FunctionDef, ast.AsyncFunctionDef)):
            continue
        body = getattr(node, "body", [])
        if body and isinstance(body[0], ast.Expr) and isinstance(getattr(body[0], "value", None), ast.Constant) \
                and isinstance(body[0].value.value, str):
            for i in range(body[0].lineno - 1, body[0].end_lineno):
                mask[i] = True
    return mask


def comment_mask_prefix(lines, prefixes):
    mask = [False] * len(lines)
    for i, line in enumerate(lines):
        s = line.strip()
        if i == 0 and s.startswith("#!"):
            continue
        mask[i] = s.startswith(prefixes)
    return mask


def runs(mask):
    out, start, n = [], -1, 0
    for i, flag in enumerate(mask + [False]):
        if flag:
            if n == 0:
                start = i
            n += 1
        else:
            if n > MAX_RUN:
                out.append((start + 1, n))
            n = 0
    return out


def tracked_files(root):
    res = subprocess.run(["git", "ls-files", "-z"], cwd=root,
                         capture_output=True, check=True)
    return [f for f in res.stdout.decode().split("\0") if f]


def check_file(root, rel):
    kind = classify(rel)
    if kind is None:
        return []
    full = os.path.join(root, rel)
    if not os.path.isfile(full):
        return []
    with open(full, encoding="utf-8", errors="replace") as fh:
        text = fh.read()
    lines = text.split("\n")
    problems = []
    if kind == "md":
        for name, rx in FORBIDDEN_MD:
            for i, line in enumerate(lines):
                if rx.search(line):
                    problems.append((i + 1, name))
        return problems
    if kind == "c":
        mask = comment_mask_c(lines)
    elif kind == "hash":
        mask = comment_mask_prefix(lines, ("#",))
    else:
        mask = comment_mask_prefix(lines, ("#", ";"))
    for start, n in runs(mask):
        problems.append((start, f"comment run of {n} lines (max {MAX_RUN})"))
    # Docstrings are documentation too: same vocabulary rules, no length limit.
    if rel.endswith(".py"):
        mask = [a or b for a, b in zip(mask, docstring_mask(text, lines))]
    for i, (line, is_comment) in enumerate(zip(lines, mask)):
        if not is_comment:
            continue
        for name, rx in FORBIDDEN:
            if rx.search(line):
                problems.append((i + 1, f"forbidden in comment: {name}"))
    return problems


def main():
    ap = argparse.ArgumentParser(description=__doc__.split("\n")[0])
    ap.add_argument("--root", default=None, help="repository root (default: git toplevel)")
    ap.add_argument("files", nargs="*", help="limit to these paths (relative to root)")
    args = ap.parse_args()
    root = args.root or subprocess.run(
        ["git", "rev-parse", "--show-toplevel"], capture_output=True,
        text=True, check=True).stdout.strip()
    files = [os.path.relpath(os.path.abspath(f), root) for f in args.files] or tracked_files(root)
    total = 0
    for rel in files:
        for line, msg in sorted(check_file(root, rel)):
            print(f"{rel}:{line}: {msg}")
            total += 1
    if total:
        print(f"{total} comment-hygiene violation(s)", file=sys.stderr)
        return 1
    return 0


if __name__ == "__main__":
    sys.exit(main())
