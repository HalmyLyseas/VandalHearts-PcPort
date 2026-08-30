/* pc_hdpack.c -- HD background replacement: pack detection, a VRAM-region registry keyed by an FNV-1a
 * hash of each LoadImage upload, and the async decode that publishes hi-res pixels for the hi-res pass
 * to sample at sub-texel precision. FMVs: pc_hdvideo.c. See docs/hd-pack.md, "Engine side". */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <pthread.h>     /* the async replacement loader thread (winpthreads on MinGW) */
#include <sys/stat.h>    /* stat(): the async loader's existence probe (MinGW has it too) */
#include <dirent.h>      /* HdDetect: sanity-count the pack videos dir */
#ifdef _WIN32
#include <direct.h>
#define HD_MKDIR(d) _mkdir(d)
#else
#define HD_MKDIR(d) mkdir((d), 0777)
#endif

#include "PsyQ/libgpu.h"
#include "pc_platform.h"
#include "pc_gpu_internal.h"
#include "pc_lang.h"      /* PC_LangBgDir: localized backgrounds ride the language selection */

#define HD_MAX_REGIONS 512
static HdRegion s_hdReg[HD_MAX_REGIONS];
static int s_hdRegN, s_hdReplaceN;
int HdRegionCount(void)  { return s_hdRegN; }      /* per-triangle gates in the DDA (pc_gpu_internal.h) */
int HdReplaceCount(void) { return s_hdReplaceN; }

/* Auto-detect + validate an installed HD pack beside the exe/AppImage: <deploy>/hdpacks/<game-id>/
 * manifest.json, whose "game" id must equal this build's HD_GAME_ID ("SLUS-00447" US / "SLPM-86007"
 * JP). A pack for the other region is rejected BY NAME so the failure is explained, not silent. */
#define HD_GAME_ID VH_HD_GAME_ID
#define HD_PATH    1024
/* Replacement-image budget, checked from header dims before any pixel decode: the sharpest
 * shipped pack (8x) tops out at 2560x1920, so this is generous headroom, not a real limit --
 * it exists to stop an oversize/hostile file forcing a multi-GB allocation. */
#define HD_MAX_SIDE    8192
#define HD_MAX_PIXELS  (32 * 1024 * 1024)
extern int PC_GetDeployDir(char *out, size_t outSize);   /* pc_bootstrap.c (exe dir, or AppImage dir) */
extern int g_vhHdPack;         /* runtime on/off toggle, owned by pc_gpu_window.c; the overlay binds it.
                                * 0 until a valid pack is detected (HdDetect sets it: persisted VH_HDPACK, or
                                * auto-ON on first detect). The VH_HD_PACK=<dir> override ignores it (CI). */
static struct { int checked, available, valid, count, videoCount, packVersion; char dir[HD_PATH + 64]; char videosDir[HD_PATH + 64]; char reason[80]; } s_hdPack;

static const char *HdEnv(const char *name, int slot) {
    static const char *v[2]; static int done[2];
    if (!done[slot]) { v[slot] = getenv(name); done[slot] = 1; }
    return v[slot];
}
const char *HdDumpDir(void) { return HdEnv("VH_HD_DUMP", 1); }

/* Minimal read of the (controlled) manifest: "game" (string) + "count" (int). 1 if a game id was found.
 * Not a general JSON parser -- just enough to validate + report, both fields near the top of the file. */
static int HdManifestRead(const char *path, char *game, int gameSz, int *count) {
    FILE *f = fopen(path, "rb"); char buf[8192], *p; size_t n;
    game[0] = '\0'; *count = 0;
    if (!f) return 0;
    n = fread(buf, 1, sizeof(buf) - 1, f); fclose(f); buf[n] = '\0';
    if ((p = strstr(buf, "\"game\"")) && (p = strchr(p, ':')) && (p = strchr(p, '"'))) {
        int i = 0; p++;
        while (*p && *p != '"' && i < gameSz - 1) game[i++] = *p++;
        game[i] = '\0';
    }
    if ((p = strstr(buf, "\"count\"")) && (p = strchr(p, ':'))) *count = atoi(p + 1);
    if ((p = strstr(buf, "\"packVersion\"")) && (p = strchr(p, ':'))) s_hdPack.packVersion = atoi(p + 1);
    if ((p = strstr(buf, "\"videos\"")) && (p = strchr(p, ':'))) s_hdPack.videoCount = atoi(p + 1);
    return game[0] != '\0';
}

