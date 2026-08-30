/* Battle action executors (segment 0x13b94): the attack, spell, chest and poison executors plus
 * the start-of-turn upkeep pass. gSignal3/4 are the "step finished" replies from the sprite and FX
 * objects; gSignal5 is the camera handshake (camera raises 1 in place, executor 99, camera 100). */
#include "common.h"
#include "object.h"
#include "battle.h"
#include "units.h"
#include "state.h"
#include "field.h"
#include "window.h"
#include "audio.h"
#include "graphics.h"

#ifdef PC_FEAT
/* pc_balance.c: the per-chapter Tactical level cap (returns 50 in Normal, so the XP-award guards
 * stay retail-identical until Tactical is on) and the mode flag. */
extern int TacticalCap(int chapter);
extern int gTacticalMode;
#endif
void ClearBattlePortraits(void) {
   Object *p;
   s32 i;

   p = gObjectArray;

   for (i = 0; i < OBJ_DATA_CT; i++) {
      if (p->functionIndex == OBJF_BATTLE_PORTRAIT) {
         p->functionIndex = OBJF_NULL;
         p->d.objf008.sprite->functionIndex = OBJF_NULL;
      }
      p++;
   }
}

#undef OBJF
#define OBJF 015
void Objf015_TargetingAttack(Object *obj) {
   // UnitStatus *attacker, *target; //regs inconsistent
   UnitStatus *unit1, *unit2;
   s32 unitIdx;
   Object *newObj;

   if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != 0) {
      gState.mapCursorOutOfRange = 0;
   } else {
      gState.mapCursorOutOfRange = 1;
   }

   switch (obj->state) {
   case 0:
      ClearGrid(1);
      unit2 = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
      if (unit2->class == CLASS_ARCHER) {
         PopulateRangedAttackGrid(obj->z1.s.hi, obj->x1.s.hi, unit2->attackRange, 0);
      } else {
         PopulateMeleeAttackGrid(obj->z1.s.hi, obj->x1.s.hi, 0, unit2->attackRange);
      }
      obj->state++;
      break;

   case 1:
      if (gSavedPadStateNewPresses & PAD_CIRCLE) {
         if (gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.team == TEAM_CHEST &&
             gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_UNSET) {

            ClearGrid(0);
            gClearSavedPadState = 1;
            gTargetX = gMapCursorX;
            gTargetZ = gMapCursorZ;
            gYellowTargetGridPtr[gMapCursorZ][gMapCursorX] = 1;
            CloseWindow(0x1e);

            unitIdx = OBJ_MAP_UNIT(obj).s.unitIdx;
            unit2 = &gUnits[unitIdx];
            UpdateCompactUnitInfoWindow(unit2, unit2, 0);
            DisplayBasicWindow(0x1d);
            SlideWindowTo(0x1d, 104, 135);
            DisplayBasicWindow(0x1a);
            SlideWindowTo(0x1a, 10, 66);

            newObj = Obj_GetLastUnused();
            newObj->functionIndex = OBJF_BATTLE_PORTRAIT;
            newObj->d.objf008.windowId = 0x41;
            newObj->d.objf008.flipped = 0;
            newObj->d.objf008.portraitId = gUnitPortraitIds[unit2->unitId];

            DrawWindow(0x41, 400, 190, 64, 63, -114, 113, WBS_CROSSED, 0);
            DisplayBasicWindow(0x41);
            SlideWindowTo(0x41, 36, 113);

            obj->state = 4;
            return;
         }

         if (gSavedPadStateNewPresses & PAD_CIRCLE) {
            if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_INVALID &&
                gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != PATH_STEP_UNSET &&
                gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx != 0) {

               gTargetX = gMapCursorX;
               gTargetZ = gMapCursorZ;
               unit2 = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
               unit1 = &gUnits[gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx];

               if (gState.debug || unit2->team != unit1->team) {
                  ClearGrid(0);
                  gYellowTargetGridPtr[gTargetZ][gTargetX] = 1;
                  gClearSavedPadState = 1;
                  UpdateCompactUnitInfoWindow(unit2, unit1, 0);
                  if (unit2->team == TEAM_PLAYER) {
                     DisplayBasicWindow(0x1d);
                  } else {
                     DisplayCustomWindow(0x1d, 0, 1, 0, 0, 23);
                  }
                  CloseWindow(0x1e);

                  unit1 = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
                  unit2 = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];
                  UpdateCompactUnitInfoWindow(unit2, unit1, 1);

                  if (unit2->team == TEAM_PLAYER) {
                     DisplayBasicWindow(0x1e);
                  } else {
                     DisplayCustomWindow(0x1e, 0, 1, 0, 0, 23);
                  }
                  SlideWindowTo(0x1d, 104, 135);
                  SlideWindowTo(0x1e, 10, 66);

                  newObj = Obj_GetLastUnused();
                  newObj->functionIndex = 8;
                  newObj->d.objf008.windowId = 0x41;
                  newObj->d.objf008.flipped = 0;
                  newObj->d.objf008.portraitId = gUnitPortraitIds[unit1->unitId];

                  newObj = Obj_GetLastUnused();
                  newObj->functionIndex = 8;
                  newObj->d.objf008.windowId = 0x42;
                  newObj->d.objf008.flipped = 1;
                  newObj->d.objf008.portraitId = gUnitPortraitIds[unit2->unitId];

                  DrawWindow(0x41, 400, 190, 64, 63, -114, 113, WBS_CROSSED, 0);
                  DrawWindow(0x42, 400, 290, 64, 63, 371, 63, WBS_CROSSED, 0);
                  DisplayBasicWindow(0x41);
                  DisplayCustomWindowWithSetChoice(0x42, 0, 1, 0, 0, 23, 0);
                  SlideWindowTo(0x41, 36, 113);
                  SlideWindowTo(0x42, 221, 63);
                  obj->state += 2;
               }
               return;
            }
         }
      } // PAD_CIRCLE (outer)

      if (gSavedPadStateNewPresses & PAD_X) {
         obj->state++;
         gSignal2 = 1;
      }
      break;

   case 2:
      ClearGrid(0);
      ClearGrid(1);
      obj->functionIndex = OBJF_NULL;
      break;

   case 3:
      if (gPadStateNewPresses & PAD_X) {
         CloseWindow(0x1d);
         CloseWindow(0x1e);
         CloseWindow(0x41);
         CloseWindow(0x42);
         ClearBattlePortraits();

         unitIdx = gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx;
         unit1 = &gUnits[unitIdx];
         UpdateCompactUnitInfoWindow(unit1, unit1, 1);
         if (unit1->team == TEAM_PLAYER) {
            DisplayBasicWindow(0x1e);
         } else {
            DisplayCustomWindow(0x1e, 0, 1, 0, 0, 23);
         }
         gClearSavedPadState = 0;
         obj->state -= 3;
      } else if (gPadStateNewPresses & PAD_CIRCLE) {
         CloseWindow(0x1d);
         CloseWindow(0x1e);
         CloseWindow(0x41);
         CloseWindow(0x42);
         ClearBattlePortraits();
         gSignal2 = 2;
         ClearGrid(1);
         ClearGrid(0);

         newObj = Obj_GetLastUnused();
         newObj->functionIndex = OBJF_UNIT_ATTACKING;
         newObj->x1.n = obj->x1.n;
         newObj->z1.n = obj->z1.n;

         obj->state = 99;
      }
      break;

   case 4:
      /* 宝箱 */
      DrawText(83, 62, 25, 2, 0, "\x95\xf3\x94\xa0");
      obj->state++;

   // fallthrough
   case 5:

      if (gPadStateNewPresses & PAD_CIRCLE) {
         CloseWindow(0x1d);
         CloseWindow(0x1a);
         CloseWindow(0x41);
         ClearBattlePortraits();
         gSignal2 = 2;
         ClearGrid(1);

         newObj = Obj_GetUnused();
         newObj->functionIndex = OBJF_OPENING_CHEST;
         newObj->x1.n = obj->x1.n;
         newObj->z1.n = obj->z1.n;

         obj->state = 99;
      }
      if (gPadStateNewPresses & PAD_X) {
         CloseWindow(0x1d);
         CloseWindow(0x1a);
         CloseWindow(0x41);
         ClearBattlePortraits();
         gClearSavedPadState = 0;
         obj->state = 0;
      }
      break;

   case 99:
      if (gSignal2 == 99) {
         obj->functionIndex = OBJF_NULL;
         return;
      }
      break;
   }
}

