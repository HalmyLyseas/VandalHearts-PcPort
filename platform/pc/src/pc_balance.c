/*
 * Stage-3 1.3 "Tactical Mode" balance package. See pc_balance.h and exchange/65.
 *
 * Design: a single restorable, idempotent patch over mutable global tables. Each patched location is
 * a (address, size, tactical-value) record with a snapshot of the pristine bytes. Apply writes the
 * tactical values; restore writes the snapshot back. The snapshot is taken lazily on the first sync
 * (tables are still retail-pristine at that point -- these are read-only config tables the game never
 * mutates), which avoids depending on constructor ordering vs generated_data.c.
 *
 * This file (a PC backend) is never compiled into the byte-exact matching build, so it cannot affect
 * `make check`. The gated logic hooks that READ gTacticalMode / TacticalCap live in src/ behind
 * #ifdef PC_FEAT and are added separately.
 */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "units.h"
#include "state.h"
#include "card.h"          /* gCardFileHeader, CardFileData_Header (padding[28]) */
#include "audio.h"         /* PerformAudioCommand, AUDIO_CMD_STOP_ALL (GAP 8 return-to-title) */
#include "battle.h"        /* gIsEnemyTurn */
#include "pc_platform.h"   /* PC_SaveDir */
#include "pc_balance.h"

int gTacticalMode = 0;

int PC_AtTitleMenu(void) {
    return gState.primary == STATE_TITLE_SCREEN;
}

/* bugreport-05 diagnostic (VH_SPELL_DUMP=1): log a unit's advancement fields + final spell list from
 * PopulateUnitSpellList. Read-only. Helps see why a Ninja's list ends empty (advFirst==0 + GAP 11). */
void PC_SpellListDump(int name, int cls, int lvl, int pathB, int advF, int advS, const void *spells) {
    static int en = -1;
    const unsigned char *sp = (const unsigned char *)spells;
    int i, cnt = 0;
    if (en < 0) en = getenv("VH_SPELL_DUMP") ? 1 : 0;
    if (!en) return;
    for (i = 0; i < 10; i++) if (sp[i] != 0) cnt++;
    fprintf(stderr, "[spelldump] name=%d class=%d lvl=%d pathB=%d advFirst=%d advSecond=%d n=%d spells=[",
            name, cls, lvl, pathB, advF, advS, cnt);
    for (i = 0; i < 10; i++) fprintf(stderr, "%d ", sp[i]);
    fprintf(stderr, "]\n");
}

void PC_ReturnToTitle(void) {
    /* GAP 8: jump straight to the title menu from anywhere, skipping the intro videos, by replicating
     * the game-over teardown (battle_eval.c). The title -> New/Load flow re-establishes run state, so
     * leftover party/chapter/objects don't need clearing. Mode stays as-is (toggle editable at title). */
    PerformAudioCommand(AUDIO_CMD_STOP_ALL);
    gIsEnemyTurn = 0;
    gState.primary   = STATE_TITLE_SCREEN;
    gState.secondary = 0;
    gState.state3    = 0;
    gState.state4    = 0;
    gState.state7    = 1;
    PC_SyncBalance();   /* keep patchApplied == gTacticalMode consistent */
}

/* GAP 4 Layer 2 -- card-header mode marker in the free padding[28] (offset 68 in the header, which is
 * at file offset 0). 'T' = Tactical, 0 = Normal. The game never touches padding (verified in card.c),
 * so the stamp rides inside the save and stays hardware-valid. */
#define TAC_MARK 0x54   /* 'T' */

extern CardFileData_Header gCardFileHeader;   /* declared in card.c; defined in generated_data.c */

static void stampSaveMarker(void) {
    gCardFileHeader.padding[0] = (unsigned char)(gTacticalMode ? TAC_MARK : 0);
}

void PC_AdoptSaveMode(void) {
    char path[PATH_MAX];
    FILE *f;
    unsigned char b = 0;
    int diskTactical;
    snprintf(path, sizeof(path), "%s/BASLUS-00447VH", PC_SaveDir());
    f = fopen(path, "rb");
    if (f) {
        if (fseek(f, 68, SEEK_SET) == 0 && fread(&b, 1, 1, f) != 1) b = 0;
        fclose(f);
    }
    diskTactical = (b == TAC_MARK);
    if (diskTactical != gTacticalMode) {   /* hand-moved file: adopt the card's true mode */
        gTacticalMode = diskTactical;
        PC_SyncBalance();
    }
}

