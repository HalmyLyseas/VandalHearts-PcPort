/* Regression fixtures for Escape's quit confirmation. Compiled and run by quit_confirm.sh; not
 * part of the real game build. See docs/gameplay-additions.md, "Overlay internals", and
 * CONF_QUIT in src/pc_overlay.c. */

/* `#include`s pc_overlay.c directly to reach its static state machine (same pattern as
 * pack_input_test.c) -- the file compiles through the STAGED real game headers (state.h,
 * battle.h, audio.h, common.h) via -I$STAGE_DIR, same profile as its real Makefile rule. */

/* The externs those headers declare (gState, gPlayerControlSuppressed, gIsEnemyTurn, ...) are
 * stubbed below rather than faked with hand-rolled types; the fixtures never call
 * PC_ApplyReturnToTitle, so those stubs only need to satisfy the linker. */
#include "../../src/pc_overlay.c"

#include <stdio.h>
#include <string.h>

static int g_fail = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { fprintf(stderr, "  FAIL: %s\n", msg); g_fail = 1; } \
} while (0)

/* ---- stubs for externs pc_overlay.c references but the fixtures never reach ---- */
int g_camInvertX = 0, g_camInvertY = 0, g_btnLabels = 0, g_vhHdPack = 0;
int PC_HdPackAvailable(void) { return 0; }
const char *PC_HdPackStatusShort(void) { return NULL; }
int PC_SaveIniConfig(const char *section, const char *key, const char *value) {
    (void)section; (void)key; (void)value; return 0;
}
int g_vhScale = 2, g_vhFullscreen = 0, g_vhInternalScale = 1;
void PC_GpuSetScale(int scale) { g_vhScale = scale; }
void PC_GpuSetFullscreen(int on) { g_vhFullscreen = on; }
void PC_GpuSetInternalScale(int scale) { g_vhInternalScale = scale; }
int gTacticalMode = 0;
int PC_AtTitleMenu(void) { return 1; }
void PC_SyncBalance(void) {}
int PC_SaveArchiveListAlloc(PC_SaveArchive **out) { *out = NULL; return 0; }
void PC_SaveArchiveListFree(PC_SaveArchive *out) { (void)out; }
int PC_SaveHasActive(void) { return 0; }
int PC_SaveBackupCurrent(void) { return 0; }
int PC_SaveRestore(const char *file) { (void)file; return 0; }
int PC_SaveDeleteArchive(const char *file) { (void)file; return 0; }
int PC_SaveReadCard(const char *file, PC_SaveCard *out) { (void)file; (void)out; return 0; }
int PC_LangListPacks(char folders[][64], char names[][64], int max) {
    (void)folders; (void)names; (void)max; return 0;
}
const char *PC_LangBootFolder(void) { return ""; }
/* PC_ApplyReturnToTitle's own body-local externs -- never reached by these fixtures. */
void Movie_AbortForReturnToTitle(void) {}
void PC_GpuSetMovieOverlay(const unsigned short *bgr555, int w, int h) {
    (void)bgr555; (void)w; (void)h;
}
void PC_MovieSubsClose(void) {}
void PerformAudioCommand(s16 cmd) { (void)cmd; }
/* Real game globals gState/gIsEnemyTurn/gPlayerControlSuppressed only need to exist here --
 * PC_ApplyReturnToTitle (their only reader/writer in this TU) is never called. */
State gState;
u8 gIsEnemyTurn;
u8 gPlayerControlSuppressed;

/* Observes the YES action instead of exiting the test process. */
static int s_quitObserved = 0;
static void observeQuit(void) { s_quitObserved = 1; }

