/* In-game options overlay: menu model + state machine (MAIN settings list, SAVES archive browser,
 * CONFIRM prompt, DETAIL slot view). No "Close" item: the SELECT+START chord is the sole show/hide
 * (a face-button close would leak the held press). See docs/gameplay-additions.md, "Overlay internals". */
#include "pc_overlay.h"
#include "pc_platform.h"   /* PC_SaveIniConfig, PC_GpuSetScale/SetFullscreen, g_vhScale/g_vhFullscreen */
#include "pc_saves.h"      /* archive list + backup/restore/delete */
#include "pc_balance.h"    /* gTacticalMode, PC_SyncBalance, PC_AtTitleMenu */
/* Game headers for the RETURN TO TITLE action: the menu MODEL stays game-agnostic, but this one
 * action rewrites engine state directly. */
#include "common.h"        /* s16, gPlayerControlSuppressed */
#include "state.h"         /* gState, STATE_TITLE_SCREEN */
#include "cd_files.h"      /* MOV_LOGO_USA_STR, MOV_TITLE_WS_STR */
#include "audio.h"         /* AUDIO_CMD_STOP_ALL */
#include "battle.h"        /* gIsEnemyTurn */
#include <stddef.h>
#include <stdlib.h>        /* free */
#include <stdio.h>         /* snprintf */
#include <string.h>        /* strncpy */

/* Live settings the overlay reads/writes. Camera invert (libetc.c) is read every frame by the pad
 * mapping. Video (pc_gpu_window.c) is applied via the apply() callback below. */
extern int g_camInvertX;
extern int g_camInvertY;
extern int g_btnLabels;   /* overlay button-label style (0=PLAYSTATION, 1=XBOX) */
extern int g_vhHdPack;    /* HD PACK toggle (pc_gpu_window.c); libgpu's HdDetect() auto-ONs it */
extern int PC_HdPackAvailable(void);   /* libgpu.c: 1 if a valid HD pack is installed beside the exe */
extern const char *PC_HdPackStatusShort(void);   /* why the pack is unusable, for the value column */

/* CHOICE value labels for BUTTON LABELS, indexed by g_btnLabels. */
static const char *const s_btnLabelText[] = { "PLAYSTATION", "XBOX" };
/* INTERNAL RES labels, indexed by value (1..4). Index 0 unused (minv is 1). X1 reads as OFF. */
static const char *const s_internalResText[] = { "", "OFF", "X2", "X3", "X4" };

/* Applying the Tactical Mode toggle: set the mode and re-sync the balance patch (idempotent). The
 * save folder follows automatically (PC_SaveDir reads gTacticalMode). */
static void apply_tactical(int nv) { gTacticalMode = nv; PC_SyncBalance(); }

/* HD PACK coupling: HD renders only at INTERNAL RES > 1 and, windowed, is only VISIBLE at WINDOW
 * SCALE > 1, so enabling HD bumps both to at least 2; dropping either back to 1 disables HD again
 * (reconcileHdPack, run from setValue). s_inHdApply suppresses that reconcile during the bump. */
static int s_inHdApply = 0;
static void setItemByValue(int *value, int nv);   /* fwd */
extern int g_vhInternalScale;                      /* video (pc_gpu_window.c); also used in the item table */
static void apply_hdpack(int nv) {
    g_vhHdPack = nv;
    if (nv) {
        s_inHdApply = 1;
        if (g_vhInternalScale < 2)            setItemByValue(&g_vhInternalScale, 2);
        if (!g_vhFullscreen && g_vhScale < 2) setItemByValue(&g_vhScale, 2);
        s_inHdApply = 0;
    }
}

/* ---- MAIN screen: data-driven settings list ---------------------------------------------------- */

enum { OVL_TOGGLE, OVL_ACTION, OVL_CHOICE };

typedef struct {
    const char *label;
    int         kind;
    int        *value;                      /* TOGGLE/CHOICE: the live setting            */
    const char *iniSection, *iniKey;        /* TOGGLE/CHOICE: where to persist it         */
    const char *offText, *onText;           /* TOGGLE: value labels (0 / 1)               */
    int         minv, maxv, step;           /* CHOICE: inclusive range + increment        */
    const char *prefix;                     /* CHOICE: display prefix, e.g. "X" -> "X3"    */
    void      (*apply)(int);                /* TOGGLE/CHOICE: apply the new value; may be NULL */
    void      (*action)(void);              /* OVL_ACTION: run on activate                */
    int       (*disabled)(void);            /* optional: 1 => greyed (visual only); may be NULL */
    int       (*locked)(void);              /* optional: 1 => read-only (blocks edit/activate); may be NULL */
    const char *const *choiceText;          /* CHOICE: value labels indexed by value (else numeric); NULL ok */
} Item;

/* Window scale and fullscreen are mutually-exclusive display modes; the INACTIVE one is greyed. */
static int dis_whenFullscreen(void) { return g_vhFullscreen; }
static int dis_whenWindowed(void)   { return !g_vhFullscreen; }

/* HD PACK is greyed AND locked (read-only) unless a valid pack is auto-detected beside the exe. */
static int dis_noHdPack(void)       { return !PC_HdPackAvailable(); }

