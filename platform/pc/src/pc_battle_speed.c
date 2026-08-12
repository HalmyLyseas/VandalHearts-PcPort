/* pc_battle_speed.c -- Stage-3 (1.4 F1) battle-only fast-forward: the speed state, the
 * battle-state gate, and the pad-2 L2/R2 input edge. Extracted verbatim from libetc.c; VSync()
 * calls in for the pacing divisor and the leave-battle reset (seams: pc_etc_internal.h). */
#include "PsyQ/libetc.h"
#include "state.h"
#include "pc_platform.h"
#include "pc_overlay.h"
#include "pc_etc_internal.h"

/* ---- Stage-3 (1.4 F1): battle-only fast-forward -------------------------------------------------
 * A whole-tick speed multiplier: VSync() divides the inter-frame idle wait by the factor, so the game
 * runs N *complete* UpdateEngine() ticks in the wall-clock of one. AI/RNG stay byte-identical -- only
 * the idle time between ticks is compressed; never a fractional or skipped tick. Effective ONLY in an
 * active battle (STATE_27 / LOAD_IN_BATTLE_SAVE); menus, world map, FMV and dialogue always run 1x.
 *
 * Input: the physical L2/R2 triggers are routed to the game's unused "pad 2" (high word) L2/R2 bits
 * in pc_pad_read() -- the same decouple the 1.1 ally-cycle uses for L1/R1 -- so the right stick keeps
 * camera pitch and no in-game control is stolen (the game never reads pad-2 L2/R2 outside three
 * debug-only object handlers). R2 = faster, L2 = slower (1x/2x, clamped). Keyboard: '.'/','.
 * Not a balance change and it never alters outcomes, so it applies in both Normal and Tactical mode.
 *
 * 2x only: the host renders a full battle frame in ~8ms (~120fps ceiling), so 2x is reached by every
 * tick still rendering; 3x would need frame-skipping the software rasterizer (strobing, poor UX) for no
 * worthwhile gain, so it was dropped. 2x is already a large, smooth speed-up. */
#define VH_BATTLE_SPEED_MAX 2
static int s_battleSpeed = 1;   /* 1..MAX; reset to 1x on leaving battle (see VSync) */

int PC_InActiveBattle(void) {
    /* Match core/main.c's State_Battle() dispatch set exactly -- every primary state that runs a real-time
     * battle tick, so fast-forward covers all battle entry paths. STATE_30 = normal story battle
     * (map-entered), STATE_LOAD_IN_BATTLE_SAVE(23) = loaded in-battle save, STATE_27 = dojo/trial +
     * debug, STATE_3/STATE_31 = other battle sub-entries. (1.4 shipped with only {27,23}, so R2 silently
     * no-op'd in a normal map-initiated battle -- the missing STATE_30 is the fix.) */
    switch (gState.primary) {
    case STATE_3:
    case STATE_LOAD_IN_BATTLE_SAVE:
    case STATE_27:
    case STATE_30:
    case STATE_31:
        return 1;
    default:
        return 0;
    }
}

/* Effective speed for the OSD indicator: 1 when not in a battle so the readout hides. */
int PC_BattleSpeedGet(void) {
    return PC_InActiveBattle() ? s_battleSpeed : 1;
}

/* Edge-detect the pad-2 (high-word) L2/R2 bits and step the speed. Fed the full 32-bit pad word from
 * PadRead(). Only adjustable in an active battle with the options overlay closed. */
void PC_BattleSpeedInput(unsigned int pad) {
    static unsigned int prev = 0;
    unsigned int hi = pad >> 16;                     /* pad 2 lives in the high word */
    unsigned int newpress = hi & ~prev;              /* rising edges -> one step per tap */
    prev = hi;
    if (!PC_InActiveBattle() || PC_OverlayIsOpen()) return;
    if ((newpress & PADR2) && s_battleSpeed < VH_BATTLE_SPEED_MAX) s_battleSpeed++;
    if ((newpress & PADL2) && s_battleSpeed > 1)                   s_battleSpeed--;
}


int PC_BattleSpeedRaw(void) { return s_battleSpeed; }   /* diagnostics label (VH_FRAME_TIME) */

/* Fast-forward resets to 1x on leaving battle, so it never carries into the next battle or an
 * overworld save -- each battle starts at normal speed (called once per VSync). */
void PC_BattleSpeedReset(void) { if (!PC_InActiveBattle()) s_battleSpeed = 1; }
