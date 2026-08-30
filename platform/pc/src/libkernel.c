/* PC backend for PsyQ/kernel.h: BIOS events, root counters, the memory-card file layer, Krom2RawAdd
 * and the BIOS rand(). Saves are real files; RCntCNT1 is wall-clock with a per-caller synthetic AI
 * throttle layered on it. See docs/pc-port/subsystems/kernel.md. */
#include <dirent.h>
#include <errno.h>
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#if defined(_WIN32)
#include <direct.h>          /* _mkdir (MinGW mkdir is 1-arg, no mode) */
#endif
#include "PsyQ/kernel.h"
#include "PsyQ/sys/file.h"
#include "pc_platform.h"     /* PC_GetDeployDir -- resolve the saves folder next to the exe/AppImage */
#include "pc_balance.h"      /* gTacticalMode -- Tactical saves live in their own folder */
#include "pc_lang.h"         /* PC_LangKromGlyph -- pack glyphs for the SJIS path */

#ifndef PATH_MAX
#define PATH_MAX 4096
#endif

#define MAX_EVENTS 16
#define MAX_CARD_FILES 8

typedef struct {
    int used;
    s32 class_;
    s32 spec;
    int enabled;
    int signaled;
} Event;

static Event s_events[MAX_EVENTS];
static int s_numEvents = 0;

s32 OpenEvent(s32 class_, s32 spec, s32 mode, s32 (*func)()) {
    (void)mode;
    (void)func;
    if (s_numEvents >= MAX_EVENTS) return -1;
    int idx = s_numEvents++;
    s_events[idx].used = 1;
    s_events[idx].class_ = class_;
    s_events[idx].spec = spec;
    s_events[idx].enabled = 0;
    s_events[idx].signaled = 0;
    return idx;
}

s32 EnableEvent(s32 event) {
    if (event < 0 || event >= s_numEvents) return -1;
    s_events[event].enabled = 1;
    return 1;
}

s32 TestEvent(s32 event) {
    if (event < 0 || event >= s_numEvents) return 0;
    if (s_events[event].enabled && s_events[event].signaled) {
        s_events[event].signaled = 0; /* auto-clear on a successful test */
        return 1;
    }
    return 0;
}

static void SignalCardEvent(s32 spec) {
    for (int i = 0; i < s_numEvents; i++) {
        if (s_events[i].used && s_events[i].spec == spec &&
            (s_events[i].class_ == HwCARD || s_events[i].class_ == SwCARD)) {
            s_events[i].signaled = 1;
        }
    }
}

/* ---- timer ------------------------------------------------------- */

/* NTSC Timer 1 in HBlank mode: 53,693,181.818 Hz / 3413 video cycles per scanline = 15,732.7 Hz
 * (psx-spx timers + GPU chapters; octoshock's GPUClockRatio / TIMER_ClockHRetrace agree). This
 * constant lands within 0.008% of it. */
#define RCNT1_HZ 15734.0

/* RCntCNT1 is never scaled globally: IsLagging() needs it slowed while graphics.c's sprite decoder
 * is starved by the same factor. AI callers get the synthetic counter below; every other reader sees
 * plain wall-clock. See docs/pc-port/subsystems/kernel.md, "Root counters and VSync timing". */

static struct timespec s_rcntStart[2];

/* AI throttle: a GetRCnt(RCntCNT1) call returning into IsLagging() or one of the seven AI state
 * machines accrues a fixed per-function tick cost per visit instead of wall-clock time, so host
 * speed cannot affect it. See docs/pc-port/subsystems/kernel.md, "The AI throttle". */
#define INSTRUCTIONS_PER_TICK (33868800.0 / RCNT1_HZ)

#define W_CASE_BOUNDARY   25
#define W_OBJF400_CASE2   15
#define W_OBJF401         867
#define W_OBJF404_A       15
#define W_OBJF589_A       15
#define PER_TILE_TEST_COST 12
#define MATCH_FRACTION_ASSUMED 0.10

extern void Objf570_AI_ChooseAction();
extern void Objf400_AI_BuildSpellValueGrid();
extern void Objf401_AI_BuildEnemyProximityGrid();
extern void Objf402_AI_PlanSpellCast();
extern void Objf403_AI_PlanAttack();
extern void Objf404_AI_PlanRetreat();
extern void Objf589_AI_MoveToEscapePoint();
extern s32 IsLagging(void);
extern s16 gMapMinX, gMapMinZ, gMapMaxX, gMapMaxZ;

