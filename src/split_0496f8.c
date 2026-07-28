#include "common.h"
#include "state.h"
#include "units.h"
#include "graphics.h"
#include "object.h"
#include "field.h"
#include "battle.h"
#include "cd_files.h"
#include "audio.h"

#include "PsyQ/libsn.h"

#ifdef PC_FEAT
/* Stage-3 1.3: pc_balance.c -- gTacticalMode + the per-chapter cap (GAP 2 Trials, GAP 3 PLASMA_WAVE). */
extern int gTacticalMode;
extern int TacticalCap(int chapter);
extern int TrialEnemyExpMulti(int chapter);   /* GAP 9: per-chapter trial attack-XP multiplier */
#endif

void ClearPortraitSet(void);
void ResetStateForNewGame(void);
void SetupParty(u8);
void SetupPartySprites(void);
void SetDefaultStatsForParty(void);
void PopulateUnitSpellList(UnitStatus *);
void SetupPartyBattleUnit(u8, u8, u8, u8);
void SyncPartyUnit(u8);
void Objf424_BattleEnder(Object *);
void func_8005AA7C(void);
void SetupBattleUnit(s16, u8, u8, s8, u8, u8, u8, u8, u8);
void CreateBattleUnitForUnit(UnitStatus *);
void SetupBattleUnits(void);
s32 PCDrv_WriteFile(s8 *, void *, s32);
void ResetGeomOffset(void);
void Initialize(void);
void SetupLight(void);
void SetupQuads(Graphics *);
void SetupSprites(void);
void SetupMap(void);
void LoadMapTextures(void);
void LoadFWD(void);
void LoadPortraits(void);
void LoadProvidedPortraits(s32);
void LoadFCOM4XX(void);
void LoadItemIcons(void);
void SetupGfx(void);
void LoadMap(void);
void SetupField(void);
void UpdateElevation(void);
// void UpdateTileElevation(u8, u8);

void ClearPortraitSet(void) {
   s32 i;

   for (i = 0; i < ARRAY_COUNT(gState.currentPortraits); i++) {
      gState.currentPortraits[i] = 0;
      gState.portraitsToLoad[i] = 0;
   }
}

void ResetStateForNewGame(void) {
   s32 i, j;

   for (i = 0; i < PARTY_CT; i++) {
      gPartyMembers[i].inParty = 0;
      gPartyMembers[i].advChosePathB = 0;
      gPartyMembers[i].advLevelFirst = 0;
      gPartyMembers[i].advLevelSecond = 0;

      for (j = 0; j < 8; j++) {
         gPartyMembers[i].experience[j] = 0;
      }
   }

   gState.gold = 0;
   gState.frameCounter = 0;
}

void SetupParty(u8 idx) {
   s32 i, j;
   s32 level;

   for (i = 1; i < PARTY_CT; i++) {
      level = gInitialLevels[idx][i - 1];

      if (level != 0) {
         gPartyMembers[i].inParty = 1;

         if (level != 1) {
            gPartyMembers[i].advChosePathB = 0;
            gPartyMembers[i].advLevelFirst = 0;
            gPartyMembers[i].advLevelSecond = 0;
            gPartyMembers[i].items[0] = gUnitInfo[i].heldItem;
            gPartyMembers[i].items[1] = ITEM_NULL;
            gPartyMembers[i].field9_0x9 = 0;
            gPartyMembers[i].weapon = gUnitInfo[i].weapon;
            gPartyMembers[i].helmet = gUnitInfo[i].helmet;
            gPartyMembers[i].armor = gUnitInfo[i].armor;

            for (j = 0; j < 8; j++) {
               gPartyMembers[i].experience[j] = gExperienceLevels[level - 1][j];
            }

            gPartyMembers[i].hpVar100 = 100;
            gPartyMembers[i].atkVar100 = 100;
            gPartyMembers[i].defVar100 = 100;
            gPartyMembers[i].agiVar100 = 100;
         }
      } else {
         gPartyMembers[i].inParty = 0;
      }
   }
}

void SetupPartySprites(void) {
   s32 i;
   s32 partyIdx;
   s32 id;
   s32 adv;
   s8 unused[8];

   for (i = 0; i < 20; i++) {
      id = gSpriteStripUnitIds[i];
      if (id >= 151 && id <= 274) {
         // IDs 151..300 are reserved for main party event sprites (sitting, etc.), but they start
         // off as a "virtual" ID, since they need to be adjusted to account for advancements.
         // Larger ranges (e.g. 157..[181..183]) are (presumably) to leave room for the preceding
         // ranges' adjustment. (Also, I wonder why Clint is out of order here.)
         if (id >= 273) { ///////// 273..274
            partyIdx = UNIT_DARIUS;
         } else if (id >= 247) { // 247..272
            partyIdx = UNIT_ZOHAR;
         } else if (id >= 245) { // 245..246
            partyIdx = UNIT_SARA;
         } else if (id >= 243) { // 243..244
            partyIdx = UNIT_AMON;
         } else if (id >= 217) { // 217..242
            partyIdx = UNIT_DOLAN;
         } else if (id >= 215) { // 215..216
            partyIdx = UNIT_GROG;
         } else if (id >= 213) { // 213..214
            partyIdx = UNIT_KIRA;
         } else if (id >= 187) { // 187..212
            partyIdx = UNIT_CLINT;
         } else if (id >= 184) { // 184..186
            partyIdx = UNIT_HUXLEY;
         } else if (id >= 157) { // 157..183
            partyIdx = UNIT_ELENI;
         } else if (id >= 154) { // 154..156
            partyIdx = UNIT_DIEGO;
         } else { ///////////////// 151..153
            partyIdx = UNIT_ASH;
         }

         adv = 0;
         if (gPartyMembers[partyIdx].advLevelFirst != 0) {
            if (gPartyMembers[partyIdx].advChosePathB) {
               adv = 2;
            } else {
               adv = 1;
            }
            if (gPartyMembers[partyIdx].advLevelSecond != 0) {
               adv += 2;
            }
         }
         gSpriteStripUnitIds[i] += (adv * 6);
      } // 151..274

      if (id >= 1 && id <= 60) {
         adv = 0;
         partyIdx = (id - 1) % 12 + 1;
         if (gPartyMembers[partyIdx].advLevelFirst != 0) {
            if (gPartyMembers[partyIdx].advChosePathB) {
               adv = 2;
            } else {
               adv = 1;
            }
            if (gPartyMembers[partyIdx].advLevelSecond != 0) {
               adv += 2;
            }
         }
         gSpriteStripUnitIds[i] += (adv * 12);
      } // 1..60
   }

   for (i = 0; i < ARRAY_COUNT(gCurrentUnitSet); i++) {
      s16 tmp;
      id = gSpriteStripUnitIds[i];
      tmp = id;
      if (id >= 151 && id <= 300) {
         gCurrentUnitSet[i] = tmp;
      }
      if (id <= 60) {
         gCurrentUnitSet[i] = gSpriteStripUnitIds[i];
      }
   }
}

void SetDefaultStatsForParty(void) {
   s32 i, j;
   u8 level;
   s32 adv1, adv2;

   if (rand() % 2 != 0) {
      adv1 = 1;
      adv2 = 0;
   } else {
      adv1 = 0;
      adv2 = 1;
   }

   for (i = 1; i < PARTY_CT; i++) {
      level = gPartyDefaultLevels[i][gState.mapNum];

      for (j = 0; j < 8; j++) {
         gPartyMembers[i].experience[j] = gExperienceLevels[level - 1][j];
      }

      gPartyMembers[i].advChosePathB = 0;
      gPartyMembers[i].advLevelFirst = 0;
      gPartyMembers[i].advLevelSecond = 0;
      gPartyMembers[i].items[0] = ITEM_NULL;
      gPartyMembers[i].items[1] = ITEM_NULL;
      gPartyMembers[i].hpVar100 = 100;
      gPartyMembers[i].atkVar100 = 100;
      gPartyMembers[i].defVar100 = 100;
      gPartyMembers[i].agiVar100 = 100;

      do {
         // Vary party advancements
         if (i % 2 != 0) {
            gPartyMembers[i].advChosePathB = adv1;
         } else {
            gPartyMembers[i].advChosePathB = adv2;
         }
      } while (0);

      if (i == UNIT_ASH) {
         gPartyMembers[1].advChosePathB = 0;
      }
      if (level >= 10) {
         gPartyMembers[i].advLevelFirst = 10;
      }
      if (level >= 20) {
         gPartyMembers[i].advLevelSecond = 20;
      }
   }
}

