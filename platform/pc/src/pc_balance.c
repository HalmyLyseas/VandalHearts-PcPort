/* Tactical Mode balance package: a restorable, idempotent patch over the game's mutable global
 * tables, applied while gTacticalMode is set and reverted otherwise. The src/ hooks that read it sit
 * behind PC_FEAT; never in the matching build. See docs/tactical-mode.md, "Implementation notes". */
#include <limits.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "common.h"
#include "units.h"
#include "state.h"
#include "card.h"          /* gCardFileHeader, CardFileData_Header (padding[28]) */
#include "audio.h"         /* PerformAudioCommand, AUDIO_CMD_STOP_ALL */
#include "battle.h"        /* gIsEnemyTurn */
#include "pc_platform.h"   /* PC_SaveDir */
#include "pc_balance.h"
#include "pc_lang.h"   /* PC_LangStr: the Tactical layer is translatable */

int gTacticalMode = 0;

int PC_AtTitleMenu(void) {
    return gState.primary == STATE_TITLE_SCREEN;
}

/* VH_SPELL_DUMP=1 diagnostic: log a unit's advancement fields + final spell list from
 * PopulateUnitSpellList. Read-only; shows e.g. why a Ninja's list ends empty (advFirst==0). */
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

/* RETURN TO TITLE is the overlay's action (pc_overlay.c); it ends by calling PC_SyncBalance. */

/* Save-mode marker: card-header padding[0] (file offset 68; the header sits at file offset 0) holds
 * 'T' in Tactical, 0 in Normal. The game never touches padding (core/card.c), so the stamp rides
 * inside the save and stays hardware-valid. */
#define TAC_MARK 0x54   /* 'T' */

extern CardFileData_Header gCardFileHeader;   /* declared in core/card.c; defined in generated_data.c */

static void stampSaveMarker(void) {
    gCardFileHeader.padding[0] = (unsigned char)(gTacticalMode ? TAC_MARK : 0);
}

void PC_AdoptSaveMode(void) {
    char path[PATH_MAX];
    /* The JP card header shares the US layout up to and including padding[28] at offset 68 -- JP
     * only appends a third icon frame at the tail (512 B vs 384 B) -- so the marker offset and the
     * untouched-padding guarantee hold in both regions. */
    FILE *f;
    unsigned char b = 0;
    int diskTactical;
    snprintf(path, sizeof(path), "%s/%s", PC_SaveDir(), VH_ACTIVE_CARD_NAME);
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
    /* [0] is a dead clamp slot; caps for chapters 1..6 climb +5 from the L5/L6 starters, then a
     * steady +4 per chapter, ending on 32 = the final boss's level.
     * See docs/tactical-mode.md, "Implementation notes". */
    static const int cap[7] = { 10, 10, 15, 19, 24, 28, 32 };
    if (!gTacticalMode) return 50;              /* Normal: the retail cap. */
    if (chapter < 1) chapter = 1;
    if (chapter > 6) chapter = 6;
    return cap[chapter];
}

/* Trial rewards (Tactical): a cleared Trial pays like that chapter's final battle -- XP scaling
 * tier + enemy XP multiplier, gold per kill, gold lost per player-unit death -- indexed by
 * gState.chapter clamped to [1,6]. See docs/tactical-mode.md, "Implementation notes". */
#define CLAMP_CH(c) ((c) < 1 ? 1 : ((c) > 6 ? 6 : (c)))
int TrialExpScalingLevel(int chapter) { static const int v[7] = { 9,  9, 14, 17, 21, 26, 29 }; return v[CLAMP_CH(chapter)]; }
int TrialEnemyExpMulti(int chapter)   { static const int v[7] = { 12, 12, 15, 16, 15, 12, 21 }; return v[CLAMP_CH(chapter)]; }
int TrialGoldReward(int chapter)      { static const int v[7] = { 170,170,330,660,1040,1820,2700 }; return v[CLAMP_CH(chapter)]; }
int TrialGoldPenalty(int chapter)     { static const int v[7] = { 110,110,220,440,700,1200,1800 }; return v[CLAMP_CH(chapter)]; }

