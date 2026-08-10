/* pc_lang_font.c -- language pack: HOW THE TEXT LOOKS (the UTF-8 side).
 *
 * Counterpart of pc_lang.c per the file map in pc_lang.h. This TU owns UTF-8 decoding, the pack's
 * codepoint->bitmap glyph table, and drawing a glyph the exact way the game does. It exists because
 * of decision D1 (exchange/80): pointer strings carry real UTF-8, and the engine renders any
 * codepoint the pack ships a glyph for -- no index space, no character-count ceiling.
 *
 * WHERE IT HOOKS. src/text.c's DrawText_Internal has exactly one glyph-emitting site; a PC_FEAT-
 * gated branch there offers each byte to PC_LangUtf8Glyph() first. If the byte starts a valid UTF-8
 * sequence (leads 0xC2..0xF4 -- distinguishable from ASCII, and gated so it can never race the SJIS
 * ranges), we consume the WHOLE sequence and draw one glyph; otherwise we return 0 and the retail
 * path runs untouched. One codepoint = one column, so the game's column/wrap arithmetic is shared,
 * not duplicated.
 *
 * DRAWING. DrawGlyphBitmap() is a verbatim sibling of src/text.c's DrawFontGlyph() -- same 1bpp ->
 * 4bpp expansion (including its s16 arithmetic, so pack glyphs pick up the exact retail colour
 * shades), same 2-word x 9-row LoadImage, same y<247 guard. It takes a bitmap instead of a glyph
 * index, which is the one thing the retail interface cannot do.
 *
 * GLYPH SOURCE. A K_FONT section in strings.bin: u32 count, then per glyph u32 codepoint +
 * 9 bitmap rows (8x9, 1bpp, MSB left), sorted by codepoint. The builder synthesises accented Latin
 * from the disc's own letterforms (the US font already contains lowercase a-z at indices 13-38) or
 * takes pack-supplied art; this TU only consumes the table.
 *
 * A missing glyph draws BLANK (and logs once per codepoint): the sequence must still be consumed,
 * or the retail path would render the continuation bytes as garbage letters.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "PsyQ/libgpu.h"
#include "pc_lang.h"
#include "pc_platform.h"   /* PC_Verbose: progress lines are chatter, warnings are not */

typedef struct {
    unsigned cp;
    unsigned char rows[9];
} LangGlyph;

static LangGlyph *s_glyphs;
static int s_glyphN;

void PC_LangFontLoad(const unsigned char *p, unsigned len) {
    unsigned n, i;
    if (len < 4) return;
    n = (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
    /* Divide form: n is pack-supplied, `4 + n * 13u` wraps u32 for large n and would pass -- the
     * parse loop (and the malloc) would then trust a count the section cannot hold. */
    if (n > (len - 4) / 13) {
        fprintf(stderr, "[lang] font section truncated (%u glyphs, %u bytes)\n", n, len);
        return;
    }
    s_glyphs = (LangGlyph *)malloc(n * sizeof(LangGlyph));
    if (!s_glyphs) return;
    for (i = 0; i < n; i++) {
        const unsigned char *r = p + 4 + i * 13;
        s_glyphs[i].cp = (unsigned)r[0] | ((unsigned)r[1] << 8) | ((unsigned)r[2] << 16) |
                         ((unsigned)r[3] << 24);
        memcpy(s_glyphs[i].rows, r + 4, 9);
    }
    s_glyphN = (int)n;
    if (PC_Verbose()) fprintf(stderr, "[lang] font: %d glyph(s) loaded\n", s_glyphN);
}

/* Records are sorted by codepoint (the builder guarantees it). */
static const unsigned char *FindGlyph(unsigned cp) {
    int lo = 0, hi = s_glyphN - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s_glyphs[mid].cp == cp) return s_glyphs[mid].rows;
        if (s_glyphs[mid].cp < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return NULL;
}

/* Strict UTF-8 decode: returns the sequence length (2..4) and the codepoint, or 0 if p does not
 * start a valid multi-byte sequence. Never reads past a NUL: a continuation check fails on it. */
static int Utf8Decode(const unsigned char *p, unsigned *cp) {
    unsigned char c = p[0];
    if (c < 0xC2) return 0;                              /* ASCII, continuation, or overlong lead */
    if (c < 0xE0) {
        if ((p[1] & 0xC0) != 0x80) return 0;
        *cp = ((unsigned)(c & 0x1F) << 6) | (p[1] & 0x3F);
        return 2;
    }
    if (c < 0xF0) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80) return 0;
        *cp = ((unsigned)(c & 0x0F) << 12) | ((unsigned)(p[1] & 0x3F) << 6) | (p[2] & 0x3F);
        return 3;
    }
    if (c < 0xF5) {
        if ((p[1] & 0xC0) != 0x80 || (p[2] & 0xC0) != 0x80 || (p[3] & 0xC0) != 0x80) return 0;
        *cp = ((unsigned)(c & 0x07) << 18) | ((unsigned)(p[1] & 0x3F) << 12) |
              ((unsigned)(p[2] & 0x3F) << 6) | (p[3] & 0x3F);
        return 4;
    }
    return 0;
}