void PopulateUnitSpellList(UnitStatus *unit) {
   s32 i;
   s32 partyIdx;
   s16 requiredLevel;

   if (unit->name < PARTY_CT && unit->name != UNIT_NULL) {
      partyIdx = unit->name;
      for (i = 0; i < ARRAY_COUNT(unit->spells); i++) {
         unit->spells[i] = gSpellLists[partyIdx][gPartyMembers[partyIdx].advChosePathB][i];
      }
      for (i = 0; i < ARRAY_COUNT(unit->spells); i++) {
         requiredLevel = gSpellLevelRequirement[unit->spells[i]];
         if (unit->level < requiredLevel) {
            unit->spells[i] = SPELL_NULL;
         } else {
            // Weird. This doesn't match as an else-if, unless nested in a sub-block.
            // Even weirder: while experimenting with variations, at some point removing the braces
            // still matched, but only with a line-break! Hopefully this kind of issue is rare.
            if ((requiredLevel > 20 && gPartyMembers[partyIdx].advLevelSecond == 0) ||
                (requiredLevel > 10 && gPartyMembers[partyIdx].advLevelFirst == 0)) {
               unit->spells[i] = SPELL_NULL;
            }
         }
      }
#ifdef PC_FEAT
      /* GAP 11 (Tactical): a Ninja -- 2nd promotion on path B (advLevelSecond set, advChosePathB==1) --
       * sheds the 3 inherited base-class spells, i.e. the reqLv<=10 ones shared with the Bishop/
       * Sorcerer path. A Monk (1st promotion only) keeps them as "past-life baggage"; the full
       * commitment at the 2nd promotion drops them for a pure Monk/Ninja specialist kit. By L20 the
       * basics are obsolete anyway (power scales them into irrelevance; Cure Wide already covers group
       * cure before 20; Perfect Guard strictly beats Mystic Shield; Spellbind is kept by the mage-path
       * sibling + the early Mad Book), so no real coverage is lost -- it just clears dead entries and
       * sharpens the class identity. Path A (Bishop/Archbishop) is untouched. Non-caster path-B units
       * (e.g. Dragoon) have empty spell lists, so this is a no-op for them. */
      if (gTacticalMode && gPartyMembers[partyIdx].advChosePathB == 1 &&
          gPartyMembers[partyIdx].advLevelSecond != 0) {
         s32 w = 0;
         for (i = 0; i < ARRAY_COUNT(unit->spells); i++) {
            if (unit->spells[i] != SPELL_NULL && gSpellLevelRequirement[unit->spells[i]] <= 10) {
               unit->spells[i] = SPELL_NULL;
            }
         }
         /* bugreport-05: GAP 11 nulls spells IN PLACE, leaving gaps -- the shed base spells sit BEFORE
          * the surviving specialists (e.g. [0,0,0,26,27,28,29,...]). The SKILL menu + battle iterate
          * until the first SPELL_NULL, so a leading gap hides everything after it and the Ninja reads
          * as having ZERO spells. Compact: shift survivors to the front, NULL-fill the tail, restoring
          * the contiguous NULL-terminated invariant the UI relies on. (The retail step-2 filter only
          * nulls the reqLv-ordered tail, so it never needs this -- only GAP 11's front-null does.) */
         for (i = 0; i < ARRAY_COUNT(unit->spells); i++) {
            if (unit->spells[i] != SPELL_NULL) {
               unit->spells[w++] = unit->spells[i];
            }
         }
         for (; w < ARRAY_COUNT(unit->spells); w++) {
            unit->spells[w] = SPELL_NULL;
         }
      }
      /* GAP 3: in Tactical, re-grant PLASMA_WAVE to Ash's de-godmoded Vandalier ONLY (weapon ==
       * V_HEART_2). Set directly into the first free slot -- Vandalier-exclusive (doesn't leak to Ash's
       * other promotions like the doc's gSpellLists-append would), and it never indexes
       * gSpellLevelRequirement[45], which would be an OOB read past that table's 36 entries. */
      if (gTacticalMode && unit->name == UNIT_ASH && unit->weapon == ITEM_V_HEART_2) {
         for (i = 0; i < ARRAY_COUNT(unit->spells); i++) {
            if (unit->spells[i] == SPELL_NULL) {
               unit->spells[i] = SPELL_PLASMA_WAVE;
               break;
            }
         }
      }
      /* bugreport-05 diagnostic (VH_SPELL_DUMP=1): read-only dump of the advancement state that drove
       * this spell list -- to see why a Ninja ends empty (advFirst==0 -> retail gate strips reqLv>10,
       * then GAP 11 strips reqLv<=10). No behavior change. */
      { extern void PC_SpellListDump(int, int, int, int, int, int, const void *);
        PC_SpellListDump((int)unit->name, (int)unit->class, (int)unit->level,
                         (int)gPartyMembers[partyIdx].advChosePathB,
                         (int)gPartyMembers[partyIdx].advLevelFirst,
                         (int)gPartyMembers[partyIdx].advLevelSecond, unit->spells); }
#endif
   }
}

void SetupPartyBattleUnit(u8 partyIdx, u8 z, u8 x, u8 direction) {
   UnitStatus *unit;
   Object *battler;
   s32 tmp;
   s32 i;
   u8 level;
   s32 class;

   if (gState.mapNum > 5 || gPartyMembers[partyIdx].inParty) {
      unit = CreateUnitInNextSlot();

      battler = Obj_GetUnused();
      unit->battler = battler;
      battler->functionIndex = OBJF_BATTLE_UNIT;
      battler->d.objf014.unit = unit;
      battler->d.objf014.unitIdx = unit->idx;
      battler->d.objf014.team = TEAM_PLAYER;

      unit->stripIdx = partyIdx + 1;
      unit->name = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].name;
      unit->unitType = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].unitType;
      unit->class = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].class;
      unit->step = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].step;
      unit->travelRange = gTravelRange[unit->step];
      unit->attackRange = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].attackRange;
      unit->ailmentSusceptibility =
          gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].ailmentSusceptibility;
      unit->magicSusceptibility = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].magicSusceptibility;
      unit->advantage = gUnitInfo[gSpriteStripUnitIds[partyIdx - 1]].advantage;
      unit->unitId = gSpriteStripUnitIds[partyIdx - 1];
      tmp = direction << 10; //?
      unit->direction = tmp;

      unit->weapon = gPartyMembers[partyIdx].weapon;
      unit->helmet = gPartyMembers[partyIdx].helmet;
      unit->armor = gPartyMembers[partyIdx].armor;
      unit->items[0] = gPartyMembers[partyIdx].items[0];
      unit->items[1] = gPartyMembers[partyIdx].items[1];
      unit->droppedItem = ITEM_NULL;

      unit->hpVar100 = gPartyMembers[partyIdx].hpVar100;
      unit->atkVar100 = gPartyMembers[partyIdx].atkVar100;
      unit->defVar100 = gPartyMembers[partyIdx].defVar100;
      unit->agiVar100 = gPartyMembers[partyIdx].agiVar100;

      level = 0;
      while (BigIntCompare(gPartyMembers[partyIdx].experience, gExperienceLevels[level]) < 2) {
         level++;
      }
      unit->level = level;

      for (i = 0; i < 8; i++) {
         unit->experience[i] = gPartyMembers[partyIdx].experience[i];
      }

      class = unit->class;
      if (unit->name == UNIT_ASH) {
         class = 7;
      }
      unit->mp = gClassMpMultiplier[class] * unit->level;
      if (unit->class == CLASS_MONK
#ifdef PC_FEAT
          /* GAP 5 (Tactical): drop the Monk/Ninja +advLevelFirst MP bonus for exact caster parity --
           * one of THREE sites that add it (also battle_0190dc.c + SyncPartyUnit below). */
          && !gTacticalMode
#endif
         ) {
         //? I guess monks can get a small mp boost by delaying advancement?
         unit->mp += gPartyMembers[partyIdx].advLevelFirst;
      }
      if (unit->mp > 99) {
         unit->mp = 99;
      }
      if (gState.debug) {
         unit->mp = 99;
      }
      if (gState.useDefaultUnits) {
         unit->weapon = gClassDefaultLoadouts[unit->class][0][gState.mapNum];
         unit->helmet = gClassDefaultLoadouts[unit->class][1][gState.mapNum];
         unit->armor = gClassDefaultLoadouts[unit->class][2][gState.mapNum];
      }
      unit->maxMp = unit->mp;
      PopulateUnitSpellList(unit);
      battler->x1.n = x * CV(1.0) + CV(0.5);
      battler->z1.n = z * CV(1.0) + CV(0.5);
   }
}

void SyncPartyUnit(u8 partyIdx) {
   UnitStatus *unit;
   s32 i;
   u8 level;
   s32 class;
   s32 unitId;

   unit = &gUnits[partyIdx];
   unit->idx = partyIdx;
   unit->team = TEAM_PLAYER;

   level = 0;
   while (BigIntCompare(gPartyMembers[partyIdx].experience, gExperienceLevels[level]) < 2) {
      level++;
   }
   unit->level = level;

   for (i = 0; i < 8; i++) {
      unit->experience[i] = gPartyMembers[partyIdx].experience[i];
   }

   if (gPartyMembers[partyIdx].advLevelSecond != 0 && level >= 20) {
      unitId = gPartyUnitIds[partyIdx][gPartyMembers[partyIdx].advChosePathB][2];
   } else if (gPartyMembers[partyIdx].advLevelFirst != 0 && level >= 10) {
      unitId = gPartyUnitIds[partyIdx][gPartyMembers[partyIdx].advChosePathB][1];
   } else {
      unitId = gPartyUnitIds[partyIdx][gPartyMembers[partyIdx].advChosePathB][0];
   }

   unit->name = gUnitInfo[unitId].name;
   unit->unitType = gUnitInfo[unitId].unitType;
   unit->class = gUnitInfo[unitId].class;
   unit->unitId = unitId;
   unit->weapon = gPartyMembers[partyIdx].weapon;
   unit->helmet = gPartyMembers[partyIdx].helmet;
   unit->armor = gPartyMembers[partyIdx].armor;
   unit->items[0] = gPartyMembers[partyIdx].items[0];
   unit->items[1] = gPartyMembers[partyIdx].items[1];
   unit->droppedItem = ITEM_NULL;
   unit->hpVar100 = gPartyMembers[partyIdx].hpVar100;
   unit->atkVar100 = gPartyMembers[partyIdx].atkVar100;
   unit->defVar100 = gPartyMembers[partyIdx].defVar100;
   unit->agiVar100 = gPartyMembers[partyIdx].agiVar100;

   class = unit->class;
   if (unit->name == UNIT_ASH) {
      class = 7;
   }
   unit->mp = gClassMpMultiplier[class] * unit->level;
   if (unit->class == CLASS_MONK
#ifdef PC_FEAT
       /* GAP 5 (Tactical): drop the Monk/Ninja +advLevelFirst MP bonus for exact caster parity (the
        * 3rd of the three sites; see battle_0190dc.c and the battle-unit setup above). */
       && !gTacticalMode
#endif
      ) {
      unit->mp += gPartyMembers[partyIdx].advLevelFirst;
   }
   if (unit->mp > 99) {
      unit->mp = 99;
   }
   unit->maxMp = unit->mp;
   PopulateUnitSpellList(unit);
   unit->hpFrac = 10000;
   CalculateUnitStats(unit);
}

