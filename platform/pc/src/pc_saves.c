/* Stage-3 (1.2b) save management -- file-level card archiving. See pc_saves.h for the design. */
#include "pc_saves.h"
#include "pc_platform.h"   /* PC_SaveDir */
#include <stdio.h>
#include <string.h>
#include <time.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#if defined(_WIN32)
#include <direct.h>        /* _mkdir */
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define ACTIVE_CARD    "BASLUS-00447VH"   /* the game's fixed card file (card.c: "bu00:BASLUS-00447VH") */
#define ARCHIVE_SUBDIR ".archive"         /* dot-prefixed => invisible to the game's firstfile() scan   */

static void activePath(char *out, size_t n)  { snprintf(out, n, "%s/%s", PC_SaveDir(), ACTIVE_CARD); }
static void archiveDirPath(char *out, size_t n) { snprintf(out, n, "%s/%s", PC_SaveDir(), ARCHIVE_SUBDIR); }
static void archivePath(char *out, size_t n, const char *file) {
    snprintf(out, n, "%s/%s/%s", PC_SaveDir(), ARCHIVE_SUBDIR, file);
}

static int makeDir(const char *p) {
#if defined(_WIN32)
    return _mkdir(p);
#else
    return mkdir(p, 0755);
#endif
}

static int fileExists(const char *p) {
    struct stat st;
    return stat(p, &st) == 0 && S_ISREG(st.st_mode);
}

/* Binary file copy. Returns 1 on success; removes a partial dst on failure. */
static int copyFile(const char *src, const char *dst) {
    FILE *in, *out;
    char buf[8192];
    size_t r;
    in = fopen(src, "rb");
    if (!in) return 0;
    out = fopen(dst, "wb");
    if (!out) { fclose(in); return 0; }
    while ((r = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) { fclose(in); fclose(out); remove(dst); return 0; }
    }
    fclose(in);
    if (fclose(out) != 0) { remove(dst); return 0; }
    return 1;
}

/* Parse "…<YYYYMMDD-HHMMSS>" (the archive-name suffix) into "YYYY-MM-DD HH:MM"; fall back to the raw
 * suffix if it isn't our timestamp format (e.g. a hand-placed file). */
static void formatLabel(const char *file, char *out, size_t cap) {
    const char *dot = strrchr(file, '.');
    const char *s = dot ? dot + 1 : file;
    if (dot && strlen(s) >= 13 && s[8] == '-')
        snprintf(out, cap, "%.4s-%.2s-%.2s %.2s:%.2s", s, s + 4, s + 6, s + 9, s + 11);
    else
        snprintf(out, cap, "%s", s);
}

/* Bytes of CardFileData_Header (card.h) that precede the listing in every card file: magic[2] +
 * type + blockCount + sjisName[64] + padding[28] + clut[32] + icon1[128] + icon2[128] = 384. The game
 * writes the listing at offset + sizeof(header) (card.c: Card_WriteFile FileSeek). No pointers in the
 * header, so this size is width-independent. */
#define CARD_HEADER_SIZE 384

int PC_SaveReadCard(const char *file, PC_SaveCard *out) {
    /* Listing block (at file offset CARD_HEADER_SIZE): checksum[0..3], slotOccupied[4..7],
     * captions[3][40] at 8. All bytes/char arrays -> width-safe to parse directly. */
    char full[PATH_MAX];
    unsigned char hdr[128];
    FILE *f;
    size_t r;
    int i, j;
    if (!file || !out) return 0;
    for (i = 0; i < 3; i++) { out->occupied[i] = 0; out->slot[i][0] = '\0'; }
    archivePath(full, sizeof(full), file);
    f = fopen(full, "rb");
    if (!f) return 0;
    if (fseek(f, CARD_HEADER_SIZE, SEEK_SET) != 0) { fclose(f); return 0; }
    r = fread(hdr, 1, sizeof(hdr), f);
    fclose(f);
    if (r < sizeof(hdr)) return 0;
    for (i = 0; i < 3; i++) {
        const unsigned char *cap = hdr + 8 + i * 40;
        int n = 0;
        out->occupied[i] = (hdr[4 + i] != 0);
        if (!out->occupied[i]) continue;
        for (j = 0; j < 39 && cap[j]; j++) {
            unsigned char c = cap[j];
            if (c >= 'a' && c <= 'z') c -= 32;           /* uppercase for the caps-only overlay font */
            if (c < 0x20 || c > 0x7e) c = ' ';           /* keep it printable/in-font */
            out->slot[i][n++] = (char)c;
        }
        while (n > 0 && out->slot[i][n - 1] == ' ') n--; /* trim trailing pad spaces */
        out->slot[i][n] = '\0';
    }
    return 1;
}

