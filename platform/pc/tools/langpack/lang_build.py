#!/usr/bin/env python3
"""lang_build.py -- compile an edited working set into a language pack the port can load.

MVP scope (exchange/80, F1): prove the PLUMBING end to end with English as the baseline. Encoding is
ASCII/Shift-JIS passthrough; the font + charmap work (the 128-slot code space) comes later and does
not change any interface here.

DESIGN: the pack is a DIFF, not a rebuild.
Every section starts from the ORIGINAL bytes on the disc and applies only the entries whose `text`
field is non-empty. Consequences that matter:
  * an unedited working set produces an EMPTY pack -- so "no edits" is provably a no-op, and the
    identity case needs no argument;
  * a changed entry is the only thing that can differ, so a regression is attributable;
  * dialogue keeps its file framing (header lines, blank-line entry toggles, END, trailing bytes)
    byte-for-byte, because we never re-synthesise the file -- we substitute lines inside it.

Fixed-width tables are shipped as the whole rebuilt blob (the game indexes them directly, so the
runtime just memcpy's). Pointer tables ship only the edited entries: the runtime allocates the new
strings and repoints those slots, so a translation is not bounded by the original string's length.
Dialogue ships the whole patched file, keyed by its ISO LBA -- which is what the game's own
gCdFiles[] holds, so the runtime can match a read without knowing any filenames.

Self-check: every UNEDITED entry is re-encoded and compared against the disc bytes. If the encoder
cannot reproduce what the game shipped, the build fails rather than emitting a subtly wrong pack.

Usage: ./lang_build.py <disc.bin> <workdir> <outdir> [--lang en] [--packart <dir>]
                       [--allow-incomplete]
       --packart supplies the glyph sheets a non-Latin script needs; it also switches the pack to
       1-byte codes throughout (script mode), since UTF-8 cannot carry Cyrillic through dialogue.
       --allow-incomplete builds a non-Latin pack that still has untranslated strings (each renders
       as nonsense) -- for testing only; a finished non-Latin pack must be complete.
       -> <outdir>/langpacks/<lang>/{manifest.json,strings.bin}
"""
import json, os, re, struct, sys, unicodedata

from lang_export import read_exe, foff, _iso, TEXT_RX, SECTOR, walk_dialogue

MAGIC = b"VHLANG\x01\x00"
K_FIXED, K_PTR, K_TEXT, K_FONT, K_CHARMAP, K_KROM, K_LITERAL = 1, 2, 3, 4, 5, 6, 7
GTEXT_BYTES = 10928   # symbol_addrs.txt: gText size 0x2ab0 -- LoadText unpacks a whole file here
FONT_VRAM = 0x801012e4   # sFontGlyphBitmaps[128][9] -- base letterforms for glyph synthesis

# id, name, kind, count, record width (fixed only), pad byte(s)
# Padding is per table and was READ OFF THE DISC, not assumed: character and spell names are plain
# ASCII padded with NUL; only item names are full-width Shift-JIS, padded with 0x8140. (Each table's
# slot 0 is an all-filler "empty" record -- 0x8140s in the SJIS-adjacent tables, NUL in names -- which
# is why unedited records are always kept verbatim rather than re-synthesised.)
TABLES = [
    (0, "gCharacterNames",    0x800eaf58, "fixed",  35,  7, b"\x00"),
    (1, "gItemNamesSjis",     0x800eed20, "fixed", 101, 17, b"\x81\x40"),
    (2, "gSpellNames",        0x800ee410, "fixed",  72, 21, b"\x00"),
    (3, "gStringTable",       0x8010102c, "ptr",   100, None, None),
    (4, "gSpellDescriptions", 0x800ee9f8, "ptr",    72, None, None),
    (5, "gItemDescriptions",  0x800ef3d8, "ptr",   101, None, None),
    (6, "gItemDescriptions2", 0x800ef56c, "ptr",   101, None, None),
    # Appended (ids never renumbered -- the runtime's table matches by id): the three StringToGlyphs
    # tables the step-1 probe run uncovered. See lang_export.py's note.
    (7, "gUnitTypeNames",         0x800eb050, "fixed",  86, 11, b"\x00"),
    (8, "gItemNames",             0x800eb404, "fixed", 139, 13, b"\x00"),
    (9, "gClassAdvancementNames", 0x801f6a34, "fixed",  18, 17, b"\x00"),
    # Function-static in battle_0201b8.c: the runtime applies it through a PC_FEAT hook rather than
    # by symbol, but the pack format treats it like any other fixed table.
    (10, "terrainText",           0x800f29f4, "fixed",  10, 12, b"\x00"),
]
LOAD = 0x80010000


def enc_sjis(s):
    """Reverse of the exporter's cp932+NFKC decode: ASCII -> full-width -> cp932 bytes."""
    wide = "".join("　" if c == " " else
                   chr(ord(c) + 0xFEE0) if 0x21 <= ord(c) <= 0x7E else c
                   for c in s)
    return wide.encode("cp932")


def enc_plain(s):
    return s.encode("latin1")


def drawn_chars(s):
    """Yield the characters the game actually DRAWS, consuming control codes exactly as the
    message-box parser does (src/text.c Objf351_MsgBoxText, cases '$' and '#'):
      $W $F $P $O eat their operand letter; $S $T eat the letter then the digits after it;
      #<digits> is a string-table insertion (eaten); the first '#' of '##' is eaten, the second
      draws. A '$'/'#' before an UNRECOGNISED byte is consumed alone, so that byte draws -- which
      is the parser's own fallthrough. Used to tell a consumed control-code operand (safe) from a
      drawn character (which a non-Latin pack must be able to render)."""
    i, n = 0, len(s)
    while i < n:
        c = s[i]
        if c == "$" and i + 1 < n:
            code = s[i + 1]
            if code in "WwFfPpOo":
                i += 2; continue                       # $ + operand letter, both consumed
            if code in "SsTt":
                i += 2                                 # $ + operand letter
                while i < n and s[i].isdigit():
                    i += 1                             # ...then the numeric argument
                continue
            i += 1; continue                           # unrecognised: $ eaten, operand drawn next
        if c == "#" and i + 1 < n:
            if s[i + 1] == "#":
                i += 1                                 # first # eaten; the second draws
                yield "#"; i += 1; continue
            if s[i + 1].isdigit():
                i += 2
                while i < n and s[i].isdigit():
                    i += 1
                continue
            i += 1; continue                           # lone # before a non-digit: eaten
        yield c
        i += 1