/* Fixture a: Escape on a closed overlay opens it straight on the quit confirm, safe default first. */
static void FixtureA_OpensOnQuitConfirm(void) {
    printf("fixture a: Escape on a closed overlay opens the quit confirm (NO selected)\n");
    PC_OverlayRequestQuit();
    CHECK(PC_OverlayIsOpen() == 1, "overlay did not open");
    CHECK(PC_OverlayScreen() == OVL_SCREEN_CONFIRM, "did not land on the CONFIRM screen");
    CHECK(PC_OverlayConfirmCount() == 2, "expected 2 options (NO / YES, QUIT)");
    CHECK(PC_OverlayConfirmSelected() == 0, "cursor did not default to option 0 (the safe NO)");
    CHECK(strcmp(PC_OverlayConfirmOption(0), "NO") == 0, "option 0 is not \"NO\"");
    CHECK(strcmp(PC_OverlayConfirmOption(1), "YES, QUIT") == 0, "option 1 is not \"YES, QUIT\"");
    CHECK(strcmp(PC_OverlayTitle(), "QUIT") == 0, "title is not \"QUIT\"");
    CHECK(strcmp(PC_OverlayConfirmMsg(), "QUIT THE GAME?") == 0, "question is not \"QUIT THE GAME?\"");
    CHECK(strcmp(PC_OverlayConfirmTarget(), "UNSAVED PROGRESS LOST") == 0, "target line is wrong");
    CHECK(PC_OverlayConfirmTargetWarn() == 1, "target line is not flagged as a warning");
    printf(g_fail ? "fixture a: FAIL\n" : "fixture a: PASS\n");
}

/* Fixture b: the Back button (Cross) on the quit confirm closes the overlay entirely, not MAIN --
 * ESC opened it directly, so cancelling must not drop the player into the settings list. */
static void FixtureB_BackClosesEntirely(void) {
    int before = g_fail;
    printf("fixture b: Cross on the quit confirm closes the overlay entirely (not MAIN)\n");
    PC_OverlayInput(OVL_BTN_CROSS);
    CHECK(PC_OverlayIsOpen() == 0, "Back did not close the overlay");
    printf((g_fail != before) ? "fixture b: FAIL\n" : "fixture b: PASS\n");
}

/* Fixture c: Escape while the overlay is already open (any screen) acts as Back -- closes it. */
static void FixtureC_EscapeOnOpenOverlayCloses(void) {
    int before = g_fail;
    printf("fixture c: Escape while the overlay is already open closes it (Back)\n");
    PC_OverlayToggle();   /* open on MAIN, e.g. via SELECT+START -- not the quit path */
    CHECK(PC_OverlayIsOpen() == 1, "setup: overlay did not open");
    PC_OverlayRequestQuit();
    CHECK(PC_OverlayIsOpen() == 0, "Escape on an already-open overlay did not close it");
    printf((g_fail != before) ? "fixture c: FAIL\n" : "fixture c: PASS\n");
}

/* Fixture d: reopen via Escape, move to YES and confirm -- the quit hook observes the request
 * instead of the harness process exiting. */
static void FixtureD_YesQuitsViaHook(void) {
    int before = g_fail;
    printf("fixture d: selecting YES, QUIT and confirming reaches the quit action\n");
    s_quitAction = observeQuit;
    PC_OverlayRequestQuit();
    CHECK(PC_OverlayIsOpen() == 1, "setup: overlay did not reopen");
    PC_OverlayInput(OVL_BTN_DOWN);
    CHECK(PC_OverlayConfirmSelected() == 1, "cursor did not move to option 1 (YES, QUIT)");
    PC_OverlayInput(OVL_BTN_CIRCLE);
    CHECK(s_quitObserved == 1, "the quit action was never invoked");
    CHECK(PC_OverlayIsOpen() == 0, "overlay should also be closed once the quit is requested");
    printf((g_fail != before) ? "fixture d: FAIL\n" : "fixture d: PASS\n");
}

int main(void) {
    FixtureA_OpensOnQuitConfirm();
    FixtureB_BackClosesEntirely();
    FixtureC_EscapeOnOpenOverlayCloses();
    FixtureD_YesQuitsViaHook();
    return g_fail ? 1 : 0;
}