#undef OBJF
#define OBJF 424
void Objf424_BattleEnder(Object *obj) {
   Object *newObj;

   switch (obj->state) {
   case 0:
      if (gState.debug && (gPadStateNewPresses & PAD_START)) {
         CommitPartyStatus();
         OBJ.timer = 1;
         obj->state = 3;
         obj->state2 = 3;
      }
      if (gState.battleEval == BATTLE_EVAL_NONE) {
         break;
      }
      if (gState.battleEval == BATTLE_EVAL_DEFEAT) {
         obj->state = 2;
      }
      if (gState.battleEval == BATTLE_EVAL_VICTORY) {
         obj->state = 1;
      }
      break;

   case 1:
      gClearSavedPadState = 1;

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_128_1);
         OBJ.timer = 20;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            OBJ.timer = 4;
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            LoadSeqSet(1);
            FinishLoadingSeq();
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(23));
            newObj = Obj_GetUnused();
            newObj->functionIndex = OBJF_BATTLE_VICTORY;
            obj->state += 2;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 2:
      gClearSavedPadState = 1;

      switch (obj->state2) {
      case 0:
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_4);
         OBJ.timer = 30;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            OBJ.timer = 4;
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            LoadSeqSet(1);
            FinishLoadingSeq();
            PerformAudioCommand(AUDIO_CMD_PLAY_SEQ(24));
            newObj = Obj_GetUnused();
            newObj->functionIndex = OBJF_BATTLE_DEFEAT;
            obj->state += 2;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 3:

      switch (obj->state2) {
      case 0:
         gSignal2 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal2 != 0) {
            obj->state2++;
         }
         break;

      case 2:
         if (gPadStateNewPresses & PAD_CIRCLE) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_4);
            FadeOutScreen(2, 4);
            OBJ.timer = 75;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            PerformAudioCommand(AUDIO_CMD_STOP_ALL);
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;

            switch (gState.mapNum) {
            case 0:
            case 1:
            case 2:
            case 3:
            case 4:
            case 5:
               SwapOutCodeToVram();
               gState.primary = STATE_TRIAL_COMPLETE;
               break;

            case 10:
               gState.primary = STATE_18;
               gState.scene = 2;
               break;

            case 11:
               gState.primary = STATE_12;
               gState.scene = 5;
               break;

            case 12:
               gState.primary = STATE_18;
               gState.scene = 9;
               break;

            case 13:
               gState.primary = STATE_18;
               gState.scene = 0xc;
               break;

            case 14:
               gState.primary = STATE_19;
               gState.scene = 0x10;
               break;

            case 15:
               gState.primary = STATE_18;
               gState.scene = 0x12;
               break;

            case 16:
               gState.primary = STATE_12;
               gState.scene = 7;
               break;

            case 17:
               gState.primary = STATE_12;
               gState.scene = 0x16;
               break;

            case 18:
               gState.primary = STATE_12;
               gState.scene = 0x19;
               break;

            case 19:
               gState.primary = STATE_12;
               gState.scene = 0x1c;
               break;

            case 20:
               gState.primary = STATE_12;
               gState.scene = 0x1e;
               break;

            case 21:
               gState.primary = STATE_19;
               gState.scene = 0x21;
               break;

            case 22:
               gState.primary = STATE_18;
               gState.scene = 0x24;
               break;

            case 23:
               gState.primary = STATE_18;
               gState.scene = 0x27;
               break;

            case 24:
               gState.primary = STATE_12;
               gState.scene = 0x29;
               break;

            case 25:
               gState.primary = STATE_12;
               gState.scene = 0x2c;
               break;

            case 26:
               gState.worldMapDestination = 0xf;
               SwapOutCodeToVram();
               gState.primary = STATE_6;
               gState.worldMapState = 0x2b;
               break;

            case 27:
               gState.primary = STATE_12;
               gState.scene = 0x31;
               break;

            case 28:
               gState.primary = STATE_12;
               gState.scene = 0x32;
               break;

            case 29:
               gState.primary = STATE_12;
               gState.scene = 0x36;
               break;

            case 30:
               gState.primary = STATE_12;
               gState.scene = 0x38;
               break;

            case 31:
               gState.primary = STATE_12;
               gState.scene = 0x3d;
               break;

            case 32:
               gState.primary = STATE_12;
               gState.scene = 0x40;
               break;

            case 33:
               gState.primary = STATE_12;
               gState.scene = 0x42;
               break;

            case 34:
               gState.worldMapDestination = 0x16;
               SwapOutCodeToVram();
               gState.primary = STATE_6;
               gState.worldMapState = 0x3d;
               break;

            case 35:
               gState.primary = STATE_12;
               gState.scene = 0x48;
               break;

            case 36:
               gState.primary = STATE_12;
               gState.scene = 0x4b;
               break;

            case 37:
               gState.worldMapDestination = 0x1b;
               SwapOutCodeToVram();
               gState.primary = STATE_6;
               gState.worldMapState = 0x42;
               break;

            case 38:
               gState.primary = STATE_12;
               gState.scene = 0x4f;
               break;

            case 39:
               gState.primary = STATE_12;
               gState.scene = 0x53;
               break;

            case 40:
               gState.worldMapDestination = 0x1e;
               SwapOutCodeToVram();
               gState.primary = STATE_6;
               gState.worldMapState = 0x48;
               break;

            case 41:
               gState.primary = STATE_12;
               gState.scene = 0x56;
               break;

            case 42:
               gState.primary = STATE_12;
               gState.scene = 0x5b;
               break;

            case 43:
               gState.primary = STATE_12;
               gState.scene = 0x5d;
               break;
            } // switch (gState.mapNum)
         }    // timer

         break;
      } // switch (obj->state2) (via state:3)

      break;

   case 4:

      switch (obj->state2) {
      case 0:
         gSignal2 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (gSignal2 != 0) {
            obj->state2++;
         }
         break;

      case 2:
         if (PressedCircleOrX()) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_4);
            OBJ.timer = 90;
            obj->state2++;
         }
         break;

      case 3:
         if (--OBJ.timer == 0) {
            gPlayerControlSuppressed = 1;
            newObj = Obj_GetUnused();
            newObj->functionIndex = OBJF_FILE_LOAD_MENU_DEFEAT;
            obj->state2++;
         }
         break;

      case 4:
         break;
      }

      break;
   }
}

void func_8005AA7C(void) {
   s32 i;

   for (i = 0; i < ARRAY_COUNT(gSpriteStripUnitIds); i++) {
      gSpriteStripUnitIds[i] = 0;
   }

   if (gState.mapNum < BATTLE_CT) {
      for (i = 0; i < 20; i++) {
         gSpriteStripUnitIds[i] = gSceneSpriteStripUnitIds[gBattleSceneId2[gState.mapNum]][i];
      }
      for (i = 0; i < 20; i++) {
         gCurrentUnitSet[i] = gSceneUnitSets[gBattleSceneId2[gState.mapNum]][i];
      }
   }

   for (i = 0; i < PARTY_CT - 1; i++) {
      if (gSpriteStripUnitIds[i] == i + 1) {
         if (gState.mapNum > 5) {
            gPartyMembers[gSpriteStripUnitIds[i]].inParty = 1;
         }
      } else {
         gPartyMembers[i + 1].inParty = 0;
      }
   }

   if (gState.mapNum != 8) {
      SetupPartySprites();
   }
}

void SetupBattleUnit(s16 stripIdx, u8 z, u8 x, s8 level, u8 team, u8 direction, u8 to_x1c,
                     u8 expMulti, u8 to_x1d) {
   extern UnitStatus *gTempUnitPtr;

   Object *battler;
   s32 i;

   if (gSpriteStripUnitIds[stripIdx] == UNIT_ID_LEENA) {
      gTempUnitPtr = CreateUnitInLastSlot();
   } else {
      gTempUnitPtr = CreateUnitInNextSlot();
   }

   battler = Obj_GetUnused();
   gTempUnitPtr->battler = battler;
   battler->functionIndex = OBJF_BATTLE_UNIT;
   battler->d.objf014.unit = gTempUnitPtr;
   battler->d.objf014.unitIdx = gTempUnitPtr->idx;
   battler->d.objf014.team = team;

   if (gState.mapNum == 8) {
      // Demo battle
      battler->d.objf014.team = (rand() % 70) + 2;
   }
   if (gState.mapNum == 9 && x > 8) {
      // Test battle
      battler->d.objf014.team = TEAM_PLAYER;
   }

   gTempUnitPtr->stripIdx = stripIdx + 2;
   gTempUnitPtr->name = gUnitInfo[gSpriteStripUnitIds[stripIdx]].name;
   gTempUnitPtr->unitType = gUnitInfo[gSpriteStripUnitIds[stripIdx]].unitType;
   if (gTempUnitPtr->unitType == UNIT_TYPE_MAGE_TOWER ||
       (gTempUnitPtr->name == 30 || gTempUnitPtr->name == 31)) {
      battler->d.objf014.team = TEAM_PLAYER;
   }

   gTempUnitPtr->class = gUnitInfo[gSpriteStripUnitIds[stripIdx]].class;
   gTempUnitPtr->weapon = gUnitInfo[gSpriteStripUnitIds[stripIdx]].weapon;
   gTempUnitPtr->helmet = gUnitInfo[gSpriteStripUnitIds[stripIdx]].helmet;
   gTempUnitPtr->armor = gUnitInfo[gSpriteStripUnitIds[stripIdx]].armor;
   gTempUnitPtr->step = gUnitInfo[gSpriteStripUnitIds[stripIdx]].step;
   gTempUnitPtr->travelRange = gTravelRange[gTempUnitPtr->step];
   gTempUnitPtr->attackRange = gUnitInfo[gSpriteStripUnitIds[stripIdx]].attackRange;
   gTempUnitPtr->ailmentSusceptibility =
       gUnitInfo[gSpriteStripUnitIds[stripIdx]].ailmentSusceptibility;
   gTempUnitPtr->magicSusceptibility = gUnitInfo[gSpriteStripUnitIds[stripIdx]].magicSusceptibility;
   gTempUnitPtr->advantage = gUnitInfo[gSpriteStripUnitIds[stripIdx]].advantage;
   gTempUnitPtr->unitId = gSpriteStripUnitIds[stripIdx];
   // tmp = direction << 10;
   gTempUnitPtr->direction = direction << 10;
   gTempUnitPtr->field7_0x1c = to_x1c; // TBD: involved in ai behavior; related to travel range
   gTempUnitPtr->mp = gUnitInfo[gSpriteStripUnitIds[stripIdx]].maxMp;
   gTempUnitPtr->maxMp = gUnitInfo[gSpriteStripUnitIds[stripIdx]].maxMp;

   for (i = 0; i < ARRAY_COUNT(gTempUnitPtr->spells); i++) {
      gTempUnitPtr->spells[i] = SPELL_NULL;
   }
   gTempUnitPtr->spells[0] = gUnitInfo[gSpriteStripUnitIds[stripIdx]].spell1;
   gTempUnitPtr->spells[1] = gUnitInfo[gSpriteStripUnitIds[stripIdx]].spell2;
   gTempUnitPtr->items[0] = ITEM_NULL;
   gTempUnitPtr->items[1] = ITEM_NULL;
   gTempUnitPtr->droppedItem = gUnitInfo[gSpriteStripUnitIds[stripIdx]].heldItem;

   if (gState.mapNum < 9) {
      gTempUnitPtr->droppedItem = ITEM_NULL;
   }

   if (gState.mapNum == 8) {
      // Demo battle
      gTempUnitPtr->spells[0] = SPELL_NULL;
      gTempUnitPtr->spells[1] = SPELL_NULL;
      gTempUnitPtr->mp = 50;
      gTempUnitPtr->maxMp = 50;

      if (gTempUnitPtr->unitId == UNIT_ID_DARK_MAGE) {
         gTempUnitPtr->spells[0] = SPELL_DARK_HURRICANE;
      }
      if (gTempUnitPtr->unitId == UNIT_ID_ELENI_MAGE) {
         gTempUnitPtr->spells[0] = SPELL_SALAMANDER;
      }
      if (gTempUnitPtr->unitId == UNIT_ID_ZOHAR_SORCEROR) {
         gTempUnitPtr->spells[0] = SPELL_AVALANCHE;
      }
      if (gTempUnitPtr->unitId == UNIT_ID_DUMAS) {
         gTempUnitPtr->spells[0] = SPELL_PLASMA_WAVE;
      }
   }

   gTempUnitPtr->hpVar100 = 100;
   gTempUnitPtr->atkVar100 = 100;
   gTempUnitPtr->defVar100 = 100;
   gTempUnitPtr->agiVar100 = 100;
   gTempUnitPtr->field8_0x1d = to_x1d; // TBD: involved in ai behavior; related to ally count
   gTempUnitPtr->expMulti = expMulti;

   if (gState.mapNum <= 5) {
      // Trials of Toroah
#ifdef PC_FEAT
      /* GAP 2: in Tactical, spawn trial enemies at the per-chapter cap instead of copying Ash's EXP
       * (which benches Ash trivialises the trial). Derive experience[] the same way the else branch
       * does, so the derived level is consistent. Normal mode keeps the retail Ash-copy below. */
      if (gTacticalMode) {
         int lvl = TacticalCap(gState.chapter);
         for (i = 0; i < 8; i++) {
            gTempUnitPtr->experience[i] = gExperienceLevels[lvl - 1][i];
         }
         gTempUnitPtr->level = lvl;
         /* GAP 9: retail trial enemies carry expMulti 0, so physical attacks paid no XP (only support
          * did). Restore a per-chapter multiplier so clearing a trial rewards attackers too; the level
          * cap keeps the total bounded. Paired with expScalingLevel in battle_eval.c. */
         gTempUnitPtr->expMulti = TrialEnemyExpMulti(gState.chapter);
      } else {
#endif
      for (i = 0; i < 8; i++) {
         gTempUnitPtr->experience[i] = gPartyMembers[UNIT_ASH].experience[i];
      }
      i = 0;
      while (BigIntCompare(gTempUnitPtr->experience, gExperienceLevels[i]) < 2) {
         i++;
      }
      gTempUnitPtr->level = i;
#ifdef PC_FEAT
      }
#endif
   } else {
      for (i = 0; i < 8; i++) {
         gTempUnitPtr->experience[i] = gExperienceLevels[level - 1][i];
      }
      gTempUnitPtr->level = level;
   }

   if (gState.debug && gTempUnitPtr->name <= UNIT_END_OF_PARTY) {
      gTempUnitPtr->mp = 99;
      gTempUnitPtr->maxMp = 99;
   }

   battler->x1.n = x * CV(1.0) + CV(0.5);
   battler->z1.n = z * CV(1.0) + CV(0.5);
}

