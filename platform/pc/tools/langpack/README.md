# Language-pack tooling

The pipeline that builds optional **language packs** for the PC port. A pack lives in
`langpacks/<name>/` beside the executable and is selected explicitly with `VH_LANG=<name>` in
`vandalhearts.ini` — no pack, or no selection, means the untouched original text. The base build
ships no language content.

Like everything else in this project, the pipeline works from **your own disc image**: game text is
never committed to this repository, and neither are built packs or working sets — the tools
regenerate everything from the disc.

## The pipeline

```
                         your disc image
                               |
  lang_export.py <disc> <work>            executable tables + on-disc dialogue -> <work>/strings/
  lang_export_literals.py <src> <work>    text hardcoded in the game code      -> strings/literals.json
  lang_export_tactical.py <bal> <work>    Tactical Mode text                   -> strings/tactical.json
  lang_group.py <work>                    entity-grouped translator views      -> <work>/translate/
                               |
                    (translate: edit translate/*.json and strings/dialogue/*.json --
                     every entry carries its English source and its display limit)
                               |
  lang_merge.py <work>                    fold translate/ edits back into strings/
                                          (--revert-cleared: a field you emptied reverts to English)
  lang_validate.py <disc> <work>          hard rules + on-screen budget lint
  lang_build.py <disc> <work> <out> --lang <name> [--name --author --version --notes]
                                          [--packart <dir>]        non-Latin script: your glyph sheets
                                          [--allow-incomplete]     script mode: build with gaps (testing)
                               |
                    <out>/langpacks/<name>/{manifest.json, strings.bin}
```

`<disc>` is your own disc image, `<src>` is the repository's `src/` directory and `<bal>` is
`platform/pc/src/pc_balance.c`. Run the four export steps in order into the same `<work>` folder;
together they produce a complete working set (roughly 1,000 strings plus 2,273 dialogue entries).

**Where to put `<work>` and `<out>`.** They hold text extracted from your disc, so they must never
be committed. Use `platform/pc/tools/langpack/work/` and `.../out/` (both already git-ignored), or
any location outside the repository. The `.gitignore` here also ignores any `strings/`,
`translate/` or `langpacks/` folder created under this directory, whatever you name it — but a path
outside the repo is the surest choice.

**Check the export worked** before starting to translate — an untouched working set should
validate clean and build to an *empty* pack:

```
lang_validate.py <disc> <work>          ->  0 error(s), 0 warning(s)
lang_build.py <disc> <work> <out> --lang test   ->  (0 sections)
```

Zero sections is the point, not a failure: a pack only carries what you changed, so "nothing
translated yet" must produce nothing. If either step reports otherwise, the export is incomplete —
fix that before translating, rather than after.

`lang_probe.py` builds a labelled test pack (one marker per text source) for engine verification;
`en_audit.py` scans the exported English for defects provable from the game's own data.

## Pack naming

`<languageTag>-<freeDescription>` — an ISO 639-1 language code (optionally with a region subtag),
then lowercase `a-z 0-9 . _ -` only: `en-fix`, `fr-fantrad`, `pt-br-fantrad`. Enforced by
`lang_build.py`; the constraint keeps pack names URL-safe, shell-safe, and immune to
case-sensitivity differences between platforms.

`manifest.json` is the pack's real identity — the runtime checks its `game` id and `format`
version before loading anything, and displays its `name`/`version`. Everything else in a pack
folder (a README, credits, notes in any language) belongs to the pack's authors; the loader reads
only `manifest.json` and `strings.bin`.

## When a pack applies

**A pack applies at game start. Changing the selection requires a restart** — the in-game
LANGUAGE setting marks a pending change with `*` and takes effect on the next launch. This is
deliberate, not a limitation to be worked around: a pack rewrites structures the game builds once
at boot (name tables, repointed string tables, the glyph sheet in video memory, Tactical Mode's
patch set), and text already loaded for the current scene lives in buffers the game only refills
on a scene change. A live switch would leave the game half in each language; the restart is the
one point where everything is guaranteed coherent.

For pack authors this sets the iteration loop: edit → `lang_build.py` → restart the game. There is
no in-game reload.