/* Verbatim sibling of src/text.c's DrawFontGlyph(), bitmap-in instead of index-in. The s16
 * arithmetic is retail's own (colour shades ride on its sign behaviour) -- do not "fix" it. */
static void DrawGlyphBitmap(const unsigned char *rows, int x, int y, int color) {
    RECT rect;
    unsigned short buffer[32];
    unsigned short *pOut = buffer;
    int i, j, k;
    short output;
    int colorPlusOne = color + 1;
    unsigned char byte;

    if (y >= 247) return;
    for (i = 0; i < 9; i++) {
        byte = *rows++;
        for (j = 0; j < 2; j++) {
            output = 0;
            for (k = 0; k < 4; k++) {
                output >>= 4;
                if (byte & 0x80) {
                    output += colorPlusOne << 14;
                }
                byte <<= 1;
            }
            *pOut++ = (unsigned short)output;
        }
    }
    rect.x = x;
    rect.y = y;
    rect.w = 8 >> 2;
    rect.h = 9;
    LoadImage(&rect, (unsigned int *)buffer);
    DrawSync(0);
}

/* --- F_WD sheet patch (strip-path names; exchange/80 "Path 1(b)") ---------------------------
 * The strip path (gCharacterNames / gUnitTypeNames / gItemNames) does not rasterize at draw time:
 * DrawGlyphStrip MoveImage-blits pre-uploaded pixels out of the glyph sheet at VRAM (640,256),
 * 32 cells/row, cell index == glyph slot. The charmap increment already routes pack codes to free
 * slots; this stamps the SAME K_CHARMAP bitmaps into those slots' SHEET CELLS whenever the sheet is
 * uploaded (LoadFWD runs per scene, and every upload passes through our LoadImage -- the source
 * buffer is patched there, so VRAM, the hires mirror and the trace stay consistent).
 *
 * Pixel format: 4bpp nibbles, 4 px/halfword, nibble n == pixel n (the same packing DrawFontGlyph
 * emits). The ink value is SAMPLED from the 'A' cell (slot 68) of the very upload being patched --
 * most frequent non-zero nibble -- rather than assumed, so it matches the sheet's own art. */
#define FWD_SHEET_X 640
#define FWD_SHEET_Y 256
#define FWD_CELLS_PER_ROW 32
#define FWD_CELL_W 2              /* halfwords: 8 px at 4bpp */
#define FWD_CELL_H 9

static unsigned SheetInkNibble(const unsigned short *pix, int w, int h) {
    int counts[16] = {0}, r, hw, n, best = 4, bestCt = 0;
    int row0 = (68 / FWD_CELLS_PER_ROW) * FWD_CELL_H, col = (68 % FWD_CELLS_PER_ROW) * FWD_CELL_W;
    if (row0 + FWD_CELL_H > h) return (unsigned)best;   /* 'A' cell not in this upload slice: default
                                                        * ink 4 (DrawFontGlyph's packing), don't read
                                                        * past the buffer -- same guard the stamp uses */
    for (r = 0; r < FWD_CELL_H; r++)
        for (hw = 0; hw < FWD_CELL_W; hw++) {
            unsigned v = pix[(row0 + r) * w + col + hw];
            for (n = 0; n < 4; n++) counts[(v >> (4 * n)) & 0xF]++;
        }
    for (n = 1; n < 16; n++)
        if (counts[n] > bestCt) { bestCt = counts[n]; best = n; }
    return (unsigned)best;
}

