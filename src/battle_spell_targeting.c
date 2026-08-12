/* Player-side spell targeting (segment 0x12dcc): one object, Objf027_TargetingSpell.
 *
 * Spawned by the unit action menu (Objf003_BattleActions, battle_field.c) once the player
 * has picked a spell into gCurrentSpell. It walks the cursor through the two grids the
 * spell data asks for: states 1-2 paint the cast range (PopulateCastingGrid onto grid 0)
 * and track the cursor, state 3 validates the picked cell against gSpells[].targeting
 * (enemy / ally / enemy group / ally group / free cell), states 4-6 repeat the pass for
 * area spells -- the fieldSize footprint on grid 1, re-validated over the yellow grid.
 * Confirm -> state 100: close the spell window, clear both grids and hand off to
 * OBJF_UNIT_CASTING (Objf028, battle_executors.c) at the caster's tile; cancel -> state 99.
 *
 * The caller is driven purely through gSignal2: 1 = cancelled, 2 = committed, 99 = the
 * casting executor finished, so this object can retire. The attack twin is
 * Objf015_TargetingAttack (battle_executors.c), same grid + gSignal2 protocol.
 *
 * State 5 re-enters state 3's case 6 body through a goto (TargetingType6_80022da4) rather
 * than a state change -- retail control flow, kept as-is. Targeting types 2 and 5 are bare
 * returns (no retail spell appears to use them). */
#include "common.h"
#include "object.h"
#include "battle.h"
#include "units.h"
#include "state.h"
#include "field.h"
#include "window.h"

