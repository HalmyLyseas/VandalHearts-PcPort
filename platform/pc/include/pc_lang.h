#ifndef PC_LANG_H
#define PC_LANG_H

/* pc_lang.h -- the optional LANGUAGE PACK subsystem.
 *
 * A pack is user-supplied and lives beside the executable, exactly like the HD pack; the base build
 * ships none and behaves identically without one:
 *
 *   <deploy>/langpacks/<lang>/manifest.json
 *   <deploy>/langpacks/<lang>/strings.bin     built by the lang tooling
 *
 * Selected with VH_LANG=<lang>; VH_LANGPACK=<dir> points straight at a pack folder (dev override).
 *
 * ---------------------------------------------------------------------------------------------
 * FILE MAP -- keep these boundaries. This subsystem is expected to grow (a UTF-8 text path is
 * planned), and the split exists so it does not become another god-file:
 *
 *   pc_lang.c        WHAT THE TEXT SAYS. Pack discovery, the strings.bin format, and applying
 *                    content: fixed tables (memcpy), pointer tables (allocate + repoint), on-disc
 *                    dialogue files (substituted in CdRead, keyed by ISO LBA).
 *   pc_lang_font.c   HOW THE TEXT LOOKS. UTF-8 decoding, the pack's codepoint->bitmap glyph table
 *                    (K_FONT section), and the VRAM upload (a verbatim sibling of DrawFontGlyph).
 *                    Everything the gated text hooks call belongs here, NOT in pc_lang.c.
 *
 * Anything that is neither "what it says" nor "how it looks" probably belongs in neither -- put it
 * where its data already lives.
 * ---------------------------------------------------------------------------------------------
 *
 * ⚠ ONE UNAVOIDABLE DUPLICATION. Gated hooks inside the decompiled `src/` declare the function they
 * call with an inline `extern` inside their own #ifdef -- the house style (see src/ai.c,
 * src/graphics.c, src/main_menu.c), and deliberate: `src/` must not include port headers, so this
 * file is NOT staged into the game-source include path. When a signature here changes, the matching
 * inline declaration in `src/` must change with it. platform/pc/ TUs have NO such excuse: they
 * #include this header (libcd.c, libetc.c, libgpu.c, libkernel.c, pc_balance.c, pc_hdpack.c,
 * pc_overlay.c do), and a new port-side caller must too -- an inline extern there just trades a
 * compile-time signature check for a runtime surprise. Current gated src/ callers:
 *
 *   src/battle_0201b8.c  PC_LangApplyTerrainText()
 *   src/text.c           PC_LangUtf8Glyph(), PC_LangUtf8SeqLen()   (DrawText_Internal + msgbox)
 *   src/text.c           PC_LangApplyCharmap()       (GetGlyphIdxForAsciiChar's hand-off)
 *   7 game files         PC_LangStr()                (via each file's PC_LANGSTR macro block)
 *   src/supplies.c       PC_LangItemNames1Byte(), PC_LangDrawItemName1Byte()  (format-2 item lists)
 *   src/window.c         PC_LangItemNames1Byte(), PC_LangDrawItemName1Byte()  (battle item list)
 *   src/battle_0201b8.c  PC_LangItemNames1Byte(), PC_LangDrawItemName1Byte()  (unit item panel)
 */

#include <stddef.h>

/* Glyph-slot capacity of the small font. MUST equal src/text.c's PERMUTER-widened
 * sFontGlyphBitmaps row count ([156][9]) and lang_build.py's slot pool bound -- raise all
 * together (text.c cannot include this header, so its copy is tracked by comment there). */
#define PC_LANG_GLYPH_SLOTS 156

/* Load the pack (once) and apply everything addressable by symbol: the fixed and pointer text
 * tables. Called from the first VSync, after the data-segment constructors have run. No-op without
 * a pack. */
void PC_LangBoot(void);