/* Detect + validate <deploy>/hdpacks (runs once; caches in s_hdPack). reason[] drives the overlay label. */
static void HdDetect(void) {
    char deploy[HD_PATH], base[HD_PATH + 32], manifest[HD_PATH + 64], game[80];
    if (s_hdPack.checked) return;
    s_hdPack.checked = 1;
    snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "no HD pack");
    if (!PC_GetDeployDir(deploy, sizeof(deploy))) return;
    /* Layout: hdpacks/<game-id>/ -- one subfolder per region, keyed by the COMPILE-TIME HD_GAME_ID
     * (so an Asia SCPS-45183 disc, served by the US core, resolves hdpacks/SLUS-00447/). Fallback:
     * the flat hdpacks/ layout, whose manifest is game-id-checked below either way. */
    snprintf(base, sizeof(base), "%s/hdpacks/%s", deploy, HD_GAME_ID);
    snprintf(manifest, sizeof(manifest), "%s/manifest.json", base);
    if (!HdManifestRead(manifest, game, sizeof(game), &s_hdPack.count)) {
        snprintf(base, sizeof(base), "%s/hdpacks", deploy);                   /* legacy flat layout */
        snprintf(manifest, sizeof(manifest), "%s/manifest.json", base);
        if (!HdManifestRead(manifest, game, sizeof(game), &s_hdPack.count)) return;   /* no pack */
    }
    s_hdPack.available = 1;
    if (strcmp(game, HD_GAME_ID) != 0) {                 /* pack for a different disc/region */
        snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "HD pack is for %.60s", game);
        fprintf(stderr, "[HD] pack present but for '%s' (this build is %s) -> HD PACK unavailable\n",
                game, HD_GAME_ID);
        return;
    }
    if (s_hdPack.packVersion < 2) {                      /* packVersion 1: no videos manifest */
        snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "OUTDATED PACK");
        fprintf(stderr, "[HD] pack manifest is v%d; this build needs v2 -- regenerate it with "
                        "tools/hdpack/vh_hdpack_manifest.py (or download the current pack)\n",
                s_hdPack.packVersion);
        return;
    }
    s_hdPack.valid = 1;
    s_hdPack.reason[0] = '\0';
    snprintf(s_hdPack.dir, sizeof(s_hdPack.dir), "%s/backgrounds", base);
    snprintf(s_hdPack.videosDir, sizeof(s_hdPack.videosDir), "%s/videos", base);
    {   /* best-practice sanity: the videos/ dir should hold what the manifest declares */
        DIR *d = opendir(s_hdPack.videosDir); int found = 0;
        if (d) { struct dirent *de;
                 while ((de = readdir(d)) != NULL) if (strstr(de->d_name, ".mp4")) found++;
                 closedir(d); }
        if (found != s_hdPack.videoCount)
            fprintf(stderr, "[HD] videos/: %d file(s) but the manifest declares %d -- FMVs missing "
                            "from the pack?\n", found, s_hdPack.videoCount);
    }
    { const char *e = getenv("VH_HDPACK");           /* persisted choice (ini->env) wins; else auto-ON */
      g_vhHdPack = e ? (atoi(e) != 0) : 1; }
    fprintf(stderr, "[HD] pack detected + valid: %s (%d backgrounds, %d videos)%s\n",
            s_hdPack.dir, s_hdPack.count, s_hdPack.videoCount, g_vhHdPack ? " -> ON" : " (off, persisted)");
}

/* Is HD replacement live right now? Explicit VH_HD_PACK override => always (CI/power-user); else the
 * auto-detected valid pack gated by the g_vhHdPack toggle. Cheap enough for the per-triangle sample gate. */
int HdActive(void) {
    const char *env = HdEnv("VH_HD_PACK", 0);
    if (env && *env) return 1;
    HdDetect();
    return s_hdPack.valid && g_vhHdPack;
}
/* The active pack directory (or NULL). */
static const char *HdPackDir(void) {
    const char *env;
    if (!HdActive()) return NULL;
    env = HdEnv("VH_HD_PACK", 0);
    return (env && *env) ? env : s_hdPack.dir;
}