## What the engine supports

- **Fixed-width name tables keep their character budgets** — an accented letter costs one
  character, exactly like its plain form. One codepoint is one screen column everywhere.
- **A pack is a diff**: untranslated entries show the original text, so a partial translation is a
  working translation.
- **A pack covers the game's content, not the port's own UI.** The in-game options overlay
  (SELECT+START) stays English in every language. It is drawn by the port with its own small
  font, and keeping it out means every pack behaves the same way regardless of script.

## How the game draws text — and what each sheet you draw will cover

The game uses **two different fonts** and picks one per screen. A script the game has never seen
(Cyrillic, Greek, Polish, Nordic) therefore needs its alphabet drawn **twice**, once at each size.
They are independent files, so you can ship one before the other.

| | **small sheet — 8×9 px** | **large sheet — 16×15 px** |
|---|---|---|
| **where your letters show up** | story dialogue, every menu, item and spell descriptions, spell names, character names, class names, terrain names, battle messages | item names in the shop, field and inventory; the TURN counter; YES/NO prompts |
| **how much of the game** | almost all of it | a handful of screens |
| **letters you can add** | **44** | no practical limit |
| **if you don't supply it** | nothing readable renders — the pack is unusable | those screens stay English; everything else still works |
| **drawing it** | very tight — plan on capitals only | roomier, and easier per letter |

**You don't draw digits, punctuation or spaces.** Those already exist at both sizes and keep
working untouched.

**44 is the real ceiling, and it only applies to the small sheet.** It fits a full Russian
alphabet (33) or Greek (~24) in capitals. It does *not* fit capitals and lowercase of a non-Latin
script — that would need about 66. This is a hard limit of how the game stores its own font, not a
setting. The game already prints most text in capitals in English, so capitals-only is not the
regression it sounds like.

**A practical way to stage the work:** translate everything *except* item names first and ship with
only the small sheet. Untranslated item names keep their original text and render normally, so you
get a playable, almost-complete pack while the large sheet is still being drawn.

## Drawing a non-Latin script

A script the game has never drawn (Cyrillic, Greek, Nordic, Polish) needs its letters drawn as glyph
art and passed to the builder with `--packart <dir>`. Supplying art also switches the whole pack to
**script mode** (1-byte codes throughout — UTF-8 cannot carry these letters through the game's
dialogue path), which is why a script pack must translate **everything**: any untranslated string
would render as nonsense, so the builder refuses an incomplete one (translate it all;
`--allow-incomplete` exists only for mid-work testing).

### The quick way: generate a starting sheet

You do **not** have to draw an alphabet from scratch. `gen_packart.py` rasterises a bitmap font (GNU
Unifont, bundled as `unifont-subset.bdf`) straight into both sheets:

```
gen_packart.py <dir> --script ru          # a preset alphabet (ru = Russian, el = Greek)
gen_packart.py <dir> --script el
gen_packart.py <dir> --cps U+0410-U+042F  # or explicit codepoints
```

It writes `font8x9.*` + `font16x15.*` ready for `--packart <dir>`, plus `proof_8x9.png` /
`proof_16x15.png` — each cell magnified with its codepoint, so you can confirm every letter came out
right. The **16×15 sheet is production quality**; the **8×9 sheet is legible** and a good base — a
team may want to hand-tweak a few of the densest letters, but nobody has to start from a blank grid.
KROMDAT / the PlayStation BIOS is not involved. (Unifont is OFL/GPL; see `NOTICE-unifont.txt`.)

### The format, if you draw or edit sheets by hand

The `<dir>` holds up to two sheets, each a **PNG image + a `.txt` manifest**:

| files | cell size | covers | required? |
|---|---|---|---|
| `font8x9.png` + `font8x9.txt` | 8×9 px | the small font — almost everything | **yes** |
| `font16x15.png` + `font16x15.txt` | 16×15 px | the large font — item names, TURN, YES/NO | optional (those screens stay in the original text without it) |

**The PNG** is a 1-bit image of the letters packed into a grid of cells, filled **left to right, then
top to bottom** (reading order). A cell is exactly the cell size above; the image width fixes how many
cells per row (`width ÷ cell width`). **A black pixel is ink** (part of the letter); white is
background. An **all-white cell means "not supplied"** and is skipped — handy for leaving a gap.