void CreateBattleUnitForUnit(UnitStatus *unit) {
   Object *battler;

   battler = Obj_GetUnused();
   unit->battler = battler;
   battler->functionIndex = OBJF_BATTLE_UNIT;
   battler->d.objf014.unit = unit;
   battler->d.objf014.unitIdx = unit->idx;
   battler->d.objf014.team = unit->team;
   battler->x1.s.hi = unit->tileX;
   battler->z1.s.hi = unit->tileZ;
   battler->x1.s.lo = CV(0.5);
   battler->z1.s.lo = CV(0.5);
}

void SetupBattleUnits(void) {
   BattleEnemyUnitInitialState *pEnemyInit;
   BattlePartyUnitInitialState *pPartyInit;

   pEnemyInit = &gBattleEnemyUnitInitialStates[gState.mapNum][0];
   while (pEnemyInit->stripIdx != 0) {
      SetupBattleUnit(pEnemyInit->stripIdx, pEnemyInit->z, pEnemyInit->x, pEnemyInit->level,
                      TEAM_ENEMY, pEnemyInit->direction, pEnemyInit->to_x1c, pEnemyInit->expMulti,
                      pEnemyInit->to_x1d);
      pEnemyInit++;
   }

   pPartyInit = &gBattlePartyUnitInitialStates[gState.mapNum][0];
   while (pPartyInit->partyIdx != UNIT_INVALID) {
      SetupPartyBattleUnit(pPartyInit->partyIdx, pPartyInit->z, pPartyInit->x,
                           pPartyInit->direction);
      pPartyInit++;
   }
}

s32 PCDrv_WriteFile(s8 *filename, void *buf, s32 size) {
   const s32 CHUNK_SIZE = 0xfffe;
   s32 fd;
   s32 offset;
   s32 chunks;
   s32 rem;
   u8 *p;

   fd = PCcreat(filename, 0);
   PClseek(fd, 0, 0);

   chunks = size / CHUNK_SIZE;
   rem = size % CHUNK_SIZE;
   offset = 0;
   p = (u8 *)buf;

   while (chunks > 0) {
      PClseek(fd, offset, 0);
      PCwrite(fd, p, CHUNK_SIZE);
      offset += CHUNK_SIZE;
      p += CHUNK_SIZE;
      chunks--;
   }

   if (rem != 0) {
      PCwrite(fd, p, rem);
   }

   PCclose(fd);
   return 0;
}

void ResetGeomOffset(void) {
   gGeomOffsetX = SCREEN_HALF_WIDTH;
   gGeomOffsetY = SCREEN_HALF_HEIGHT;
   SetGeomOffset(gGeomOffsetX, gGeomOffsetY);
}

void Initialize(void) {
   MATRIX localColorMatrix = {{{ONE, 0, 0}, {ONE, 0, 0}, {ONE, 0, 0}}};
   s32 i;

   ResetCallback();
   PadInit(0);
   Card_Init();
   CDInit();
   ResetGraph(0);
   SetGraphDebug(0);
   InitGeom();
   ResetGeomOffset();
   SetGeomScreen(0x200);
   SetBackColor(0, 0, 0);
   SetColorMatrix(&localColorMatrix);
   SetDefDrawEnv(&gGraphicBuffers[0].drawEnv, 0, 16, SCREEN_WIDTH, SCREEN_HEIGHT);
   SetDefDrawEnv(&gGraphicBuffers[1].drawEnv, 0, 272, SCREEN_WIDTH, SCREEN_HEIGHT);
   SetDefDispEnv(&gGraphicBuffers[0].dispEnv, 0, 272, SCREEN_WIDTH, SCREEN_HEIGHT);
   SetDefDispEnv(&gGraphicBuffers[1].dispEnv, 0, 16, SCREEN_WIDTH, SCREEN_HEIGHT);
   gGraphicBuffers[0].drawEnv.dtd = gGraphicBuffers[1].drawEnv.dtd = 0;
   gGraphicBuffers[0].drawEnv.isbg = gGraphicBuffers[1].drawEnv.isbg = 0;
   gGraphicBuffers[0].dispEnv.isinter = gGraphicBuffers[1].dispEnv.isinter = 0;
   gGraphicBuffers[0].dispEnv.isrgb24 = gGraphicBuffers[1].dispEnv.isrgb24 = 0;
   gTempRect.x = 0;
   gTempRect.y = 0;
   gTempRect.w = VRAM_WIDTH;
   gTempRect.h = VRAM_HEIGHT;
   ClearImage(&gTempRect, 0, 0, 0);
   DrawSync(0);
   SetupQuads(&gGraphicBuffers[0]);
   SetupQuads(&gGraphicBuffers[1]);

   for (i = 9; i != -1; i--) {
      gGraphicsPtr =
          (gGraphicsPtr == &gGraphicBuffers[0]) ? &gGraphicBuffers[1] : &gGraphicBuffers[0];
      ClearOTag(gGraphicsPtr->ot, OT_SIZE);
      gTempRect.x = 0;
      gTempRect.y = 0;
      gTempRect.w = VRAM_WIDTH;
      gTempRect.h = VRAM_HEIGHT;
      ClearImage(&gTempRect, 0, 0, 0);
      DrawSync(0);
      VSync(0);
      PutDrawEnv(&gGraphicsPtr->drawEnv);
      PutDispEnv(&gGraphicsPtr->dispEnv);
      DrawOTag(gGraphicsPtr->ot);
   }
}

void SetupLight(void) {
   gLightRotation.vx = 0;
   gLightRotation.vy = 0xe00;
   gLightRotation.vz = 0xa00;
   SetupLightNormals();
   UpdateMapLighting();
   gLightColor.r = 0x80;
   gLightColor.g = 0x80;
   gLightColor.b = 0x80;
}

void SetupQuads(Graphics *g) {
   s32 i;

   g->drawEnv.isbg = 0;
   g->drawEnv.r0 = 5;
   g->drawEnv.g0 = 5;
   g->drawEnv.b0 = 120;

   for (i = 0; i < ARRAY_COUNT(g->quads); i++) {
      SetPolyFT4(&g->quads[i]);
   }
}

void SetupSprites(void) {
   s32 i;

   for (i = 0; i < ARRAY_COUNT(gSpriteStripUnitIds); i++) {
      //?: Won't this read out-of-bounds of gUnitAnimSets?
      gSpriteStripAnimSets[2 + i] = gUnitAnimSets[gSpriteStripUnitIds[i]];
      gSpriteStripClutIds[2 + i] = gUnitClutIds[gSpriteStripUnitIds[i]];
   }

   for (i = 0; i < ARRAY_COUNT(gSpriteStripClutIds); i++) {
      gSpriteStripClutIds[i] = gClutIds[gSpriteStripClutIds[i]];
   }
}

void SetupMap(void) {
   Object *newObj;

   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_BATTLE_MAP_CURSOR_CONTROL;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_BATTLE_MAP_CURSOR;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_COMPASS;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_PANORAMA;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_BATTLE_MGR;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_FIELD_INFO;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_BATTLE_OPTIONS;
   newObj = Obj_GetUnused();
   newObj->functionIndex = OBJF_SETUP_MAP_OBJECTS;

   SetupMapExtras();
}