int TacticalCap(int chapter) {
    /* [0] is a dead clamp slot; real caps are 1..6 = {10,15,20,25,27,30} (GAP 1/2, finalized). */
    static const int cap[7] = { 10, 10, 15, 20, 25, 27, 30 };
    if (!gTacticalMode) return 50;              /* Normal: the retail cap. */
    if (chapter < 1) chapter = 1;
    if (chapter > 6) chapter = 6;
    return cap[chapter];
}

/* --- GAP 9/10: Trial Rewards (Tactical) --------------------------------------------------------
 * Design (user 2026-07-26): a cleared Trial pays like *that chapter's final battle*. Values are the
 * chapter-final battle's real numbers, read straight from the byte-exact binary (see
 * exchange/features1.3/trialsReward): expScalingLevel + regular-enemy expMulti drive attack XP;
 * reward is per-kill gold; penalty is gold lost per player-unit death (careless runs now cost).
 * Bosses use the regular value (no 2x tier) -- user's call, "mental-battle" framing. Every value is
 * indexed by gState.chapter (one trial map replays across chapters), clamped to [1,6]. These are read
 * ONLY from the PC_FEAT trial hooks, always under a gTacticalMode && mapNum<=5 guard. */
#define CLAMP_CH(c) ((c) < 1 ? 1 : ((c) > 6 ? 6 : (c)))
int TrialExpScalingLevel(int chapter) { static const int v[7] = { 9,  9, 14, 17, 21, 26, 29 }; return v[CLAMP_CH(chapter)]; }
int TrialEnemyExpMulti(int chapter)   { static const int v[7] = { 12, 12, 15, 16, 15, 12, 21 }; return v[CLAMP_CH(chapter)]; }
int TrialGoldReward(int chapter)      { static const int v[7] = { 170,170,330,660,1040,1820,2700 }; return v[CLAMP_CH(chapter)]; }
int TrialGoldPenalty(int chapter)     { static const int v[7] = { 110,110,220,440,700,1200,1800 }; return v[CLAMP_CH(chapter)]; }

/* --- Restorable, idempotent scalar-table patch -------------------------------------------------- */

typedef struct {
    void          *addr;      /* target location in a mutable global table */
    unsigned char  size;      /* 1/2/4/8 bytes for scalars; up to sizeof(orig) for string patches */
    unsigned long  tac;       /* scalar Tactical value (size<=8; host is little-endian) */
    const void    *tacptr;    /* byte/string source when size>8 (addStrPatch); NULL for scalars */
    unsigned char  orig[24];  /* pristine snapshot */
} Patch;

#define MAX_PATCH 160
static Patch s_patch[MAX_PATCH];
static int   s_nPatch  = 0;
static int   s_inited  = 0;
static int   s_applied = 0;

static void addPatch(void *addr, unsigned char size, unsigned long tac) {
    Patch *p;
    if (s_nPatch >= MAX_PATCH) return;
    p = &s_patch[s_nPatch++];
    p->addr = addr;
    p->size = size;
    p->tac  = tac;
    p->tacptr = 0;
    memcpy(p->orig, addr, size);   /* snapshot the pristine bytes now (tables untouched pre-patch) */
}

/* In-place fixed-width string patch (e.g. gSpellNames[] -- a char[][21] array, NOT a pointer table,
 * so it can't be repointed like gSpellDescriptions). Copies s (incl. NUL) over addr; restorable. */
static void addStrPatch(void *addr, const char *s) {
    Patch *p;
    unsigned int n = (unsigned int)strlen(s) + 1;
    if (s_nPatch >= MAX_PATCH || n > sizeof p->orig) return;
    p = &s_patch[s_nPatch++];
    p->addr = addr;
    p->size = (unsigned char)n;
    p->tac  = 0;
    p->tacptr = s;
    memcpy(p->orig, addr, n);
}

/* gItemDescriptions is declared in units.h (s8*); gItemDescriptions2 only in supplies.c. Both defined in
 * the generated pc_item_descriptions.c with the RETAIL strings; we repoint them in Tactical only. */
extern u8 *gItemDescriptions2[101];


/* GAP 6: point an item's description (both the single-line and shop tables) at `flavor` in Tactical.
 * `flavor` is original authored text (static storage), NOT extracted ROM text. Normal keeps retail. */
