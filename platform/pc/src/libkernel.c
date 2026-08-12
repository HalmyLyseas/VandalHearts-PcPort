/*
 * PC backend for PsyQ/kernel.h -- the BIOS event/timer API, plus the
 * memory-card file-I/O functions card.c declares locally (not via any
 * header, matching how the original relies on old-GCC implicit
 * declaration -- see the Kernel step file for the full story).
 *
 * Memory card: "bu00:FILENAME" paths map onto real local files under the
 * saves/ folder (resolved by SaveDir(), next to the exe/AppImage) -- card.c's
 * own block/capacity accounting (BYTES_PER_BLOCK,
 * TOTAL_BLOCKS) is driven entirely by real file sizes via firstfile/
 * nextfile, so this layer only needs honest file existence/size/read/
 * write behavior, not a simulated PS1 card filesystem format.
 *
 * Timer: GetRCnt/ResetRCnt approximate PS1 Root Counter 1's free-running
 * rate using a real elapsed-time clock rather than emulating the exact
 * hardware tick source (HBLANK vs. 1/8 system clock depends on
 * configuration this project doesn't appear to set) -- see the step file
 * for the reasoning behind the chosen rate.
 */
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
#include "pc_balance.h"      /* gTacticalMode -- Tactical saves live in a separate folder (GAP 4) */
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

/* Approximates NTSC Timer1-in-HBLANK-mode's real tick rate. Verified
 * 2026-07-12 against psx-spx.github.io/docs/timers.md and
 * graphicsprocessingunitgpu.md, and cross-checked against BizHawk's
 * octoshock core (psx/gpu.cpp's GPUClockRatio, psx/timer.cpp's
 * TIMER_ClockHRetrace): the precise nominal-NTSC-clock-derived scanline
 * rate is 53,693,181.818 Hz / 3413 video-cycles-per-scanline =~ 15,732.7 Hz
 * -- this constant lands within 0.008% of that, so the *rate* was already
 * right; it's not the source of the bug described below. */
#define RCNT1_HZ 15734.0

/* A global PS1_CPU_SPEED_SCALE multiplier here (tried 2026-07-12, reverted
 * same day) does NOT work: RCntCNT1 is a shared hardware resource with (at
 * least) two independent real call sites with different thresholds and
 * different tolerances for being scaled --
 *   - src/ai.c's IsLagging() (GetRCnt(RCntCNT1) > 450) -- the enemy AI
 *     turn-evaluation throttle this was calibrated against (7x, from
 *     exchange/demoDrift/). Needs slowing down: on real hardware this trips
 *     often enough to spread AI work across ~112 frames; on a modern host
 *     it almost never trips, collapsing the same work into ~16 frames.
 *   - src/graphics.c's incremental unit-sprite decompressor (case 1 of the
 *     sprite-cache decode state machine, ~line 90): gates decode work off
 *     entirely once GetRCnt(RCntCNT1) > 470 while gIsEnemyTurn is set, and
 *     separately bounds each decode burst to a ~256-tick window
 *     (`while (GetRCnt(RCntCNT1) <= maxTicks)`). This runs every frame
 *     (not just during AI turns), and is NOT bottlenecked on a modern CPU --
 *     it already finishes well within its budget. Scaling the counter
 *     multiplies how fast that window closes in real time, starving the
 *     decoder int before it does its share of work.
 * A single global multiplier applied in GetRCnt can't satisfy both: 7x
 * (calibrated for IsLagging()) starves the sprite decoder -- confirmed via
 * exchange/videos/analysis_newExtract_02 (corrupted/half-drawn geometry
 * starting frame ~59 of a fresh capture, and the demo looping forever
 * instead of completing, both consistent with gDecodingSprites never
 * clearing because decode keeps getting cut off mid-stream). On real
 * hardware this isn't a conflict because the timer reflects genuine elapsed
 * CPU cost for whatever code is running, so both consumers get naturally
 * correct pacing "for free" -- a synthetic wall-clock scale can't replicate
 * that without knowing which caller (and which workload) is asking.
 *
 * Conclusion: fixing IsLagging()'s pacing needs a per-call-site (or a real
 * per-instruction synthetic cycle-cost) model, not a change to the shared
 * GetRCnt primitive -- see exchange/12-phase-c-bootstrap.md and the
 * followup_ai_battlemgr_timing memory entry for the fix-option writeup.
 * GetRCnt/ResetRCnt below are back to a plain, unscaled wall-clock
 * implementation (RCNT1_HZ is independently confirmed accurate -- see
 * above -- so this is a correct, if not real-hardware-paced, baseline). */

