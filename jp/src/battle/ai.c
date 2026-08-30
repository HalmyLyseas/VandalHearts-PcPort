/* Enemy/ally battle AI (segment 0x560f8): Objf570_AI_ChooseAction picks a plan and spawns one
 * planner (Objf402 spell, 403 attack, 404 retreat, 589 escape point) that publishes the move/target
 * cells and gAiActionType, then raises gAiPlanDone. See docs/game-mechanics/ai-decision-making.md. */
#include "common.h"
#include "object.h"
#include "field.h"
#include "units.h"
#include "battle.h"
#include "state.h"

#include "PsyQ/kernel.h"


void Objf570_AI_ChooseAction(Object *);
s32 AI_IsTeamDepleted(UnitStatus *);
s32 IsLagging(void);
void Objf400_AI_BuildSpellValueGrid(Object *);
void AI_PickFacingTowardEnemies(UnitStatus *);
void AI_ScoreSpellTargets(UnitStatus *);
s32 AI_ScoreAttackOption(UnitStatus *, UnitStatus *, u8, u8, u8, u8);
s32 AI_ScoreCastingPosition(UnitStatus *, u8, u8, u8, u8);
void Objf401_AI_BuildEnemyProximityGrid(Object *);
void Objf402_AI_PlanSpellCast(Object *);
void Objf403_AI_PlanAttack(Object *);
void Objf404_AI_PlanRetreat(Object *);
void Objf589_AI_MoveToEscapePoint(Object *);

extern u8 gAiSelfCastFallback, gAiPlanDone, gAiSubtaskDone;
extern s16 gAiTargetScores[40];
/* AI casting-score grid, indexed gAiCastValueGrid[iz][ix] where iz is an ABSOLUTE map-Z coordinate
 * (gMapMinZ..gMapMaxZ), not a 0-based index. Only the INNER dimension (64) enters the address
 * computation `base + (iz*64 + ix)*2`, so the outer bound is codegen-neutral. */
extern s16 gAiCastValueGrid[27][64];
extern u8 gAiSubtaskDone;

#undef OBJF
#define OBJF 570
void Objf570_AI_ChooseAction(Object *obj) {
   Object *newObj;
   UnitStatus *unit;
   u8 spellEffectA, spellEffectB;

   unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];

   switch (obj->state) {
   case 0:
      gAiPlanDone = 0;
      if (gState.mapNum == 21) {
         obj->state = 5;
         obj->state2 = 0;
         break;
      }
      if (unit->name == UNIT_LEENA) {
         obj->state = 6;
         obj->state2 = 0;
         break;
      }
      if (gState.mapNum == 26) {
         obj->state = 6;
         obj->state2 = 0;
         break;
      }

      spellEffectA = 0;
      spellEffectB = 0;

      if (unit->spells[0] == SPELL_NULL) {
         obj->state = 1;
         obj->state2 = 0;
      } else {
         spellEffectA = gSpellsEx[unit->spells[0]][SPELL_EX_EFFECT] + 1;
         if (unit->mp < gSpells[unit->spells[0]].mpCost) {
            spellEffectA = 0;
         }
         if (unit->spells[1] != SPELL_NULL) {
            spellEffectB = gSpellsEx[unit->spells[0]][SPELL_EX_EFFECT] + 1; //? Why spells[0]?
            if (unit->mp < gSpells[unit->spells[1]].mpCost) {
               spellEffectB = 0;
            }
         }
         if (spellEffectA == 0) {
            spellEffectA = spellEffectB;
            spellEffectB = 0;
            gCurrentSpell = unit->spells[1];
         } else {
            gCurrentSpell = unit->spells[0];
         }
         if (spellEffectB == 0) {
            if (spellEffectA == 0) {
               obj->state = 1;
               obj->state2 = 0;
            } else if (spellEffectA != SPELL_EFFECT_DAMAGE + 1) {
               if (gSpells[gCurrentSpell].range == 0 && gSpells[gCurrentSpell].fieldSize == 0) {
                  if (unit->hpFrac < 5000) {
                     obj->state = 3;
                     obj->state2 = 0;
                  } else {
                     obj->state = 1;
                     obj->state2 = 0;
                  }
               } else {
                  if (unit->class == CLASS_PRIEST || unit->hpFrac < 5000) {
                     obj->state = 4;
                     obj->state2 = 0;
                  } else {
                     obj->state = 1;
                     obj->state2 = 0;
                  }
               }
            } else {
               obj->state = 2;
               obj->state2 = 0;
            }
         } else if (spellEffectA == SPELL_EFFECT_DAMAGE + 1 &&
                    spellEffectB == SPELL_EFFECT_DAMAGE + 1) {
            if (rand() % 2 == 0) {
               gCurrentSpell = unit->spells[0];
            } else {
               gCurrentSpell = unit->spells[1];
            }
            obj->state = 2;
            obj->state2 = 0;
         } else if (unit->hpFrac < 5000) {
            gCurrentSpell = unit->spells[0];
            if (gSpells[unit->spells[0]].range == 0 && gSpells[unit->spells[0]].fieldSize == 0) {
               obj->state = 3;
               obj->state2 = 0;
            } else {
               obj->state = 4;
               obj->state2 = 0;
            }
         } else {
            gCurrentSpell = unit->spells[1];
            obj->state = 2;
            obj->state2 = 0;
         }
      }


      break;

   case 1:

      switch (obj->state2) {
      case 0:
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_PLAN_ATTACK;
         obj->state2++;

      // fallthrough
      case 1:
         if (gAiPlanDone) {
            obj->state = 99;
         }
         break;
      }

      break;

   case 2:

      switch (obj->state2) {
      case 0:
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_PLAN_SPELL_CAST;
         obj->state2++;

      // fallthrough
      case 1:
         if (gAiPlanDone) {
            obj->state = 99;
         }
         break;
      }

      break;

   case 3:

      switch (obj->state2) {
      case 0:
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_PLAN_RETREAT;
         obj->state2++;

      // fallthrough
      case 1:
         if (gAiPlanDone) {
            gAiActionType = 2;
            gTargetX_80123414 = gX_801233d8;
            gTargetZ_80123418 = gZ_801233dc;
            obj->state = 99;
         }
         break;
      }

      break;

   case 4:

      switch (obj->state2) {
      case 0:
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_PLAN_SPELL_CAST;
         obj->state2++;

      // fallthrough
      case 1:
         if (gAiPlanDone) {
            if (gAiSelfCastFallback) {
               gAiPlanDone = 0;
               newObj = Obj_GetUnused();
               newObj->x1.s.hi = obj->x1.s.hi;
               newObj->z1.s.hi = obj->z1.s.hi;
               newObj->functionIndex = OBJF_AI_PLAN_RETREAT;
               obj->state2++;
            } else {
               obj->state = 99;
            }
         }
         break;

      case 2:
         if (gAiPlanDone) {
            gAiActionType = 2;
            gTargetX_80123414 = gX_801233d8;
            gTargetZ_80123418 = gZ_801233dc;
            obj->state = 99;
         }
         break;
      }

      break;

   case 5:

      switch (obj->state2) {
      case 0:
         gAiPlanDone = 0;
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_PLAN_RETREAT;
         obj->state2++;
         break;

      case 1:
         if (gAiPlanDone) {
            obj->state = 99;
         }
         break;
      }

      break;

   case 6:

      switch (obj->state2) {
      case 0:
         gAiPlanDone = 0;
         newObj = Obj_GetUnused();
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         newObj->functionIndex = OBJF_AI_MOVE_TO_ESCAPE_POINT;
         obj->state2++;
         break;

      case 1:
         if (gAiPlanDone) {
            obj->state = 99;
         }
         break;
      }

      break;

   case 99:
      gAiPlanReady = 1;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

s32 AI_IsTeamDepleted(UnitStatus *unit) {
   s32 ix, iz;
   u8 team = unit->team;
   s32 ct = 0;

   for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
      for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
         if (gMapUnitsPtr[iz][ix].s.unitIdx != 0 && gMapUnitsPtr[iz][ix].s.team == team) {
            ct++;
         }
      }
   }

   return (ct <= unit->field8_0x1d);
}