/* PC_LangBgDir (pc_lang.h) is the langpack backgrounds/ source -- gated on MANIFEST ACCEPTANCE,
 * independent of the HD PACK toggle (localized backgrounds are translation, not an enhancement).
 * Resolved before the HD pack (see BgSourceDir), so a translated background wins. */

/* Any background-replacement source live right now (HD pack OR a langpack backgrounds/). Gates the
 * per-triangle region resolve in the DDA (pc_raster.c) and the registration in HdPack_OnLoad. */
int HdAnyActive(void) { return HdActive() || PC_LangBgDir() != NULL; }

/* Public API for the options overlay. The toggle itself is g_vhHdPack (bound directly). */
int PC_HdPackAvailable(void)      { HdDetect(); return s_hdPack.valid; }
int PC_HdPackEnabled(void)        { HdDetect(); return s_hdPack.valid && g_vhHdPack; }
int PC_HdPackCount(void)          { HdDetect(); return s_hdPack.count; }
int PC_HdPackVideoCount(void)     { HdDetect(); return s_hdPack.videoCount; }
/* Short UPPERCASE status for the overlay's value column when the pack can't be used (the OSD font is
 * caps-only); NULL when a valid pack is installed (normal ON/OFF handling applies). */
const char *PC_HdPackStatusShort(void) {
    HdDetect();
    if (s_hdPack.valid) return NULL;
    if (!s_hdPack.available) return "NO PACK";
    if (s_hdPack.packVersion && s_hdPack.packVersion < 2) return "OUTDATED PACK";
    return "WRONG GAME";
}
const char *PC_HdPackReason(void) { HdDetect(); return s_hdPack.reason; }
/* HD FMV directory (<deploy>/hdpacks/videos), or NULL when HD is off / no valid pack. VH_HD_VIDEOS=<dir>
 * overrides (CI). Gated by the same g_vhHdPack toggle as backgrounds -- videos ride the HD PACK option. */
const char *PC_HdPackVideosDir(void) {
    const char *env = getenv("VH_HD_VIDEOS");
    if (env && *env) return env;
    if (!g_vhHdPack) return NULL;
    HdDetect();
    return s_hdPack.valid ? s_hdPack.videosDir : NULL;
}

static unsigned long long HdHash(const unsigned short *s, int n) {
    unsigned long long h = 1469598103934665603ULL; int i;
    for (i = 0; i < n; i++) { h ^= s[i]; h *= 1099511628211ULL; }
    return h;
}
static HdRegion *HdFind(unsigned long long h) {
    int i; for (i = 0; i < s_hdRegN; i++) if (s_hdReg[i].hash == h) return &s_hdReg[i];
    return NULL;
}
#ifdef VH_HD_WEBP
#include <webp/decode.h>
/* Decode <dir>/<hash>.webp to RGBA8 (WebPDecodeRGBAInto emits R,G,B,A byte order = our px layout).
 * WebP is ~7x smaller than PNG for these backgrounds; decode happens once at scene load, never per frame. */