def pack_code_collisions(s, charmap):
    """The DRAWN ASCII characters of `s` whose byte a script-mode pack reassigns to a glyph slot.
    Emitting one would draw the wrong script's letter -- the silent-nonsense failure a non-Latin
    pack must never ship. Control-code operands are excluded because the parser consumes them."""
    return sorted({ch for ch in drawn_chars(s)
                   if ord(ch) < 0x80 and ord(ch) in charmap.pack_code_bytes})


def fnv1a_str(text):
    h = 14695981039346656037
    for b in text.encode("utf-8"):
        h = ((h ^ b) * 1099511628211) & 0xFFFFFFFFFFFFFFFF
    return h


# --- UTF-8 + glyph synthesis (decision D1/D2, exchange/80) ------------------------------------
# Pointer strings carry REAL UTF-8; the engine (pc_lang_font.c) draws any codepoint the pack ships
# a glyph for. Glyphs for accented Latin are SYNTHESISED from the disc's own letterforms: the US
# font already contains lowercase a-z at indices 13-38 (uppercase at 68-93), and an accent is a few
# pixels OR'd into the rows the base letter leaves blank.
#
# Marks are drawn in rows 0-1, which lowercase letterforms leave empty -- UPPERCASE occupies row 1,
# so uppercase accents are NOT synthesisable and error out ("needs pack art") rather than merging
# into the letter. Cedilla uses row 8, blank in every US letterform.
MARKS = {
    0x301: [(0, 0x08), (1, 0x10)],   # acute        ´
    0x300: [(0, 0x10), (1, 0x08)],   # grave        `
    0x302: [(0, 0x10), (1, 0x28)],   # circumflex   ^
    0x308: [(0, 0x28)],              # diaeresis    ¨
    0x303: [(0, 0x14), (1, 0x28)],   # tilde        ~
    0x327: [(8, 0x10)],              # cedilla      ¸  (below)
}


ABOVE_MARKS = {0x300, 0x301, 0x302, 0x308, 0x303}   # everything but the cedilla sits ON TOP


def synth_one(exe, cp):
    """One codepoint -> 9-byte bitmap, or None if not synthesisable from the US font."""
    o = foff(FONT_VRAM)
    d = unicodedata.normalize("NFD", chr(cp))
    if not d or not ("a" <= d[0] <= "z") or len(d) < 2:
        return None                                   # uppercase: row-1 collision; see MARKS note
    rows = list(exe[o + (13 + ord(d[0]) - 97) * 9:][:9])
    shift = 0
    if d[0] in "ij" and any(ord(m) in ABOVE_MARKS for m in d[1:]):
        # Typography, caught in-game (L12 read as "i with a cross on top"): an above-mark REPLACES
        # the dot of i/j -- î is dotless-i + circumflex, never dot + circumflex. Clear everything
        # above the letter body (the dot sits isolated at row 2, body starts row 4), then seat the
        # mark one row lower so it does not float over the empty dot row.
        rows[0] = rows[1] = rows[2] = rows[3] = 0
        shift = 1
    for m in d[1:]:
        if ord(m) not in MARKS:
            return None
        for r, bits in MARKS[ord(m)]:
            rows[r + shift] |= bits
    return bytes(rows)


def synth_glyphs(exe, cps, errors):
    """codepoints -> sorted [(cp, 9-byte bitmap)]; unsynthesisable ones become build errors."""
    out = []
    for cp in sorted(cps):
        rows = synth_one(exe, cp)
        if rows is None:
            errors.append(f"U+{cp:04X} {chr(cp)!r}: not synthesisable from the US font "
                          f"(lowercase+mark only) -- supply a drawn glyph for it via --packart")
            continue
        out.append((cp, rows))
    return out


