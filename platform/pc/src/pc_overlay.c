/*
 * Stage-3 in-game options overlay -- menu model + state machine. See pc_overlay.h for the API.
 *
 * MAIN: a data-driven settings list (camera invert, video scale/fullscreen) + a "SAVE MANAGEMENT"
 *       entry. Toggle/choice changes apply immediately and persist to vandalhearts.ini.
 * SAVES: a browser over the whole-card archives (pc_saves.c). Flat list + face-button actions:
 *        Square = back up current, Circle = restore, Triangle = delete, Cross = back. Restore/Delete
 *        route through CONFIRM.
 * CONFIRM: a small prompt. Restore is a 3-way ("back up then restore" is the safe default) so the
 *          current card is never lost by surprise; Delete is Yes/No.
 *
 * There is no "Close" item: the SELECT+START chord is the sole show/hide (a face-button close would
 * leak the held press to the game behind -- see libetc.c's pad filter).
 */
#include "pc_overlay.h"
#include "pc_platform.h"   /* PC_SaveIniConfig, PC_GpuSetScale/SetFullscreen, g_vhScale/g_vhFullscreen */
#include "pc_saves.h"      /* archive list + backup/restore/delete */
#include <stddef.h>
#include <stdio.h>         /* snprintf */
#include <string.h>        /* strncpy */

/* Live settings the overlay reads/writes. Camera invert (libetc.c) is read every frame by the pad
 * mapping. Video (pc_gpu_window.c) is applied via the apply() callback below. */
extern int g_camInvertX;
extern int g_camInvertY;

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
    int       (*disabled)(void);            /* optional: 1 => greyed; may be NULL         */
} Item;

/* Window scale and fullscreen are mutually-exclusive display modes; the INACTIVE one is greyed. */
static int dis_whenFullscreen(void) { return g_vhFullscreen; }
static int dis_whenWindowed(void)   { return !g_vhFullscreen; }

static void act_enterSaves(void);           /* SAVE MANAGEMENT -> the saves screen */

/* A global's address is a compile-time constant, so this const table with &g_* is valid. */
static const Item s_items[] = {
    { "WINDOW SCALE",    OVL_CHOICE, &g_vhScale,      "video",  "VH_SCALE",
      NULL, NULL,             1, 8, 1, "X",  PC_GpuSetScale,      NULL,           dis_whenFullscreen },
    { "FULLSCREEN",      OVL_TOGGLE, &g_vhFullscreen, "video",  "VH_FULLSCREEN",
      "OFF", "ON",            0, 0, 0, NULL, PC_GpuSetFullscreen, NULL,           dis_whenWindowed },
    { "CAMERA X-AXIS",   OVL_TOGGLE, &g_camInvertX,   "camera", "VH_CAM_INVERT_X",
      "NORMAL", "INVERTED",   0, 0, 0, NULL, NULL,                NULL,           NULL },
    { "CAMERA Y-AXIS",   OVL_TOGGLE, &g_camInvertY,   "camera", "VH_CAM_INVERT_Y",
      "NORMAL", "INVERTED",   0, 0, 0, NULL, NULL,                NULL,           NULL },
    { "SAVE MANAGEMENT", OVL_ACTION, NULL, NULL, NULL,
      NULL, NULL,             0, 0, 0, NULL, NULL,                act_enterSaves, NULL },
};
#define N_ITEMS ((int)(sizeof(s_items) / sizeof(s_items[0])))

/* ---- state ------------------------------------------------------------------------------------- */

#define MAX_ARCHIVES 64
enum { CONF_RESTORE, CONF_DELETE };

static int s_open   = 0;
static int s_screen = OVL_SCREEN_MAIN;
static int s_sel    = 0;                    /* MAIN cursor */

static PC_SaveArchive s_arch[MAX_ARCHIVES];
static int s_archCount = 0;
static int s_saveSel   = 0;                 /* SAVES cursor */

static int  s_confKind = CONF_DELETE;       /* CONFIRM: which action */
static char s_confFile[64];                 /* CONFIRM: target archive filename */
static char s_confLabel[24];                /* CONFIRM: target archive display label */
static int  s_confSel  = 0;                 /* CONFIRM: option cursor */