static unsigned int *HdLoadWebp(const char *dir, unsigned long long h, int *ow, int *oh) {
    char path[HD_PATH + 64]; FILE *f; long sz; unsigned char *buf; unsigned int *px = NULL; int w, hh;
    snprintf(path, sizeof(path), "%s/%016llx.webp", dir, h);
    f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    buf = (unsigned char *)malloc((size_t)sz);
    /* Budget from the header BEFORE decoding -- WebPGetInfo only reads the container, so a tiny
     * solid-colour file cannot force a huge WebPDecodeRGBA allocation. */
    if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz &&
        WebPGetInfo(buf, (size_t)sz, &w, &hh) &&
        w > 0 && hh > 0 && w <= HD_MAX_SIDE && hh <= HD_MAX_SIDE &&
        (long long)w * hh <= HD_MAX_PIXELS) {
        px = (unsigned int *)malloc((size_t)w * hh * 4);
        if (px) {
            /* Decode straight into our own buffer -- no WebPDecodeRGBA + memcpy + WebPFree copy. */
            if (WebPDecodeRGBAInto(buf, (size_t)sz, (uint8_t *)px, (size_t)w * hh * 4, w * 4))
                { *ow = w; *oh = hh; }
            else
                { free(px); px = NULL; }
        }
    }
    free(buf); fclose(f);
    return px;
}
#endif
static unsigned int *HdLoadHdi(const char *dir, unsigned long long h, int *ow, int *oh) {
    char path[HD_PATH + 64]; unsigned char hd[12]; FILE *f; int w, hh; unsigned int *px; size_t n;
    snprintf(path, sizeof(path), "%s/%016llx.hdi", dir, h);
    f = fopen(path, "rb"); if (!f) return NULL;
    if (fread(hd, 1, 12, f) != 12 || memcmp(hd, "HDI1", 4) != 0) { fclose(f); return NULL; }
    w  = hd[4] | hd[5] << 8 | hd[6] << 16 | hd[7] << 24;
    hh = hd[8] | hd[9] << 8 | hd[10] << 16 | hd[11] << 24;
    if (w <= 0 || hh <= 0 || w > HD_MAX_SIDE || hh > HD_MAX_SIDE ||
        (long long)w * hh > HD_MAX_PIXELS) { fclose(f); return NULL; }
    n = (size_t)w * hh; px = (unsigned int *)malloc(n * 4);
    if (!px) { fclose(f); return NULL; }
    if (fread(px, 4, n, f) != n) { free(px); fclose(f); return NULL; }
    fclose(f); *ow = w; *oh = hh; return px;
}
/* Load a replacement: prefer <hash>.webp (compressed), else <hash>.hdi (raw). */
static unsigned int *HdLoadImage(const char *dir, unsigned long long h, int *ow, int *oh) {
#ifdef VH_HD_WEBP
    unsigned int *px = HdLoadWebp(dir, h, ow, oh);
    if (px) return px;
#endif
    return HdLoadHdi(dir, h, ow, oh);
}
/* Pre-pack the loaded RGBA8 replacement to the 16-bit target texel format ONCE at load: halves resident
 * memory (cache-friendlier for the ~1.2M-px/frame sample loop) and removes the per-pixel RGBA->555 pack
 * from the hot loop. 0x0000 = transparent (native texel!=0 rule); opaque black becomes 0x0421 (1,1,1). */
static unsigned short *HdPack16(const unsigned int *rgba, int n) {
    unsigned short *px = (unsigned short *)malloc((size_t)n * 2); int i;
    if (!px) return NULL;
    for (i = 0; i < n; i++) {
        unsigned int p = rgba[i];
        if (((p >> 24) & 0xFF) < 128) { px[i] = 0; continue; }
        { unsigned short t = (unsigned short)((((p >> 16) & 0xFF) >> 3) << 10 |
                                              (((p >> 8) & 0xFF) >> 3) << 5 | ((p & 0xFF) >> 3));
          px[i] = t ? t : 0x0421; }
    }
    return px;
}

/* ---- async replacement loader --------------------------------------------------------------------
 * LoadImage only stat()s and queues the region; one detached thread decodes and publishes r->px with
 * a release store (LIFO: newest upload first). VH_HD_SYNC=1 decodes inline. See docs/hd-pack.md. */
static int HdFileExists(const char *dir, unsigned long long h) {
    char path[HD_PATH + 64]; struct stat st;
    snprintf(path, sizeof(path), "%s/%016llx.webp", dir, h);
    if (stat(path, &st) == 0) return 1;
    snprintf(path, sizeof(path), "%s/%016llx.hdi", dir, h);
    return stat(path, &st) == 0;
}
/* Which source holds a replacement for this hash? Langpack backgrounds/ first (priority), then
 * the HD pack. NULL if neither. Up to two stats -- called once per unique background at registration. */
static const char *BgSourceDir(unsigned long long h) {
    const char *ld = PC_LangBgDir();
    if (ld && HdFileExists(ld, h)) return ld;
    { const char *hd = HdPackDir(); if (hd && HdFileExists(hd, h)) return hd; }
    return NULL;
}
static pthread_mutex_t s_hdLoadMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_hdLoadCv  = PTHREAD_COND_INITIALIZER;
static HdRegion *s_hdLoadQ[HD_MAX_REGIONS];
static int s_hdLoadQn, s_hdLoaderUp;
/* Each region carries its own source dir (r->dir): langpack and HD backgrounds can both be live, so
 * there is no single pack-dir snapshot; the loader reads from the region's own source. */