typedef struct {
    void *start;
    unsigned int size;
} AddrRange;

/* Sizes from `nm -S build/src/ai.o`; the base is a runtime &Func, so the match is PIE-correct. They
 * go stale if any of these 8 functions changes compiled size -- regenerate with:
 *   nm -S --size-sort platform/pc/build/src/ai.o | grep -iE "objf(570|4|589)|islagging" */
static const AddrRange s_aiThrottleRanges[] = {
    {(void *)IsLagging,      0x30},
    {(void *)Objf570_AI_ChooseAction, 0x850},
    {(void *)Objf400_AI_BuildSpellValueGrid, 0x4d3},
    {(void *)Objf401_AI_BuildEnemyProximityGrid, 0x298},
    {(void *)Objf402_AI_PlanSpellCast, 0xb2c},
    {(void *)Objf403_AI_PlanAttack, 0x946},
    {(void *)Objf404_AI_PlanRetreat, 0x465},
    {(void *)Objf589_AI_MoveToEscapePoint, 0x56e},
};
#define NUM_AI_THROTTLE_RANGES (sizeof(s_aiThrottleRanges) / sizeof(s_aiThrottleRanges[0]))

/* Cumulative GetRCnt visits per AI range (table order: 0=IsLagging 1=Objf570 2=Objf400 3=Objf401
 * 4=Objf402 5=Objf403 6=Objf404 7=Objf589). Diffed across an AI phase in the VH_AI_LOG chain CSV to
 * calibrate s_aiThrottleTicks[]; the increment is trivial, so it stays always-on. */
int g_aiVisitCount[NUM_AI_THROTTLE_RANGES] = {0};


/* Returns the AI function-range index containing retAddr (PIE-correct: the range .start is a
 * runtime &Func pointer, so retAddr - start cancels the ASLR base), or -1 if not an AI caller. */
static int AiThrottleRangeIndex(void *retAddr) {
    unsigned i;
    for (i = 0; i < NUM_AI_THROTTLE_RANGES; i++) {
        uintptr_t off = (uintptr_t)retAddr - (uintptr_t)s_aiThrottleRanges[i].start;
        if (off < s_aiThrottleRanges[i].size) {
            return (int)i;
        }
    }
    return -1;
}

/* Per-visit synthetic tick cost per AI range, in s_aiThrottleRanges[] order. Calibrated in 30 Hz game
 * updates as 450 ticks * HW_target_updates / measured_visits_per_scan; a function yields after
 * ceil(450 / cost) visits. See docs/pc-port/subsystems/kernel.md, "The AI throttle". */
static const double s_aiThrottleTicks[NUM_AI_THROTTLE_RANGES] = {
    5.0,     /* 0 IsLagging  -- case-boundary checks, negligible (must not gate) */
    5.0,     /* 1 Objf570    -- orchestrator, few visits */
    30.0,    /* 2 Objf400    -- not in demo; sibling default (= Objf403) */
    285.0,   /* 3 Objf401    -- threat-map; validated 8 updates == HW target 8 */
    42.0,    /* 4 Objf402    -- caster eval (Dark Mage/Eleni/Zohar/Dumas); fitted on a Dark Mage turn:
              *                341 visits + 606 IsLagging + 14 Objf401 against an HW target of ~54 updates.
              *                Only casters use Objf402, so the value is isolated to them. */
    30.0,    /* 5 Objf403    -- move-scoring; linear fit of measured vs HW updates (67->113, 23->44)
              *                => 30 for a ~54-update phase. Melee ~+6%, ranged ~-6% residual (the
              *                per-checkpoint refinement target). */
    30.0,    /* 6 Objf404    -- not in demo; sibling default */
    30.0,    /* 7 Objf589    -- not in demo; sibling default */
};

static double s_aiSyntheticTicks[2];

/* Opt-in (VH_RCNT1_NORMALIZE=1): make RCntCNT1 frame-relative by scaling elapsed time by nominal /
 * previous frame duration (clamped to <= 1.0), so a slow host cannot starve graphics.c's sprite
 * decoder. See docs/pc-port/subsystems/kernel.md, "Frame-relative `RCntCNT1`". */
