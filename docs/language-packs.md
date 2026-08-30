# Language packs

An optional feature: the port can display the game's text in another language, loaded from a
**language pack** beside the executable. The **source tree and base build contain no translated
text**, and the game runs in its original English without a pack. A pack is **built from your own
disc** with the tools under [`platform/pc/tools/langpack/`](../platform/pc/tools/langpack/) — text
is the game's copyrighted expression, so this project ships none (see [NOTICE](../NOTICE)).

A pack translates **the game's own content** — story dialogue, menus, battle messages, item and
spell names and descriptions, character and class names, terrain names, the text baked into the
game's code, and Tactical Mode's text. It does **not** translate the port's own SELECT+START options
overlay, which stays English in every language (see *What a pack does not cover*).

Packs are a **US-disc feature**: they are built on the US game's text engine, so they apply to the
USA and Asia discs only. On the Japanese game the overlay's LANGUAGE row is greyed (the game plays
with its own original Japanese text) — though with a US disc switch pending in the DISC row, you
can already queue a pack there for the restart.

## Installing and selecting a pack

Place the pack in a `langpacks/<name>/` folder beside the executable (the same place the game finds
its disc image and `vandalhearts.ini`):

```
langpacks/
  fr-fantrad/
    manifest.json
    strings.bin
```

`langpacks/` is one level deeper than the HD pack's folder because several languages can be
installed at once and chosen from a list. Select one with the **LANGUAGE** row in the SELECT+START
options overlay (System category), or set `VH_LANG=<name>` in `vandalhearts.ini`. `VH_LANGPACK=<dir>`
points straight at a pack folder as a developer override.

**A pack applies when the game starts — changing the language needs a restart.** The in-game LANGUAGE
setting marks a pending change with `*` and applies it on the next launch. This is deliberate: a pack
rewrites structures the game builds once at boot (name tables, repointed string tables, the glyph
sheet in video memory, Tactical Mode's patch set), and text already loaded for the current scene
lives in buffers the game only refills on a scene change. The restart is the one point where
everything is guaranteed coherent.

`manifest.json` is the pack's identity: the runtime checks its `game` id (`vandal-hearts-usa`) and
`format` version before loading anything, and refuses a pack for a different game or a newer format
loudly, continuing in English rather than silently doing nothing. It also carries the pack's display
name, version, author and notes.

## What a pack covers

Every place the game draws text is reachable: story dialogue, all menus and battle messages, item
and spell names and their descriptions, character and class names, terrain names, the strings
hardcoded in the game's code (menus like the party panel, "Battle results", the after-battle
experience message), the Tactical Mode flavour text, and the save/load slot captions. A pack is a
**diff** — an entry it does not translate keeps the original text, so a partial Latin translation is
still a working translation.

A pack may also opt into **longer item names** — 16 characters instead of the original 8 — which
then render in the smaller font on the shop, depot and battle item screens. It is the translator's
choice, baked into the pack (`format: 2` in its manifest); an older port build refuses such a pack
loudly rather than showing broken text.

The save-slot caption (`Chap. 1 Sct. 1  L5   0:15`) is translated at **display time**: the caption
stored in the save file stays language-neutral English, and its labels are swapped for the pack's
when the load/save menu draws it. So the save file stays portable (it still round-trips to emulators
and console, and reads correctly under any pack or none), and even saves made before a pack was
installed show the translated caption.

## Subtitled story videos

The narration burned into the **story videos** — the six chapter intros and both endings — can be
subtitled *(packs built with the 1.7.1 toolchain or later)*. The port covers the burned-in text and
draws the pack's translation in the game's large font, at the original narration's own on-screen
size — identically with or without the HD pack, at every INTERNAL RES setting. Subtitles are a
**per-line diff** even in a non-Latin pack: a line the pack has not translated keeps its burned-in
English, which stays readable — so a partially subtitled pack is still a working pack. Each cue's
cover rect must lie inside the native 320×240 frame; a cue outside it is rejected rather than shown.

## Localized backgrounds