static struct timespec s_rcntStart[2];

/* ---- AI throttle decoupling (2026-07-12, Option B revised) -----------
 * IsLagging() (src/ai.c:300, GetRCnt(RCntCNT1) > 450) is checked from
 * inside 7 enemy/ally turn-evaluation state machines (Objf570/400/401/
 * 402/403/404/589_AI_TBD) -- confirmed by grep to be IsLagging()'s only
 * callers, and to be the only functions with direct
 * `GetRCnt(RCntCNT1) > 450` checks too. Tracing the actual per-frame call
 * order in src/engine.c's UpdateEngine() showed RCntCNT1 is a genuine
 * whole-frame-elapsed clock (ResetRCnt happens once, at the END of each
 * frame, after Obj_Execute() and DecodeUnitSprites() both already ran) --
 * so a synthetic model that tried to faithfully reconstruct "real elapsed
 * time" would need to cost RenderField(), every active object, and the
 * sprite decoder too, not just these 7 functions. That's intractable.
 *
 * Instead, borrowing the core idea from ctr-native's GetRCnt (a sibling
 * native PS1 decomp PC port for Crash Team Racing): decouple the counter
 * from host wall-clock speed entirely, so host CPU speed cannot affect it.
 * ctr-native does this coarsely (fixed ticks per real vblank); IsLagging()
 * needs sub-frame resolution, so instead: any GetRCnt(RCntCNT1) call whose
 * return address falls inside IsLagging() or one of the 7 functions above
 * gets a separate synthetic counter that advances by a fixed amount per
 * *checkpoint visit* (not per unit of real time) -- immune to host speed
 * by construction, same as ctr-native's approach, just keyed to call
 * count instead of vblank count. Every other caller (src/graphics.c's
 * sprite decoder, src/engine.c's debug FntPrint) falls through to the
 * original, untouched, unscaled wall-clock path below -- so this cannot
 * repeat the 2026-07-12 regression where a global scale starved the
 * decoder (exchange/videos/analysis_newExtract_02).
 *
 * Caller identification uses __builtin_return_address(0) (always reliable
 * -- provided directly by the `call` instruction, no frame-pointer stack
 * walking needed) against each function's real compiled address range.
 * IsLagging() itself needs no separate unwinding to find its own caller:
 * grep confirms it has no callers outside these 7 functions, so "called
 * via IsLagging()" already implies "AI-related" without knowing which of
 * the 7 functions or which case triggered it -- adequate for the uniform
 * per-visit increment used below; a future per-checkpoint weighting would
 * need __builtin_return_address(1), not attempted here (see writeup).
 *
 * 2026-07-12, Option B: per-checkpoint-visit weighting.
 * AI_CHECKPOINT_TICKS_PER_VISIT (a flat 200 for every checkpoint) was a
 * first-pass calibration that got the median cycle roughly right (~7.5x
 * too slow, matching the original demoDrift 7x data point) but couldn't
 * fix the underlying 2.44x-12.02x spread across 14 real-vs-ours cycles
 * measured via full-demo BizHawk-vs-gdb comparison -- a single constant
 * can't track that different checkpoints gate genuinely different amounts
 * of real work. Replaced with real per-checkpoint costs derived from the
 * project's own byte-exact MIPS assembly (build/src/ai.c.s and
 * build/src/path_grids.c.s, the actual old-GCC output verified byte-exact
 * against the original binary in stage 1 -- not a reconstruction).
 *
 * Of the ~35 IsLagging()/GetRCnt() call sites in ai.c, ~24 go through
 * IsLagging() (straight-line case-boundary checks -- confirmed via
 * objdump that IsLagging() is a real non-inlined function, so they all
 * share ONE return address from GetRCnt's perspective and get a single
 * shared "case-boundary" weight) and ~10 call GetRCnt(RCntCNT1) directly
 * (each individually addressable via __builtin_return_address(0), no
 * frame-unwinding needed). Of those 10, 8 form 4 matched "A/B" pairs in
 * Objf402/403/404/589_AI_TBD's per-map-tile scanning loops: an inner
 * per-tile checkpoint (A, fires only for tiles matching a criteria) and
 * an outer per-row checkpoint (B, fires once per row regardless).
 *
 * Costs, in real instructions (converted to synthetic ticks below):
 *  - Case-boundary (shared bucket, ~24 sites via IsLagging()): 25,
 *    representative of the measured range (10-45 instrs across all of
 *    them) for simple straight-line state-transition bodies.
 *  - Objf400 case 2 (ai.c:402): 15 -- per-tile checkpoint, but the two
 *    branches (array lookup+assign vs. nothing) differ negligibly, so a
 *    flat weight is accurate, not just convenient.
 *  - Objf400 case 1 (ai.c:376): NOT given its own weight -- it goes
 *    through IsLagging() (confirmed via objdump: `call IsLagging` at
 *    this exact line), so it shares the case-boundary bucket above.
 *    Deliberately deferred: unlike the A/B pairs below, this checkpoint
 *    fires once per tile with no separate lump-sum checkpoint to offload
 *    the "skipped tile" cost onto, so its true cost is genuinely
 *    branch-dependent per visit -- approximating it as case-boundary-cost
 *    undercounts when the expensive branch is taken, but avoids a worse
 *    guess. Revisit if validation shows this cycle-type is still off.
 *  - Objf401 (ai.c:639): 41 (ClearGrid) + 816 (AccumulateProximityGrid, its actual
 *    measured size -- much larger than the other helpers) + ~10 call
 *    overhead = 867. Fires only for team-differing units, bounded by
 *    UNIT_CT (~40), so the "how many skipped" variance is small in
 *    absolute terms even though technically data-dependent.
 *  - Objf402/403 checkpoint A (785/1039): overhead(10) + the matching
 *    helper (PopulateCastingGrid=248 for 402; avg(PopulateMeleeAttackGrid
 *    =94, PopulateRangedAttackGrid=114)=104 for 403 -- can't tell which
 *    branch at runtime without knowing which object is executing, which
 *    GetRCnt has no visibility into, so this averages them rather than
 *    guessing) + the inner iz2/ix2 scan, which the source shows always
 *    runs the FULL map area (not data-dependent in trip count) with a
 *    cheap per-cell test (~12 instrs) gating an occasional expensive
 *    per-cell scorer call (AI_ScoreCastingPosition=57 for 402, AI_ScoreAttackOption=177
 *    for 403) -- assumed 10%-of-cells match (a documented, revisable
 *    guess, not hidden inside a bigger unexplained number) since that's
 *    the one genuinely unknowable-without-deeper-instrumentation piece.
 *  - Objf402/403 checkpoint B (821/1078): mapWidth x 12 -- the outer
 *    loop's per-tile cheap-test cost, accumulated across one row. This
 *    is now a clean, separate cost from A's (no more double-counting or
 *    guessing at a "lump sum" split) since A only fires -- and only
 *    bills its own cost -- when a tile actually matches.
 *  - Objf404/589 checkpoint A (1236/1393): 15 -- per-tile, but the body
 *    is pure fixed-cost arithmetic (no helper call, no nested loop), so
 *    same reasoning as Objf400 case 2.
 *  - Objf404/589 checkpoint B (1258/1413): mapWidth x 12, same pattern
 *    as Objf402/403's B.
 *
 * mapWidth/mapArea are read live from gMapMinX/gMapMaxX/gMapMinZ/gMapMaxZ
 * (include/field.h externs, no src .c changes needed, same category as
 * reading gState/gObjectArray for earlier RAM-watch diagnostics) so the
 * cost genuinely scales with each battle's real map size instead of
 * assuming one.
 *
 * Instruction counts convert to synthetic ticks via INSTRUCTIONS_PER_TICK
 * = R3000A clock / RCNT1_HZ = 33,868,800 / 15,734 ~= 2153 (~1 cycle per
 * instruction, consistent with how RCNT1_HZ itself was derived and
 * verified against psx-spx/octoshock earlier). Accumulated as raw
 * instruction-equivalent units internally (s_aiSyntheticInstrs) and only
 * divided down to ticks when GetRCnt() is actually queried, to avoid
 * losing individually-small per-visit weights to integer truncation. */
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