#define RCNT1_NOMINAL_FRAME_SEC (1.0 / 59.94)
#define RCNT1_MIN_SCALE 0.05 /* cap compensation at 20x, so a pathological stall stays bounded */

static int RCnt1NormalizeEnabled(void) {
    static int cached = -1;
    if (cached < 0) {
        const char *e = getenv("VH_RCNT1_NORMALIZE");
        cached = (e && *e && *e != '0') ? 1 : 0;
        if (cached)
            fprintf(stderr, "libkernel: VH_RCNT1_NORMALIZE=1 -- RCnt1 is frame-relative "
                            "(sprite-decoder budget will not starve on a slow host)\n");
    }
    return cached;
}

static double s_rcnt1Scale = 1.0;

u32 ResetRCnt(s32 which) {
    int idx = which & 1;
    struct timespec now;

    clock_gettime(CLOCK_MONOTONIC, &now);
    if (idx == 1 && RCnt1NormalizeEnabled()) {
        /* Interval since the previous reset = the frame that just ended. */
        double prev = (double)(now.tv_sec - s_rcntStart[idx].tv_sec) +
                      (double)(now.tv_nsec - s_rcntStart[idx].tv_nsec) / 1e9;
        if (prev > RCNT1_NOMINAL_FRAME_SEC && prev < 10.0) {
            s_rcnt1Scale = RCNT1_NOMINAL_FRAME_SEC / prev;
            if (s_rcnt1Scale < RCNT1_MIN_SCALE) s_rcnt1Scale = RCNT1_MIN_SCALE;
        } else {
            s_rcnt1Scale = 1.0;
        }
    }
    s_rcntStart[idx] = now;
    s_aiSyntheticTicks[idx] = 0.0;
    return 0;
}

u32 GetRCnt(s32 which) {
    int idx = which & 1;
    void *retAddr = __builtin_return_address(0);
    struct timespec now;
    double elapsed;
    int aiRange = AiThrottleRangeIndex(retAddr);

    if (aiRange >= 0) {
        g_aiVisitCount[aiRange]++;   /* kept: calibration/validation diagnostic (VH_AI_LOG) */
        s_aiSyntheticTicks[idx] += s_aiThrottleTicks[aiRange];
        return (u32)s_aiSyntheticTicks[idx];
    }

    /* Non-AI callers (sprite decoder, debug FntPrint): honest unscaled wall-clock, unless
     * VH_RCNT1_NORMALIZE makes it frame-relative (RCnt1NormalizeEnabled above). */
    clock_gettime(CLOCK_MONOTONIC, &now);
    elapsed = (double)(now.tv_sec - s_rcntStart[idx].tv_sec) +
              (double)(now.tv_nsec - s_rcntStart[idx].tv_nsec) / 1e9;
    if (idx == 1 && RCnt1NormalizeEnabled()) elapsed *= s_rcnt1Scale;
    return (u32)(elapsed * RCNT1_HZ);
}

/* ---- memory card ---------------------------------------------------- */

static const char *StripDevicePrefix(const unsigned char *path) {
    const char *p = (const char *)path;
    const char *colon = strchr(p, ':');
    return colon ? colon + 1 : p;
}

/* Create dir `p` (portably). Returns 0 if it now exists as a directory, -1 otherwise. */
static int MakeDir(const char *p) {
#if defined(_WIN32)
    if (_mkdir(p) == 0) return 0;
#else
    if (mkdir(p, 0755) == 0) return 0;
#endif
    if (errno == EEXIST) {
        struct stat st;
        if (stat(p, &st) == 0 && S_ISDIR(st.st_mode)) return 0;
    }
    return -1;
}

/* The saves folder, resolved once per mode and cached: <deploy>/saves next to the exe/AppImage if it
 * exists, else an existing cwd-relative "saves", else create <deploy>/saves, else cwd-relative.
 * See docs/pc-port/subsystems/kernel.md, "Memory card and saves". */