void LoadMapTextures(void) {
   const NUM_TEXTURES = 257;
   s32 i, j;
   u8 *pFileData;
   u8 *pClutData;

   gTempRect.w = 256 >> 2;
   gTempRect.h = 256;
   gTempRect.x = 320;
   gTempRect.y = 0;

   switch (gState.mapNum) {
   case 3:
   case 5:
   case 8:
   case 12:
   case 16:
   case 17:
   case 21:
   case 24:
   case 26:
      LoadCdFile(CDF_F_TEX02_DAT, 1);
      break;

   case 14:
      LoadCdFile(CDF_F_TEX03_DAT, 1);
      break;

   case 15:
   case 34:
      LoadCdFile(CDF_F_TEX04_DAT, 1);
      break;

   case 4:
   case 18:
   case 19:
   case 25:
   case 40:
   case 66:
   case 67:
      LoadCdFile(CDF_F_TEX05_DAT, 1);
      break;

   case 20:
   case 44:
   case 61:
      LoadCdFile(CDF_F_TEX06_DAT, 1);
      break;

   case 22:
   case 23:
      LoadCdFile(CDF_F_TEX07_DAT, 1);
      break;

   case 27:
   case 31:
   case 46:
   case 49:
   case 54:
      LoadCdFile(CDF_F_TEX08_DAT, 1);
      break;

   case 28:
   case 30:
      LoadCdFile(CDF_F_TEX09_DAT, 1);
      break;

   case 29:
      LoadCdFile(CDF_F_TEX10_DAT, 1);
      break;

   case 32:
      LoadCdFile(CDF_F_TEX11_DAT, 1);
      break;

   case 33:
   case 68:
      LoadCdFile(CDF_F_TEX12_DAT, 1);
      break;

   case 2:
   case 35:
      LoadCdFile(CDF_F_TEX13_DAT, 1);
      break;

   case 0:
   case 36:
      LoadCdFile(CDF_F_TEX14_DAT, 1);
      break;

   case 1:
   case 37:
   case 43:
      LoadCdFile(CDF_F_TEX15_DAT, 1);
      break;

   case 38:
      LoadCdFile(CDF_F_TEX16_DAT, 1);
      break;

   case 39:
      LoadCdFile(CDF_F_TEX17_DAT, 1);
      break;

   case 42:
      LoadCdFile(CDF_F_TEX18_DAT, 1);
      break;

   case 45:
      LoadCdFile(CDF_F_IVE01_DAT, 1);
      break;

   case 47:
   case 48:
   case 50:
   case 51:
      LoadCdFile(CDF_F_IVE02_DAT, 1);
      break;

   case 52:
   case 53:
   case 55:
      LoadCdFile(CDF_F_IVE03_DAT, 1);
      break;

   case 56:
   case 57:
   case 58:
   case 62:
      LoadCdFile(CDF_F_IVE04_DAT, 1);
      break;

   case 59:
   case 60:
      LoadCdFile(CDF_F_IVE05_DAT, 1);
      break;

   case 63:
      LoadCdFile(CDF_F_IVE06_DAT, 1);
      break;

   default:
      LoadCdFile(CDF_F_TEX01_DAT, 1);
      break;
   }

   LoadImage(&gTempRect, &gScratch3_80180210[0]);
   DrawSync(0);

   LoadCdFile(CDF_LAND_DT_DAT, 1);
   pFileData = &gScratch3_80180210[gState.mapNum * 1797];

   for (i = GFX_MAP_TEXTURES_OFS; i < GFX_MAP_TEXTURES_OFS + NUM_TEXTURES; i++) {
      gGfxSubTextures[i][0] = *pFileData++;
      gGfxSubTextures[i][1] = *pFileData++;
      gGfxSubTextures[i][2] = *pFileData++;
      gGfxSubTextures[i][3] = *pFileData++;
   }

   for (i = GFX_MAP_TEXTURES_OFS; i < GFX_MAP_TEXTURES_OFS + NUM_TEXTURES; i++) {
      gGfxClutIds[i] = *pFileData++;
   }

   pClutData = &gScratch1_801317c0[0];
   for (i = 0; i < 512; i++) {
      *pClutData++ = *pFileData++;
   }

   gTempRect.x = 672;
   gTempRect.y = 496;
   gTempRect.w = 16;
   gTempRect.h = 16;
   LoadImage(&gTempRect, gScratch1_801317c0);
   DrawSync(0);

   i = 32;
   for (j = 0; j < 16; j++) {
      gClutIds[i++] = GetClut(672, j + 496);
   }

   for (i = GFX_MAP_TEXTURES_OFS; i < GFX_MAP_TEXTURES_OFS + NUM_TEXTURES; i++) {
      gGfxClutIds[i] = gClutIds[gGfxClutIds[i]];
      gGfxTPageIds[i] = gTPageIds[gGfxTPageCells[i]];
   }
}

void LoadFWD(void) {
   DrawSync(0);
   LoadCdFile(CDF_F_WD_DAT, 1);
   gTempRect.x = 640;
   gTempRect.y = 256;
   gTempRect.w = 256 >> 2;
   gTempRect.h = 96;
   LoadImage(&gTempRect, &gScratch3_80180210[0]);
   gTempRect.h = 256;
   gTempRect.x = 320;
   gTempRect.y = 256;
   LoadImage(&gTempRect, &gScratch3_80180210[0x8000]);
   gTempRect.x = 384;
   LoadImage(&gTempRect, &gScratch3_80180210[0x10000]);
   gTempRect.x = 448;
   LoadImage(&gTempRect, &gScratch3_80180210[0x18000]);
   gTempRect.x = 384;
   gTempRect.y = 0;
   LoadImage(&gTempRect, &gScratch3_80180210[0x20000]);
   DrawSync(0);
}

void LoadPortraits(void) {
   s32 i, j;
   // s32 adv, ct;
   s32 t0;
   s16 advOfs[PARTY_CT];

   DrawSync(0);

   for (j = 0, i = 1; i < PARTY_CT; i++, j++) {
      t0 = 0;
      if (gPartyMembers[i].advLevelFirst != 0) {
         if (gPartyMembers[i].advLevelSecond != 0) {
            t0 = 3;
         } else {
            t0 = 1;
         }
         if (gPartyMembers[i].advChosePathB) {
            t0 += 1;
         }
      }
      // Slots 0..11 (party) 12..23 (injured party)
      gState.portraitsToLoad[j] = 1 + (i - 1) * 7 + t0 * 12 * 7;
      gState.portraitsToLoad[j + 12] = gState.portraitsToLoad[j] + 1;
      advOfs[i] = t0 * 12 * 7;
   }

   // Extra portraits/expressions needed by the scene (adjusted to match advancement)
   for (i = 24; i < 50; i++) {
      t0 = gState.portraitsToLoad[i];
      if (t0 <= PORTRAIT_END_OF_PARTY) {
         t0 = (t0 - 1) / 7;
         t0 %= 12;
         gState.portraitsToLoad[i] += advOfs[t0 + 1];
      }
   }

   // Load in associated cluts / overlays
   for (i = 0; i < 50; i++) {
      t0 = gState.portraitsToLoad[i];
      for (j = 0; j < 4; j++) {
         gPortraitOverlayOffsets[i].bytes[j] = gPortraitOverlayOffsetsDb[t0].bytes[j];
      }
      gPortraitClutIds[i] = gPortraitsDb.cluts[t0];
   }

   // For each of the five portrait files, scan for any not-already-loaded ids provided by it (e.g.,
   // F_CLAS01.DAT provides ids 0..143)

   t0 = 0;
   for (i = 0; i < 50; i++) {
      j = gState.portraitsToLoad[i];
      if (j != 0 && j < 144 && j != gState.currentPortraits[i]) {
         t0++;
      }
   }
   if (t0 != 0) {
      LoadCdFile(CDF_F_CLAS01_DAT, 1);
      LoadProvidedPortraits(0);
   }

   t0 = 0;
   for (i = 0; i < 50; i++) {
      j = gState.portraitsToLoad[i];
      if (j != 0 && j >= 144 && j < 288 && j != gState.currentPortraits[i]) {
         t0++;
      }
   }
   if (t0 != 0) {
      LoadCdFile(CDF_F_CLAS02_DAT, 1);
      LoadProvidedPortraits(1);
   }

   t0 = 0;
   for (i = 0; i < 50; i++) {
      j = gState.portraitsToLoad[i];
      if (j != 0 && j >= 288 && j < 432 && j != gState.currentPortraits[i]) {
         t0++;
      }
   }
   if (t0 != 0) {
      LoadCdFile(CDF_F_CLAS03_DAT, 1);
      LoadProvidedPortraits(2);
   }

   t0 = 0;
   for (i = 0; i < 50; i++) {
      j = gState.portraitsToLoad[i];
      if (j != 0 && j >= 432 && j < 576 && j != gState.currentPortraits[i]) {
         t0++;
      }
   }
   if (t0 != 0) {
      LoadCdFile(CDF_F_FACE01_DAT, 1);
      LoadProvidedPortraits(3);
   }

   t0 = 0;
   for (i = 0; i < 50; i++) {
      j = gState.portraitsToLoad[i];
      if (j != 0 && j >= 576 && j < 720 && j != gState.currentPortraits[i]) {
         t0++;
      }
   }
   if (t0 != 0) {
      LoadCdFile(CDF_F_FACE02_DAT, 1);
      LoadProvidedPortraits(4);
   }

   for (i = 0; i < 50; i++) {
      t0 = gState.portraitsToLoad[i];
      if (t0 <= PORTRAIT_END_OF_PARTY) {
         t0 = (t0 - 1) / 7;
         t0 %= 12;
         gState.portraitsToLoad[i] -= advOfs[t0 + 1];
      }
   }
}

