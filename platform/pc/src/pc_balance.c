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

/* --- Restorable, idempotent scalar-table patch -------------------------------------------------- */

typedef struct {
    void          *addr;      /* target location in a mutable global table */
    unsigned char  size;      /* 1/2/4/8 bytes */
    unsigned long  tac;       /* the Tactical value (low `size` bytes used; host is little-endian) */
    unsigned char  orig[8];   /* pristine snapshot */
} Patch;

#define MAX_PATCH 128
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
    memcpy(p->orig, addr, size);   /* snapshot the pristine bytes now (tables untouched pre-patch) */
}

extern u8 gShopInventories[10][10][20];   /* [shopId][category][slot]; category 0 = weapons */
/* gItemDescriptions is declared in units.h (s8*); gItemDescriptions2 only in supplies.c. Both defined in
 * the generated pc_item_descriptions.c with the RETAIL strings; we repoint them in Tactical only. */
extern u8 *gItemDescriptions2[101];

/* GAP 7: replace the first slot holding `fromItem` in a shop's weapon list with `toItem`. */
static void addShopSwap(int shopId, int fromItem, int toItem) {
    int slot;
    for (slot = 0; slot < 20; slot++) {
        if (gShopInventories[shopId][0][slot] == (u8)fromItem) {
            addPatch(&gShopInventories[shopId][0][slot], 1, (unsigned long)toItem);
            return;
        }
    }
}

/* GAP 6: point an item's description (both the single-line and shop tables) at `flavor` in Tactical.
 * `flavor` is original authored text (static storage), NOT extracted ROM text. Normal keeps retail. */
static void addDescSwap(int id, const char *flavor) {
    addPatch(&gItemDescriptions[id],  sizeof(char *), (unsigned long)(uintptr_t)flavor);
    addPatch(&gItemDescriptions2[id], sizeof(char *), (unsigned long)(uintptr_t)flavor);
}

/* Build the patch list + snapshot originals, once. Lazily invoked by PC_SyncBalance so it runs
 * after generated_data.c has populated the tables. */
static void ensureInit(void) {
    /* C4 -- Guardsman/Dragoon mobility (step). Player Armored units: Clint/Grog/Dolan, both forms. */
    static const int GUARDSMAN[] = { 26, 31, 32 };            /* step 1 -> 2 */
    static const int DRAGOON[]   = { 50, 55, 56 };            /* step 4 -> 6 */
    /* A1 -- Monk/Ninja magic resistance. Player caster-path-B units (Eleni/Huxley/Sara/Zohar). */
    static const int MONKNINJA[] = { 28, 29, 34, 35, 52, 53, 58, 59 };  /* magSusc 3 -> 2 */
    int i;

    if (s_inited) return;
    s_inited = 1;

    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[GUARDSMAN[i]].step, 1, 2);
    for (i = 0; i < 3; i++) addPatch(&gUnitInfo[DRAGOON[i]].step, 1, 6);
    for (i = 0; i < 8; i++) addPatch(&gUnitInfo[MONKNINJA[i]].magicSusceptibility, 1, 2);

    addPatch(&gClassMpMultiplier[CLASS_MONK], 1, 2);                      /* A3  MP mult 1 -> 2 */
    addPatch(&gItemEquipmentPower[ITEM_P_CLAWS], 1, 12);                  /* B3  P.claws 10 -> 12 */
    addPatch(&gItemEquipmentPower[ITEM_D_CLAWS], 1, 13);                  /* B3  D.claws 12 -> 13 */
    addPatch((void *)&gItemEquipmentDisplayPower[ITEM_BLOODAXE], 1, 24);  /* GAP7 display 27 -> 24 */

    /* B -- Monk/Ninja spell-list rework (gSpells[26-32]; only the changed fields, per the GAP 5 table). */
    addPatch(&gSpells[26].power,     1, 8);  addPatch(&gSpells[26].mpCost,    1, 7);   /* STONE_SHOWER  */
    addPatch(&gSpells[27].fieldSize, 1, 3);  addPatch(&gSpells[27].mpCost,    1, 6);   /* CURE_WIDE     */
    addPatch(&gSpells[28].fieldSize, 1, 3);  addPatch(&gSpells[28].power,     1, 20);
    addPatch(&gSpells[28].mpCost,    1, 7);                                            /* HEALING_CIRCLE */
    addPatch(&gSpells[29].range,     1, 7);                                            /* PERFECT_GUARD  */
    addPatch(&gSpells[30].fieldSize, 1, 1);  addPatch(&gSpells[30].power,     1, 17);
    addPatch(&gSpells[30].mpCost,    1, 9);                                            /* THUNDER_FLASH  */
    addPatch(&gSpells[31].fieldSize, 1, 3);                                            /* HEALING_WAVE   */
    /* MYSTIC_ENERGY: single->AOE ally-group, rng 0, field 3, mp 30. The defBoosted/magSusc effect is a
     * separate PC_FEAT logic hook (added with the other logic hooks), NOT a gSpells field. */
    addPatch(&gSpells[32].area,      1, SPELL_AREA_AOE);
    addPatch(&gSpells[32].targeting, 1, SPELL_TARGET_ALLY_GROUP);
    addPatch(&gSpells[32].range,     1, 0);  addPatch(&gSpells[32].fieldSize, 1, 3);
    addPatch(&gSpells[32].mpCost,    1, 30);                                           /* MYSTIC_ENERGY  */

    /* GAP 7 -- cut-weapon restoration: swap each shop's lower weapon for the cut item (keeps the
     * "2 best normal tier" rule). Sorbo/Frontier (Ch V): Iron->cut; Glasgow (Ch VI): Steel/Battle->cut. */
    addShopSwap(7, ITEM_IRON_BOW, ITEM_KILL_BOW); addShopSwap(7, ITEM_IRON_AXE, ITEM_BLOODAXE);
    addShopSwap(8, ITEM_IRON_BOW, ITEM_KILL_BOW); addShopSwap(8, ITEM_IRON_AXE, ITEM_BLOODAXE);
    addShopSwap(9, ITEM_STL_BOW,  ITEM_KILL_BOW); addShopSwap(9, ITEM_BATTLEAX, ITEM_BLOODAXE);

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
    addDescSwap(35, "Bloodaxe  AT+24");     /* the restored cut weapon (display value 24; see GAP 7) */
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
        for (i = 0; i < s_nPatch; i++)
            memcpy(s_patch[i].addr, &s_patch[i].tac, s_patch[i].size);
        s_applied = 1;
    } else if (!gTacticalMode && s_applied) {
        for (i = 0; i < s_nPatch; i++)
            memcpy(s_patch[i].addr, s_patch[i].orig, s_patch[i].size);
        s_applied = 0;
    }
    stampSaveMarker();   /* keep the save-header mode marker in sync with gTacticalMode */
}
