/* PC backend for PsyQ/libetc.h's pad/vsync interface, backed by SDL2. VSync() reproduces the
 * original's frame-locked NTSC timing via a fractional-deadline limiter; PadRead() combines SDL2
 * keyboard state and any connected SDL_GameController onto the standard PS1 digital-pad word. */
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "PsyQ/libetc.h"
#include "pc_lang.h"
#include "pc_platform.h"
#include "pc_etc_internal.h"
#include "pc_balance.h"
#include "pc_overlay.h"

/* PSX NTSC vblank rate, paced by a fractional-deadline accumulator (not the naive integer
 * 1000/60 ms, which runs ~4% fast). See docs/pc-port/subsystems/kernel.md, "VSync". */
#define VBLANK_HZ 59.94
#define FRAME_MS_F (1000.0 / VBLANK_HZ)   /* ~16.683 ms */

static double s_nextVBlankMs = 0.0; /* fractional frame deadline (SDL_GetTicks ms basis) */
static int s_vsyncInitialized = 0;
static int s_vblankCount = 0;

void PadInit(int mode) {
    (void)mode;
    /* SDL2 is initialized by the platform's own startup code; nothing to
     * do here beyond letting the caller know pad reads are ready. */
}

/* Optional SDL_GameController support, alongside the keyboard map. Any plugged-in controller
 * (Xbox/PlayStation/etc., via SDL's built-in mapping DB) is read each frame and OR'd into the
 * same PSX pad word. Lazily initialized so a headless run or a session with no pad costs nothing. */
static SDL_GameController *s_pad = NULL;
static int s_gcSubsysReady = -1; /* -1 untried, 0 failed, 1 ready */

/* Overlay button-label style (0=PLAYSTATION, 1=XBOX). Xbox-layout pads are the large majority on
 * PC, so it defaults to XBOX when unset -- a PlayStation player flips it once in the overlay and
 * it persists. Only the port's own overlay footers use this; the game's own prompts are untouched. */
enum { BTN_PLAYSTATION = 0, BTN_XBOX = 1 };
int g_btnLabels = BTN_XBOX;

int PC_ButtonLabelStyle(void) { return g_btnLabels; }

static void pc_pad_ensure_open(void) {
    int i, n;
    if (s_gcSubsysReady == -1) {
        s_gcSubsysReady = (SDL_InitSubSystem(SDL_INIT_GAMECONTROLLER) == 0) ? 1 : 0;
    }
    if (s_gcSubsysReady != 1) return;
    /* Drop a controller that was unplugged. */
    if (s_pad && !SDL_GameControllerGetAttached(s_pad)) {
        SDL_GameControllerClose(s_pad);
        s_pad = NULL;
    }
    if (s_pad) return;
    /* Open the first available controller (covers plug-in after launch). */
    n = SDL_NumJoysticks();
    for (i = 0; i < n; i++) {
        if (SDL_IsGameController(i)) {
            s_pad = SDL_GameControllerOpen(i);
            if (s_pad) break;
        }
    }
}

/* Right-stick camera axis invert. Runtime-mutable (the in-game options overlay toggles these and
 * persists them to vandalhearts.ini's [camera] section); initialised once from VH_CAM_INVERT_X/Y
 * (which the ini loader sets). Not static -- the overlay reads/writes them. */
int g_camInvertX = 0;
int g_camInvertY = 1;   /* default INVERTED (modern twin-stick); PC_LoadCamInvert honours env/ini */

static void PC_LoadCamInvert(void) {
    static int loaded = 0;
    const char *e;
    if (loaded) return;
    loaded = 1;
    /* X defaults to normal; Y defaults to INVERTED (the modern twin-stick convention -- push up =
     * tilt the view down). An explicit VH_CAM_INVERT_Y=0 (env or the shipped ini) restores normal. */
    e = getenv("VH_CAM_INVERT_X"); g_camInvertX = (e && e[0] == '1') ? 1 : 0;
    e = getenv("VH_CAM_INVERT_Y"); g_camInvertY = e ? (e[0] == '1' ? 1 : 0) : 1;
    /* Overlay button-label style (0=PLAYSTATION, 1=XBOX); default XBOX when the ini is silent. */
    e = getenv("VH_BUTTON_LABELS");
    if (e) { int v = e[0] - '0'; g_btnLabels = (v == BTN_PLAYSTATION || v == BTN_XBOX) ? v : BTN_XBOX; }
}

