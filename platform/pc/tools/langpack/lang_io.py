"""lang_io.py -- UTF-8 JSON I/O for the whole langpack toolchain, in one place.

Pack text is UTF-8 end to end (dialogue carries real UTF-8; every write uses ensure_ascii=False), and
a translator edits the working-set JSON in their own editor -- which saves UTF-8. But Python's bare
open() uses the PLATFORM LOCALE, not UTF-8: on Linux that happens to be UTF-8, so it works; on Windows
(the translator's platform) it is the ANSI code page, so a UTF-8 file with Cyrillic or accents is
silently mis-decoded on read and a UnicodeEncodeError is raised on write. Routing every json.load /
json.dump through here forces UTF-8 regardless of platform, so the chain behaves the same everywhere --
and the next open() someone adds inherits it for free instead of re-introducing the bug.
"""
import json


def fnv1a(data):
    """64-bit FNV-1a -- THE pack identity hash, in exactly one place.

    This value is the contract between the exporter (literal keys), the builder (K_LITERAL
    records), and the runtime (PC_LangStr in platform/pc/src/pc_lang.c hashes the literal's
    bytes with the same constants): if any party computes it differently, translated literals
    silently stop matching and drop out of packs. str input is hashed as its UTF-8 bytes --
    identical to hashing the encoded bytes directly."""
    if isinstance(data, str):
        data = data.encode("utf-8")
    h = 14695981039346656037
    for b in data:
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


def load_json(path):
    """Read a JSON file as UTF-8. A file saved in another encoding fails with a plain, actionable
    message (name the file, say 'save as UTF-8') instead of a codec traceback or silent mojibake."""
    try:
        with open(path, encoding="utf-8") as f:
            return json.load(f)
    except UnicodeDecodeError as e:
        raise SystemExit(f"{path}: not valid UTF-8 (byte 0x{e.object[e.start]:02X} at offset "
                         f"{e.start}). Re-save this file as UTF-8 -- most editors offer "
                         f"'Save with Encoding' / 'Reopen with Encoding'.")


def write_json(obj, path):
    """Write a JSON file as UTF-8, non-ASCII kept literal (ensure_ascii=False) -- the toolchain's one
    dump style. UTF-8 can encode every codepoint, so this never raises on the content."""
    with open(path, "w", encoding="utf-8") as f:
        json.dump(obj, f, indent=1, ensure_ascii=False)