static const char *SaveDir(void) {
    static char cached[PATH_MAX + 32];
    static int cachedMode = -1;
    char deploy[PATH_MAX];   /* cached[] below carries +32 headroom for the "/saves*" suffix */
    struct stat st;
    /* Tactical saves live in "saves_tactical/", Normal in "saves/". Re-resolve when the mode changes
     * (only ever at the title menu, so no save op is in flight). */
    const char *sub = gTacticalMode ? "saves_tactical" : "saves";
    if (cachedMode == gTacticalMode) return cached;
    cachedMode = gTacticalMode;
    if (PC_GetDeployDir(deploy, sizeof(deploy))) {
        snprintf(cached, sizeof(cached), "%s/%s", deploy, sub);
        if (stat(cached, &st) == 0 && S_ISDIR(st.st_mode)) return cached;   /* 1 */
        if (stat(sub, &st) == 0 && S_ISDIR(st.st_mode)) {                   /* 2 (legacy, cwd-relative) */
            snprintf(cached, sizeof(cached), "%s", sub);
            return cached;
        }
        if (MakeDir(cached) == 0) return cached;                            /* 3 */
    }
    snprintf(cached, sizeof(cached), "%s", sub);                            /* 4 */
    return cached;
}

static void LocalPath(const unsigned char *cardPath, char *out, size_t outSize) {
    snprintf(out, outSize, "%s/%s", SaveDir(), StripDevicePrefix(cardPath));
}

/* Public accessor (pc_platform.h) so the save-management backend (pc_saves.c) archives/restores from
 * the exact same folder the game reads its card from. */
const char *PC_SaveDir(void) { return SaveDir(); }

void _bu_init(void) {}

/* On hardware these start ASYNC card BIOS operations whose completion interrupt signals a SwCARD/
 * HwCARD event that core/card.c busy-waits on via TestEvent. The virtual card is synchronous, so
 * signal I/O-complete (EvSpIOE) inline -- without it New Game / Load Game spin forever. */
s32 _card_info(s32 port) { (void)port; SignalCardEvent(EvSpIOE); return 0; }
s32 _card_async_load_directory(s32 port) { (void)port; SignalCardEvent(EvSpIOE); return 0; }
s32 _card_clear(s32 port) { (void)port; SignalCardEvent(EvSpIOE); return 0; }

void InitCard(s32 padEnable) {
    (void)padEnable;
    MakeDir(SaveDir());         /* resolve (next to the exe/AppImage) and ensure it exists */
}

s32 StartCard(void) {
    SignalCardEvent(EvSpNEW); /* a (virtual) card is present */
    return 1;
}

s32 FormatDevice(unsigned char *deviceName) {
    (void)deviceName;
    /* Nothing exercises this; a no-op is safer than deleting real save data under the saves folder. */
    return 0;
}

typedef struct {
    FILE *fp;
    int used;
} CardFile;
static CardFile s_cardFiles[MAX_CARD_FILES];

s32 FileOpen(unsigned char *filename, s32 flag) {
    char path[512];
    LocalPath(filename, path, sizeof(path));

    const char *mode = "rb";
    if (flag & O_CREAT) mode = "wb";
    else if (flag & O_WRONLY) mode = "r+b";
    else if (flag & O_RDONLY) mode = "rb";

    /* A creating open needs the saves folder: InitCard makes it at startup, but it can be deleted
     * mid-session (or the deploy dir may only become writable later). Re-create it so an in-battle
     * save cannot fail on a missing directory; a no-op when it exists. */
    if (flag & O_CREAT) MakeDir(SaveDir());

    for (int i = 0; i < MAX_CARD_FILES; i++) {
        if (!s_cardFiles[i].used) {
            FILE *fp = fopen(path, mode);
            if (!fp) {
                SignalCardEvent(EvSpIOE);
                return -1;
            }
            s_cardFiles[i].fp = fp;
            s_cardFiles[i].used = 1;
            return i;
        }
    }
    return -1;
}

s32 FileClose(s32 fd) {
    if (fd < 0 || fd >= MAX_CARD_FILES || !s_cardFiles[fd].used) return -1;
    fclose(s_cardFiles[fd].fp);
    s_cardFiles[fd].used = 0;
    return 0;
}

s32 FileRead(s32 fd, void *buf, s32 size) {
    if (fd < 0 || fd >= MAX_CARD_FILES || !s_cardFiles[fd].used) return -1;
    return (s32)fread(buf, 1, (size_t)size, s_cardFiles[fd].fp);
}

