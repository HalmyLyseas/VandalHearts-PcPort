#define _GNU_SOURCE 1   /* dladdr (PC_DiagSpellFxLog's symbol resolution) */
/* pc_diag.c -- the port's env-gated diagnostics, extracted verbatim from libetc.c (which had
 * become the dumping ground for every investigation logger). Nothing here runs unless its VH_*
 * env var is set; the per-tick rows are driven from VSync() via PC_DiagVSyncRows(), and the
 * VH_SMOKE boot harness + VH_FRAME_TIME/VH_FPS_LOG meters via the other hooks (pc_etc_internal.h).
 * The frame column in every CSV is VSync(-1) -- the same vblank counter the old inline code read. */
#include <SDL2/SDL.h>
#include <stddef.h>
#include <stdio.h>
#include <stdlib.h>
#include "PsyQ/libetc.h"
#include "state.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "pc_platform.h"
#include "pc_etc_internal.h"

/* Diagnostic FPS logging (2026-07-12, investigating whether VSync(2) during
 * battle actually halves the real display-update rate, or whether real
 * hardware's positive-mode VSync() means something other than "block for
 * mode frame-durations"). Counts real VSync() *calls* (once per UpdateEngine()
 * call, i.e. once per real logic tick / display swap) separately from the
 * inner mode-driven wait loop, and prints calls-per-real-second once a
 * second to stderr, tagged with the mode last seen. */
static Uint32 s_fpsWindowStart = 0;
static int s_fpsWindowCalls = 0;
static int s_fpsLastMode = -2;

/* Camera-target full-watch-log (2026-07-12, exchange/20-camera-viewport-coordinates.md).
 * Mirrors exchange/21-camera-target-full-watch-log.lua's BizHawk capture field-for-field so
 * the two CSVs are diffable at matching game-state checkpoints (bmFuncIdx/bmState/bmState2),
 * not by raw frame number -- the two builds' frame counts never lined up 1:1 (see
 * feedback-02.md's timing table), but the BattleMgr state machine is the same code on both
 * platforms and gives a reliable alignment anchor. Logs every VSync() call (one row per real
 * logic tick), matching the "dense capture, not sampled" methodology now standard for this
 * project (see the phase-c-pc-port skill's "BizHawk as ground truth" section).
 *
 * One deliberate column difference from the Lua script: BizHawk's `targetAddr` is a raw PS1
 * KUSEG address, meaningless as a cross-platform value -- our own process's pointer values are
 * just as meaningless to a human reader. Logging the target's own gObjectArray index instead
 * (targetObjIdx) keeps the column comparable in spirit (identifies WHICH object is the target)
 * without a nonsensical raw-address diff; targetX/Y/Z remain real, directly diffable
 * coordinate values either way. */
static FILE *s_camTraceFile = NULL;

static s32 FindCameraObjIdx(s16 *outFuncIdx) {
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        s16 f = gObjectArray[i].functionIndex;
        if (f == OBJF_ATTACK_CAMERA || f == OBJF_FOCUS_CAMERA || f == OBJF_FOCUS_CAMERA_WIDE) {
            *outFuncIdx = f;
            return i;
        }
    }
    *outFuncIdx = -1;
    return -1;
}

static s32 ObjPtrToIdx(struct Object *p) {
    if (p == NULL) return -1;
    ptrdiff_t d = p - gObjectArray;
    if (d < 0 || d >= OBJ_DATA_CT) return -1;
    return (s32)d;
}

/* feedback-14 follow-up: per-sprite pipeline log. Enabled by VH_SPRITE_LOG env var. */
static FILE *s_spriteFateFile = NULL;
void PC_DebugSpriteLog(int tileX, int tileZ, int winX, int winZ, int mapSX, int mapSZ,
                       int culled, int gfxIdx, int sx, int sy, int otz, int otIdx) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_SPRITE_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_spriteFateFile == NULL) {
        s_spriteFateFile = fopen("vh_sprite_fate.csv", "w");
        if (s_spriteFateFile == NULL) { enabled = 0; return; }
        fprintf(s_spriteFateFile,
                "frame,tileX,tileZ,winX,winZ,mapSX,mapSZ,inWindowX,inWindowZ,"
                "culled,gfxIdx,sx,sy,otz,otIdx,ofx,ofy,h,rt00,rt02,rt22,trx,trz,terrainOtz,zsf4\n");
    }
    /* also compute the window-membership per axis to make the cull reason obvious */
    int inX = (tileX >= winX && tileX <= mapSX + winX - 1);
    int inZ = (tileZ >= winZ && tileZ <= mapSZ + winZ - 1);
    /* GTE projection state used for THIS sprite (ofx/ofy are stored <<16; report >>16 too) */
    int ofx = 0, ofy = 0; int h = 0, rt00 = 0, rt02 = 0, rt22 = 0, trx = 0, trz = 0;
    PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
    fprintf(s_spriteFateFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), tileX, tileZ, winX, winZ, mapSX, mapSZ, inX, inZ,
            culled, gfxIdx, sx, sy, otz, otIdx,
            (int)(ofx >> 16), (int)(ofy >> 16), h, rt00, rt02, rt22, trx, trz,
            PC_GteLastOtz(), PC_GteZsf4());
    fflush(s_spriteFateFile);
}

/* Per event-entity anim-set probe (build SPRITE_LOG=1, enable VH_SPRITE_LOG). Dumps each event
 * entity's baseAnimSet/altAnimSet pointer, the animSet actually used, animIdx, facing, animData,
 * and resulting gfxIdx + tile -- to find a prop entity spawned with the WRONG (character) anim-set.
 * Deduped per object slot: only writes a row when that slot's gfxIdx changes, so volume stays low. */
