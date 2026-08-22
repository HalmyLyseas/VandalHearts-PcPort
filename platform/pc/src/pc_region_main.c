/* pc_region_main.c -- the unified binary's real main() (P5, exchange/104).
 *
 * Only `make unified` compiles this file. The unified executable carries BOTH region cores as
 * prefix-renamed blobs (us_* / jp_*): each blob is that region's complete, individually-validated
 * object set (game code + generated/reconstructed data + region-compiled backends), partial-linked
 * and renamed, so only one of them ever executes per process. This file is the thin shared layer:
 *
 *   1. GPU-trace replay hook (regression harness) -- must run before anything else, no disc.
 *   2. Config (vandalhearts.ini -> env) and the PSX RAM/scratchpad arena reservations, called
 *      ONCE via the US blob's copies (region-neutral code; the blobs' own constructors for these
 *      are compiled out under VH_UNIFIED -- see pc_bootstrap.c). Data-init constructors (the
 *      generated_data memcpys, pointer-table fixups) still run normally in BOTH blobs at load;
 *      they touch only their own renamed globals.
 *   3. Disc discovery + region classification (self-contained here: both regions' boot layouts).
 *   4. Dispatch: <region>_PC_BootstrapRegion(path) then <region>_main().
 *
 * Region selection: VH_REGION=auto|us|jp (env or vandalhearts.ini, like every other key).
 * auto = whichever region's disc is found; both present -> US (the established default) with a
 * printed override hint. VH_DISC_IMAGE still overrides the PATH (and is classified, not trusted).
 */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

/* US-blob entry points (region-neutral pieces + the US core). */
extern void us_PC_LoadIniConfig(void);
extern void us_PC_ReservePsxRam(void);
extern int  us_PC_GetDeployDir(char *out, size_t outSize);
extern int  us_PC_GpuReplayTrace(const char *path);
extern void us_PC_BootstrapRegion(const char *discPath);
extern void us_main(void);
/* JP core. */
extern void jp_PC_BootstrapRegion(const char *discPath);
extern void jp_main(void);

#define RAW_SECTOR 2352
#define DATA_OFF 24
#define SECTOR_DATA 2048
#define EXE_LOAD_VRAM 0x80010000u
#define EXE_HDR 0x800u

typedef enum { R_NONE = 0, R_US, R_JP } Region;

/* Read the 14-byte memory-card id embedded in a region's boot exe (pinned at its boot LBA in
 * contiguous ISO sectors) -- same math as libcd.c's classifier, standalone here because the
 * blobs' copies are renamed per region and this scan runs before either region is chosen. */
static int CardIdAt(FILE *f, long bootLba, unsigned long cardVram, char id[14]) {
    unsigned long imgOff = (cardVram - EXE_LOAD_VRAM) + EXE_HDR;
    long lba = bootLba + (long)(imgOff / SECTOR_DATA);
    long raw = lba * RAW_SECTOR + DATA_OFF + (long)(imgOff % SECTOR_DATA);
    if (fseek(f, raw, SEEK_SET) != 0) return 0;
    return fread(id, 1, 14, f) == 14;
}

static int BootSigAt(FILE *f, long lba) {
    unsigned char magic[8];
    if (fseek(f, lba * RAW_SECTOR + DATA_OFF, SEEK_SET) != 0) return 0;
    if (fread(magic, 1, 8, f) != 8) return 0;
    return memcmp(magic, "PS-X EXE", 8) == 0;
}

/* idOut (16 bytes, may be NULL): the disc's release id, e.g. "SLUS-00447" / "SCPS-45183" /
 * "SLPM-86007" -- chars [2..11] of the card id. The overlay's DISC row displays it. */