Object *s_unitSprite_80123190;
u8 s_gainedLevel_80123194;
u8 s_countering_80123198;

#undef OBJF
#define OBJF 021
void Objf021_UnitAttacking(Object *obj) {
   Object *obj1, *obj2;
   Object *newObj;
   UnitStatus *attacker;
   UnitStatus *defender;
   s16 opposite;
   s16 value1, value2;
   u8 attackResult;
   u8 x, z, team;

   // TODO: This resolves the s3/s4 swap, but should look for a real fix
   do {
      obj1 = OBJ.attackerSprite;
      obj2 = OBJ.defenderSprite;
      attacker = OBJ.attacker;
      defender = OBJ.defender;
   } while (0);

   switch (obj->state) {
   case 0:
      ClearGrid(0);
      OBJ.mapSizeX = gMapSizeX;
      OBJ.mapSizeZ = gMapSizeZ;

      OBJ.attacker = attacker = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
      OBJ.attackerSprite = attacker->sprite;

      OBJ.defender = defender = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];
      OBJ.defenderSprite = defender->sprite;
      //@be8
      obj1 = Obj_GetUnused();
      obj1->functionIndex = OBJF_ATTACK_CAMERA;
      obj1->d.objf017.sprite = obj2 = attacker->sprite;

      if (attacker->class != CLASS_ARCHER) {
         // ?: LO byte of camSavedX doubles as a caller-set arg for specifying melee/ranged
         // LO(obj1->d.objf017.camSavedX) = 1;
         #ifdef PC_PORT
         /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
          * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
          * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
         LO(obj1->d.objf017.camSavedX) = 1;
         #else
         obj1->d.bytes[4] = 1;
         #endif
      }
      if (gTargetX > obj2->x1.s.hi) {
         obj2->d.sprite.direction = ANGLE_WEST;
      }
      if (gTargetX < obj2->x1.s.hi) {
         obj2->d.sprite.direction = ANGLE_EAST;
      }
      if (gTargetZ > obj2->z1.s.hi) {
         obj2->d.sprite.direction = ANGLE_SOUTH;
      }
      if (gTargetZ < obj2->z1.s.hi) {
         obj2->d.sprite.direction = ANGLE_NORTH;
      }
      attacker->direction = obj2->d.sprite.direction;

      if (gIsEnemyTurn) {
         UpdateCompactUnitInfoWindow(attacker, defender, 0);
         if (attacker->team == TEAM_PLAYER) {
            DisplayBasicWindow(0x1d);
         } else {
            DisplayCustomWindow(0x1d, 0, 1, 0, 0, 23);
         }
         UpdateCompactUnitInfoWindow(defender, attacker, 1);
         if (defender->team == TEAM_PLAYER) {
            DisplayBasicWindow(0x1e);
         } else {
            DisplayCustomWindow(0x1e, 0, 1, 0, 0, 23);
         }
      }

      OBJ.timer = 6;
      obj->state++;
      return;

   case 1:
      if (--OBJ.timer == 0) {
         z = obj1->z1.s.hi;
         x = obj1->x1.s.hi;
         team = attacker->team;
         DisplaySupporterBonus(gTargetZ - 1, gTargetX, z, x, team);
         DisplaySupporterBonus(gTargetZ + 1, gTargetX, z, x, team);
         DisplaySupporterBonus(gTargetZ, gTargetX - 1, z, x, team);
         DisplaySupporterBonus(gTargetZ, gTargetX + 1, z, x, team);

         if (gMapSizeX > 16) {
            gMapSizeX = 16;
         }
         if (gMapSizeZ > 16) {
            gMapSizeZ = 16;
         }
      }
      if (gSignal5 == 1) {
         if (gIsEnemyTurn) {
            OBJ.timer = 10;
            obj->state++;
         } else {
            OBJ.timer = 1;
            obj->state += 2;
         }
      }
      return;

   case 2:
      if (--OBJ.timer == 0) {
         CloseWindow(0x1d);
         CloseWindow(0x1e);
         OBJ.timer = 10;
         obj->state++;
      }
      return;

   case 3:
      if (--OBJ.timer == 0) {
         if (attacker->class == CLASS_ARCHER) {
            OBJ_TILE_STATE(obj1).action = TA_RANGED_ATK;
         } else {
            OBJ_TILE_STATE(obj1).action = TA_MELEE_ATK;
         }
         gSignal3 = 0;
         gSignal4 = 0;
         obj->state++;
      }
      return;

   case 4:
      if (gSignal3 != 1) {
         return;
      }

      opposite = obj1->d.sprite.direction + DEG(180);
      if (opposite >= 0x1000) {
         opposite = obj1->d.sprite.direction - DEG(180);
      }
      obj2->d.sprite.direction = opposite;

      value1 = defender->hpFrac / 100;
      if (value1 == 0) {
         value1 = 1;
      }
      value2 = CalculateAttackDamage(attacker, defender);
      defender->hpFrac -= value2;

      value2 = defender->hpFrac / 100;
      if (value2 == 0 && defender->hpFrac != 0) {
         value2 = 1;
      }
      SaveRestoreHpPercents(&value1, &value2, 0);

      value1 = defender->hp;
      CalculateUnitStats(defender);
      value2 = value1 - defender->hp;
      SaveRestoreDamage(&value2, 0);
      SaveRestoreHp(&defender->maxHp, &value1, &defender->hp, 0);

      if (value2 == 0) {
         attackResult = ATK_RES_BLOCKED;
      } else {
         attackResult = ATK_RES_HIT;
      }
      if (defender->hp == 0) {
         attackResult = ATK_RES_DEFEATED;
      }

      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_DISPLAY_DAMAGE;
      newObj->x1.s.hi = gTargetX;
      newObj->z1.s.hi = gTargetZ;

      newObj = Obj_GetUnused();
      newObj->functionIndex = OBJF_BOUNCE_ZOOM;
      gSignal3 = 0;

      if (attackResult == ATK_RES_HIT) {
         gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_HIT;
         obj->state += 1;
      } else if (attackResult == ATK_RES_BLOCKED) {
         gTileStateGridPtr[gTargetZ][gTargetX].action = TA_BLOCK;
         newObj->d.objf024.soft = 1;
         obj->state += 2;
      } else {
         // ATK_RES_DEFEATED:
         if (!HasDefeatSpeech(defender)) {
            gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT;
         } else {
            gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT_MSG;
         }
         obj->state += 3;
      }

      obj->state2 = 0;
      return;

   case 5:
      if (gSignal3 == 1 && gSignal4 == 1) {
         obj->state = 8;
         obj->state2 = 0;
      }
      return;

   case 6:
      if (gSignal3 == 1 && gSignal4 == 1) {
         obj->state = 8;
         obj->state2 = 0;
      }
      return;

   case 8:

      switch (obj->state2) {

      case 0:
         s_countering_80123198 = 0;
         if (defender->class == CLASS_ARCHER) {
            PopulateRangedAttackGrid(obj2->z1.s.hi, obj2->x1.s.hi, defender->attackRange, 0);
         } else {
            PopulateMeleeAttackGrid(obj2->z1.s.hi, obj2->x1.s.hi, 0, defender->attackRange);
         }
         if (gRedAttackGridPtr[obj1->z1.s.hi][obj1->x1.s.hi] != PATH_STEP_UNSET &&
             !defender->paralyzed) {

            s_countering_80123198 = 1;
            gTargetX = obj1->x1.s.hi;
            gTargetZ = obj1->z1.s.hi;

            newObj = Obj_GetUnused();
            newObj->functionIndex = OBJF_FOCUS_CAMERA;
            newObj->d.objf026.target = obj2;
            newObj->d.objf026.type = 3;
            if (attacker->class != CLASS_ARCHER) {
               newObj->d.objf026.wideOcclusionTest = 1;
            }

            OBJ.countering = 1;
            OBJ.timer = 20;
            obj->state2++;
         } else {
            obj->state = 99;
            obj->state2 = 0;
         }
         ClearGrid(0);
         return;

      case 1:
         if (OBJ.timer == 15) {
            z = obj2->z1.s.hi;
            x = obj2->x1.s.hi;
            team = defender->team;
            DisplaySupporterBonus(gTargetZ - 1, gTargetX, z, x, team);
            DisplaySupporterBonus(gTargetZ + 1, gTargetX, z, x, team);
            DisplaySupporterBonus(gTargetZ, gTargetX - 1, z, x, team);
            DisplaySupporterBonus(gTargetZ, gTargetX + 1, z, x, team);
         }
         if (--OBJ.timer == 0) {
            StartUnitSpritesDecoder(obj2->d.sprite.stripIdx);
            OBJ.timer = 5;
            obj->state2++;
         }
         return;

      case 2:
         if (--OBJ.timer != 0) {
            return;
         }
         if (!gDecodingSprites) {
            if (defender->class == CLASS_ARCHER) {
               OBJ_TILE_STATE(obj2).action = TA_RANGED_ATK;
            } else {
               OBJ_TILE_STATE(obj2).action = TA_MELEE_ATK;
            }
            gSignal3 = 0;
            obj->state2++;
         } else {
            OBJ.timer = 1;
         }
         return;

      case 3:
         if (gSignal3 != 1) {
            return;
         }

         value1 = attacker->hpFrac / 100;
         if (value1 == 0) {
            value1 = 1;
         }
         value2 = CalculateAttackDamage(defender, attacker);
         attacker->hpFrac -= value2;

         value2 = attacker->hpFrac / 100;
         if (value2 == 0 && attacker->hpFrac != 0) {
            value2 = 1;
         }
         SaveRestoreHpPercents(&value1, &value2, 0);

         value1 = attacker->hp;
         CalculateUnitStats(attacker);
         value2 = value1 - attacker->hp;
         SaveRestoreDamage(&value2, 0);
         SaveRestoreHp(&attacker->maxHp, &value1, &attacker->hp, 0);

         if (value2 == 0) {
            attackResult = ATK_RES_BLOCKED;
         } else {
            attackResult = ATK_RES_HIT;
         }
         if (attacker->hp == 0) {
            attackResult = ATK_RES_DEFEATED;
         }

         newObj = Obj_GetUnused();
         newObj->functionIndex = OBJF_DISPLAY_DAMAGE;
         newObj->x1.s.hi = gTargetX;
         newObj->z1.s.hi = gTargetZ;

         newObj = Obj_GetUnused();
         newObj->functionIndex = OBJF_BOUNCE_ZOOM;
         gSignal3 = 0;

         if (attackResult == ATK_RES_HIT) {
            gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_HIT;
            obj->state += 1;
         } else if (attackResult == ATK_RES_BLOCKED) {
            gTileStateGridPtr[gTargetZ][gTargetX].action = TA_BLOCK;
            newObj->d.objf024.soft = 1;
            obj->state += 2;
         } else {
            // ATK_RES_DEFEATED:
            if (!HasDefeatSpeech(attacker)) {
               gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT;
            } else {
               gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT_MSG;
            }
            obj->state += 3;
         }
         return;
      } // switch(obj->state2) (via state:8)

      return;

   case 7:
   case 9:
   case 10:
   case 11:
      if (gSignal3 == 1 && gSignal4 == 1) {
         obj->state2 = 0;
         obj->state = 12;
      }
      return;
   case 12:

      switch (obj->state2) {
      case 0:
         OBJ.timer = 15;
         obj->state2++;
      // fallthrough
      case 1:
         if (--OBJ.timer == 0) {
            obj->state = 99;
            obj->state2 = 0;
         }
      }
      return;

   case 99:

      switch (obj->state2) {
      case 0:
         if (gSignal3 != 1) {
            return;
         }
         if (OBJ_MAP_UNIT(obj1).s.unitIdx != 0) {
            OBJ_TILE_STATE(obj1).action = TA_11;
         }
         if (OBJ_MAP_UNIT(obj2).s.unitIdx != 0) {
            OBJ_TILE_STATE(obj2).action = TA_11;
         }
         gSignal5 = 99;
         gMapSizeX = OBJ.mapSizeX;
         gMapSizeZ = OBJ.mapSizeZ;
         obj->state2++;
         return;
      case 1:
         if (gSignal5 != 100) {
            return;
         }
         OBJ.timer = 1;
         s_gainedLevel_80123194 = 0;

         if (s_countering_80123198 && (attacker->team != TEAM_PLAYER) &&
             (defender->team == TEAM_PLAYER)) {
            // Allow player unit to gain EXP when countering
            attacker = defender;
         }
         if (attacker->hpFrac && attacker->team == TEAM_PLAYER) {
            u32 prevExp;
            s32 prevLevel;
            prevExp = attacker->exp;
            SyncGainedHp(attacker, 0);
            prevLevel = attacker->level;
#ifdef PC_FEAT
            if (attacker->level < TacticalCap(gState.chapter)) {   /* GAP 1: per-chapter cap (=50 Normal) */
#else
            if (attacker->level < 50) {
#endif
               BigIntAdd(attacker->experience, gState.experience);
            }
            CalculateUnitStats(attacker);
            if (prevLevel != attacker->level) {
               DetermineMaxMpAndStatVariance(attacker);
               CalculateUnitStats(attacker);
               s_gainedLevel_80123194++;
               s_unitSprite_80123190 = attacker->sprite;
               PopulateUnitSpellList(attacker);
               StartUnitSpritesDecoder(s_unitSprite_80123190->d.sprite.stripIdx);
            }
            SyncGainedHp(attacker, 1);
            prevExp = attacker->exp - prevExp;
            if (prevExp != 0) {
               ShowExpDialog(prevExp, 0x3c);
               OBJ.timer = 50;
            }
         }
         obj->state2++;
         return;
      case 2:
         if ((--OBJ.timer == 0) || (gPadStateNewPresses & PAD_CIRCLE)) {
            CloseWindow(0x3c);
            if (!s_gainedLevel_80123194) {
               obj->state2 = 99;
               return;
            }
            obj->state2++;
         }
         return;
      case 3:
         newObj = Obj_GetUnused();
         newObj->functionIndex = OBJF_LEVEL_UP;
         newObj->d.objf571.sprite = s_unitSprite_80123190;
         obj->state2++;
         return;
      case 4:
         if (gSignal5 == 100) {
            obj->state2 = 99;
         }
         return;
      case 99:
         ClearGrid(0);
         gSignal2 = 99;
         obj->functionIndex = OBJF_NULL;
         return;
      } // switch (obj->state2) (via state:99)
   }
}