void PC_LangPatchFwdUpload(int px, int py, int pw, int ph, unsigned short *pix) {
    unsigned len, n, i, ink;
    const unsigned char *blob;
    static int logged;
    if (px != FWD_SHEET_X || py != FWD_SHEET_Y || pw < FWD_CELLS_PER_ROW * FWD_CELL_W) return;
    blob = PC_LangCharmapBlob(&len);
    if (!blob || len < 4) return;
    n = (unsigned)blob[0] | ((unsigned)blob[1] << 8) | ((unsigned)blob[2] << 16) |
        ((unsigned)blob[3] << 24);
    if (n > (len - 4) / 11) return;          /* divide form: the addition wraps for hostile n */
    ink = SheetInkNibble(pix, pw, ph);
    {
        unsigned stamped = 0;
        for (i = 0; i < n; i++) {
            const unsigned char *rec = blob + 4 + i * 11;
            int slot = rec[1], r, blank = 1;
            int row0 = (slot / FWD_CELLS_PER_ROW) * FWD_CELL_H;
            int col = (slot % FWD_CELLS_PER_ROW) * FWD_CELL_W;
            /* Same reject rule as PC_LangApplyCharmap (pc_lang.c) -- this is the SECOND consumer
             * of the same hostile blob and must not be laxer: slot 1 is the window-background tile
             * in THIS sheet (stamping it tiles every window with a letter -- the witnessed
             * regression), slot 128 is NUL/space and must stay blank. And a blank record means
             * "map only" there, so it must not ERASE the sheet cell here (the target of a map-only
             * remap already holds the right art, e.g. the lowercase a-z cells). */
            if (slot == 1 || slot == 128 || slot >= PC_LANG_GLYPH_SLOTS) continue;
            for (r = 0; r < FWD_CELL_H; r++)
                if (rec[2 + r]) { blank = 0; break; }
            if (blank) continue;
            if (row0 + FWD_CELL_H > ph) continue;          /* outside this upload slice */
            for (r = 0; r < FWD_CELL_H; r++) {
                unsigned char bits = rec[2 + r];
                unsigned short hw0 = 0, hw1 = 0;
                int p2;
                for (p2 = 0; p2 < 4; p2++) {
                    if (bits & (0x80 >> p2))       hw0 |= (unsigned short)(ink << (4 * p2));
                    if (bits & (0x80 >> (p2 + 4))) hw1 |= (unsigned short)(ink << (4 * p2));
                }
                pix[(row0 + r) * pw + col] = hw0;
                pix[(row0 + r) * pw + col + 1] = hw1;
            }
            stamped++;
        }
        if (!logged) {
            logged = 1;
            if (PC_Verbose())
                fprintf(stderr, "[lang] F_WD sheet: %u charmap cell(s) stamped (ink nibble %u)\n",
                        stamped, ink);
        }
    }
}

/* --- Krom extension (SJIS item names; exchange/80 "Path 2") ---------------------------------
 * K_KROM section: u16 pack code (0x8440+, a range the retail krom map never answers) + 30-byte
 * 16x15 bitmap (2 bytes/row, MSB left), 32 bytes/record, sorted by code. libkernel.c's
 * Krom2RawAdd consults PC_LangKromGlyph first, so DrawSjisGlyph renders pack glyphs with its own
 * anti-aliasing -- zero src/ involvement. */
typedef struct { unsigned code; unsigned char rows[30]; } KromGlyph;
static KromGlyph *s_krom;
static int s_kromN;

void PC_LangKromLoad(const unsigned char *p, unsigned len) {
    unsigned n, i;
    if (len < 4) return;
    n = (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
    if (n > (len - 4) / 32) {                /* divide form: the addition wraps for hostile n */
        fprintf(stderr, "[lang] krom section truncated (%u glyphs, %u B)\n", n, len);
        return;
    }
    s_krom = (KromGlyph *)malloc(n * sizeof(KromGlyph));
    if (!s_krom) return;
    for (i = 0; i < n; i++) {
        const unsigned char *r = p + 4 + i * 32;
        s_krom[i].code = (unsigned)r[0] | ((unsigned)r[1] << 8);
        memcpy(s_krom[i].rows, r + 2, 30);
    }
    s_kromN = (int)n;
    if (PC_Verbose()) fprintf(stderr, "[lang] krom: %d wide glyph(s) loaded\n", s_kromN);
}

const void *PC_LangKromGlyph(unsigned sjis) {
    int lo = 0, hi = s_kromN - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s_krom[mid].code == sjis) return s_krom[mid].rows;
        if (s_krom[mid].code < sjis) lo = mid + 1;
        else hi = mid - 1;
    }
    return NULL;
}

/* Langpack F3 movie subtitles: the codepoint-keyed pack glyph table, exposed for the subtitle
 * renderer (pc_lang.c's PC_LangSubtitleGlyph tries this before the game's ASCII store). */
const unsigned char *PC_LangFontGlyph(unsigned cp) {
    return s_glyphN ? FindGlyph(cp) : NULL;
}

/* K_FONT16: codepoint-keyed 16x15 glyphs (u32 cp + 30 bytes/record, sorted) -- the subtitle
 * renderer's PRIMARY font. Same shape as K_FONT one size up; ASCII never appears here (the
 * built-in BIOS charset serves it). */