void LoadProvidedPortraits(s32 fileIdx) {
   const s32 PORTRAITS_PER_FILE = 144;
   const s32 PORTRAIT_WIDTH = 48 >> 2;
   const s32 PORTRAIT_HEIGHT = 48;
   const s32 OVERLAY_WIDTH = 16 >> 2;
   const s32 OVERLAY_HEIGHT = 16;
   const s32 BYTES_PER_PORTRAIT = 1536;
   // const s32 BYTES_PER_PORTRAIT =
   //    ((PORTRAIT_WIDTH * PORTRAIT_HEIGHT) + (3 * OVERLAY_WIDTH * OVERLAY_HEIGHT)) * 2;
   s32 i;
   u8 *p;
   s32 idx;
   s32 firstId;

   firstId = fileIdx;
   firstId *= PORTRAITS_PER_FILE;
   for (i = 0; i < 50; i++) {
      idx = gState.portraitsToLoad[i];
      if (idx != gState.currentPortraits[i] && idx != 0 && idx >= firstId &&
          idx < firstId + PORTRAITS_PER_FILE) {
         gState.currentPortraits[i] = idx;
         idx -= firstId;
         p = &gScratch3_80180210[idx * BYTES_PER_PORTRAIT];

         idx = i;
         if (i < 25) {
            gTempRect.x = 640;
            gTempRect.y = 0;
         } else {
            idx -= 25;
            gTempRect.x = 448;
            gTempRect.y = 0;
         }
         gTempRect.x += (idx % 5) * PORTRAIT_WIDTH;
         gTempRect.y += (idx / 5) * PORTRAIT_HEIGHT;
         gTempRect.w = PORTRAIT_WIDTH;
         gTempRect.h = PORTRAIT_HEIGHT;
         // Face
         LoadImage(&gTempRect, p);
         DrawSync(0);

         // Load the overlay graphics used for speaking/blinking animations
         idx = i;
         if (i >= 25) {
            idx -= 25;
            gTempRect.y = 256;
         } else {
            gTempRect.y = 0;
         }
         gTempRect.w = OVERLAY_WIDTH;
         gTempRect.h = OVERLAY_HEIGHT;
         gTempRect.x = (idx / 5) * 64 + (idx % 5) * 12;
         // Mouth overlay 1/2
         LoadImage(&gTempRect, p + 1152);
         DrawSync(0);
         gTempRect.x += OVERLAY_WIDTH;
         // Mouth overlay 2/2
         LoadImage(&gTempRect, p + 1280);
         DrawSync(0);
         gTempRect.x += OVERLAY_WIDTH;
         // Eye-blink overlay
         LoadImage(&gTempRect, p + 1408);
         DrawSync(0);
      }
   }
}

void LoadFCOM4XX(void) {
   gTempRect.w = 256 >> 2;
   gTempRect.h = 256;
   gTempRect.x = 384;
   gTempRect.y = 0;

   switch (gState.mapNum) {
   case 10:
   case 16:
   case 24:
   case 29:
      LoadCdFile(CDF_F_COM41X_DAT, 1);
      break;

   case 8:
   case 13:
   case 15:
   case 30:
   case 34:
   case 39:
      LoadCdFile(CDF_F_COM42X_DAT, 1);
      break;

   case 12:
   case 19:
   case 26:
   case 35:
   case 42:
      LoadCdFile(CDF_F_COM43X_DAT, 1);
      break;

   case 14:
   case 18:
   case 28:
   case 33:
   case 36:
      LoadCdFile(CDF_F_COM44X_DAT, 1);
      break;

   case 22:
   case 23:
   case 37:
      LoadCdFile(CDF_F_COM45X_DAT, 1);
      break;

   case 11:
   case 17:
   case 25:
   case 32:
   case 38:
   case 40:
   case 41:
   case 63:
      LoadCdFile(CDF_F_COM46X_DAT, 1);
      break;

   case 0:
   case 1:
   case 2:
   case 3:
   case 4:
   case 5:
   case 21:
   case 58:
   case 64:
   case 65:
   case 66:
   case 67:
   case 68:
      LoadCdFile(CDF_F_COM48X_DAT, 1);
      break;

   default:
      LoadCdFile(CDF_F_COM47X_DAT, 1);
      break;
   }

   LoadImage(&gTempRect, &gScratch3_80180210[0]);
   DrawSync(0);
}

void LoadItemIcons(void) {
   const s32 ICON_WIDTH = 16 >> 2;
   const s32 ICON_HEIGHT = 16;
   s32 i, j;
   s16 col;
   s16 x, y;

   DrawSync(0);
   LoadCdFile(CDF_F_ITEM_DAT, 1);
   gTempRect.x = 704;
   gTempRect.y = 0;
   gTempRect.w = 256 >> 2;
   gTempRect.h = 256;
   LoadImage(&gTempRect, &gScratch3_80180210[0]);
   DrawSync(0);

   gTempRect.w = ICON_WIDTH;
   gTempRect.h = ICON_HEIGHT;
   gTempRect.x = 704;
   gTempRect.y = 0;

   x = 700;
   y = 352;
   col = 0;
   for (i = 0; i < 9; i++) {
      MoveImage(&gTempRect, x, y);
      col++;
      y += ICON_HEIGHT;
      gTempRect.x += ICON_WIDTH;
   }

   x = 764;
   y = 352;
   for (i = 0; i < 9; i++) {
      MoveImage(&gTempRect, x, y);
      gTempRect.x += ICON_WIDTH;
      y += ICON_HEIGHT;
      col++;
      if (col == 16) {
         col = 0;
         gTempRect.x = 704;
         gTempRect.y += ICON_HEIGHT;
      }
   }

   for (j = 2; j < 6; j++) {
      x = 700 + j * 64;
      y = 0;
      for (i = 0; i < 15; i++) {
         MoveImage(&gTempRect, x, y);
         gTempRect.x += ICON_WIDTH;
         y += ICON_HEIGHT;
         col++;
         if (col == 16) {
            col = 0;
            gTempRect.x = 704;
            gTempRect.y += ICON_HEIGHT;
         }
      }
   }

   for (j = 2; j < 6; j++) {
      x = 700 + j * 64;
      y = 256;
      for (i = 0; i < 15; i++) {
         MoveImage(&gTempRect, x, y);
         gTempRect.x += ICON_WIDTH;
         y += ICON_HEIGHT;
         col++;
         if (col == 16) {
            col = 0;
            gTempRect.x = 704;
            gTempRect.y += ICON_HEIGHT;
         }
      }
   }

   DrawSync(0);
}

u8 s_idsConverted_8012333c;

void SetupGfx(void) {
   s32 i, j, k; //?
   s32 y;
   s16 *pClut;

   DrawSync(0);
   LoadCdFile(CDF_COL_DAT_DAT, 1);

   for (i = 0; i < 4; i++) {
      y = 0;
      for (j = 0; j < 32; j++) {
         k = j;
         if (k >= 16) {
            k -= 16;
            y = 256;
         }
         gTPageIds[i * 32 + j] = GetTPage(0, i, k * 64, y);
      }
   }

   pClut = (s16 *)&gScratch1_801317c0[0];
   for (j = 0; j < 108; j++) {
      for (k = 0; k < 16; k++) {
         if ((j >= 16 && j < 25) || j == 25) {
            if (k == 0) {
               *pClut |= (1 << 15);
            }
         } else {
            if (*pClut != 0) {
               *pClut |= (1 << 15);
            }
         }
         pClut++;
      }
   }

   DrawSync(0);
   gTempRect.x = 640;
   gTempRect.y = 496;
   gTempRect.w = 16;
   gTempRect.h = 16;

   for (j = 0; j < 13; j++) {
      LoadImage(&gTempRect, &gScratch1_801317c0[j * 512]);
      gTempRect.x += 16;
   }
   DrawSync(0);

   j = 0;
   for (i = 0; i < 8; i++) {
      for (k = 0; k < 16; k++) {
         gClutIds[j++] = GetClut((i * 16) + 640, k + 496);
      }
   }

   if (!s_idsConverted_8012333c) {
      s_idsConverted_8012333c++;
      for (j = 0; j < GFX_CT; j++) {
         gGfxClutIds[j] = gClutIds[gGfxClutIds[j]];
         gGfxTPageIds[j] = gTPageIds[gGfxTPageCells[j]];
      }
   }

   for (j = 0; j < ARRAY_COUNT(gSpriteStripTPageIds); j++) {
      gSpriteStripTPageIds[j] = gTPageIds[gSpriteStripTPageCells[j]];
   }
}

