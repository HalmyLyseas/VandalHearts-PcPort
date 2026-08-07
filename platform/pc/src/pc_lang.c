/* pc_lang.c -- language pack: WHAT THE TEXT SAYS.
 *
 * Pack discovery, the strings.bin format, and applying content. How the text LOOKS is a separate
 * translation unit -- see the file map in pc_lang.h, and keep the boundary.
 *
 * A PACK IS A DIFF. It carries only what the translator changed, so an unedited working set compiles
 * to an empty pack and loading it is provably a no-op. Three section kinds:
 *
 *   FIXED  a whole rebuilt table blob (gCharacterNames / gItemNamesSjis / gSpellNames) -> memcpy;
 *          the game indexes these records directly, so they must keep their exact width and filler.
 *   PTR    only the edited slots of a pointer table -> we allocate the new string and repoint that
 *          slot alone. A translation is therefore NOT bounded by the original string's length.
 *   TEXT   a whole patched on-disc dialogue file, keyed by its ISO LBA.
 *
 * WHY THE DIALOGUE PATH NEEDS NO src/ EDIT. LoadText -> LoadCdFile -> CdRead, and for a text file
 * that is ONE contiguous read of the file's own LBA (cd.c:985 reads gCdFiles[cdf].sectorCt sectors
 * from gCdFiles[cdf].startingSector, which is the plain ISO9660 LBA). So substituting the buffer
 * right after the read, keyed by LBA, is indistinguishable from the disc having held our bytes --
 * the game parses them with its own unmodified LoadText.
 *
 * NOT COVERED YET: the 8x9 glyph bitmaps and the ASCII->glyph map are private to src/text.c and need
 * gated hooks. Until then a pack is limited to the glyphs the US font already has.
 */
#include <dirent.h>          /* pack enumeration for the overlay picklist (portable; MinGW has it) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc_lang.h"

extern int PC_GetDeployDir(char *out, size_t outSize);   /* pc_bootstrap.c (exe dir, or AppImage dir) */

/* Fixed-size tables the game indexes directly (declared in include/units.h, defined by the
 * data-segment generator from OUR US executable -- that is exactly what we are overriding). */
extern signed char gCharacterNames[35][7];
extern unsigned char gItemNamesSjis[101][17];
extern signed char gSpellNames[72][21];
/* Drawn through StringToGlyphs (a sprite glyph strip), not DrawText -- which is why the first
 * coverage pass missed them; the step-1 probe run found them. gItemNames is a SECOND item-name
 * table used by the equip/status panel, longer than the Shift-JIS one the shop and field use. */
extern signed char gUnitTypeNames[86][11];
extern signed char gItemNames[139][13];
extern unsigned char gClassAdvancementNames[18][17];
/* Pointer tables reconstructed by platform/pc/src/pc_*.c (see those files' own headers). */
extern unsigned char *gStringTable[100];
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
    /* terrainText: a function-static in src/battle_0201b8.c, so there is no address to apply at load
     * time. Its blob is held until that file's PC_FEAT hook hands us the table (see below). */
    { NULL, 0, NULL, 0, "terrainText" },
};
#define TID_TERRAIN 10
#define NTABLES ((int)(sizeof kTables / sizeof kTables[0]))

typedef struct { int lba; unsigned len; unsigned char *bytes; } LangText;

static struct {
    int loaded;                       /* 0 = not tried, 1 = tried (with or without a pack) */
    int active;                       /* a pack was found and applied */
    LangText text[MAX_TEXT_FILES];
    int textN;
    unsigned char *terrain;           /* deferred blob for the function-static terrainText */
    unsigned terrainLen;
    unsigned char *charmap;           /* deferred blob for the static map/bitmaps in text.c */
    unsigned charmapLen;
    struct LangLit { unsigned long long hash; char *str; char *osd; } *lits;   /* K_LITERAL, parsed;
                                         osd = lazily cached caps-folded form (PC_LangOsdStr) */
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
    snprintf(out, n, "%s/langpacks/%s", deploy, lang);
    return 1;
}