/* --- Restorable, idempotent scalar-table patch -------------------------------------------------- */

typedef struct {
    void          *addr;      /* target location in a mutable global table */
    unsigned char  size;      /* 1/2/4/8 bytes for scalars; up to sizeof(orig) for string patches */
    unsigned long long tac;   /* scalar Tactical value (size<=8; host is little-endian). 64-bit on
                               * purpose: the desc swaps pass POINTERS through this field, and
                               * `unsigned long` is 32-bit on Windows (LLP64). */
    const void    *tacptr;    /* byte/string source when size>8 (addStrPatch); NULL for scalars */
    unsigned char  orig[24];  /* pristine snapshot */
} Patch;

#define MAX_PATCH 160
static Patch s_patch[MAX_PATCH];
static int   s_nPatch  = 0;
static int   s_inited  = 0;
static int   s_applied = 0;

static void addPatch(void *addr, unsigned char size, unsigned long long tac) {
    Patch *p;
    if (s_nPatch >= MAX_PATCH) return;
    p = &s_patch[s_nPatch++];
    p->addr = addr;
    p->size = size;
    p->tac  = tac;
    p->tacptr = 0;
    memcpy(p->orig, addr, size);   /* snapshot the pristine bytes now (tables untouched pre-patch) */
}

/* In-place fixed-width string patch (gSpellNames[] is a char[][21] array, not a pointer table, so
 * it can't be repointed). `dstWidth` is the record width at addr: a longer replacement would run
 * into the NEXT record, so it is logged and skipped -- it means a mis-built language pack. */
static void addStrPatch(void *addr, const char *s, unsigned int dstWidth) {
    Patch *p;
    unsigned int n;
    s = (const char *)PC_LangStr(s);           /* translatable (pack encodes fixed-width targets
                                                  in 1-byte pack codes at build time) */
    n = (unsigned int)strlen(s) + 1;
    if (s_nPatch >= MAX_PATCH) return;
    if (n > dstWidth || n > sizeof p->orig) {  /* dstWidth bounds the record; orig[] bounds the snapshot */
        fprintf(stderr, "[balance] string patch %u B exceeds the %u B record -- skipped "
                        "(rebuild the language pack)\n", n, dstWidth);
        return;
    }
    p = &s_patch[s_nPatch++];
    p->addr = addr;
    p->size = (unsigned char)n;
    p->tac  = 0;
    p->tacptr = s;
    memcpy(p->orig, addr, n);
}

/* gItemDescriptions is declared in units.h (s8*); gItemDescriptions2 only in ui/supplies.c. Both defined in
 * the generated pc_item_descriptions.c with the RETAIL strings; we repoint them in Tactical only. */
extern u8 *gItemDescriptions2[101];


/* Point an item's description (both the single-line and shop tables) at `flavor` in Tactical.
 * `flavor` is original authored text (static storage), NOT extracted ROM text. Normal keeps retail. */
static void addDescSwap(int id, const char *flavor) {
    /* The Tactical layer is translatable text too: offer the flavor string to the active language
     * pack by content (PC_LangStr; the literal itself when no pack / no entry). Resolved ONCE here,
     * at patch-build time, so apply/restore stay simple value writes. */
    flavor = (const char *)PC_LangStr(flavor);
    addPatch(&gItemDescriptions[id],  sizeof(char *), (unsigned long long)(uintptr_t)flavor);
    addPatch(&gItemDescriptions2[id], sizeof(char *), (unsigned long long)(uintptr_t)flavor);
}

/* The spell-info bar (ui/window.c) draws gSpellDescriptions[] -- a BAKED string with Rng/Fld/MP
 * hardcoded, NOT read from gSpells. Balance patches change the real gSpells fields, so the reworked
 * spells' info line is repointed in Tactical to keep the display in step. */