A pack can also replace backgrounds that have **text baked into the image** — the title card,
signposts, map labels — with translated versions, using the same mechanism as the HD pack's
backgrounds (and a translated background wins over the HD one). Nothing extra is needed on the
player's side: they show at **every INTERNAL RES setting**, with or without the HD pack installed
or enabled. At 1× the art is displayed at native resolution.

## What a pack does not cover

These are deliberate, not omissions:

- **The port's own options overlay (SELECT+START) stays English.** It is drawn by the port with its
  own small Latin-only font; translating it would work for French but never for Russian or Greek, so
  a pack would behave differently depending on the script. Keeping it English holds one rule for
  every language: *packs translate the game, the port's menu stays English* (it is simple English).
- **Six layout labels stay Latin — `HP`, `MP`, `AT`, `DF`, `LEVEL`, and the `L` before a level
  number.** These are not text: the game spells them out as lists of picture numbers baked into the
  window layout, so there is no string for a pack to replace. They are short, universally understood
  abbreviations, already Latin in the Japanese original.
- **The ending's credit roll stays English.** The staff names and their roles ("Producer",
  "Character Design") are universally understood, and replacing people's names adds nothing — the
  shipped subtitle templates deliberately leave the credit slides out.

## Languages

- **Latin scripts need no drawing.** English and any Latin-script language render today; accented
  letters are composed automatically from the game's own letterforms (with the usual substitutions —
  `ß`→`ss`, `œ`→`oe`, and Spanish `¡ ¿` unrendered).
- **Non-Latin scripts need drawn glyph art.** A team supplies two sheets of drawn letters (a small
  one for almost everything, a large one for item names); Cyrillic and Greek are proven in game.
  Nordic and Polish letters that cannot be composed use the same route.
- **CJK is not possible** — thousands of characters against ~44 free glyph slots, and the game's 8×9
  pixel letter cells are too small for a legible kanji.

Text is mostly capitals by default, as the game itself is in English. A pack can opt into **true
mixed case** at build time (the `--mixed-case` builder option), rendering text in the case it was
written — the translator's choice, baked into the pack.

## Making a pack

The toolchain and a full authoring guide live in
[`platform/pc/tools/langpack/README.md`](../platform/pc/tools/langpack/README.md): export the game's
text from your disc into a working set, translate it, validate the on-screen budgets, and build a
pack. Working sets and built packs contain disc-derived text and are never committed.

## Developer reference

The engine side of the feature, for anyone changing the loader, the text hooks or the builder. The
authoring pipeline itself is documented in the tools README.

### Runtime layout

The engine is split by concern, and the boundary is deliberate:

| unit | owns |
|---|---|
| `platform/pc/src/pc_lang.c` | **what the text says** — pack discovery, parsing `strings.bin`, applying content: fixed tables (`memcpy`), pointer tables (allocate + repoint one slot), on-disc dialogue files (substituted in `CdRead`, keyed by ISO LBA), code literals, and the deferred hand-offs for tables with no external linkage. |
| `platform/pc/src/pc_lang_font.c` | **how the text looks** — strict UTF-8 decoding, the codepoint-keyed glyph tables (`K_FONT`, `K_FONT16`, `K_KROM`), the glyph upload (a verbatim sibling of `DrawFontGlyph`), and the glyph-sheet patch. Everything the gated text hooks in `src/` call lives here. |
| `platform/pc/src/pc_lang_list.c` | pack **enumeration** for the overlay picklist and the one manifest reader, `PC_LangManifestCheck`. Compiled in **both** region cores so a JP session can list installed packs and queue one for a pending US-disc restart. |
| `platform/pc/src/pc_lang_stub.c` | the JP core's stand-in for the two engine units: every shared-backend entry point returns its documented "no pack loaded" answer, so a JP core behaves exactly like a US core without a pack. |

The engine is US-only because it is built on the US ASCII text path
(`DrawFontGlyph` / `GetGlyphIdxForAsciiChar`), which the JP game lacks, and its dialogue
substitution is keyed by US ISO LBAs.

**One unavoidable duplication.** Gated hooks inside the decompiled `src/` declare the function they
call with an inline `extern` inside their own `#ifdef` — `src/` must not include port headers, so
`pc_lang.h` is not staged into the game-source include path. When a signature in `pc_lang.h`
changes, the matching inline declaration in `src/` must change with it. Port-side units under
`platform/pc/` have no such excuse and include the header. The gated `src/` callers:

| caller | entry points |
|---|---|
| `src/core/text.c` | `PC_LangUtf8Glyph`, `PC_LangUtf8SeqLen` (`DrawText_Internal` + the message box), `PC_LangApplyCharmap` (`GetGlyphIdxForAsciiChar`'s hand-off) |
| `src/battle/field.c` | `PC_LangApplyTerrainText`, `PC_LangItemNames1Byte`, `PC_LangDrawItemName1Byte` |
| `src/ui/supplies.c`, `src/ui/window.c` | `PC_LangItemNames1Byte`, `PC_LangDrawItemName1Byte` |
| seven game files | `PC_LangStr` via each file's `PC_LANGSTR` macro block |

### Load timing and the boot latch

`LangLoad` runs once (a `loaded` latch), lazily, from whichever entry point is reached first — the
first VSync at the latest — and is safe from any point after the data-segment constructors have
run. Every public accessor calls it, so nothing depends on the order in which backends first ask.
The boot selection is remembered **by folder name** (`VH_LANG`); a `VH_LANGPACK` developer override
deliberately reports `""`, because it is not a `langpacks/` selection the overlay could offer. The
localized-backgrounds directory is gated on **manifest acceptance**, so a refused pack cannot
smuggle visuals in: the refusal contract is "the game continues in English", visuals included.

### The `strings.bin` format

All integers are little-endian. The file starts with the 7-byte magic `VHLANG\x01`, a format-version
byte (`0`), and a `u32` section count; then sections back to back, each `u32 kind`, `u32 id`,
`u32 len`, payload. A pack is a **diff**: an unedited working set builds to zero sections.

| kind | id | payload |
|---|---|---|
| `K_FIXED` (1) | table id | a whole rebuilt table blob; `len` must equal the table's byte size or the section is skipped. The game indexes these records directly, so width and filler are exact. |
| `K_PTR` (2) | table id | `u32 count`, then per entry `u16 index`, `u16 len`, bytes. Only edited slots ship; the loader allocates the string and repoints that slot alone, so a translation is not bounded by the original's length. |
| `K_TEXT` (3) | ISO LBA | `u32 len`, then the whole patched dialogue file. |
| `K_FONT` (4) | — | `u32 count`, then per glyph `u32 codepoint` + 9 rows (8×9, 1 bpp, MSB left), sorted by codepoint. |
| `K_CHARMAP` (5) | — | `u32 count`, then 11-byte records `code, slot, rows[9]`: `map[code] = slot`, and a non-blank bitmap is written into that glyph slot. All-zero rows mean "map only" (mixed case remaps `a`–`z` onto the lowercase art at slots 13–38 without shipping art). |
| `K_KROM` (6) | — | `u32 count`, then 32-byte records `u16 code` (`0x8440`+, a range the retail krom map never answers) + 30 bytes (16×15, 2 bytes/row, MSB left), sorted by code. |
| `K_LITERAL` (7) | — | `u32 count`, then per record `u64` FNV-1a hash of the C literal's bytes, `u16 len`, replacement bytes. |
| `K_CUES` (8) | — | movie subtitle cues, handed whole to `pc_movie_subs.c`. |
| `K_FONT16` (9) | — | like `K_FONT` with 30-byte 16×15 bitmaps: the subtitle renderer's primary font. ASCII never appears here — the built-in BIOS charset serves it. |

Table ids match `lang_build.py`'s `TABLES` list and `pc_lang.c`'s `kTables`: 0 `gCharacterNames`,
1 `gItemNamesSjis`, 2 `gSpellNames`, 3 `gStringTable`, 4 `gSpellDescriptions`,
5 `gItemDescriptions`, 6 `gItemDescriptions2`, 7 `gUnitTypeNames`, 8 `gItemNames`,
9 `gClassAdvancementNames`, 10 `terrainText`.

### Every number in a pack is hostile

A pack is a third-party download, and the loader treats it that way:

- A size cap of 128 MB keeps all `u32` offset arithmetic far from wrap range (a full translation
  measures in hundreds of KB).
- Section bounds use the **subtraction form** (`len > size - off`), never `off + len > size`: a
  pack-supplied `len` can wrap the addition and pass. Invariant: `off <= size` at the top of every
  iteration.
- Record counts use the **divide form** (`n > (len - 4) / recsize`): `4 + n * recsize` wraps for a
  hostile `n`, and `n * sizeof` wraps at 32 bits (the M32 A/B build) — and even a non-wrapping
  multi-GB `malloc` can "succeed" under overcommit.
- Table ids are checked on **both** bounds: a `u32` cast to `int` arrives negative for values
  `>= 0x80000000` and would sail past a one-sided `>= NTABLES` test into a garbage `memcpy`
  destination.
- Charmap records are rejected for `code >= 128`, `slot >= glyphCount`, and slots 1 and 128 (see
  below). The glyph-sheet patch is a second consumer of the same blob and applies the same rule.

### Glyph slots and pack codes

The small font is `sFontGlyphBitmaps[156][9]` in the PC build of `src/core/text.c` (retail:
`[128][9]`). `PC_LANG_GLYPH_SLOTS` (156) in `pc_lang.h`, that array's row count, `DrawFontGlyph`'s
index guard and `lang_build.py`'s slot pool are the same number and move together — otherwise pack
glyphs simply do not draw. The **44 free slots are 111–127 and 129–155**. Two slots inside that
span are forbidden and invisible until something renders:

- **slot 1** is blank in the bitmap array but is `GLYPH_BG` in the `F_WD` glyph sheet — the window
  background tile. Stamping it tiles every window in the game with a letter.
- **slot 128** is where the retail map sends NUL and space; it must stay blank.

A **pack code** is a 1-byte value in a fixed-width record (byte = char = column). The code pool is
printable ASCII whose retail map entry is 0 (renders blank), minus `#`/`$` (parser markup) and minus
any character that retail text actually uses — a code retail draws would render as the pack glyph
wherever retail draws it. That leaves ~17 bytes. **Script mode** (a `--packart` pack that replaces
every string) adds `A`–`Z` to the pool, which is the only reason a non-Latin alphabet fits; it is
why a script pack must be complete, and why any Latin character left in its text is a build error.
A punctuation byte the pack's sheet supplies art for (Greek's `;`) is installed at its own byte and
withdrawn from the pool.