#undef OBJF
#define OBJF 567
void Objf567_OpeningChest(Object *obj) {
   Object *obj1, *obj2;
   UnitStatus *opener;
   s32 item;
   s32 unitIdx;

   obj1 = OBJ.openerSprite;
   opener = OBJ.opener;

   switch (obj->state) {
   case 0:
      unitIdx = OBJ_MAP_UNIT(obj).s.unitIdx;
      OBJ.opener = opener = &gUnits[unitIdx];
      OBJ.openerSprite = opener->sprite;
      if (opener->items[0] != ITEM_NULL && opener->items[1] != ITEM_NULL) {
         OBJ.sendToDepot = 1;
      }
      gTileStateGridPtr[gTargetZ][gTargetX].action = TA_CHEST_2;
      obj->state++;
      break;

   case 1:
      item = gTileStateGridPtr[gTargetZ][gTargetX].cachedByte;
      if (item != ITEM_NULL) {
         if (opener->items[0] == ITEM_NULL) {
            opener->items[0] = item;
         } else if (opener->items[1] == ITEM_NULL) {
            opener->items[1] = item;
         } else {
            gState.depot[item]++;
         }
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(36));
      } else {
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(132));
      }

      obj1 = Obj_GetUnused();
      obj1->functionIndex = OBJF_ATTACK_CAMERA;
      obj1->d.objf017.sprite = obj2 = opener->sprite;
      if (opener->class != CLASS_ARCHER) {
         #ifdef PC_PORT
         /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
          * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
          * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
         LO(obj1->d.objf017.camSavedX) = 1;
         #else
         obj1->d.bytes[4] = 1;
         #endif
      }

      if (gTargetX > obj2->x1.s.hi) {
         obj2->d.sprite.direction = ANGLE_WEST;
      }
      if (gTargetX < obj2->x1.s.hi) {
         obj2->d.sprite.direction = ANGLE_EAST;
      }
      if (gTargetZ > obj2->z1.s.hi) {
         obj2->d.sprite.direction = ANGLE_SOUTH;
      }
      if (gTargetZ < obj2->z1.s.hi) {
         obj2->d.sprite.direction = ANGLE_NORTH;
      }
      opener->direction = obj2->d.sprite.direction;
      gTileStateGridPtr[gTargetZ][gTargetX].action = TA_CHEST_3;

      if (opener->direction == ANGLE_SOUTH) {
         gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 1;
      } else if (opener->direction == ANGLE_NORTH) {
         gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 0;
      } else {
         gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 2;
      }

      gTileStateGridPtr[gTargetZ][gTargetX].cachedShort =
          ((opener->direction + DEG(180)) & 0xfff) >> 10;

      obj->state++;
      break;

   case 2:
      if (gSignal5 == 1) {
         obj->state++;
      }
      break;

   case 3:
      if (opener->class == CLASS_ARCHER) {
         OBJ_TILE_STATE(obj1).action = TA_RANGED_ATK;
      } else {
         OBJ_TILE_STATE(obj1).action = TA_MELEE_ATK;
      }
      gSignal3 = 0;
      gSignal4 = 0;
      obj->state++;
      break;

   case 4:
      if (gSignal3 == 1) {
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_BOUNCE_ZOOM;
         gTileStateGridPtr[gTargetZ][gTargetX].action = TA_CHEST_1;
         gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = OBJ.sendToDepot;
         gSignal3 = 0;
         obj->state++;
         obj->state2 = 0;
      }
      break;

   case 5:
      if (gTileStateGridPtr[gTargetZ][gTargetX].action == TA_NONE) {
         obj->state = 99;
         obj->state2 = 0;
      }
      break;

   case 99:

      switch (obj->state2) {
      case 0:
         gSignal5 = 99;
         obj->state2++;
         break;
      case 1:
         if (gSignal5 == 100) {
            gSignal2 = 99;
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 007
void Objf007_ApplyPoison(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.timer = 70;
      obj->state++;
   // fallthrough
   case 1:
      if (--OBJ.timer == 0) {
         OBJ.unit->poisoned = 1;
         obj->functionIndex = OBJF_NULL;
      }
   }
}

u8 *s_pTargetCoordsToGet_8012319c;
u8 s_gainedLevel_801231ac;
u8 s_targetCount_801231b0;

#undef OBJF
#define OBJF 028
void Objf028_UnitCasting(Object *obj) {
   UnitStatus *targetUnit;
   UnitStatus *casterUnit;
   u8 *pSet;
   // obj->x3: targetX
   // obj->z3: targetZ
   s32 iz, ix;
   Object *obj_s1, *obj_a2;
   s16 xdist, zdist;
   s16 value1, value2;
   u8 unitIdx;
   u8 result;

   // TODO PerformAudioCommand definition?
   // TODO Any way around re-use shenanigans?

   switch (obj->state) {
   case 0:
      iz = 0;
      gWindowActiveIdx = 0;
      s_targetCount_801231b0 = 0;

      if (gSpellSounds[gCurrentSpell] != SPELL_NULL) {
         // Prepare XA for main sound (played by Objf014_BattleUnit)
         PerformAudioCommand(gSpellSounds[gCurrentSpell] + 0x1000);
      }

      casterUnit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
      casterUnit->mp -= gSpells[gCurrentSpell].mpCost;
      //@23ec
      do {
         gState.experience[iz] = 0;
         iz++;
      } while (iz < 8);
      // for (iz = 0; iz < 8; iz++)
      //   gState.experience[iz] = 0;

      OBJ.mapSizeX = gMapSizeX;
      OBJ.mapSizeZ = gMapSizeZ;
      pSet = gTargetCoords;
      obj->state2 = gSpells[gCurrentSpell].targeting;
      obj->state++;
   // fallthrough
   case 1:

      switch (obj->state2) {

      case 0:
      case 1:
      case 2:
      case 6:
         *pSet++ = gTargetX;
         *pSet++ = gTargetZ;
         obj->state++;
         break;

      case 3:
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].fieldSize, 0);
         } else {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);
         }
         //@258c

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0 &&
                   gMapUnitsPtr[iz][ix].s.team != OBJ_MAP_UNIT(obj).s.team) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;

      case 4:
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].fieldSize, 0);
         } else {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);
         }

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0 &&
                   gMapUnitsPtr[iz][ix].s.team == OBJ_MAP_UNIT(obj).s.team) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;

      case 5:
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].fieldSize, 0);
         } else {
            PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);
         }

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;

      case 7:
         PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0 &&
                   gMapUnitsPtr[iz][ix].s.team != OBJ_MAP_UNIT(obj).s.team) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;

      case 8:
         PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0 &&
                   gMapUnitsPtr[iz][ix].s.team == OBJ_MAP_UNIT(obj).s.team) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;

      case 9:
         PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);

         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != PATH_STEP_UNSET &&
                   gMapUnitsPtr[iz][ix].s.unitIdx != 0) {

                  *pSet++ = ix;
                  *pSet++ = iz;
               }
            }
         }
         obj->state++;
         break;
      } // switch (obj->state2) (via state:1)

   // fallthrough
   case 2:
      ClearGrid(0);
      *pSet++ = 0xff;
      obj->state++;

   // fallthrough
   case 3:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_ATTACK_CAMERA;
      #ifdef PC_PORT
      /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
       * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
       * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
      LO(obj_s1->d.objf017.camSavedX) = 0;
      #else
      obj_s1->d.bytes[4] = 0;
      #endif

      obj_a2 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj_s1->d.objf017.sprite = obj_a2;

      xdist = gTargetX - obj_a2->x1.s.hi;
      if (xdist < 0) {
         xdist = -xdist;
      }
      zdist = gTargetZ - obj_a2->z1.s.hi;
      if (zdist < 0) {
         zdist = -zdist;
      }
      if (xdist != 0 || zdist != 0) {
         if (xdist >= zdist) {
            if (gTargetX > obj_a2->x1.s.hi) {
               obj_a2->d.sprite.direction = ANGLE_WEST;
            }
            if (gTargetX < obj_a2->x1.s.hi) {
               obj_a2->d.sprite.direction = ANGLE_EAST;
            }
         } else {
            if (gTargetZ > obj_a2->z1.s.hi) {
               obj_a2->d.sprite.direction = ANGLE_SOUTH;
            }
            if (gTargetZ < obj_a2->z1.s.hi) {
               obj_a2->d.sprite.direction = ANGLE_NORTH;
            }
         }
         casterUnit->direction = obj_a2->d.sprite.direction;
      }
      OBJ.timer = 4;
      obj->state++;
      break;

   case 4:
      if (--OBJ.timer == 0) {
         if (gMapSizeX > 16) {
            gMapSizeX = 16;
         }
         if (gMapSizeZ > 16) {
            gMapSizeZ = 16;
         }
      }
      if (gSignal5 != 1) {
         return;
      }
      obj_a2 = Obj_GetUnused();
      obj_a2->functionIndex = OBJF_ITEM_SPELL;
      obj_a2->x1.s.hi = obj->x1.s.hi;
      obj_a2->z1.s.hi = obj->z1.s.hi;

      DrawWindow(0x38, 0, 100, 144, 36, 172, 200, WBS_CROSSED, 0);
      DrawText(12, 111, 20, 0, 0, gSpellNames[gCurrentSpell]);

      unitIdx = OBJ_MAP_UNIT(obj).s.unitIdx;
      casterUnit = &gUnits[unitIdx];
      if (casterUnit->team == TEAM_PLAYER) {
         DisplayBasicWindow(0x38);
      } else {
         DisplayCustomWindow(0x38, 0, 1, 0, 0, 23);
      }
      //@310c

      iz = gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT];
      if (iz == SPELL_EFFECT_DAMAGE || iz == SPELL_EFFECT_POISON || iz == SPELL_EFFECT_PARALYZE) {
         // Harmful spell - allies should "take cover"
         gState.shielding = 1;
         for (iz = 0; iz < gMapMaxZ; iz++) {
            for (ix = 0; ix < gMapMaxX; ix++) {
               u8 unitIdxAtLocation = gMapUnitsPtr[iz][ix].s.unitIdx;
               if (unitIdxAtLocation != 0 &&
                   gMapUnitsPtr[iz][ix].s.team == casterUnit->team &&
                   unitIdx != unitIdxAtLocation) {
                  gTileStateGridPtr[iz][ix].action = TA_31;
               }
            }
         }
      }

      OBJ_TILE_STATE(obj).action = TA_CAST;
      gSignal3 = 0;
      obj->state++;
      break;

   case 5:
      if (gSignal3 != 2) {
         return;
      }
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_MAIN];
      obj_s1->x1.s.hi = obj->x1.s.hi;
      obj_s1->z1.s.hi = obj->z1.s.hi;
      gSignal3 = 0;
      obj->state++;
      break;

   case 6:
      if (gSignal3 != 1) {
         return;
      }
      obj->state++;
      break;

   case 7:
      s_pTargetCoordsToGet_8012319c = gTargetCoords;
      obj->state++;

   // fallthrough
   case 8:
      //@331c
      if (*s_pTargetCoordsToGet_8012319c != 0xff) {
         s32 bx, bz;
         bx = obj->x3.s.hi = *s_pTargetCoordsToGet_8012319c++;
         bz = obj->z3.s.hi = *s_pTargetCoordsToGet_8012319c++;

         obj_s1 = GetUnitSpriteAtPosition(bz, bx);
         obj_a2 = Obj_GetUnused();
         obj_a2->functionIndex = OBJF_FOCUS_CAMERA;
         obj_a2->d.objf026.target = obj_s1;
         obj_a2->d.objf026.type = 1;
         OBJ.timer = 10;
         obj->state2 = 0;
         obj->state++;
         break;
      } else {
         obj->state = 99;
         return;
      }
      break;

   case 9:

      switch (obj->state2) {
      case 0:
         if (OBJ.timer == 4 && s_targetCount_801231b0 != 0) {
            // Play XA for hit sound
            PerformAudioCommand(gSpellSounds2[gCurrentSpell]);
         }
         if (--OBJ.timer != 0) {
            return;
         }
         casterUnit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
         targetUnit = &gUnits[OBJ_TARGET_MAP_UNIT(obj).s.unitIdx];

         switch (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT]) {
         case SPELL_EFFECT_DAMAGE:
            value1 = targetUnit->hpFrac / 100;
            if (value1 == 0) {
               value1 = 1;
            }
            value2 = CalculateSpellPowerAndExp(casterUnit, targetUnit);
            targetUnit->hpFrac -= value2;

            value2 = targetUnit->hpFrac / 100;
            if (value2 == 0 && targetUnit->hpFrac != 0) {
               value2 = 1;
            }
            //@35e8
            SaveRestoreHpPercents(&value1, &value2, 0);

            value1 = targetUnit->hp;
            CalculateUnitStats(targetUnit);
            value2 = value1 - targetUnit->hp;
            SaveRestoreDamage(&value2, 0);
            SaveRestoreHp(&targetUnit->maxHp, &value1, &targetUnit->hp, 0);
            //@364c
            if (targetUnit->hp == 0) {
               result = ATK_RES_DEFEATED;
            } else {
               result = ATK_RES_HIT;
            }
            obj_s1 = Obj_GetUnused();
            obj_s1->x1.s.hi = obj->x3.s.hi;
            obj_s1->z1.s.hi = obj->z3.s.hi;
            gSignal4 = 1;

            switch (result) {
            case ATK_RES_HIT:
               OBJ_TARGET_TILE_STATE(obj).action = TA_MAG_HIT;
               obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_TARGET];
               break;
            case ATK_RES_DEFEATED:
               if (!HasDefeatSpeech(targetUnit)) {
                  obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_DEFEAT];
                  OBJ_TARGET_TILE_STATE(obj).action = TA_MAG_DEFEAT;
               } else {
                  OBJ_TARGET_TILE_STATE(obj).action = TA_MAG_DEFEAT_MSG;
                  obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_TARGET];
                  gSignal4 = 0;
               }
               break;
            }

            break;

         case SPELL_EFFECT_RESTORE_HP:
         case SPELL_EFFECT_RESTORE_HP_UNK:
            if (gCurrentSpell == SPELL_LIFE_ORB) {
               targetUnit->mp = targetUnit->maxMp;
            }
            obj_s1 = Obj_GetUnused();
            obj_s1->x1.s.hi = obj->x3.s.hi;
            obj_s1->z1.s.hi = obj->z3.s.hi;
            obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_TARGET];
            value1 = targetUnit->hpFrac / 100;
            value2 = CalculateSpellPowerAndExp(casterUnit, targetUnit);
            targetUnit->hpFrac += value2;

            value2 = targetUnit->hpFrac / 100;
            SaveRestoreHpPercents(&value1, &value2, 0);

            value1 = targetUnit->hp;
            CalculateUnitStats(targetUnit);
            value2 = targetUnit->hp - value1;
            SaveRestoreDamage(&value2, 0);
            SaveRestoreHp(&targetUnit->maxHp, &value1, &targetUnit->hp, 0);
            gSignal4 = 1;
            break;

         case SPELL_EFFECT_POISON:
         case SPELL_EFFECT_PARALYZE:
            if (targetUnit->poisoned != 0) {
               targetUnit->poisoned = 1;
            }
            TryInflictingAilment(casterUnit, targetUnit,
                                 gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] ==
                                     SPELL_EFFECT_PARALYZE);

            if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_POISON) {
               ix = 1;
               switch (targetUnit->poisoned) {
               case 1:
                  targetUnit->poisoned = 0;
                  obj_a2 = Obj_GetUnused();
                  obj_a2->functionIndex = OBJF_APPLY_POISON;
                  obj_a2->d.objf007.unit = targetUnit;
                  break;
               case 0:
                  ix = 2;
                  break;
               }
            } else {
               // SPELL_EFFECT_PARALYZE:
               if (targetUnit->paralyzed != 0) {
                  targetUnit->paralyzed = 1;
                  ix = 1;
               } else {
                  ix = 2;
               }
            }
            //@3a58
            obj_s1 = Obj_GetUnused();
            obj_s1->x1.s.hi = obj->x3.s.hi;
            obj_s1->z1.s.hi = obj->z3.s.hi;
            obj_s1->functionIndex = gSpellsEx[gCurrentSpell][ix];
            gSignal4 = 1;
            break;

         case SPELL_EFFECT_BOOST_ATK:
            if (!targetUnit->atkBoosted) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->atkBoosted = 1;
            break;

         case SPELL_EFFECT_BOOST_DEF:
            if (!targetUnit->defBoosted) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->defBoosted = 1;
            break;

         case SPELL_EFFECT_BOOST_ATK_AND_DEF:
            if (!targetUnit->atkBoosted || !targetUnit->defBoosted) {
               CalculateSupportSpellExp(targetUnit);
            }