static PC_SaveCard s_detail;                /* DETAIL: the inspected archive's 3 slots */
static char s_detailLabel[24];              /* DETAIL: the inspected archive's label */

static void loadArchives(void) {
    s_archCount = PC_SaveArchiveList(s_arch, MAX_ARCHIVES);
    if (s_saveSel >= s_archCount) s_saveSel = (s_archCount > 0) ? s_archCount - 1 : 0;
}

int  PC_OverlayIsOpen(void) { return s_open; }
int  PC_OverlayScreen(void) { return s_screen; }

void PC_OverlayToggle(void) {
    s_open = !s_open;
    if (s_open) { s_screen = OVL_SCREEN_MAIN; s_sel = 0; }   /* always reopen at the top */
}

/* ---- MAIN screen logic ------------------------------------------------------------------------- */

static void persist(const Item *it) {
    char buf[16];
    if (!it->iniKey) return;
    if (it->kind == OVL_CHOICE) { snprintf(buf, sizeof(buf), "%d", *it->value);
                                  PC_SaveIniConfig(it->iniSection, it->iniKey, buf); }
    else if (it->kind == OVL_TOGGLE) PC_SaveIniConfig(it->iniSection, it->iniKey, *it->value ? "1" : "0");
}

static void setValue(const Item *it, int nv) {
    if (it->apply) it->apply(nv); else *it->value = nv;
    persist(it);
    /* Cross-item rule: changing the window scale drops fullscreen (so the new scale is visible). */
    if (it->value == &g_vhScale && g_vhFullscreen) {
        int i;
        for (i = 0; i < N_ITEMS; i++)
            if (s_items[i].value == &g_vhFullscreen) { setValue(&s_items[i], 0); break; }
    }
}

static void mainMove(int d)   { s_sel += d; if (s_sel < 0) s_sel = N_ITEMS - 1; if (s_sel >= N_ITEMS) s_sel = 0; }

static void mainAdjust(int d) {
    const Item *it = &s_items[s_sel];
    if (it->kind == OVL_TOGGLE) { int nv = (d < 0) ? 0 : 1; if (*it->value != nv) setValue(it, nv); }
    else if (it->kind == OVL_CHOICE) {
        int nv = *it->value + d * it->step;
        if (nv < it->minv) nv = it->minv; if (nv > it->maxv) nv = it->maxv;
        if (nv != *it->value) setValue(it, nv);
    }
}

static void mainActivate(void) {
    const Item *it = &s_items[s_sel];
    if (it->kind == OVL_TOGGLE) setValue(it, !*it->value);
    else if (it->kind == OVL_CHOICE) { int nv = *it->value + it->step; if (nv > it->maxv) nv = it->minv; setValue(it, nv); }
    else if (it->action) it->action();
}

static void act_enterSaves(void) { s_screen = OVL_SCREEN_SAVES; s_saveSel = 0; loadArchives(); }

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

static void openDetail(void) {
    if (s_archCount <= 0) return;
    PC_SaveReadCard(s_arch[s_saveSel].file, &s_detail);
    strncpy(s_detailLabel, s_arch[s_saveSel].label, sizeof(s_detailLabel) - 1);
    s_detailLabel[sizeof(s_detailLabel) - 1] = '\0';
    s_screen = OVL_SCREEN_DETAIL;
}