### The glyph-sheet patch

The strip path (`gCharacterNames`, `gUnitTypeNames`, `gItemNames`) does not rasterize at draw
time: `DrawGlyphStrip` blits pre-uploaded pixels out of the `F_WD` glyph sheet at VRAM (640,256),
32 cells per row, cell index == glyph slot. `PC_LangPatchFwdUpload` runs on every `LoadImage` and,
when the rect is that sheet, stamps the same `K_CHARMAP` bitmaps into their cells in the **source
buffer** — so VRAM, the hi-res mirror and the GPU trace stay consistent. Pixels are 4 bpp nibbles,
4 per halfword, nibble n == pixel n (the packing `DrawFontGlyph` emits). The ink value is sampled
from the `A` cell (slot 68) of the very upload being patched — the most frequent non-zero nibble —
rather than assumed, so it matches the sheet's own art; when that cell is outside the upload slice,
ink 4 is used. A map-only (blank) record never erases a sheet cell: the target of a map-only remap
already holds the right art.

### The krom extension

`gItemNamesSjis` draws through `DrawSjisGlyph` → `Krom2RawAdd`, which on PC is the port's own map
and table (`libkernel.c` / `pc_kanji_font.c`). `Krom2RawAdd` consults `PC_LangKromGlyph` before its
retail map, so pack glyphs render with the game's own anti-aliasing and no `src/` involvement. At 15
rows there is room to shift an uppercase letterform down one row to free the top for a mark, so
this path supports true uppercase accents, unlike 8×9.