static FILE *s_evtEntFile = NULL;
void PC_DebugEvtEntityLog(int objIdx, int tileX, int tileZ, const void *base, const void *alt,
                          const void *cur, int animIdx, int facing, const void *animData,
                          int gfxIdx, int usingAlt, int runState, int opcode, int cmdState,
                          int cmdOff, int cmdArg) {
    static int enabled = -1;
    static int lastGfx[1024], lastOp[1024], lastState[1024];
    static unsigned char seen[1024];
    if (enabled < 0) enabled = (getenv("VH_SPRITE_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (objIdx >= 0 && objIdx < 1024) {
        /* log when the sprite OR the command-processing state changes (so we catch the
         * command each entity is parked on, not just gfx changes) */
        if (seen[objIdx] && lastGfx[objIdx] == gfxIdx && lastOp[objIdx] == opcode && lastState[objIdx] == runState) return;
        seen[objIdx] = 1; lastGfx[objIdx] = gfxIdx; lastOp[objIdx] = opcode; lastState[objIdx] = runState;
    }
    if (s_evtEntFile == NULL) {
        s_evtEntFile = fopen("vh_evt_entity.csv", "w");
        if (s_evtEntFile == NULL) { enabled = 0; return; }
        fprintf(s_evtEntFile,
                "frame,objIdx,tileX,tileZ,baseAnimSet,altAnimSet,animSetUsed,animIdx,facing,animData,gfxIdx,usingAlt,runState,opcode,cmdState,cmdOff,cmdArg\n");
    }
    fprintf(s_evtEntFile, "%d,%d,%d,%d,%p,%p,%p,%d,%d,%p,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), objIdx, tileX, tileZ, base, alt, cur, animIdx, facing, animData, gfxIdx, usingAlt,
            runState, opcode, cmdState, cmdOff, cmdArg);
    fflush(s_evtEntFile);
}

/* AI spell-target scoring log (build with AI_LOG=1, enable at runtime with VH_AI_LOG). One line per
 * candidate target a caster's AI scored in AI_ScoreSpellTargets; the highest SCORE is the target it picks.
 * Shows the term breakdown so a "wrong" choice is explainable -- especially the type-matchup term
 * ADV = -gAdvantage[caster.advantage][target.advantage] (your mage->armor / ranged->flyer doctrine).
 * A high raw gAdvantage means a poor matchup, so it SUBTRACTS from the score (less-preferred). */
static FILE *s_aiLogFile = NULL;
void PC_DebugAiTargetLog(int casterName, int casterAdv, int casterLvl,
                         int tgtName, int tgtClass, int tgtAdv, int tgtLvl, int tgtHpFrac,
                         int lvlTerm, int hpTerm, int advTerm, int advRaw, int terrTerm,
                         int magSusc, int magTerm, int score) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiLogFile == NULL) {
        s_aiLogFile = fopen("vh_ai_log.txt", "w");
        if (s_aiLogFile == NULL) { enabled = 0; return; }
    }
    /* magSusc: 1=resistant .. 5=weak. magTerm = F3 (magSusc-3)*K applied to the score (0 if K unset /
     * Normal mode). SCORE already includes magTerm. */
    fprintf(s_aiLogFile,
            "f=%d caster[name=%d adv=%d lvl=%d] -> target[name=%d class=%d adv=%d lvl=%d hpFrac=%d]"
            "  terms: base=+280 lvl=%+d hp=%+d ADV=%+d(gAdv=%d) terr=%+d magSusc=%d magTerm=%+d  = SCORE %d\n",
            VSync(-1), casterName, casterAdv, casterLvl,
            tgtName, tgtClass, tgtAdv, tgtLvl, tgtHpFrac,
            lvlTerm, hpTerm, advTerm, advRaw, terrTerm, magSusc, magTerm, score);
    fflush(s_aiLogFile);
}

/* bugreport-03 (DEATHANGEL caster never casts): dumps the AI's top-level spell decision in
 * Objf570_AI_ChooseAction case 0 -- every input that can force the melee branch (state 1) plus the state it
 * actually chose. `state` 1 = melee/move only (OBJF_AI_PLAN_ATTACK), 2/4 = cast (OBJF_AI_PLAN_SPELL_CAST).
 * The two zeroing inputs to watch: spells[]==SPELL_NULL, and mp < gSpells[spell].mpCost (which
 * zeroes spellEffectA/B and drops the AI to state 1 even for a fully-equipped caster). */
static FILE *s_aiDecFile = NULL;
void PC_DebugAiDecisionLog(int name, int cls, int team, int lvl, int mp, int maxMp,
                           int spell0, int spell1, int cost0, int cost1,
                           int effect0, int effect1, int effA, int effB, int state) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiDecFile == NULL) {
        s_aiDecFile = fopen("vh_ai_decision.csv", "w");
        if (s_aiDecFile == NULL) { enabled = 0; return; }
        fprintf(s_aiDecFile, "name,class,team,level,mp,maxMp,spell0,spell1,mpCost0,mpCost1,"
                             "exEffect0,exEffect1,spellEffectA,spellEffectB,chosenState\n");
    }
    fprintf(s_aiDecFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            name, cls, team, lvl, mp, maxMp, spell0, spell1, cost0, cost1,
            effect0, effect1, effA, effB, state);
    fflush(s_aiDecFile);
}

/* Dumps the game state-machine fields for freeze diagnosis (called from the SIGUSR1 handler in
 * pc_bootstrap.c). A "freeze" where the frame loop keeps ticking = a state stuck waiting on a
 * condition; these fields say which state/scene/map it's stuck in so we know where to look. */
/* bugreport (epilogue ocarina silent): log every PerformAudioCommand so we can see whether the
 * ending's PLAY_XA(102) == 0x366 is ever issued (trigger fired) vs never reaching the audio layer.
 * Decodes the 0x?00 opcode nibble for readability. Build AUDIO_CMD_LOG=1, enable VH_AUDIO_LOG. */