static void *HdLoaderMain(void *arg) {
    (void)arg;
    for (;;) {
        HdRegion *r; int w = 0, hh = 0; unsigned int *rgba; unsigned short *px = NULL;
        pthread_mutex_lock(&s_hdLoadMtx);
        while (s_hdLoadQn == 0) pthread_cond_wait(&s_hdLoadCv, &s_hdLoadMtx);
        r = s_hdLoadQ[--s_hdLoadQn];             /* LIFO: newest upload = current scene first */
        pthread_mutex_unlock(&s_hdLoadMtx);
        rgba = HdLoadImage(r->dir, r->hash, &w, &hh);
        if (rgba) { px = HdPack16(rgba, (int)((size_t)w * hh)); free(rgba); }
        if (px) {
            r->w = w; r->h = hh;                 /* dims first, then the pointer gates visibility */
            __atomic_store_n(&r->px, px, __ATOMIC_RELEASE);
            if (PC_Verbose())
                fprintf(stderr, "[HD] REPLACED %016llx rect=(%d,%d) %dx%dw hd=%dx%d src=%s (async)\n",
                        r->hash, r->rx, r->ry, r->rw, r->rh, w, hh, r->dir);
        } else {
            fprintf(stderr, "[HD] async load FAILED %016llx (file vanished or bad?)\n", r->hash);
        }
    }
    return NULL;
}
static void HdLoaderQueue(HdRegion *r) {         /* source is r->dir, set at registration */
    pthread_mutex_lock(&s_hdLoadMtx);
    if (!s_hdLoaderUp) {
        pthread_t th; pthread_attr_t at;
        pthread_attr_init(&at); pthread_attr_setdetachstate(&at, PTHREAD_CREATE_DETACHED);
        if (pthread_create(&th, &at, HdLoaderMain, NULL) == 0) s_hdLoaderUp = 1;
        pthread_attr_destroy(&at);
    }
    if (s_hdLoaderUp && s_hdLoadQn < HD_MAX_REGIONS) {
        s_hdLoadQ[s_hdLoadQn++] = r;
        pthread_cond_signal(&s_hdLoadCv);
        pthread_mutex_unlock(&s_hdLoadMtx);
    } else {                                     /* thread refused to start: fall back to inline */
        int w = 0, hh = 0; unsigned int *rgba;
        pthread_mutex_unlock(&s_hdLoadMtx);
        rgba = HdLoadImage(r->dir, r->hash, &w, &hh);
        if (rgba) { r->px = HdPack16(rgba, (int)((size_t)w * hh)); free(rgba);
                    if (r->px) { r->w = w; r->h = hh; } }
    }
}

/* Called from LoadImage after the VRAM write. Many assets (e.g. every full-screen background) reuse the
 * SAME VRAM rect, so regions are found-by-hash but must be matched-by-rect at draw time -- we track which
 * region is LIVE (its content is what's in VRAM at that rect right now) and update it on every upload. */