s32 FileWrite(s32 fd, void *buf, s32 size) {
    if (fd < 0 || fd >= MAX_CARD_FILES || !s_cardFiles[fd].used) return -1;
    return (s32)fwrite(buf, 1, (size_t)size, s_cardFiles[fd].fp);
}

s32 FileSeek(s32 fd, s32 offset, s32 mode) {
    if (fd < 0 || fd >= MAX_CARD_FILES || !s_cardFiles[fd].used) return -1;
    fseek(s_cardFiles[fd].fp, offset, mode);
    return (s32)ftell(s_cardFiles[fd].fp);
}

/* ---- directory enumeration ------------------------------------------ */

static DIR *s_scanDir = NULL;
struct DIRENTRY *nextfile(struct DIRENTRY *entry);

struct DIRENTRY *firstfile(unsigned char *path, struct DIRENTRY *entry) {
    (void)path;
    if (s_scanDir) closedir(s_scanDir);
    s_scanDir = opendir(SaveDir());
    if (!s_scanDir) return NULL;
    return nextfile(entry);
}

struct DIRENTRY *nextfile(struct DIRENTRY *entry) {
    if (!s_scanDir) return NULL;
    struct dirent *de;
    while ((de = readdir(s_scanDir)) != NULL) {
        if (de->d_name[0] == '.') continue;
        char path[PATH_MAX];
        snprintf(path, sizeof(path), "%s/%.128s", SaveDir(), de->d_name);
        struct stat st;
        if (stat(path, &st) != 0) continue;
        /* Regular files ONLY: a real card cannot hold directories, and a subfolder returned as a
         * card file makes Card_CountFreeBlocks read garbage as its header block count ("no free
         * blocks" on a near-empty card). */
        if (!S_ISREG(st.st_mode)) continue;
        strncpy(entry->name, de->d_name, sizeof(entry->name) - 1);
        entry->name[sizeof(entry->name) - 1] = '\0';
        entry->size = (int)st.st_size;
        entry->attr = 0;
        entry->next = NULL;
        entry->head = 0;
        return entry;
    }
    closedir(s_scanDir);
    s_scanDir = NULL;
    return NULL;
}

/* PS1 BIOS kanji ROM glyphs (pc_kanji_font.c, generated from PsyQ KROMDAT.BIN by
 * tools/gen_kanji_font.py). 30 bytes per glyph. Size is region-dependent (US: the audited
 * 209-glyph alphanumeric subset; JP: all 3489 charset-2+3 glyphs), hence unsized here. */
extern const unsigned char pc_kanji_charset2[];

/* Map a full-width Shift-JIS code to its glyph index in charset 2. The BIOS packs the charset
 * compacted (undefined codes skipped), so the anchors are recovered empirically (gen_kanji_font.py).
 * Unmapped codes return -1 (blank). See docs/pc-port/subsystems/kernel.md, "Kanji glyph lookup". */
#ifdef VH_REGION_JP
/* JP: the full BIOS kanji ROM layout (psx-spx kernel chapter, "BIOS Character Sets"). Charset 2 =
 * glyphs 0..523, SJIS 0x8140..0x84BE (kuten rows 1-8, COMPACTED); charset 3 = glyphs 524..3488, SJIS
 * 0x889F..0x9872 (rows 16-47, 2965 level-1 kanji, dense). Others return -1, as the BIOS does. */