#ifdef PC_FEAT
            /* Tactical MYSTIC_ENERGY is a defensive-only group buff: defBoosted 3 (retail 1) plus
             * magSusc=1, and no atk boost. Keyed on gCurrentSpell so any other BOOST_ATK_AND_DEF spell
             * keeps retail behaviour. See docs/tactical-mode.md, "Monk & Ninja". */
            if (gTacticalMode && gCurrentSpell == SPELL_MYSTIC_ENERGY) {
               targetUnit->defBoosted = 3;
               targetUnit->magicSusceptibility = 1;
            } else {
               targetUnit->atkBoosted = 1;
               targetUnit->defBoosted = 1;
            }
#else
            targetUnit->atkBoosted = 1;
            targetUnit->defBoosted = 1;
#endif
            break;

         case SPELL_EFFECT_CURE_AILMENTS:
            if (targetUnit->poisoned || targetUnit->paralyzed) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->poisoned = 0;
            targetUnit->paralyzed = 0;
            break;

         case SPELL_EFFECT_BOOST_AGL:
            if (!targetUnit->aglBoosted) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->aglBoosted = 1;
#ifdef PC_FEAT
            /* Tactical PERFECT_GUARD also grants magic resistance (magSusc=1) to its single target.
             * Keyed on gCurrentSpell so any other BOOST_AGL spell keeps retail behaviour; the team-turn
             * clear loop that restores magSusc from gUnitInfo covers the reset. */
            if (gTacticalMode && gCurrentSpell == SPELL_PERFECT_GUARD) {
               targetUnit->magicSusceptibility = 1;
            }