void PC_DebugAudioCmdLog(int cmd, int enabled) {
    static int en = -1;
    static FILE *f = NULL;
    if (en < 0) en = (getenv("VH_AUDIO_LOG") != NULL) ? 1 : 0;
    if (!en) return;
    if (f == NULL) {
        f = fopen("vh_audio_cmd.csv", "w");
        if (f == NULL) { en = 0; return; }
        fprintf(f, "t_ms,cmd_hex,op,id,audioEnabled\n");
    }
    {
        int op = (cmd >> 8) & 0xff, id = cmd & 0xff;
        const char *name = op == 0 ? "PARAMLESS" : op == 2 ? "PLAY_SEQ" : op == 3 ? "PLAY_XA"
                         : op == 5 || op == 6 || op == 7 ? "SFX" : op == 0x13 ? "PREPARE_XA" : "?";
        fprintf(f, "%u,0x%04x,%s,%d,%d\n", VSync(-1), cmd, name, id, enabled);
        fflush(f);
    }
}

void PC_DumpGameState(int fd) {
    (void)fd;
    fprintf(stderr,
        "gState: primary=%d secondary=%d state3=%d state4=%d state6=%d state7=%d "
        "scene=%d mapNum=%d movieIdx=%d inEvent=%d\n",
        (int)gState.primary, (int)gState.secondary, (int)gState.state3, (int)gState.state4,
        (int)gState.state6, (int)gState.state7, (int)gState.scene, (int)gState.mapNum,
        (int)gState.movieIdxToPlay, (int)gState.inEvent);
    fflush(stderr);
}

/* feedback-11 follow-up: mirror BizHawk's RAM Watch on our own window. Called from libgpu.c
 * right before the present, so the label matches the frame being shown. Rendered by
 * pc_gpu_window.c only when VH_CAM_OSD is set. */
void PC_UpdateCamOsd(void) {
    snprintf(g_camOsdText, sizeof(g_camOsdText), "PIT:%d YAW:%d X:%d Z:%d FRD:%d",
             gCameraRotation.vx, gCameraRotation.vy, gCameraPos.vx, gCameraPos.vz,
             gState.fieldRenderingDisabled);
}

static void LogCameraTraceRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_CAM_LOG") != NULL) ? 1 : 0;   /* opt-in; off for end users */
    if (!enabled) return;
    if (s_camTraceFile == NULL) {
        s_camTraceFile = fopen("vh_camera_target_full_watch_log_pc.csv", "w");
        if (s_camTraceFile == NULL) return;
        fprintf(s_camTraceFile,
                "frame,primary,bmFuncIdx,bmState,bmState2,"
                "camPosX,camPosY,camPosZ,camRotX,camRotY,camZoomZ,"
                "camObjIdx,camObjFuncIdx,camObjState,targetObjIdx,targetX,targetY,targetZ,"
                /* 30-fresh-look-deterministic-camera-hypothesis.md: winOrigin[X/Z] is the
                 * actual render-window origin (D_80122E28/2C, src/battle_0201b8.c:193), the
                 * derived quantity the terrain-window + sprite-cull both key off; camDirQuad is
                 * the yaw quadrant RenderField selects (src/graphics.c:1178). startCur[X/Z/Y]
                 * is gMapCursorStartingPos[mapNum] as read at RUNTIME on this build -- confirms
                 * the (byte-exact-static) table isn't being misread through a port stride/endian
                 * bug. These are what make the pre-AI frame-0 diff decisive. */
                /* 30-* (a): fieldRenderDisabled (gState) = field draws only when 0 -- the
                 * "first non-loading frame" un-blank moment; fadeLevel = gScreenFade's Objf795
                 * fade level (255=black .. 0=full brightness), the fade-in ramp. Together these
                 * pin the exact frame the scene becomes visible to the concurrent camPos, so we
                 * can state same-run whether our field un-blanks while the camera is still parked
                 * at (-32,-32) or already mid-pan. -1 fadeLevel = no active gScreenFade object. */
                "winOriginX,winOriginZ,camDirQuad,startCurX,startCurZ,startCurY,"
                /* live map cursor = the AI-turn camera-focus target CenterCamera eases toward
                 * (battle_0201b8.c:190); pairs with 43-camera-focus-easein-trace.lua's mapCursorX/Z
                 * so both platforms carry the ease-in target for the iteration-vs-gate diff. */
                "mapCursorX,mapCursorZ,"
                "fieldRenderDisabled,fadeLevel\n");
    }

    s16 bmFuncIdx = -1, bmState = -1, bmState2 = -1;
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        if (gObjectArray[i].functionIndex == OBJF_BATTLE_MGR) {
            bmFuncIdx = gObjectArray[i].functionIndex;
            bmState = gObjectArray[i].state;
            bmState2 = gObjectArray[i].state2;
            break;
        }
    }

    s16 camObjFuncIdx = -1;
    s32 camObjIdx = FindCameraObjIdx(&camObjFuncIdx);
    s16 camObjState = -1;
    s32 targetObjIdx = -1;
    s16 targetX = -1, targetY = -1, targetZ = -1;
    if (camObjIdx >= 0) {
        Object *camObj = &gObjectArray[camObjIdx];
        camObjState = camObj->state;
        struct Object *target = (camObjFuncIdx == OBJF_ATTACK_CAMERA)
                                     ? camObj->d.objf017.sprite
                                     : camObj->d.objf026.target;
        targetObjIdx = ObjPtrToIdx(target);
        if (targetObjIdx >= 0) {
            targetX = gObjectArray[targetObjIdx].x1.n;
            targetY = gObjectArray[targetObjIdx].y1.n;
            targetZ = gObjectArray[targetObjIdx].z1.n;
        }
    }

    s16 camDirQuad = (s16)((gCameraRotation.vy & 0xfff) >> 10);
    /* mapNum can be out of the 0..BATTLE_CT-1 range briefly outside a battle; clamp the read. */
    s16 startCurX = -1, startCurZ = -1, startCurY = -1;
    if (gState.mapNum < BATTLE_CT) {   /* mapNum is u8: no negative range to clamp */
        startCurX = gMapCursorStartingPos[gState.mapNum].x;
        startCurZ = gMapCursorStartingPos[gState.mapNum].z;
        startCurY = gMapCursorStartingPos[gState.mapNum].y;
    }

    s16 fadeLevel = (gScreenFade != NULL) ? gScreenFade->d.objf795.fade : -1;

    fprintf(s_camTraceFile,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), gState.primary, bmFuncIdx, bmState, bmState2,
            gCameraPos.vx, gCameraPos.vy, gCameraPos.vz,
            gCameraRotation.vx, gCameraRotation.vy, gCameraZoom.vz,
            camObjIdx, camObjFuncIdx, camObjState, targetObjIdx, targetX, targetY, targetZ,
            D_80122E28, D_80122E2C, camDirQuad, startCurX, startCurZ, startCurY,
            gMapCursorX, gMapCursorZ,
            gState.fieldRenderingDisabled, fadeLevel);
    fflush(s_camTraceFile);
}