static Region ClassifyDisc(const char *path, char *idOut) {
    FILE *f = fopen(path, "rb");
    char id[14];
    Region r = R_NONE;
    if (!f) return R_NONE;
    if (BootSigAt(f, 23) && CardIdAt(f, 23, 0x800f5551u, id) &&
        (memcmp(id, "BASLUS-00447VH", 14) == 0 || memcmp(id, "BISCPS-45183VH", 14) == 0))
        r = R_US;
    else if (BootSigAt(f, 15200) && CardIdAt(f, 15200, 0x800f76a9u, id) &&
             memcmp(id, "BISLPM-86007VH", 14) == 0)
        r = R_JP;
    fclose(f);
    if (r != R_NONE && idOut) { memcpy(idOut, id + 2, 10); idOut[10] = '\0'; }
    return r;
}

static int HasBinExt(const char *name) {
    size_t n = strlen(name);
    return n >= 4 && name[n - 4] == '.' && (name[n - 3] | 0x20) == 'b' &&
           (name[n - 2] | 0x20) == 'i' && (name[n - 1] | 0x20) == 'n';
}

/* The three release ids the classifier accepts, as inventory slots. SLUS and SCPS are the same
 * master (same region core) but DISTINCT discs -- the overlay's DISC row lists each found disc
 * individually, so they get separate slots, not one shared "US" slot. */
enum { D_SLUS = 0, D_SCPS, D_SLPM, D_CT };
static const char *const kDiscIds[D_CT] = { "SLUS-00447", "SCPS-45183", "SLPM-86007" };
static char s_discPath[D_CT][4200];

static int DiscSlot(const char *id) {
    int i;
    for (i = 0; i < D_CT; i++)
        if (strcmp(id, kDiscIds[i]) == 0) return i;
    return -1;   /* unreachable: ClassifyDisc only accepts these three card ids */
}

/* Scan a directory's *.bin files; record the first disc found for each release id. */
static void ScanDir(const char *dir) {
    DIR *d = opendir(dir);
    struct dirent *e;
    if (!d) return;
    while ((e = readdir(d)) != NULL) {
        char full[4200], id[16];
        int slot;
        if (!HasBinExt(e->d_name)) continue;
        snprintf(full, sizeof(full), "%s/%s", dir, e->d_name);
        if (ClassifyDisc(full, id) == R_NONE) continue;
        slot = DiscSlot(id);
        if (slot >= 0 && !s_discPath[slot][0])
            snprintf(s_discPath[slot], sizeof(s_discPath[slot]), "%s", full);
    }
    closedir(d);
}

#if defined(_WIN32)
#define PC_Setenv(k, v) _putenv_s((k), (v))
#else
#define PC_Setenv(k, v) setenv((k), (v), 1)
#endif