#undef OBJF
#define OBJF 027
void Objf027_TargetingSpell(Object *obj) {
   UnitStatus *caster;
   s32 unitIdx;
   s32 ix, iz;
   Object *newObj;

   if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != 0) {
      gState.mapCursorOutOfRange = 0;
   } else {
      gState.mapCursorOutOfRange = 1;
   }

   caster = OBJ.caster;

   switch (obj->state) {
   case 0:
      caster = &gUnits[OBJ_MAP_UNIT(obj).s.unitIdx];
      OBJ.caster = caster;

      if (gSpells[gCurrentSpell].area == SPELL_AREA_NULL) {
         gTargetX = obj->x1.s.hi;
         gTargetZ = obj->z1.s.hi;
         obj->state = 100;
         return;
      }
      obj->state++;

   // fallthrough
   case 1:
      ClearGrid(1);
      gTargetZ = obj->z1.s.hi;
      gTargetX = obj->x1.s.hi;
      PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].range, 0);
      gYellowTargetGridPtr[gMapCursorZ][gMapCursorX] = 1;
      obj->state++;
      return;

   case 2:
      if (!(gSavedPadStateNewPresses & PAD_X)) {
         if (gSpells[gCurrentSpell].area == SPELL_AREA_AOE) {
            obj->state++;
            obj->state2 = 6;
            return;
         }
         if (gTargetX != gMapCursorX || gTargetZ != gMapCursorZ) {
            gTargetX = gMapCursorX;
            gTargetZ = gMapCursorZ;
            ClearGrid(1);
            if (gRedAttackGridPtr[gTargetZ][gTargetX] != 0) {
               gYellowTargetGridPtr[gTargetZ][gTargetX] = 1;
            }
         }
         if (gSavedPadStateNewPresses & PAD_CIRCLE) {
            obj->state++;
            obj->state2 = gSpells[gCurrentSpell].targeting;
         }
      } else {
         obj->state = 99;
      }
      return;

   case 3:

      switch (obj->state2) {
      case SPELL_TARGET_ENEMY:
         if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != 0) {
            unitIdx = gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx;
            if (unitIdx != 0 && caster->team != gUnits[unitIdx].team) {
               gTargetX = gMapCursorX;
               gTargetZ = gMapCursorZ;
               obj->state = 4;
               return;
            }
         }
         obj->state = 2;
         obj->state2 = 0;
         return;
      case SPELL_TARGET_ALLY:
         if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != 0) {
            unitIdx = gMapUnitsPtr[gMapCursorZ][gMapCursorX].s.unitIdx;
            if (unitIdx != 0 && caster->team == gUnits[unitIdx].team) {
               gTargetX = gMapCursorX;
               gTargetZ = gMapCursorZ;
               obj->state = 4;
               return;
            }
         }
         obj->state = 2;
         obj->state2 = 0;
         return;
      case 2:
         return;
      case SPELL_TARGET_ENEMY_GROUP:
         obj->state = 2;
         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != 0) {
                  unitIdx = gMapUnitsPtr[iz][ix].s.unitIdx;
                  if (unitIdx != 0 && caster->team != gUnits[unitIdx].team) {
                     gTargetX = obj->x1.s.hi;
                     gTargetZ = obj->z1.s.hi;
                     obj->state = 4;
                  }
               }
            }
         }
         obj->state2 = 0;
         return;
      case SPELL_TARGET_ALLY_GROUP:
         obj->state = 2;
         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gRedAttackGridPtr[iz][ix] != 0) {
                  unitIdx = gMapUnitsPtr[iz][ix].s.unitIdx;
                  if (unitIdx != 0 && caster->team == gUnits[unitIdx].team) {
                     gTargetX = obj->x1.s.hi;
                     gTargetZ = obj->z1.s.hi;
                     obj->state = 4;
                  }
               }
            }
         }
         obj->state2 = 0;
         return;
      case 5:
         return;
      case 6:
      TargetingType6_80022da4:
         if (gRedAttackGridPtr[gMapCursorZ][gMapCursorX] != 0) {
            gTargetX = gMapCursorX;
            gTargetZ = gMapCursorZ;
            obj->state = 4;
            break; // -> state:4
         }
         ClearGrid(1);
         if (gSavedPadStateNewPresses & PAD_X) {
            obj->state = 99;
            return;
         }
         obj->state = 2;
         obj->state2 = 0;
         return;
      case 7:
      case 8:
      case 9:
         gTargetX = obj->x1.s.hi;
         gTargetZ = obj->z1.s.hi;
         obj->state = 4;
         return;
      default:
         return;
      } // switch (obj->state2) (via state:3)

   // fallthrough
   case 4:
      if (gSpells[gCurrentSpell].area == SPELL_AREA_SINGLE) {
         obj->state = 100;
         return;
      }
      PopulateCastingGrid(gTargetZ, gTargetX, gSpells[gCurrentSpell].fieldSize, 1);
      obj->state++;

   // fallthrough
   case 5:
      if (gTargetX != gMapCursorX || gTargetZ != gMapCursorZ) {
         obj->state = 3;
         obj->state2 = 6;

         // Might explain the misidentified while loop and buggy analysis:
         goto TargetingType6_80022da4;
      }
      if (gSavedPadStateNewPresses & PAD_X) {
         obj->state = 99;
         return;
      }
      if (!(gSavedPadStateNewPresses & PAD_CIRCLE)) {
         return;
      }
      obj->state2 = gSpells[gCurrentSpell].targeting;
      obj->state++;

   // fallthrough
   case 6:

      switch (obj->state2) {
      case SPELL_TARGET_ENEMY:
         unitIdx = gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx;
         if (unitIdx != 0 && caster->team != gUnits[unitIdx].team) {
            obj->state = 100;
            return;
         }
         obj->state = 5;
         obj->state2 = 0;
         return;
      case SPELL_TARGET_ALLY:
         unitIdx = gMapUnitsPtr[gTargetZ][gTargetX].s.unitIdx;
         if (unitIdx != 0 && caster->team == gUnits[unitIdx].team) {
            obj->state = 100;
            return;
         }
         obj->state = 5;
         obj->state2 = 0;
         return;
      case SPELL_TARGET_ENEMY_GROUP:
         obj->state = 5;
         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gYellowTargetGridPtr[iz][ix] != 0) {
                  unitIdx = gMapUnitsPtr[iz][ix].s.unitIdx;
                  if (unitIdx != 0 && caster->team != gUnits[unitIdx].team) {
                     obj->state = 100;
                  }
               }
            }
         }
         obj->state2 = 0;
         return;
      case SPELL_TARGET_ALLY_GROUP:
         obj->state = 5;
         for (iz = gMapMinZ; iz <= gMapMaxZ; iz++) {
            for (ix = gMapMinX; ix <= gMapMaxX; ix++) {
               if (gYellowTargetGridPtr[iz][ix] != 0) {
                  unitIdx = gMapUnitsPtr[iz][ix].s.unitIdx;
                  if (unitIdx != 0 && caster->team == gUnits[unitIdx].team) {
                     obj->state = 100;
                  }
               }
            }
         }
         obj->state2 = 0;
         return;
      case 6:
         obj->state = 100;
         return;
      default:
         return;
      } // switch (obj->state2) (via state:6)

      return;

   case 99:
      gSignal2 = 1;
      ClearGrid(0);
      ClearGrid(1);
      obj->functionIndex = OBJF_NULL;
      return;

   case 100:
      CloseWindow(0x1e);
      ClearGrid(0);
      ClearGrid(1);
      gClearSavedPadState = 1;
      gSignal2 = 2;

      newObj = Obj_GetLastUnused();
      newObj->functionIndex = OBJF_UNIT_CASTING;
      newObj->x1.n = obj->x1.n;
      newObj->z1.n = obj->z1.n;

      obj->state++;
      return;

   case 101:
      if (gSignal2 == 99) {
         gState.mapCursorOutOfRange = 0;
         obj->functionIndex = OBJF_NULL;
      }
      return;

   } // switch (obj->state)
}