# --- pack-supplied glyph art (NON-LATIN packs) ------------------------------------------------
# A script the game has never drawn cannot be synthesised: there is no base letterform to build on.
# Such a pack ships two sheets, small and large, each a 1-bit PNG of packed cells plus a manifest
# listing one codepoint per cell in reading order. See the tools README.
def load_packart(d, errors):
    """<dir> -> ({cp: 9 rows of 8 bits}, {cp: 15 rows of 16 bits}); either may be empty."""
    try:
        from PIL import Image
    except ImportError:
        errors.append("pack art needs Pillow (pip install pillow)")
        return {}, {}

    def one(png, txt, w, h):
        if not (os.path.exists(png) and os.path.exists(txt)):
            return {}
        cps = [int(l.split()[0][2:], 16) for l in open(txt, encoding="utf-8")
               if l.strip() and not l.startswith("#")]
        im = Image.open(png).convert("1")
        per_row = im.width // w
        out = {}
        for n, cp in enumerate(cps):
            cx, cy = (n % per_row) * w, (n // per_row) * h
            rows = []
            for y in range(h):
                v = 0
                for x in range(w):
                    if im.getpixel((cx + x, cy + y)) == 0:      # black = ink
                        v |= 1 << (w - 1 - x)
                rows.append(v)
            if any(rows):                                        # a blank cell means "not supplied"
                # 8-wide rows become bytes, matching what synth_one returns so both glyph sources
                # are interchangeable downstream; 16-wide rows stay ints for the krom packer.
                out[cp] = bytes(rows) if w == 8 else rows
        return out

    small = one(os.path.join(d, "font8x9.png"),   os.path.join(d, "font8x9.txt"),   8, 9)
    big   = one(os.path.join(d, "font16x15.png"), os.path.join(d, "font16x15.txt"), 16, 15)
    if not small:
        errors.append(f"{d}: no usable font8x9.png + font8x9.txt (the small sheet is mandatory)")
    return small, big


# --- 1-byte pack codes for fixed-width tables (decision D2, exchange/80) -----------------------
# Fixed records keep byte = char = column: a non-ASCII character there gets a FREE 1-BYTE CODE
# assigned by the builder, the retail map is rewritten so that code names a FREE GLYPH SLOT, and
# the synthesised bitmap is written into that slot (all via the K_CHARMAP section + text.c's
# hand-off hook). Provenance of the pools:
#   codes: printable ASCII whose retail map entry is 0 (renders blank), minus '#'/'$' (parser
#          markup) and minus any character that actually appears in retail text -- a code that
#          retail uses would suddenly render as the pack glyph wherever retail draws it;
#   slots: 111-127 ONLY (17 cells). ⚠ NOT slot 1: it is blank in sFontGlyphBitmaps but in the
#          SHEET it is GLYPH_BG -- the window background tile. Assigning it stamped an accent over
#          every window fill in the game (increment-4 captures, "huge bleeding in other menus").
#          The free set is the INTERSECTION of blank-in-bitmaps and unnamed-in-sheet, and 1 is
#          named. With D3 (widen sFontGlyphBitmaps to [156]) the shared range becomes 111-155 = 45.
RETAIL_MAP = [
    128, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
    0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 128, 97, 40, 42, 0, 99, 0, 39, 0, 0, 0, 102,
    95, 12, 94, 101, 2, 3, 4, 5, 6, 7, 8, 9, 10, 11, 96, 0, 41, 100, 0, 98, 0, 68,
    69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80, 81, 82, 83, 84, 85, 86, 87, 88, 89, 90,
    91, 92, 93, 0, 0, 0, 0, 0, 0, 68, 69, 70, 71, 72, 73, 74, 75, 76, 77, 78, 79, 80,
    81, 82, 83, 84, 85, 86, 87, 88, 89, 90, 91, 92, 93, 0, 0, 0, 0, 0]

# Fixed tables that carry 1-byte pack codes. Since the F_WD sheet patch (Path 1b), this includes
# the strip-path names: the same K_CHARMAP records feed BOTH stores (bitmaps via the text.c
# hand-off, sheet cells via the LoadImage-side stamp).
CHARMAP_TABLES = {"gSpellNames", "gClassAdvancementNames", "terrainText",
                  "gCharacterNames", "gUnitTypeNames", "gItemNames"}


# --- 16x15 wide glyphs for the SJIS path (Path 2: the krom extension) --------------------------
# gItemNamesSjis draws through DrawSjisGlyph -> Krom2RawAdd, which on PC is OUR map + OUR table
# (libkernel.c / pc_kanji_font.c). A pack assigns accented characters 2-byte codes at 0x8440+ (a
# range the retail map never answers) and ships 16x15 bitmaps; the runtime consults the pack first.
# Base letterforms come from the BIOS charset itself (parsed out of pc_kanji_font.c), composed with
# the marks below. At 15 rows there is room to SHIFT: uppercase (ink from row 1) moves down one row
# to free the top for the mark -- so this path supports TRUE UPPERCASE ACCENTS, unlike 8x9.
KROM_MARKS = {                       # two 16-bit rows per mark, MSB = leftmost pixel
    0x301: [0x00C0, 0x0300],         # acute
    0x300: [0x0600, 0x00C0],         # grave
    0x302: [0x0180, 0x0660],         # circumflex
    0x308: [0x0660, 0x0660],         # diaeresis
    0x303: [0x0320, 0x04C0],         # tilde
    0x327: [0x0080, 0x0180],         # cedilla (below)
}
KROM_A, KROM_LOWER_A = 157, 183      # libkernel.c sjis_to_krom_glyph: A-Z at 157+, a-z at 183+


def load_krom_base():
    import re as _re2
    src = open(os.path.join(os.path.dirname(os.path.abspath(__file__)),
                            "..", "..", "src", "pc_kanji_font.c")).read()
    body = _re2.search(r"pc_kanji_charset2\[\d+\]\s*=\s*\{(.*?)\};", src, _re2.S).group(1)
    return bytes(int(x, 16) for x in _re2.findall(r"0x([0-9a-fA-F]{2})", body))


def krom_synth(krom_data, cp):
    """One codepoint -> 30-byte 16x15 bitmap, or None. Dynamic mark placement: shift the letter
    down (or up, for a below-mark) inside the 15 rows to make room, instead of refusing."""
    d = unicodedata.normalize("NFD", chr(cp))
    if len(d) < 2:
        return None
    base = d[0]
    if "A" <= base <= "Z":
        idx = KROM_A + ord(base) - 65
    elif "a" <= base <= "z":
        idx = KROM_LOWER_A + ord(base) - 97
    else:
        return None
    g = krom_data[idx * 30:(idx + 1) * 30]
    rows = [(g[r * 2] << 8) | g[r * 2 + 1] for r in range(15)]
    marks = [ord(m) for m in d[1:]]
    if any(m not in KROM_MARKS for m in marks):
        return None
    above = [m for m in marks if m != 0x327]
    below = [m for m in marks if m == 0x327]

    if base in "ij" and above:                       # accent replaces the dot (typography rule)
        inked = [r for r, v in enumerate(rows) if v]
        gaps = [r for r in range(inked[0], inked[-1]) if not rows[r]]
        if gaps:                                     # clear the dot: everything above the first gap
            for r in range(0, gaps[0] + 1):
                rows[r] = 0

    def top():  return next((r for r, v in enumerate(rows) if v), 15)
    def bot():  return max((r for r, v in enumerate(rows) if v), default=0)

    if above:
        shift = max(0, 3 - top())                    # want mark(2) + gap(1) above the letter
        if bot() + shift > 14:
            shift = max(0, 2 - top())                # no gap, mark touches
        if bot() + shift > 14 or len(above) > 1:
            return None
        rows = [0] * shift + rows[:15 - shift]
        m0 = top() - (3 if top() >= 3 else 2)
        for i2, bits in enumerate(KROM_MARKS[above[0]]):
            rows[m0 + i2] |= bits
    if below:
        up = max(0, bot() + 2 - 14)
        if top() - up < 0:
            return None
        rows = rows[up:] + [0] * up
        b = bot()
        for i2, bits in enumerate(KROM_MARKS[below[0]]):
            rows[b + 1 + i2] |= bits
    out = bytearray()
    for v in rows:
        out += bytes([(v >> 8) & 0xFF, v & 0xFF])
    return bytes(out)


class KromAssign:
    def __init__(self, art=None):
        self.art = art or {}          # cp -> 15 rows of 16 bits, from the pack's large sheet
        self.krom_data = None
        self.assigned = {}   # cp -> (code, rows30)
        self.next = 0x8440   # DrawSjisGlyph's own guard ends at 0x843f; retail map never answers here

    def code_for(self, ch, errors, ctx):
        cp = ord(ch)
        if cp in self.assigned:
            return self.assigned[cp][0]
        if cp in self.art:
            rows = b"".join(struct.pack(">H", r) for r in self.art[cp])   # 15 rows, MSB left
        else:
            if self.krom_data is None:
                self.krom_data = load_krom_base()
            rows = krom_synth(self.krom_data, cp)
        if rows is None:
            errors.append(f"{ctx}: {ch!r} has no 16x15 glyph -- not synthesisable and absent from "
                          f"the pack's font16x15 sheet (item names need it)")
            return None
        code = self.next
        self.next += 1
        if (self.next & 0xFF) == 0x7F:               # skip the invalid SJIS second byte
            self.next += 1
        self.assigned[cp] = (code, rows)
        return code

    def section(self):
        recs = sorted((code, rows) for code, rows in self.assigned.values())
        if not recs:
            return None
        blob = struct.pack("<I", len(recs))
        for code, rows in recs:
            blob += struct.pack("<H", code) + rows
        return blob


class CharmapAssign:
    def __init__(self, exe, retail_chars, art=None):
        self.exe = exe
        self.art = art or {}          # cp -> 9 rows, for scripts that cannot be synthesised
        self.assigned = {}   # cp -> (code, slot, rows)
        if art:
            # SCRIPT MODE. A pack that replaces EVERY string leaves no untranslated English to
            # collide with, so the letter bytes become assignable -- which is the whole reason a
            # non-Latin alphabet fits at all (33 letters against ~17 spare punctuation codes
            # otherwise). Uppercase first, punctuation after, so the common case is predictable.
            # Digits and punctuation the game still prints are never taken.
            self.codes = ([b for b in range(0x41, 0x5B)] +
                          [b for b in range(0x21, 0x7F)
                           if RETAIL_MAP[b] == 0 and b not in (0x23, 0x24)
                           and chr(b) not in retail_chars])
        else:
            self.codes = [b for b in range(0x21, 0x7F)
                          if RETAIL_MAP[b] == 0 and b not in (0x23, 0x24)
                          and chr(b) not in retail_chars]
        # The bytes this pack can hand to a glyph slot -- captured before any get popped, so the
        # collision guard (pack_code_collisions) can tell whether a DRAWN ASCII byte would be
        # reassigned. Script mode adds A-Z here; a Latin pack never reassigns letters.
        self.pack_code_bytes = set(self.codes)
        # Free glyph slots, 44 of them. Two exclusions inside this span, both learned the hard
        # way and both invisible until something renders:
        #   slot 1   = GLYPH_BG, the window background tile in the F_WD sheet (assigning it
        #              tiled every window in the game with a letter)
        #   slot 128 = where the retail map sends NUL and space; it has to stay blank
        # The upper bound tracks src/text.c's PC-side sFontGlyphBitmaps[156] and DrawFontGlyph's
        # matching index guard -- raise both together or glyphs simply do not draw.
        self.slots = list(range(111, 128)) + list(range(129, 156))

    def reserve_literal(self, ch):
        """The translator used this ASCII char literally: it must never become a pack code."""
        b = ord(ch)
        if b in self.codes:
            self.codes.remove(b)
        return any(cp for cp, (code, _, _) in self.assigned.items() if code == b)

    def code_for(self, ch, errors, ctx):
        cp = ord(ch)
        if cp in self.assigned:
            return self.assigned[cp][0]
        rows = self.art.get(cp) or synth_one(self.exe, cp)
        if rows is None:
            errors.append(f"{ctx}: {ch!r} has no glyph -- not synthesisable from the US font "
                          f"(lowercase+mark only) and absent from the pack's font8x9 sheet")
            return None
        if not self.slots:
            errors.append(f"{ctx}: out of glyph SLOTS for {ch!r} "
                          f"(44 available: 111-127 and 129-155)")
            return None
        if not self.codes:
            # The usual wall, and it is the CODE pool rather than the slot pool: a pack code has
            # to be a byte that untranslated retail text can never produce, so every byte English
            # text uses is off-limits, leaving ~17. A pack that translates EVERYTHING frees the
            # letter bytes too -- that is the non-Latin mode, still to be built.
            errors.append(f"{ctx}: out of free CODES for {ch!r} "
                          f"(~17 bytes are unused by retail English text; a full-script pack "
                          f"needs the non-Latin mode, which reclaims the letter bytes)")
            return None
        code, slot = self.codes.pop(0), self.slots.pop(0)
        self.assigned[cp] = (code, slot, rows)
        return code

    def section(self):
        recs = sorted((code, slot, rows) for code, slot, rows in self.assigned.values())
        if not recs:
            return None
        blob = struct.pack("<I", len(recs))
        for code, slot, rows in recs:
            blob += bytes([code, slot]) + rows
        return blob


def enc_codes(s, charmap, errors, ctx):
    """SCRIPT MODE encoding: every non-ASCII character becomes its 1-byte pack code.

    Latin packs put UTF-8 in pointer strings and dialogue (decision D1) because the glyph table is
    unbounded there. A non-Latin pack cannot: Cyrillic in UTF-8 puts continuation bytes inside
    0x81-0x9F, which the engine's two byte-pairing consumers read as SJIS leads (see
    dialogue_bytes_safe). 1-byte codes sidestep that entirely and are what the fan translation used.
    Returns None if any character has no code, so the caller can skip the entry."""
    # A non-Latin pack reassigns the letter (and some punctuation) bytes to its own glyphs, so any
    # LATIN character left in the text -- an untranslated word, a Latin proper noun -- would draw as
    # a Cyrillic/Greek letter, not itself. That is the silent nonsense the framework exists to stop,
    # and unlike a Latin pack it cannot degrade gracefully. Refuse it at build time. (Control-code
    # operands like the W of $W are consumed by the parser, never drawn, so drawn_chars excludes
    # them -- which is why the proven Russian pack, full of $W/$T6, still builds.)
    bad = pack_code_collisions(s, charmap)
    if bad:
        errors.append(f"{ctx}: {', '.join(repr(c) for c in bad)} would draw as a pack glyph, not "
                      f"itself -- Latin left in a non-Latin pack renders as nonsense; translate or "
                      f"remove it")
        return None
    out = bytearray()
    for ch in s:
        if ch == "\n":
            out.append(0x0A)
        elif ord(ch) < 0x80:
            out.append(ord(ch))
        else:
            code = charmap.code_for(ch, errors, ctx)
            if code is None:
                return None
            out.append(code)
    return bytes(out)


def build_fixed(exe, vram, count, width, pad, entries, name, errors, charmap=None, krom=None,
                script=False):
    """Whole-table blob: original record bytes, with edited records re-encoded."""
    base = foff(vram)
    blob = bytearray(exe[base:base + count * width])
    edited = 0
    sjis = (name == "gItemNamesSjis")
    for i in range(count):
        e = entries[i]
        orig = bytes(blob[i * width:(i + 1) * width])
        want = e.get("text") or ""
        en = e.get("en") or ""
        src = want if want else en
        if want and sjis and not want.isascii():
            # Path 2 (krom): accented chars become 2-byte pack codes at 0x8440+, big-endian on the
            # wire (DrawSjisGlyph reads (p[0]<<8)|p[1]). 2 bytes per glyph, budgets unchanged.
            out, ok = bytearray(), True
            for c in want:
                if c.isascii():
                    out += enc_sjis(c)
                else:
                    code = krom.code_for(c, errors, f"{name}[{i}]")
                    if code is None:
                        ok = False; break
                    out += bytes([(code >> 8) & 0xFF, code & 0xFF])
            if not ok:
                continue
            raw = bytes(out)
        elif want and not sjis and not want.isascii():
            if charmap is None:
                errors.append(f"{name}[{i}]: non-ASCII in a fixed-width table without a charmap")
                continue
            # D2: encode with 1-byte pack codes -- byte = char = column, budgets unchanged.
            out, ok = bytearray(), True
            for c in want:
                if c.isascii():
                    if charmap.reserve_literal(c):
                        errors.append(f"{name}[{i}]: literal {c!r} collides with an assigned pack "
                                      f"code -- reorder edits or avoid that character")
                        ok = False; break
                    out.append(ord(c))
                else:
                    code = charmap.code_for(c, errors, f"{name}[{i}]")
                    if code is None:
                        ok = False; break
                    out.append(code)
            if not ok:
                continue
            raw = bytes(out)
        else:
            # A pure-ASCII TRANSLATED field skips the charmap branch above, so in script mode its
            # letter bytes would reach enc_plain and then draw as reassigned pack glyphs -- the same
            # nonsense enc_codes refuses on the pointer/dialogue paths. Guard it here too.
            if want and script and not sjis and charmap is not None:
                bad = pack_code_collisions(want, charmap)
                if bad:
                    errors.append(f"{name}[{i}]: {', '.join(repr(c) for c in bad)} would draw as a "
                                  f"pack glyph, not itself -- Latin in a non-Latin pack renders as "
                                  f"nonsense; translate or remove it")
                    continue
            try:
                raw = enc_sjis(src) if sjis else enc_plain(src)
            except Exception as ex:
                errors.append(f"{name}[{i}]: cannot encode ({ex})"); continue
        if len(raw) > width - 1:
            errors.append(f"{name}[{i}]: {len(raw)} bytes, record holds {width - 1}")
            continue
        rec = bytearray(pad * width)[:width]         # re-pad in full: a SHORTER replacement must not
        rec[:len(raw)] = raw                         # leave the old text's tail behind
        rec[width - 1] = 0
        if not want:
            # Self-check on REAL content only: re-encoding what the game shipped must reproduce it.
            # Empty slots are all-filler records (see the table note) and are exempt.
            if en and bytes(rec) != orig:
                errors.append(f"{name}[{i}]: round-trip mismatch -- encoder disagrees with the disc "
                              f"(disc {orig.hex()} != rebuilt {bytes(rec).hex()})")
            continue                                 # unedited: keep the disc's bytes verbatim
        blob[i * width:(i + 1) * width] = rec
        edited += 1
    return bytes(blob), edited


def build_ptr(exe, vram, count, entries, name, errors, used_cps, charmap=None):
    """Only edited slots travel: (index, bytes). Length is unconstrained -- the runtime allocates.
    Encoding is UTF-8 (D1): the engine hook in DrawText_Internal consumes multi-byte sequences as
    one column each. Non-ASCII codepoints are collected so the font section can cover them."""
    out, n = bytearray(), 0
    for i in range(count):
        e = entries[i]
        want = e.get("text") or ""
        if not want:
            continue
        if charmap is not None:                    # script mode: 1-byte codes, no UTF-8 anywhere
            raw = enc_codes(want, charmap, errors, f"{name}[{i}]")
            if raw is None:
                continue
        else:
            raw = want.encode("utf-8")
            used_cps.update(ord(c) for c in want if ord(c) > 0x7F)
        if len(raw) > 511:
            errors.append(f"{name}[{i}]: {len(raw)} bytes, cap is 511"); continue
        out += struct.pack("<HH", i, len(raw)) + raw
        n += 1
    return struct.pack("<I", n) + bytes(out), n


def dialogue_bytes_safe(raw):
    """Dialogue lines travel through TWO byte-pairing consumers before our engine ever sees them:
    CopySjisString (LoadText's unpacker) and the message-box parser's SJIS branch, both of which
    treat 0x81-0x9F / 0xE0-0xFC as a 2-byte lead and consume the NEXT byte with it. A UTF-8 byte in
    those ranges can therefore swallow a line's NUL terminator (buffer overrun) or bypass our hook.
    RULE: every byte of an encoded dialogue line must avoid both ranges. In practice: all lowercase
    accented Latin (U+00E0-00FF) and A-grave pass; other UPPERCASE accents and every 3-/4-byte
    codepoint fail -- which matches the glyph-synthesis constraint anyway."""
    return not any(0x81 <= b <= 0x9F or 0xE0 <= b <= 0xFC for b in raw)


def gtext_occupancy(plain):
    """Exact bytes LoadText writes into gText[10928] for a decoded dialogue file -- simulated the way
    the game does it (src/text.c LoadText/CopySjisString/DecodeLineOfText), because reconstructing it
    from the working set's entries silently under-counts:
      * every CONTENT line costs len + 1 -- DecodeLineOfText appends '\\n' and CopySjisString copies
        it (the '\\n' is not a byte-pair lead, so it is one byte);
      * every entry CLOSE and the terminating END each write one NUL;
      * a blank line closes the current entry AND re-opens the next (LoadText does not advance its
        input on the close), so blanks between entries are counted once as a close;
      * entries the exporter dropped as empty are still blank lines here, so their NULs are counted.
    Operates on the PATCHED bytes, so the per-line length already reflects the real encoding (1-byte
    script codes or multi-byte UTF-8) -- no separate mode arithmetic needed."""
    lines = plain.split(b"\r\n")
    reading, entry, total, i = 0, 1, 0, 0
    while entry <= 100 and i < len(lines):
        ln = lines[i]
        if ln[:3] == b"END":
            total += 1                      # END writes a NUL, then stops
            break
        if ln == b"":
            if reading == 0:
                reading = 1; entry += 1; i += 1       # open: advance past the blank
            else:
                reading = 0; total += 1               # close: NUL, re-read the same blank (no advance)
            continue
        if len(ln) >= 2 and (0x81 <= ln[0] <= 0x9F or 0xE0 <= ln[0] <= 0xFC) and ln[1] == 0x94:
            i += 1; continue                # SJIS-comment line: skipped, not copied
        total += len(ln) + 1                # content + the appended '\n' -- the game's else branch
        i += 1                              # copies regardless of entry state, so do not gate on it
    return total


def build_text(raw_file, doc, stem, budget, errors, used_cps, charmap=None):
    """Substitute edited LINES inside the decoded file, keeping every other byte untouched."""
    edits = {}
    for ent in doc["entries"]:
        edits[ent["key"]] = ent
    plain = bytearray(~b & 0xFF for b in raw_file)
    lines = plain.split(b"\r\n")
    changed = 0
    # ONE walker, shared with the exporter (walk_dialogue), so the entry/line numbering that keys the
    # translations here is guaranteed to match the numbering the translator saw.
    for n, li, idx, ln in walk_dialogue(lines):
        ent = edits.get(f"{stem}[{n}]")
        if not ent or li >= len(ent.get("text", [])):
            continue
        want = ent["text"][li]
        if not want:
            continue
        if charmap is not None:                    # script mode: 1-byte codes
            raw = enc_codes(want, charmap, errors, f"{stem}[{n}] line {li}")
            if raw is None:
                continue
            lines[idx] = raw
            changed += 1
            continue
        raw = want.encode("utf-8")
        if not dialogue_bytes_safe(raw):
            bad = ", ".join(f"{c!r}" for c in want if not dialogue_bytes_safe(c.encode("utf-8")))
            errors.append(f"{stem}[{n}] line {li}: {bad} unsafe in dialogue (byte collides "
                          f"with the engine's SJIS lead ranges) -- lowercase accents are "
                          f"safe, most uppercase accents are not")
        else:
            lines[idx] = raw
            used_cps.update(ord(c) for c in want if ord(c) > 0x7F)
            changed += 1
    if not changed:
        return None, 0
    new = b"\r\n".join(lines)
    if len(new) < len(plain):                       # keep the original tail; only length may shrink
        new = new + plain[len(new):]
    if len(new) > budget:
        errors.append(f"{stem}: patched file is {len(new)} B, the game reads only {budget} B")
        return None, 0
    # SECOND budget, and it is a different one: LoadText UNPACKS the whole file into gText[10928],
    # one shared buffer, so a file's entries must also fit there once the framing is stripped. Measure
    # it by simulating LoadText on the PATCHED bytes -- exact, and it charges the real encoding a
    # script pack writes (1-byte codes, not UTF-8; charging UTF-8 lengths once inflated SAKABA_T from
    # its real ~8.6 KB to 14.7 KB and failed on text the retail game loads without trouble).
    unpacked = gtext_occupancy(new)
    if unpacked > GTEXT_BYTES:
        errors.append(f"{stem}: unpacks to {unpacked} B, gText holds {GTEXT_BYTES} B")
        return None, 0
    return bytes(~b & 0xFF for b in new), changed


PACK_NAME_RX = None   # compiled below; module-level so the CLI and build() share one rule


def check_pack_name(lang):
    """Packaging convention (exchange/80): <languageTag>-<freeDescription>, lowercase [a-z0-9._-]
    only -- URL-safe (packs travel as zip links; '#' truncates in browsers), shell-safe, and immune
    to the Windows/Linux case-sensitivity mismatch (two packs on Linux, a collision on Windows)."""
    import re as _re3
    global PACK_NAME_RX
    if PACK_NAME_RX is None:
        PACK_NAME_RX = _re3.compile(r"^[a-z]{2}(-[a-z]{2})?-[a-z0-9._-]+$|^[a-z]{2}(-[a-z]{2})?$")
    if not PACK_NAME_RX.match(lang):
        raise SystemExit(f"pack name {lang!r} breaks the convention: <languageTag>-<description>, "
                         f"lowercase [a-z0-9._-] only (e.g. en-fix, fr-fantrad, pt-br-fantrad)")


def count_untranslated(work):
    """Entries with English to translate but an empty translation. A Latin pack renders these as the
    original English (a partial translation is still playable); a non-Latin pack renders EVERY one as
    nonsense, because the charmap reassigns the letter codes -- so in script mode completeness is a
    correctness requirement, not a nicety. Filler/dead records have empty `en` and are not counted."""
    n = 0
    tables = json.load(open(os.path.join(work, "strings", "tables.json")))["tables"]
    for t in tables.values():
        for e in t["entries"]:
            if (e.get("en") or "").strip() and not (e.get("text") or "").strip():
                n += 1
    for fn in ("literals.json", "tactical.json"):
        p = os.path.join(work, "strings", fn)
        if os.path.exists(p):
            for e in json.load(open(p))["entries"]:
                if (e.get("en") or "").strip() and not (e.get("text") or "").strip():
                    n += 1
    dd = os.path.join(work, "strings", "dialogue")
    if os.path.isdir(dd):
        for fn in os.listdir(dd):
            for e in json.load(open(os.path.join(dd, fn)))["entries"]:
                en, tx = e.get("en") or [], e.get("text") or []
                for i, l in enumerate(en):
                    if l.strip() and not (i < len(tx) and (tx[i] or "").strip()):
                        n += 1
    return n


def build(disc, work, outdir, lang, meta=None, packart=None, allow_incomplete=False):
    meta = meta or {}
    check_pack_name(lang)
    exe = read_exe(disc)
    tables = json.load(open(os.path.join(work, "strings", "tables.json")))["tables"]
    errors, sections, stats = [], [], []
    used_cps = set()

    # Every character retail text actually draws, from every source -- a pack code must never be a
    # character retail uses, or retail text would sprout pack glyphs. Literals are included too: the
    # hardcoded menu strings go through the same map.
    retail_chars = set()
    for t in tables.values():
        for e in t["entries"]:
            retail_chars.update(e.get("en") or "")
    dlg_dir_scan = os.path.join(work, "strings", "dialogue")
    if os.path.isdir(dlg_dir_scan):
        for fn in os.listdir(dlg_dir_scan):
            for e in json.load(open(os.path.join(dlg_dir_scan, fn)))["entries"]:
                for l in e["en"]:
                    retail_chars.update(l)
    lit_path = os.path.join(work, "strings", "literals.json")
    if os.path.exists(lit_path):
        for e in json.load(open(lit_path))["entries"]:
            retail_chars.update(e.get("en") or "")
    art_small, art_big = ({}, {})
    if packart:
        art_small, art_big = load_packart(packart, errors)
    charmap = CharmapAssign(exe, retail_chars, art=art_small or None)
    krom = KromAssign(art=art_big or None)
    # SCRIPT MODE is implied by pack art: with it, every string is encoded as 1-byte pack codes
    # rather than UTF-8 (see enc_codes for why non-Latin cannot use the UTF-8 path).
    script = bool(art_small)

    # In script mode an untranslated string renders as nonsense (the charmap reassigns the letter
    # codes), so completeness is a correctness requirement -- refuse an incomplete non-Latin pack
    # unless the author is deliberately building a partial one for testing. A Latin pack degrades
    # gracefully, so there it is only worth a note.
    untr = count_untranslated(work)
    if untr:
        if not script:
            print(f"[lang] note: {untr} string(s) untranslated -- they will show the original "
                  f"English (fine for a Latin pack).", file=sys.stderr)
        elif allow_incomplete:
            print(f"[lang] WARNING: {untr} string(s) untranslated -- a non-Latin pack renders each "
                  f"one as NONSENSE. Building anyway (--allow-incomplete).", file=sys.stderr)
        else:
            errors.append(f"{untr} string(s) untranslated -- a non-Latin pack renders every "
                          f"untranslated string as nonsense (the pack reassigns the letter codes). "
                          f"Finish the translation, or pass --allow-incomplete to build a partial "
                          f"pack for testing.")

    for tid, name, vram, kind, count, width, pad in TABLES:
        entries = tables[name]["entries"]
        if kind == "fixed":
            blob, n = build_fixed(exe, vram, count, width, pad, entries, name, errors,
                                  charmap if name in CHARMAP_TABLES else None,
                                  krom if name == "gItemNamesSjis" else None, script=script)
            if n:
                sections.append((K_FIXED, tid, blob))
        else:
            blob, n = build_ptr(exe, vram, count, entries, name, errors, used_cps,
                                charmap if script else None)
            if n:
                sections.append((K_PTR, tid, blob))
        if n:
            stats.append((name, n))


    f, files, sec = _iso(disc)
    dlg_dir = os.path.join(work, "strings", "dialogue")
    nfiles = nlines = 0
    for nm in sorted(files):
        m = TEXT_RX.fullmatch(nm)
        if not m:
            continue
        stem = m.group(1)
        jp = os.path.join(dlg_dir, f"{stem}.json")
        if not os.path.exists(jp):
            continue
        lba, size = files[nm]
        nsec = (size + 2047) // 2048
        raw = sec(lba, nsec)[:size]
        patched, n = build_text(raw, json.load(open(jp)), stem, nsec * 2048, errors,
                                used_cps, charmap if script else None)
        if patched:
            sections.append((K_TEXT, lba, struct.pack("<I", len(patched)) + patched))
            nfiles += 1; nlines += n
    f.close()

    # Code literals (K_LITERAL): entries keyed by content hash ("literal:<fnv1a>"), replacement
    # encoded per the ORIGINAL literal's encoding -- SJIS literals (the TURN banners, the dojo
    # YES/NO) get full-width SJIS + krom codes for accents; everything else is UTF-8 feeding the
    # same font section as the rest of the pack.
    nlits = 0
    recs = []
    lit_path = os.path.join(work, "strings", "literals.json")
    if os.path.exists(lit_path):
        for e in json.load(open(lit_path))["entries"]:
            want = e.get("text") or ""
            if not want:
                continue
            h = int(e["key"].split(":")[1], 16)
            if e.get("encoding") == "sjis":
                out, ok = bytearray(), True
                for c in want:
                    if c == "\n":
                        out.append(0x0A)
                    elif c.isascii():
                        out += enc_sjis(c)
                    else:
                        code = krom.code_for(c, errors, e["key"])
                        if code is None:
                            ok = False; break
                        out += bytes([(code >> 8) & 0xFF, code & 0xFF])
                if not ok:
                    continue
                raw = bytes(out)
            elif script:
                raw = enc_codes(want, charmap, errors, e["key"])
                if raw is None:
                    continue
            else:
                raw = want.encode("utf-8")
                used_cps.update(ord(c) for c in want if ord(c) > 0x7F)
            if len(raw) > 0xFFFF:
                errors.append(f"{e['key']}: replacement too long"); continue
            recs.append((h, raw))
    # The TACTICAL LAYER rides the same K_LITERAL mechanism: pc_balance.c resolves its flavor
    # strings through PC_LangStr at patch-build time, so a record whose hash matches the ENGLISH
    # tactical string swaps in the translation. Encoding follows the TARGET: gSpellNames entries
    # are fixed-width 1-byte-code strings (charmap; <= 20 chars), descriptions are UTF-8.
    tac_path = os.path.join(work, "strings", "tactical.json")
    if os.path.exists(tac_path):
        seen = set()
        for e in json.load(open(tac_path))["entries"]:
            want = e.get("text") or ""
            en = e.get("en") or ""
            if not want or not en:
                continue
            h = fnv1a_str(en)
            if h in seen:
                continue                       # addDescSwap patches two tables with ONE string
            seen.add(h)
            if e["key"].startswith("gSpellNames"):
                out, ok = bytearray(), True
                for c in want:
                    if c.isascii():
                        out.append(ord(c))
                    else:
                        code = charmap.code_for(c, errors, f"tactical {e['key']}")
                        if code is None:
                            ok = False; break
                        out.append(code)
                if not ok:
                    continue
                if len(out) > 20:
                    errors.append(f"tactical {e['key']}: {len(out)} chars, record holds 20")
                    continue
                raw = bytes(out)
            elif script:
                raw = enc_codes(want, charmap, errors, f"tactical {e['key']}")
                if raw is None:
                    continue
            else:
                raw = want.encode("utf-8")
                used_cps.update(ord(c) for c in want if ord(c) > 0x7F)
            recs.append((h, raw))

    if recs:
        recs.sort()
        blob = struct.pack("<I", len(recs))
        for h, raw in recs:
            blob += struct.pack("<QH", h, len(raw)) + raw
        sections.append((K_LITERAL, 0, blob))
        nlits = len(recs)

    # The font section is built LAST so it covers codepoints from every source (ptr tables AND
    # dialogue). Sorted by codepoint: the runtime binary-searches.
    nglyphs = 0
    if used_cps:
        glyphs = synth_glyphs(exe, used_cps, errors)
        if glyphs:
            blob = struct.pack("<I", len(glyphs))
            for cp, rows in glyphs:
                blob += struct.pack("<I", cp) + rows
            sections.append((K_FONT, 0, blob))
            nglyphs = len(glyphs)

    cm = charmap.section()
    if cm:
        sections.append((K_CHARMAP, 0, cm))
    km = krom.section()
    if km:
        sections.append((K_KROM, 0, km))

    if errors:
        print("BUILD FAILED:\n  " + "\n  ".join(errors[:20]), file=sys.stderr)
        if len(errors) > 20:
            print(f"  ... and {len(errors) - 20} more", file=sys.stderr)
        raise SystemExit(1)

    d = os.path.join(outdir, "langpacks", lang)
    os.makedirs(d, exist_ok=True)
    with open(os.path.join(d, "strings.bin"), "wb") as fo:
        fo.write(MAGIC + struct.pack("<I", len(sections)))
        for kind, sid, payload in sections:
            fo.write(struct.pack("<III", kind, sid, len(payload)) + payload)
    # Manifest = the MACHINE TRUTH about the pack (the folder name is only a human convention).
    # The runtime refuses to load without a matching "game" and a readable "format".
    lang_tag = lang.split("-")[0]
    json.dump({"game": "vandal-hearts-usa",
               "format": 1,
               "language": lang_tag,
               "name": meta.get("name") or lang,
               "author": meta.get("author") or "",
               "version": meta.get("version") or "",
               "notes": meta.get("notes") or "",
               "contains": ["strings"],
               "tables_edited": dict(stats), "dialogue_files": nfiles, "dialogue_lines": nlines,
               "font_glyphs": nglyphs},
              open(os.path.join(d, "manifest.json"), "w"), indent=1, ensure_ascii=False)
    return d, stats, nfiles, nlines, len(sections), nglyphs


def print_repertoire():
    """The renderable repertoire, GENERATED from the mark set the synthesisers actually use -- run
    `lang_build.py --repertoire` after any MARKS change and paste into the tooling README, so the
    documentation can never drift from what the builder enforces."""
    assert set(MARKS) == set(KROM_MARKS), "8x9 and 16x15 mark sets diverged -- repertoire is ambiguous"
    lower, upper = [], []
    for cp in range(0x80, 0x2000):
        d = unicodedata.normalize("NFD", chr(cp))
        if len(d) == 2 and ord(d[1]) in MARKS:
            if "a" <= d[0] <= "z":
                lower.append(chr(cp))
            elif "A" <= d[0] <= "Z":
                upper.append(chr(cp))
    print("Marks:", " ".join(unicodedata.name(chr(m)).replace("COMBINING ", "").lower()
                             for m in sorted(MARKS)))
    print(f"\nLowercase -- renders EVERYWHERE ({len(lower)}):")
    print("  " + " ".join(lower))
    print(f"\nUppercase -- item-name path only ({len(upper)}):")
    print("  " + " ".join(upper))
    print("\nNot synthesised from the US font -- supply drawn glyph art with --packart:")
    print("  å ø æ ß ð þ œ ¡ ¿ · -- and every non-Latin script (Cyrillic, Greek: proven in game)")


if __name__ == "__main__":
    if "--repertoire" in sys.argv:
        print_repertoire()
        sys.exit(0)
    if len(sys.argv) < 4:
        raise SystemExit(__doc__)
    lang = "en"
    meta = {}
    if "--lang" in sys.argv:
        lang = sys.argv[sys.argv.index("--lang") + 1]
    for k in ("name", "author", "version", "notes"):
        flag = f"--{k}"
        if flag in sys.argv:
            meta[k] = sys.argv[sys.argv.index(flag) + 1]
    packart = None
    if "--packart" in sys.argv:
        packart = sys.argv[sys.argv.index("--packart") + 1]
    allow_incomplete = "--allow-incomplete" in sys.argv
    d, stats, nf, nl, ns, ng = build(sys.argv[1], sys.argv[2], sys.argv[3], lang, meta, packart,
                                     allow_incomplete)
    print(f"wrote {d}/strings.bin  ({ns} sections)")
    for n, c in stats:
        print(f"  {n:22}{c:>5} entries")
    print(f"  dialogue{'':14}{nf:>5} files, {nl} lines")
    if ng:
        print(f"  font{'':18}{ng:>5} synthesised glyph(s)")
    if not ns:
        print("  (nothing edited -- an empty pack, which the runtime treats as absent)")