int main(void) {
    const char *want;
    Region pick = R_NONE;
    const char *pickPath = NULL;
    int pickSlot = -1;

    /* 1. Regression-harness replay: no disc, no region, exits when done. */
    { const char *rp = getenv("VH_GPU_REPLAY");
      if (rp && *rp) exit(us_PC_GpuReplayTrace(rp)); }

    /* 2. Once-per-process, region-neutral init (the blobs' constructors for these are
     *    compiled out under VH_UNIFIED). Config first so VH_REGION/VH_DISC_IMAGE from the
     *    ini are visible to the scan below -- same relative order as the constructor
     *    priorities it replaces. */
    us_PC_LoadIniConfig();
    us_PC_ReservePsxRam();

    /* 3. Find candidate discs. VH_DISC_IMAGE pins the path but is still classified. */
    { const char *forced = getenv("VH_DISC_IMAGE");
      if (forced && *forced) {
          char id[16];
          int slot = (ClassifyDisc(forced, id) != R_NONE) ? DiscSlot(id) : -1;
          if (slot >= 0) snprintf(s_discPath[slot], sizeof(s_discPath[slot]), "%s", forced);
          else fprintf(stderr, "PC_RegionMain: VH_DISC_IMAGE '%s' is not a recognized "
                               "Vandal Hearts disc\n", forced);
      } else {
          char dir[4096], sub[4200];
          if (us_PC_GetDeployDir(dir, sizeof(dir))) {
              snprintf(sub, sizeof(sub), "%s/game", dir);
              ScanDir(sub);
              ScanDir(dir);
              /* dev-repo layout: external/{game,alt} three levels up from platform/pc/build* */
              snprintf(sub, sizeof(sub), "%s/../../../external/game", dir);
              ScanDir(sub);
              snprintf(sub, sizeof(sub), "%s/../../../external/alt", dir);
              ScanDir(sub);
          }
      }
    }

    /* Publish the disc inventory to the blob about to run: the in-game options overlay's DISC row
     * lists every disc found (one entry per release id) and persists the pick as VH_REGION +
     * VH_DISC_ID in the ini. Launcher-owned, not user config -- always overwritten. */
    if (s_discPath[D_SLUS][0]) PC_Setenv("VH_DISC_ID_US",   kDiscIds[D_SLUS]);
    if (s_discPath[D_SCPS][0]) PC_Setenv("VH_DISC_ID_ASIA", kDiscIds[D_SCPS]);
    if (s_discPath[D_SLPM][0]) PC_Setenv("VH_DISC_ID_JP",   kDiscIds[D_SLPM]);

    /* 4. Select. Within the US family (SLUS/SCPS -- same master, same core), VH_DISC_ID picks the
     * exact disc (the overlay's DISC row persists it); otherwise SLUS is the canonical default. */
    { const char *wantId = getenv("VH_DISC_ID");
      int usSlot = -1;
      if (wantId && strcmp(wantId, kDiscIds[D_SCPS]) == 0 && s_discPath[D_SCPS][0]) usSlot = D_SCPS;
      else if (s_discPath[D_SLUS][0]) usSlot = D_SLUS;
      else if (s_discPath[D_SCPS][0]) usSlot = D_SCPS;

      want = getenv("VH_REGION");
      if (want && (want[0] | 0x20) == 'u') {
          if (usSlot >= 0) { pick = R_US; pickSlot = usSlot; }
      } else if (want && (want[0] | 0x20) == 'j') {
          if (s_discPath[D_SLPM][0]) { pick = R_JP; pickSlot = D_SLPM; }
      } else { /* auto */
          if (usSlot >= 0 && s_discPath[D_SLPM][0]) {
              fprintf(stderr, "PC_RegionMain: both regions' discs found -- using USA/Asia "
                              "('%s'). Set VH_REGION=jp (vandalhearts.ini or env) or switch "
                              "via the in-game OPTIONS > DISC row for the Japanese game.\n",
                              s_discPath[usSlot]);
              pick = R_US; pickSlot = usSlot;
          } else if (usSlot >= 0)            { pick = R_US; pickSlot = usSlot; }
          else if (s_discPath[D_SLPM][0])    { pick = R_JP; pickSlot = D_SLPM; }
      }
    }
    if (pickSlot >= 0) {
        pickPath = s_discPath[pickSlot];
        PC_Setenv("VH_DISC_BOOTED", kDiscIds[pickSlot]);   /* the overlay's no-'*' baseline */
    }

    if (pick == R_NONE) {
        fprintf(stderr,
            "\n*** Vandal Hearts - no usable disc image found ***\n"
            "%s\n"
            "Put your Vandal Hearts .bin -- USA (SLUS-00447), Asia (SCPS-45183) or "
            "Japan (SLPM-86007) -- in a \"game\" folder next to the executable (or right "
            "beside it), or set VH_DISC_IMAGE in vandalhearts.ini.\n",
            (want && (want[0]|0x20) == 'j') ? "VH_REGION=jp is set but no Japanese disc was found." :
            (want && (want[0]|0x20) == 'u') ? "VH_REGION=us is set but no USA/Asia disc was found." :
                                              "No recognized disc image was found.");
        exit(1);
    }

    fprintf(stderr, "PC_RegionMain: region %s ('%s')\n", pick == R_JP ? "JAPAN" : "USA/ASIA",
            pickPath);
    if (pick == R_JP) { jp_PC_BootstrapRegion(pickPath); jp_main(); }
    else              { us_PC_BootstrapRegion(pickPath); us_main(); }
    return 0;
}