/* ---- LANGUAGE picklist: every valid pack under <deploy>/langpacks/ by manifest name; OFF = retail.
 * NO auto-select (a picklist is not a binary state): nothing loads until chosen here or via VH_LANG
 * in the ini, which this row persists. A pack applies at BOOT, so a pending change shows '*'. ---- */
#include "pc_lang.h"
#define LANG_MAX 12
static int  g_langSel;                          /* 0 = OFF, 1..N = s_langFolders[sel-1] */
static int  s_langCount;
static char s_langFolders[LANG_MAX][64];
static char s_langNames[LANG_MAX][64];
static char s_langLabelBuf[LANG_MAX + 1][32];   /* uppercased, ascii-folded, '*' when pending */
static const char *s_langChoice[LANG_MAX + 1 + 1];

/* OSD-safe fold: the 5x7 font is caps-only ASCII, but a pack's manifest name may carry accents, so
 * Latin-1-range codepoints fold to their base letter ("FRANCAIS", not "FRANAIS"). This is the ONLY
 * place pack text reaches the overlay; its own labels stay English (see PC_OverlayTitle). */
static void langFoldName(const char *in, char *out, int cap) {
    static const char *fold = "AAAAAAACEEEEIIIIDNOOOOO*OUUUUY__aaaaaaaceeeeiiiidnooooo/ouuuuy_y";
    int o = 0;
    while (*in && o < cap - 2) {
        unsigned char c = (unsigned char)*in++;
        if (c < 0x80) { out[o++] = (char)((c >= 'a' && c <= 'z') ? c - 32 : c); continue; }
        if ((c == 0xC3 || c == 0xC2) && *in) {              /* 2-byte UTF-8, Latin-1 range */
            unsigned cp = ((c & 0x1F) << 6) | ((unsigned char)*in & 0x3F);
            in++;
            if (cp >= 0xC0 && cp <= 0xFF) {
                char f = fold[cp - 0xC0];
                out[o++] = (char)((f >= 'a' && f <= 'z') ? f - 32 : f);
            }
            continue;
        }
        /* anything else (3/4-byte, stray) is skipped */
    }
    out[o] = '\0';
}

static void langRefreshLabels(void) {
    int i, bootSel = 0;
    for (i = 0; i < s_langCount; i++)
        if (strcmp(s_langFolders[i], PC_LangBootFolder()) == 0) bootSel = i + 1;
    snprintf(s_langLabelBuf[0], sizeof s_langLabelBuf[0], "OFF%s",
             (g_langSel != bootSel && g_langSel == 0) ? " *" : "");
    s_langChoice[0] = s_langLabelBuf[0];
    for (i = 0; i < s_langCount; i++) {
        char folded[28];
        langFoldName(s_langNames[i], folded, sizeof folded);
        snprintf(s_langLabelBuf[i + 1], sizeof s_langLabelBuf[i + 1], "%s%s", folded,
                 (g_langSel == i + 1 && g_langSel != bootSel) ? " *" : "");
        s_langChoice[i + 1] = s_langLabelBuf[i + 1];
    }
    for (i = s_langCount + 1; i <= LANG_MAX; i++) s_langChoice[i] = "";
}

static void langScan(void) {
    char pending[64];
    int i;
    /* keep the user's pending choice across rescans BY FOLDER (indices may shift) */
    snprintf(pending, sizeof pending, "%s",
             (g_langSel > 0 && g_langSel <= s_langCount) ? s_langFolders[g_langSel - 1]
                                                         : (s_langCount ? "" : PC_LangBootFolder()));
    s_langCount = PC_LangListPacks(s_langFolders, s_langNames, LANG_MAX);
    g_langSel = 0;
    for (i = 0; i < s_langCount; i++)
        if (pending[0] && strcmp(s_langFolders[i], pending) == 0) g_langSel = i + 1;
    langRefreshLabels();
}

static void apply_language(int v) {
    if (v < 0) v = 0;                            /* safety only; the generic clamp already bounds v */
    if (v > s_langCount) v = s_langCount;
    g_langSel = v;
    PC_SaveIniConfig("language", "VH_LANG", v ? s_langFolders[v - 1] : "");
    langRefreshLabels();
}

/* ---- DISC picklist: every disc the unified launcher found, ONE ENTRY PER RELEASE ID (the
 * VH_DISC_ID_US/ASIA/JP env from pc_region_main.c). Selecting another persists VH_REGION + VH_DISC_ID;
 * the running blob IS its region, so ' *' marks a pending restart (baseline VH_DISC_BOOTED). ------ */
#define DISC_MAX 3
static int  g_discSel;                          /* index into the parallel arrays below */
static int  s_discCount;
static int  s_discBootSel;                      /* the booted disc's entry */
static int  s_discScanned;                      /* keep a pending choice across reopens */
static char s_discTag[DISC_MAX][4];             /* "us" / "jp" -- the VH_REGION value */
static char s_discId[DISC_MAX][16];
static char s_discLabelBuf[DISC_MAX][20];       /* id + " *" when pending */
static const char *s_discChoice[DISC_MAX + 1];

