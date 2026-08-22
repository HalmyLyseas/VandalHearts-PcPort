/* pc_lang_stub.c -- the JP region's stand-in for the langpack engine (pc_lang.c +
 * pc_lang_font.c), which is US-only by design: it is built on the US ASCII text path
 * (DrawFontGlyph/GetGlyphIdxForAsciiChar), which the JP game does not have, and its
 * dialogue substitution is keyed by US ISO LBAs (exchange/101 §4.3).
 *
 * Shared backends (libcd.c, libkernel.c, libgpu.c, pc_gpu_window.c, pc_movie_subs.c,
 * pc_overlay.c) call these entry points unconditionally; each stub implements the
 * engine's documented "no pack loaded" behaviour (see include/pc_lang.h), so a JP core
 * behaves exactly like a US core with no language pack installed.
 *
 * Only the entry points shared backends actually reference are stubbed (link-verified);
 * the game-source hooks (PC_LangUtf8Glyph et al.) are behind PC_FEAT sites the JP region
 * does not compile. */
#include <stddef.h>
#include "pc_lang.h"

void PC_LangBoot(void) {}

/* Leave the read buffer exactly as the disc supplied it. */
void PC_LangPatchRead(int lba, int sectors, unsigned char *out) {
    (void)lba; (void)sectors; (void)out;
}

/* NULL = no glyph -> the subtitle renderer draws its tofu box (subtitles never load
 * without a pack, so this is unreachable in practice). */
const unsigned char *PC_LangSubtitleGlyph(unsigned cp) { (void)cp; return NULL; }
const unsigned char *PC_LangSubtitleGlyph16(unsigned cp) { (void)cp; return NULL; }

/* 0 = not a pack-drawable sequence (ASCII/invalid lead byte). */
int PC_LangUtf8Decode(const unsigned char *p, unsigned *cp) {
    (void)p; (void)cp;
    return 0;
}

/* Glyph-sheet upload passes through untouched. */
void PC_LangPatchFwdUpload(int px, int py, int pw, int ph, unsigned short *pix) {
    (void)px; (void)py; (void)pw; (void)ph; (void)pix;
}

/* NULL = not a pack code; Krom2RawAdd's retail path runs untouched. */
const void *PC_LangKromGlyph(unsigned sjis) { (void)sjis; return NULL; }

/* Literal replacement: no pack -> the input literal, untouched. */
unsigned char *PC_LangStr(const char *lit) { return (unsigned char *)lit; }

/* No accepted pack -> no localized-backgrounds dir. */
const char *PC_LangBgDir(void) { return NULL; }

/* PC_LangListPacks is NOT stubbed (2026-08-22): enumeration lives in pc_lang_list.c, compiled
 * in both regions, so the overlay can list installed packs on a JP session (queueing one for a
 * pending US-disc restart via the DISC row). Loading stays stubbed: "" = none selected at boot
 * (per the header contract -- callers print it, so NULL would be wrong here). */
const char *PC_LangBootFolder(void) { return ""; }