typedef struct { unsigned cp; unsigned char rows[30]; } LangGlyph16;
static LangGlyph16 *s_glyphs16;
static int s_glyph16N;

void PC_LangFont16Load(const unsigned char *p, unsigned len) {
    unsigned n, i;
    if (len < 4) return;
    n = (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
    if (n > (len - 4) / 34) {
        fprintf(stderr, "[lang] font16 section truncated (%u glyphs, %u bytes)\n", n, len);
        return;
    }
    s_glyphs16 = (LangGlyph16 *)malloc(n * sizeof(LangGlyph16));
    if (!s_glyphs16) return;
    for (i = 0; i < n; i++) {
        const unsigned char *r = p + 4 + i * 34;
        s_glyphs16[i].cp = (unsigned)r[0] | ((unsigned)r[1] << 8) | ((unsigned)r[2] << 16) |
                           ((unsigned)r[3] << 24);
        memcpy(s_glyphs16[i].rows, r + 4, 30);
    }
    s_glyph16N = (int)n;
    if (PC_Verbose()) fprintf(stderr, "[lang] font16: %d glyph(s) loaded\n", s_glyph16N);
}

const unsigned char *PC_LangFont16Glyph(unsigned cp) {
    int lo = 0, hi = s_glyph16N - 1;
    while (lo <= hi) {
        int mid = (lo + hi) / 2;
        if (s_glyphs16[mid].cp == cp) return s_glyphs16[mid].rows;
        if (s_glyphs16[mid].cp < cp) lo = mid + 1;
        else hi = mid - 1;
    }
    return NULL;
}

/* Public strict UTF-8 decode for the subtitle renderer -- same rules as the game text path
 * (ASCII/continuation/overlong leads return 0; the caller treats the byte as one codepoint). */
int PC_LangUtf8Decode(const unsigned char *p, unsigned *cp) {
    return Utf8Decode(p, cp);
}

/* Length-only twin of PC_LangUtf8Glyph, for callers that must CONSUME a sequence they cannot draw
 * (the message box's hard-clip region past maxCharsPerLine): 2..4 if *p starts a valid sequence and
 * a pack font is loaded, else 0. */
int PC_LangUtf8SeqLen(const unsigned char *p) {
    unsigned cp;
    if (!s_glyphN || p[0] < 0xC2) return 0;
    return Utf8Decode(p, &cp);
}

/* One item name in the small font (format-2 packs: gItemNamesSjis holds plain 1-byte ASCII).
 * Hard-truncate to cap chars and never wrap, so a long name clips at its box edge instead of
 * spilling outside or dropping onto a second line. THE one implementation behind the three gated
 * draw sites (supplies.c confirm boxes, window.c battle item list, battle_0201b8.c unit panel) --
 * the per-box caps are the callers' knowledge (feedback-35/36 pixel tuning), the truncate-and-draw
 * behaviour is this function's, so a future tuning pass lands once. */
extern void DrawText(int x, int y, int maxCharsPerLine, int lineSpacing, int color,
                     unsigned char *text);                                        /* src/text.c */
void PC_LangDrawItemName1Byte(int x, int y, int color, const unsigned char *name, int cap) {
    unsigned char buf[17];
    int i;
    if (cap > 16) cap = 16;
    for (i = 0; i < cap && name[i] != '\0'; i++) buf[i] = name[i];
    buf[i] = '\0';
    DrawText(x, y, cap + 1, 0, color, buf);
}

int PC_LangUtf8Glyph(unsigned char **pp, int x, int y, int color) {
    unsigned cp;
    int n;
    const unsigned char *g;
    if (!s_glyphN) return 0;                 /* no pack font -> everything stays retail */
    if (**pp < 0xC2) return 0;               /* fast path: plain ASCII */
    n = Utf8Decode(*pp, &cp);
    if (n == 0) return 0;                    /* not UTF-8: let the retail path have the byte */
    *pp += n;                                /* consume the whole sequence either way */
    g = FindGlyph(cp);
    if (g) {
        DrawGlyphBitmap(g, x, y, color);
    } else {
        static unsigned warned[16];
        static int warnedN;
        int i, seen = 0;
        for (i = 0; i < warnedN; i++)
            if (warned[i] == cp) { seen = 1; break; }
        if (!seen) {
            if (warnedN < 16) warned[warnedN++] = cp;
            fprintf(stderr, "[lang] no glyph for U+%04X -- drawn blank\n", cp);
        }
    }
    return 1;
}