static void addSpellDescSwap(int id, const char *s) {
    s = (const char *)PC_LangStr(s);           /* translatable, same as addDescSwap */
    addPatch(&gSpellDescriptions[id], sizeof(char *), (unsigned long long)(uintptr_t)s);
}

/* Build the patch list + snapshot originals, once. Lazily invoked by PC_SyncBalance so it runs
 * after generated_data.c has populated the tables. */
static void ensureInit(void) {
    /* Guardsman/Dragoon mobility (Clint/Grog/Dolan, both forms). step indexes a whole movement
     * profile (MOVE + climb + terrain cost; docs/game-mechanics/classes.md): Guardsman -> Bowman
     * profile 5 (MOVE stays 5, climb +1 -> +2); Dragoon -> Sniper profile 6 (MOVE 5 -> 6, climb +2). */
    static const int GUARDSMAN[] = { 26, 31, 32 };            /* step 1 -> 5 (Bowman) */
    static const int DRAGOON[]   = { 50, 55, 56 };            /* step 4 -> 6 (Sniper) */
    /* Monk/Ninja magic resistance. Player caster-path-B units (Eleni/Huxley/Sara/Zohar). */
    static const int MONKNINJA[] = { 28, 29, 34, 35, 52, 53, 58, 59 };  /* magSusc 3 -> 2 */
    int i;

    if (s_inited) return;
    s_inited = 1;

    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[GUARDSMAN[i]].step, 1, 5);   /* Bowman profile */
    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[DRAGOON[i]].step, 1, 6);     /* Sniper profile */
    for (i = 0; i < 8; i++) addPatch(&gUnitInfo[MONKNINJA[i]].magicSusceptibility, 1, 2);

    addPatch(&gClassMpMultiplier[CLASS_MONK], 1, 2);                      /* Monk MP mult 1 -> 2 */
    addPatch(&gItemEquipmentPower[ITEM_P_CLAWS], 1, 12);                  /* P.claws 10 -> 12 */
    addPatch(&gItemEquipmentPower[ITEM_D_CLAWS], 1, 13);                  /* D.claws 12 -> 13 */

    /* Monk/Ninja + mage spell rework: two swaps make spells 13/26 class-EXCLUSIVE so their stats and
     * reqLv tune freely -- Monk (path B) takes the real SPREAD_FORCE(13), the mage (path A) takes
     * THUNDER_BALL in slot 26. Stone Shower is dropped in Tactical only; nothing else casts it. */

    /* Spread Force(13): Monk-exclusive, toned down, unlocks at 12 (Stone Shower's retail level). */
    addPatch(&gSpells[13].power, 1, 8);  addPatch(&gSpells[13].mpCost, 1, 8);   /* 13/7 -> 8/8 (rng0 fld3 kept) */
    addPatch(&gSpellLevelRequirement[13], 1, 12);                              /* retail 21 (mage tier) -> Monk @12 */

    /* Slot 26 becomes THUNDER_BALL for the mage: long-range small AOE, keeps retail Spread Force pow 13.
     * FX/effect copied from spell 41 (item-cast-only, so id 41 itself stays untouched for enemy unit 124). */
    addPatch(&gSpells[26].range, 1, 5);  addPatch(&gSpells[26].fieldSize, 1, 1);
    addPatch(&gSpells[26].power, 1, 13); addPatch(&gSpells[26].mpCost,   1, 10);   /* area/tgt already AOE/enemy-grp */
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_MAIN],   2, 224);
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_TARGET], 2, 128);
    addPatch(&gSpellsEx[26][SPELL_EX_OBJF_DEFEAT], 2, 129);
    addPatch(&gSpellLevelRequirement[26], 1, 21);                              /* mage learns it @21 (Spread Force's slot) */