static unsigned int pc_pad_read(void) {
    unsigned int p = 0;
    SDL_GameController *c;
    const int AXIS_DZ = 16000;

    PC_LoadCamInvert();          /* load invert defaults once, before the no-controller early-out, so
                                    keyboard-only play + the overlay still see the right values */
    pc_pad_ensure_open();
    c = s_pad;
    if (!c) return 0;

#define GC_BTN(b) SDL_GameControllerGetButton(c, (b))
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_UP))       p |= PADLup;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_DOWN))     p |= PADLdown;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_LEFT))     p |= PADLleft;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_DPAD_RIGHT))    p |= PADLright;
    /* SDL face buttons are positional (A=south, B=east, X=west, Y=north),
     * matching PSX Cross/Circle/Square/Triangle by position. */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_A))             p |= PADRdown;   /* Cross    */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_B))             p |= PADRright;  /* Circle   */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_X))             p |= PADRleft;   /* Square   */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_Y))             p |= PADRup;     /* Triangle */
    /* The physical shoulders drive the ally-cycle, not the camera: they go into the HIGH pad word
     * ("pad 2", PADL1/PADR1 << 16, gPad2State) while the right stick feeds the LOW-word L1/R1
     * below (gPadState). See docs/pc-port/subsystems/kernel.md, "Pad input". */
    if (GC_BTN(SDL_CONTROLLER_BUTTON_LEFTSHOULDER))  p |= (unsigned)PADL1 << 16;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_RIGHTSHOULDER)) p |= (unsigned)PADR1 << 16;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_START))         p |= PADstart;
    if (GC_BTN(SDL_CONTROLLER_BUTTON_BACK))          p |= PADselect;
    /* (L3/R3 are analog-pad only; VH reads a digital pad, so they're unmapped.) */
#undef GC_BTN
    /* Analog triggers -> pad-2 (high word) L2/R2 for the 1.4 battle fast-forward, mirroring the
     * L1/R1 ally-cycle decouple above. The game never reads pad-2 L2/R2 in normal play; right-stick
     * vertical (below) keeps low-word L2/R2 = camera pitch, so no camera control is lost. */
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERLEFT)  > AXIS_DZ) p |= (unsigned)PADL2 << 16;
    if (SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_TRIGGERRIGHT) > AXIS_DZ) p |= (unsigned)PADR2 << 16;
    /* Left analog stick also drives the D-pad, so movement works either way. */
    {
        int lx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTX);
        int ly = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_LEFTY);
        if (lx < -AXIS_DZ) p |= PADLleft;  else if (lx > AXIS_DZ) p |= PADLright;
        if (ly < -AXIS_DZ) p |= PADLup;    else if (ly > AXIS_DZ) p |= PADLdown;
    }
    /* Right analog stick -> camera shoulder buttons, twin-stick feel. Horizontal = L1/R1 (camera
     * rotate), vertical = L2/R2 (camera elevation); ORs with the physical shoulders/triggers above
     * (both inputs work). SDL Y axis is +down, matching the left-stick convention. */
    {
        int rx = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTX);
        int ry = SDL_GameControllerGetAxis(c, SDL_CONTROLLER_AXIS_RIGHTY);
        /* Per-axis right-stick invert (loaded once at the top of pc_pad_read). */
        if (g_camInvertX) rx = -rx;
        if (g_camInvertY) ry = -ry;
        if (rx < -AXIS_DZ) p |= PADL1;  else if (rx > AXIS_DZ) p |= PADR1;  /* left->L1, right->R1 */
        if (ry >  AXIS_DZ) p |= PADL2;  else if (ry < -AXIS_DZ) p |= PADR2; /* down->L2, up->R2   */
    }
    return p;
}

/* Options-overlay pad filter: sits between the raw pad word and what the game reads, idempotent
 * across repeated same-frame calls (PadRead(0) is read twice per battle frame). See
 * docs/pc-port/subsystems/kernel.md, "Pad input". */