static s32 sjis_to_krom_glyph(u32 sjis) {
    /* JIS X 0208 defined kuten spans for rows 1-8, with cumulative charset-2 bases. */
    static const struct { unsigned char row, first, last; unsigned short base; } k2[] = {
        {1,  1, 94,   0},
        {2,  1, 14,  94}, {2, 26, 33, 108}, {2, 42, 48, 116},
        {2, 60, 74, 123}, {2, 82, 89, 138}, {2, 94, 94, 146},
        {3, 16, 25, 147}, {3, 33, 58, 157}, {3, 65, 90, 183},
        {4,  1, 83, 209},
        {5,  1, 86, 292},
        {6,  1, 24, 378}, {6, 33, 56, 402},
        {7,  1, 33, 426}, {7, 49, 81, 459},
        {8,  1, 32, 492},
    };
    u32 hi = (sjis >> 8) & 0xff, lo = sjis & 0xff;
    u32 row, cell;
    unsigned i;
    if (hi < 0x81 || hi > 0x98 || lo < 0x40 || lo == 0x7f || lo > 0xfc) return -1;
    row = (hi - 0x81) * 2 + 1;                 /* kuten row (first of the byte's pair) */
    cell = lo - 0x40 - (lo > 0x7f ? 1 : 0);    /* 0..187 across the two rows */
    if (cell >= 94) { row += 1; cell -= 94; }
    cell += 1;                                 /* 1-based kuten position */
    if (row <= 8) {
        for (i = 0; i < sizeof(k2) / sizeof(k2[0]); i++) {
            if (k2[i].row == row && cell >= k2[i].first && cell <= k2[i].last)
                return (s32)(k2[i].base + (cell - k2[i].first));
        }
        return -1;
    }
    if (row >= 16 && row <= 47) {
        if (row == 47 && cell > 51) return -1; /* level 1 ends at 47-51 (SJIS 0x9872) */
        return (s32)(524 + (row - 16) * 94 + (cell - 1));
    }
    return -1;
}
#else
static s32 sjis_to_krom_glyph(u32 sjis) {
    /* Kuten row 1 (0x8140-0x817C) is packed LINEARLY in charset 2: the recovered anchors (space=0,
     * period=4, plus=59, minus=60) all satisfy index == sjis - 0x8140. Serving the whole span gives
     * the langpack subtitle renderer its punctuation (comma, quotes, ?, !) -- keep it. */
    if (sjis >= 0x8140 && sjis <= 0x817c) return (s32)(sjis - 0x8140);
    if (sjis == 0x8194) return 65;                                /* #      */
    if (sjis >= 0x824f && sjis <= 0x8258) return 147 + (sjis - 0x824f); /* 0-9 */
    if (sjis >= 0x8260 && sjis <= 0x8279) return 157 + (sjis - 0x8260); /* A-Z */
    if (sjis >= 0x8281 && sjis <= 0x829a) return 183 + (sjis - 0x8281); /* a-z */
    return -1;
}
#endif /* VH_REGION_JP */

/* BIOS call B(51h): a pointer to the 30-byte glyph bitmap for a Shift-JIS code, or -1 -- what
 * core/text.c's DrawSjisGlyph() expects. Returns void *, not s32: both callers dereference it, and
 * an s32 return truncates the address under LP64 (first battle menu crashes). Right at both widths. */
void *Krom2RawAdd(s32 sjisCode) {
    s32 idx;
    /* Language pack (pc_lang_font.c): pack-assigned 2-byte codes (0x8440+, a range the retail map
     * never answers) resolve to pack-supplied 16x15 glyphs -- accented item names ride the existing
     * DrawSjisGlyph path, anti-aliasing included. NULL without a pack; retail lookup unchanged. */
    { const void *g = PC_LangKromGlyph((unsigned)sjisCode & 0xffff);
      if (g) return (void *)g; }
    idx = sjis_to_krom_glyph((u32)sjisCode & 0xffff);
    if (idx < 0) {
        return (void *)(intptr_t)-1;
    }
    return (void *)&pc_kanji_charset2[idx * 30];
}

/* BIOS misc call A(2Fh), not libc: x = x*41C64E6Dh + 3039h; return (x>>16) & 7FFFh (psx-spx kernel
 * chapter). Game logic randomizes per-demo unit teams through it, so the exact stream matters. The
 * game never calls srand(); hardware's live seed starts at 0 (BizHawk RAM 0x80009010 trace). */
static u32 s_randSeed = 0;

s32 rand(void) {
    s_randSeed = s_randSeed * 0x41c64e6dU + 0x3039U;
    return (s32)((s_randSeed >> 16) & 0x7fffU);
}

void srand(u32 seed) {
    s_randSeed = seed;
}

/* Debug accessor for pc_diag.c's VH_RAND_LOG trace: observe s_randSeed without changing its linkage,
 * so the PRNG state can be diffed against hardware's (RAM 0x80009010). */
u32 GetRandSeedForDebug(void) {
    return s_randSeed;
}