void HdPack_OnLoad(const RECT *rect, const unsigned short *src) {
    const char *dp = HdDumpDir(); unsigned long long h; HdRegion *r = NULL; int i;   /* source resolved per-hash via BgSourceDir */
    if (rect->w <= 0 || rect->h <= 0) return;
    /* Hash + register/replace only when a pack (or dump) is active. The LIVE-region invalidation below
     * runs on EVERY upload regardless of the toggle: a background uploaded while HD PACK is OFF still
     * evicts the stale live region it overwrites, so toggling ON mid-scene never resamples an old scene. */
    if (HdAnyActive() || dp) {                     /* any bg source (HD pack OR langpack), or dumping */
        if (dp) { static int mk; if (!mk) { mk = 1; HD_MKDIR(dp); } }
        h = HdHash(src, rect->w * rect->h);
        r = HdFind(h);
        if (!r) {                                 /* first time we see this content -> register it */
            if (s_hdRegN >= HD_MAX_REGIONS) {
                static int warned; if (!warned) { warned = 1; fprintf(stderr, "[HD] region cap %d hit -- raise HD_MAX_REGIONS\n", HD_MAX_REGIONS); }
            } else {
                r = &s_hdReg[s_hdRegN];
                r->hash = h; r->rx = rect->x; r->ry = rect->y; r->rw = rect->w; r->rh = rect->h; r->px = NULL; r->w = r->h = 0; r->dumped = 0; r->live = 0; r->dir = NULL;
                if (dp && getenv("VH_HD_RAW")) {   /* diagnostic: exact hashed source bytes, to reverse the on-disc->VRAM layout offline */
                    char rp[600]; FILE *rf; snprintf(rp, sizeof(rp), "%s/%016llx_%dx%d.raw", dp, h, rect->w, rect->h);
                    rf = fopen(rp, "wb"); if (rf) { fwrite(src, 2, (size_t)rect->w * rect->h, rf); fclose(rf); }
                }
                {
                    const char *repl = BgSourceDir(h);   /* langpack first, then HD pack (cheap stat) */
                    int hasRepl = repl != NULL;
                    if (hasRepl) {
                        static int syncMode = -1;
                        if (syncMode < 0) { const char *e = getenv("VH_HD_SYNC"); syncMode = e && atoi(e) != 0; }
                        r->dir = repl;             /* the loader (async or inline) reads from this source */
                        s_hdReplaceN++;            /* counts as replaced; draw path skips while px==NULL */
                        if (syncMode) {            /* old inline decode, for A/B + debugging */
                            unsigned int *rgba = HdLoadImage(repl, h, &r->w, &r->h);
                            if (rgba) { r->px = HdPack16(rgba, r->w * r->h); free(rgba); }
                            if (r->px) { if (PC_Verbose()) fprintf(stderr, "[HD] REPLACED %016llx rect=(%d,%d) %dx%dw hd=%dx%d src=%s (sync)\n", h, rect->x, rect->y, rect->w, rect->h, r->w, r->h, repl); }
                            else { r->w = r->h = 0; hasRepl = 0; s_hdReplaceN--; r->dir = NULL; }
                        } else {
                            HdLoaderQueue(r);      /* decode + publish on the loader thread (reads r->dir) */
                        }
                    }
                    if (hasRepl || dp) s_hdRegN++; else r = NULL;   /* track: has/awaits a replacement, or dumping */
                }
            }
        }
    }
    /* This upload now occupies (part of) VRAM: invalidate EVERY region whose rect it OVERLAPS, not just
     * exact-rect reuse -- a dynamic texture uploaded into a sub-rect of a backdrop would otherwise leave
     * it stale-live and HD-replace whatever samples that VRAM. Runs even with HD off (r==NULL). */
    for (i = 0; i < s_hdRegN; i++) {
        HdRegion *o = &s_hdReg[i];
        if (o == r) continue;
        if (!(rect->x + rect->w <= o->rx || rect->x >= o->rx + o->rw ||
              rect->y + rect->h <= o->ry || rect->y >= o->ry + o->rh))
            o->live = 0;
    }
    if (r) r->live = 1;
}
/* Decode a dump-pending region to a .ppm, using the CLUT actually being used to sample it (VH_HD_DUMP).
 * Called from the sample hook the first time a region's pixel is read -> guarantees the right palette
 * (the upload-time palette is unknown) and the correct native pixel size, for matching to HD assets. */
static void HdDecodeAndDump(HdRegion *r, int tp, int clut) {
    const char *dp = HdDumpDir();
    unsigned short (*vram)[PC_GPU_VRAM_W] = PC_GpuVram();
    int clutX = (clut & 0x3F) * 16, clutY = (clut >> 6) & 0x1FF;
    int ppw = (tp == 0) ? 4 : (tp == 1) ? 2 : 1, nW = r->rw * ppw, nH = r->rh, row, word, k;
    char path[600]; FILE *f;
    r->dumped = 1;
    snprintf(path, sizeof(path), "%s/%016llx_%dx%d.ppm", dp, r->hash, nW, nH);
    f = fopen(path, "wb");
    if (!f) { fprintf(stderr, "[HD] dump OPEN-FAIL %s\n", path); return; }
    fprintf(stderr, "[HD] DUMP %016llx %dx%d tp%d clut(%d,%d)\n", r->hash, nW, nH, tp, clutX, clutY);
    fprintf(f, "P6\n%d %d\n255\n", nW, nH);
    for (row = 0; row < nH; row++) for (word = 0; word < r->rw; word++) {
        unsigned short hw = vram[r->ry + row][r->rx + word];
        for (k = 0; k < ppw; k++) {
            unsigned short raw; int R, G, B;
            if (tp == 0)      raw = vram[clutY][clutX + ((hw >> (k * 4)) & 0xF)];
            else if (tp == 1) raw = vram[clutY][clutX + (k ? ((hw >> 8) & 0xFF) : (hw & 0xFF))];
            else              raw = hw;
            UnpackColor(raw, &R, &G, &B); fputc(R, f); fputc(G, f); fputc(B, f);
        }
    }
    fclose(f);
}
/* Per-TRIANGLE dump (VH_HD_DUMP): cheap. Given a textured triangle's tpage/clut and its texel-UV
 * bounding box, dump any not-yet-dumped region whose VRAM footprint it samples, with the correct CLUT. */