static void discRefreshLabels(void) {
    int i;
    for (i = 0; i < s_discCount; i++) {
        /* Precision cap makes the worst case (15 + 2 + NUL = 18) provably fit the 20-byte
         * buffer, which is what silences -Wformat-truncation (it reasons from the format
         * string alone) -- same idiom as pc_lang.c's LangPackDir. */
        snprintf(s_discLabelBuf[i], sizeof s_discLabelBuf[i], "%.15s%s", s_discId[i],
                 (g_discSel == i && g_discSel != s_discBootSel) ? " *" : "");
        s_discChoice[i] = s_discLabelBuf[i];
    }
    for (i = s_discCount; i <= DISC_MAX; i++) s_discChoice[i] = "";
}

static void discScan(void) {
    static const struct { const char *env; const char *tag; } kSlots[DISC_MAX] = {
        { "VH_DISC_ID_US",   "us" },
        { "VH_DISC_ID_ASIA", "us" },
        { "VH_DISC_ID_JP",   "jp" },
    };
    const char *booted = getenv("VH_DISC_BOOTED");
#ifdef VH_REGION_JP
    const char *ownTag = "jp", *ownId = "SLPM-86007";
#else
    const char *ownTag = "us", *ownId = "SLUS-00447";
#endif
    int i;
    s_discCount = 0;
    for (i = 0; i < DISC_MAX; i++) {
        const char *id = getenv(kSlots[i].env);
        if (!id || !*id) continue;
        snprintf(s_discTag[s_discCount], 4, "%s", kSlots[i].tag);
        snprintf(s_discId[s_discCount], 16, "%s", id);
        s_discCount++;
    }
    if (s_discCount == 0) {   /* per-region dev build: no launcher inventory */
        snprintf(s_discTag[0], 4, "%s", ownTag);
        snprintf(s_discId[0], 16, "%s", ownId);
        s_discCount = 1;
    }
    /* Baseline entry: the exact disc that booted; fall back to the first entry of our own region
     * (per-region builds, or an unexpectedly absent VH_DISC_BOOTED). */
    s_discBootSel = 0;
    for (i = 0; i < s_discCount; i++)
        if (strcmp(s_discTag[i], ownTag) == 0) { s_discBootSel = i; break; }
    if (booted && *booted)
        for (i = 0; i < s_discCount; i++)
            if (strcmp(s_discId[i], booted) == 0) { s_discBootSel = i; break; }
    if (!s_discScanned || g_discSel >= s_discCount) { g_discSel = s_discBootSel; s_discScanned = 1; }
    discRefreshLabels();
}

static void apply_disc(int v) {
    if (v < 0) v = 0;                            /* safety only; the generic clamp already bounds v */
    if (v >= s_discCount) v = s_discCount - 1;
    g_discSel = v;
    PC_SaveIniConfig("region", "VH_REGION", s_discTag[v]);
    PC_SaveIniConfig("region", "VH_DISC_ID", s_discId[v]);   /* picks WITHIN the US family */
    discRefreshLabels();
}

static int dis_oneDisc(void)        { return s_discCount < 2; }

/* LANGUAGE is greyed+locked when the SELECTED disc is the Japanese game (running it, or a pending
 * restart into it) -- langpacks are US-only; the JP core links pc_lang_stub.c, which also reports
 * 0 installed packs, so JP greys through both terms -- and when no pack is installed. */
static int dis_langRow(void) {
    return (s_discCount > 0 && strcmp(s_discTag[g_discSel], "jp") == 0) || s_langCount == 0;
}

/* Tactical Mode is editable ONLY at the main title menu (a run's mode is fixed). Greyed
 * (read-only) during a run, where it just reflects the current run's mode. */
static int dis_notAtTitle(void)     { return !PC_AtTitleMenu(); }
/* RETURN TO TITLE is the opposite -- only meaningful during a run, so greyed AT the title menu. */
static int dis_atTitle(void)        { return PC_AtTitleMenu(); }

static void act_enterSaves(void);           /* SAVE MANAGEMENT -> the saves screen */
static void act_returnToTitle(void);        /* RETURN TO TITLE -> confirm -> PC_ReturnToTitle */

/* A global's address is a compile-time constant, so this static table with &g_* is valid. (Not
 * const: the LANGUAGE row's maxv is set at scan time to the number of installed packs, so the
 * generic clamp/cycle logic treats the picklist exactly like every other CHOICE row.) */
