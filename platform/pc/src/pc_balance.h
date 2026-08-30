/* Tactical Mode: an opt-in rebalance, PC-side only. Every change is gated on gTacticalMode; Normal
 * (0) is byte-for-byte retail. The runtime table patch is restorable + idempotent, and the invariant
 * patchApplied == gTacticalMode is enforced by PC_SyncBalance(), called after ANY mode change. */
#ifndef PLATFORM_PC_BALANCE_H
#define PLATFORM_PC_BALANCE_H

/* 0 = Normal (retail), 1 = Tactical. Read live by the gated src/ hooks (XP cap, Trials, Vandalier,
 * MYSTIC_ENERGY) under #ifdef PC_FEAT, and used as the overlay toggle's value (hence int). */
extern int gTacticalMode;

/* 1 while the player is at the main title menu (gState.primary == STATE_TITLE_SCREEN) -- the only
 * place the Tactical Mode toggle is editable. Lets pc_overlay.c stay game-header-free. */
int PC_AtTitleMenu(void);

/* Per-chapter level cap for the XP-cap and Trials hooks. Returns 50 (the retail cap) in Normal, and
 * the chapter's cap in Tactical with `chapter` clamped to [1,6] (chapter 1 is a valid trial
 * index -> 10). */
int TacticalCap(int chapter);

/* Per-chapter Trial reward parameters (Tactical): each maps gState.chapter (clamped [1,6]) to that
 * chapter's final-battle value from the retail binary. Read only by the PC_FEAT trial hooks, always
 * under gTacticalMode && mapNum <= 5. See docs/tactical-mode.md, "Implementation notes". */
int TrialExpScalingLevel(int chapter);  /* attack-XP base tier */
int TrialEnemyExpMulti(int chapter);    /* attack-XP per-enemy multiplier (retail trial value is 0) */
int TrialGoldReward(int chapter);       /* gold per trial kill (retail flat 10) */
int TrialGoldPenalty(int chapter);      /* gold lost per player-unit death (retail flat 10) */

/* Diagnostic: VH_SPELL_DUMP=1 logs a unit's advancement state + resulting spell list whenever
 * PopulateUnitSpellList runs. Read-only; no behavior change. `spells` = u8[10]. */
void PC_SpellListDump(int name, int cls, int lvl, int pathB, int advF, int advS, const void *spells);

/* Apply or restore the mutable-table balance patch to satisfy  patchApplied == gTacticalMode .
 * Idempotent. Snapshots the pristine tables lazily on first call (which happens post-constructor
 * and before the first patch, so the snapshot captures retail values). Call after every mode change. */
void PC_SyncBalance(void);

/* Load-adopt: before applying a loaded save, adopt the on-disk card's mode from its header marker
 * (padding[0]). Self-heals a hand-moved file (a Tactical card in the Normal folder switches us to
 * Tactical, so its run uses the right tables). No-op when the marker already matches. */
void PC_AdoptSaveMode(void);

/* Apply the boot-default mode from config (VH_TACTICAL in vandalhearts.ini / env), once, at startup.
 * Call from a post-constructor point (first VSync) so the table snapshot sees retail values. */
void PC_BalanceBoot(void);

#endif /* PLATFORM_PC_BALANCE_H */