/* Sizes taken from `nm -S build/src/ai.o` against the current src/ai.c --
 * stable across relinks (only the absolute base address, read here via
 * &Func at runtime, depends on final link layout), but WILL go stale if
 * any of these 8 functions' compiled code changes size. Regenerate via:
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

/* Cumulative GetRCnt(RCntCNT1) visit counts per AI function-range (order matches the table above:
 * 0=IsLagging 1=Objf570 2=Objf400 3=Objf401 4=Objf402 5=Objf403 6=Objf404 7=Objf589). Used to
 * CALIBRATE option-2 per-function costs: cost_per_visit = 450*HW_target_frames / visits_per_scan.
 * Diff these across an AI phase (bracket by the AI-chain log's frames) to get visits_per_scan.
 * Exposed for the ai-chain CSV; the increment is trivial so it stays always-on. */
int g_aiVisitCount[NUM_AI_THROTTLE_RANGES] = {0};

/* (Retired 2026-07-14: the per-checkpoint `s_checkpointAddrs[]` table of hardcoded LINK-TIME return
 * addresses + `DirectCheckpointCost()` — they never matched under PIE and are replaced by the
 * per-function tick model below. Option 1 will reintroduce per-checkpoint granularity PIE-correctly.) */

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

/* ---- AI throttle: option-2 per-function synthetic TICK budget (2026-07-14) -----------
 * Replaces the previous per-checkpoint INSTRUCTION model, which was doubly broken:
 *  (1) its per-checkpoint costs were applied via `s_checkpointAddrs[]` hardcoded LINK-TIME
 *      addresses compared against runtime `__builtin_return_address(0)` -- under the PIE binary
 *      these NEVER matched, so every AI GetRCnt fell to a flat 25-instr case-boundary cost; and
 *  (2) even applied, instruction counts are ~100x too small vs the 450-tick frame budget, because
 *      on real HW most of that budget is eaten by non-AI frame work (RenderField/objects/decoder)
 *      BEFORE the AI runs -- an intractable-to-model baseline.
 * This model sidesteps both: give each AI FUNCTION one flat synthetic-tick cost per GetRCnt visit,
 * CALIBRATED from measured visits-per-scan vs the BizHawk frame targets:
 *     cost_per_visit = 450 ticks * HW_target_frames / measured_visits_per_scan
 * The baseline is absorbed into the constant, and because both visits and target-frames scale with
 * map size, a flat per-function constant is ~map-invariant (generalizes to other maps).
 * Calibration data (exchange/42-timing-ai-tick-drift.md, VH_AI_LOG visit counts vs 43/44 HW traces):
 *   Objf403 (move-scoring, dominant): 561 visits/76fr (Bahamut) & 320/53 (C.Archer) => 61..74 -> 67
 *   Objf401 (threat-map):             14 visits/16fr => ~500 (yields ~1 visit/frame; 14-frame floor)
 *   IsLagging (case boundaries):      must stay tiny so straight-line state checks never gate.
 * Ranges 400/402/404/589 aren't exercised by the demo -> seeded from their sibling Objf403 (~67)
 * pending per-map calibration. Order matches s_aiThrottleRanges[]:
 *   0=IsLagging 1=Objf570 2=Objf400 3=Objf401 4=Objf402 5=Objf403 6=Objf404 7=Objf589
 * REFINEMENT (option 1, later): split the flat per-function cost back into PIE-CORRECT per-checkpoint
 * costs (anchored to &Func + within-func delta, not raw link literals) to remove the ~±10%
 * melee-vs-ranged residual seen in Objf403. */