**The `.txt`** lists which codepoint each cell holds, one per line, in the **same reading order** as
the cells, as `U+XXXX` (hex). Lines starting with `#` are comments; blank lines are ignored. So cell 0
(top-left) is the first `U+XXXX` line, cell 1 the next, and so on.

You draw **only the letters your alphabet adds** — 33 for Russian, ~24 for Greek. Digits,
punctuation and spaces already exist and must not be redrawn. Rendering the sheet back out is the
best check: every letter should land in the cell its codepoint predicts.

## Supported characters

**Encoding and repertoire are two different things.** Pack text files are written in **UTF-8** —
that is the encoding, not a capability claim. What can *render* is defined by glyph supply, and there
are two sources:

- **the synthesiser (no art needed):** ASCII, plus any Latin letter that decomposes into a base
  letter (a–z) and one of six marks. Accented glyphs are composed automatically from the game's own
  letterforms — no drawing required inside this repertoire.
- **pack-supplied glyph art (`--packart`):** everything else — Nordic (`å ø æ ð þ`), Polish
  (`ł ż ą ę`), and every non-Latin script (Cyrillic and Greek are drawn and proven in game). You
  draw the letters once; see *Drawing a non-Latin script* below.

A codepoint with neither a synthesised nor a supplied glyph is **refused at build time** — never
silently dropped.

Generated by `lang_build.py --repertoire` (regenerate after any mark change — this table cannot
drift from what the builder enforces):

```
Marks: grave, acute, circumflex, tilde, diaeresis, cedilla

Lowercase -- renders EVERYWHERE (67):
  à á â ã ä ç è é ê ë ì í î ï ñ ò ó ô õ ö ù ú û ü ý ÿ ć ĉ ĝ ģ ĥ ĩ ĵ ķ ĺ ļ ń ņ ŕ ŗ
  ś ŝ ş ţ ũ ŵ ŷ ź ǵ ǹ ȩ ḑ ḧ ḩ ḱ ḿ ṕ ṽ ẁ ẃ ẅ ẍ ẑ ẗ ẽ ỳ ỹ

Uppercase -- item-name path only (66):
  À Á Â Ã Ä Ç È É Ê Ë Ì Í Î Ï Ñ Ò Ó Ô Õ Ö Ù Ú Û Ü Ý Ć Ĉ Ĝ Ģ Ĥ Ĩ Ĵ Ķ Ĺ Ļ Ń Ņ Ŕ Ŗ
  Ś Ŝ Ş Ţ Ũ Ŵ Ŷ Ÿ Ź Ǵ Ǹ Ȩ Ḑ Ḧ Ḩ Ḱ Ḿ Ṕ Ṽ Ẁ Ẃ Ẅ Ẍ Ẑ Ẽ Ỳ Ỹ

Not synthesised from the US font -- supply drawn glyph art with --packart:
  å ø æ ß ð þ œ ¡ ¿ · -- and every non-Latin script (Cyrillic, Greek: proven in game)
```

Uppercase accents outside item names: use the accepted convention of unaccented capitals
(`Etat`, `A bientot`).

### Language coverage, honestly

| status | languages |
|---|---|
| **full, no art** | Italian, Portuguese |
| **with standard substitutions, no art** | French (`œ`→`oe`), German (`ß`→`ss`), Spanish (`¡ ¿` not yet renderable), Catalan (`l·l` middle dot not yet renderable) |
| **needs pack art (`--packart`)** | Swedish / Danish / Norwegian / Icelandic (`å ø æ ð þ`), Polish (`ł ż ą ę`); every non-Latin script — **Cyrillic and Greek work today** once you draw their sheets, proven in game |
| **not possible** | CJK — thousands of characters against ~44 glyph slots, and 8×9 px is too small for a legible kanji |

Validation is two-layered: `lang_build` enforces the hard rules (record widths, encoding safety,
glyph capacity) and refuses to build a broken pack; `lang_validate` additionally lints what fits
on screen, counting rendered glyphs — control codes are free, insertions are measured at their
translated width.