void LoadMap(void) {
   s32 i;
   s32 ix, iz;
   s32 zSize;
   // s32 faceCt;
   s32 iFace;
   MapTileModel(*pTiles)[28];

   gMapDataSizeX = 28;
   gMapDataSizeZ = 22;
   gMapSizeX = 28;
   gMapSizeZ = 22;
   gMapMarginX = 13;
   gMapMarginZ = 13;
   gMapMinX = 0;
   gMapMinZ = 0;
   gMapMaxX = 27;
   gMapMaxZ = 21;

   for (i = 0; i < 22; i++) {
      gMapRowPointers[i] = ((MapTileModel(*)[28])gMapDataPtr)[i];
   }

   switch (gState.mapNum) {
   case 0:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 19;
      gMapSizeZ = 16;
      gMapMarginX = 9;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 18;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_KEY01_PRS, 1);
      break;

   case 1:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 24;
      gMapSizeZ = 16;
      gMapMarginX = 11;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_KEY02_PRS, 1);
      break;

   case 2:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 21;
      gMapSizeX = 24;
      gMapSizeZ = 21;
      gMapMarginX = 11;
      gMapMarginZ = 10;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 20;
      LoadCdFile(CDF_M_KEY03_PRS, 1);
      break;

   case 3:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 20;
      gMapSizeZ = 16;
      gMapMarginX = 9;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 19;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_KEY04_PRS, 1);
      break;

   case 4:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 20;
      gMapSizeX = 20;
      gMapSizeZ = 20;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 19;
      gMapMaxZ = 19;
      LoadCdFile(CDF_M_KEY05_PRS, 1);
      break;

   case 5:
      gMapDataSizeX = 64;
      gMapDataSizeZ = 2;
      gMapSizeX = 64;
      gMapSizeZ = 2;
      gMapMarginX = 31;
      gMapMarginZ = 0;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 63;
      gMapMaxZ = 1;
      LoadCdFile(CDF_M_KEY06_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[64])gMapDataPtr)[i];
      }
      break;

   case 8:
      gMapDataSizeX = 24;
      gMapDataSizeZ = 24;
      gMapSizeX = 21;
      gMapSizeZ = 21;
      gMapMarginX = 10;
      gMapMarginZ = 10;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 23;
      LoadCdFile(CDF_M_SHOW_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[24])gMapDataPtr)[i];
      }
      break;

   case 9:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 16;
      gMapSizeZ = 16;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 15;
      gMapMaxZ = 15;
      LoadCdFile(CDF_IVENT_PRS, 1);
      break;

   case 10:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 16;
      gMapSizeZ = 16;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 15;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP01_PRS, 1);
      break;

   case 11:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 12;
      gMapSizeX = 24;
      gMapSizeZ = 12;
      gMapMarginX = 11;
      gMapMarginZ = 5;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 11;
      LoadCdFile(CDF_M_MAP02_PRS, 1);
      break;

   case 12:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 18;
      gMapSizeX = 18;
      gMapSizeZ = 18;
      gMapMarginX = 8;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 17;
      gMapMaxZ = 17;
      LoadCdFile(CDF_M_MAP03_PRS, 1);
      break;

   case 13:
   case 65:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 12;
      gMapSizeX = 24;
      gMapSizeZ = 12;
      gMapMarginX = 11;
      gMapMarginZ = 5;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 11;
      LoadCdFile(CDF_M_MAP04_PRS, 1);
      break;

   case 14:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 18;
      gMapSizeX = 18;
      gMapSizeZ = 18;
      gMapMarginX = 8;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 17;
      gMapMaxZ = 17;
      LoadCdFile(CDF_M_MAP05_PRS, 1);
      break;

   case 15:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 21;
      gMapSizeX = 19;
      gMapSizeZ = 18;
      gMapMarginX = 9;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 20;
      gMapMaxZ = 20;
      LoadCdFile(CDF_M_MAP06_PRS, 1);
      break;

   case 16:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 12;
      gMapSizeX = 24;
      gMapSizeZ = 12;
      gMapMarginX = 11;
      gMapMarginZ = 5;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 11;
      LoadCdFile(CDF_M_MAP07_PRS, 1);
      break;

   case 17:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 18;
      gMapSizeX = 19;
      gMapSizeZ = 18;
      gMapMarginX = 8;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 17;
      LoadCdFile(CDF_M_MAP08_PRS, 1);
      break;

   case 18:
   case 66:
      gMapDataSizeX = 14;
      gMapDataSizeZ = 28;
      gMapSizeX = 14;
      gMapSizeZ = 23;
      gMapMarginX = 6;
      gMapMarginZ = 10;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 13;
      gMapMaxZ = 27;
      LoadCdFile(CDF_M_MAP09_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[14])gMapDataPtr)[i];
      }
      break;

   case 19:
      gMapDataSizeX = 32;
      gMapDataSizeZ = 16;
      gMapSizeZ = 16;
      gMapSizeX = 21;
      gMapMarginZ = 7;
      gMapMarginX = 9;
      gMapMinZ = 0;
      gMapMinX = 0;
      gMapMaxX = 31;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP10_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[32])gMapDataPtr)[i];
      }
      break;

   case 20:
      gMapDataSizeX = 15;
      gMapDataSizeZ = 28;
      gMapSizeX = 15;
      gMapSizeZ = 25;
      gMapMarginX = 7;
      gMapMarginZ = 11;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 14;
      gMapMaxZ = 27;
      LoadCdFile(CDF_M_MAP11_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[15])gMapDataPtr)[i];
      }
      break;

   case 21:
   case 67:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 20;
      gMapSizeX = 20;
      gMapSizeZ = 20;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 19;
      gMapMaxZ = 19;
      LoadCdFile(CDF_M_MAP12_PRS, 1);
      break;

   case 22:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 17;
      gMapSizeX = 17;
      gMapSizeZ = 17;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 16;
      gMapMaxZ = 16;
      LoadCdFile(CDF_M_MAP13_PRS, 1);
      break;

   case 23:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 17;
      gMapSizeX = 17;
      gMapSizeZ = 17;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 16;
      gMapMaxZ = 16;
      LoadCdFile(CDF_M_MAP14_PRS, 1);
      break;

   case 24:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 18;
      gMapSizeX = 24;
      gMapSizeZ = 18;
      gMapMarginX = 11;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 17;
      LoadCdFile(CDF_M_MAP15_PRS, 1);
      break;

   case 25:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 20;
      gMapSizeX = 16;
      gMapSizeZ = 20;
      gMapMarginX = 7;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 15;
      gMapMaxZ = 19;
      LoadCdFile(CDF_M_MAP16_PRS, 1);
      break;

   case 26:
      gMapDataSizeX = 48;
      gMapDataSizeZ = 12;
      gMapSizeX = 31;
      gMapSizeZ = 12;
      gMapMarginX = 15;
      gMapMarginZ = 5;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 47;
      gMapMaxZ = 11;
      LoadCdFile(CDF_M_MAP17_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[48])gMapDataPtr)[i];
      }
      break;

   case 27:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 24;
      gMapSizeZ = 16;
      gMapMarginX = 11;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP18_PRS, 1);
      break;

   case 28:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 22;
      gMapSizeX = 17;
      gMapSizeZ = 22;
      gMapMarginX = 8;
      gMapMarginZ = 10;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 16;
      gMapMaxZ = 21;
      LoadCdFile(CDF_M_MAP19_PRS, 1);
      break;

   case 29:
      gMapDataSizeX = 20;
      gMapDataSizeZ = 28;
      gMapSizeX = 20;
      gMapSizeZ = 19;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 19;
      gMapMaxZ = 27;
      LoadCdFile(CDF_M_MAP20_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[20])gMapDataPtr)[i];
      }
      break;

   case 30:
      gMapDataSizeX = 20;
      gMapDataSizeZ = 24;
      gMapSizeX = 20;
      gMapSizeZ = 20;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 19;
      gMapMaxZ = 23;
      LoadCdFile(CDF_M_MAP21_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[20])gMapDataPtr)[i];
      }
      break;

   case 31:
      gMapDataSizeX = 23;
      gMapDataSizeZ = 23;
      gMapSizeX = 19;
      gMapSizeZ = 19;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 22;
      gMapMaxZ = 22;
      LoadCdFile(CDF_M_MAP22_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[23])gMapDataPtr)[i];
      }
      break;

   case 32:
      gMapDataSizeX = 64;
      gMapDataSizeZ = 8;
      gMapSizeX = 27;
      gMapSizeZ = 8;
      gMapMarginX = 13;
      gMapMarginZ = 3;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 63;
      gMapMaxZ = 7;
      LoadCdFile(CDF_M_MAP23_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[64])gMapDataPtr)[i];
      }
      break;

   case 33:
   case 68:
      gMapDataSizeX = 23;
      gMapDataSizeZ = 23;
      gMapSizeX = 16;
      gMapSizeZ = 16;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 22;
      gMapMaxZ = 22;
      LoadCdFile(CDF_M_MAP24_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[23])gMapDataPtr)[i];
      }
      break;

   case 34:
      gMapDataSizeX = 21;
      gMapDataSizeZ = 24;
      gMapSizeX = 16;
      gMapSizeZ = 16;
      gMapMarginX = 7;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 20;
      gMapMaxZ = 23;
      LoadCdFile(CDF_M_MAP25_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[21])gMapDataPtr)[i];
      }
      break;

   case 35:
      gMapDataSizeX = 32;
      gMapDataSizeZ = 16;
      gMapSizeX = 21;
      gMapSizeZ = 16;
      gMapMarginX = 10;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 31;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP26_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[32])gMapDataPtr)[i];
      }
      break;

   case 36:
      gMapDataSizeX = 24;
      gMapDataSizeZ = 24;
      gMapSizeX = 24;
      gMapSizeZ = 24;
      gMapMarginX = 11;
      gMapMarginZ = 11;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 23;
      LoadCdFile(CDF_M_MAP27_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[24])gMapDataPtr)[i];
      }
      break;

   case 37:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 20;
      gMapSizeX = 19;
      gMapSizeZ = 19;
      gMapMarginX = 9;
      gMapMarginZ = 9;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 19;
      LoadCdFile(CDF_M_MAP28_PRS, 1);
      break;

   case 38:
      gMapDataSizeX = 32;
      gMapDataSizeZ = 16;
      gMapSizeX = 21;
      gMapSizeZ = 16;
      gMapMarginX = 9;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 31;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP29_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[32])gMapDataPtr)[i];
      }
      break;

   case 39:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 14;
      gMapSizeX = 28;
      gMapSizeZ = 14;
      gMapMarginX = 13;
      gMapMarginZ = 6;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 27;
      gMapMaxZ = 13;
      LoadCdFile(CDF_M_MAP30_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[28 * 2]) gMapDataPtr)[i];
      }
      break;

   case 40:
      gMapDataSizeX = 44;
      gMapDataSizeZ = 14;
      gMapSizeX = 21;
      gMapSizeZ = 14;
      gMapMarginX = 10;
      gMapMarginZ = 6;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 43;
      gMapMaxZ = 13;
      LoadCdFile(CDF_M_MAP31_PRS, 1);
      for (i = 0; i < gMapDataSizeZ; i++) {
         gMapRowPointers[i] = ((MapTileModel(*)[44])gMapDataPtr)[i];
      }
      break;

   case 41:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 16;
      gMapSizeX = 24;
      gMapSizeZ = 16;
      gMapMarginX = 11;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 23;
      gMapMaxZ = 15;
      LoadCdFile(CDF_M_MAP32_PRS, 1);
      break;

   case 42:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 17;
      gMapSizeX = 21;
      gMapSizeZ = 17;
      gMapMarginX = 10;
      gMapMarginZ = 7;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 25;
      gMapMaxZ = 16;
      LoadCdFile(CDF_M_MAP33_PRS, 1);
      break;

   case 43:
      gMapDataSizeX = 28;
      gMapDataSizeZ = 17;
      gMapSizeX = 21;
      gMapSizeZ = 17;
      gMapMarginX = 10;
      gMapMarginZ = 8;
      gMapMinX = 0;
      gMapMinZ = 0;
      gMapMaxX = 25;
      gMapMaxZ = 16;
      LoadCdFile(CDF_M_MAP34_PRS, 1);
      break;

   case 44:
      LoadCdFile(CDF_M_IVE01_PRS, 1);
      break;

   case 45:
      LoadCdFile(CDF_M_IVE02_PRS, 1);
      break;

   case 46:
      LoadCdFile(CDF_M_IVE03_PRS, 1);
      break;

   case 47:
      LoadCdFile(CDF_M_IVE04_PRS, 1);
      break;

   case 48:
      LoadCdFile(CDF_M_IVE05_PRS, 1);
      break;

   case 49:
      LoadCdFile(CDF_M_IVE06_PRS, 1);
      break;

   case 50:
      LoadCdFile(CDF_M_IVE07_PRS, 1);
      break;

   case 51:
      LoadCdFile(CDF_M_IVE08_PRS, 1);
      break;

   case 52:
      LoadCdFile(CDF_M_IVE09_PRS, 1);
      break;

   case 53:
      LoadCdFile(CDF_M_IVE10_PRS, 1);
      break;

   case 54:
      LoadCdFile(CDF_M_IVE11_PRS, 1);
      break;

   case 55:
      LoadCdFile(CDF_M_IVE12_PRS, 1);
      break;

   case 56:
      LoadCdFile(CDF_M_IVE13_PRS, 1);
      break;

   case 57:
      LoadCdFile(CDF_M_IVE14_PRS, 1);
      break;

   case 58:
      LoadCdFile(CDF_M_IVE15_PRS, 1);
      break;

   case 59:
      LoadCdFile(CDF_M_IVE16_PRS, 1);
      break;

   case 60:
      LoadCdFile(CDF_M_IVE17_PRS, 1);
      break;

   case 61:
      LoadCdFile(CDF_M_IVE18_PRS, 1);
      break;

   case 62:
      LoadCdFile(CDF_M_IVE19_PRS, 1);
      break;

   case 63:
      LoadCdFile(CDF_M_IVE20_PRS, 1);
      break;

   case 64:
      LoadCdFile(CDF_M_IVE21_PRS, 1);
      break;
   }

   ProcessMapFileData(&gScratch1_801317c0[0]);

   if (gState.mapNum == 39) {
      zSize = 28;
   } else {
      zSize = 22;
   }

   for (iz = 0; iz < zSize; iz++) {
      pTiles = (MapTileModel(*)[28])gMapDataPtr;
      for (ix = 0; ix < 28; ix++) {
         i = pTiles[iz][ix].faceCt;
         if (i <= 18) {
            for (iFace = 0; iFace < i; iFace++) {
               if (pTiles[iz][ix].gfx[iFace] != 0) {
                  pTiles[iz][ix].gfx[iFace] += GFX_MAP_TEXTURES_OFS;
               }
            }
         }
      }
   }

   SetupField();
}