/* NB targets are in GAME-UPDATES (30Hz, VSync(2)) = HW-vblank-frames / 2. The AI state machine steps
 * once per update, so calibrate to updates, not the raw BizHawk framecounts (76/16 vbl -> 38/8 updates).
 * Re-derived empirically 2026-07-14: at cost 67/500 the phases ran 111/14 updates -> scale by
 * target/measured (38/111, 8/14). Yields after ceil(450/cost) visits per update. */
static const double s_aiThrottleTicks[NUM_AI_THROTTLE_RANGES] = {
    5.0,     /* 0 IsLagging  -- case-boundary checks, negligible (must not gate) */
    5.0,     /* 1 Objf570    -- orchestrator, few visits */
    30.0,    /* 2 Objf400    -- not in demo; sibling default (= Objf403) */
    285.0,   /* 3 Objf401    -- threat-map; validated 8 updates == HW target 8 */
    42.0,    /* 4 Objf402    -- caster eval (Dark Mage/Eleni/Zohar/Dumas). Calibrated 2026-07-14 from
              *                VH_AI_LOG Dark Mage turn: v402 341 visits + vIsLag 606(@5) + v401 14(@285);
              *                at cost 30 the AI-chain ran 45 updates but HW needs ~54.5 (feedback-27 Dark
              *                Mage +19 undershoot) -> +~21% tick-work via Objf402 alone -> 30->42. Only
              *                casters use Objf402, so this is isolated to them. option-1 first pass;
              *                validate on Dark Mage, then generalize to Eleni/Zohar/Dumas. */
    30.0,    /* 5 Objf403    -- move-scoring; linear fit (67->113, 23->44 updates) => 30 for ~54-update
              *                whole phase (HW ~54). Bahamut ~+6%, C.Archer ~-6% (melee/ranged residual
              *                = the option-1 per-checkpoint refinement target). */
    30.0,    /* 6 Objf404    -- not in demo; sibling default */
    30.0,    /* 7 Objf589    -- not in demo; sibling default */
};

