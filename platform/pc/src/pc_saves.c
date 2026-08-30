/* Save management: file-level card archiving. See docs/gameplay-additions.md, "Save-file internals". */
#include "pc_saves.h"
#include "pc_platform.h"   /* PC_SaveDir */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <errno.h>
#include <dirent.h>
#include <sys/stat.h>
#include <limits.h>
#if defined(_WIN32)
#include <direct.h>        /* _mkdir */
#include <io.h>            /* _commit, _fileno */
#include <process.h>       /* _getpid */
#include <windows.h>       /* MoveFileExA */
#define VH_FILENO _fileno
#define VH_FSYNC  _commit
#define VH_GETPID _getpid
#else
#include <fcntl.h>         /* open (directory durability) */
#include <unistd.h>        /* fsync, fileno, getpid */
#define VH_FILENO fileno
#define VH_FSYNC  fsync
#define VH_GETPID getpid
#endif

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

/* The game's fixed card file (core/card.c: "bu00:<id>") is region-derived: "BASLUS-00447VH" US /
 * "BISLPM-86007VH" JP. One shared saves/ dir stays safe: every archive name, listing filter and
 * restore check below carries this prefix, so no cross-region restore is possible. */
#define ACTIVE_CARD    VH_ACTIVE_CARD_NAME
#define ARCHIVE_SUBDIR ".archive"         /* dot-prefixed => invisible to the game's firstfile() scan   */

static void activePath(char *out, size_t n)  { snprintf(out, n, "%s/%s", PC_SaveDir(), ACTIVE_CARD); }
static void archiveDirPath(char *out, size_t n) { snprintf(out, n, "%s/%s", PC_SaveDir(), ARCHIVE_SUBDIR); }
static void archivePath(char *out, size_t n, const char *file) {
    snprintf(out, n, "%s/%s/%.200s", PC_SaveDir(), ARCHIVE_SUBDIR, file);
}