static void addDescSwap(int id, const char *flavor) {
    addPatch(&gItemDescriptions[id],  sizeof(char *), (unsigned long)(uintptr_t)flavor);
    addPatch(&gItemDescriptions2[id], sizeof(char *), (unsigned long)(uintptr_t)flavor);
}

/* bugreport-03: the spell-info bar (window.c:2292) draws gSpellDescriptions[] -- a BAKED string with
 * Rng/Fld/MP hardcoded, NOT read from gSpells. Balance patches change the real gSpells fields (gameplay
 * is correct) but the display string stays stale. Repoint the reworked spells' info line in Tactical. */
static void addSpellDescSwap(int id, const char *s) {
    addPatch(&gSpellDescriptions[id], sizeof(char *), (unsigned long)(uintptr_t)s);
}

/* Build the patch list + snapshot originals, once. Lazily invoked by PC_SyncBalance so it runs
 * after generated_data.c has populated the tables. */
static void ensureInit(void) {
    /* C4 -- Guardsman/Dragoon mobility (step). Player Armored units: Clint/Grog/Dolan, both forms.
     * step indexes a whole movement profile (MOVE + climb + terrain cost), see docs/game-mechanics/
     * classes.md. Goal: keep the Armored line's role but fix its impractical mobility.
     *   Guardsman -> Bowman profile (step 5): MOVE stays 5, climb +1 -> +2 (better verticality only).
     *   Dragoon   -> Sniper profile (step 6): MOVE 5 -> 6 and climb +2. */
    static const int GUARDSMAN[] = { 26, 31, 32 };            /* step 1 -> 5 (Bowman) */
    static const int DRAGOON[]   = { 50, 55, 56 };            /* step 4 -> 6 (Sniper) */
    /* A1 -- Monk/Ninja magic resistance. Player caster-path-B units (Eleni/Huxley/Sara/Zohar). */
    static const int MONKNINJA[] = { 28, 29, 34, 35, 52, 53, 58, 59 };  /* magSusc 3 -> 2 */
    int i;

    if (s_inited) return;
    s_inited = 1;

    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[GUARDSMAN[i]].step, 1, 5);   /* Bowman profile */
    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[DRAGOON[i]].step, 1, 6);     /* Sniper profile */
    for (i = 0; i < 8; i++) addPatch(&gUnitInfo[MONKNINJA[i]].magicSusceptibility, 1, 2);

    addPatch(&gClassMpMultiplier[CLASS_MONK], 1, 2);                      /* A3  MP mult 1 -> 2 */
    addPatch(&gItemEquipmentPower[ITEM_P_CLAWS], 1, 12);                  /* B3  P.claws 10 -> 12 */
    addPatch(&gItemEquipmentPower[ITEM_D_CLAWS], 1, 13);                  /* B3  D.claws 12 -> 13 */

    /* B -- Monk/Ninja + mage caster-spell rework (exchange/features1.3/spellsRework, 2026-07-28).
     * The melee-caster tax is repaid in spell efficiency so the Monk mage-path is a real alternative,
     * not a trap. Two swaps make spells 13/26 class-EXCLUSIVE, so their stats+reqLv tune freely:
     *   - Monk (path B): STONE_SHOWER(26) -> real SPREAD_FORCE(13) -- better FX, self-centred melee AOE.
     *   - Mage (path A): SPREAD_FORCE(13) -> THUNDER_BALL (copied into learnable slot 26; ranged nuke).
     * STONE_SHOWER is discarded in Tactical only -- no enemy/item/code casts it (gUnitInfo scan clean).
     * Monk damage spells follow "1 power = 1 mana". */

    /* Monk gets Spread Force(13): nerfed, now Monk-exclusive, unlocks at 12 (Stone Shower's old slot). */
    addPatch(&gSpells[13].power, 1, 8);  addPatch(&gSpells[13].mpCost, 1, 8);   /* 13/7 -> 8/8 (rng0 fld3 kept) */
    addPatch(&gSpellLevelRequirement[13], 1, 12);                              /* was 21 (mage tier) -> Monk @12 */

    /* Slot 26 repurposed as THUNDER_BALL for the mage: long-range small AOE, keeps old Spread Force pow 13.
     * FX/effect copied from spell 41 (item-cast-only, so id 41 itself stays untouched for enemy unit 124). */
    addPatch(&gSpells[26].range, 1, 5);  addPatch(&gSpells[26].fieldSize, 1, 1);
    addPatch(&gSpells[26].power, 1, 13); addPatch(&gSpells[26].mpCost,   1, 10);   /* area/tgt already AOE/enemy-grp */
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_MAIN],   2, 224);
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_TARGET], 2, 128);
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_DEFEAT], 2, 129);
    addPatch(&gSpellLevelRequirement[26], 1, 21);                              /* mage learns it @21 (Spread Force's slot) */
    addStrPatch(gSpellNames[26], "Thunder Ball");                             /* was "Stone Shower" */

    /* Spell-list swaps (gSpellLists[party][path][slot], s32). Monk path=1 slot 3; mage path=0 slot 7. */
    addPatch(&gSpellLists[4][1][3],  4, 13); addPatch(&gSpellLists[5][1][3],  4, 13);  /* Eleni/Huxley Monk -> Spread Force */
    addPatch(&gSpellLists[10][1][3], 4, 13); addPatch(&gSpellLists[11][1][3], 4, 13);  /* Sara/Zohar  Monk -> Spread Force */
    addPatch(&gSpellLists[4][0][7],  4, 26); addPatch(&gSpellLists[11][0][7], 4, 26);  /* Eleni/Zohar Mage -> Thunder Ball */

    /* Remaining Monk/Ninja spells (all path-B exclusive). */
    addPatch(&gSpells[27].fieldSize, 1, 3);  addPatch(&gSpells[27].mpCost,    1, 6);   /* CURE_WIDE      */
    addPatch(&gSpells[28].fieldSize, 1, 3);  addPatch(&gSpells[28].power,     1, 20);
    addPatch(&gSpells[28].mpCost,    1, 8);                                            /* HEALING_CIRCLE mp 7->8 (heals shouldn't out-cost the Bishop path) */
    addPatch(&gSpells[29].range,     1, 6);                                            /* PERFECT_GUARD  rng ->6 */
    addPatch(&gSpells[30].fieldSize, 1, 3);  addPatch(&gSpells[30].power,     1, 14);
    addPatch(&gSpells[30].mpCost,    1, 14);                                           /* THUNDER_FLASH fld3/pow14/mp14 */
    addPatch(&gSpells[31].fieldSize, 1, 3);  addPatch(&gSpells[31].mpCost,    1, 12);  /* HEALING_WAVE  mp 10->12 */
    /* MYSTIC_ENERGY: single->AOE ally-group, rng 0, field 3, mp 30 (defBoosted/magSusc is a PC_FEAT hook). */
    addPatch(&gSpells[32].area,      1, SPELL_AREA_AOE);
    addPatch(&gSpells[32].targeting, 1, SPELL_TARGET_ALLY_GROUP);
    addPatch(&gSpells[32].range,     1, 0);  addPatch(&gSpells[32].fieldSize, 1, 3);
    addPatch(&gSpells[32].mpCost,    1, 30);                                           /* MYSTIC_ENERGY  */

    /* 1.3.x playtest tuning (chapter-3/4 findings, exchange/65). */
    addPatch(&gSpells[11].power, 1, 9);  /* C3-3 ROMAN_FIRE power 7->9: on par w/ PHASE_SHIFT (learned ~2 lvls later, half MP -> niche = clustered foes + MP economy) */
    addPatch(&gBattleExpScalingLevels[29], 1, 18);  /* C4-0 map 29 (ch4 b1) expScaling 16->18: only downward dip in maps 10-43 (28=17, 30=18) -> restores kill-XP for the lvl19-21 opener */

    /* GAP 7 (cut-weapon restoration) FULLY DROPPED 2026-07-28. Data archaeology (gUnitInfo scan) showed
     * Bloodaxe(35) and Kill bow(23) are NOT unused content -- they are enemy-COMMANDER weapons (Bloodaxe
     * = Dallas + generic troopers; Kill bow = Lando only), and are temporary BiS at best (the "Bloodaxe"
     * is even a hammer/mace sprite, not an axe). Decision: keep them boss-exclusive, no player access.
     * Reverted the shop swaps AND the Bloodaxe display-power/description tweaks -> item 35/23 behave
     * byte-for-byte retail. (A boss-kill depot-reward hook was scoped but judged not worth the code.) */

    /* GAP 6 -- Tactical-only item descriptions (Normal keeps the retail "??????????"/blank). The nine
     * ?????-items (88-96) are hidden-tile finds; 8-11 are the generic "Attack magic item" entries;
     * 35 is the restored Bloodaxe. Original authored flavor text. */
    addDescSwap(88, "Casts Spellbind");
    addDescSwap(89, "Casts Poison Cloud");
    addDescSwap(8,  "Burn enemies in field");
    addDescSwap(90, "Casts Self Healing");
    addDescSwap(9,  "Crushes foes in rings");
    addDescSwap(91, "Casts Perfect Guard");
    addDescSwap(10, "Mega light attack");
    addDescSwap(92, "Casts Rainbow Storm");
    addDescSwap(93, "Casts Healing Circle");
    addDescSwap(94, "Casts Thunder Ball");
    addDescSwap(11, "Huge rings of fire");
    addDescSwap(95, "Casts Dagger Storm");
    addDescSwap(96, "Casts Dark Hurricane");

    /* bugreport-03 -- sync the spell-info bar text with the GAP-5/C3-2 balance (display-only; the real
     * gSpells fields are already patched above). Format/spacing copied from the retail strings. */
    addSpellDescSwap(13, "Attack magic  Rng:0  Fld:3  MP:8");    /* SPREAD_FORCE   (Monk) fld3 pow8 mp8 */
    addSpellDescSwap(26, "Attack Magic  Rng:5  Fld:1  MP:10");   /* THUNDER_BALL   (mage) ranged nuke  */
    addSpellDescSwap(27, "Cure Status  Rng:0  Fld:3  MP:6");     /* CURE_WIDE                          */
    addSpellDescSwap(28, "Healing Magic  Rng:0  Fld:3  MP:8");   /* HEALING_CIRCLE MP 7->8             */
    addSpellDescSwap(29, "Protect Magic  Rng:6  F:0  MP:15");    /* PERFECT_GUARD  Rng 7->6            */
    addSpellDescSwap(30, "Attack Magic  Rng:0  Fld:3  MP:14");   /* THUNDER_FLASH  Fld3 MP14           */
    addSpellDescSwap(31, "Healing Magic  Rng:0  F:3  MP:12");    /* HEALING_WAVE   MP 10->12           */
    addSpellDescSwap(32, "DEF,AT Up  Rng:0  Fld:3  MP:30");      /* MYSTIC_ENERGY                      */
}