static double s_aiSyntheticTicks[2];

/* ---- RCnt1 frame normalization (opt-in: VH_RCNT1_NORMALIZE=1) --------------------------
 *
 * OFF BY DEFAULT. The honest wall-clock counter below is the calibrated behaviour and stays
 * the default so the AI/timing work (exchange/42) is untouched.
 *
 * The problem this exists for: src/graphics.c's incremental sprite decoder gates itself with
 * `if (!gIsEnemyTurn || GetRCnt(RCntCNT1) <= 470)`. RCnt1 is the HBlank counter and 470 ticks
 * at RCNT1_HZ is ~30 ms, so on real hardware -- where a frame IS 16.7 ms -- the gate can never
 * trip: the counter only reaches ~262 by end of frame. The game code is written assuming that.
 * Our wall-clock implementation makes it host-speed-dependent instead: at ~12 FPS (an 83 ms
 * frame, which is what the 32-bit ASAN build gets in a battle scene) the counter passes 470
 * thirty milliseconds in, so during an enemy turn the decoder is gated off EVERY frame,
 * gDecodingSprites never clears, and the enemy turn never begins -- the camera pans forever.
 * Confirmed 2026-07-21 by SIGUSR1 sampling a stuck ASAN run: the render loop was alive and
 * moving (DrawOTag / RenderMapTile / VSync) while gState was frozen bit-identical across 30 s.
 * This is the same failure the long comment at the top of this file records from the 7x-GetRCnt
 * experiment ("the demo looping forever ... gDecodingSprites never clearing"), reached by a
 * different route: real slowness rather than a synthetic multiplier.
 *
 * The fix, when enabled: scale elapsed time by (nominal frame / PREVIOUS frame duration), so the
 * counter sweeps the same 0..~262 range per frame regardless of how long the frame really took --
 * i.e. frame-relative, which is what RCnt1 physically is on hardware (a position within the
 * frame). ResetRCnt(RCntCNT1) is called exactly once per frame (src/engine.c:76), so the gap
 * between consecutive resets IS the previous frame duration: this is self-tuning, with no
 * host-specific factor to guess.
 *
 * Clamped to <= 1.0 so it can only ever compensate for a host running SLOWER than 60 FPS; at or
 * above target the scale is 1.0 and this is a no-op. The counter still advances monotonically
 * within a frame, which matters: the decode burst loop is `while (GetRCnt(RCntCNT1) <= maxTicks)`
 * and would never terminate if the counter were simply clamped to a constant. */
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
     * VH_RCNT1_NORMALIZE makes it frame-relative -- see the comment on ResetRCnt above. */
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