s32 IsLagging(void) { return (GetRCnt(RCntCNT1) > 450); }

#undef OBJF
#define OBJF 400
void Objf400_AI_BuildSpellValueGrid(Object *obj) {
   UnitStatus *unit;
   UnitStatus *targetUnit;
   s8 team;
   // s32 i;
   s32 iz, ix;
   s32 iz2, ix2;
   s32 sVar6;

   unit = OBJ.unit;
   team = unit->team;

   switch (obj->state) {
   case 0:
      if (IsLagging()) {
         return;
      }

      OBJ.resumeZ = gMapMinZ;
      OBJ.resumeX = gMapMinX;

      if (gSpells[gCurrentSpell].targeting < SPELL_TARGET_ENEMY_GROUP) {
         obj->state += 2;
      } else {
         ClearGrid(6);

         for (iz = 1; iz < UNIT_CT; iz++) {
            targetUnit = &gUnits[iz];
            if (targetUnit->idx != 0) {

               if (gSpells[gCurrentSpell].targeting == SPELL_TARGET_ENEMY_GROUP) {
                  if (targetUnit->team == team) {
                     continue;
                  }
               } else if (unit == targetUnit || targetUnit->team != team) {
                  continue;
               }

               MarkSpellFieldCoverage(targetUnit->sprite->z1.s.hi, targetUnit->sprite->x1.s.hi,
                             gSpells[gCurrentSpell].fieldSize, 6);
            }
         }
         obj->state++;
         break;
      }
      break;

   case 1:
      iz = OBJ.resumeZ;
      ix = OBJ.resumeX;

      while (iz <= gMapMaxZ) {
         while (ix <= gMapMaxX) {
            sVar6 = 0;
            if (gPathGrid6_Ptr[iz][ix] != PATH_STEP_UNSET) {
               PopulateCastingGrid(iz, ix, gSpells[gCurrentSpell].fieldSize, 5);

               for (iz2 = gMapMinZ; iz2 <= gMapMaxZ; iz2++) {
                  for (ix2 = gMapMinX; ix2 <= gMapMaxX; ix2++) {
                     if (gPathGrid5_Ptr[iz2][ix2] != PATH_STEP_UNSET &&
                         gMapUnitsPtr[iz2][ix2].s.unitIdx != 0) {
                        sVar6 += gAiTargetScores[gUnits[gMapUnitsPtr[iz2][ix2].s.unitIdx].idx];
                     }
                  }
               }

               gAiCastValueGrid[iz][ix] = sVar6;
            } else {
               gAiCastValueGrid[iz][ix] = 0;
            }
            //@b9c
            ix++;
            if (IsLagging()) {
               OBJ.resumeX = ix;
               OBJ.resumeZ = iz;
               return;
            }
         }
         iz++;
         ix = gMapMinX;
      }

      gAiSubtaskDone++;
      obj->functionIndex = OBJF_NULL;
      break;

   case 2:
      iz = OBJ.resumeZ;
      ix = OBJ.resumeX;

      while (iz <= gMapMaxZ) {
         while (ix <= gMapMaxX) {
            sVar6 = 0;
            if (gMapUnitsPtr[iz][ix].s.unitIdx != 0) {
               sVar6 = gAiTargetScores[gUnits[gMapUnitsPtr[iz][ix].s.unitIdx].idx];
            }
            gAiCastValueGrid[iz][ix] = sVar6;
            ix++;
            if (GetRCnt(RCntCNT1) > 450) {
               OBJ.resumeX = ix;
               OBJ.resumeZ = iz;
               return;
            }
         }
         iz++;
         ix = gMapMinX;
      }

      gAiSubtaskDone++;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void AI_PickFacingTowardEnemies(UnitStatus *unit) {
   Object *sprite;
   UnitStatus *otherUnit;
   s32 i;
   s16 xDist, zDist;
   s16 largest;
   s16 a = 0;
   s16 b = 0;
   s16 c = 0;
   s16 d = 0;

   for (i = 1; i < UNIT_CT; i++) {
      otherUnit = &gUnits[i];
      sprite = otherUnit->sprite;

      if (otherUnit->idx != 0 && unit->team != otherUnit->team) {
         xDist = (gX_801233d8 - sprite->x1.s.hi);
         if (xDist < 0) {
            xDist = -xDist;
         }
         zDist = gZ_801233dc - sprite->z1.s.hi;
         if (zDist < 0) {
            zDist = -zDist;
         }
         if (xDist >= zDist) {
            if (sprite->x1.s.hi < gX_801233d8) {
               d += ((0x40 - xDist) * 2) + (0x40 - zDist);
            } else {
               b += ((0x40 - xDist) * 2) + (0x40 - zDist);
            }
         }
         if (zDist >= xDist) {
            if (sprite->z1.s.hi < gZ_801233dc) {
               c += ((0x40 - zDist) * 2) - (xDist - 0x40);
            } else {
               a += ((0x40 - zDist) * 2) - (xDist - 0x40);
            }
         }
      }
   }

   if (a == 0 && b == 0 && c == 0 && d == 0) {
      gDir_80123470 = 0xffff;
   } else {
      gDir_80123470 = ANGLE_SOUTH;
      largest = a;
      if (largest < b) {
         gDir_80123470 = ANGLE_WEST;
         largest = b;
      }
      if (largest < c) {
         gDir_80123470 = ANGLE_NORTH;
         largest = c;
      }
      if (largest < d) {
         gDir_80123470 = ANGLE_EAST;
         largest = d;
      }
   }
}

void AI_ScoreSpellTargets(UnitStatus *unit) {
   s16 i;
   UnitStatus *otherUnit;
   Object *sprite;
   s16 sVar4;

   for (i = 1; i < UNIT_CT; i++) {
      otherUnit = &gUnits[i];
      sprite = otherUnit->sprite;
      sVar4 = 0;
      if (otherUnit->idx != 0) {

         switch (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT]) {
         case SPELL_EFFECT_DAMAGE:
            if ((gSpells[gCurrentSpell].targeting == SPELL_TARGET_ENEMY ||
                 gSpells[gCurrentSpell].targeting == SPELL_TARGET_ENEMY_GROUP) &&
                unit->team != otherUnit->team) {
               sVar4 += (unit->level - otherUnit->level) * 10;
               sVar4 += 280;
               sVar4 -= (otherUnit->hpFrac / 125);
               sVar4 -= gAdvantage[unit->advantage][otherUnit->advantage];
               sVar4 -= gTerrainPreference[OBJ_TERRAIN(sprite).s.terrain] / 100;
            }
            break;

         case SPELL_EFFECT_RESTORE_HP:
            if ((gSpells[gCurrentSpell].targeting == SPELL_TARGET_ALLY ||
                 gSpells[gCurrentSpell].targeting == SPELL_TARGET_ALLY_GROUP) &&
                unit->team == otherUnit->team && unit != otherUnit && otherUnit->hpFrac < 7000) {
               sVar4 += 400;
               sVar4 -= (otherUnit->hpFrac / 25);
            }
            break;
         }

         gAiTargetScores[i] = sVar4;
      }
   }
}

s32 AI_ScoreAttackOption(UnitStatus *unit1, UnitStatus *unit2, u8 z1, u8 x1, u8 z2, u8 x2) {
   s32 result;

   if (unit2->name == UNIT_LEENA) {
      result = 5000;
   } else {
      result = 10000;
   }

   switch (unit2->direction >> 10) {
   case DIR_SOUTH:
      if (x1 != x2) {
         result += 20;
      } else if (z1 < z2) {
         result += 30;
      }
      break;
   case DIR_WEST:
      if (z1 != z2) {
         result += 20;
      } else if (x1 < x2) {
         result += 30;
      }
      break;
   case DIR_NORTH:
      if (x1 != x2) {
         result += 20;
      } else if (z1 > z2) {
         result += 30;
      }
      break;
   default:
      if (z1 != z2) {
         result += 20;
      } else if (x1 > x2) {
         result += 30;
      }
      break;
   }

   result += gAiTargetScores[unit2->idx];
   result -= gPathGrid6_Ptr[z1][x1] * 3;
   result -= (gPathGrid3_Ptr[z1][x1] - 1) / unit1->travelRange * 1000;
   if (result < 1) {
      result = 1;
   }
   result += (gTerrainPtr[z1][x1].s.elevation - gTerrainPtr[z2][x2].s.elevation) * 10;
   result += gTerrainPreference[gTerrainPtr[z1][x1].s.terrain] / 100;

   if (unit1->class == CLASS_ARCHER) {
      if (unit2->class == CLASS_ARCHER) {
         PopulateRangedAttackGrid(z2, x2, unit2->attackRange, 0);
         if (gPathGrid0_Ptr[z1][x1] != PATH_STEP_UNSET) {
            result -= 200;
         }
      } else {
         PopulateMeleeAttackGrid(z2, x2, 0, 1);
         if (gPathGrid0_Ptr[z1][x1] != PATH_STEP_UNSET) {
            result -= 200;
         }
      }
      result += 200;
      result -= (unit2->hpFrac / 50);
      ClearGrid(0);
   }

   return result;
}

s32 AI_ScoreCastingPosition(UnitStatus *unit, u8 z1, u8 x1, u8 z2, u8 x2) {
   s32 result = gAiCastValueGrid[z2][x2];

   if (result >= 1) {
      result += 10000;
      result -= (gPathGrid3_Ptr[z1][x1] - 1) / unit->travelRange * 1000;
      result += gTerrainPreference[gTerrainPtr[z1][x1].s.terrain] / 100;
      result -= gPathGrid6_Ptr[z1][x1] * 3;
   } else {
      result = 0;
   }

   return result;
}

#undef OBJF
#define OBJF 401
void Objf401_AI_BuildEnemyProximityGrid(Object *obj) {
   UnitStatus *unit1;
   UnitStatus *unit2;
   Object *sprite1;
   Object *sprite2;
   s16 i;

   unit1 = &gUnits[OBJ.unitIdx];
   sprite1 = unit1->sprite;

   switch (obj->state) {
   case 0:
      unit1 = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
      sprite1 = unit1->sprite;
      ClearGrid(0);
      ClearGrid(6);

      OBJ.team = OBJ_MAP_UNIT(sprite1).s.team;
      OBJ.unitIdx = OBJ_MAP_UNIT(sprite1).s.unitIdx;
      OBJ_MAP_UNIT(sprite1).raw = 0;
      OBJ.unitIter = 0;
      obj->state++;
      break;

   case 1:
      i = OBJ.unitIter;

      while (++i != UNIT_CT) {
         unit2 = &gUnits[i];
         sprite2 = unit2->sprite;

         if (unit2->idx != 0 && unit1->team != unit2->team) {
            AccumulateProximityGrid(sprite2->z1.s.hi, sprite2->x1.s.hi, 0xff, 0, 6);
            ClearGrid(0);
            if (GetRCnt(RCntCNT1) > 450) {
               OBJ.unitIter = i;
               return;
            }
         }
      }

      OBJ_MAP_UNIT(sprite1).s.team = OBJ.team;
      OBJ_MAP_UNIT(sprite1).s.unitIdx = OBJ.unitIdx;
      ClearGrid(0);
      gAiSubtaskDone++;
      obj->functionIndex = OBJF_NULL;
      return;
   }
}

u8 s_shouldHealSelf_801232d8;
s32 s_x1_801232dc;
s32 s_z1_801232e0;
s32 s_pref_801232e4;
s32 s_z2_801232e8;
s32 s_x2_801232ec;
s32 s_x3_801232f0;
s32 s_z3_801232f4;

#undef OBJF
#define OBJF 402
void Objf402_AI_PlanSpellCast(Object *obj) {
   UnitStatus *unit;
   Object *sprite;
   Object *newObj;
   s32 i;
   s32 ix2, iz2;
   s32 tmp;

   unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
   sprite = unit->sprite;

   switch (obj->state) {
   case 0:
      if (IsLagging()) {
         return;
      }

      gAiPlanDone = 0;
      gAiSelfCastFallback = 0;

      if (unit->hpFrac < 5000 &&
          gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_RESTORE_HP) {
         s_shouldHealSelf_801232d8 = 1;
      } else {
         s_shouldHealSelf_801232d8 = 0;
      }

      AI_ScoreSpellTargets(unit);
      gDir_80123470 = 0xffff;
      ClearGrid(0);
      ClearGrid(1);
      obj->state++;

   // fallthrough
   case 1:
      if (IsLagging()) {
         return;
      }

      PopulateMovementGrid(sprite->z1.s.hi, sprite->x1.s.hi, 0xff, 2);
      obj->state++;

   // fallthrough
   case 2:
      if (IsLagging()) {
         return;
      }

      i = (unit->field7_0x1c + 1) * unit->travelRange;
      if (i > 0xff) {
         i = 0xff;
      }
      if (unit->hp != unit->maxHp) {
         i = 0xff;
      }
      if (AI_IsTeamDepleted(unit)) {
         i = 0xff;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, i, 3);
      obj->state++;

   // fallthrough
   case 3:
      if (IsLagging()) {
         return;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, unit->travelRange, 4);
      obj->state++;

   // fallthrough
   case 4:
      if (IsLagging()) {
         return;
      }

      newObj = Obj_GetLastUnused();
      newObj->functionIndex = OBJF_AI_BUILD_SPELL_VALUE_GRID;
      newObj->d.objf400.unit = unit;
      gAiSubtaskDone = 0;
      obj->state++;
      return;

   case 5:
      if (gAiSubtaskDone != 0) {
         newObj = Obj_GetLastUnused();
         newObj->functionIndex = OBJF_AI_BUILD_ENEMY_PROXIMITY_GRID;
         newObj->x1.s.hi = obj->x1.s.hi;
         newObj->z1.s.hi = obj->z1.s.hi;
         gAiSubtaskDone = 0;
         obj->state++;
      }
      return;

   case 6:
      if (gAiSubtaskDone != 0) {
         OBJ.resumeZ = gMapMinZ;
         OBJ.resumeX = gMapMinX;
         s_pref_801232e4 = 0;
         obj->state++;
      }
      return;

   case 7:
      if (IsLagging()) {
         return;
      }

      s_z1_801232e0 = OBJ.resumeZ;
      s_x1_801232dc = OBJ.resumeX;

      while (s_z1_801232e0 <= gMapMaxZ) {
         while (s_x1_801232dc <= gMapMaxX) {

            if ((s_x1_801232dc == sprite->x1.s.hi && s_z1_801232e0 == sprite->z1.s.hi) ||
                (gPathGrid3_Ptr[s_z1_801232e0][s_x1_801232dc] != PATH_STEP_UNSET &&
                 gMapUnitsPtr[s_z1_801232e0][s_x1_801232dc].s.unitIdx == 0)) {

               if (GetRCnt(RCntCNT1) > 450) {
                  OBJ.resumeX = s_x1_801232dc;
                  OBJ.resumeZ = s_z1_801232e0;
                  return;
               }

               if (s_shouldHealSelf_801232d8 && gSpells[gCurrentSpell].range != 0) {
                  ClearGrid(5);
                  gPathGrid5_Ptr[s_z1_801232e0][s_x1_801232dc] += 1;
               } else {
                  PopulateCastingGrid(s_z1_801232e0, s_x1_801232dc, gSpells[gCurrentSpell].range,
                                      5);
               }

               for (iz2 = gMapMinZ; iz2 <= gMapMaxZ; iz2++) {
                  for (ix2 = gMapMinX; ix2 <= gMapMaxX; ix2++) {
                     if (gPathGrid5_Ptr[iz2][ix2] != PATH_STEP_UNSET) {
                        i = AI_ScoreCastingPosition(unit, s_z1_801232e0, s_x1_801232dc, iz2, ix2);
                        if (i > s_pref_801232e4) {
                           s_pref_801232e4 = i;
                           s_z2_801232e8 = s_z1_801232e0;
                           s_x2_801232ec = s_x1_801232dc;
                           s_z3_801232f4 = iz2;
                           s_x3_801232f0 = ix2;
                        }
                     }
                  }
               }
            }

            s_x1_801232dc++;
         }

         s_z1_801232e0++;
         s_x1_801232dc = gMapMinX;

         if (GetRCnt(RCntCNT1) > 450) {
            OBJ.resumeX = s_x1_801232dc;
            OBJ.resumeZ = s_z1_801232e0;
            return;
         }
      }
      if (s_pref_801232e4 < 1) {
         s_x2_801232ec = obj->x1.s.hi;
         s_z2_801232e8 = obj->z1.s.hi;
      }
      obj->state++;
      return;

   case 8:
      if (IsLagging()) {
         return;
      }

      gX_801233d8 = s_x2_801232ec;
      gZ_801233dc = s_z2_801232e8;

      if (obj->x1.s.hi == s_x2_801232ec && obj->z1.s.hi == s_z2_801232e8) {
         if (s_pref_801232e4 > 0) {
            gAiActionType = 2;
            gTargetX_80123414 = s_x3_801232f0;
            gTargetZ_80123418 = s_z3_801232f4;
         } else {
            gAiActionType = 0;
            AI_PickFacingTowardEnemies(unit);
         }
         obj->state = 99;
      } else if (gPathGrid4_Ptr[s_z2_801232e8][s_x2_801232ec] != PATH_STEP_UNSET) {
         gAiActionType = 1;
         gX_801233d8 = s_x2_801232ec;
         gZ_801233dc = s_z2_801232e8;
         gTargetX_80123414 = s_x3_801232f0;
         gTargetZ_80123418 = s_z3_801232f4;
         gAiActionType = 2;
         obj->state = 99;
      } else {
         obj->state++;
      }

      return;

   case 9:

      while (gPathGrid4_Ptr[s_z2_801232e8][s_x2_801232ec] == PATH_STEP_UNSET ||
             gMapUnitsPtr[s_z2_801232e8][s_x2_801232ec].s.unitIdx != 0) {

         switch (gPathGrid2_Ptr[s_z2_801232e8][s_x2_801232ec]) {
         case PATH_STEP_SOUTH:
            s_z2_801232e8--;
            continue;
         case PATH_STEP_WEST:
            s_x2_801232ec--;
            continue;
         case PATH_STEP_NORTH:
            s_z2_801232e8++;
            continue;
         case PATH_STEP_EAST:
            s_x2_801232ec++;
            continue;
         }

         break;
      }

      gAiActionType = 0;
      gX_801233d8 = s_x2_801232ec;
      gZ_801233dc = s_z2_801232e8;
      AI_PickFacingTowardEnemies(unit);
      obj->state = 99;
      return;

   case 99:

      if (s_shouldHealSelf_801232d8) {
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            if (gAiActionType == 0) {
               gAiActionType = 2;
               gTargetZ_80123418 = gZ_801233dc;
               gTargetX_80123414 = gX_801233d8;
               gAiSelfCastFallback = 1;
            }
         } else if (gSpells[gCurrentSpell].area == SPELL_AREA_NULL) {
            if (gAiActionType == 0) {
               gAiActionType = 2;
               gTargetZ_80123418 = gZ_801233dc;
               gTargetX_80123414 = gX_801233d8;
               gAiSelfCastFallback = 1;
            }
         } else {
            if (gAiActionType == 0) {
               gAiActionType = 2;
               gTargetZ_80123418 = gZ_801233dc;
               gTargetX_80123414 = gX_801233d8;
               gAiSelfCastFallback = 1;
            }
         }
      }

      gAiPlanDone = 1;
      obj->functionIndex = OBJF_NULL;
      return;
   }
}

