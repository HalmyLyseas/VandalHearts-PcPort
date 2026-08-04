/* pc_hdpack.c -- HD asset-replacement: backgrounds (1.6 HD pack).
 * Extracted verbatim from libgpu.c (it accreted there because LoadImage and the DDA sampler are its
 * hooks; the subsystem itself is pack detection + region registry + async decode, not GPU emulation).
 * Seams with libgpu.c: pc_gpu_internal.h. The FMV half of the pack lives in pc_hdvideo.c.
 *
 * Replace a native texture region (e.g. a background) with a hi-res image, sampled at SUB-TEXEL
 * precision during the hi-res pass -- so it shows real HD detail, not the native-texel NN block that
 * VH_INTERNAL_SCALE already gives. Native pass + non-HD runs are byte-for-byte untouched.
 *
 * Identity = an FNV-1a hash of the raw uploaded VRAM block (in LoadImage). Regions are keyed by VRAM
 * WORD coords, so multi-quad / multi-tpage drawing of one upload maps back to the right HD pixel.
 *   VH_HD_DUMP=<dir>  -> write a grayscale .pgm of each unique upload (shape, to identify it)
 *   VH_HD_PACK=<dir>  -> load <dir>/<hash>.hdi (raw RGBA8 + header) and replace that region
 * HD assets (.hdi) are produced offline from PNG by workflow/vh_hdi_pack.py. */
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

#define HD_MAX_REGIONS 512
static HdRegion s_hdReg[HD_MAX_REGIONS];
static int s_hdRegN, s_hdReplaceN;
int HdRegionCount(void)  { return s_hdRegN; }      /* per-triangle gates in the DDA (pc_gpu_internal.h) */
int HdReplaceCount(void) { return s_hdReplaceN; }

/* GRAD 1 (1.6): auto-detect + validate an installed HD pack beside the exe/AppImage (disc-.bin pattern).
 *   <deploy>/hdpacks/manifest.json               validated: its "game" id must match this build
 *   <deploy>/hdpacks/backgrounds/<hash>.webp     the replacement images
 * VH_HD_PACK=<dir> still overrides (points straight at a <hash>.webp folder, skipping detection). */
#define HD_GAME_ID "SLUS-00447"
#define HD_PATH    1024
extern int PC_GetDeployDir(char *out, size_t outSize);   /* pc_bootstrap.c (exe dir, or AppImage dir) */
extern int g_vhHdPack;         /* runtime on/off toggle, owned by pc_gpu_window.c; the overlay binds it.
                                * 0 until a valid pack is detected (HdDetect sets it: persisted VH_HDPACK, or
                                * auto-ON on first detect). The VH_HD_PACK=<dir> override ignores it (CI). */
static struct { int checked, available, valid, count, videoCount, packVersion; char dir[HD_PATH + 32]; char videosDir[HD_PATH + 32]; char reason[80]; } s_hdPack;

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
    char deploy[HD_PATH], manifest[HD_PATH + 32], game[80];
    if (s_hdPack.checked) return;
    s_hdPack.checked = 1;
    snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "no HD pack");
    if (!PC_GetDeployDir(deploy, sizeof(deploy))) return;
    snprintf(manifest, sizeof(manifest), "%s/hdpacks/manifest.json", deploy);
    if (!HdManifestRead(manifest, game, sizeof(game), &s_hdPack.count)) return;   /* no/blank manifest */
    s_hdPack.available = 1;
    if (strcmp(game, HD_GAME_ID) != 0) {                 /* pack for a different disc/region */
        snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "HD pack is for %.60s", game);
        fprintf(stderr, "[HD] pack present but for '%s' (this build is %s) -> HD PACK unavailable\n",
                game, HD_GAME_ID);
        return;
    }
    if (s_hdPack.packVersion < 2) {                      /* 1.6.0-era pack: no videos manifest */
        snprintf(s_hdPack.reason, sizeof(s_hdPack.reason), "OUTDATED PACK");
        fprintf(stderr, "[HD] pack manifest is v%d; this build needs v2 -- regenerate it with "
                        "tools/hdpack/vh_hdpack_manifest.py (or download the current pack)\n",
                s_hdPack.packVersion);
        return;
    }
    s_hdPack.valid = 1;
    s_hdPack.reason[0] = '\0';
    snprintf(s_hdPack.dir, sizeof(s_hdPack.dir), "%s/hdpacks/backgrounds", deploy);
    snprintf(s_hdPack.videosDir, sizeof(s_hdPack.videosDir), "%s/hdpacks/videos", deploy);
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