static Item s_items[] = {
    /* Tactical Mode is greyed AND locked off-title (a run's mode is fixed). Window Scale / Fullscreen
     * are greyed to show the inactive display mode, but stay INTERACTIVE -- toggling the greyed one is
     * how you switch to it (locked = NULL). Return to Title is greyed AND locked at the title. */
    { "TACTICAL MODE",   OVL_TOGGLE, &gTacticalMode,  "tactical", "VH_TACTICAL",
      "OFF", "ON",            0, 0, 0, NULL, apply_tactical,      NULL,           dis_notAtTitle, dis_notAtTitle },
    /* Greyed+locked with no valid pack; toggleable + auto-ON when one is installed. apply_hdpack
     * couples it to INTERNAL RES / WINDOW SCALE (see above). */
    { "HD PACK",         OVL_TOGGLE, &g_vhHdPack,     "video",  "VH_HDPACK",
      "OFF", "ON",            0, 0, 0, NULL, apply_hdpack,        NULL,           dis_noHdPack, dis_noHdPack },
    { "INTERNAL RES",    OVL_CHOICE, &g_vhInternalScale, "video", "VH_INTERNAL_SCALE",
      NULL, NULL,             1, 4, 1, NULL, PC_GpuSetInternalScale, NULL,        NULL, NULL, s_internalResText },
    { "WINDOW SCALE",    OVL_CHOICE, &g_vhScale,      "video",  "VH_SCALE",
      NULL, NULL,             1, 8, 1, "X",  PC_GpuSetScale,      NULL,           dis_whenFullscreen, NULL },
    { "FULLSCREEN",      OVL_TOGGLE, &g_vhFullscreen, "video",  "VH_FULLSCREEN",
      "OFF", "ON",            0, 0, 0, NULL, PC_GpuSetFullscreen, NULL,           dis_whenWindowed, NULL },
    { "CAMERA X-AXIS",   OVL_TOGGLE, &g_camInvertX,   "camera", "VH_CAM_INVERT_X",
      "NORMAL", "INVERTED",   0, 0, 0, NULL, NULL,                NULL,           NULL, NULL },
    { "CAMERA Y-AXIS",   OVL_TOGGLE, &g_camInvertY,   "camera", "VH_CAM_INVERT_Y",
      "NORMAL", "INVERTED",   0, 0, 0, NULL, NULL,                NULL,           NULL, NULL },
    { "BUTTON LABELS",   OVL_CHOICE, &g_btnLabels,    "controls", "VH_BUTTON_LABELS",
      NULL, NULL,             0, 1, 1, NULL, NULL,                NULL,           NULL, NULL, s_btnLabelText },
    /* Which disc the unified binary boots. Persists VH_REGION (its own apply -- iniKey NULL
     * skips the numeric persist); ' *' = applies on next launch. Greyed+locked with one disc. */
    { "DISC",            OVL_CHOICE, &g_discSel,      NULL, NULL,
      NULL, NULL,             0, DISC_MAX - 1, 1, NULL, apply_disc, NULL,         dis_oneDisc, dis_oneDisc, s_discChoice },
    /* Language packs: picklist of installed packs by manifest name; persists VH_LANG (its own
     * apply -- iniKey NULL skips the numeric persist); '*' = applies on next launch. Greyed+locked
     * with no valid pack, and whenever the selected DISC is the Japanese game (packs are US-only). */
    { "LANGUAGE",        OVL_CHOICE, &g_langSel,      NULL, NULL,
      NULL, NULL,             0, LANG_MAX, 1, NULL, apply_language, NULL,         dis_langRow, dis_langRow, s_langChoice },
    { "SAVE MANAGEMENT", OVL_ACTION, NULL, NULL, NULL,
      NULL, NULL,             0, 0, 0, NULL, NULL,                act_enterSaves, NULL, NULL },
    { "RETURN TO TITLE", OVL_ACTION, NULL, NULL, NULL,
      NULL, NULL,             0, 0, 0, NULL, NULL,                act_returnToTitle, dis_atTitle, dis_atTitle },
};
#define N_ITEMS ((int)(sizeof(s_items) / sizeof(s_items[0])))

/* ---- state ------------------------------------------------------------------------------------- */

enum { CONF_RESTORE, CONF_DELETE, CONF_RETURN_TITLE, CONF_QUIT };

static int s_open   = 0;
static int s_screen = OVL_SCREEN_MAIN;
static int s_sel    = 0;                    /* MAIN cursor */

static PC_SaveArchive *s_arch = NULL;
static int s_archCount = 0;
static int s_saveSel   = 0;                 /* SAVES cursor */
static char s_saveStatus[32];               /* last backup/restore/delete result */

static int  s_confKind = CONF_DELETE;       /* CONFIRM: which action */
static char s_confFile[64];                 /* CONFIRM: target archive filename */
static char s_confLabel[24];                /* CONFIRM: target archive display label */
static int  s_confSel  = 0;                 /* CONFIRM: option cursor */

static PC_SaveCard s_detail;                /* DETAIL: the inspected archive's 3 slots */
static char s_detailLabel[24];              /* DETAIL: the inspected archive's label */

static void loadArchives(void) {
    PC_SaveArchive *fresh = NULL;
    int count = PC_SaveArchiveListAlloc(&fresh);
    if (count < 0) {
        strncpy(s_saveStatus, "BACKUP LIST FAILED", sizeof(s_saveStatus) - 1);
        s_saveStatus[sizeof(s_saveStatus) - 1] = '\0';
        return;
    }
    PC_SaveArchiveListFree(s_arch);
    s_arch = fresh;
    s_archCount = count;
    if (s_saveSel >= s_archCount) s_saveSel = (s_archCount > 0) ? s_archCount - 1 : 0;
}

int  PC_OverlayIsOpen(void) { return s_open; }
int  PC_OverlayScreen(void) { return s_screen; }

/* Every entry point that opens the overlay must come through here. The ESC prompt can be the first
 * action while the renderer sizes from MAIN rows, so refresh the variable-length picklists first;
 * their choiceText/maxv pairs are then valid. */