#endif
            break;

         case SPELL_EFFECT_RESTORE_MP:
            if (targetUnit->maxMp > targetUnit->mp) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->mp += gSpellsEx[gCurrentSpell][SPELL_EX_MP_BONUS];
            if (targetUnit->maxMp < targetUnit->mp) {
               targetUnit->mp = targetUnit->maxMp;
            }
            break;

         case SPELL_EFFECT_TRANSFER_MP:
            if (casterUnit != targetUnit &&
                targetUnit->maxMp > targetUnit->mp) {
               CalculateSupportSpellExp(targetUnit);
            }
            targetUnit->mp += gSpellsEx[gCurrentSpell][SPELL_EX_MP_BONUS];
            if (targetUnit->maxMp < targetUnit->mp) {
               targetUnit->mp = targetUnit->maxMp;
            }
            break;

         } // switch (effect) (via state:9->state2:0)

         if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] >= SPELL_EFFECT_SUPPORT_OFS) {
            obj_s1 = Obj_GetUnused();
            obj_s1->x1.s.hi = obj->x3.s.hi;
            obj_s1->z1.s.hi = obj->z3.s.hi;
            obj_s1->functionIndex = gSpellsEx[gCurrentSpell][SPELL_EX_OBJF_TARGET];
            gSignal4 = 1;
         }
         gSignal3 = 0;
         obj->state2++;
         return;

      case 1:
         if (gSignal3 == 0 || gSignal4 == 0) {
            return;
         }
         OBJ.timer = 7;
         obj->state2++;
         return;
      case 2:
         if (--OBJ.timer == 0) {
            if (gSpellSounds2[gCurrentSpell] != 0) {
               // Prepare XA for hit sound
               PerformAudioCommand(gSpellSounds2[gCurrentSpell] + 0x1000);
            }
            s_targetCount_801231b0++;
            obj->state--;
         }
         return;
      } // switch (obj->state2) (via state:9)
      return;

   case 99:
      gState.shielding = 0;
      CloseWindow(0x38);
      gMapSizeX = OBJ.mapSizeX;
      gMapSizeZ = OBJ.mapSizeZ;
      gSignal5 = 99;
      obj->state++;
      obj->state2 = 0;
      return;

   case 100:

      switch (obj->state2) {
      case 0:
         if (gSignal5 != 100) {
            return;
         }
         OBJ.timer = 1;
         s_gainedLevel_801231ac = 0;
         casterUnit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];

         if (casterUnit->team == TEAM_PLAYER) {
            u32 prevExp;
            s32 prevLevel;
            prevExp = casterUnit->exp;
            SyncGainedHp(casterUnit, 0);
            prevLevel = casterUnit->level;
#ifdef PC_FEAT
            if (casterUnit->level < TacticalCap(gState.chapter)) {  /* GAP 1: per-chapter cap */
#else
            if (casterUnit->level < 50) {
#endif
               BigIntAdd(casterUnit->experience, gState.experience);
            }
            CalculateUnitStats(casterUnit);
            if (prevLevel != casterUnit->level) {
               DetermineMaxMpAndStatVariance(casterUnit);
               CalculateUnitStats(casterUnit);
               s_gainedLevel_801231ac++;
               PopulateUnitSpellList(casterUnit);
               StartUnitSpritesDecoder(casterUnit->sprite->d.sprite.stripIdx);
            }
            SyncGainedHp(casterUnit, 1);
            prevExp = casterUnit->exp - prevExp;
            if (prevExp != 0) {
               ShowExpDialog(prevExp, 0x3c);
               OBJ.timer = 50;
            }
         }
         obj->state2++;
         return;

      case 1:
         if ((--OBJ.timer == 0) || (gPadStateNewPresses & PAD_CIRCLE)) {
            CloseWindow(0x3c);
            if (!s_gainedLevel_801231ac) {
               obj->state2 = 99;
               return;
            }
            obj_a2 = Obj_GetUnused();
            obj_a2->functionIndex = OBJF_LEVEL_UP;
            casterUnit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
            obj_a2->d.objf571.sprite = casterUnit->sprite;
            obj->state2++;
         }
         return;

      case 2:
         if (gSignal5 == 100) {
            obj->state2 = 99;
         }
         return;

      case 99:
         gSignal2 = 99;
         obj->functionIndex = OBJF_NULL;
         return;

      } // switch (obj->state2) (via state:100)
   }
}

