/* pc_lang.c -- language pack: what the text says. Pack discovery, parsing strings.bin (a diff: an
 * unedited working set builds to an empty pack) and applying its sections. How the text looks is
 * pc_lang_font.c. Format + contracts: docs/language-packs.md, "Developer reference". */
#include <dirent.h>          /* pack enumeration for the overlay picklist (portable; MinGW has it) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc_lang.h"
#include "pc_platform.h"   /* PC_GetDeployDir; PC_Verbose -- progress lines are chatter, warnings are not */

/* Fixed-size tables the game indexes directly (declared in include/units.h, defined by the
 * data-segment generator from OUR US executable -- that is exactly what we are overriding). */
extern signed char gCharacterNames[35][7];
extern unsigned char gItemNamesSjis[101][17];
extern signed char gSpellNames[72][21];
/* Drawn through StringToGlyphs (a sprite glyph strip), not DrawText. gItemNames is a second
 * item-name table used by the equip/status panel, longer than the SJIS one the shop and field use. */
extern signed char gUnitTypeNames[86][11];
extern signed char gItemNames[139][13];
extern unsigned char gClassAdvancementNames[18][17];
/* Pointer tables reconstructed by platform/pc/src/pc_*.c (see those files' own headers). */
extern unsigned char *gStringTable[101];   /* [100] is the PC sentinel entry (see src/core/text.c) */
extern char *gSpellDescriptions[72];
extern char *gItemDescriptions[101];
extern char *gItemDescriptions2[101];

/* ---------------------------------------------------------------------------------------------
 * Language pack
 * ------------------------------------------------------------------------------------------- */

#define LANG_MAGIC "VHLANG\x01"          /* 7 chars + the format-version byte that follows */
#define K_FIXED 1
#define K_PTR   2
#define K_TEXT  3
#define K_FONT  4
#define K_CHARMAP 5
#define K_KROM  6
#define K_LITERAL 7
#define K_CUES  8   /* movie subtitles -- handed to pc_movie_subs.c */
#define K_FONT16 9  /* codepoint-keyed 16x15 glyphs for the subtitle renderer (pc_lang_font.c) */
#define MAX_TEXT_FILES 200

/* Section ids for tables, matching lang_build.py's TABLES list. */
static const struct { void *fixed; size_t bytes; void **ptr; int count; const char *name; } kTables[] = {
    { gCharacterNames, sizeof gCharacterNames, NULL,                        0,  "gCharacterNames"    },
    { gItemNamesSjis,  sizeof gItemNamesSjis,  NULL,                        0,  "gItemNamesSjis"     },
    { gSpellNames,     sizeof gSpellNames,     NULL,                        0,  "gSpellNames"        },
    { NULL, 0, (void **)gStringTable,       100, "gStringTable"       },
    { NULL, 0, (void **)gSpellDescriptions,  72, "gSpellDescriptions" },
    { NULL, 0, (void **)gItemDescriptions,  101, "gItemDescriptions"  },
    { NULL, 0, (void **)gItemDescriptions2, 101, "gItemDescriptions2" },
    { gUnitTypeNames,         sizeof gUnitTypeNames,         NULL, 0, "gUnitTypeNames"         },
    { gItemNames,             sizeof gItemNames,             NULL, 0, "gItemNames"             },
    { gClassAdvancementNames, sizeof gClassAdvancementNames, NULL, 0, "gClassAdvancementNames" },
    /* terrainText: a function-static in src/battle/field.c, so there is no address to apply at load
     * time. Its blob is held until that file's PC_FEAT hook hands us the table (see below). */
    { NULL, 0, NULL, 0, "terrainText" },
};
#define TID_ITEMNAMES 1
#define TID_TERRAIN 10
#define NTABLES ((int)(sizeof kTables / sizeof kTables[0]))

typedef struct { int lba; unsigned len; unsigned char *bytes; } LangText;