static void overlayOpen(void) {
    int i;
    s_open = 1;
    s_screen = OVL_SCREEN_MAIN;
    s_sel = 0;                                   /* always reopen at the top */
    langScan();                                  /* refresh the LANGUAGE picklist (cheap dir scan) */
    discScan();                                  /* refresh the DISC picklist (launcher env, cheap) */
    /* Dynamic ceilings: arrows clamp at the real last entry, confirm cycles first..last -- the
     * same behaviour as every other CHOICE row, no special wrap on one arrow only. */
    for (i = 0; i < N_ITEMS; i++) {
        if (s_items[i].value == &g_langSel) s_items[i].maxv = s_langCount;
        if (s_items[i].value == &g_discSel) s_items[i].maxv = s_discCount - 1;
    }
}

void PC_OverlayToggle(void) {
    if (s_open) s_open = 0;
    else        overlayOpen();
}

/* ---- MAIN screen logic ------------------------------------------------------------------------- */

static void persist(const Item *it) {
    char buf[16];
    if (!it->iniKey) return;
    if (it->kind == OVL_CHOICE) { snprintf(buf, sizeof(buf), "%d", *it->value);
                                  PC_SaveIniConfig(it->iniSection, it->iniKey, buf); }
    else if (it->kind == OVL_TOGGLE) PC_SaveIniConfig(it->iniSection, it->iniKey, *it->value ? "1" : "0");
}

static void reconcileHdPack(void);   /* fwd (defined below; keeps HD PACK consistent with its modes) */

static void setValue(const Item *it, int nv) {
    if (it->apply) it->apply(nv); else *it->value = nv;
    persist(it);
    /* Cross-item rule: changing the window scale drops fullscreen (so the new scale is visible). */
    if (it->value == &g_vhScale && g_vhFullscreen) {
        int i;
        for (i = 0; i < N_ITEMS; i++)
            if (s_items[i].value == &g_vhFullscreen) { setValue(&s_items[i], 0); break; }
    }
    reconcileHdPack();   /* internal<=1, or (windowed) window<=1, turns HD PACK off */
}

/* Find the MAIN item bound to *value and set it (applies + persists + re-runs cross-item rules). */
static void setItemByValue(int *value, int nv) {
    int i;
    for (i = 0; i < N_ITEMS; i++)
        if (s_items[i].value == value) { setValue(&s_items[i], nv); return; }
}
/* HD renders only at INTERNAL RES > 1, and windowed only shows at WINDOW SCALE > 1. If HD is on but
 * a mode dropped below that, turn it off. No-op while apply_hdpack is mid-bump (s_inHdApply) or HD is off. */
static void reconcileHdPack(void) {
    if (s_inHdApply || !g_vhHdPack) return;
    if (g_vhInternalScale <= 1 || (!g_vhFullscreen && g_vhScale <= 1))
        setItemByValue(&g_vhHdPack, 0);
}

static void mainMove(int d)   { s_sel += d; if (s_sel < 0) s_sel = N_ITEMS - 1; if (s_sel >= N_ITEMS) s_sel = 0; }

static void mainAdjust(int d) {
    const Item *it = &s_items[s_sel];
    if (it->locked && it->locked()) return;   /* locked items are read-only (e.g. Tactical Mode off-title) */
    if (it->kind == OVL_TOGGLE) { int nv = (d < 0) ? 0 : 1; if (*it->value != nv) setValue(it, nv); }
    else if (it->kind == OVL_CHOICE) {
        int nv = *it->value + d * it->step;
        if (nv < it->minv) { nv = it->minv; } if (nv > it->maxv) { nv = it->maxv; }
        if (nv != *it->value) setValue(it, nv);
    }
}

static void mainActivate(void) {
    const Item *it = &s_items[s_sel];
    if (it->locked && it->locked()) return;   /* locked items are read-only (e.g. Tactical Mode off-title) */
    if (it->kind == OVL_TOGGLE) setValue(it, !*it->value);
    else if (it->kind == OVL_CHOICE) { int nv = *it->value + it->step; if (nv > it->maxv) nv = it->minv; setValue(it, nv); }
    else if (it->action) it->action();
}

static void act_enterSaves(void) {
    s_screen = OVL_SCREEN_SAVES;
    s_saveSel = 0;
    s_saveStatus[0] = '\0';
    loadArchives();
}

/* ---- SAVES screen logic ------------------------------------------------------------------------ */

static void startConfirm(int kind) {
    s_confKind = kind;
    s_confSel  = 0;                          /* default to the safe first option */
    if (s_archCount > 0) {
        strncpy(s_confFile,  s_arch[s_saveSel].file,  sizeof(s_confFile)  - 1); s_confFile[sizeof(s_confFile) - 1]   = '\0';
        strncpy(s_confLabel, s_arch[s_saveSel].label, sizeof(s_confLabel) - 1); s_confLabel[sizeof(s_confLabel) - 1] = '\0';
    }
    s_screen = OVL_SCREEN_CONFIRM;
}

static void act_returnToTitle(void) {
    startConfirm(CONF_RETURN_TITLE);
    /* startConfirm may have copied an archive label; replace it with the stakes warning. */
    strncpy(s_confLabel, "UNSAVED PROGRESS LOST", sizeof(s_confLabel) - 1);
    s_confLabel[sizeof(s_confLabel) - 1] = '\0';
}