u8 *s_pTargetCoordsToGet_801231b4;
u8 s_unitTypeToSpawn_801231b8;

#undef OBJF
#define OBJF 592
void Objf592_BattleTurnStart(Object *obj) {
   UnitStatus *unit;
   u8 *pSet;
   s32 i, ct;
   Object *obj_s1, *obj_v1;
   s16 value1, value2;

   const u8 antArmSpawnCoords[8] = {0, 0, 0, 17, 17, 0, 17, 17};
   const u8 sparkieSpawnCoords[8] = {0, 0, 0, 16, 16, 0, 16, 16};

   switch (obj->state) {
   case 0:
      // Clear buffs; gather targets atop healing circles
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(141));
      gSignal2 = 0;
      unit = &gUnits[1];

      for (i = 1; i < UNIT_CT; i++, unit++) {
         if (unit->idx != 0 && unit->team == OBJ.team) {
            unit->atkBoosted = 0;
            unit->defBoosted = 0;
#ifdef PC_FEAT
            /* MYSTIC_ENERGY / PERFECT_GUARD set magSusc=1 for a turn; restore each team unit's real
             * value (in Tactical the rebalanced gUnitInfo value) as its buffs clear at turn start. */
            if (gTacticalMode) {
               unit->magicSusceptibility = gUnitInfo[unit->unitId].magicSusceptibility;
            }
#endif
         }
      }
      pSet = gTargetCoords;
      unit = &gUnits[1];
      ct = 0;

      for (i = 1; i < UNIT_CT; i++, unit++) {
         obj_s1 = unit->sprite;
         if (unit->idx != 0 && unit->team == OBJ.team &&
             gTerrainPtr[obj_s1->z1.s.hi][obj_s1->x1.s.hi].s.terrain == TERRAIN_BOUNDARY) {
            *pSet++ = obj_s1->z1.s.hi;
            *pSet++ = obj_s1->x1.s.hi;
            ct++;
         }
      }

      if (ct == 0) {
         // No suitable targets: advance state to paralysis check
         obj->state += 3;
         return;
      }

      // Finish gathering healing circle targets
      *pSet++ = 0xff;

      s_pTargetCoordsToGet_801231b4 = gTargetCoords;
      obj->state++;

   // fallthrough
   case 1:
      if (*s_pTargetCoordsToGet_801231b4 != 0xff) {
         // Initiate next healing circle target
         gTargetZ = *s_pTargetCoordsToGet_801231b4++;
         gTargetX = *s_pTargetCoordsToGet_801231b4++;

         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);

         if (OBJ.targets == 0) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_ATTACK_CAMERA;
            #ifdef PC_PORT
            /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
             * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
             * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
            LO(obj_v1->d.objf017.camSavedX) = 0;
            #else
            obj_v1->d.bytes[4] = 0;
            #endif
            obj_v1->d.objf017.sprite = obj_s1;
         } else {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
            obj_v1->d.objf026.target = obj_s1;
            obj_v1->d.objf026.type = 1;
         }
         OBJ.targets++;
         OBJ.timer = 20;
         obj->state2 = 0;
         obj->state++;
      } else {
         // Finished processing healing circle units
         obj->state = 3;
      }
      return;

   case 2:
      // Healing circle
      switch (obj->state2) {
      case 0:
         if (--OBJ.timer != 0) {
            return;
         }
         //@443c
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(141));
         unit = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];
         unit->mp += unit->maxMp / 5;
         if (unit->mp > unit->maxMp) {
            unit->mp = unit->maxMp;
         }
         value1 = unit->hpFrac / 100;
         if (value1 == 0) {
            value1 = 1;
         }
         value2 = 3000;
         unit->hpFrac += value2;
         if (unit->hpFrac > 10000) {
            unit->hpFrac = 10000;
         }
         value2 = unit->hpFrac / 100;
         if (value2 == 0 && unit->hpFrac != 0) {
            value2 = 1;
         }
         SaveRestoreHpPercents(&value1, &value2, 0);
         value1 = unit->hp;
         CalculateUnitStats(unit);
         value2 = unit->hp - value1;
         SaveRestoreDamage(&value2, 0);
         SaveRestoreHp(&unit->maxHp, &value1, &unit->hp, 0);

         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_HEALING_FX2;
         obj_v1->x1.s.hi = gTargetX;
         obj_v1->z1.s.hi = gTargetZ;

         gSignal3 = 0;
         obj->state2++;
         return;
      case 1:
         if (gSignal3 == 0) {
            return;
         }
         OBJ.timer = 10;
         obj->state2++;
         return;
      case 2:
         if (--OBJ.timer != 0) {
            return;
         }
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(141));
         obj->state--;
         return;
      } // switch (obj->state2) (via state:2)

      return;

   case 3:
      // Gather targets for paralysis removal
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(135));
      pSet = gTargetCoords;
      unit = &gUnits[1];

      ct = 0;
      for (i = 1; i < UNIT_CT; i++, unit++) {
         obj_s1 = unit->sprite;
         if (unit->idx != 0 && unit->team == OBJ.team &&
             unit->paralyzed) {
            if ((rand() & 0x7fff) < (unit->paralyzed - 1) * 0x1999) {
               unit->paralyzed = 0;
               *pSet++ = obj_s1->z1.s.hi;
               *pSet++ = obj_s1->x1.s.hi;
               ct++;
            } else {
               // Increase odds for next time
               unit->paralyzed++;
            }
         }
      }
      if (ct == 0) {
         // No suitable targets: advance state to poison check
         obj->state += 3;
         return;
      }

      *pSet++ = 0xff;
      s_pTargetCoordsToGet_801231b4 = gTargetCoords;
      obj->state++;

   // fallthrough
   case 4:
      if (*s_pTargetCoordsToGet_801231b4 != 0xff) {
         // Initiate next paralysis removal
         gTargetZ = *s_pTargetCoordsToGet_801231b4++;
         gTargetX = *s_pTargetCoordsToGet_801231b4++;

         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);

         if (OBJ.targets == 0) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_ATTACK_CAMERA;
            #ifdef PC_PORT
            /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
             * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
             * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
            LO(obj_v1->d.objf017.camSavedX) = 0;
            #else
            obj_v1->d.bytes[4] = 0;
            #endif
            obj_v1->d.objf017.sprite = obj_s1;
         } else {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
            obj_v1->d.objf026.target = obj_s1;
            obj_v1->d.objf026.type = 1;
         }
         OBJ.targets++;
         StartUnitSpritesDecoder(obj_s1->d.sprite.stripIdx);
         OBJ.timer = 20;
         obj->state2 = 0;
         obj->state++;
      } else {
         // Finished processing paralysis removals
         obj->state = 6;
      }
      return;

   case 5:
      // Paralysis removal
      switch (obj->state2) {
      case 0:
         if (--OBJ.timer != 0) {
            return;
         }
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(135));
         unit = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];

         gTileStateGridPtr[gTargetZ][gTargetX].action = TA_30;
         if (unit->team == TEAM_PLAYER) {
            gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 22;
         } else {
            gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 12;
         }

         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_REMOVE_PARALYSIS;
         obj_v1->x1.n = obj_s1->x1.n;
         obj_v1->z1.n = obj_s1->z1.n;
         obj_v1->state2 = 50;
         OBJ.timer = 60;
         obj->state2++;
         return;
      case 1:
         if (--OBJ.timer != 0) {
            return;
         }
         OBJ.timer = 10;
         obj->state2++;
         return;
      case 2:
         if (--OBJ.timer != 0) {
            return;
         }
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(135));
         obj->state--;
         return;
      } // switch (obj->state2) (via state:5)

      return;

   case 6:
      // Gather targets for poison damage
      pSet = gTargetCoords;
      unit = &gUnits[1];

      ct = 0;
      for (i = 1; i < UNIT_CT; i++, unit++) {
         if (unit->idx != 0 && unit->team == OBJ.team) {
            obj_s1 = unit->sprite;
            if (unit->poisoned) {
               *pSet++ = obj_s1->z1.s.hi;
               *pSet++ = obj_s1->x1.s.hi;
               ct++;
            }
         }
      }
      if (ct == 0 && OBJ.targets == 0) {
         obj->state += 3;
         return;
      }
      *pSet++ = 0xff;
      s_pTargetCoordsToGet_801231b4 = gTargetCoords;
      obj->state++;

   // fallthrough
   case 7:
      if (*s_pTargetCoordsToGet_801231b4 != 0xff) {
         // Initiate next poison damage
         gTargetZ = *s_pTargetCoordsToGet_801231b4++;
         gTargetX = *s_pTargetCoordsToGet_801231b4++;

         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);

         if (OBJ.targets == 0) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_ATTACK_CAMERA;
            #ifdef PC_PORT
            /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
             * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
             * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
            LO(obj_v1->d.objf017.camSavedX) = 0;
            #else
            obj_v1->d.bytes[4] = 0;
            #endif
            obj_v1->d.objf017.sprite = obj_s1;
         } else {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
            obj_v1->d.objf026.target = obj_s1;
            obj_v1->d.objf026.type = 1;
         }
         OBJ.targets++;
         OBJ.timer = 20;
         obj->state2 = 0;
         obj->state++;
      } else {
         // Finished processing poison damages
         obj->state = 9;
      }
      return;

   case 8:
      // Poison damage
      switch (obj->state2) {
      case 0:
         if (--OBJ.timer != 0) {
            return;
         }
         unit = &gUnits[gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx];

         value1 = unit->hpFrac / 100;
         if (value1 == 0) {
            value1 = 1;
         }
         value2 = 2500;
         unit->hpFrac -= value2;
         if (unit->hpFrac < 0) {
            unit->hpFrac = 0;
         }
         value2 = unit->hpFrac / 100;
         if (value2 == 0 && unit->hpFrac != 0) {
            value2 = 1;
         }
         SaveRestoreHpPercents(&value1, &value2, 0);
         value1 = unit->hp;
         CalculateUnitStats(unit);
         value2 = value1 - unit->hp;
         SaveRestoreDamage(&value2, 0);
         SaveRestoreHp(&unit->maxHp, &value1, &unit->hp, 0);

         if (unit->hp != 0) {
            gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_HIT;
         } else {
            if (!HasDefeatSpeech(unit)) {
               gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT;
            } else {
               gTileStateGridPtr[gTargetZ][gTargetX].action = TA_PHYS_DEFEAT_MSG;
            }
         }
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DISPLAY_DAMAGE;
         obj_v1->x1.s.hi = gTargetX;
         obj_v1->z1.s.hi = gTargetZ;

         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_BOUNCE_ZOOM;

         gSignal3 = 0;
         obj->state2++;
         return;
      case 1:
         if (gSignal3 == 0) {
            return;
         }
         OBJ.timer = 10;
         obj->state2++;
         return;
      case 2:
         if (--OBJ.timer != 0) {
            return;
         }
         obj->state--;
         return;

      } // switch (obj->state2) (via state:8)
      return;

   case 9:
      // Enemy respawns
      if (OBJ.team == TEAM_PLAYER) {
         obj->state = 99;
         return;
      }
      if (gState.mapNum == 14) {
         s_unitTypeToSpawn_801231b8 = UNIT_TYPE_ANT_ARM;
         s_pTargetCoordsToGet_801231b4 = antArmSpawnCoords;
      } else if (gState.mapNum == 23) {
         s_unitTypeToSpawn_801231b8 = UNIT_TYPE_SPARKIE;
         s_pTargetCoordsToGet_801231b4 = sparkieSpawnCoords;
      } else {
         obj->state = 99;
         return;
      }
      if (CountUnitsOfType(s_unitTypeToSpawn_801231b8) > 5) {
         obj->state = 99;
         return;
      }
      obj->state++;

   // fallthrough
   case 10:
      if (s_unitTypeToSpawn_801231b8 == UNIT_TYPE_ANT_ARM) {
         s_pTargetCoordsToGet_801231b4 = antArmSpawnCoords;
      } else {
         s_pTargetCoordsToGet_801231b4 = sparkieSpawnCoords;
      }
      ct = 0;
      for (i = 0; i < 4; i++) {
         if (gMapUnitsPtr[s_pTargetCoordsToGet_801231b4[i * 2]]
                         [s_pTargetCoordsToGet_801231b4[i * 2 + 1]]
                             .s.unitIdx == 0) {
            ct++;
         }
      }
      if (ct == 0) {
         obj->state = 99;
         return;
      }

      i = rand() % 4;
      while (1) {
         gTargetZ = s_pTargetCoordsToGet_801231b4[i * 2];
         gTargetX = s_pTargetCoordsToGet_801231b4[i * 2 + 1];
         i++;
         if (gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx == 0) {
            break;
         }
         if (i == 4) {
            i = 0;
         }
      }
      gTileStateGridPtr[gTargetZ][gTargetX].action = TA_25;
      if (s_unitTypeToSpawn_801231b8 == UNIT_TYPE_ANT_ARM) {
         SetupBattleUnit(7, gTargetZ, gTargetX, 1, TEAM_ENEMY, DIR_SOUTH, 99, 0, 99);
      } else {
         SetupBattleUnit(1, gTargetZ, gTargetX, 10, TEAM_ENEMY, DIR_SOUTH, 99, 10, 99);
      }
      obj->state++;
      return;

   case 11:
      obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
      StartUnitSpritesDecoder(obj_s1->d.sprite.stripIdx);

      if (OBJ.targets == 0) {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_ATTACK_CAMERA;
         #ifdef PC_PORT
         /* `d.bytes[4]` is a raw byte offset into the Object union assuming a 4-byte `sprite`
          * pointer; on LP64 it lands inside the pointer (docs/width-bugs.md). Write the meant field
          * instead: the low byte of camSavedX, the melee/ranged flag Objf017_AttackCamera reads. */
         LO(obj_v1->d.objf017.camSavedX) = 0;
         #else
         obj_v1->d.bytes[4] = 0;
         #endif
         obj_v1->d.objf017.sprite = obj_s1;
      } else {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
         obj_v1->d.objf026.target = obj_s1;
         obj_v1->d.objf026.type = 1;
      }
      OBJ.targets++;
      OBJ.timer = 20;
      obj->state++;
      return;

   case 12:
      if (--OBJ.timer != 0) {
         return;
      }
      if (s_unitTypeToSpawn_801231b8 == UNIT_TYPE_ANT_ARM) {
         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
         gTileStateGridPtr[gTargetZ][gTargetX].action = TA_30;
         gTileStateGridPtr[gTargetZ][gTargetX].cachedByte = 16;
         OBJ.timer = 70;
      } else {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_STRETCH_WARP_SPRITE;
         obj_v1->mem = 1;
         obj_v1->z1.s.hi = gTargetZ;
         obj_v1->x1.s.hi = gTargetX;
         OBJ.timer = 20;
      }
      obj->state++;
      return;

   case 13:
      if (s_unitTypeToSpawn_801231b8 == UNIT_TYPE_ANT_ARM) {
         obj_s1 = GetUnitSpriteAtPosition(gTargetZ, gTargetX);
         if (OBJ.timer == 68) {
            obj_s1->d.sprite.hidden = 0;
         }
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_CLOUD;
         obj_v1->x1.n = obj_s1->x1.n;
         obj_v1->z1.n = obj_s1->z1.n;
         obj_v1->y1.n = obj_s1->y1.n;
      }
      if (--OBJ.timer != 0) {
         return;
      }
      obj->state = 9;
      return;

   case 99:
      if (OBJ.targets == 0) {
         obj->state += 2;
         return;
      }
      gSignal5 = 99;
      obj->state++;

   // fallthrough
   case 100:
      if (gSignal5 == 100) {
         obj->state++;
      }
      return;

   case 101:
      gSignal2 = 99;
      obj->functionIndex = OBJF_NULL;
      return;

   } // switch (obj->state)
}