static int validArchiveName(const char *file) {
    const unsigned char *p;
    const size_t prefix = strlen(ACTIVE_CARD ".");
    if (!file || strncmp(file, ACTIVE_CARD ".", prefix) != 0 || strstr(file, "..")) return 0;
    for (p = (const unsigned char *)file; *p; p++)
        if (!( (*p >= 'a' && *p <= 'z') || (*p >= 'A' && *p <= 'Z') ||
               (*p >= '0' && *p <= '9') || *p == '.' || *p == '-' || *p == '_' )) return 0;
    return 1;
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

static int replaceFile(const char *temporary, const char *dst) {
#if defined(_WIN32)
    return MoveFileExA(temporary, dst, MOVEFILE_REPLACE_EXISTING | MOVEFILE_WRITE_THROUGH) != 0;
#else
    return rename(temporary, dst) == 0;
#endif
}

static FILE *openTemporary(const char *dst, char *temporary, size_t cap) {
#if defined(_WIN32)
    if (snprintf(temporary, cap, "%s.tmp.%ld", dst, (long)VH_GETPID()) >= (int)cap) return NULL;
    return fopen(temporary, "wb");
#else
    int fd;
    FILE *out;
    if (snprintf(temporary, cap, "%s.tmp.XXXXXX", dst) >= (int)cap) return NULL;
    fd = mkstemp(temporary);                     /* exclusive, same directory => rename stays atomic */
    if (fd < 0) return NULL;
    out = fdopen(fd, "wb");
    if (!out) { close(fd); remove(temporary); }
    return out;
#endif
}

static void syncParentDirectory(const char *path) {
#if !defined(_WIN32)
    char parent[PATH_MAX];
    char *slash;
    int fd;
    if (strlen(path) >= sizeof(parent)) return;
    strcpy(parent, path);
    slash = strrchr(parent, '/');
    if (!slash) return;
    *slash = '\0';
    fd = open(parent, O_RDONLY);
    if (fd >= 0) { (void)fsync(fd); close(fd); }
#else
    (void)path;
#endif
}

/* Durable binary copy: write and sync a sibling temporary, then atomically replace dst. */
static int copyFileAtomic(const char *src, const char *dst) {
    FILE *in, *out;
    char buf[8192];
    char temporary[PATH_MAX];
    size_t r;
    int ok = 1;
    in = fopen(src, "rb");
    if (!in) return 0;
    out = openTemporary(dst, temporary, sizeof(temporary));
    if (!out) { fclose(in); return 0; }
    while ((r = fread(buf, 1, sizeof(buf), in)) > 0) {
        if (fwrite(buf, 1, r, out) != r) { ok = 0; break; }
    }
    if (ferror(in)) ok = 0;
    if (fclose(in) != 0) ok = 0;
    if (ok && fflush(out) != 0) ok = 0;
    if (ok && VH_FSYNC(VH_FILENO(out)) != 0) ok = 0;
    if (fclose(out) != 0) ok = 0;
    if (ok) {
        ok = replaceFile(temporary, dst);
        if (ok) syncParentDirectory(dst);
    }
    if (!ok) remove(temporary);
    return ok;
}

/* Parse "…<YYYYMMDD-HHMMSS>" (the archive-name suffix) into "YYYY-MM-DD HH:MM"; fall back to the raw
 * suffix if it isn't our timestamp format (e.g. a hand-placed file). */
static void formatLabel(const char *file, char *out, size_t cap) {
    const char *dot = strrchr(file, '.');
    const char *s = dot ? dot + 1 : file;
    if (dot && strlen(s) >= 13 && s[8] == '-')
        snprintf(out, cap, "%.4s-%.2s-%.2s %.2s:%.2s", s, s + 4, s + 6, s + 9, s + 11);
    else
        snprintf(out, cap, "%.*s", (int)cap - 1, s);   /* truncation of odd filenames is intended */
}

/* Bytes of CardFileData_Header (card.h) before the listing: 384 on the US card, 512 on the JP card
 * (an appended icon3[128]). The icon-frame type byte is 0x12 on BOTH, so the layout is PROBED via
 * the listing's CRC32 at 384 then 512. See docs/gameplay-additions.md, "Save-file internals". */
#define CARD_HEADER_SIZE_US 384
#define CARD_HEADER_SIZE_JP 512

static unsigned long readU32LE(const unsigned char *p) {
    return (unsigned long)p[0] | (unsigned long)p[1] << 8 |
           (unsigned long)p[2] << 16 | (unsigned long)p[3] << 24;
}

static unsigned long listingCrc(const unsigned char *p, size_t n) {
    unsigned long crc = 0xffffffffUL;
    size_t i;
    int bit;
    for (i = 0; i < n; i++) {
        crc ^= p[i];
        for (bit = 0; bit < 8; bit++)
            crc = (crc & 1) ? ((crc >> 1) ^ 0xedb88320UL) : (crc >> 1);
    }
    return (~crc) & 0xffffffffUL;
}

/* Returns the card's header size (384 US / 512 JP) with the validated listing in *listing, or 0. */
static long readValidatedListing(const char *path, unsigned char *listing) {
    static const long trySizes[2] = { CARD_HEADER_SIZE_US, CARD_HEADER_SIZE_JP };
    unsigned char header[4];
    FILE *f = fopen(path, "rb");
    size_t n;
    int t;
    long found = 0;
    if (!f) return 0;
    n = fread(header, 1, sizeof(header), f);
    if (n != sizeof(header) || header[0] != 'S' || header[1] != 'C' ||
        header[2] != 0x12 || header[3] != 0x02) {
        fclose(f);
        return 0;
    }
    for (t = 0; t < 2 && !found; t++) {
        if (fseek(f, trySizes[t], SEEK_SET) != 0) break;
        n = fread(listing, 1, 128, f);
        if (ferror(f)) break;
        if (n == 128 && readU32LE(listing) == listingCrc(listing + 4, 124))
            found = trySizes[t];
    }
    if (fclose(f) != 0) return 0;
    return found;
}

static int validateCard(const char *path, unsigned char *listing) {
    enum { REGULAR_SAVE_SIZE = 0x300, CARD_RECORD_STRIDE = 0x400 };
    unsigned char save[REGULAR_SAVE_SIZE];
    FILE *f;
    int i;
    long hdrSize = readValidatedListing(path, listing);
    if (!hdrSize) return 0;
    f = fopen(path, "rb");
    if (!f) return 0;
    for (i = 0; i < 3; i++) {
        size_t n;
        if (!listing[4 + i]) continue;
        if (fseek(f, hdrSize + (long)(i + 1) * CARD_RECORD_STRIDE, SEEK_SET) != 0) {
            fclose(f); return 0;
        }
        n = fread(save, 1, sizeof(save), f);
        if (n != sizeof(save) || ferror(f) ||
            readU32LE(save) != listingCrc(save + 4, sizeof(save) - 4)) {
            fclose(f); return 0;
        }
    }
    return fclose(f) == 0;
}

int PC_SaveReadCard(const char *file, PC_SaveCard *out) {
    /* Listing block (at file offset CARD_HEADER_SIZE): checksum[0..3], slotOccupied[4..7],
     * captions[3][40] at 8. All bytes/char arrays -> width-safe to parse directly. */
    char full[PATH_MAX];
    unsigned char hdr[128];
    int i, j;
    if (!file || !out) return 0;
    for (i = 0; i < 3; i++) { out->occupied[i] = 0; out->slot[i][0] = '\0'; }
    archivePath(full, sizeof(full), file);
    if (!validArchiveName(file) || !validateCard(full, hdr)) return 0;
    for (i = 0; i < 3; i++) {
        const unsigned char *cap = hdr + 8 + i * 40;
        int n = 0;
        out->occupied[i] = (hdr[4 + i] != 0);
        if (!out->occupied[i]) continue;
        for (j = 0; j < 39 && cap[j]; j++) {
            unsigned char c = cap[j];
            /* JP captions are full-width SJIS ("１章１節　Ｌ５　０：１０"); the overlay font is caps-only
             * ASCII, so fold digits/letters/space/colon to ASCII, 章 -> '-', drop 節 => "1-1 L5 0:10".
             * (SJIS trail bytes are ASCII-range, so a byte-wise filter would shred the pairs.) */
            if ((c >= 0x81 && c <= 0x9f) || (c >= 0xe0 && c <= 0xef)) {
                unsigned int pair;
                unsigned char c2 = cap[j + 1];
                if (j + 1 >= 39 || !c2) break;           /* truncated lead byte: stop cleanly */
                pair = ((unsigned int)c << 8) | c2;
                j++;                                     /* consume the trail byte */
                if (n >= (int)sizeof(out->slot[0]) - 1) break;
                if (pair == 0x8140)                       out->slot[i][n++] = ' ';
                else if (pair == 0x8146)                  out->slot[i][n++] = ':';
                else if (pair >= 0x824f && pair <= 0x8258) out->slot[i][n++] = (char)('0' + (pair - 0x824f));
                else if (pair >= 0x8260 && pair <= 0x8279) out->slot[i][n++] = (char)('A' + (pair - 0x8260));
                else if (pair >= 0x8281 && pair <= 0x829a) out->slot[i][n++] = (char)('A' + (pair - 0x8281));
                else if (pair == 0x8fcd)                  out->slot[i][n++] = '-';   /* 章 */
                else if (pair == 0x90df)                  ;                          /* 節: dropped */
                else                                      out->slot[i][n++] = ' ';   /* other kanji */
                continue;
            }
            if (c >= 'a' && c <= 'z') c -= 32;           /* uppercase for the caps-only overlay font */
            if (c < 0x20 || c > 0x7e) c = ' ';           /* keep it printable/in-font */
            if (n >= (int)sizeof(out->slot[0]) - 1) break;
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
    if (!strftime(name, sizeof(name), ACTIVE_CARD ".%Y%m%d-%H%M%S", tm)) return 0;
    archivePath(dst, sizeof(dst), name);
    if (fileExists(dst)) {
        int suffix;
        for (suffix = 1; suffix <= 999; suffix++) {
            snprintf(name, sizeof(name), ACTIVE_CARD ".%04d%02d%02d-%02d%02d%02d-%03d",
                     tm->tm_year + 1900, tm->tm_mon + 1, tm->tm_mday,
                     tm->tm_hour, tm->tm_min, tm->tm_sec, suffix);
            archivePath(dst, sizeof(dst), name);
            if (!fileExists(dst)) break;
        }
        if (suffix > 999) return 0;
    }
    return copyFileAtomic(active, dst);
}

int PC_SaveRestore(const char *file) {
    char src[PATH_MAX], dst[PATH_MAX];
    unsigned char listing[128];
    if (!validArchiveName(file)) return 0;
    archivePath(src, sizeof(src), file);
    if (!fileExists(src) || !validateCard(src, listing)) return 0;
    activePath(dst, sizeof(dst));
    return copyFileAtomic(src, dst);
}

int PC_SaveDeleteArchive(const char *file) {
    char p[PATH_MAX];
    if (!validArchiveName(file)) return 0;
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
    if (ferror(f)) n = -1;
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
    if (ferror(f)) { fclose(f); return 0; }
    if (fclose(f) != 0) return 0;
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

static int archiveCompare(const void *av, const void *bv) {
    const PC_SaveArchive *a = (const PC_SaveArchive *)av;
    const PC_SaveArchive *b = (const PC_SaveArchive *)bv;
    if (a->mtime > b->mtime) return -1;
    if (a->mtime < b->mtime) return 1;
    return strcmp(b->file, a->file);
}

int PC_SaveArchiveListAlloc(PC_SaveArchive **out) {
    char adir[PATH_MAX], full[PATH_MAX];
    DIR *d;
    struct dirent *e;
    PC_SaveArchive *items = NULL;
    int n = 0, cap = 0;
    const size_t plen = strlen(ACTIVE_CARD ".");
    if (!out) return -1;
    *out = NULL;
    archiveDirPath(adir, sizeof(adir));
    d = opendir(adir);
    if (!d) return errno == ENOENT ? 0 : -1;   /* no archive folder yet => none; other errors matter */
    while ((e = readdir(d)) != NULL) {
        struct stat st;
        PC_SaveArchive *grown;
        if (e->d_name[0] == '.') continue;                                   /* . .. hidden */
        if (strncmp(e->d_name, ACTIVE_CARD ".", plen) != 0) continue;        /* only our archives */
        if (!validArchiveName(e->d_name)) continue;
        archivePath(full, sizeof(full), e->d_name);
        if (stat(full, &st) != 0 || !S_ISREG(st.st_mode)) continue;
        if (n == cap) {
            int next = cap ? cap * 2 : 16;
            grown = (PC_SaveArchive *)realloc(items, (size_t)next * sizeof(*items));
            if (!grown) { free(items); closedir(d); return -1; }
            items = grown;
            cap = next;
        }
        strncpy(items[n].file, e->d_name, sizeof(items[n].file) - 1);
        items[n].file[sizeof(items[n].file) - 1] = '\0';
        formatLabel(e->d_name, items[n].label, sizeof(items[n].label));
        items[n].mtime = (long long)st.st_mtime;
        n++;
    }
    closedir(d);
    qsort(items, (size_t)n, sizeof(*items), archiveCompare);
    markActive(items, n);                      /* flag the archive(s) matching the current card */
    *out = items;
    return n;
}

void PC_SaveArchiveListFree(PC_SaveArchive *out) { free(out); }

int PC_SaveArchiveList(PC_SaveArchive *out, int cap) {
    PC_SaveArchive *all = NULL;
    int n, take;
    if (!out || cap <= 0) return 0;
    n = PC_SaveArchiveListAlloc(&all);
    if (n < 0) return 0;
    take = n < cap ? n : cap;
    if (take > 0) memcpy(out, all, (size_t)take * sizeof(*out));
    free(all);
    return take;
}