/* ---- ESC quit confirmation. Reuses the CONFIRM screen/plumbing above; the only new wiring is
 * this entry point and execConfirm's CONF_QUIT branch (PC_GpuPumpEvents calls this on Escape
 * instead of exiting directly). The quit hook lets a harness observe YES without exiting. ----- */
static void doExit(void) { exit(0); }
static void (*s_quitAction)(void) = doExit;

/* No progress exists in the title flow. State_Movie's transition table makes MOV_LOGO_USA_STR and
 * MOV_TITLE_WS_STR the boot/title-attract pair; every later MOV_* leads into (or follows) a run,
 * so story FMVs retain the confirmation. PC_AtTitleMenu is the corresponding non-movie state. */
static int quitImmediately(void) {
    return PC_AtTitleMenu() ||
           (gState.primary == STATE_MOVIE &&
            (gState.movieIdxToPlay == MOV_LOGO_USA_STR ||
             gState.movieIdxToPlay == MOV_TITLE_WS_STR));
}

void PC_OverlayRequestQuit(void) {
    if (s_open) {                 /* overlay already open: Escape is Back -- close it entirely */
        s_open = 0;
        return;
    }
    if (quitImmediately()) {
        s_quitAction();           /* title / title-attract: no progress to protect */
        return;
    }
    overlayOpen();
    startConfirm(CONF_QUIT);
    /* startConfirm may have copied an archive label; replace it with the stakes warning. */
    strncpy(s_confLabel, "UNSAVED PROGRESS LOST", sizeof(s_confLabel) - 1);
    s_confLabel[sizeof(s_confLabel) - 1] = '\0';
}

/* ---- RETURN TO TITLE. The confirm only REQUESTS the jump; PC_ApplyReturnToTitle performs it at
 * the top of the main loop (main.c, before UpdateState) -- the one point where no game code is
 * mid-frame, unlike the pad path. See docs/gameplay-additions.md, "Overlay internals". ---------- */
static int s_returnToTitlePending;

static void PC_ReturnToTitle(void) {
    s_returnToTitlePending = 1;
}

void PC_ApplyReturnToTitle(void) {
    if (!s_returnToTitlePending) return;
    s_returnToTitlePending = 0;
    {
        /* A movie stream is NOT object-driven, so abort it explicitly (the START-skip teardown;
         * no-op when idle), then DROP the held final frame: CdlPause keeps the last decoded frame
         * on the overlay until the movie flow's ClearScreen, which this jump never reaches. */
        extern void Movie_AbortForReturnToTitle(void);
        extern void PC_MovieSubsClose(void);
        Movie_AbortForReturnToTitle();
        PC_GpuSetMovieOverlay(NULL, 0, 0);
        PC_MovieSubsClose();
    }
    {
        /* audio.h only carries a commented-out FIXME decl; declare the real signature
         * (src/core/audio.c:1346) locally -- an implicit int declaration is UB at LP64. */
        extern void PerformAudioCommand(s16 cmd);
        PerformAudioCommand(AUDIO_CMD_STOP_ALL);
    }
    gIsEnemyTurn = 0;
    /* A request can arrive mid-event/mid-load; clear the flags the abandoned sequence would
     * have cleared itself (mirrors the demo-battle-end teardown, evaluators.c). */
    gState.inEvent = 0;
    gPlayerControlSuppressed = 0;
    gState.primary   = STATE_TITLE_SCREEN;
    gState.secondary = 0;
    gState.state3    = 0;
    gState.state4    = 0;
    gState.state7    = 1;
    PC_SyncBalance();   /* keep patchApplied == gTacticalMode consistent */
}

static void openDetail(void) {
    if (s_archCount <= 0) return;
    if (!PC_SaveReadCard(s_arch[s_saveSel].file, &s_detail)) {
        strncpy(s_saveStatus, "INVALID BACKUP", sizeof(s_saveStatus) - 1);
        s_saveStatus[sizeof(s_saveStatus) - 1] = '\0';
        return;
    }
    strncpy(s_detailLabel, s_arch[s_saveSel].label, sizeof(s_detailLabel) - 1);
    s_detailLabel[sizeof(s_detailLabel) - 1] = '\0';
    s_screen = OVL_SCREEN_DETAIL;
}

static void savesInput(int b) {
    switch (b) {
    case OVL_BTN_UP:       if (s_saveSel > 0) s_saveSel--; break;
    case OVL_BTN_DOWN:     if (s_saveSel < s_archCount - 1) s_saveSel++; break;
    case OVL_BTN_SQUARE:
        if (PC_SaveBackupCurrent()) strncpy(s_saveStatus, "BACKUP CREATED", sizeof(s_saveStatus) - 1);
        else                        strncpy(s_saveStatus, "BACKUP FAILED", sizeof(s_saveStatus) - 1);
        s_saveStatus[sizeof(s_saveStatus) - 1] = '\0';
        s_saveSel = 0; loadArchives(); break; /* new one lands on top */
    case OVL_BTN_CIRCLE:   if (s_archCount > 0) startConfirm(CONF_RESTORE); break;
    case OVL_BTN_TRIANGLE: if (s_archCount > 0) startConfirm(CONF_DELETE);  break;
    case OVL_BTN_START:    openDetail(); break;    /* inspect the selected archive's slots */
    case OVL_BTN_CROSS:    s_screen = OVL_SCREEN_MAIN; break;
    default: break;
    }
}