/* Public API for the options overlay (GRAD 2/3). The toggle itself is g_vhHdPack (bound directly). */
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
/* Decode <dir>/<hash>.webp to RGBA8 (WebPDecodeRGBA already emits R,G,B,A byte order = our px layout).
 * WebP is ~7x smaller than PNG for these backgrounds; decode happens once at scene load, never per frame. */
static unsigned int *HdLoadWebp(const char *dir, unsigned long long h, int *ow, int *oh) {
    char path[HD_PATH + 64]; FILE *f; long sz; unsigned char *buf; unsigned int *px = NULL; int w, hh; uint8_t *rgba;
    snprintf(path, sizeof(path), "%s/%016llx.webp", dir, h);
    f = fopen(path, "rb"); if (!f) return NULL;
    fseek(f, 0, SEEK_END); sz = ftell(f); fseek(f, 0, SEEK_SET);
    if (sz <= 0) { fclose(f); return NULL; }
    buf = (unsigned char *)malloc((size_t)sz);
    if (buf && fread(buf, 1, (size_t)sz, f) == (size_t)sz) {
        rgba = WebPDecodeRGBA(buf, (size_t)sz, &w, &hh);
        if (rgba) {
            if (w > 0 && hh > 0 && w <= 16384 && hh <= 16384) {
                px = (unsigned int *)malloc((size_t)w * hh * 4);
                if (px) { memcpy(px, rgba, (size_t)w * hh * 4); *ow = w; *oh = hh; }
            }
            WebPFree(rgba);
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
    if (w <= 0 || hh <= 0 || w > 16384 || hh > 16384) { fclose(f); return NULL; }
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
/* PERF (1.6): pre-pack the loaded RGBA8 replacement to the 16-bit target texel format ONCE at load.
 * Halves resident memory (2 vs 4 B/px -> much friendlier to the 1.2M-px/frame sample loop's cache) and
 * removes the per-pixel RGBA->555 pack from the hot loop. 0x0000 = transparent (native texel!=0 rule);
 * opaque black is nudged to 0x0421 (1,1,1) so it still draws instead of vanishing. */
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

/* ---- async replacement loader (5a) -------------------------------------------------------------
 * The webp decode + 16-bit pack of a full background (~1.2Mpx) costs enough to dip a scene-load
 * frame from 60 to ~55 fps when run inline in LoadImage (the render thread). Instead: LoadImage
 * does only a cheap EXISTENCE probe (stat) and queues the region; a single detached loader thread
 * decodes and then PUBLISHES r->px with a release store. Until it lands, the draw path's acquire
 * load sees NULL and samples the native texels -- scene loads sit behind fades, so the one-or-two-
 * frame native window is invisible. LIFO queue: the most recent upload is the current scene.
 * The region registry (s_hdReg/s_hdRegN/live) stays single-threaded (render thread only); the
 * loader touches ONLY r->w/r->h/r->px of already-registered entries, exactly once each.
 * VH_HD_SYNC=1 restores the old inline decode (A/B + debugging). */
static int HdFileExists(const char *dir, unsigned long long h) {
    char path[HD_PATH + 64]; struct stat st;
    snprintf(path, sizeof(path), "%s/%016llx.webp", dir, h);
    if (stat(path, &st) == 0) return 1;
    snprintf(path, sizeof(path), "%s/%016llx.hdi", dir, h);
    return stat(path, &st) == 0;
}
static pthread_mutex_t s_hdLoadMtx = PTHREAD_MUTEX_INITIALIZER;
static pthread_cond_t  s_hdLoadCv  = PTHREAD_COND_INITIALIZER;
static HdRegion *s_hdLoadQ[HD_MAX_REGIONS];
static int s_hdLoadQn, s_hdLoaderUp;
static char s_hdLoaderDir[HD_PATH + 32];        /* pack dir snapshot; never changes mid-run */

static void *HdLoaderMain(void *arg) {
    (void)arg;
    for (;;) {
        HdRegion *r; int w = 0, hh = 0; unsigned int *rgba; unsigned short *px = NULL;
        pthread_mutex_lock(&s_hdLoadMtx);
        while (s_hdLoadQn == 0) pthread_cond_wait(&s_hdLoadCv, &s_hdLoadMtx);
        r = s_hdLoadQ[--s_hdLoadQn];             /* LIFO: newest upload = current scene first */
        pthread_mutex_unlock(&s_hdLoadMtx);
        rgba = HdLoadImage(s_hdLoaderDir, r->hash, &w, &hh);
        if (rgba) { px = HdPack16(rgba, (int)((size_t)w * hh)); free(rgba); }
        if (px) {
            r->w = w; r->h = hh;                 /* dims first, then the pointer gates visibility */
            __atomic_store_n(&r->px, px, __ATOMIC_RELEASE);
            fprintf(stderr, "[HD] REPLACED %016llx rect=(%d,%d) %dx%dw hd=%dx%d (async)\n",
                    r->hash, r->rx, r->ry, r->rw, r->rh, w, hh);
        } else {
            fprintf(stderr, "[HD] async load FAILED %016llx (file vanished or bad?)\n", r->hash);
        }
    }
    return NULL;
}
static void HdLoaderQueue(const char *dir, HdRegion *r) {
    pthread_mutex_lock(&s_hdLoadMtx);
    if (!s_hdLoaderUp) {
        pthread_t th; pthread_attr_t at;
        snprintf(s_hdLoaderDir, sizeof(s_hdLoaderDir), "%s", dir);
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
        rgba = HdLoadImage(dir, r->hash, &w, &hh);
        if (rgba) { r->px = HdPack16(rgba, (int)((size_t)w * hh)); free(rgba);
                    if (r->px) { r->w = w; r->h = hh; } }
    }
}

/* Called from LoadImage after the VRAM write. Many assets (e.g. every full-screen background) reuse the
 * SAME VRAM rect, so regions are found-by-hash but must be matched-by-rect at draw time -- we track which
 * region is LIVE (its content is what's in VRAM at that rect right now) and update it on every upload. */
void HdPack_OnLoad(const RECT *rect, const unsigned short *src) {
    const char *pk = HdPackDir(), *dp = HdDumpDir(); unsigned long long h; HdRegion *r = NULL; int i;
    if (rect->w <= 0 || rect->h <= 0) return;
    /* Hash + register/replace only when the pack (or dump) is active. But the LIVE-region invalidation at
     * the end runs on EVERY upload regardless of the toggle -- so a background uploaded while HD PACK is
     * OFF (pk==NULL) still evicts the stale live region it overwrites. Without this, toggling HD ON mid-
     * scene resampled the PREVIOUS scene's HD image (its region stayed live because its VRAM eviction was
     * skipped); now the current scene correctly shows native until its own background reloads. */
    if (pk || dp) {
        if (dp) { static int mk; if (!mk) { mk = 1; HD_MKDIR(dp); } }
        h = HdHash(src, rect->w * rect->h);
        r = HdFind(h);
        if (!r) {                                 /* first time we see this content -> register it */
            if (s_hdRegN >= HD_MAX_REGIONS) {
                static int warned; if (!warned) { warned = 1; fprintf(stderr, "[HD] region cap %d hit -- raise HD_MAX_REGIONS\n", HD_MAX_REGIONS); }
            } else {
                r = &s_hdReg[s_hdRegN];
                r->hash = h; r->rx = rect->x; r->ry = rect->y; r->rw = rect->w; r->rh = rect->h; r->px = NULL; r->w = r->h = 0; r->dumped = 0; r->live = 0;
                if (dp && getenv("VH_HD_RAW")) {   /* diagnostic: exact hashed source bytes, to reverse the on-disc->VRAM layout offline */
                    char rp[600]; FILE *rf; snprintf(rp, sizeof(rp), "%s/%016llx_%dx%d.raw", dp, h, rect->w, rect->h);
                    rf = fopen(rp, "wb"); if (rf) { fwrite(src, 2, (size_t)rect->w * rect->h, rf); fclose(rf); }
                }
                {
                    int hasRepl = pk && HdFileExists(pk, h);   /* cheap stat on the render thread */
                    if (hasRepl) {
                        static int syncMode = -1;
                        if (syncMode < 0) { const char *e = getenv("VH_HD_SYNC"); syncMode = e && atoi(e) != 0; }
                        s_hdReplaceN++;            /* counts as replaced; draw path skips while px==NULL */
                        if (syncMode) {            /* old inline decode, for A/B + debugging */
                            unsigned int *rgba = HdLoadImage(pk, h, &r->w, &r->h);
                            if (rgba) { r->px = HdPack16(rgba, r->w * r->h); free(rgba); }
                            if (r->px) fprintf(stderr, "[HD] REPLACED %016llx rect=(%d,%d) %dx%dw hd=%dx%d (sync)\n", h, rect->x, rect->y, rect->w, rect->h, r->w, r->h);
                            else { r->w = r->h = 0; hasRepl = 0; s_hdReplaceN--; }
                        } else {
                            HdLoaderQueue(pk, r);  /* decode + publish on the loader thread */
                        }
                    }
                    if (hasRepl || dp) s_hdRegN++; else r = NULL;   /* track: has/awaits a replacement, or dumping */
                }
            }
        }
    }
    /* This upload now occupies (part of) VRAM. Invalidate EVERY region whose rect it OVERLAPS -- their
     * content is no longer intact, so they must not be sampled as a stale replacement. Exact-rect reuse
     * (same backdrop reloaded across scenes) is the common case; the overlap test additionally closes the
     * battle regression where dynamic textures uploaded to sub-rects of a backdrop's VRAM left the backdrop
     * region stale-live -> overlays/effects/tiles sampling that VRAM were wrongly HD-replaced (opaque,
     * wrong content). A partially-overwritten backdrop correctly reverts to native. Runs even with HD off
     * (r==NULL) so a mid-scene toggle never sees a stale live region. */
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
/* Per-TRIANGLE replace resolve (hi-res pass): return the replaced region this textured triangle samples
 * (via its texel-UV bbox -> VRAM footprint), or NULL. The per-pixel HD sampling is then inlined in
 * dda_span with precomputed constants -- no per-pixel scan/divide. Replace mode registers only regions
 * that have a replacement, so this scan is short. */
HdRegion *HdFindTriRegion(int tpage, int uMin, int uMax, int vMin, int vMax) {
    int tpX, tpY, tp, ppw, wx0, wx1, wy0, wy1, i;
    TPageOrigin(tpage, &tpX, &tpY, &tp);
    /* Every HD-replaced asset is an 8bpp background (tp==1). Battle draws (unit sprites, effects, cursor
     * tiles, HP bars) are 4bpp and merely share the same VRAM words as a still-live background region at a
     * DIFFERENT bit depth -- reading them as the background is the regression. They never go through
     * LoadImage (bulk VRAM DMA), so eviction can't catch them; the bit-depth guard does, cleanly. */
    if (tp != 1) return NULL;
    ppw = (tp == 0) ? 4 : (tp == 1) ? 2 : 1;
    wx0 = tpX + uMin / ppw; wx1 = tpX + uMax / ppw; wy0 = tpY + vMin; wy1 = tpY + vMax;
    for (i = 0; i < s_hdRegN; i++) {
        HdRegion *r = &s_hdReg[i];
        /* acquire pairs with the loader thread's release publish of px: once non-NULL, w/h and the
         * pixel data are visible too. Until then the region is skipped -> native texels draw. */
        if (!__atomic_load_n(&r->px, __ATOMIC_ACQUIRE) || !r->live) continue;
        if (wx1 < r->rx || wx0 >= r->rx + r->rw || wy1 < r->ry || wy0 >= r->ry + r->rh) continue;
        return r;
    }
    return NULL;
}