int PC_SaveHasActive(void) {
    char p[PATH_MAX];
    activePath(p, sizeof(p));
    return fileExists(p);
}

int PC_SaveBackupCurrent(void) {
    char active[PATH_MAX], adir[PATH_MAX], dst[PATH_MAX], name[64];
    time_t t;
    struct tm *tm;
    activePath(active, sizeof(active));
    if (!fileExists(active)) return 0;         /* nothing to back up */
    archiveDirPath(adir, sizeof(adir));
    makeDir(adir);                             /* ensure .archive/ (ignore EEXIST) */
    t = time(NULL);
    tm = localtime(&t);
    if (!tm) return 0;
    strftime(name, sizeof(name), ACTIVE_CARD ".%Y%m%d-%H%M%S", tm);
    archivePath(dst, sizeof(dst), name);
    return copyFile(active, dst);
}

int PC_SaveRestore(const char *file) {
    char src[PATH_MAX], dst[PATH_MAX];
    if (!file || !file[0]) return 0;
    archivePath(src, sizeof(src), file);
    if (!fileExists(src)) return 0;
    activePath(dst, sizeof(dst));
    return copyFile(src, dst);
}

int PC_SaveDeleteArchive(const char *file) {
    char p[PATH_MAX];
    if (!file || !file[0]) return 0;
    archivePath(p, sizeof(p), file);
    return remove(p) == 0;
}

/* Read the active card into `buf` (cap bytes). Returns its size, or -1 if absent / larger than cap. */
static long readActiveCard(unsigned char *buf, long cap) {
    char p[PATH_MAX];
    FILE *f;
    long n;
    activePath(p, sizeof(p));
    f = fopen(p, "rb");
    if (!f) return -1;
    n = (long)fread(buf, 1, (size_t)cap, f);
    if (fgetc(f) != EOF) n = -1;               /* file bigger than the buffer -> can't confirm a match */
    fclose(f);
    return n;
}

/* 1 if file `path` is byte-identical to the `n`-byte buffer `ref`. */
static int fileMatchesBuf(const char *path, const unsigned char *ref, long n) {
    FILE *f;
    unsigned char buf[4096];
    long off = 0;
    size_t r;
    f = fopen(path, "rb");
    if (!f) return 0;
    while ((r = fread(buf, 1, sizeof(buf), f)) > 0) {
        if (off + (long)r > n || memcmp(buf, ref + off, r) != 0) { fclose(f); return 0; }
        off += (long)r;
    }
    fclose(f);
    return off == n;                           /* same length AND every byte matched */
}

/* Flag each archive that is byte-identical to the current active card (buffered once). */
static void markActive(PC_SaveArchive *out, int n) {
    static unsigned char active[0x8000];       /* card is <= 0x4000 (2 blocks); generous margin */
    char full[PATH_MAX];
    long an;
    int i;
    for (i = 0; i < n; i++) out[i].active = 0;
    an = readActiveCard(active, sizeof(active));
    if (an < 0) return;                         /* no active card / too large -> nothing to match */
    for (i = 0; i < n; i++) {
        archivePath(full, sizeof(full), out[i].file);
        out[i].active = fileMatchesBuf(full, active, an);
    }
}

int PC_SaveArchiveList(PC_SaveArchive *out, int cap) {
    char adir[PATH_MAX], full[PATH_MAX];
    DIR *d;
    struct dirent *e;
    int n = 0, i, j;
    const size_t plen = strlen(ACTIVE_CARD ".");
    if (cap <= 0) return 0;
    archiveDirPath(adir, sizeof(adir));
    d = opendir(adir);
    if (!d) return 0;                          /* no archive folder yet => none */
    while ((e = readdir(d)) != NULL && n < cap) {
        struct stat st;
        if (e->d_name[0] == '.') continue;                                   /* . .. hidden */
        if (strncmp(e->d_name, ACTIVE_CARD ".", plen) != 0) continue;        /* only our archives */
        archivePath(full, sizeof(full), e->d_name);
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        strncpy(out[n].file, e->d_name, sizeof(out[n].file) - 1);
        out[n].file[sizeof(out[n].file) - 1] = '\0';
        formatLabel(e->d_name, out[n].label, sizeof(out[n].label));
        out[n].mtime = (long)st.st_mtime;
        n++;
    }
    closedir(d);
    /* Newest first. Timestamp names sort chronologically, so a descending filename sort orders by
     * time; mtime is the tie-breaker for any non-timestamp name. Small list -> insertion sort. */
    for (i = 1; i < n; i++) {
        PC_SaveArchive key = out[i];
        j = i - 1;
        while (j >= 0 && strcmp(out[j].file, key.file) < 0) { out[j + 1] = out[j]; j--; }
        out[j + 1] = key;
    }
    markActive(out, n);                        /* flag the archive(s) matching the current card */
    return n;
}