/* Called from CdRead (libcd.c) right after a read completes. A translated on-disc text file is
 * substituted here, keyed by the read's LBA: cd.c reads a text file whole in one CdRead from
 * gCdFiles[cdf].startingSector -- the plain ISO9660 LBA -- so the swap is indistinguishable from the
 * disc having held those bytes, and the game parses them with its own unmodified LoadText. */
void PC_LangPatchRead(int lba, int sectors, unsigned char *out);

/* Called once from src/battle_0201b8.c's PC_FEAT hook with the address of its function-static
 * terrainText (the battle terrain info box). That table has no external linkage, so this hand-off is
 * the only way a pack can reach it; its blob is held from load time until this call arrives. */
void PC_LangApplyTerrainText(void *table, int bytes);

/* Called once from src/text.c's PC_FEAT hook in GetGlyphIdxForAsciiChar with its static code->glyph
 * map and the static glyph-bitmap array (neither externally linkable). Applies the pack's K_CHARMAP
 * records: code -> slot assignments, plus pack bitmaps written into free slots (all-zero rows =
 * map-only, for the mixed-case remap onto the existing lowercase art). */
void PC_LangApplyCharmap(unsigned char *map128, unsigned char (*glyphs)[9], int glyphCount);

/* --- pc_lang_font.c ------------------------------------------------------------------------- */

/* Consume a K_FONT section: u32 count, then per glyph u32 codepoint + 9 bitmap rows (8x9 1bpp,
 * MSB left), sorted by codepoint. Called by pc_lang.c while parsing strings.bin. */
void PC_LangFontLoad(const unsigned char *p, unsigned len);

/* The gated emit-site hook (src/text.c, DrawText_Internal). If *pp starts a valid UTF-8 sequence
 * and a pack font is loaded: consume the WHOLE sequence, draw one glyph (blank if the codepoint has
 * none) at the given position, and return 1 -- the caller then advances its column exactly as for
 * one retail character. Returns 0 untouched for plain ASCII / no pack, and the retail path runs. */
int PC_LangUtf8Glyph(unsigned char **pp, int x, int y, int color);

/* Length-only twin, for the message box's hard-clip region (a clipped sequence must still be
 * consumed whole): sequence length 2..4, or 0 when not a pack-drawable sequence start. */
int PC_LangUtf8SeqLen(const unsigned char *p);

/* Langpack F3 movie subtitles (pc_gpu_window.c's cue renderer). SubtitleGlyph resolves a Unicode
 * codepoint to 9 bitmap rows (8x1bpp, MSB left): pack K_FONT first, then the game's live ASCII
 * map+bitmap store captured at the charmap hand-off. NULL = no glyph (renderer draws a tofu box).
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

/* Format-2 packs (exchange/91): item names are 1-byte/16-char. The accessor answers 1 only when a
 * pack's item-name table actually APPLIED (see pc_lang.c) -- the gated item-list draw sites in
 * src/ switch on it. The draw helper hard-truncates to the caller's per-box cap and draws through
 * the small font; one implementation for all three gated sites. */
int PC_LangItemNames1Byte(void);
void PC_LangDrawItemName1Byte(int x, int y, int color, const unsigned char *name, int cap);

/* F2 (exchange/92): the accepted pack's backgrounds/ dir (localized HD backgrounds), or NULL --
 * gated on manifest acceptance, so a refused pack cannot smuggle visuals in. pc_hdpack.c resolves
 * this source before the HD pack. */
const char *PC_LangBgDir(void);


/* Overlay picklist support: enumerate installed packs (same game/format gate as loading, applied
 * quietly) and report the folder selected at boot ("" = none) so a pending change can show its
 * restart marker. A pack applies at BOOT; the overlay persists the choice, it never live-swaps. */
int PC_LangListPacks(char folders[][64], char names[][64], int max);
const char *PC_LangBootFolder(void);

#endif /* PC_LANG_H */