/* The saves directory, resolved once and cached. Prefer a "saves" folder NEXT TO the executable /
 * .AppImage (via PC_GetDeployDir), so it's the same predictable place as the disc regardless of the
 * working directory the game was launched from -- fixes saves silently landing in $HOME/saves when
 * an AppImage is double-clicked. Resolution order:
 *   1. <deploy>/saves already exists          -> use it (established location)
 *   2. legacy cwd-relative "saves" exists      -> use it (don't orphan pre-1.1 saves)
 *   3. create <deploy>/saves                   -> use it (the new default)
 *   4. deploy dir unusable/read-only           -> fall back to cwd-relative "saves"           */
static const char *SaveDir(void) {
    static char cached[PATH_MAX + 32];
    static int cachedMode = -1;
    char deploy[PATH_MAX];   /* cached[] below carries +32 headroom for the "/saves*" suffix */
    struct stat st;
    /* Mode isolation (GAP 4): Tactical saves live in "saves_tactical/", Normal in "saves/". Re-resolve
     * when the mode changes (it only changes at the title menu, so no save op is ever in flight). */
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

/* On real hardware these three kick off ASYNC memory-card BIOS operations whose completion fires a
 * card interrupt that signals a SwCARD/HwCARD event; card.c's Card_CheckState() then blocks in
 * Card_WaitForSw/HwCardEvent() (a while(1) polling TestEvent) until one arrives. Our virtual card
 * is synchronous and always present, so we signal the I/O-complete event (EvSpIOE) right here --
 * WITHOUT it the wait-loops spin forever and the game FREEZES the instant New Game / Load Game
 * touch the card (Options never does, hence it worked). EvSpIOE drives Card_CheckState() to
 * gCardState=-1 ("card ready"), so file ops proceed. (SignalCardEvent marks both the Sw and Hw
 * IOE events, covering Card_WaitForHwCardEvent too.) */
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
    /* Not exercised by any test yet -- left as a documented no-op rather
     * than speculatively deleting real save data under the saves/ folder. */
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

    /* A creating open needs the saves folder to exist -- InitCard made it at startup, but the user
     * can delete it mid-session (or it may never have been created if the deploy dir only became
     * writable later). Re-create it here so an in-battle save can't fail with "Save unsuccessful"
     * just because the directory is missing. (No-op when it already exists.) */
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

/* PS1 BIOS kanji charset 2, glyph bitmaps 0..208 (pc_kanji_font.c, generated
 * from PsyQ KROMDAT.BIN by tools/gen_kanji_font.py). 30 bytes per glyph. */
extern const unsigned char pc_kanji_charset2[6270];

/* Map a full-width Shift-JIS code to its glyph index within charset 2. The
 * BIOS packs the charset compacted (undefined codes skipped), so these anchors
 * were recovered empirically -- see gen_kanji_font.py. Covers every SJIS code
 * Vandal Hearts actually draws (space, period, 0-9, A-Z, a-z) PLUS the rest of
 * kuten row 1, which the langpack subtitle renderer (pc_lang.c AsciiToWideSjis)
 * relies on for punctuation -- keep the row-1 span when touching this. Anything
 * else returns -1 (blank), same as the game's own out-of-range guard. */
static s32 sjis_to_krom_glyph(u32 sjis) {
    /* Kuten row 1 (0x8140-0x817C) is packed LINEARLY in charset 2: the empirically recovered
     * anchors (space=0, period=4, plus=59, minus=60) all satisfy index == sjis - 0x8140, which
     * proves no undefined-code skips inside that span. Serving the whole span gives the
     * subtitle renderer its punctuation (comma, quotes, ?, !) from the built-in font. */
    if (sjis >= 0x8140 && sjis <= 0x817c) return (s32)(sjis - 0x8140);
    if (sjis == 0x8194) return 65;                                /* #      */
    if (sjis >= 0x824f && sjis <= 0x8258) return 147 + (sjis - 0x824f); /* 0-9 */
    if (sjis >= 0x8260 && sjis <= 0x8279) return 157 + (sjis - 0x8260); /* A-Z */
    if (sjis >= 0x8281 && sjis <= 0x829a) return 183 + (sjis - 0x8281); /* a-z */
    return -1;
}

/* Emulates BIOS call B(51h) Krom2RawAdd: returns a pointer to the 30-byte glyph bitmap for a
 * Shift-JIS code, or -1 on error -- exactly what src/text.c's DrawSjisGlyph() expects. Without
 * this every DrawSjisText() (TURN counter, PLAYER/ENEMY TURN banner, item names, gold, party
 * lists) rendered nothing on the PC port.
 *
 * Stage 2.3: the return type was `s32` "matching the -m32 pointer width", which truncated the
 * address to 32 bits under -m64. Both callers immediately dereference it
 * (`u8 *p = Krom2RawAdd(...)`), so the 64-bit build crashed in DrawSjisGlyph the first time a
 * battle menu was drawn. Returning `void *` is correct at both widths and needs no src/ change:
 * the cast in text.c and the implicit assign in window.c both still work, as does their
 * `== -1` sentinel test. */
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

/* rand() is declared in common.h as a plain extern, matching how the
 * decompiled game code reaches it via old-GCC implicit declaration -- but on
 * real hardware it isn't a C library function at all, it's BIOS misc
 * function A(2Fh): "x = x*41C64E6Dh + 3039h; return (x>>16) & 7FFFh"
 * (psx-spx kernelbios.md, "A(2Fh) - rand()"). Confirmed as the actual root
 * cause of the demo battle (gState.mapNum==8) running ~9-10x too fast: real
 * gameplay logic reads rand() to intentionally randomize battle-unit team
 * assignments per-demo (src/game_setup.c's SetupBattleUnit), so if this
 * doesn't reproduce the BIOS's exact sequence, every unit's team ends up
 * wrong and the AI never finds a valid attack target. Linking against the
 * host's glibc rand() (the default before this fix) is deterministic too,
 * but glibc's algorithm has nothing to do with the PSX BIOS's, so the
 * *values* differ even though both are seedless/repeatable. Same fix CTR-native
 * (this project's other native-PC-port reference) already validated
 * (ctr-native/game/MixRNG/PSX_BIOS_Rand.c) against the real BIOS stub at
 * 0x80078be8. The game itself never calls srand() (confirmed via grep), so
 * the initial seed is whatever the BIOS's own default is.
 *
 * 2026-07-12 correction: that default is 0, not 1. exchange/
 * 24-find-rand-seed-address.lua found real hardware's live seed empirically
 * (RAM address 0x80009010, verified via the exact LCG signature across a
 * real tick) and exchange/25-rand-seed-full-watch-log.lua's full-demo trace
 * showed it starts at 0x00000000 and its first-ever transition (frame 1771)
 * is exactly 0x00003039 -- precisely `0*0x41C64E6D + 0x3039`, only consistent
 * with a starting seed of 0. CTR-native's choice of 1 was apparently never
 * cross-checked against this address; not re-litigated here, just not
 * followed. See exchange/20-camera-viewport-coordinates.md's "ROOT CAUSE
 * FOUND" section for the full comparison (our build's first rand() call
 * landed at frame 15 with the wrong seed=1 default, ~1750 frames before real
 * hardware's actual first call -- compounding with the CD-seek-jitter bug
 * fixed alongside this in platform/pc/src/libcd.c). */
static u32 s_randSeed = 0;

s32 rand(void) {
    s_randSeed = s_randSeed * 0x41c64e6dU + 0x3039U;
    return (s32)((s_randSeed >> 16) & 0x7fffU);
}

void srand(u32 seed) {
    s_randSeed = seed;
}

/* Debug-only accessor (2026-07-12, exchange/20-camera-viewport-coordinates.md's "ROOT CAUSE
 * FOUND" section) -- lets libetc.c's per-tick trace logger observe s_randSeed without changing
 * its linkage/visibility for anything else. Used to directly compare this PRNG's state against
 * real hardware's (found empirically at RAM address 0x80009010 via exchange/
 * 24-find-rand-seed-address.lua's LCG-signature scan) and confirm/deny whether
 * platform/pc/src/libcd.c's CD-seek-jitter simulation desyncs it from the shared game-logic
 * stream. */
u32 GetRandSeedForDebug(void) {
    return s_randSeed;
}