/* AI-decision-chain trace (timing topic, Finding #1: the "unit active -> blue movement overlay"
 * early-gate). The overlay (gShowBlueMovementGrid=1, set in Objf013_BattleMgr state 7.2) can't fire
 * until BattleMgr STATE 6 exits, and state 6 blocks on `gAiPlanReady != 0` (battle_0201b8.c:3202),
 * which is set only when the AI-decision object chain finishes (Objf570_AI_ChooseAction -> Objf40x/Objf589,
 * ai.c:278 case 99, via the gAiPlanDone hand-off). On hardware state 6 lasts ~108 frames; on our build
 * ~18 -> the AI chain completes ~6x faster and the overlay shows early (mid camera-pan). This logs the
 * chain's live state so we can see EXACTLY which sub-state exits early. Env-gated (VH_AI_LOG); reads
 * only game RAM (no src/ changes, matching build untouched). Pairs with 43-camera-focus-easein. */
extern u8 gAiPlanReady; /* BattleMgr state-6 release flag (battle.h) */
extern u8 gAiPlanDone; /* Objf570 sub-object hand-off flag (ai.c) */
extern int g_aiVisitCount[8]; /* cumulative GetRCnt visits per AI range (libkernel.c); for calibration */
static FILE *s_aiChainFile = NULL;

static void LogAiChainRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_AI_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_aiChainFile == NULL) {
        s_aiChainFile = fopen("vh_ai_chain_pc.csv", "w");
        if (s_aiChainFile == NULL) return;
        fprintf(s_aiChainFile,
                "frame,primary,bmState,bmState2,showBlue,D468,D480,"
                "ai570Idx,ai570State,ai570State2,subFuncIdx,subState,subState2,"
                "camPosX,camPosZ,mapCursorX,mapCursorZ,"
                /* cumulative GetRCnt visits per AI range: diff across an AI phase = visits_per_scan */
                "vIsLag,v570,v400,v401,v402,v403,v404,v589\n");
    }

    s16 bmState = -1, bmState2 = -1;
    s16 ai570Idx = -1, ai570State = -1, ai570State2 = -1;
    s16 subFuncIdx = -1, subState = -1, subState2 = -1;
    for (s32 i = 0; i < OBJ_DATA_CT; i++) {
        s16 fi = gObjectArray[i].functionIndex;
        if (fi == OBJF_BATTLE_MGR) {
            bmState = gObjectArray[i].state;
            bmState2 = gObjectArray[i].state2;
        } else if (fi == OBJF_AI_CHOOSE_ACTION) {
            ai570Idx = (s16)i;
            ai570State = gObjectArray[i].state;
            ai570State2 = gObjectArray[i].state2;
        } else if (fi == OBJF_AI_BUILD_SPELL_VALUE_GRID || fi == OBJF_AI_BUILD_ENEMY_PROXIMITY_GRID || fi == OBJF_AI_PLAN_SPELL_CAST ||
                   fi == OBJF_AI_PLAN_ATTACK || fi == OBJF_AI_PLAN_RETREAT || fi == OBJF_AI_MOVE_TO_ESCAPE_POINT) {
            /* the currently-active AI sub-object (first found; the chain runs them one at a time) */
            subFuncIdx = fi;
            subState = gObjectArray[i].state;
            subState2 = gObjectArray[i].state2;
        }
    }

    fprintf(s_aiChainFile,
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), gState.primary, bmState, bmState2, gShowBlueMovementGrid,
            gAiPlanReady, gAiPlanDone, ai570Idx, ai570State, ai570State2,
            subFuncIdx, subState, subState2,
            gCameraPos.vx, gCameraPos.vz, gMapCursorX, gMapCursorZ,
            g_aiVisitCount[0], g_aiVisitCount[1], g_aiVisitCount[2], g_aiVisitCount[3],
            g_aiVisitCount[4], g_aiVisitCount[5], g_aiVisitCount[6], g_aiVisitCount[7]);
    fflush(s_aiChainFile);
}

/* rand()-seed full-watch-log (2026-07-12, exchange/20-camera-viewport-coordinates.md's "ROOT
 * CAUSE FOUND" section). Mirrors the camera trace's dense, every-tick, state-anchored capture
 * style, but for the shared PS1-BIOS-LCG rand() state (platform/pc/src/libkernel.c's
 * s_randSeed, exposed via GetRandSeedForDebug() to avoid changing that variable's linkage for
 * anything else) -- to directly confirm/deny whether libcd.c's CD-seek-jitter simulation
 * desyncs it from real hardware's stream (found empirically at RAM address 0x80009010 via
 * exchange/24-find-rand-seed-address.lua's LCG-signature scan; mirrored here as
 * exchange/25-rand-seed-full-watch-log.lua). */
extern u32 GetRandSeedForDebug(void);
static FILE *s_randTraceFile = NULL;