void HdMaybeDump(int tpage, int clut, int uMin, int uMax, int vMin, int vMax) {
    int tpX, tpY, tp, ppw, wx0, wx1, wy0, wy1, i;
    if (!HdDumpDir() || !s_hdRegN) return;
    TPageOrigin(tpage, &tpX, &tpY, &tp);
    ppw = (tp == 0) ? 4 : (tp == 1) ? 2 : 1;
    wx0 = tpX + uMin / ppw; wx1 = tpX + uMax / ppw; wy0 = tpY + vMin; wy1 = tpY + vMax;
    for (i = 0; i < s_hdRegN; i++) {
        HdRegion *r = &s_hdReg[i];
        if (r->dumped) continue;
        if (wx1 < r->rx || wx0 >= r->rx + r->rw || wy1 < r->ry || wy0 >= r->ry + r->rh) continue;  /* no overlap */
        HdDecodeAndDump(r, tp, clut);
    }
}
/* Per-TRIANGLE replace resolve (hi-res pass): the replaced region this textured triangle samples (via
 * its texel-UV bbox -> VRAM footprint), or NULL. Per-pixel HD sampling is then inlined in dda_span with
 * precomputed constants. Only regions with a replacement are registered, so the scan is short. */
HdRegion *HdFindTriRegion(int tpage, int uMin, int uMax, int vMin, int vMax) {
    int tpX, tpY, tp, ppw, wx0, wx1, wy0, wy1, i;
    TPageOrigin(tpage, &tpX, &tpY, &tp);
    /* Every HD-replaced asset is an 8bpp background (tp==1). Battle draws (unit sprites, effects,
     * cursor tiles, HP bars) are 4bpp and merely share VRAM words with a still-live background region;
     * they bypass LoadImage (bulk DMA) so eviction cannot catch them -- the bit-depth guard does. */
    if (tp != 1) return NULL;
    ppw = (tp == 0) ? 4 : (tp == 1) ? 2 : 1;
    wx0 = tpX + uMin / ppw; wx1 = tpX + uMax / ppw; wy0 = tpY + vMin; wy1 = tpY + vMax;
    for (i = 0; i < s_hdRegN; i++) {
        HdRegion *r = &s_hdReg[i];
        /* acquire pairs with the loader thread's release publish of px: once non-NULL, w/h and the
         * pixel data are visible too. Until then the region is skipped -> native texels draw. */
        if (!__atomic_load_n(&r->px, __ATOMIC_ACQUIRE) || !r->live) continue;
        /* The HD PACK toggle gates HD-PACK-sourced regions PER SAMPLE (a live region would otherwise
         * keep drawing after toggling OFF mid-scene). Langpack regions (r->dir is PC_LangBgDir()'s one
         * static buffer -- pointer identity is exact) are translation, not an enhancement: never gated. */
        if (!HdActive() && r->dir != PC_LangBgDir()) continue;
        /* At internal scale 1 the shadow pass exists ONLY for the langpack (pc_raster.c HiresWanted):
         * localized backgrounds must not depend on a graphics setting, while HD-pack backgrounds stay
         * a >= 2x enhancement ("HD PACK requires internal res" in the manual). */
        if (PC_GpuGetInternalScale() == 1 && r->dir != PC_LangBgDir()) continue;
        if (wx1 < r->rx || wx0 >= r->rx + r->rw || wy1 < r->ry || wy0 >= r->ry + r->rh) continue;
        return r;
    }
    return NULL;
}
