#include "common.h"
#include "units.h"
#include "object.h"
#include "window.h"
#include "battle.h"
#include "state.h"


u8 s_currentIdx_8012329c;

void Objf596_StatusWindowMgr(Object *obj) {
   // Null-terminated list of gUnits-indices of each available party member
   extern u8 gPartyMemberUnitIdx[PARTY_CT];

   s32 i, j;
   s32 ct;
   UnitStatus *pUnit;
   Object *statusWindow;

   switch (obj->state) {
   case 0:
      ct = 0;
      for (i = 1; i < PARTY_CT; i++) {
         if (gPartyMembers[i].inParty) {
            pUnit = &gUnits[1];
            for (j = 1; j < ARRAY_COUNT(gUnits); j++) {
               if (pUnit->idx != 0 && pUnit->name == i) {
                  gPartyMemberUnitIdx[ct++] = j;
                  break;
               }
               pUnit++;
            }
         }
      }
      gPartyMemberUnitIdx[ct] = 0; //<- Null-terminator
      s_currentIdx_8012329c = 0;
      obj->state++;

   // fallthrough
   case 1:
      gSignal2 = 0;
      statusWindow = Obj_GetUnused();
      statusWindow->functionIndex = OBJF_STATUS_WINDOW;
      statusWindow->d.objf595.unit = &gUnits[gPartyMemberUnitIdx[s_currentIdx_8012329c]];
      if (gPartyMemberUnitIdx[1] != 0) {
         // Multiple party members are available to switch between.
         statusWindow->d.objf595.multiple = 1;
      }
      obj->state++;

   // fallthrough
   case 2:
      if (gSignal2 == 99) {
         gSignal2 = 100;
         obj->functionIndex = OBJF_NULL;
      }
      if (gSignal2 == 1) {
         // Switching left
         if (s_currentIdx_8012329c == 0) {
            // Already on first unit, so walk to last
            s_currentIdx_8012329c = 0;
            while (gPartyMemberUnitIdx[s_currentIdx_8012329c] != 0) {
               s_currentIdx_8012329c++;
            }
         }
         s_currentIdx_8012329c--;
         obj->state--;
      }
      if (gSignal2 == 2) {
         // Switching right
         if (gPartyMemberUnitIdx[s_currentIdx_8012329c + 1] == 0) {
            // Already on last unit, so loop back to first
            s_currentIdx_8012329c = 0;
         } else {
            s_currentIdx_8012329c++;
         }
         obj->state--;
      }
   }
}