static unsigned int PC_OverlayFilterPad(unsigned int raw) {
    static unsigned int prev = 0;
    static int chordLatch = 0;
    static unsigned int swallow = 0;             /* buttons held at close, masked until released */
    int wasOpen = PC_OverlayIsOpen();
    unsigned int lo = raw & 0xFFFFu;
    int selHeld   = (lo & PADselect) != 0;
    int startHeld = (lo & PADstart)  != 0;
    unsigned int newpress = raw & ~prev;         /* rising edges vs the previous distinct read */
    unsigned int out = raw;

    if (selHeld && startHeld) {                   /* chord: toggle once per press (latched) */
        if (!chordLatch) { PC_OverlayToggle(); chordLatch = 1; }
    } else {
        chordLatch = 0;
    }
    if (selHeld) out &= ~(unsigned)(PADstart | PADselect);   /* SELECT gates START (movie-skip safe) */

    if (PC_OverlayIsOpen()) {
        /* Forward one edge at a time; the overlay routes each per its current screen. Cross is
         * Back/Cancel inside the overlay (the SELECT+START chord remains the only full close). */
        if (newpress & PADLup)    PC_OverlayInput(OVL_BTN_UP);
        if (newpress & PADLdown)  PC_OverlayInput(OVL_BTN_DOWN);
        if (newpress & PADLleft)  PC_OverlayInput(OVL_BTN_LEFT);
        if (newpress & PADLright) PC_OverlayInput(OVL_BTN_RIGHT);
        if (newpress & PADRright) PC_OverlayInput(OVL_BTN_CIRCLE);   /* Circle   */
        if (newpress & PADRdown)  PC_OverlayInput(OVL_BTN_CROSS);    /* Cross    */
        if (newpress & PADRleft)  PC_OverlayInput(OVL_BTN_SQUARE);   /* Square   */
        if (newpress & PADRup)    PC_OverlayInput(OVL_BTN_TRIANGLE); /* Triangle */
        /* Start alone = an overlay action (e.g. inspect a save); Start WITH Select is the close chord,
         * so only forward Start when Select isn't held (else the closing chord would also fire it). */
        if ((newpress & PADstart) && !selHeld) PC_OverlayInput(OVL_BTN_START);
        swallow = raw;                                       /* keep the swallow set primed with all held */
        prev = raw;
        return 0;                                            /* freeze the game's pad while open */
    }
    if (wasOpen) swallow |= raw;                  /* closed THIS frame (e.g. via chord): swallow held */
    swallow &= raw;                               /* drop buttons the user has since released */
    out &= ~swallow;                              /* hide the still-held leftovers from the game */
    prev = raw;
    return out;
}

unsigned int PadRead(int id) {
    (void)id;

    SDL_PumpEvents();
    const Uint8 *keys = SDL_GetKeyboardState(NULL);

    unsigned int p1 = 0;
    if (keys[SDL_SCANCODE_UP])     p1 |= PADLup;
    if (keys[SDL_SCANCODE_DOWN])   p1 |= PADLdown;
    if (keys[SDL_SCANCODE_LEFT])   p1 |= PADLleft;
    if (keys[SDL_SCANCODE_RIGHT])  p1 |= PADLright;
    if (keys[SDL_SCANCODE_W])      p1 |= PADRup;       /* Triangle */
    if (keys[SDL_SCANCODE_S])      p1 |= PADRdown;     /* X */
    if (keys[SDL_SCANCODE_A])      p1 |= PADRleft;     /* Square */
    if (keys[SDL_SCANCODE_D])      p1 |= PADRright;    /* Circle */
    if (keys[SDL_SCANCODE_Q])      p1 |= PADL1;                 /* camera rotate (low word) */
    if (keys[SDL_SCANCODE_E])      p1 |= PADR1;
    /* Camera elevation (low-word L2/R2) -- the keyboard equivalent of the gamepad right-stick vertical.
     * R raises the view angle (toward overhead), F lowers it (toward horizontal). Distinct from the
     * ,/. fast-forward keys, which drive the *high-word* (pad-2) L2/R2. */
    if (keys[SDL_SCANCODE_R])      p1 |= PADR2;
    if (keys[SDL_SCANCODE_F])      p1 |= PADL2;
    /* Keyboard ally-cycle on [ / ] -> high word (pad 2), mirroring the gamepad shoulders. Full
     * keyboard layout is in docs/controls.md. */
    if (keys[SDL_SCANCODE_LEFTBRACKET])  p1 |= (unsigned)PADL1 << 16;   /* cycle previous */
    if (keys[SDL_SCANCODE_RIGHTBRACKET]) p1 |= (unsigned)PADR1 << 16;   /* cycle next */
    if (keys[SDL_SCANCODE_RETURN]) p1 |= PADstart;
    if (keys[SDL_SCANCODE_SPACE])  p1 |= PADselect;
    /* Battle fast-forward on ',' / '.' -> pad-2 (high word) L2/R2, mirroring the gamepad triggers
     * below. ',' = slower, '.' = faster. */
    if (keys[SDL_SCANCODE_COMMA])  p1 |= (unsigned)PADL2 << 16;
    if (keys[SDL_SCANCODE_PERIOD]) p1 |= (unsigned)PADR2 << 16;

    /* OR in any connected gamepad. */
    p1 |= pc_pad_read();

    p1 |= PC_SmokePadHold();   /* VH_SMOKE boot harness: hold START through the intro movies (pc_diag.c) */

    /* PadRead(0) packs both controller ports into one 32-bit value (port 0 in the low 16 bits, port
     * 1 in the high 16); no second controller is mapped, so the high half stays zero. */
    PC_BattleSpeedInput(p1);          /* battle fast-forward, pad-2 L2/R2 */
    return PC_OverlayFilterPad(p1);   /* chord/overlay input filter */
}