void PC_BalanceBoot(void) {
    const char *e = getenv("VH_TACTICAL");
    gTacticalMode = (e && e[0] == '1') ? 1 : 0;   /* boot default from vandalhearts.ini / env */
    PC_SyncBalance();
    fprintf(stderr, "PC_Balance: Tactical Mode %s at startup\n", gTacticalMode ? "ON" : "OFF");
}

void PC_SyncBalance(void) {
    int i;
    ensureInit();
    if (gTacticalMode && !s_applied) {
        for (i = 0; i < s_nPatch; i++) {
            const void *src = s_patch[i].tacptr ? s_patch[i].tacptr : (const void *)&s_patch[i].tac;
            memcpy(s_patch[i].addr, src, s_patch[i].size);
        }
        s_applied = 1;
        if (getenv("VH_BALANCE_DUMP")) {   /* diagnostic: runtime gSpells after apply */
            fprintf(stderr, "[baldump] nPatch=%d  26(f=%d p=%d mp=%d) 27(f=%d mp=%d) 28(f=%d p=%d mp=%d) 30(f=%d p=%d mp=%d)\n",
                s_nPatch,
                gSpells[26].fieldSize, gSpells[26].power, gSpells[26].mpCost,
                gSpells[27].fieldSize, gSpells[27].mpCost,
                gSpells[28].fieldSize, gSpells[28].power, gSpells[28].mpCost,
                gSpells[30].fieldSize, gSpells[30].power, gSpells[30].mpCost);
            for (i = 0; i < s_nPatch; i++)
                if (s_patch[i].addr == (void *)&gSpells[26].mpCost || s_patch[i].addr == (void *)&gSpells[26].fieldSize)
                    fprintf(stderr, "[baldump] patch#%d addr=%p size=%u tac=%lu orig=%u cur=%u\n",
                        i, s_patch[i].addr, s_patch[i].size, s_patch[i].tac, s_patch[i].orig[0], *(unsigned char *)s_patch[i].addr);
        }
    } else if (!gTacticalMode && s_applied) {
        for (i = 0; i < s_nPatch; i++)
            memcpy(s_patch[i].addr, s_patch[i].orig, s_patch[i].size);
        s_applied = 0;
    }
    stampSaveMarker();   /* keep the save-header mode marker in sync with gTacticalMode */
}