void SetupField(void) {
   s32 i;
   s32 j; // + terrain
   s32 iz, ix;
   s32 gfx;
   // s32 terrain;

   gMapUnitsPtr = &gMapUnits[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gMapUnits[i][j].raw = 0;
      }
   }

   gBlueMovementGridPtr = &gPathGrid10[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid10[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid10[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid10[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid10[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid10[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid10[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gPathGrid0_Ptr = &gPathGrid0[1];
   gPathGrid1_Ptr = &gPathGrid1[1];
   gPathGrid2_Ptr = &gPathGrid2[1];

   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid2[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid2[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid2[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid2[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid2[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid2[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gPathGrid3_Ptr = &gPathGrid3[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid3[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid3[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid3[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid3[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid3[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid3[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gPathGrid4_Ptr = &gPathGrid4[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid4[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid4[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid4[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid4[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid4[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid4[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gPathGrid5_Ptr = &gPathGrid5[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid5[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid5[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid5[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid5[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid5[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid5[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gPathGrid6_Ptr = &gPathGrid6[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid6[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid6[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid6[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid6[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid6[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid6[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gTerrainPtr = &gTerrain[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gTerrain[i][j].raw = 0;
      }
   }

   for (j = 0; j < 65; j++) {
      gTerrain[0][j].raw = 0xff7f;
   }
   for (j = 0; j < 65; j++) {
      gTerrain[29][j].raw = 0xff7f;
   }
   for (j = 0; j < 30; j++) {
      gTerrain[j][64].raw = 0xff7f;
   }
   for (j = 0; j < 65; j++) {
      gTerrain[gMapMaxZ + 2][j].raw = 0xff7f;
   }
   for (j = 0; j < 30; j++) {
      gTerrain[j][gMapMaxX + 1].raw = 0xff7f;
   }

   gRedAttackGridPtr = &gPathGrid0[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid0[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid0[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid0[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid0[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid0[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid0[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gYellowTargetGridPtr = &gPathGrid1[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gPathGrid1[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gPathGrid1[0][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid1[29][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid1[j][64] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 65; j++) {
      gPathGrid1[gMapMaxZ + 2][j] = PATH_STEP_INVALID;
   }
   for (j = 0; j < 30; j++) {
      gPathGrid1[j][gMapMaxX + 1] = PATH_STEP_INVALID;
   }

   gTileStateGridPtr = &gTileStateGrid[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gTileStateGrid[i][j].action = TA_NONE;
      }
   }

   gCrateGrid_Ptr = &gCrateGrid[1];
   for (i = 0; i < 30; i++) {
      for (j = 0; j < 65; j++) {
         gCrateGrid[i][j] = PATH_STEP_UNSET;
      }
   }
   for (j = 0; j < 65; j++) {
      gCrateGrid[0][j] = 0x7f;
   }
   for (j = 0; j < 65; j++) {
      gCrateGrid[29][j] = 0x7f;
   }
   for (j = 0; j < 30; j++) {
      gCrateGrid[j][64] = 0x7f;
   }
   for (j = 0; j < 65; j++) {
      gCrateGrid[gMapMaxZ + 2][j] = 0x7f;
   }
   for (j = 0; j < 30; j++) {
      gCrateGrid[j][gMapMaxX + 1] = 0x7f;
   }

   for (iz = 0; iz < gMapDataSizeZ; iz++) {
      for (ix = 0; ix < gMapDataSizeX; ix++) {
         // Terrain type determined by texture
         j = TERRAIN_PLAINS;
         gfx = gMapRowPointers[iz][ix].gfx[0];

         switch (gState.mapNum) {
         case 3:
         case 4:
         case 5:
         case 8:
         case 9:
         case 10:
         case 11:
         case 12:
         case 13:
         case 16:
         case 17:
         case 18:
         case 19:
         case 21:
         case 24:
         case 25:
         case 26:
         case 28:
         case 29:
         case 30:
         case 38:
         case 39:
         case 40:
         case 41:
         case 42:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_5) {
               j = TERRAIN_BARREN;
            }
            if (gfx >= GFX_MAP_TEXTURE_21) {
               j = TERRAIN_VILE_BOG;
            }
            if (gfx >= GFX_MAP_TEXTURE_25) {
               j = TERRAIN_PRAIRIE;
            }
            if (gfx >= GFX_MAP_TEXTURE_51) {
               j = TERRAIN_WATER;
            }
            if (gfx >= GFX_MAP_TEXTURE_54) {
               j = TERRAIN_BOUNDARY;
            }
            if (gfx >= GFX_MAP_TEXTURE_55) {
               j = TERRAIN_THICKET;
            }
            if (gfx >= GFX_MAP_TEXTURE_56) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 15:
         case 34:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_6) {
               j = TERRAIN_WATER;
            }
            if (gfx >= GFX_MAP_TEXTURE_8) {
               j = TERRAIN_PLAINS;
            }
            if (gfx >= GFX_MAP_TEXTURE_212) {
               j = TERRAIN_PRAIRIE;
            }
            if (gfx >= GFX_MAP_TEXTURE_229) {
               j = TERRAIN_THICKET;
            }
            break;

         case 20:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_BOUNDARY;
            }
            if (gfx >= GFX_MAP_TEXTURE_5) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 22:
         case 23:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_WATER;
            }
            if (gfx >= GFX_MAP_TEXTURE_5) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 27:
         case 31:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_5) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 32:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 33:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_5) {
               j = TERRAIN_BARREN;
            }
            if (gfx >= GFX_MAP_TEXTURE_21) {
               j = TERRAIN_VILE_BOG;
            }
            if (gfx >= GFX_MAP_TEXTURE_25) {
               j = TERRAIN_PRAIRIE;
            }
            if (gfx >= GFX_MAP_TEXTURE_51) {
               j = TERRAIN_WATER;
            }
            if (gfx >= GFX_MAP_TEXTURE_54) {
               j = TERRAIN_BOUNDARY;
            }
            if (gfx >= GFX_MAP_TEXTURE_55) {
               j = TERRAIN_THICKET;
            }
            if (gfx >= GFX_MAP_TEXTURE_56) {
               j = TERRAIN_PLAINS;
            }
            if (gfx >= GFX_MAP_TEXTURE_198) {
               j = TERRAIN_LAVA;
            }
            if (gfx == GFX_MAP_TEXTURE_206) {
               j = TERRAIN_PLAINS;
            }
            if (gfx == GFX_MAP_TEXTURE_207) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 2:
         case 35:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_WATER;
            }
            if (gfx >= GFX_MAP_TEXTURE_6) {
               j = TERRAIN_BARREN;
            }
            if (gfx >= GFX_MAP_TEXTURE_62) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 0:
         case 36:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_BARREN;
            }
            if (gfx >= GFX_MAP_TEXTURE_18) {
               j = TERRAIN_THICKET;
            }
            if (gfx >= GFX_MAP_TEXTURE_19) {
               j = TERRAIN_PRAIRIE;
            }
            if (gfx >= GFX_MAP_TEXTURE_28) {
               j = TERRAIN_PLAINS;
            }
            break;

         case 1:
         case 37:
         case 43:
            j = TERRAIN_NO_ENTRY;
            if (gfx >= GFX_MAP_TEXTURE_4) {
               j = TERRAIN_LAVA;
            }
            if (gfx >= GFX_MAP_TEXTURE_12) {
               j = TERRAIN_PLAINS;
            }
            break;
         }

         gTerrainPtr[iz][ix].s.terrain = j;
         UpdateTileElevation(iz, ix);
      }
   }
}

void UpdateElevation(void) {
   s32 iz, ix;

   for (iz = 0; iz < gMapMaxZ + 1; iz++) {
      for (ix = 0; ix < gMapMaxX + 1; ix++) {
         UpdateTileElevation(iz, ix);
      }
   }
}

void UpdateTileElevation(u8 tileZ, u8 tileX) {
   s32 i;
   u8 h;
   u8 highest;

   gTerrainPtr[tileZ][tileX].s.elevation = (-gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4);

   if (gState.mapNum == 14 && gTerrainPtr[tileZ][tileX].s.terrain == TERRAIN_NO_ENTRY) {
      // Adjust for trees in desert battle against Death Ant
      gMapRowPointers[tileZ][tileX].height =
          (-gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4) + 4;
   } else {
      highest = 0;
      for (i = 0; i < 22; i++) {
         h = -gMapRowPointers[tileZ][tileX].vertices[i].vy >> 4;
         if (-gMapRowPointers[tileZ][tileX].vertices[i].vy % 16) {
            h++;
         }
         if (highest < h) {
            highest = h;
         }
         gMapRowPointers[tileZ][tileX].height = highest;
      }
   }
}
