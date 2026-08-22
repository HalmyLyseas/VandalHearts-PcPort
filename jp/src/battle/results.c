/* The post-battle results screen and the slain-unit tally that feeds it (segment
 * 0x38148).
 *
 * TallySlainUnit is called from every death path of Objf014_BattleUnit (units/actor.c):
 * party members set a flag in gPartyMemberSlain, everyone else bumps the matching
 * gSlainUnits count. Both arrays are zeroed per battle by battle/evaluators.c and mirrored into
 * the in-battle save by core/card.c. CommitPartyStatus flushes every live gUnits entry back
 * into gPartyMembers; the results object runs it first, and Objf424_BattleEnder's debug
 * skip reuses it.
 *
 * Objf594_BattleResults is spawned by battle/evaluators.c once the victory banner finishes, and
 * runs three concurrent lanes over the shared s_* statics: obj->state draws the windows;
 * obj->state2 walks the kill list and then the lost-party list, spawning one Objf593 child
 * every 10 frames and moving to a fresh row when the first penalty entry appears (a
 * negative gBattleUnitRewards value, or any lost party member); obj->state3 is the gold
 * counter, re-rendering the SJIS total whenever a child posts a reward and adding it to
 * gState.gold at the end. Slots wrap at 8 per row; the grid restarts past 32.
 *
 * Objf593_BattleResultsUnit draws one slot: the unit's strip sprite, with GFX_RED_X laid
 * under it when the entry is a penalty. */
#include "common.h"
#include "units.h"
#include "object.h"
#include "window.h"
#include "battle.h"
#include "state.h"
#include "audio.h"



#ifdef PC_FEAT
/* Stage 3 1.3 GAP 10 -- Trial rewards (pc_balance.c): per-chapter gold penalty per lost unit on trial
 * maps (mapNum <= 5) in Tactical. Normal keeps the retail gBattlePenalties table. */
extern int gTacticalMode;
extern int TrialGoldPenalty(int chapter);
#endif

void CommitPartyStatus(void) {
   s32 i;
   UnitStatus *pUnit = &gUnits[1];

   for (i = 1; i < ARRAY_COUNT(gUnits); i++) {
      if (pUnit->idx != 0) {
         CommitPartyMemberStatus(pUnit);
      }
      pUnit++;
   }
}

u8 s_i_80123284;
u8 s_slot_80123288;
u8 s_delay_8012328c;
u8 s_penalized_80123290;
s32 s_totalReward_80123294;
s32 s_currentReward_80123298;