int VSync(int mode) {
    if (!s_vsyncInitialized) {
        s_nextVBlankMs = (double)SDL_GetTicks();
        s_vsyncInitialized = 1;
        /* Boot order matters: PC_LangBoot() must run before PC_BalanceBoot(). See
         * docs/pc-port/subsystems/kernel.md, "VSync". */
        PC_LangBoot();   /* language pack: load + apply once (inert without a pack) */
        PC_BalanceBoot();
    }

    if (mode < 0) {
        /* Query mode: report the running vblank count without waiting. */
        return (int)s_vblankCount;
    }

    PC_DiagFrameEntry(mode);   /* VH_FRAME_TIME work/idle split (pc_diag.c) */

    /* Answer the compositor's responsiveness ping (and keep close/Ctrl+C live) even inside the
     * game's own VSync wait loops. See docs/pc-port/subsystems/kernel.md, "VSync". */
    { extern void PC_GpuPumpEvents(void); PC_GpuPumpEvents(); }

    int waits = (mode == 0) ? 1 : mode;
    /* Fast-forward resets to 1x on leaving battle, so it never carries into the next battle or an
     * overworld save -- each battle starts at normal speed. */
    PC_BattleSpeedReset();
    /* Divide the per-tick idle wait by the speed factor so N whole ticks fit the wall-clock of one --
     * outside battle PC_BattleSpeedGet() returns 1 (no-op). */
    double frameMs = FRAME_MS_F / (double)PC_BattleSpeedGet();
    /* One consolidated deadline per call, absorbing sub-frame overshoot in the remaining budget; only
     * a full frame of lateness resyncs. See docs/pc-port/subsystems/kernel.md, "VSync". */
    {
        double totalMs = frameMs * waits;
        Uint32 now = SDL_GetTicks();
        s_nextVBlankMs += totalMs;
        if ((double)now < s_nextVBlankMs) {
            PC_DiagIdleDelay((unsigned)(s_nextVBlankMs - now));   /* SDL_Delay, timed under VH_FRAME_TIME */
        } else if ((double)now - s_nextVBlankMs > totalMs) {
            s_nextVBlankMs = (double)now;     /* > one frame behind: genuine stall, resync */
        }
        s_vblankCount += waits;
    }

    PC_DiagVSyncRows();   /* the VH_*_LOG per-tick CSV rows (pc_diag.c) */

    { extern void PC_CdXaUpdate(void); PC_CdXaUpdate(); } /* XA music streaming pump */
    { extern void PC_SeqTick(void); PC_SeqTick(); }       /* SEQ (sequenced music) sequencer */

    /* GPU-trace recording, battle-gated (VH_GPU_RECORD_BATTLE=1): arm the recorder from the same
     * battle-state set the fast-forward gate uses, so a trace captures pure battle content. */
    { extern void PC_GpuTraceArmBattle(int on); PC_GpuTraceArmBattle(PC_InActiveBattle()); }

    PC_SmokeFrame();   /* VH_SMOKE boot harness: exit at the title screen (pc_diag.c) */

    PC_DiagFps(mode);   /* opt-in per-second FPS meter, VH_FPS_LOG=1 (pc_diag.c) */

    return (int)s_vblankCount;
}

int ResetCallback(void) {
    /* No VSync/interrupt callbacks are registered by anything in this
     * interface yet -- nothing to reset. */
    return 0;
}