#ifdef VH_REGION_JP
    /* The JP renderer draws full-width Shift-JIS spell names; an ASCII rename renders BLANK in the
     * spell list and cast banner. So the name is katakana, consistent with the surrounding retail
     * names (the same region-faithful treatment the description swaps get). */
    addStrPatch(gSpellNames[26],
        "\x83\x54\x83\x93\x83\x5f\x81\x5b\x83\x7b\x81\x5b\x83\x8b",  /* サンダーボール */
        sizeof gSpellNames[0]);                                        /* retail ストーンシャワー */
#else
    addStrPatch(gSpellNames[26], "Thunder Ball", sizeof gSpellNames[0]);      /* retail "Stone Shower" */
#endif

    /* Spell-list swaps (gSpellLists[party][path][slot], s32). Monk path=1 slot 3; mage path=0 slot 7. */
    addPatch(&gSpellLists[4][1][3],  4, 13); addPatch(&gSpellLists[5][1][3],  4, 13);  /* Eleni/Huxley Monk -> Spread Force */
    addPatch(&gSpellLists[10][1][3], 4, 13); addPatch(&gSpellLists[11][1][3], 4, 13);  /* Sara/Zohar  Monk -> Spread Force */
    addPatch(&gSpellLists[4][0][7],  4, 26); addPatch(&gSpellLists[11][0][7], 4, 26);  /* Eleni/Zohar Mage -> Thunder Ball */

    /* Remaining Monk/Ninja spells (all path-B exclusive). */
    addPatch(&gSpells[27].fieldSize, 1, 3);  addPatch(&gSpells[27].mpCost,    1, 6);   /* CURE_WIDE      */
    addPatch(&gSpells[28].fieldSize, 1, 3);                                            /* HEALING_CIRCLE field 1->3 (kept); power stays retail 15 -- pow 20 out-classed Healing Wave (28@10mp) */
    addPatch(&gSpells[28].mpCost,    1, 6);                                            /* HEALING_CIRCLE mp 7->6: cheap efficient top-up heal, clearly below Healing Wave (28pow/12mp) */
    addPatch(&gSpells[29].range,     1, 6);                                            /* PERFECT_GUARD  rng ->6 */
    addPatch(&gSpells[29].mpCost,    1, 12);                                           /* PERFECT_GUARD  mp 15->12: single-target/1-turn, and half a Mystic Energy (30) is too steep for its niche */
    addPatch(&gSpells[30].fieldSize, 1, 3);  addPatch(&gSpells[30].power,     1, 14);
    addPatch(&gSpells[30].mpCost,    1, 14);                                           /* THUNDER_FLASH fld3/pow14/mp14 */
    addPatch(&gSpells[31].fieldSize, 1, 3);  addPatch(&gSpells[31].mpCost,    1, 12);  /* HEALING_WAVE  mp 10->12 */
    /* MYSTIC_ENERGY: single->AOE ally-group, rng 0, field 3, mp 30 (defBoosted/magSusc is a PC_FEAT hook). */
    addPatch(&gSpells[32].area,      1, SPELL_AREA_AOE);
    addPatch(&gSpells[32].targeting, 1, SPELL_TARGET_ALLY_GROUP);
    addPatch(&gSpells[32].range,     1, 0);  addPatch(&gSpells[32].fieldSize, 1, 3);
    addPatch(&gSpells[32].mpCost,    1, 35);                                           /* MYSTIC_ENERGY mp 35: >half of L32 Huxley's 64 pool -> a 2nd back-to-back cast needs an MP refill item; still castable on acquisition at L25 */
    /* Retail Mystic Energy is single-target, so its per-target hit sound gSpellSounds2[32] is 0. As
     * an AOE ally-group buff the per-target loop (battle/executors.c) plays gSpellSounds2 once per
     * ally, so it mirrors the cast sound 911 -- the Sounds1==Sounds2 pattern Cure Wide uses. */
    addPatch(&gSpellSounds2[32], 2, 911);
    /* Per-ally aura recolored blue: OBJF 112 is Mystic Shield's blue-CLUT twin of retail's green 113
     * (same OBJF_CASTING_STAT_BUFF object, so still AOE-safe), and the stat-up text keys off the aura
     * CLUT -- BLUES = "DF up!" only. See docs/tactical-mode.md, "Implementation notes". */
    addPatch(&gSpellsEx[32][SPELL_EX_OBJF_TARGET], 2, 112);

    /* Mage damage + chapter-4 XP-curve tuning. */
    addPatch(&gSpells[11].power, 1, 9);  /* ROMAN_FIRE power 7->9: on par w/ PHASE_SHIFT (learned ~2 lvls later, half MP -> niche = clustered foes + MP economy) */
    addPatch(&gBattleExpScalingLevels[29], 1, 18);  /* map 29 (ch4 b1) expScaling 16->18: only downward dip in maps 10-43 (28=17, 30=18) -> restores kill-XP for the lvl19-21 opener */

    /* The unequippable Bloodaxe (item 35) and Kill bow (item 23) are enemy-commander weapons (Dallas
     * and troopers; Lando) and stay boss-exclusive: both items are byte-for-byte retail in Tactical. */

    /* Tactical-only item descriptions (Normal keeps the retail "??????????"/blank). Items 88-96 are
     * the hidden-tile ?????-finds; 8-11 are the generic "Attack magic item" entries. Authored text. */
