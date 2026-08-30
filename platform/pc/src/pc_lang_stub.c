/* pc_lang_stub.c -- the JP core's stand-in for the US-only langpack engine: each entry point the
 * shared backends call returns its "no pack loaded" answer, so a JP core behaves like a US core
 * without a pack. Only linked entry points are stubbed. See docs/language-packs.md, "Runtime layout". */
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

/* PC_LangListPacks is not stubbed: enumeration lives in pc_lang_list.c, compiled in both regions.
 * Loading stays stubbed: "" = none selected at boot (callers print it, so NULL would be wrong). */
const char *PC_LangBootFolder(void) { return ""; }