static void LogRandSeedRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_RAND_LOG") != NULL) ? 1 : 0;   /* opt-in; off for end users */
    if (!enabled) return;
    if (s_randTraceFile == NULL) {
        s_randTraceFile = fopen("vh_rand_seed_full_watch_log_pc.csv", "w");
        if (s_randTraceFile == NULL) return;
        fprintf(s_randTraceFile, "frame,primary,seed\n");
    }
    fprintf(s_randTraceFile, "%d,%d,0x%08x\n", VSync(-1), gState.primary,
            GetRandSeedForDebug());
    fflush(s_randTraceFile);
}

/* Camera-matrix / view-Z (OTZ) baseline mirror (exchange/31-camera-matrix-otz-baseline.lua).
 * Writes the exact same columns as the BizHawk baseline so the two CSVs diff directly, to
 * localize why our terrain OTZ (= avg view-Z) runs high enough (>=406) that graphics.c's
 * distance-darkening saturates terrain to black. Read at the same per-frame point as the
 * BizHawk end-of-frame read, so gCameraMatrix.t[] is equally "post-render stale" on both sides
 * (the reliable comparands are the inputs gCameraRotation/gCameraZoom/gMapScale/enableMapScaling
 * and the rotation submatrix m[][]; align rows by matched pose, not frame number). Env-gated
 * (VH_MTX_LOG) so normal runs are unaffected. */
static FILE *s_mtxTraceFile = NULL;

static void LogCameraMatrixRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_MTX_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;

    if (s_mtxTraceFile == NULL) {
        s_mtxTraceFile = fopen("vh_camera_matrix_otz_pc.csv", "w");
        if (s_mtxTraceFile == NULL) return;
        fprintf(s_mtxTraceFile,
                "frame,primary,mapNum,fieldDis,mapScaleEn,"
                "camRotX,camRotY,camRotZ,camPosX,camPosY,camPosZ,"
                "camZoomX,camZoomY,camZoomZ,mapScaleX,mapScaleY,mapScaleZ,"
                "m00,m01,m02,m10,m11,m12,m20,m21,m22,t0,t1,t2\n");
    }

    fprintf(s_mtxTraceFile,
            "%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,"
            "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), gState.primary, gState.mapNum, gState.fieldRenderingDisabled,
            gState.enableMapScaling,
            gCameraRotation.vx, gCameraRotation.vy, gCameraRotation.vz,
            gCameraPos.vx, gCameraPos.vy, gCameraPos.vz,
            gCameraZoom.vx, gCameraZoom.vy, gCameraZoom.vz,
            gMapScale.vx, gMapScale.vy, gMapScale.vz,
            gCameraMatrix.m[0][0], gCameraMatrix.m[0][1], gCameraMatrix.m[0][2],
            gCameraMatrix.m[1][0], gCameraMatrix.m[1][1], gCameraMatrix.m[1][2],
            gCameraMatrix.m[2][0], gCameraMatrix.m[2][1], gCameraMatrix.m[2][2],
            gCameraMatrix.t[0], gCameraMatrix.t[1], gCameraMatrix.t[2]);
    fflush(s_mtxTraceFile);
}

/* Terrain OTZ probe (otz-overflow / black-terrain investigation). RenderMapTile /
 * RenderEdgeMapTile (src/graphics.c) call PC_DebugTerrainTile once per drawn tile with the
 * post-clamp otz and the final vertex-0 colour. We accumulate per-frame aggregates (tile count,
 * otz min/max/mean, and how many tiles ended fully black -- graphics.c's distance-darkening
 * zeroes r0/g0/b0 for otz>=406) and LogTerrainRow() dumps one pose-aligned row per frame. This
 * answers directly whether our terrain otz is really high enough to darken to black at the
 * matched opening pose (camera matrix already confirmed == real hardware). Env-gated
 * (VH_TERRAIN_LOG); the call site itself is a #ifdef PC_DEBUG_TERRAIN_LOG (TERRAIN_LOG=1 build),
 * so the matching build is untouched. */
static FILE *s_terrFile = NULL;
static int s_terrCount = 0, s_terrBlack = 0, s_terrOtzSum = 0;
static int s_terrOtzMin = 0x7fffffff, s_terrOtzMax = 0;
/* GTE state captured on the first tile of the frame (same for every tile that frame):
 * trx/trz = g.tr[0]/g.tr[2] actually loaded at terrain render (the number that bisects
 * "tr too large" vs "vertices too far"); h = projection distance; rt22 = rotation row-2 z. */
static int s_terrTrx = 0, s_terrTrz = 0, s_terrH = 0, s_terrRt22 = 0;

void PC_DebugTerrainTile(int otz, int r0, int g0, int b0) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_TERRAIN_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_terrCount == 0) {
        int ofx, ofy; int h, rt00, rt02, rt22, trx, trz;
        PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
        s_terrTrx = trx; s_terrTrz = trz; s_terrH = h; s_terrRt22 = rt22;
    }
    s_terrCount++;
    s_terrOtzSum += otz;
    if (otz < s_terrOtzMin) s_terrOtzMin = otz;
    if (otz > s_terrOtzMax) s_terrOtzMax = otz;
    if (r0 == 0 && g0 == 0 && b0 == 0) s_terrBlack++;
}

static void LogTerrainRow(void) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_TERRAIN_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;

    if (s_terrFile == NULL) {
        s_terrFile = fopen("vh_terrain_otz_pc.csv", "w");
        if (s_terrFile == NULL) return;
        fprintf(s_terrFile, "frame,primary,fieldDis,pitch,yaw,tileCount,"
                            "otzMin,otzMax,otzMean,blackCount,trx,trz,h,rt22\n");
    }
    if (s_terrCount > 0) {
        fprintf(s_terrFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
                VSync(-1), gState.primary, gState.fieldRenderingDisabled,
                gCameraRotation.vx, gCameraRotation.vy,
                s_terrCount, s_terrOtzMin, s_terrOtzMax, s_terrOtzSum / s_terrCount, s_terrBlack,
                s_terrTrx, s_terrTrz, s_terrH, s_terrRt22);
        fflush(s_terrFile);
    }
    s_terrCount = 0; s_terrBlack = 0; s_terrOtzSum = 0;
    s_terrOtzMin = 0x7fffffff; s_terrOtzMax = 0;
}