/* ---- CONFIRM screen logic ---------------------------------------------------------------------- */

static int confCount(void) { return (s_confKind == CONF_RESTORE) ? 3 : 2; }

static void execConfirm(void) {
    if (s_confKind == CONF_QUIT) {
        /* yes: quit (exactly what Escape did before this feature); no: opened by Escape, so close
         * entirely rather than falling back to MAIN. Either way the overlay closes. */
        if (s_confSel == 1) s_quitAction();
        s_open = 0;
        return;
    }
    if (s_confKind == CONF_RETURN_TITLE) {
        if (s_confSel == 0) { PC_ReturnToTitle(); s_open = 0; }  /* yes: teardown to title + close overlay */
        else                { s_screen = OVL_SCREEN_MAIN; }      /* cancel: back to the settings list */
        return;
    }
    if (s_confKind == CONF_RESTORE) {
        int restored = 0;
        int attempted = 0;
        if (s_confSel == 0) {
            if (PC_SaveBackupCurrent()) { attempted = 1; restored = PC_SaveRestore(s_confFile); }
            else strncpy(s_saveStatus, "BACKUP FAILED", sizeof(s_saveStatus) - 1);
        } else if (s_confSel == 1) {
            attempted = 1;
            restored = PC_SaveRestore(s_confFile);
        }
        if (attempted)
            strncpy(s_saveStatus, restored ? "RESTORE COMPLETE" : "RESTORE FAILED", sizeof(s_saveStatus) - 1);
        /* s_confSel == 2: cancel */
    } else { /* CONF_DELETE */
        if (s_confSel == 0)
            strncpy(s_saveStatus, PC_SaveDeleteArchive(s_confFile) ? "BACKUP DELETED" : "DELETE FAILED",
                    sizeof(s_saveStatus) - 1);
        /* s_confSel == 1: cancel */
    }
    s_saveStatus[sizeof(s_saveStatus) - 1] = '\0';
    loadArchives();                          /* backup-first / delete may have changed the list */
    s_screen = OVL_SCREEN_SAVES;
}

static void confirmInput(int b) {
    int n = confCount();
    switch (b) {
    case OVL_BTN_UP:    case OVL_BTN_LEFT:  if (s_confSel > 0)     s_confSel--; break;
    case OVL_BTN_DOWN:  case OVL_BTN_RIGHT: if (s_confSel < n - 1) s_confSel++; break;
    case OVL_BTN_CIRCLE:                    execConfirm(); break;
    case OVL_BTN_CROSS:  /* cancel -> back to where the confirm was opened from */
        if (s_confKind == CONF_QUIT)              s_open = 0;   /* opened by Escape: close entirely */
        else if (s_confKind == CONF_RETURN_TITLE) s_screen = OVL_SCREEN_MAIN;
        else                                       s_screen = OVL_SCREEN_SAVES;
        break;
    default: break;
    }
}

/* ---- input router ------------------------------------------------------------------------------ */

void PC_OverlayInput(int b) {
    if (!s_open) return;
    if (s_screen == OVL_SCREEN_MAIN) {
        switch (b) {
        case OVL_BTN_UP:     mainMove(-1); break;
        case OVL_BTN_DOWN:   mainMove(+1); break;
        case OVL_BTN_LEFT:   mainAdjust(-1); break;
        case OVL_BTN_RIGHT:  mainAdjust(+1); break;
        case OVL_BTN_CIRCLE: mainActivate(); break;
        default: break;
        }
    } else if (s_screen == OVL_SCREEN_SAVES) {
        savesInput(b);
    } else if (s_screen == OVL_SCREEN_DETAIL) {
        if (b == OVL_BTN_CROSS || b == OVL_BTN_START) s_screen = OVL_SCREEN_SAVES;  /* back */
    } else {
        confirmInput(b);
    }
}

/* ---- renderer accessors ------------------------------------------------------------------------ */

/* The overlay's own text is ENGLISH and deliberately not translatable: language packs cover the
 * GAME's content, not the port's UI. The 5x7 font is caps-only Latin, so a non-Latin pack could not
 * render here anyway; English keeps every language's behaviour identical. */
const char *PC_OverlayTitle(void) {
    if (s_screen == OVL_SCREEN_SAVES) return "SAVE MANAGEMENT";
    /* CONFIRM: save-mgmt confirms belong to that screen; return-to-title is an OPTIONS action;
     * quit gets its own title (it wasn't opened from either the settings list or the archive browser). */
    if (s_screen == OVL_SCREEN_CONFIRM) {
        if (s_confKind == CONF_QUIT) return "QUIT";
        return (s_confKind == CONF_RETURN_TITLE) ? "OPTIONS" : "SAVE MANAGEMENT";
    }
    return "OPTIONS";
}

int  PC_OverlayCount(void)    { return N_ITEMS; }
int  PC_OverlaySelected(void) { return s_sel; }