`pc_kanji_font.c` is generated per region by `gen_kanji_font.py` from the PsyQ SDK's `KROMDAT.BIN`,
whose layout is byte-identical to the BIOS ROM kanji charset it stands in for. The US build draws
only alphanumerics and punctuation through `Krom2RawAdd`, so it embeds an audited 209-glyph subset;
the JP build draws its entire text repertoire through the BIOS font, so it embeds the whole ROM
region `KROMDAT` carries (charset 2 + charset 3, 3489 glyphs, contiguous as in the BIOS). A SHA-256
of each region's charset guards against an arbitrary or wrong-region input file producing
plausible-sized but corrupt glyph data.

### Format 2: 1-byte item names

A format-2 pack re-encodes `gItemNamesSjis` as 1-byte/16-char names and the item-list draw sites
route through the small font. `PC_LangItemNames1Byte` answers 1 **only** when the pack loaded
successfully **and** its `gItemNamesSjis` section actually applied. Any earlier bail-out, or a
format-2 pack missing the table, leaves the retail 2-byte SJIS in the table, and routing that
through the 1-byte path renders every item screen as garbage glyphs — the fallback must be
English, never mojibake. For the same reason the builder always ships the whole re-encoded table
with a format-2 pack, even with zero translated names. `PC_LangDrawItemName1Byte` is the one
implementation behind the three gated draw sites: it hard-truncates to the caller's per-box cap
and never wraps, so a long name clips at its box edge.

The layout constants (`src/ui/supplies.c`) — row height 17px, 20 columns, an 11-char "tight" name
width, and a gold column shifted to x=233 with width 80 — earn their specific values: the column
count is sized to fit the price inside the *narrower* Sell/Depot list boxes, not just the widened
Buy box; the tight name width accounts for the confirm box also carrying an item icon; the gold
column sits one cell left of the SJIS layout's own x=233 so the box's right edge stays put after
widening. Row composition is fixed-width — a prefix, then a 16-character name, then a
right-aligned price — because a scroll redraw blanks the previous row by overwriting with spaces,
so a shorter new name must not leave a leftover tail from the old one. The prefix column also
restores the retail party-list markers (`ListPartyMemberInventory`'s SJIS `#` on equipped rows,
a space on carried rows) that the 1-byte path would otherwise drop.

### Composed strings avoid fixed-offset embedding

A couple of on-screen lines splice a number into translated text — the battle-options turn counter
("TURN  ") and the level-up dialog's "You got " experience line. Retail embeds the number at a fixed
byte offset inside a static buffer, which works only because the English prefix has a fixed width; a
translated prefix of any other length would misplace the digits. Both call sites instead compose the
line in a local buffer, so a script-neutral (full-width) digit run always follows the prefix at
whatever length it ends up — with no pack loaded the layout is byte-for-byte identical to retail. The
turn-counter and "You got " prefixes are exported through `lang_export_literals.py`'s
`PC_LANGSTR`-wrapped-literal scan, since they are not draw-call arguments; the level-up dialog's second
line is exported through the ordinary draw-call sweep instead.

### Movie subtitle glyph resolution

The subtitle renderer in `pc_gpu_window.c` resolves glyphs in tiers. **Primary** is the wide
16×15 font: the pack's `K_FONT16` by codepoint, then the BIOS charset for ASCII through
`Krom2RawAdd` (which itself consults pack krom glyphs first). **Fallback** is 8×9: the pack's
`K_FONT` by codepoint, then the game's live ASCII map + bitmap store, captured at the charmap
hand-off — so subtitles draw from the exact store the game draws from, pack patches included.
Movies can play before the game's first text draw, so the hand-off is forced once if needed. Slot 0
is how the game's map spells "unassigned" (glyph 0 is a solid block), so it is never returned; a
`NULL` result draws a visible tofu box rather than skipping silently. No case logic lives in the
runtime: cue text arrives pre-folded from the builder.

The builder mirrors the runtime's hard limits, because each has a runtime twin that silently
degrades rather than erroring: cue lines and text bytes (`pc_movie_subs.h` `PC_SUBS_MAX_LINES` /
`_TEXT`), movies and cues per movie (`pc_movie_subs.c` `PACK_MAX_MOVIES` and the 512-cue loader
cap — an oversize set is dropped as malformed), decoded codepoints per cue
(`pc_gpu_window.c` `SUBS_MAX_CPS`, past which text is not drawn), and at most 2 active cues per
frame (band + card; a third overlap is dropped).