#ifdef VH_REGION_JP
    /* JP stays faithful to its disc: the JP binary ships accurate descriptions for the spells these
     * items cast (gSpellDescriptions[gItemSpells[id]]; spells 63-71 take no Tactical patches), so
     * the nine ?????-items reuse them. Items 8-11 need no swap: the JP retail text states the specs. */
    {
        static const unsigned char mystery[] = { 88, 89, 90, 91, 92, 93, 94, 95, 96 };
        int mi;
        for (mi = 0; mi < 9; mi++)
            addDescSwap(mystery[mi], (const char *)gSpellDescriptions[gItemSpells[mystery[mi]]]);
    }
#else
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
#endif

    /* Spell-info bar text, kept in step with the gSpells patches above (display-only). Format and
     * spacing copied from the retail strings. */
#ifdef VH_REGION_JP
    /* JP: each line is the RETAIL JP string with only the numerals swapped to the Tactical values
     * (format <retail prefix> 射程R 範囲F 消費ＭＰn; full-width digit d is 0x82,0x4f+d). Spell 32
     * borrows spell 29's protective prefix since Mystic Energy is a defense-only buff in Tactical. */
    addSpellDescSwap(13,
        "\x8d\x55\x8c\x82\x96\x82\x96\x40\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x57");  /* SPREAD_FORCE   mp 7->8 */
    addSpellDescSwap(26,
        "\x8d\x55\x8c\x82\x96\x82\x96\x40\x81\x40\x8e\xcb\x92\xf6\x82\x54\x81\x40\x94\xcd\x88\xcd\x82\x50\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x50\x82\x4f");  /* THUNDER_BALL   rng 0->5 */
    addSpellDescSwap(27,
        "\x8f\xf3\x91\xd4\x89\xf1\x95\x9c\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x55");  /* CURE_WIDE      fld 1->3, mp 4->6 */
    addSpellDescSwap(28,
        "\x89\xf1\x95\x9c\x96\x82\x96\x40\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x55");  /* HEALING_CIRCLE fld 1->3 */
    addSpellDescSwap(29,
        "\x95\xa8\x97\x9d\x8d\x55\x8c\x82\x82\xf0\x96\x68\x8c\xe4\x81\x40\x8e\xcb\x92\xf6\x82\x55\x81\x40\x94\xcd\x88\xcd\x82\x4f\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x50\x82\x51");  /* PERFECT_GUARD  rng 4->6, mp 15->12 */
    addSpellDescSwap(30,
        "\x8d\x55\x8c\x82\x96\x82\x96\x40\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x50\x82\x53");  /* THUNDER_FLASH  fld 2->3, mp 12->14 */
    addSpellDescSwap(31,
        "\x89\xf1\x95\x9c\x96\x82\x96\x40\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x50\x82\x51");  /* HEALING_WAVE   fld 2->3, mp 10->12 */
    addSpellDescSwap(32,
        "\x95\xa8\x97\x9d\x8d\x55\x8c\x82\x82\xf0\x96\x68\x8c\xe4\x81\x40\x8e\xcb\x92\xf6\x82\x4f\x81\x40\x94\xcd\x88\xcd\x82\x52\x81\x40\x8f\xc1\x94\xef\x82\x6c\x82\x6f\x82\x52\x82\x54");  /* MYSTIC_ENERGY  rng 4->0, fld 0->3, mp 15->35; prefix = PERFECT_GUARD (29) retail wording: the Tactical spell is protective */