/* AVSZ sprite-path probe (thread: which routine draws the opening characters?). AddObjPrim4
 * (object.c) uses RotAverage4 => AVSZ otz (= sz3/4-ish), so its sprites interleave with terrain,
 * unlike the raw-sz3 RenderUnitSprite path. If the demo-opening characters are drawn here, their
 * otz/otIdx will land in the terrain band; if AddObjPrim4 isn't called during the opening, our
 * build is missing the intro path entirely. Env-gated (VH_OBJPRIM4_LOG); the call site is
 * PC_DEBUG_SPRITE_LOG (SPRITE_LOG=1 build) so the matching build is untouched. */
static FILE *s_op4File = NULL;

void PC_DebugObjPrim4Log(int gfx, int otz, int otIdx, int sx, int sy, int otOfs) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_OBJPRIM4_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_op4File == NULL) {
        s_op4File = fopen("vh_objprim4.csv", "w");
        if (s_op4File == NULL) return;
        fprintf(s_op4File, "frame,primary,pitch,gfx,otz,otIdx,sx,sy,otOfs\n");
    }
    fprintf(s_op4File, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n",
            VSync(-1), gState.primary, gCameraRotation.vx,
            gfx, otz, otIdx, sx, sy, otOfs);
    fflush(s_op4File);
}

/* Opaque-sprite gfx diagnostic (casting-blob investigation). object.c's AddObjPrim* variants take
 * the OPAQUE branch (poly->tpage = gGfxTPageIds[gfx]) when d.sprite.semiTrans==0; the blob is a mesh
 * of those, some resolving to tpage 0x0000 (gGfxTPageIds[gfx]==0 -> samples framebuffer page 0). This
 * logs gfx + resolved tpage + owning effect functionIndex + screen pos, so we can (a) filter by the
 * blob's screen location to read its gfx, and (b) find which gfx's gGfxTPageIds is 0 and why. Env-gated
 * VH_OPAQUE_GFX_LOG; call site is #ifdef PC_DEBUG_SPRITE_LOG so the matching build is untouched. */
static FILE *s_opgFile = NULL;
void PC_DebugOpaqueGfx(int gfx, int tpage, int fn, int sx, int sy) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_OPAQUE_GFX_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_opgFile == NULL) {
        s_opgFile = fopen("vh_opaque_gfx.csv", "w");
        if (s_opgFile == NULL) return;
        fprintf(s_opgFile, "frame,gfx,tpage,fn,sx,sy\n");
    }
    fprintf(s_opgFile, "%d,%d,0x%04x,%d,%d,%d\n", VSync(-1), gfx, tpage & 0xffff, fn, sx, sy);
    fflush(s_opgFile);
}

/* Terrain projection diagnostic (terrain-collapse / pull-to-center investigation). RenderMapTile
 * calls this with a tile's 4 input model vertices right after the 4 corners are projected; we read
 * back what each corner projected to from the GTE ring (PC_GteProjEntry) plus H/OFX/OFY, so we can
 * compute the exact spread SX-OFX = (n*IR1)>>16 = H*IR1/SZ3 and see whether H, SZ3, or IR is
 * collapsing the spread toward the centre. One tile per frame (first RenderMapTile call), env-gated
 * (VH_TERRAINPROJ_LOG); the call site is #ifdef PC_DEBUG_TERRAIN_LOG so the matching build is untouched. */
static FILE *s_tprojFile = NULL;