static void ApplyFixed(int id, const unsigned char *p, unsigned len) {
    if (id == TID_TERRAIN) {                    /* no symbol to write: hold it for the hook */
        s_lang.terrain = (unsigned char *)malloc(len);
        if (!s_lang.terrain) return;
        memcpy(s_lang.terrain, p, len);
        s_lang.terrainLen = len;
        fprintf(stderr, "[lang] %-18s %5u B held for the battle hook\n", kTables[id].name, len);
        return;
    }
    if (id >= NTABLES || !kTables[id].fixed) { fprintf(stderr, "[lang] bad FIXED id %d\n", id); return; }
    if (len != kTables[id].bytes) {
        fprintf(stderr, "[lang] %s: pack has %u B, the table is %zu B -- skipped (rebuild the pack)\n",
                kTables[id].name, len, kTables[id].bytes);
        return;
    }
    memcpy(kTables[id].fixed, p, len);
    s_lang.tablesApplied++;
    fprintf(stderr, "[lang] %-18s %5u B replaced\n", kTables[id].name, len);
}

static void ApplyPtr(int id, const unsigned char *p, unsigned len) {
    unsigned n, i, off = 4;
    if (id >= NTABLES || !kTables[id].ptr) { fprintf(stderr, "[lang] bad PTR id %d\n", id); return; }
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
    fprintf(stderr, "[lang] %-18s %5u entries re-pointed\n", kTables[id].name, n);
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

/* Minimal manifest reads (same posture as pc_hdpack.c's HdManifestRead: the manifest is a
 * CONTROLLED file our own builder writes -- this is a field extractor, not a JSON parser). */
static int MiniJsonStr(const char *buf, const char *key, char *out, size_t n) {
    char pat[64];
    const char *p, *e;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(buf, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++;
    e = strchr(p, '"');
    if (!e || (size_t)(e - p) >= n) return 0;
    memcpy(out, p, (size_t)(e - p));
    out[e - p] = '\0';
    return 1;
}

static int MiniJsonInt(const char *buf, const char *key, int *out) {
    char pat[64];
    const char *p;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(buf, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    *out = atoi(p + 1);
    return 1;
}

#define LANG_GAME_ID "vandal-hearts-usa"
#define LANG_FORMAT  1

/* The manifest is LOAD-BEARING (packaging decision, 2026-08-07): the folder name is a human
 * convention, the manifest is the machine truth. A pack with a missing/foreign/newer manifest is
 * refused LOUDLY and the game continues in English -- a renamed folder must never smuggle a pack
 * past identification. Returns 1 if the pack may load. */
static int LangManifestCheck(const char *dir) {
    char path[600], buf[2048], game[64], name[96], version[32];
    FILE *f;
    size_t n;
    int format = 0;
    snprintf(path, sizeof path, "%s/manifest.json", dir);
    f = fopen(path, "r");
    if (!f) {
        fprintf(stderr, "[lang] %s: no manifest.json -- not a language pack (or built by a "
                        "pre-manifest tool; rebuild it)\n", dir);
        return 0;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = '\0';
    fclose(f);
    if (!MiniJsonStr(buf, "game", game, sizeof game) || strcmp(game, LANG_GAME_ID) != 0) {
        fprintf(stderr, "[lang] %s: pack is for game \"%s\", this build is \"%s\" -- refused\n",
                dir, MiniJsonStr(buf, "game", game, sizeof game) ? game : "?", LANG_GAME_ID);
        return 0;
    }
    if (!MiniJsonInt(buf, "format", &format) || format > LANG_FORMAT) {
        fprintf(stderr, "[lang] %s: pack format v%d, this build reads v%d -- refused (update the "
                        "port, or rebuild the pack)\n", dir, format, LANG_FORMAT);
        return 0;
    }
    if (!MiniJsonStr(buf, "name", name, sizeof name)) snprintf(name, sizeof name, "(unnamed)");
    if (!MiniJsonStr(buf, "version", version, sizeof version)) version[0] = '\0';
    fprintf(stderr, "[lang] pack \"%s\"%s%s\n", name, version[0] ? " v" : "", version);
    return 1;
}

/* The folder name selected at boot ("" when no pack loaded) -- the overlay's picklist compares its
 * pending selection against this to show the restart marker. */
static char s_bootFolder[64];

const char *PC_LangBootFolder(void) {
    if (!s_lang.loaded) LangLoad();
    return s_bootFolder;
}

/* Enumerate installed packs for the overlay picklist: every <deploy>/langpacks/<folder> whose
 * manifest passes the same game/format gate the loader applies (quietly -- listing is not loading).
 * Returns the count; folders and display names are parallel arrays. */
int PC_LangListPacks(char folders[][64], char names[][64], int max) {
    char deploy[512], root[560], mpath[700], buf[2048], game[64];
    DIR *d;
    struct dirent *e;
    int n = 0;
    if (!PC_GetDeployDir(deploy, sizeof deploy)) return 0;
    snprintf(root, sizeof root, "%s/langpacks", deploy);
    d = opendir(root);
    if (!d) return 0;
    while ((e = readdir(d)) != NULL && n < max) {
        FILE *f;
        size_t r;
        int format = 0;
        if (e->d_name[0] == '.') continue;
        /* a folder name too long for the picklist buffers can't be a valid pack -- skip it
         * (also proves to -Wformat-truncation that truncation is handled, not ignored) */
        if (strlen(e->d_name) >= 64) continue;
        if (snprintf(mpath, sizeof mpath, "%s/%s/manifest.json", root, e->d_name)
            >= (int)sizeof mpath) continue;
        f = fopen(mpath, "r");
        if (!f) continue;
        r = fread(buf, 1, sizeof buf - 1, f);
        buf[r] = '\0';
        fclose(f);
        if (!MiniJsonStr(buf, "game", game, sizeof game) || strcmp(game, LANG_GAME_ID) != 0)
            continue;
        if (!MiniJsonInt(buf, "format", &format) || format > LANG_FORMAT)
            continue;
        memcpy(folders[n], e->d_name, strlen(e->d_name) + 1);   /* length-checked above */
        if (!MiniJsonStr(buf, "name", names[n], 64))
            memcpy(names[n], e->d_name, strlen(e->d_name) + 1);
        n++;
    }
    closedir(d);
    return n;
}

/* Load + apply once. Safe to call from anywhere after the data-segment constructors (i.e. after
 * main() starts), and idempotent -- both entry points below call it. */
static void LangLoad(void) {
    char dir[512], path[600];
    FILE *f;
    long size;
    unsigned char *buf;
    unsigned nsec, i, off;

    if (s_lang.loaded) return;
    s_lang.loaded = 1;
    if (!LangPackDir(dir, sizeof dir)) return;
    if (!LangManifestCheck(dir)) return;
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
    fprintf(stderr, "[lang] pack %s (%u section(s))\n", dir, nsec);
    off = 12;
    for (i = 0; i < nsec; i++) {
        unsigned kind, id, len;
        if (off + 12 > (unsigned)size) break;
        kind = RdU32(buf + off); id = RdU32(buf + off + 4); len = RdU32(buf + off + 8);
        off += 12;
        if (off + len > (unsigned)size) break;
        if (kind == K_FIXED)      ApplyFixed((int)id, buf + off, len);
        else if (kind == K_PTR)   ApplyPtr((int)id, buf + off, len);
        else if (kind == K_TEXT)  AddText((int)id, buf + off, len);
        else if (kind == K_FONT)  PC_LangFontLoad(buf + off, len);   /* pc_lang_font.c */
        else if (kind == K_KROM)  PC_LangKromLoad(buf + off, len);   /* pc_lang_font.c */
        else if (kind == K_LITERAL) LitLoad(buf + off, len);   /* consumed by PC_LangStr */
        else if (kind == K_CHARMAP) {                /* held until text.c's hand-off (like terrain) */
            s_lang.charmap = (unsigned char *)malloc(len);
            if (s_lang.charmap) {
                memcpy(s_lang.charmap, buf + off, len);
                s_lang.charmapLen = len;
                fprintf(stderr, "[lang] charmap: %u record(s) held for the text hook\n",
                        len >= 4 ? RdU32(buf + off) : 0);
            }
        }
        off += len;
    }
    s_lang.active = 1;
    if (s_lang.textN)
        fprintf(stderr, "[lang] %d dialogue file(s) will be substituted as they load\n", s_lang.textN);
    free(buf);                                  /* sections were copied out where they are kept */
}

/* K_LITERAL section: u32 count, then per record u64 hash (FNV-1a of the C literal's bytes) +
 * u16 len + the replacement bytes. Parsed into (hash, string) pairs at load. */
static void LitLoad(const unsigned char *p, unsigned len) {
    unsigned n, i, off2 = 4;
    if (len < 4) return;
    n = RdU32(p);
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
        s_lang.lits[i].osd = NULL;
        off2 += sl;
        s_lang.litsN = (int)i + 1;
    }
    fprintf(stderr, "[lang] %d code literal(s) replaced\n", s_lang.litsN);
}

/* Called (via the PC_LANGSTR macro, PC_FEAT builds only) wherever game code passes a string
 * literal straight to a text-draw call. Identity is the literal's own CONTENT -- an FNV-1a hash,
 * matching the exporter's -- so no id table exists to drift. Returns the pack's replacement, or
 * the literal itself untouched (no pack / no entry): retail behaviour is the fallthrough. */
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

/* The overlay/OSD twin of PC_LangStr: the port's own UI renders with the 5x7 caps-only bitmap
 * font, so a hit is returned CAPS-FOLDED to OSD-safe ASCII (PC_LangOsdFold), cached per literal.
 * A miss returns the input untouched -- the English strings are already OSD-safe by authorship. */
const char *PC_LangOsdStr(const char *lit) {
    if (!s_lang.loaded) LangLoad();
    if (s_lang.litsN) {
        unsigned long long h = 14695981039346656037ULL;
        const unsigned char *q = (const unsigned char *)lit;
        int i;
        while (*q) h = (h ^ *q++) * 1099511628211ULL;
        for (i = 0; i < s_lang.litsN; i++)
            if (s_lang.lits[i].hash == h) {
                struct LangLit *L = &s_lang.lits[i];
                if (!L->osd) {
                    int cap = (int)strlen(L->str) + 2;   /* fold never grows: >=1 byte per char out */
                    L->osd = (char *)malloc((size_t)cap);
                    if (!L->osd) return lit;
                    PC_LangOsdFold(L->str, L->osd, cap);
                }
                return L->osd;
            }
    }
    return lit;
}

/* The raw K_CHARMAP blob, for pc_lang_font.c's F_WD sheet patcher -- the same records that rewrite
 * the map/bitmaps also say which SHEET CELLS need the pack glyph stamped in. */
const unsigned char *PC_LangCharmapBlob(unsigned *len) {
    if (!s_lang.loaded) LangLoad();
    *len = s_lang.charmapLen;
    return s_lang.charmap;
}

/* Called once from src/text.c's PC_FEAT hook in GetGlyphIdxForAsciiChar, with the addresses of its
 * function-static code->glyph map and the file-static bitmap array -- neither has external linkage,
 * so like terrainText below this hand-off is the only route in. Applies the pack's K_CHARMAP blob:
 * per 11-byte record (code, slot, rows[9]): map[code] = slot, and a non-blank bitmap is written
 * into that glyph slot. All-zero rows mean "map only" -- the mixed-case option remaps a-z onto the
 * lowercase art already present at slots 13-38 without shipping any art. */
void PC_LangApplyCharmap(unsigned char *map128, unsigned char (*glyphs)[9], int glyphCount) {
    unsigned n, i;
    const unsigned char *p;
    int mapped = 0, drawn = 0;
    if (!s_lang.loaded) LangLoad();
    if (!s_lang.charmap || s_lang.charmapLen < 4) return;
    p = s_lang.charmap;
    n = RdU32(p);
    if (4 + n * 11u > s_lang.charmapLen) {
        fprintf(stderr, "[lang] charmap section truncated (%u records, %u B)\n", n, s_lang.charmapLen);
        return;
    }
    for (i = 0; i < n; i++) {
        const unsigned char *r = p + 4 + i * 11;
        unsigned code = r[0], slot = r[1];
        int z, blank = 1;
        if (code >= 128 || (int)slot >= glyphCount) {
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
    fprintf(stderr, "[lang] charmap: %d code(s) mapped, %d glyph slot(s) written\n", mapped, drawn);
}

/* Called once from src/battle_0201b8.c's Objf030_FieldInfo (PC_FEAT-gated) with the address of its
 * function-static terrainText -- the battle terrain info box, "Plains   0%" and friends. That table
 * has no external linkage, so this hand-off is the only way a pack can reach it. */
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
    fprintf(stderr, "[lang] terrainText        %5d B replaced\n", bytes);
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
            fprintf(stderr, "[lang] dialogue lba=%d: %u B substituted\n", lba, n);
        }
        return;
    }
}

/* Called once, lazily, from the first VSync -- after the data-segment constructors have run. */
void PC_LangBoot(void) {
    static int done = 0;
    if (done) return;
    done = 1;
    LangLoad();
}