#undef OBJF
#define OBJF 595
void Objf595_StatusWindow(Object *obj) {
   Object *portrait;
   Object *arrow;
   s16 size;
   s32 hasItems;
   UnitStatus *unit = OBJ.unit;

   switch (obj->state2) {
   case 0:
      obj->state2++;

   // fallthrough
   case 1:
      portrait = Obj_GetLastUnused();
      portrait->functionIndex = OBJF_STATUS_PORTRAIT;
      portrait->d.objf575.portraitId = gUnitPortraitIds[unit->unitId];
      UpdateSkillStatusWindow(unit);
      if (unit->team == TEAM_PLAYER) {
         DisplayBasicWindow(0x20);
      } else {
         DisplayCustomWindow(0x20, 0, 1, 0, 0, 23);
      }
      gWindowChoiceHeight = 18;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x3c;

      if (unit->items[0] != ITEM_NULL || unit->items[1] != ITEM_NULL) {
         hasItems = 1;
      } else {
         hasItems = 0;
      }

      if (unit->spells[0] == SPELL_NULL && !hasItems) {
         obj->state2 += 1;
      }
      if (unit->spells[0] != SPELL_NULL && !hasItems) {
         DrawWindow(0x3c, 316, 0, 48, 54, 242, 90, WBS_CROSSED, 2);
         DrawText(320, 11, 2, 2, 0, "\x94\x5c\x97\xcd\x96\x82\x96\x40");
         DisplayBasicWindow(0x3c);
         obj->state2 += 2;
      }
      if (unit->spells[0] == SPELL_NULL && hasItems) {
         DrawWindow(0x3c, 316, 0, 48, 54, 242, 90, WBS_CROSSED, 2);
         DrawText(320, 11, 2, 2, 0, "\x94\x5c\x97\xcd\x8e\x9d\x95\xa8");
         DisplayBasicWindow(0x3c);
         obj->state2 += 3;
      }
      if (unit->spells[0] != SPELL_NULL && hasItems) {
         DrawWindow(0x3c, 316, 0, 48, 72, 242, 90, WBS_CROSSED, 3);
         DrawText(320, 11, 2, 2, 0, "\x94\x5c\x97\xcd\x96\x82\x96\x40\x8e\x9d\x95\xa8");
         DisplayBasicWindow(0x3c);
         obj->state2 += 4;
      }

      obj->mem = 0;
      break;

   case 2:
      break;

   case 3:

      switch (obj->mem) {
      case 0:
         if (GetWindowChoice(0x3c) == 2) {
            CloseWindow(0x20);
            ClearIcons();
            Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
            DisplaySpellStatusWindow(unit, 0x3d);
            obj->mem++;
         }
         break;
      case 1:
         if (GetWindowChoice(0x3c) == 1) {
            ClearIcons();
            CloseWindow(0x3d);
            obj->state2 = 1;
         }
         break;
      }

      break;

   case 4:

      switch (obj->mem) {
      case 0:
         if (GetWindowChoice(0x3c) == 2) {
            CloseWindow(0x20);
            ClearIcons();
            Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
            DisplayItemsStatusWindow(unit, 0x3d);
            obj->mem++;
         }
         break;
      case 1:
         if (GetWindowChoice(0x3c) == 1) {
            ClearIcons();
            CloseWindow(0x3d);
            obj->state2 = 1;
         }
         break;
      }

   // fallthrough
   case 5:

      switch (obj->mem) {
      case 0:
         if (GetWindowChoice(0x3c) == 2) {
            CloseWindow(0x20);
            Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
            ClearIcons();
            DisplaySpellStatusWindow(unit, 0x3d);
            obj->mem++;
         }
         if (GetWindowChoice(0x3c) == 3) {
            CloseWindow(0x20);
            Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
            ClearIcons();
            DisplayItemsStatusWindow(unit, 0x3d);
            obj->mem += 2;
         }
         break;

      case 1:
         if (GetWindowChoice(0x3c) == 1) {
            ClearIcons();
            CloseWindow(0x3d);
            obj->state2 = 1;
         }
         if (GetWindowChoice(0x3c) == 3) {
            DisplayItemsStatusWindow(unit, 0x3d);
            obj->mem++;
         }
         break;

      case 2:
         if (GetWindowChoice(0x3c) == 2) {
            ClearIcons();
            DisplaySpellStatusWindow(unit, 0x3d);
            obj->mem--;
         }
         if (GetWindowChoice(0x3c) == 1) {
            ClearIcons();
            CloseWindow(0x3d);
            obj->state2 = 1;
         }
         break;
      }

      break;
   }

   if (gPadStateNewPresses & PAD_X) {
      CloseWindow(0x20);
      Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
      ClearIcons();
      CloseWindow(0x3c);
      CloseWindow(0x3d);
      gSignal2 = 99;
      obj->functionIndex = OBJF_NULL;
   }
   if (OBJ.multiple) {
      // Pulsating selection arrows
      size = (rcos(gOscillation) * 3 >> 12) + 10;
      arrow = Obj_GetUnused();
      arrow->x1.n = 20 + size;
      arrow->x3.n = 20 - size;
      arrow->y1.n = 136 - size;
      arrow->y3.n = 136 + size;
      arrow->d.sprite.gfxIdx = GFX_SELECTION_ARROW;
      AddObjPrim_Gui(gGraphicsPtr->ot, arrow);
      arrow->x1.n = 300 - size;
      arrow->x3.n = 300 + size;
      AddObjPrim_Gui(gGraphicsPtr->ot, arrow);
      if (gPadStateNewPresses & PAD_LEFT) {
         CloseWindow(0x20);
         Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
         ClearIcons();
         CloseWindow(0x3c);
         CloseWindow(0x3d);
         gSignal2 = 1;
         obj->functionIndex = OBJF_NULL;
      }
      if (gPadStateNewPresses & PAD_RIGHT) {
         CloseWindow(0x20);
         Obj_ResetByFunction(OBJF_STATUS_PORTRAIT);
         ClearIcons();
         CloseWindow(0x3c);
         CloseWindow(0x3d);
         gSignal2 = 2;
         obj->functionIndex = OBJF_NULL;
      }
   }
}