s32 s_x1_801232f8;
s32 s_z1_801232fc;
s32 s_pref_80123300;
s32 s_z2_80123304;
s32 s_x2_80123308;
s32 s_x3_8012330c;
s32 s_z3_80123310;

#undef OBJF
#define OBJF 403
void Objf403_AI_PlanAttack(Object *obj) {
   UnitStatus *unit1;
   UnitStatus *unit2;
   Object *sprite;
   Object *newObj;
   s32 i;
   s32 iz2, ix2;

   unit1 = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
   sprite = unit1->sprite;

   switch (obj->state) {
   case 0:
      if (IsLagging()) {
         return;
      }

      gAiPlanDone = 0;
      AI_ScoreSpellTargets(unit1);
      gDir_80123470 = 0xffff;
      ClearGrid(0);
      ClearGrid(1);
      obj->state++;

   // fallthrough
   case 1:
      if (IsLagging()) {
         return;
      }

      PopulateMovementGrid(sprite->z1.s.hi, sprite->x1.s.hi, 0xff, 2);
      obj->state++;

   // fallthrough
   case 2:
      if (IsLagging()) {
         return;
      }

      i = (unit1->field7_0x1c + 1) * unit1->travelRange;
      if (i > 0xff) {
         i = 0xff;
      }
      if (unit1->hp != unit1->maxHp) {
         i = 0xff;
      }
      if (AI_IsTeamDepleted(unit1) != 0) {
         i = 0xff;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, i, 3);
      obj->state++;

   // fallthrough
   case 3:
      if (IsLagging()) {
         return;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, unit1->travelRange, 4);
      obj->state++;

   // fallthrough
   case 4:
      if (IsLagging()) {
         return;
      }

      newObj = Obj_GetLastUnused();
      newObj->functionIndex = OBJF_AI_BUILD_ENEMY_PROXIMITY_GRID;
      newObj->x1.s.hi = obj->x1.s.hi;
      newObj->z1.s.hi = obj->z1.s.hi;
      gAiSubtaskDone = 0;
      obj->state++;
      break;

   case 5:
      if (gAiSubtaskDone != 0) {
         OBJ.resumeZ = gMapMinZ;
         OBJ.resumeX = gMapMinX;
         s_pref_80123300 = 0;
         obj->state++;
      }
      break;

   case 6:
      if (IsLagging()) {
         return;
      }

      s_z1_801232fc = OBJ.resumeZ;
      s_x1_801232f8 = OBJ.resumeX;

      while (s_z1_801232fc <= gMapMaxZ) {
         while (s_x1_801232f8 <= gMapMaxX) {

            if ((s_x1_801232f8 == sprite->x1.s.hi && s_z1_801232fc == sprite->z1.s.hi) ||
                (gPathGrid3_Ptr[s_z1_801232fc][s_x1_801232f8] != PATH_STEP_UNSET &&
                 gMapUnitsPtr[s_z1_801232fc][s_x1_801232f8].s.unitIdx == 0)) {

               if (GetRCnt(RCntCNT1) > 450) {
                  OBJ.resumeX = s_x1_801232f8;
                  OBJ.resumeZ = s_z1_801232fc;
                  return;
               }

               if (unit1->class == CLASS_ARCHER) {
                  PopulateRangedAttackGrid(s_z1_801232fc, s_x1_801232f8, unit1->attackRange, 5);
               } else {
                  PopulateMeleeAttackGrid(s_z1_801232fc, s_x1_801232f8, 5, 1);
               }

               for (iz2 = gMapMinZ; iz2 <= gMapMaxZ; iz2++) {
                  for (ix2 = gMapMinX; ix2 <= gMapMaxX; ix2++) {
                     if (gPathGrid5_Ptr[iz2][ix2] != PATH_STEP_UNSET &&
                         gMapUnitsPtr[iz2][ix2].s.unitIdx != 0) {

                        unit2 = &gUnits[gMapUnitsPtr[iz2][ix2].s.unitIdx];
                        if (unit1->team != unit2->team) {
                           i = AI_ScoreAttackOption(unit1, unit2, s_z1_801232fc, s_x1_801232f8, iz2, ix2);
                           if (i > s_pref_80123300) {
                              s_pref_80123300 = i;
                              s_z2_80123304 = s_z1_801232fc;
                              s_x2_80123308 = s_x1_801232f8;
                              s_z3_80123310 = iz2;
                              s_x3_8012330c = ix2;
                           }
                        }
                     }
                  }
               }
            }

            s_x1_801232f8++;
         }

         s_z1_801232fc++;
         s_x1_801232f8 = gMapMinX;

         if (GetRCnt(RCntCNT1) > 450) {
            OBJ.resumeX = s_x1_801232f8;
            OBJ.resumeZ = s_z1_801232fc;
            return;
         }
      }
      if (s_pref_80123300 == 0) {
         s_x2_80123308 = obj->x1.s.hi;
         s_z2_80123304 = obj->z1.s.hi;
      }
      obj->state++;
      break;

   case 7:
      if (IsLagging()) {
         return;
      }

      gX_801233d8 = s_x2_80123308;
      gZ_801233dc = s_z2_80123304;

      if (obj->x1.s.hi == s_x2_80123308 && obj->z1.s.hi == s_z2_80123304) {
         if (s_pref_80123300 != 0) {
            gAiActionType = 1;
            gTargetX_80123414 = s_x3_8012330c;
            gTargetZ_80123418 = s_z3_80123310;
         } else {
            gAiActionType = 0;
            AI_PickFacingTowardEnemies(unit1);
         }
         obj->state = 99;
      } else if (gPathGrid4_Ptr[s_z2_80123304][s_x2_80123308] != PATH_STEP_UNSET) {
         gAiActionType = 1;
         gX_801233d8 = s_x2_80123308;
         gZ_801233dc = s_z2_80123304;
         gTargetX_80123414 = s_x3_8012330c;
         gTargetZ_80123418 = s_z3_80123310;
         obj->state = 99;
      } else {
         obj->state++;
      }

      break;

   case 8:

      while (gPathGrid4_Ptr[s_z2_80123304][s_x2_80123308] == PATH_STEP_UNSET ||
             gMapUnitsPtr[s_z2_80123304][s_x2_80123308].s.unitIdx != 0) {

         switch (gPathGrid2_Ptr[s_z2_80123304][s_x2_80123308]) {
         case PATH_STEP_SOUTH:
            s_z2_80123304--;
            continue;
         case PATH_STEP_WEST:
            s_x2_80123308--;
            continue;
         case PATH_STEP_NORTH:
            s_z2_80123304++;
            continue;
         case PATH_STEP_EAST:
            s_x2_80123308++;
            continue;
         }

         break;
      }

      gAiActionType = 0;
      gX_801233d8 = s_x2_80123308;
      gZ_801233dc = s_z2_80123304;
      AI_PickFacingTowardEnemies(unit1);
      obj->state = 99;
      break;

   case 99:
      gAiPlanDone = 1;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

s32 s_pref_80123314;
s32 s_z_80123318;
s32 s_x_8012331c;

#undef OBJF
#define OBJF 404
void Objf404_AI_PlanRetreat(Object *obj) {
   UnitStatus *unit;
   Object *sprite;
   Object *newObj;
   s32 i;
   s32 iz, ix;

   unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
   sprite = unit->sprite;

   switch (obj->state) {
   case 0:
      if (IsLagging()) {
         return;
      }

      gAiPlanDone = 0;
      gDir_80123470 = 0xffff;
      ClearGrid(0);
      ClearGrid(1);
      obj->state++;

   // fallthrough
   case 1:
      if (IsLagging()) {
         return;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, unit->travelRange, 4);
      obj->state++;

   // fallthrough
   case 2:
      if (IsLagging()) {
         return;
      }

      newObj = Obj_GetLastUnused();
      newObj->functionIndex = OBJF_AI_BUILD_ENEMY_PROXIMITY_GRID;
      newObj->x1.s.hi = obj->x1.s.hi;
      newObj->z1.s.hi = obj->z1.s.hi;
      gAiSubtaskDone = 0;
      obj->state++;
      break;

   case 3:
      if (gAiSubtaskDone == 0) {
         break;
      }

      OBJ.resumeZ = gMapMinZ;
      OBJ.resumeX = gMapMinX;
      s_pref_80123314 = 10000;
      obj->state++;

   // fallthrough
   case 4:
      if (IsLagging()) {
         return;
      }

      iz = OBJ.resumeZ;
      ix = OBJ.resumeX;

      while (iz <= gMapMaxZ) {
         while (ix <= gMapMaxX) {

            if ((ix == sprite->x1.s.hi && iz == sprite->z1.s.hi) ||
                (gPathGrid4_Ptr[iz][ix] != PATH_STEP_UNSET &&
                 gMapUnitsPtr[iz][ix].s.unitIdx == 0)) {

               if (GetRCnt(RCntCNT1) > 450) {
                  OBJ.resumeX = ix;
                  OBJ.resumeZ = iz;
                  return;
               }

               i = (gPathGrid6_Ptr[iz][ix] * 10) -
                   (gTerrainPreference[gTerrainPtr[iz][ix].s.terrain] / 100) +
                   -(gTerrainPtr[iz][ix].s.elevation * 10);
               if (s_pref_80123314 > i) {
                  s_pref_80123314 = i;
                  s_z_80123318 = iz;
                  s_x_8012331c = ix;
               }
            }

            ix++;
         }

         iz++;
         ix = gMapMinX;

         if (GetRCnt(RCntCNT1) > 450) {
            OBJ.resumeX = ix;
            OBJ.resumeZ = iz;
            return;
         }
      }

      obj->state++;
      break;

   case 5:
      gAiActionType = 0;
      gX_801233d8 = s_x_8012331c;
      gZ_801233dc = s_z_80123318;
      AI_PickFacingTowardEnemies(unit);
      obj->state = 99;
      break;

   case 99:
      gAiPlanDone = 1;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

s32 D_80123320; // unused
s32 D_80123324; // unused

s32 s_pref_80123328;
s32 s_z_8012332c;
s32 s_x_80123330;

#undef OBJF
#define OBJF 589
void Objf589_AI_MoveToEscapePoint(Object *obj) {
   UnitStatus *unit;
   Object *sprite;
   s32 i;
   s32 iz, ix;
   s32 PVar4;

   unit = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
   sprite = unit->sprite;

   switch (obj->state) {
   case 0:
      if (IsLagging()) {
         return;
      }

      gAiPlanDone = 0;
      gDir_80123470 = 0xffff;
      ClearGrid(0);
      ClearGrid(1);
      obj->state++;

   // fallthrough
   case 1:
      if (IsLagging()) {
         return;
      }

      PopulateMoveCostGrid(sprite->z1.s.hi, sprite->x1.s.hi, unit->travelRange, 4);
      PopulateReachGradientGrid(sprite->z1.s.hi, sprite->x1.s.hi, 0xfe, 6);
      obj->state++;

   // fallthrough
   case 2:
      if (IsLagging()) {
         return;
      }

      PVar4 = 0;

      if (gState.mapNum == 39) {
         iz = 11;
         ix = 16;
         if (gTerrainPtr[2][10].s.terrain == TERRAIN_PLAINS) {
            iz = 2;
            ix = 10;
         }
         if (gTerrainPtr[7][7].s.terrain == TERRAIN_PLAINS) {
            iz = 6;
            ix = 3;
         }
      } else {
         iz = sprite->z1.s.hi;
         ix = sprite->x1.s.hi;

         for (i = 0; i < 12; i++) {
            if (PVar4 < gPathGrid6_Ptr[i][47]) {
               iz = i;
               ix = 47;
               PVar4 = gPathGrid6_Ptr[i][47];
            }
         }
         if (gState.mapState.s.field_0x13 != 0) {
            for (i = 0; i < 12; i++) {
               if (PVar4 < gPathGrid6_Ptr[i][0]) {
                  iz = i;
                  ix = 0;
                  PVar4 = gPathGrid6_Ptr[i][0];
               }
            }
         }
      }

      PopulateReachGradientGridFrom(iz, ix, 0xfe, 6, sprite->z1.s.hi, sprite->x1.s.hi);
      obj->state++;
      break;

   case 3:
      s_z_8012332c = obj->z1.s.hi;
      s_x_80123330 = obj->x1.s.hi;
      OBJ.resumeZ = gMapMinZ;
      OBJ.resumeX = gMapMinX;
      s_pref_80123328 = 0;
      obj->state++;

   // fallthrough
   case 4:
      if (IsLagging()) {
         return;
      }

      iz = OBJ.resumeZ;
      ix = OBJ.resumeX;

      while (iz <= gMapMaxZ) {
         while (ix <= gMapMaxX) {

            if ((ix == sprite->x1.s.hi && iz == sprite->z1.s.hi) ||
                (gPathGrid4_Ptr[iz][ix] != PATH_STEP_UNSET &&
                 gMapUnitsPtr[iz][ix].s.unitIdx == 0)) {

               if (GetRCnt(RCntCNT1) > 450) {
                  OBJ.resumeX = ix;
                  OBJ.resumeZ = iz;
                  return;
               }

               i = gPathGrid6_Ptr[iz][ix];
               if (s_pref_80123328 < i) {
                  s_pref_80123328 = i;
                  s_z_8012332c = iz;
                  s_x_80123330 = ix;
               }
            }

            ix++;
         }

         iz++;
         ix = gMapMinX;

         if (GetRCnt(RCntCNT1) > 450) {
            OBJ.resumeX = ix;
            OBJ.resumeZ = iz;
            return;
         }
      }

      obj->state++;
      break;

   case 5:
      gAiActionType = 0;
      gX_801233d8 = s_x_80123330;
      gZ_801233dc = s_z_8012332c;
      gDir_80123470 = 0xffff;
      obj->state = 99;
      break;

   case 99:
      gAiPlanDone = 1;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}