void PC_DebugTerrainProjLog(int v0x, int v0y, int v0z, int v1x, int v1y, int v1z,
                            int v2x, int v2y, int v2z, int v3x, int v3y, int v3z) {
    static int enabled = -1;
    static int lastFrame = -1;
    int c, vin[4][3];
    int ofx, ofy; int h, rt00, rt02, rt22, trx, trz;

    if (enabled < 0) enabled = (getenv("VH_TERRAINPROJ_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (VSync(-1) == lastFrame) return;   /* first drawn tile of the frame only */
    lastFrame = VSync(-1);

    if (s_tprojFile == NULL) {
        s_tprojFile = fopen("vh_terrain_proj.csv", "w");
        if (s_tprojFile == NULL) return;
        fprintf(s_tprojFile, "frame,pitch,yaw,h,ofx,ofy");
        for (c = 0; c < 4; c++)
            fprintf(s_tprojFile, ",v%dx,v%dy,v%dz,sx%d,sy%d,ir1_%d,ir2_%d,ir3_%d,sz3_%d,n%d",
                    c, c, c, c, c, c, c, c, c, c);
        fprintf(s_tprojFile, "\n");
    }

    PC_GteDebugState(&ofx, &ofy, &h, &rt00, &rt02, &rt22, &trx, &trz);
    vin[0][0]=v0x; vin[0][1]=v0y; vin[0][2]=v0z;
    vin[1][0]=v1x; vin[1][1]=v1y; vin[1][2]=v1z;
    vin[2][0]=v2x; vin[2][1]=v2y; vin[2][2]=v2z;
    vin[3][0]=v3x; vin[3][1]=v3y; vin[3][2]=v3z;

    fprintf(s_tprojFile, "%d,%d,%d,%d,%d,%d", VSync(-1),
            gCameraRotation.vx, gCameraRotation.vy, h, ofx, ofy);
    for (c = 0; c < 4; c++) {
        int sx, sy, ir1, ir2, ir3, sz3, n;
        /* corners were pushed v0,v1,v2 (RTPT) then v3 (RTPS) => back = 3,2,1,0 */
        PC_GteProjEntry(3 - c, &sx, &sy, &ir1, &ir2, &ir3, &sz3, &n);
        fprintf(s_tprojFile, ",%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
                vin[c][0], vin[c][1], vin[c][2], sx, sy, ir1, ir2, ir3, sz3, n);
    }
    fprintf(s_tprojFile, "\n");
    fflush(s_tprojFile);
}

/* Full projected sprite-quad log (flip-vs-occlusion investigation). RenderUnitSprite calls this
 * right after RotTransPers4 sets the 4 quad corners, for every DRAWN unit sprite (past the otIdx
 * gate). With all 4 corners we can read the actual quad geometry: whether the "top-half crop" is a
 * well-formed quad occluded by terrain vs a malformed/degenerate quad; whether the "flip" is a
 * corner-X-order reversal; and whether both correlate with the same otz(=SZ3) threshold. Env-gated
 * (VH_SPRITE_QUAD_LOG); call site is #ifdef PC_DEBUG_SPRITE_LOG (SPRITE_LOG=1 build). */
static FILE *s_squadFile = NULL;

void PC_DebugSpriteQuadLog(int gfx, int otz, int otIdx, int facingLeft,
                           int direction, int coords0z, int facingFront,
                           int x0, int y0, int x1, int y1, int x2, int y2, int x3, int y3,
                           int tileX, int tileZ) {
    static int enabled = -1;
    if (enabled < 0) enabled = (getenv("VH_SPRITE_QUAD_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (s_squadFile == NULL) {
        s_squadFile = fopen("vh_sprite_quad.csv", "w");
        if (s_squadFile == NULL) return;
        /* pitch/yaw = gCameraRotation.vx/vy; facingLeft is driven ENTIRELY by
         * dir = ((yaw + direction + coords0z) & 0xfff) >> 10; facingLeft = !(dir==0||dir==1).
         * Logging the three raw inputs lets us reconstruct facingLeft and diff it against
         * BizHawk at a matched pose (see 41-sprite-orientation-baseline.lua). */
        fprintf(s_squadFile, "frame,pitch,yaw,gfx,otz,otIdx,facingLeft,direction,coords0z,facingFront,"
                            "tileX,tileZ,x0,y0,x1,y1,x2,y2,x3,y3");
        { int c; for (c = 0; c < 4; c++)
              fprintf(s_squadFile, ",ir2_%d,sz3_%d,n%d", c, c, c); }
        fprintf(s_squadFile, "\n");
    }
    fprintf(s_squadFile, "%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d,%d",
            VSync(-1), gCameraRotation.vx, gCameraRotation.vy, gfx, otz, otIdx, facingLeft,
            direction, coords0z, facingFront,
            tileX, tileZ, x0, y0, x1, y1, x2, y2, x3, y3);
    /* box-quad corners went through TransformOne (RotTransPers4: v0,v1,v2 via RTPT then v3) so the
     * projection ring holds them back=3,2,1,0; dump each corner's IR2/SZ3/n to pin the ~4x Y-squish
     * (height = H*IR2/SZ3): is SZ3 4x too big, IR2 (box Y-half-extent) 4x too small, or n saturated? */
    { int c; int sx, sy, ir1, ir2, ir3, sz3, n;
      for (c = 0; c < 4; c++) {
          PC_GteProjEntry(3 - c, &sx, &sy, &ir1, &ir2, &ir3, &sz3, &n);
          fprintf(s_squadFile, ",%d,%d,%d", ir2, sz3, n);
      } }
    fprintf(s_squadFile, "\n");
    fflush(s_squadFile);
}

/* VH_FRAME_TIME=1: split each VSync-to-VSync interval into WORK (logic + GTE + raster + present =
 * time from the previous VSync's return to this call) vs IDLE (this call's pacing delay). Mean
 * over 120 frames, tagged with the VSync mode + current battle speed. Complement of
 * VH_RASTER_TIME / VH_PRESENT_TIME: work - raster - present = game-side cost. */
static int s_ftOn = -1; static double s_ftWork, s_ftIdle; static unsigned s_ftN;
static double FtNowMs(void) {
    return (double)SDL_GetPerformanceCounter() * 1000.0 / (double)SDL_GetPerformanceFrequency();
}


/* ---- VSync-driven hooks (pc_etc_internal.h) ---------------------------------------------------- */

/* Entry of every paced VSync: close the previous frame's work/idle interval (VH_FRAME_TIME=1). */
void PC_DiagFrameEntry(int mode) {
    static double lastEntry, idleSum; static int lastMode;
    double now;
    if (s_ftOn < 0) s_ftOn = getenv("VH_FRAME_TIME") ? 1 : 0;
    if (!s_ftOn) return;
    now = FtNowMs();
    if (lastEntry > 0) {
        s_ftWork += (now - lastEntry) - s_ftIdle;  /* everything that wasn't the pacing delay */
        idleSum  += s_ftIdle;
        if (++s_ftN >= 120) {
            double w = s_ftWork / s_ftN;
            fprintf(stderr, "[frame] mode=%d speed=%d work=%.2f idle=%.2f ms/frame "
                            "(work-only ceiling %.1f fps)\n",
                    lastMode, PC_BattleSpeedRaw(), w, idleSum / s_ftN, w > 0 ? 1000.0 / w : 0.0);
            s_ftWork = idleSum = 0; s_ftN = 0;
        }
    }
    lastEntry = now; lastMode = mode; s_ftIdle = 0;
}

/* The pacing delay itself, timed into the idle bucket when VH_FRAME_TIME is on. */
void PC_DiagIdleDelay(unsigned ms) {
    if (s_ftOn == 1) { double d0 = FtNowMs(); SDL_Delay(ms); s_ftIdle += FtNowMs() - d0; }
    else SDL_Delay(ms);
}

/* One call per paced VSync: every opt-in dense-capture CSV emits its per-tick row here. */
void PC_DiagVSyncRows(void) {
    LogCameraTraceRow();
    LogRandSeedRow();
    LogAiChainRow();
    LogCameraMatrixRow();
    LogTerrainRow();
}

/* VH_SMOKE boot harness, PadRead half: hold START through the intro movies so the port's
 * START-skip cuts them short -- the boot smoke reaches the title in ~15s instead of sitting
 * through the FMVs. Alternating press/release gives the skip logic its rising edge. */
unsigned int PC_SmokePadHold(void) {
    static int smoke = -1;
    if (smoke < 0) { const char *e = getenv("VH_SMOKE"); smoke = e && atoi(e) != 0; }
    if (smoke && gState.primary == STATE_MOVIE) {
        static unsigned t; if ((++t / 8) & 1) return PADstart;
    }
    return 0;
}

/* VH_SMOKE boot harness, VSync half (tools/regress/smoke_boot.sh): exit 0 the moment the title
 * screen is reached -- proving the whole boot chain (data-segment constructors, disc mount, MDEC
 * logo movie, SPU/XA init, font, rasterizer) ran; exit 1 on timeout with the stuck state. Runs
 * headless under SDL_VIDEODRIVER=dummy (window-less present is already a no-op). */
void PC_SmokeFrame(void) {
    static int smoke = -1; static unsigned smokeFrames;
    if (smoke < 0) { const char *e = getenv("VH_SMOKE"); smoke = e && atoi(e) != 0; }
    if (!smoke) return;
    smokeFrames++;
    if (gState.primary == STATE_TITLE_SCREEN) {
        /* VH_SMOKE_LINGER=N: keep running N more frames once the title is up, so a recording
         * run (raster_check.sh) captures the title screen actually rendering. Default 0. */
        static int linger = -1; static unsigned atTitle;
        if (linger < 0) { const char *e = getenv("VH_SMOKE_LINGER"); linger = e ? atoi(e) : 0; }
        if (atTitle == 0) atTitle = smokeFrames;
        if (smokeFrames - atTitle >= (unsigned)linger) {
            fprintf(stderr, "SMOKE: reached the title screen after %u frames -- boot chain OK\n", atTitle);
            exit(0);
        }
    }
    if (smokeFrames > 60u * 120u) {
        fprintf(stderr, "SMOKE: TIMEOUT after %u frames (primary=%d) -- boot never reached the title\n",
                smokeFrames, (int)gState.primary);
        exit(1);
    }
}

/* Per-second FPS meter -- opt-in (VH_FPS_LOG=1): a line per second is pure console noise for
 * players, and it drowns the actual signal in the logs bug reports paste. */
void PC_DiagFps(int mode) {
    static int fpsLog = -1;
    if (fpsLog < 0) { const char *e = getenv("VH_FPS_LOG"); fpsLog = e && atoi(e) != 0; }
    if (!fpsLog) return;
    s_fpsLastMode = mode;
    s_fpsWindowCalls++;
    if (s_fpsWindowStart == 0) {
        s_fpsWindowStart = SDL_GetTicks();
    } else {
        Uint32 windowElapsed = SDL_GetTicks() - s_fpsWindowStart;
        if (windowElapsed >= 1000) {
            double fps = s_fpsWindowCalls * 1000.0 / windowElapsed;
            fprintf(stderr, "[FPS] VSync() calls/sec=%.2f (mode=%d) window_ms=%u\n",
                    fps, s_fpsLastMode, windowElapsed);
            s_fpsWindowStart = SDL_GetTicks();
            s_fpsWindowCalls = 0;
        }
    }
}

/* Witness-pass logger (decomp-improvement track): one line per gSpellsEx FX dispatch.
 * Compiled into battle_013b94.c only under `make link SPELLFX_LOG=1`; runtime-gated on
 * VH_SPELLFX_LOG. Writes vh_spellfx_log.txt and echoes to stderr. The handler NAME is
 * resolved from the live function pointer via dladdr (the port links -rdynamic), so the
 * log shows exactly which Objf* symbol ran -- a cast-everything Vandalier session then
 * validates every table-derived FX name in-game. On Windows (no dladdr) the index alone
 * is logged. */
#ifndef _WIN32
#include <dlfcn.h>
#endif
void PC_DiagSpellFxLog(int spellId, int slot, int objf) {
    static int enabled = -1;
    static FILE *f = NULL;
    static const char *slotNames[] = {"main", "target", "defeat"};
    extern s8 gSpellNames[72][21];
    char name[24];
    const char *sym = "?";
    int i, j;
    if (enabled < 0) enabled = (getenv("VH_SPELLFX_LOG") != NULL) ? 1 : 0;
    if (!enabled) return;
    if (f == NULL) {
        f = fopen("vh_spellfx_log.txt", "w");
        if (f == NULL) { enabled = 0; return; }
    }
    /* spell name: 21-byte fixed entries, SJIS full-width spaces (81 40) -> ASCII */
    for (i = 0, j = 0; spellId >= 0 && spellId < 72 && j < 20; i++) {
        u8 c = (u8)gSpellNames[spellId][i];
        if (c == 0) break;
        if (c == 0x81 && (u8)gSpellNames[spellId][i + 1] == 0x40) { name[j++] = ' '; i++; }
        else name[j++] = (char)c;
    }
    name[j] = '\0';
#ifndef _WIN32
    if (objf >= 0 && objf < 804 && gObjFunctionPointers[objf] != NULL) {
        Dl_info info;
        if (dladdr((void *)gObjFunctionPointers[objf], &info) && info.dli_sname != NULL)
            sym = info.dli_sname;
    }
#endif
    fprintf(f, "spell=%2d name=\"%s\" slot=%s objf=%3d handler=%s\n",
            spellId, name, (slot >= 0 && slot <= 2) ? slotNames[slot] : "?", objf, sym);
    fflush(f);
    fprintf(stderr, "[spellfx] %2d %-16s %-6s -> [%3d] %s\n",
            spellId, name, (slot >= 0 && slot <= 2) ? slotNames[slot] : "?", objf, sym);
}