#else
    addSpellDescSwap(13, "Attack magic  Rng:0  Fld:3  MP:8");    /* SPREAD_FORCE   (Monk) fld3 pow8 mp8 */
    addSpellDescSwap(26, "Attack Magic  Rng:5  Fld:1  MP:10");   /* THUNDER_BALL   (mage) ranged nuke  */
    addSpellDescSwap(27, "Cure Status  Rng:0  Fld:3  MP:6");     /* CURE_WIDE                          */
    addSpellDescSwap(28, "Healing Magic  Rng:0  Fld:3  MP:6");   /* HEALING_CIRCLE MP 7->6             */
    addSpellDescSwap(29, "Protect Magic  Rng:6  F:0  MP:12");    /* PERFECT_GUARD  Rng 7->6, MP 15->12 */
    addSpellDescSwap(30, "Attack Magic  Rng:0  Fld:3  MP:14");   /* THUNDER_FLASH  Fld3 MP14           */
    addSpellDescSwap(31, "Healing Magic  Rng:0  F:3  MP:12");    /* HEALING_WAVE   MP 10->12           */
    addSpellDescSwap(32, "Protect Magic  Rng:0  Fld:3  MP:35");  /* MYSTIC_ENERGY  protective in Tactical (DEF + anti-magic, no ATK) -- same wording as PERFECT_GUARD (29) */
#endif
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
            fprintf(stderr, "[baldump] 32(area=%d tgt=%d rng=%d fld=%d mp=%d | objMain=%d objTgt=%d objDef=%d eff=%d)\n",
                gSpells[32].area, gSpells[32].targeting, gSpells[32].range,
                gSpells[32].fieldSize, gSpells[32].mpCost,
                gSpellsEx[32][SPELL_EX_OBJF_MAIN], gSpellsEx[32][SPELL_EX_OBJF_TARGET],
                gSpellsEx[32][SPELL_EX_OBJF_DEFEAT], gSpellsEx[32][SPELL_EX_EFFECT]);
            for (i = 0; i < s_nPatch; i++)
                if (s_patch[i].addr == (void *)&gSpells[26].mpCost || s_patch[i].addr == (void *)&gSpells[26].fieldSize)
                    fprintf(stderr, "[baldump] patch#%d addr=%p size=%u tac=%llu orig=%u cur=%u\n",
                        i, s_patch[i].addr, s_patch[i].size, s_patch[i].tac, s_patch[i].orig[0], *(unsigned char *)s_patch[i].addr);
        }
    } else if (!gTacticalMode && s_applied) {
        for (i = 0; i < s_nPatch; i++)
            memcpy(s_patch[i].addr, s_patch[i].orig, s_patch[i].size);
        s_applied = 0;
    }
    stampSaveMarker();   /* keep the save-header mode marker in sync with gTacticalMode */
}
