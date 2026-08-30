#ifndef PC_LANG_H
#define PC_LANG_H

/* pc_lang.h -- the optional language-pack subsystem: <deploy>/langpacks/<lang>/{manifest.json,
 * strings.bin}, selected by VH_LANG (VH_LANGPACK=<dir> is a dev override). Gated src/ hooks redeclare
 * these signatures inline; keep them in sync. See docs/language-packs.md, "Developer reference". */

#include <stddef.h>

/* Glyph-slot capacity of the small font. MUST equal src/core/text.c's PERMUTER-widened
 * sFontGlyphBitmaps row count ([156][9]) and lang_build.py's slot pool bound -- raise all
 * together (core/text.c cannot include this header, so its copy is tracked by comment there). */
#define PC_LANG_GLYPH_SLOTS 156

/* Load the pack (once) and apply everything addressable by symbol: the fixed and pointer text
 * tables. Called from the first VSync, after the data-segment constructors have run. No-op without
 * a pack. */
void PC_LangBoot(void);

/* Called from CdRead (libcd.c) right after a read completes: a translated on-disc text file is
 * substituted keyed by the read's LBA. core/cd.c reads a text file whole in one CdRead from its
 * ISO9660 LBA, so the swap is indistinguishable from the disc having held those bytes. */
void PC_LangPatchRead(int lba, int sectors, unsigned char *out);

/* Called once from src/battle/field.c's PC_FEAT hook with its function-static terrainText (the
 * battle terrain info box): no external linkage, so this hand-off is the only route in. */
void PC_LangApplyTerrainText(void *table, int bytes);

/* Called once from src/core/text.c's PC_FEAT hook in GetGlyphIdxForAsciiChar with its static
 * code->glyph map and glyph-bitmap array. Applies the pack's K_CHARMAP records (code -> slot, plus
 * bitmaps for free slots; all-zero rows = map-only). See docs/language-packs.md, "Developer reference". */
void PC_LangApplyCharmap(unsigned char *map128, unsigned char (*glyphs)[9], int glyphCount);

/* --- pc_lang_font.c ------------------------------------------------------------------------- */

/* Consume a K_FONT section: u32 count, then per glyph u32 codepoint + 9 bitmap rows (8x9 1bpp,
 * MSB left), sorted by codepoint. Called by pc_lang.c while parsing strings.bin. */
void PC_LangFontLoad(const unsigned char *p, unsigned len);

/* The gated emit-site hook (src/core/text.c, DrawText_Internal). If *pp starts a valid UTF-8
 * sequence and a pack font is loaded: consume the whole sequence, draw one glyph (blank if none),
 * return 1 (the caller advances one column). Returns 0 for plain ASCII / no pack: retail path runs. */
int PC_LangUtf8Glyph(unsigned char **pp, int x, int y, int color);

/* Length-only twin, for the message box's hard-clip region (a clipped sequence must still be
 * consumed whole): sequence length 2..4, or 0 when not a pack-drawable sequence start. */
int PC_LangUtf8SeqLen(const unsigned char *p);

/* Movie subtitles (pc_gpu_window.c's cue renderer). SubtitleGlyph: codepoint -> 9 bitmap rows
 * (pack K_FONT, then the game's live ASCII store); NULL = no glyph (renderer draws a tofu box).
 * Utf8Decode is the strict decoder shared with the game text path (0 for ASCII/invalid leads). */
const unsigned char *PC_LangSubtitleGlyph(unsigned cp);
const unsigned char *PC_LangFontGlyph(unsigned cp);
int PC_LangUtf8Decode(const unsigned char *p, unsigned *cp);

/* Wide (16x15) subtitle font -- the renderer's PRIMARY tier (the 8x9 path above is the
 * fallback). Glyph16 returns 30 bytes: 15 rows x u16 big-endian, MSB = leftmost pixel. */
void PC_LangFont16Load(const unsigned char *p, unsigned len);
const unsigned char *PC_LangFont16Glyph(unsigned cp);
const unsigned char *PC_LangSubtitleGlyph16(unsigned cp);

/* Called from libgpu.c's LoadImage on EVERY upload: when the rect is the F_WD glyph sheet at
 * (640,256), the pack's charmap glyphs are stamped into their sheet cells in the SOURCE buffer, so
 * the strip path (DrawGlyphStrip blits) shows them too. No-op for any other rect / no pack. */
void PC_LangPatchFwdUpload(int px, int py, int pw, int ph, unsigned short *pix);

/* K_KROM section (16x15 wide glyphs for the SJIS path): loader (called by pc_lang.c) and the
 * lookup libkernel.c's Krom2RawAdd consults before its retail map. NULL when the code is not a
 * pack code -- the retail behaviour is untouched. */
void PC_LangKromLoad(const unsigned char *p, unsigned len);
const void *PC_LangKromGlyph(unsigned sjis);

/* The raw K_CHARMAP blob (pc_lang.c owns it; the sheet patcher reads it). */
const unsigned char *PC_LangCharmapBlob(unsigned *len);

/* Literal replacement (the PC_LANGSTR macro in game files): content-hash lookup; returns the
 * pack's string or the input literal untouched. */
unsigned char *PC_LangStr(const char *lit);

/* Format-2 packs: item names are 1-byte/16-char. The accessor answers 1 only when the pack's
 * item-name table actually applied; the gated item-list draw sites switch on it. The draw helper
 * hard-truncates to the caller's per-box cap and draws through the small font (one implementation). */
int PC_LangItemNames1Byte(void);
void PC_LangDrawItemName1Byte(int x, int y, int color, const unsigned char *name, int cap);

/* The accepted pack's backgrounds/ dir (localized HD backgrounds), or NULL. Gated on manifest
 * acceptance so a refused pack cannot smuggle visuals in; pc_hdpack.c resolves it before the HD pack. */
const char *PC_LangBgDir(void);


/* Overlay picklist support: enumerate installed packs (the loader's game/format gate, applied
 * quietly) and report the folder selected at boot ("" = none) for the pending-change restart marker.
 * A pack applies at boot; the overlay persists the choice, it never live-swaps. */
int PC_LangListPacks(char folders[][64], char names[][64], int max);
const char *PC_LangBootFolder(void);
/* THE ONE MANIFEST READER (pc_lang_list.c -- compiled in BOTH regions, like PC_LangListPacks):
 * the loader and the overlay picklist share this accept rule. Returns 1 if the pack may load. */
int PC_LangManifestCheck(const char *dir, int *formatOut, char *nameOut, size_t nameN, int quiet);

#endif /* PC_LANG_H */