static struct {
    int loaded;                       /* 0 = not tried, 1 = tried (with or without a pack) */
    int active;                       /* a pack was found and applied */
    int mfOk;                         /* manifest passed the game/format gate (set before apply) */
    int item1b;                       /* format 2: item names are 1-byte/16-char. Latched only at
                                       * load success and only if gItemNamesSjis applied, so a
                                       * broken pack falls back to English, never mojibake. */
    int itemNamesApplied;             /* the gItemNamesSjis K_FIXED section landed (see item1b) */
    LangText text[MAX_TEXT_FILES];
    int textN;
    unsigned char *terrain;           /* deferred blob for the function-static terrainText */
    unsigned terrainLen;
    unsigned char *charmap;           /* deferred blob for the static map/bitmaps in core/text.c */
    unsigned charmapLen;
    struct LangLit { unsigned long long hash; char *str; } *lits;   /* K_LITERAL, parsed */
    int litsN;
    int tablesApplied, ptrsApplied, textPatched;
} s_lang;

static void LitLoad(const unsigned char *p, unsigned len);   /* defined below LangLoad */
static void LangLoad(void);

static unsigned RdU32(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

/* Resolve the pack folder: VH_LANGPACK=<dir> wins, else <deploy>/langpacks/$VH_LANG. */
static int LangPackDir(char *out, size_t n) {
    const char *ov = getenv("VH_LANGPACK"), *lang = getenv("VH_LANG");
    char deploy[512];
    if (ov && *ov) { snprintf(out, n, "%s", ov); return 1; }
    if (!lang || !*lang) return 0;
    if (strchr(lang, '/') || strchr(lang, '\\')) {         /* a language name, not a path */
        fprintf(stderr, "[lang] VH_LANG must be a plain name (got \"%s\")\n", lang);
        return 0;
    }
    if (!PC_GetDeployDir(deploy, sizeof deploy)) return 0;
    /* Precision caps make the worst case (400+11+64+NUL = 476) provably fit the callers' 512-byte
     * buffers, which also silences MinGW GCC's -Wformat-truncation (it reasons from the format
     * alone). Sized to the buffers, not below: a deploy path that fits must come through whole. */
    snprintf(out, n, "%.400s/langpacks/%.64s", deploy, lang);
    return 1;
}

/* The accepted pack's backgrounds/ dir (<hash>.webp, the HD-pack convention) or NULL; resolved once
 * into a process-lifetime static. Gated on manifest acceptance (mfOk): a refused pack must not
 * smuggle visuals in. See docs/language-packs.md, "Load timing and the boot latch". */
const char *PC_LangBgDir(void) {
    static char dir[600];
    static int resolved;               /* 0 = not yet, 1 = present, -1 = none */
    if (!resolved) {
        char pack[512]; DIR *d;
        if (!s_lang.loaded) LangLoad();          /* decides mfOk */
        resolved = -1;
        if (s_lang.mfOk && LangPackDir(pack, sizeof pack)) {
            snprintf(dir, sizeof dir, "%s/backgrounds", pack);
            if ((d = opendir(dir)) != NULL) { closedir(d); resolved = 1; }
        }
    }
    return resolved == 1 ? dir : NULL;
}

static void ApplyFixed(int id, const unsigned char *p, unsigned len) {
    if (id == TID_TERRAIN) {                    /* no symbol to write: hold it for the hook */
        s_lang.terrain = (unsigned char *)malloc(len);
        if (!s_lang.terrain) return;
        memcpy(s_lang.terrain, p, len);
        s_lang.terrainLen = len;
        if (PC_Verbose()) fprintf(stderr, "[lang] %-18s %5u B held for the battle hook\n", kTables[id].name, len);
        return;
    }
    /* id is a pack-supplied u32 cast to int: >= 0x80000000 arrives negative and would sail past a
     * one-sided >= NTABLES test into kTables[id], a garbage memcpy destination. Both bounds, always. */
    if (id < 0 || id >= NTABLES || !kTables[id].fixed) { fprintf(stderr, "[lang] bad FIXED id %d\n", id); return; }
    if (len != kTables[id].bytes) {
        fprintf(stderr, "[lang] %s: pack has %u B, the table is %zu B -- skipped (rebuild the pack)\n",
                kTables[id].name, len, kTables[id].bytes);
        return;
    }
    memcpy(kTables[id].fixed, p, len);
    s_lang.tablesApplied++;
    if (id == TID_ITEMNAMES) s_lang.itemNamesApplied = 1;   /* format-2 gate: see item1b */
    if (PC_Verbose()) fprintf(stderr, "[lang] %-18s %5u B replaced\n", kTables[id].name, len);
}

static void ApplyPtr(int id, const unsigned char *p, unsigned len) {
    unsigned n, i, off = 4;
    if (id < 0 || id >= NTABLES || !kTables[id].ptr) { fprintf(stderr, "[lang] bad PTR id %d\n", id); return; }   /* both bounds: see ApplyFixed */
    if (len < 4) return;
    n = RdU32(p);
    for (i = 0; i < n; i++) {
        unsigned idx, sl;
        char *copy;
        if (off + 4 > len) break;
        idx = (unsigned)p[off] | ((unsigned)p[off + 1] << 8);
        sl  = (unsigned)p[off + 2] | ((unsigned)p[off + 3] << 8);
        off += 4;
        if (off + sl > len) break;
        if ((int)idx >= kTables[id].count) { off += sl; continue; }
        copy = (char *)malloc(sl + 1);
        if (!copy) { off += sl; continue; }
        memcpy(copy, p + off, sl); copy[sl] = '\0';
        kTables[id].ptr[idx] = copy;          /* deliberately leaked: lives for the process */
        s_lang.ptrsApplied++;
        off += sl;
    }
    if (PC_Verbose()) fprintf(stderr, "[lang] %-18s %5u entries re-pointed\n", kTables[id].name, n);
}

static void AddText(int lba, const unsigned char *p, unsigned len) {
    unsigned nbytes;
    unsigned char *copy;
    if (len < 4) return;
    nbytes = RdU32(p);
    if (nbytes > len - 4) return;
    if (s_lang.textN >= MAX_TEXT_FILES) {
        static int warned;
        if (!warned) { warned = 1; fprintf(stderr, "[lang] more than %d text files -- raise MAX_TEXT_FILES\n",
                                           MAX_TEXT_FILES); }
        return;
    }
    copy = (unsigned char *)malloc(nbytes);
    if (!copy) return;
    memcpy(copy, p + 4, nbytes);
    s_lang.text[s_lang.textN].lba = lba;
    s_lang.text[s_lang.textN].len = nbytes;
    s_lang.text[s_lang.textN].bytes = copy;
    s_lang.textN++;
}

/* Manifest reading + pack enumeration live in pc_lang_list.c (compiled in both region cores);
 * PC_LangManifestCheck is the one manifest reader the loader and the picklist share. */

/* The folder name selected at boot ("" when no pack loaded) -- the overlay's picklist compares its
 * pending selection against this to show the restart marker. */
static char s_bootFolder[64];

const char *PC_LangBootFolder(void) {
    if (!s_lang.loaded) LangLoad();
    return s_bootFolder;
}

/* The loaded pack encodes item names as 1-byte/16-char (format 2): the gated item-list draw sites
 * then use the small font instead of the wide SJIS one. */
int PC_LangItemNames1Byte(void) {
    if (!s_lang.loaded) LangLoad();
    return s_lang.item1b;
}

/* Load + apply once. Safe to call from anywhere after the data-segment constructors (i.e. after
 * main() starts), and idempotent -- both entry points below call it. */
static void LangLoad(void) {
    char dir[512], path[600];
    FILE *f;
    long size;
    unsigned char *buf;
    unsigned nsec, i, off;
    int fmt = 0;

    if (s_lang.loaded) return;
    s_lang.loaded = 1;
    if (!LangPackDir(dir, sizeof dir)) return;
    if (!PC_LangManifestCheck(dir, &fmt, NULL, 0, 0)) return;
    s_lang.mfOk = 1;                  /* manifest accepted: PC_LangBgDir may activate backgrounds */
    {   /* remember the boot selection BY FOLDER NAME (the overlay's restart marker compares to it);
         * a VH_LANGPACK dev override deliberately stays "" -- it is not a langpacks/ selection */
        const char *lang = getenv("VH_LANG");
        if (lang && *lang && !getenv("VH_LANGPACK"))
            snprintf(s_bootFolder, sizeof s_bootFolder, "%s", lang);
    }
    snprintf(path, sizeof path, "%s/strings.bin", dir);
    f = fopen(path, "rb");
    if (!f) { fprintf(stderr, "[lang] no pack at %s\n", path); return; }
    fseek(f, 0, SEEK_END); size = ftell(f); fseek(f, 0, SEEK_SET);
    if (size < 12) { fclose(f); fprintf(stderr, "[lang] %s is too small to be a pack\n", path); return; }
    /* A pack is a third-party download: every number in it is hostile until checked. The size cap
     * keeps all the u32 offset arithmetic below well clear of wrap range (a full translation
     * measures in hundreds of KB; 128 MB is absurd headroom). */
    if (size > 0x08000000L) {
        fclose(f);
        fprintf(stderr, "[lang] %s is %ld B -- not plausibly a language pack, refused\n", path, size);
        return;
    }
    buf = (unsigned char *)malloc((size_t)size);
    if (!buf) { fclose(f); return; }
    if (fread(buf, 1, (size_t)size, f) != (size_t)size) { free(buf); fclose(f); return; }
    fclose(f);
    if (memcmp(buf, LANG_MAGIC, 7) != 0) {
        fprintf(stderr, "[lang] %s is not a language pack (bad magic)\n", path);
        free(buf); return;
    }
    if (buf[7] != 0) {
        fprintf(stderr, "[lang] pack format v%d, this build reads v0 -- rebuild it with the current "
                        "lang_build.py\n", buf[7]);
        free(buf); return;
    }
    nsec = RdU32(buf + 8);
    if (PC_Verbose()) fprintf(stderr, "[lang] pack %s (%u section(s))\n", dir, nsec);
    off = 12;
    for (i = 0; i < nsec; i++) {
        unsigned kind, id, len;
        /* Subtraction form on both checks: `len` comes straight from the pack, so `off + len` can
         * wrap u32 and pass an addition-form test, then read off the end of buf. Invariant:
         * off <= size at the top of every iteration (off advances by 12 + len only after both). */
        if ((unsigned)size - off < 12) break;
        kind = RdU32(buf + off); id = RdU32(buf + off + 4); len = RdU32(buf + off + 8);
        off += 12;
        if (len > (unsigned)size - off) break;
        if (kind == K_FIXED)      ApplyFixed((int)id, buf + off, len);
        else if (kind == K_PTR)   ApplyPtr((int)id, buf + off, len);
        else if (kind == K_TEXT)  AddText((int)id, buf + off, len);
        else if (kind == K_FONT)  PC_LangFontLoad(buf + off, len);   /* pc_lang_font.c */
        else if (kind == K_KROM)  PC_LangKromLoad(buf + off, len);   /* pc_lang_font.c */
        else if (kind == K_LITERAL) LitLoad(buf + off, len);   /* consumed by PC_LangStr */
        else if (kind == K_CUES) {                   /* movie subtitles (pc_movie_subs.c) */
            extern void PC_MovieSubsLoadPack(const unsigned char *p, unsigned len);
            PC_MovieSubsLoadPack(buf + off, len);
        }
        else if (kind == K_FONT16) PC_LangFont16Load(buf + off, len);   /* pc_lang_font.c */
        else if (kind == K_CHARMAP) {                /* held until core/text.c's hand-off (like terrain) */
            s_lang.charmap = (unsigned char *)malloc(len);
            if (s_lang.charmap) {
                memcpy(s_lang.charmap, buf + off, len);
                s_lang.charmapLen = len;
                if (PC_Verbose())
                    fprintf(stderr, "[lang] charmap: %u record(s) held for the text hook\n",
                            len >= 4 ? RdU32(buf + off) : 0);
            }
        }
        off += len;
    }
    s_lang.active = 1;
    /* Format 2 = 1-byte item names through the small-font path. Latched here, at load success, and
     * only if gItemNamesSjis actually landed: otherwise the table still holds retail 2-byte SJIS and
     * the 1-byte path would draw garbage. See docs/language-packs.md, "Format 2: 1-byte item names". */
    if (fmt >= 2) {
        s_lang.item1b = s_lang.itemNamesApplied;
        if (!s_lang.itemNamesApplied)
            fprintf(stderr, "[lang] format-2 pack has no item-name table -- item names stay retail "
                            "(rebuild the pack)\n");
    }
    if (s_lang.textN && PC_Verbose())
        fprintf(stderr, "[lang] %d dialogue file(s) will be substituted as they load\n", s_lang.textN);
    free(buf);                                  /* sections were copied out where they are kept */
}

/* K_LITERAL section: u32 count, then per record u64 hash (FNV-1a of the C literal's bytes) +
 * u16 len + the replacement bytes. Parsed into (hash, string) pairs at load. */
static void LitLoad(const unsigned char *p, unsigned len) {
    unsigned n, i, off2 = 4;
    if (len < 4) return;
    n = RdU32(p);
    /* Records are >= 10 B each, so a count the section cannot hold is a lie -- and it must be
     * caught HERE: n * sizeof wraps at 32-bit (the M32 A/B build), and even where it doesn't, a
     * multi-GB malloc can "succeed" under overcommit. */
    if (n > (len - 4) / 10) {
        fprintf(stderr, "[lang] literal section truncated (%u records, %u B) -- skipped\n", n, len);
        return;
    }
    s_lang.lits = (struct LangLit *)malloc(n * sizeof(struct LangLit));
    if (!s_lang.lits) return;
    for (i = 0; i < n; i++) {
        unsigned long long h = 0;
        unsigned sl;
        int b;
        char *copy;
        if (off2 + 10 > len) break;
        for (b = 7; b >= 0; b--) h = (h << 8) | p[off2 + b];
        sl = (unsigned)p[off2 + 8] | ((unsigned)p[off2 + 9] << 8);
        off2 += 10;
        if (off2 + sl > len) break;
        copy = (char *)malloc(sl + 1);
        if (!copy) break;
        memcpy(copy, p + off2, sl);
        copy[sl] = '\0';
        s_lang.lits[i].hash = h;
        s_lang.lits[i].str = copy;
        off2 += sl;
        s_lang.litsN = (int)i + 1;
    }
    if (PC_Verbose()) fprintf(stderr, "[lang] %d code literal(s) replaced\n", s_lang.litsN);
}

/* Called via the PC_LANGSTR macro (PC_FEAT builds) wherever game code passes a string literal to a
 * text-draw call. Identity is the literal's content -- an FNV-1a hash matching the exporter's -- so
 * no id table exists to drift. Returns the pack's replacement or the literal itself untouched. */
unsigned char *PC_LangStr(const char *lit) {
    if (!s_lang.loaded) LangLoad();
    if (s_lang.litsN) {
        unsigned long long h = 14695981039346656037ULL;
        const unsigned char *q = (const unsigned char *)lit;
        int i;
        while (*q) h = (h ^ *q++) * 1099511628211ULL;
        for (i = 0; i < s_lang.litsN; i++)
            if (s_lang.lits[i].hash == h) return (unsigned char *)s_lang.lits[i].str;
    }
    return (unsigned char *)lit;
}

/* The raw K_CHARMAP blob, for pc_lang_font.c's F_WD sheet patcher -- the same records that rewrite
 * the map/bitmaps also say which SHEET CELLS need the pack glyph stamped in. */
const unsigned char *PC_LangCharmapBlob(unsigned *len) {
    if (!s_lang.loaded) LangLoad();
    *len = s_lang.charmapLen;
    return s_lang.charmap;
}

/* The charmap hand-off (src/core/text.c's PC_FEAT hook in GetGlyphIdxForAsciiChar) is the only
 * route to the game's live code->glyph map and bitmap store, so the pointers are captured for the
 * subtitle renderer: it then draws from the exact store the game draws from, pack patches included. */
static unsigned char *s_subsMap = NULL;
static unsigned char (*s_subsGlyphs)[9] = NULL;
static int s_subsGlyphN = 0;

/* Applies the pack's K_CHARMAP blob: per 11-byte record (code, slot, rows[9]), map[code] = slot and
 * a non-blank bitmap is written into that slot. All-zero rows mean "map only" (mixed case remaps a-z
 * onto the lowercase art already at slots 13-38). Static map + bitmaps have no external linkage. */
void PC_LangApplyCharmap(unsigned char *map128, unsigned char (*glyphs)[9], int glyphCount) {
    unsigned n, i;
    const unsigned char *p;
    int mapped = 0, drawn = 0;
    s_subsMap = map128; s_subsGlyphs = glyphs; s_subsGlyphN = glyphCount;   /* before any return */
    if (!s_lang.loaded) LangLoad();
    if (!s_lang.charmap || s_lang.charmapLen < 4) return;
    p = s_lang.charmap;
    n = RdU32(p);
    /* Divide form: n is pack-supplied, `4 + n * 11u` wraps u32 for large n and would pass. */
    if (n > (s_lang.charmapLen - 4) / 11) {
        fprintf(stderr, "[lang] charmap section truncated (%u records, %u B)\n", n, s_lang.charmapLen);
        return;
    }
    for (i = 0; i < n; i++) {
        const unsigned char *r = p + 4 + i * 11;
        unsigned code = r[0], slot = r[1];
        int z, blank = 1;
        /* Slots 1 and 128 are inside glyphCount but forbidden: 1 is GLYPH_BG, the window-background
         * tile in the F_WD sheet (stamping it tiles every window with a letter); 128 is where the
         * retail map sends NUL and space. The builder never emits them; a pack is hostile input. */
        if (code >= 128 || (int)slot >= glyphCount || slot == 1 || slot == 128) {
            fprintf(stderr, "[lang] charmap: bad record code=%u slot=%u -- skipped\n", code, slot);
            continue;
        }
        for (z = 0; z < 9; z++)
            if (r[2 + z]) { blank = 0; break; }
        if (!blank) {
            memcpy(glyphs[slot], r + 2, 9);
            drawn++;
        }
        map128[code] = (unsigned char)slot;
        mapped++;
    }
    if (PC_Verbose()) fprintf(stderr, "[lang] charmap: %d code(s) mapped, %d glyph slot(s) written\n", mapped, drawn);
}

/* Movie-subtitle glyph (8x9 tier): pack K_FONT by codepoint first, then the game's own ASCII path
 * through the captured live map+bitmaps. Movies can play before the game's first text draw (the
 * hand-off is lazy), so force it once if needed. NULL = renderer draws a tofu box, never a skip. */
const unsigned char *PC_LangSubtitleGlyph(unsigned cp) {
    const unsigned char *g = PC_LangFontGlyph(cp);
    if (g) return g;
    if (!s_subsMap) {
        extern unsigned char GetGlyphIdxForAsciiChar(unsigned char);
        (void)GetGlyphIdxForAsciiChar(' ');
    }
    if (cp < 128 && s_subsMap && s_subsGlyphs) {
        unsigned slot = s_subsMap[cp];
        /* Slot 0 is how the game's map spells "unassigned" (core/text.c mappings[] defaults to 0),
         * and glyph 0 is real art -- a solid block. Fall through to NULL instead, so the renderer's
         * tofu box stays the one visible signal for "letter not in the font". */
        if (slot != 0 && (int)slot < s_subsGlyphN) return s_subsGlyphs[slot];
    }
    return NULL;
}

/* Wide (16x15) subtitle glyph: the pack's K_FONT16 by codepoint, then the BIOS charset for ASCII
 * via Krom2RawAdd (which consults pack krom glyphs first). No case logic: cue text arrives
 * pre-folded from the builder, and a dev VH_MOVIE_SUBS file must match the pack's alphabet. */
static unsigned AsciiToWideSjis(unsigned cp) {
    if (cp == ' ') return 0x8140;
    if (cp >= '0' && cp <= '9') return 0x824F + (cp - '0');
    if (cp >= 'A' && cp <= 'Z') return 0x8260 + (cp - 'A');
    if (cp >= 'a' && cp <= 'z') return 0x8281 + (cp - 'a');
    switch (cp) {                     /* kuten row 1 -- linear from 0x8140 (see libkernel.c) */
    case ',':  return 0x8143;
    case '.':  return 0x8144;
    case ':':  return 0x8146;
    case ';':  return 0x8147;
    case '?':  return 0x8148;
    case '!':  return 0x8149;
    case '\'': return 0x8166;         /* typographic right single quote */
    case '"':  return 0x8168;         /* typographic right double quote */
    case '(':  return 0x8169;
    case ')':  return 0x816A;
    case '+':  return 0x817B;
    case '-':  return 0x817C;
    case '/':  return 0x815E;
    }
    return 0;
}

const unsigned char *PC_LangSubtitleGlyph16(unsigned cp) {
    const unsigned char *g = PC_LangFont16Glyph(cp);
    if (g) return g;
    if (cp < 0x80) {
        unsigned sjis = AsciiToWideSjis(cp);
        if (sjis) {
            extern void *Krom2RawAdd(int sjisCode);
            void *p = Krom2RawAdd((int)sjis);
            if (p && p != (void *)-1) return (const unsigned char *)p;
        }
    }
    return NULL;
}

/* Called once from src/battle/field.c's Objf030_FieldInfo (PC_FEAT-gated) with its function-static
 * terrainText (the battle terrain info box). No external linkage, so this hand-off is the only way in. */
void PC_LangApplyTerrainText(void *table, int bytes) {
    if (!s_lang.loaded) LangLoad();
    if (!s_lang.terrain || bytes <= 0) return;
    if (s_lang.terrainLen != (unsigned)bytes) {
        fprintf(stderr, "[lang] terrainText: pack has %u B, the table is %d B -- skipped\n",
                s_lang.terrainLen, bytes);
        return;
    }
    memcpy(table, s_lang.terrain, (size_t)bytes);
    s_lang.tablesApplied++;
    if (PC_Verbose()) fprintf(stderr, "[lang] terrainText        %5d B replaced\n", bytes);
}

/* Called from CdRead (platform/pc/src/libcd.c) right after a read completes. `lba` is the read's
 * first sector; text files are read whole in one call, so an exact LBA match is the whole test. */
void PC_LangPatchRead(int lba, int sectors, unsigned char *out) {
    int i;
    if (!s_lang.loaded) LangLoad();
    if (!s_lang.textN || sectors <= 0) return;
    for (i = 0; i < s_lang.textN; i++) {
        if (s_lang.text[i].lba != lba) continue;
        {
            unsigned cap = (unsigned)sectors * 2048u;
            unsigned n = s_lang.text[i].len < cap ? s_lang.text[i].len : cap;
            memcpy(out, s_lang.text[i].bytes, n);
            s_lang.textPatched++;
            if (PC_Verbose()) fprintf(stderr, "[lang] dialogue lba=%d: %u B substituted\n", lba, n);
        }
        return;
    }
}

/* Called once, lazily, from the first VSync -- after the data-segment constructors have run.
 * LangLoad carries its own idempotence latch (s_lang.loaded); no second flag here. */
void PC_LangBoot(void) {
    LangLoad();
}