#undef OBJF
#define OBJF 594
void Objf594_BattleResults(Object *obj) {
   static u8 goldBuffer[15] = "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x82\x66";
   s32 i;
   Object *rewardObj;

   switch (obj->state) {
   case 0:
      CommitPartyStatus();
      DrawWindow(0x3e, 0, 180, 88, 36, 100, 16, WBS_CROSSED, 0);
      DisplayCustomWindow(0x3e, 0, 1, 2, 0, 0);
      DrawText(20, 191, 25, 2, 0, "\x90\xed\x93\xac\x8c\x8b\x89\xca");
      DrawWindow(0x3c, 0, 0, 296, 162, 10, 39, WBS_DRAGON, 0);
      DisplayCustomWindow(0x3c, 0, 1, 2, 0, 0);
      DisplayCustomWindow(0x3d, 0, 1, 2, 0, 0);
      s_delay_8012328c = 15;
      obj->state++;
      break;

   case 1:

      switch (obj->state2) {
      case 0:
         if (--s_delay_8012328c == 0) {
            s_slot_80123288 = 0;
            s_i_80123284 = 0;
            s_penalized_80123290 = 0;
            obj->state2++;
         }
         break;

      case 1:

         while (gSlainUnits[s_i_80123284].count == 0 || gSlainUnits[s_i_80123284].reward == 0) {
            s_i_80123284++;
            if (s_i_80123284 == 20) {
               obj->state2 += 2;
               return;
            }
         }

         if (!s_penalized_80123290) {
            i = 0;
            while (gSlainUnits[s_i_80123284].unitId != gSpriteStripUnitIds[i]) {
               i++;
            }
            if (gBattleUnitRewards[gState.mapNum][i] < 0) {
               s_penalized_80123290 = 1;
               i = s_slot_80123288 % 8;
               if (i != 0) {
                  // Penalties are shown on a separate row
                  s_slot_80123288 -= (i - 8);
               }
            }
         }
         gSlainUnits[s_i_80123284].count -= 1;
         if (s_slot_80123288 >= 32) {
            // Clear existing if too many
            s_slot_80123288 = 0;
            Obj_ResetByFunction(OBJF_BATTLE_RESULTS_UNIT);
         }
         rewardObj = Obj_GetUnused();
         rewardObj->functionIndex = OBJF_BATTLE_RESULTS_UNIT;
         rewardObj->d.objf593.unitId = gSlainUnits[s_i_80123284].unitId;
         rewardObj->d.objf593.slot = s_slot_80123288++;
         if (s_penalized_80123290) {
            rewardObj->d.objf593.isPenalty = 1;
         }
         s_currentReward_80123298 = gSlainUnits[s_i_80123284].reward;
         s_delay_8012328c = 10;
         obj->state2++;

      // fallthrough
      case 2:
         if (--s_delay_8012328c == 0) {
            obj->state2--;
         }
         break;

      case 3:
         if (!s_penalized_80123290) {
            i = s_slot_80123288 % 8;
            if (i != 0) {
               s_slot_80123288 -= (i - 8);
            }
         }
         s_i_80123284 = 0;
         obj->state2++;

      // fallthrough
      case 4:
         while (!gPartyMemberSlain[s_i_80123284]) {
            s_i_80123284++;
            if (s_i_80123284 == 12) {
               obj->state2 += 2;
               return;
            }
         }
         gPartyMemberSlain[s_i_80123284] = 0;
         if (s_slot_80123288 >= 32) {
            // Clear existing if too many
            s_slot_80123288 = 0;
            Obj_ResetByFunction(OBJF_BATTLE_RESULTS_UNIT);
         }
         rewardObj = Obj_GetUnused();
         rewardObj->functionIndex = OBJF_BATTLE_RESULTS_UNIT;
         rewardObj->d.objf593.unitId = gSpriteStripUnitIds[s_i_80123284];
         rewardObj->d.objf593.slot = s_slot_80123288++;
         rewardObj->d.objf593.isPenalty = 1;
#ifdef PC_FEAT
         /* GAP 10: a careless trial run now costs gold per lost unit, scaled per chapter (retail trial
          * penalty is a flat 10). Matches the raised trial reward so trials carry real stakes. */
         if (gTacticalMode && gState.mapNum <= 5) {
            s_currentReward_80123298 -= TrialGoldPenalty(gState.chapter);
         } else {
            s_currentReward_80123298 -= gBattlePenalties[gState.mapNum];
         }
#else
         s_currentReward_80123298 -= gBattlePenalties[gState.mapNum];
#endif
         s_delay_8012328c = 10;
         obj->state2++;

      // fallthrough
      case 5:
         if (--s_delay_8012328c == 0) {
            obj->state2--;
         }
         break;

      case 6:
         gState.gold += s_totalReward_80123294;
         obj->state++;

      // fallthrough
      case 7:
         gSignal2 = 1;
         break;
      } // switch (obj->state2) (via state:1)

      break;
   } // switch (obj->state)

   switch (obj->state3) {
   case 0:
      s_totalReward_80123294 = 0;
      s_currentReward_80123298 = 0;
      DrawWindow(0x3f, 0, 218, 104, 36, 188, 188, WBS_CROSSED, 0);
      EmbedIntAsSjis(s_totalReward_80123294, goldBuffer, 6);
      DrawSjisText(12, 229, 10, 2, 0, goldBuffer);
      DisplayCustomWindow(0x3f, 0, 1, 2, 0, 0);
      obj->state3++;
   // fallthrough
   case 1:
      if (s_currentReward_80123298 != 0) {
         s_totalReward_80123294 += s_currentReward_80123298;
         if (s_totalReward_80123294 < 0) {
            s_totalReward_80123294 = 0;
         }
         s_currentReward_80123298 = 0;
         EmbedIntAsSjis(s_totalReward_80123294, goldBuffer, 6);
         DrawSjisText(12, 229, 10, 2, 0, goldBuffer);
      }
   }
}

#undef OBJF
#define OBJF 593
void Objf593_BattleResultsUnit(Object *obj) {
   s32 i;

   switch (obj->state) {
   case 0:
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(239));
      OBJ.unitClut = gUnitClutIds[OBJ.unitId];

      // Find matching sprite strip
      i = 0;
      while (gSpriteStripUnitIds[i] != OBJ.unitId) {
         i++;
      }
      OBJ.unitGfxIdx = GFX_STRIP_SPRITES_OFS + i;

      i = OBJ.slot;
      obj->x1.n = (i % 8) * 32 + 28;
      obj->y1.n = (i / 8) * 30 + 57;
      obj->x3.n = obj->x1.n + 33;
      obj->y3.n = obj->y1.n + 33;
      OBJ.otOfs = 2;
      obj->state++;

   // fallthrough
   case 1:
      if (OBJ.isPenalty) {
         OBJ.clut = CLUT_NULL;
         OBJ.gfxIdx = GFX_RED_X;
         AddObjPrim_Gui(gGraphicsPtr->ot, obj);
      }
      OBJ.gfxIdx = OBJ.unitGfxIdx;
      OBJ.clut = OBJ.unitClut;
      AddObjPrim_Gui(gGraphicsPtr->ot, obj);
   }
}

void TallySlainUnit(UnitStatus *unit) {
   s32 i;

   if (unit->unitId <= UNIT_ID_END_OF_PARTY) {
      gPartyMemberSlain[unit->name - 1] = 1;
   } else {
      for (i = 0; i < ARRAY_COUNT(gSlainUnits); i++) {
         if (unit->unitId == gSlainUnits[i].unitId) {
            gSlainUnits[i].count++;
         }
      }
   }
}