int PC_OverlayItem(int i, const char **label, const char **valueText) {
    static char vbuf[32];
    const Item *it;
    if (i < 0 || i >= N_ITEMS) { if (label) *label = ""; if (valueText) *valueText = NULL; return 0; }
    it = &s_items[i];
    if (label) *label = it->label;
    /* When the HD PACK row is unusable, its value says WHY ("NO PACK" / "OUTDATED PACK" / "WRONG
     * GAME") instead of a meaningless OFF. With a valid pack it stays a plain ON/OFF: the content
     * counts (75 BG, 16 FMV) are console-only ([HD] detect log), and the OSD font has no '+'. */
    if (it->value == &g_vhHdPack && valueText) {
        const char *st = PC_HdPackStatusShort();
        if (st) { *valueText = st; return 1; }
    }
    if (it->kind == OVL_TOGGLE) {
        if (valueText) *valueText = *it->value ? it->onText : it->offText;
        return 1;
    }
    if (it->kind == OVL_CHOICE) {
        if (valueText) {
            if (it->choiceText) { *valueText = it->choiceText[*it->value]; }   /* text-labelled choice */
            else { snprintf(vbuf, sizeof(vbuf), "%s%d", it->prefix ? it->prefix : "", *it->value);
                   *valueText = vbuf; }
        }
        return 1;
    }
    if (valueText) *valueText = NULL;         /* ACTION: label only */
    return 0;
}

/* Widest value string an item can display -- used ONLY to size the MAIN panel so it doesn't jump scale
 * (or shrink into a blocky 2x) when a text-choice cycles between a long and short value. For a
 * text-labelled CHOICE this is the longest choiceText entry; otherwise it's just the current value. */
const char *PC_OverlayItemWidestValue(int i) {
    const Item *it;
    if (i < 0 || i >= N_ITEMS) return NULL;
    it = &s_items[i];
    if (it->kind == OVL_CHOICE && it->choiceText) {
        /* measure the TRANSLATED forms -- the panel is sized off this */
        const char *best = NULL;
        int v;
        for (v = it->minv; v <= it->maxv; v++) {
            const char *t = it->choiceText[v];
            if (t && (!best || strlen(t) > strlen(best))) best = t;
        }
        return best;
    }
    { const char *val = NULL; PC_OverlayItem(i, NULL, &val); return val; }
}

int PC_OverlayItemDisabled(int i) {
    if (i < 0 || i >= N_ITEMS) return 0;
    return s_items[i].disabled ? s_items[i].disabled() : 0;
}

int  PC_OverlaySaveCount(void)     { return s_archCount; }
int  PC_OverlaySaveSelected(void)  { return s_saveSel; }
int  PC_OverlaySaveHasActive(void) { return PC_SaveHasActive(); }
const char *PC_OverlaySaveStatus(void) { return s_saveStatus; }
const char *PC_OverlaySaveLabel(int i) {
    if (i < 0 || i >= s_archCount) return NULL;
    return s_arch[i].label;
}
int PC_OverlaySaveActive(int i) {   /* 1 if row i matches the current active card */
    if (i < 0 || i >= s_archCount) return 0;
    return s_arch[i].active;
}

const char *PC_OverlayConfirmMsg(void) {
    if (s_confKind == CONF_QUIT)         return "QUIT THE GAME?";
    if (s_confKind == CONF_RETURN_TITLE) return "RETURN TO TITLE?";
    return (s_confKind == CONF_RESTORE) ? "REPLACE CURRENT CARD?" : "DELETE THIS BACKUP?";
}
int PC_OverlayConfirmCount(void) { return confCount(); }
int PC_OverlayConfirmSelected(void) { return s_confSel; }
const char *PC_OverlayConfirmTarget(void) { return s_confLabel; }   /* the archive label being acted on */
/* A destructive confirm gets ONE red "eye-catch" line, on the element that matters most: DELETE /
 * RESTORE -> the QUESTION line (the target is a neutral backup date; delete is permanent, restore
 * overwrites the current card); RETURN TO TITLE / QUIT -> the TARGET line, itself the stakes warning. */
int PC_OverlayConfirmMsgWarn(void)    { return s_confKind == CONF_DELETE || s_confKind == CONF_RESTORE; }
int PC_OverlayConfirmTargetWarn(void) { return s_confKind == CONF_RETURN_TITLE || s_confKind == CONF_QUIT; }
const char *PC_OverlayConfirmOption(int i) {
    static const char *RESTORE[3] = { "BACK UP THEN RESTORE", "RESTORE ONLY", "CANCEL" };
    static const char *DELETE_[2] = { "DELETE", "CANCEL" };
    static const char *RETTITLE[2] = { "RETURN TO TITLE", "CANCEL" };
    static const char *QUIT[2] = { "NO", "YES, QUIT" };
    if (s_confKind == CONF_QUIT)         return (i >= 0 && i < 2) ? QUIT[i] : "";
    if (s_confKind == CONF_RETURN_TITLE) return (i >= 0 && i < 2) ? RETTITLE[i] : "";
    if (s_confKind == CONF_RESTORE)      return (i >= 0 && i < 3) ? RESTORE[i] : "";
    return (i >= 0 && i < 2) ? DELETE_[i] : "";
}

const char *PC_OverlayDetailTitle(void) { return s_detailLabel; }
const char *PC_OverlayDetailSlot(int i) {
    if (i < 0 || i > 2 || !s_detail.occupied[i]) return NULL;   /* NULL => renderer shows "EMPTY" */
    return s_detail.slot[i];
}