static void savesInput(int b) {
    switch (b) {
    case OVL_BTN_UP:       if (s_saveSel > 0) s_saveSel--; break;
    case OVL_BTN_DOWN:     if (s_saveSel < s_archCount - 1) s_saveSel++; break;
    case OVL_BTN_SQUARE:   PC_SaveBackupCurrent(); s_saveSel = 0; loadArchives(); break; /* new one lands on top */
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
    if (s_confKind == CONF_RESTORE) {
        if (s_confSel == 0)      { PC_SaveBackupCurrent(); PC_SaveRestore(s_confFile); }  /* back up first */
        else if (s_confSel == 1) { PC_SaveRestore(s_confFile); }                          /* restore only */
        /* s_confSel == 2: cancel */
    } else { /* CONF_DELETE */
        if (s_confSel == 0)      { PC_SaveDeleteArchive(s_confFile); }                     /* delete */
        /* s_confSel == 1: cancel */
    }
    loadArchives();                          /* backup-first / delete may have changed the list */
    s_screen = OVL_SCREEN_SAVES;
}

static void confirmInput(int b) {
    int n = confCount();
    switch (b) {
    case OVL_BTN_UP:    case OVL_BTN_LEFT:  if (s_confSel > 0)     s_confSel--; break;
    case OVL_BTN_DOWN:  case OVL_BTN_RIGHT: if (s_confSel < n - 1) s_confSel++; break;
    case OVL_BTN_CIRCLE:                    execConfirm(); break;
    case OVL_BTN_CROSS:                     s_screen = OVL_SCREEN_SAVES; break;   /* cancel */
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

const char *PC_OverlayTitle(void) {
    if (s_screen == OVL_SCREEN_SAVES || s_screen == OVL_SCREEN_CONFIRM) return "SAVE MANAGEMENT";
    return "OPTIONS";
}

int  PC_OverlayCount(void)    { return N_ITEMS; }
int  PC_OverlaySelected(void) { return s_sel; }

int PC_OverlayItem(int i, const char **label, const char **valueText) {
    static char vbuf[24];
    const Item *it;
    if (i < 0 || i >= N_ITEMS) { if (label) *label = ""; if (valueText) *valueText = NULL; return 0; }
    it = &s_items[i];
    if (label) *label = it->label;
    if (it->kind == OVL_TOGGLE) { if (valueText) *valueText = *it->value ? it->onText : it->offText; return 1; }
    if (it->kind == OVL_CHOICE) {
        if (valueText) { snprintf(vbuf, sizeof(vbuf), "%s%d", it->prefix ? it->prefix : "", *it->value);
                         *valueText = vbuf; }
        return 1;
    }
    if (valueText) *valueText = NULL;         /* ACTION: label only */
    return 0;
}

int PC_OverlayItemDisabled(int i) {
    if (i < 0 || i >= N_ITEMS) return 0;
    return s_items[i].disabled ? s_items[i].disabled() : 0;
}

int  PC_OverlaySaveCount(void)     { return s_archCount; }
int  PC_OverlaySaveSelected(void)  { return s_saveSel; }
int  PC_OverlaySaveHasActive(void) { return PC_SaveHasActive(); }
const char *PC_OverlaySaveLabel(int i) {
    if (i < 0 || i >= s_archCount) return NULL;
    return s_arch[i].label;
}
int PC_OverlaySaveActive(int i) {   /* 1 if row i matches the current active card */
    if (i < 0 || i >= s_archCount) return 0;
    return s_arch[i].active;
}

const char *PC_OverlayConfirmMsg(void) {
    return (s_confKind == CONF_RESTORE) ? "REPLACE CURRENT CARD?" : "DELETE THIS BACKUP?";
}
int PC_OverlayConfirmCount(void) { return confCount(); }
int PC_OverlayConfirmSelected(void) { return s_confSel; }
const char *PC_OverlayConfirmTarget(void) { return s_confLabel; }   /* the archive label being acted on */
const char *PC_OverlayConfirmOption(int i) {
    static const char *RESTORE[3] = { "BACK UP THEN RESTORE", "RESTORE ONLY", "CANCEL" };
    static const char *DELETE_[2] = { "DELETE", "CANCEL" };
    if (s_confKind == CONF_RESTORE) return (i >= 0 && i < 3) ? RESTORE[i] : "";
    return (i >= 0 && i < 2) ? DELETE_[i] : "";
}

const char *PC_OverlayDetailTitle(void) { return s_detailLabel; }
const char *PC_OverlayDetailSlot(int i) {
    if (i < 0 || i > 2 || !s_detail.occupied[i]) return NULL;   /* NULL => renderer shows "EMPTY" */
    return s_detail.slot[i];
}
