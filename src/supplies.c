#include "common.h"
#include "object.h"
#include "graphics.h"
#include "state.h"
#include "cd_files.h"
#include "units.h"
#include "window.h"

#ifdef PC_FEAT
/* Language packs (platform/pc/src/pc_lang.c): text held in the sPartyNames array below cannot be wrapped at the
 * array itself -- a static initializer needs a compile-time constant. It is wrapped where the array
 * is READ instead: PC_LangStr hashes whatever string it is handed at run time, so one wrap covers
 * every entry in every array. Transparent in the matching build. */
extern u8 *PC_LangStr(const char *lit);
#define PC_LANGSTR(s) ((u8 *)PC_LangStr((const char *)(s)))
#else
#define PC_LANGSTR(s) (s)
#endif

static u8 *sPartyNames[15] = {
    "\x83\x5f\x83\x7e\x81\x5b",                         // Dummy
    "\x82\x60\x82\x93\x82\x88",                         // Ash
    "\x82\x62\x82\x8c\x82\x89\x82\x8e\x82\x94",         // Clint
    "\x82\x63\x82\x89\x82\x85\x82\x87\x82\x8f",         // Diego
    "\x82\x64\x82\x8c\x82\x85\x82\x8e\x82\x89",         // Eleni
    "\x82\x67\x82\x95\x82\x98\x82\x8c\x82\x85\x82\x99", // Huxley
    "\x82\x6a\x82\x89\x82\x92\x82\x81",                 // Kira
    "\x82\x66\x82\x92\x82\x8f\x82\x87",                 // Grog
    "\x82\x63\x82\x8f\x82\x8c\x82\x81\x82\x8e",         // Dolan
    "\x82\x60\x82\x8d\x82\x8f\x82\x8e",                 // Amon
    "\x82\x72\x82\x81\x82\x92\x82\x81",                 // Sara
    "\x82\x79\x82\x8f\x82\x88\x82\x81\x82\x92",         // Zohar
    "\x82\x63\x82\x81\x82\x92\x82\x89\x82\x95\x82\x93", // Darius
    "\x82\x63\x82\x85\x82\x90\x82\x8f\x82\x94",         // Depot (13)
    NULL,
};

// SJIS text of the visible portion of the inventory list
static u8 sInventoryBuffer[] =
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"
    "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81"
    "\x40\x81\x40\x81\x40\x81\x40";

static u8 sGoldBuffer[] = "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x82\x66";

// For displaying equipment stat differences (AT +, etc.)
static u8 sStatChangeBuffer[] = "\0\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40";

// Template strings used as base for the stat-change buffer
static u8 sAttackUpStr[] = "\x82\x60\x82\x73\x81\x40\n"
                           "\x81\x7b\x81\x40\x81\x40";

static u8 sAttackDownStr[] = "\x82\x60\x82\x73\x81\x40\n"
                             "\x81\x7c\x81\x40\x81\x40";

static u8 sDefenseUpStr[] = "\x82\x63\x82\x65\x81\x40\n"
                            "\x81\x7b\x81\x40\x81\x40";

static u8 sDefenseDownStr[] = "\x82\x63\x82\x65\x81\x40\n"
                              "\x81\x7c\x81\x40\x81\x40";

static u8 sIncompatibleStr[] = "\x81\x40\x81\x40\x81\x40\n"
                               "\x81\x40\x81\x40\x81\x40";

// TODO: Rename? Cursor memory seems to be the common term.
// Cursor memory for scrolling lists
typedef struct MenuMem1 {
   u8 ofs; // (from top)
   u8 top;
} MenuMem1;

typedef struct MenuMem2 {
   s16 ofs;
   s16 top;
} MenuMem2;

// Cursor memory for each inventory category
extern MenuMem2 gMenuMem_ShopOrDepot[3];
extern MenuMem2 gMenuMem_SellingFromDepot[3];
// Cursor memory for each party member inventory
extern u8 gMenuMem_PartyInventory[PARTY_CT];
extern u8 gMenuMem_SellingFromParty[PARTY_CT];
// Cursor memory for selecting a party member
extern MenuMem1 gMenuMem_TransferFrom;
extern MenuMem1 gMenuMem_TransferTo;

extern s16 gScrollIndicatorX;
extern s16 gScrollIndicatorY;
extern s16 *gDepotInventoryPtr; // Expanded list of item indices
extern u8 gPartyMemberInventory[5];
extern u8 gShopInventories[10][10][20]; // [shopId][category][item]
extern u8 *gItemDescriptions2[101];

// TODO
extern s32 D_801F6D40; // gold as displayed last
extern u8 D_801F6D44;  // timer
extern s16 D_8012338C; // wnd
extern u8 D_801F6D88;  // is equipped
extern u8 D_801F6D8C;  // transferring from depot
extern u8 D_801F6D90;  // choice (slot+1) of party member item to transfer
extern u8 D_801F6D94;  // item to transfer
extern u8 D_801F6D98;  // partyIdx of transfer src

void RenderScrollIndicator(u8 downward, s16 x, s16 y) {
   s16 halfSize;
   Object *sprite;

   halfSize = 7 + (rcos(gOscillation) * 3 >> 12);
   sprite = Obj_GetUnused();
   sprite->x1.n = x + halfSize;
   sprite->x3.n = x - halfSize;
   if (!downward) {
      sprite->y1.n = y - halfSize;
      sprite->y3.n = y + halfSize;
   } else {
      sprite->y1.n = y + halfSize;
      sprite->y3.n = y - halfSize;
   }
   sprite->d.sprite.otOfs = 5;
   sprite->d.sprite.gfxIdx = GFX_SCROLL_INDICATOR;
   AddObjPrim_Gui(gGraphicsPtr->ot, sprite);
}

s32 PositionScrollIndicator(s32 windowId, u8 downward) {
   s32 i;
   Object *p;
   Object *window;

   p = &gObjectArray[0];

   for (i = 0; i < OBJ_DATA_CT; i++, p++) {
      if ((p->functionIndex == OBJF_WINDOW_TBD_004 || p->functionIndex == OBJF_WINDOW_TBD_005) &&
          p->d.objf004.windowId == windowId && p->state == 2) {
         window = p->d.objf004.window;
         gScrollIndicatorX = window->d.sprite2.coords[0].x +
                             (window->d.sprite2.coords[1].x - window->d.sprite2.coords[0].x) / 2;
         if (!downward) {
            gScrollIndicatorY = window->d.sprite2.coords[0].y;
         } else {
            gScrollIndicatorY = window->d.sprite2.coords[2].y;
         }
         return 1;
      }
   }

   return 0;
}

void State_ShopOrDepot(void) {
   // Per town state
   static u8 townShopIds[36] = {0, 0, 0, 0, 0, 0, 1, 1, 1, 1, 1, 2, 2, 3, 3, 3, 3, 4,
                                5, 5, 6, 6, 6, 6, 6, 7, 7, 8, 8, 8, 8, 8, 9, 0, 0, 0};

   s32 cdFile;
   s32 i;
   Object *iconMgr;

   switch (gState.secondary) {
   case 0:
      Obj_ResetFromIdx10();
      gDepotInventoryPtr = (s16 *)gScratch1_801317c0;
      Obj_ResetFromIdx10();
      gState.vsyncMode = 0;
      gClearSavedPadState = 1;
      gState.fieldRenderingDisabled = 1;

      if (gState.townState >= 32) {
         cdFile = CDF_SP_GRSGO_TIM;
      } else if (gState.townState >= 27) {
         cdFile = CDF_SP_HNKYO_TIM;
      } else if (gState.townState >= 25) {
         cdFile = CDF_SP_SORBO_TIM;
      } else if (gState.townState >= 20) {
         cdFile = CDF_SP_KRCH_TIM;
      } else if (gState.townState >= 18) {
         cdFile = CDF_SP_KNRTH_TIM;
      } else if (gState.townState >= 17) {
         cdFile = CDF_SP_TESTA_TIM;
      } else if (gState.townState >= 13) {
         cdFile = CDF_SP_JIGEN_TIM;
      } else if (gState.townState >= 11) {
         cdFile = CDF_SP_USU_TIM;
      } else if (gState.townState >= 6) {
         cdFile = CDF_SP_PORTS_TIM;
      } else {
         cdFile = CDF_SP_SYM_TIM;
      }

      if (gState.primary != STATE_SHOP) {
         // Depot
         cdFile = CDF_D_EXCHG_TIM;
      }

      LoadFullscreenImage(cdFile);
      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;

      LoadText(CDF_SHOP_T_DAT, gText, gTextPointers);
      gState.currentTextPointers = &gTextPointers[1];

      for (i = 0; i < 7; i++) {
         gTempObj = Obj_GetLastUnused();
         gTempObj->functionIndex = OBJF_DYNAMIC_ICON;
         gTempObj->d.objf577.idx = i;
      }

      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_SHOP_OR_DEPOT;
      gTempObj->d.objf406.shopId = townShopIds[gState.townState];

      iconMgr = Obj_GetLastUnused();
      iconMgr->functionIndex = OBJF_ITEM_ICON_MGR;
      iconMgr->d.objf009.shopOrDepot = gTempObj;
      gState.secondary++;
      break;

   case 1:
      break;
   }
}

void ListDepotInventory(u8 category, s16 top, u8 rows, s32 unused, u8 showCosts) {
   s32 i, j, k;
   s32 n;
   u8 *pDst;
   u8 *pSrc;
   s16 cost;

   k = 0;
   switch (category) {
   case INVENTORY_WEAPON:
      for (i = 1; i < DEPOT_CT; i++) {
         n = gState.depot[i];
         if (n != 0 && gItemEquipmentTypes[i] < EQUIPMENT_TYPE_HELM) {
            // Expand to a list of n separate entries of item i
            for (j = 0; j < n; j++) {
               gDepotInventoryPtr[k++] = i;
            }
         }
      }
      for (i = 0; i < 8; i++) {
         // Clear any junk values within visible range
         gDepotInventoryPtr[k++] = ITEM_NULL;
      }
      break;

   case INVENTORY_ARMOR:
      for (i = 1; i < DEPOT_CT; i++) {
         n = gState.depot[i];
         if (n != 0 && gItemEquipmentTypes[i] >= EQUIPMENT_TYPE_HELM &&
             gItemEquipmentTypes[i] != EQUIPMENT_TYPE_ITEM) {
            for (j = 0; j < n; j++) {
               gDepotInventoryPtr[k++] = i;
            }
         }
      }
      for (i = 0; i < 8; i++) {
         gDepotInventoryPtr[k++] = ITEM_NULL;
      }
      break;

   case INVENTORY_ITEM:
      for (i = 1; i < DEPOT_CT; i++) {
         n = gState.depot[i];
         if (n != 0 && gItemEquipmentTypes[i] == EQUIPMENT_TYPE_ITEM) {
            for (j = 0; j < n; j++) {
               gDepotInventoryPtr[k++] = i;
            }
         }
      }
      for (i = 0; i < 8; i++) {
         gDepotInventoryPtr[k++] = ITEM_NULL;
      }
      break;
   }

   pDst = &sInventoryBuffer[0];

   for (k = 0; k < rows; k++) {
      pSrc = &gItemNamesSjis[gDepotInventoryPtr[top + k]][0];
      for (i = 0; i < 8; i++) {
         *pDst++ = *pSrc++;
         *pDst++ = *pSrc++;
      }
      for (i = 0; i < 5; i++) {
         // SJIS 8140: Space
         *pDst++ = 0x81;
         *pDst++ = 0x40;
      }
      *pDst++ = '\n';
   }

   *pDst = '\0';

   for (k = 0; k < rows; k++) {
      j = 0; // Number of printed digits; tracked to allow printing of non-leading zeroes
      pDst = &sInventoryBuffer[k * 27 + 18];
      cost = gItemCosts[gDepotInventoryPtr[top + k]] / 2;
      if (cost != 0 && showCosts) {
         if (cost >= 1000) {
            n = cost / 1000;
            cost %= 1000;
            // SJIS 824F: Zero
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 100 || j != 0) {
            n = cost / 100;
            cost %= 100;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 10 || j != 0) {
            n = cost / 10;
            cost %= 10;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         // Final digit
         *pDst++ = 0x82;
         *pDst++ = 0x4f + cost;
      }
   }
}

void ListPartyMemberInventory(u8 partyIdx, u8 showCosts) {
   s32 i, j, k;
   u8 *pDst;
   u8 *pSrc;
   s16 cost;
   s32 n;

   for (i = 0; i < ARRAY_COUNT(gPartyMemberInventory); i++) {
      gPartyMemberInventory[i] = ITEM_NULL;
   }
   i = 0;
   gPartyMemberInventory[i++] = gPartyMembers[partyIdx].weapon;
   gPartyMemberInventory[i++] = gPartyMembers[partyIdx].helmet;
   gPartyMemberInventory[i++] = gPartyMembers[partyIdx].armor;
   if (gPartyMembers[partyIdx].items[0] != ITEM_NULL) {
      gPartyMemberInventory[i++] = gPartyMembers[partyIdx].items[0];
   }
   gPartyMemberInventory[i++] = gPartyMembers[partyIdx].items[1];

   pDst = &sInventoryBuffer[0];

   for (k = 0; k < 5; k++) {
      // TODO: This generates a jtbl lw that confuses the consistency check in fixup.py
      switch (k) {
      case 0:
         // SJIS 8194: #
         *pDst++ = 0x81;
         *pDst++ = 0x94;
         break;
      case 1:
         *pDst++ = 0x81;
         *pDst++ = 0x94;
         break;
      case 2:
         *pDst++ = 0x81;
         *pDst++ = 0x94;
         break;
      case 3:
      case 4:
         *pDst++ = 0x81;
         *pDst++ = 0x40;
         break;
      }

      pSrc = &gItemNamesSjis[gPartyMemberInventory[k]][0];
      for (i = 0; i < 8; i++) {
         *pDst++ = *pSrc++;
         *pDst++ = *pSrc++;
      }
      for (i = 0; i < 4; i++) {
         *pDst++ = 0x81;
         *pDst++ = 0x40;
      }
      *pDst++ = '\n';
   }

   *pDst = '\0';

   for (k = 0; k < 5; k++) {
      j = 0;
      pDst = &sInventoryBuffer[k * 27 + 18];
      cost = gItemCosts[gPartyMemberInventory[k]] / 2;
      if (cost != 0 && showCosts) {
         if (cost >= 1000) {
            n = cost / 1000;
            cost %= 1000;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 100 || j != 0) {
            n = cost / 100;
            cost %= 100;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 10 || j != 0) {
            n = cost / 10;
            cost %= 10;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         // Final digit
         *pDst++ = 0x82;
         *pDst++ = 0x4f + cost;
      }
   }
}

void UpdateStatChangeText(u8 item, u8 partyIdx) {
   u8 *pDst;
   u8 *pSrc;
   s32 i;
   u8 slot;
   s16 diff;

   diff = 0;
   pDst = &sStatChangeBuffer[0];

   if (gItemEquipmentTypes[item] >= EQUIPMENT_TYPE_HELM) {
      slot = INVENTORY_SLOT_HELMET;
   } else {
      slot = INVENTORY_SLOT_WEAPON;
   }
   if (gItemEquipmentTypes[item] >= EQUIPMENT_TYPE_ARMOR) {
      slot = INVENTORY_SLOT_ARMOR;
   }

   switch (slot) {
   case INVENTORY_SLOT_WEAPON:
      diff = gItemEquipmentDisplayPower[item] -
             gItemEquipmentDisplayPower[gPartyMembers[partyIdx].weapon];
      pSrc = &sAttackUpStr[0];
      if (diff < 0) {
         diff = -diff;
         pSrc = &sAttackDownStr[0];
      }
      break;

   case INVENTORY_SLOT_HELMET:
      diff = gItemEquipmentDisplayPower[item] -
             gItemEquipmentDisplayPower[gPartyMembers[partyIdx].helmet];
      pSrc = &sDefenseUpStr[0];
      if (diff < 0) {
         diff = -diff;
         pSrc = &sDefenseDownStr[0];
      }
      break;

   case INVENTORY_SLOT_ARMOR:
      diff = gItemEquipmentDisplayPower[item] -
             gItemEquipmentDisplayPower[gPartyMembers[partyIdx].armor];
      pSrc = &sDefenseUpStr[0];
      if (diff < 0) {
         diff = -diff;
         pSrc = &sDefenseDownStr[0];
      }
      break;
   }

   if (!gEquipmentTypeClassCapability[gItemEquipmentTypes[item]][gUnits[partyIdx].class]) {
      pSrc = &sIncompatibleStr[0];
      for (i = 0; i < 13; i++) {
         *pDst++ = *pSrc++;
      }
   } else {
      for (i = 0; i < 13; i++) {
         *pDst++ = *pSrc++;
      }
      EmbedIntAsSjis(diff, &sStatChangeBuffer[9], 2);
   }
   sStatChangeBuffer[13] = '\0';
}

#ifdef PC_FEAT
/* --- ITEM-NAME WIDTH: 1-byte / 16-char item lists (exchange/91) ---------------------------------
 * A format-2 pack (PC_LangItemNames1Byte()) stores item names as 16 one-byte chars instead of 8
 * two-byte SJIS ones. They must draw through the small 8x9 font, not the wide SJIS one, and PER ROW
 * at the choice pitch (the small font's own ~9 px advance desyncs from the 17 px choice rows). The
 * name draws LEFT; the price is RIGHT-ALIGNED so a long name never pushes it off the widened window. */
extern int PC_LangItemNames1Byte(void);
#define ITEM1B_ROW_H  17
#define ITEM1B_COLS   20     /* name (<=16) + right-aligned price. 20 cols keeps the price inside the
                              * NARROWER Sell/Depot list boxes too (not just the widened Buy box), so it
                              * never rides the border. A long name yields its right columns to the
                              * price (exchange/91, feedback-35/36). */
#define ITEM1B_NAME_TIGHT 11 /* char cap for the purchase/sell/transfer confirm box -- it is sized for
                              * 8 SJIS chars AND carries an item icon on the left, so it fits ~11 small
                              * chars before the name rides the border (feedback-36). */
#define GOLD1B_X      233    /* gold box screen x (feedback-35: shifted 1 cell left of 241, box grown
                              * the same 8 px so its right edge stays put) */
#define GOLD1B_W      80     /* 10 cells @ 8 px: holds max gold "990000G" (7 small chars) with room. */
/* Compose one FIXED-WIDTH list row -- 16-char name (space-padded) then, when priced, a right-aligned
 * price in an 8-col field. Fixed width matters: a scroll redraw draws spaces over the previous row's
 * cells, so a shorter new name leaves no leftover tail (ASCII space -> blank glyph, which clears).
 * showPrice==0 or a 0 cost (an empty slot) leaves the price blank, matching the SJIS path. */
static void ItemRow1Byte(u8 *row, const u8 *name, s16 cost, s32 showPrice) {
   s32 i, done = 0, m = 0;
   char tmp[8];
   for (i = 0; i < 16; i++) { if (!done && name[i] == '\0') done = 1; row[i] = done ? ' ' : name[i]; }
   for (i = 16; i < ITEM1B_COLS; i++) row[i] = ' ';
   row[ITEM1B_COLS] = '\0';
   if (showPrice && cost > 0) {
      while (cost) { tmp[m++] = (char)('0' + cost % 10); cost /= 10; }         /* digits, reversed */
      for (i = 0; i < m && i < 8; i++) row[ITEM1B_COLS - 1 - i] = (u8)tmp[i];  /* right-align */
   }
}
/* Draw one list row at the choice pitch (28, 110 + row*17). Callers differ only in how they source
 * the item id and cost: shop = full price, depot/sell = half price, view = no price. */
static void DrawItemRow1Byte(s32 rowIdx, u8 item, s32 showPrice, s32 halfCost) {
   u8 row[ITEM1B_COLS + 1];
   s16 cost = (s16)(halfCost ? gItemCosts[item] / 2 : gItemCosts[item]);
   ItemRow1Byte(row, gItemNamesSjis[item], cost, showPrice);
   DrawText(28, (s16)(110 + rowIdx * ITEM1B_ROW_H), ITEM1B_COLS + 1, 0, 0, row);
}
static void DrawShopList1Byte(u8 shopId, u8 category, u8 top, u8 rows) {
   s32 k;
   for (k = 0; k < rows; k++)
      DrawItemRow1Byte(k, gShopInventories[shopId][category][top + k], 1, 0);
}
static void DrawDepotList1Byte(s16 top, u8 rows, s32 showPrice) {  /* ids are s16 in gDepotInventoryPtr */
   s32 k;
   for (k = 0; k < rows; k++)
      DrawItemRow1Byte(k, (u8)gDepotInventoryPtr[top + k], showPrice, 1);
}
static void DrawPartyItemList1Byte(s32 showPrice) {  /* the 5 equipped/carried slots */
   s32 k;
   for (k = 0; k < 5; k++)
      DrawItemRow1Byte(k, gPartyMemberInventory[k], showPrice, 1);
}
/* One item name in the small font (confirm boxes) -- gItemNamesSjis holds plain ASCII in a 1-byte
 * pack. Hard-truncate to maxChars (never wrap) so a long name clips at the box edge instead of
 * spilling outside or dropping onto a second line. */
static void DrawItemName1Byte(s16 x, s16 y, s32 color, u8 item, s32 maxChars) {
   u8 buf[17];
   const u8 *name = gItemNamesSjis[item];
   s32 i;
   if (maxChars > 16) maxChars = 16;
   for (i = 0; i < maxChars && name[i] != '\0'; i++) buf[i] = name[i];
   buf[i] = '\0';
   DrawText(x, y, maxChars + 1, 0, color, buf);
}
/* The gold total in the small 8x9 font (ASCII), right-aligned inside the narrow box the widened list
 * needs. Right-align so short totals sit against the same edge as "990000G" instead of drifting. */
static void DrawGold1Byte(s32 gold) {
   u8 buf[12];
   char tmp[10];
   s32 n = 0, m = 0, i;
   if (gold <= 0) tmp[m++] = '0';
   else { s32 g = gold; while (g) { tmp[m++] = (char)('0' + g % 10); g /= 10; } }
   for (i = 0; i < m; i++) buf[n++] = (u8)tmp[m - 1 - i];
   buf[n++] = 'G';
   buf[n] = '\0';
   /* right-align hard against the border (feedback-36: nudged the last few px right), backing off n
    * glyphs (8 px each) -- keeps the amount against the right border instead of drifting left. */
   DrawText((s16)(GOLD1B_X + GOLD1B_W + 2 - n * 8), 109, n + 1, 0, 0, buf);
}
#endif

void ListShopInventory(u8 shopId, u8 category, u8 top, u8 rows, s32 unused) {
   u8 *pDst;
   u8 *pSrc;
   s32 i, j, k;
   s16 cost;
   s32 n;

   pDst = &sInventoryBuffer[0];

   for (k = 0; k < rows; k++) {
      pSrc = &gItemNamesSjis[gShopInventories[shopId][category][top + k]][0];
      for (i = 0; i < 8; i++) {
         *pDst++ = *pSrc++;
         *pDst++ = *pSrc++;
      }
      for (i = 0; i < 5; i++) {
         *pDst++ = 0x81;
         *pDst++ = 0x40;
      }
      *pDst++ = '\n';
   }

   *pDst = '\0';

   for (k = 0; k < rows; k++) {
      j = 0;
      pDst = &sInventoryBuffer[k * 27 + 18];
      cost = gItemCosts[gShopInventories[shopId][category][top + k]];
      if (cost != 0) {
         if (cost >= 1000) {
            n = cost / 1000;
            cost %= 1000;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 100 || j != 0) {
            n = cost / 100;
            cost %= 100;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         if (cost >= 10 || j != 0) {
            n = cost / 10;
            cost %= 10;
            *pDst++ = 0x82;
            *pDst++ = 0x4f + n;
            j++;
         } else {
            *pDst++ = 0x81;
            *pDst++ = 0x40;
         }
         // Final digit
         *pDst++ = 0x82;
         *pDst++ = 0x4f + cost;
      }
   }
}

void ListParty(u8 top, u8 rows, s32 unused) {
   u8 *pDst;
   u8 *pSrc;
   s32 i, j;

   pDst = &gPartyListBuffer[0];

   for (i = 0; i < rows; i++) {
      for (j = 0; j < 6; j++) {
         *pDst++ = 0x81;
         *pDst++ = 0x40;
      }
      pDst -= 12;
      pSrc = PC_LANGSTR(sPartyNames[gCurrentParty[top + i]]);
      for (j = 0; j < 6; j++) {
         if (*pSrc != '\0') {
            *pDst++ = *pSrc++;
            *pDst++ = *pSrc++;
         }
      }
      *pDst++ = '\n';
   }

   *pDst = '\0';
}

#undef OBJF
#define OBJF 406
void Objf406_ShopOrDepot(Object *obj) {
   s32 i, j;
   Object *obj_v1;
   s16 newEquipment;

   gClearSavedPadState = 1;
   if (gWindowActiveIdx == 0x38 && GetWindowChoice2(0x38) != 0) {
      gState.choices[0] = GetWindowChoice2(0x38) - 1;
   }
   if (gWindowActiveIdx == 0x39 && GetWindowChoice2(0x39) != 0) {
      gState.choices[1] = GetWindowChoice2(0x39) - 1;
   }
   if (gWindowActiveIdx == 0x3a && GetWindowChoice2(0x3a) != 0) {
      gState.choices[2] = GetWindowChoice2(0x3a) - 1;
   }
   if (gWindowActiveIdx == 0x3b && GetWindowChoice2(0x3b) != 0) {
      gState.choices[3] = GetWindowChoice2(0x3b) - 1;
   }
   if (gWindowActiveIdx == 0x3f && GetWindowChoice2(0x3f) != 0) {
      gState.choices[4] = GetWindowChoice2(0x3f) - 1;
   }
   if (gWindowActiveIdx == 0x40 && GetWindowChoice2(0x40) != 0) {
      gState.choices[5] = GetWindowChoice2(0x40) - 1;
   }
   if (gState.gold > 990000) {
      gState.gold = 990000;
   }

   switch (obj->state) {
   case 0:
      gMenuMem_TransferFrom.ofs = 0;
      gMenuMem_TransferFrom.top = 0;
      gMenuMem_TransferTo.ofs = 0;
      gMenuMem_TransferTo.top = 0;
      for (i = 0; i < PARTY_CT; i++) {
         gMenuMem_PartyInventory[i] = 0;
         gMenuMem_SellingFromParty[i] = 0;
      }
      for (i = 0; i < 3; i++) {
         gMenuMem_ShopOrDepot[i].ofs = 0;
         gMenuMem_ShopOrDepot[i].top = 0;
         gMenuMem_SellingFromDepot[i].ofs = 0;
         gMenuMem_SellingFromDepot[i].top = 0;
      }
      gState.choices[0] = 0;
      gState.choices[1] = 0;
      gState.choices[2] = 0;
      gState.choices[3] = 0;
      gState.choices[4] = 0;
      gState.choices[5] = 0;
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      FadeInScreen(2, 10);
      D_801F6D44 = 30;
      obj->state++;

   // fallthrough
   case 1:
      if (--D_801F6D44 == 0) {
         j = 0;
         for (i = 1; i < PARTY_CT; i++) {
            if (gPartyMembers[i].inParty) {
               gCurrentParty[j++] = i;
            }
         }
         gCurrentParty[j++] = 13; // Add an entry for Depot
         gCurrentParty[j] = 0;    // Null terminator

         i = 0;
         while (gCurrentParty[i] != 0) {
            i++;
         }
         if (i > 7) {
            i = 7;
         }
         OBJ.partyRows = i;
         DisplayBasicWindow(0x01);

         obj_v1 = Obj_GetLastUnused();
         obj_v1->functionIndex = OBJF_UNIT_PORTRAIT;
         obj_v1->d.objf447.windowId = 0x3e;
         DrawWindow(0x3e, 400, 0, 64, 63, 210, 280, WBS_CROSSED, 0);
         DisplayCustomWindow(0x3e, 2, 1, 20, 0, 0);
         obj->state = 2;
         obj->state2 = 0;

         if (gState.primary != STATE_SHOP) {
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DisplayCustomWindow(0x34, 0, 1, 50, 0, 0);
            DisplayCustomWindow(0x35, 0, 1, 50, 0, 0);
            obj->state = 50;
            obj->state2 = 0;
         }
      }
      break;

   case 2:
      // "Welcome. What can I do for you?"
      DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
      DisplayCustomWindow(0x34, 0, 1, 50, 0, 0);
      DisplayCustomWindow(0x35, 0, 1, 50, 0, 0);
      SlideWindowTo(0x34, 4, 10);
      SlideWindowTo(0x35, 252, 10);
      SetupTownMsgBox(gState.portraitsToLoad[25], 0);
      DrawText(68, 20, 30, 2, 0, gTextPointers[1]);
      obj->state = 3;
      obj->state2 = 0;
      break;

   case 3:

      switch (obj->state2) {
      case 0:
         // Buy/Sell/Exit
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x38, 320, 0, 72, 72, 110, 110, WBS_CROSSED, 3);
         DrawText(324, 11, 10, 2, 0, gTextPointers[2]);
         gWindowActiveIdx = 0x38;
         DisplayCustomWindowWithSetChoice(0x38, 5, 1, 40, 0, 0, gState.choices[0]);
         obj->state2 += 2;
         break;

      case 1:
         gWindowActiveIdx = 0x38;
         SlideWindowTo(0x38, 110, 110);
         obj->state2++;
         break;

      case 2:
         if (gWindowChoice.raw == 0x3801) {
            // Buy
            SlideWindowTo(0x38, 12, 90);
            obj->state = 4;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3802) {
            // Sell
            SlideWindowTo(0x38, 12, 90);
            obj->state = 30;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3803 || gWindowChoice.raw == 0x38ff) {
            // Exit
            CloseWindow(0x38);
            obj->state = 99;
            obj->state2 = 0;
         }
      }

      break;

   case 4:

      switch (obj->state2) {
      case 0:
         // Weapon/Armor/Item/Quit
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x39, 400, 0, 80, 90, 400, 110, WBS_CROSSED, 4);
         DrawText(404, 11, 10, 2, 0, gTextPointers[3]);
         DisplayCustomWindowWithSetChoice(0x39, 2, 1, 30, 0, 0, gState.choices[1]);
         SlideWindowTo(0x39, 110, 110);
         obj->state2++;

      // fallthrough
      case 1:
         // "What sort of thing are you looking for?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[4]);
         obj->state2++;

      // fallthrough
      case 2:
         gWindowActiveIdx = 0x39;
         if (gWindowChoice.raw == 0x3901) {
            OBJ.category = INVENTORY_WEAPON;
            OBJ.shopRows = 7;
            OBJ.shopTop = gMenuMem_ShopOrDepot[INVENTORY_WEAPON].top;
            obj->state = 6;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3902) {
            OBJ.category = INVENTORY_ARMOR;
            OBJ.shopRows = 7;
            OBJ.shopTop = gMenuMem_ShopOrDepot[INVENTORY_ARMOR].top;
            obj->state = 6;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3903) {
            OBJ.category = INVENTORY_ITEM;
            OBJ.shopRows = 7;
            OBJ.shopTop = gMenuMem_ShopOrDepot[INVENTORY_ITEM].top;
            obj->state = 6;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3904 || gWindowChoice.raw == 0x39ff) {
            SlideWindowTo(0x39, 325, 110);
            CloseWindow(0x39);
            obj->state = 5;
            obj->state2 = 0;
         }
      }

      break;

   case 5:
      // "Let me know if you need anything else."
      DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
      DrawText(68, 20, 30, 2, 0, gTextPointers[5]);
      obj->state = 3;
      obj->state2 = 1;
      break;

   case 6:

      switch (obj->state2) {
      case 0:
         // "Do you see anything that interests you?"
         SlideWindowTo(0x39, 460, 110);
         CloseWindow(0x39);
         SlideWindowTo(0x38, -100, 90);
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[6]);
         obj->state2++;

      // fallthrough
      case 1:
         gState.choices[2] = gMenuMem_ShopOrDepot[OBJ.category].ofs;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
#ifdef PC_FEAT
         /* 1-byte item names span wider; give the list box one extra cell (208) so the right-aligned
          * price hugs the border. Retail / non-langpack keeps the original 200. */
         DrawWindow(0x3a, 0, 100, PC_LangItemNames1Byte() ? 208 : 200, 136, 84, 250, WBS_CROSSED,
                    OBJ.shopRows);
         if (PC_LangItemNames1Byte()) {
            DrawShopList1Byte(OBJ.shopId, OBJ.category, OBJ.shopTop, OBJ.shopRows);
         } else
#else
         DrawWindow(0x3a, 0, 100, 200, 136, 84, 250, WBS_CROSSED, OBJ.shopRows);
#endif
         {
            ListShopInventory(OBJ.shopId, OBJ.category, OBJ.shopTop, OBJ.shopRows, 0);
            DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         }
         DisplayCustomWindowWithSetChoice(0x3a, 5, 1, 50, 1, 0, gState.choices[2]);
         obj->state2++;
         break;

      case 2:
         SlideWindowTo(0x3e, 210, 250);
         SlideWindowTo(0x01, 350, 90);
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 3:
         gMenuMem_ShopOrDepot[OBJ.category].ofs = gState.choices[2];
         gMenuMem_ShopOrDepot[OBJ.category].top = OBJ.shopTop;
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gShopInventories[OBJ.shopId][OBJ.category][OBJ.shopTop + gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 29, 2, 0, gItemDescriptions2[i]);
         }
         gWindowActiveIdx = 0x3a;
         D_8012338C = 0x3a;
         SlideWindowTo(0x3a, 10, 90);
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x3a, 84, 250);
            CloseWindow(0x3a);
            gWindowActiveIdx = 0;
            D_8012338C = 0;
            obj->state = 4;
            obj->state2 = 0;
            SlideWindowTo(0x38, 12, 90);
         } else if (gWindowChoice.s.windowId == 0x3a && gWindowChoice.s.choice != 0) {
            OBJ.itemToPurchase =
                gShopInventories[OBJ.shopId][OBJ.category][OBJ.shopTop + gHighlightedChoice - 1];
            if (OBJ.itemToPurchase != ITEM_NULL) {
               gWindowActiveIdx = 0;
               obj->state = 7;
               obj->state2 = 0;
            }
         }
         break;
      }

      break;

   case 7:

      switch (obj->state2) {
      case 0:
         // "Would you like to purchase this?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[7]);
         // YES/NO
         gWindowActiveIdx = 0x37;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[8]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            // No
            CloseWindow(0x37);
            gWindowActiveIdx = 0;
            obj->state = 8;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3701) {
            // Yes
            CloseWindow(0x37);
            gWindowActiveIdx = 0;
            if (gItemCosts[OBJ.itemToPurchase] > gState.gold) {
               obj->state2++;
            } else {
               obj->state = 9;
               obj->state2 = 0;
            }
         }
         break;

      case 2:
         // "You don't have enough money. Is there anything else I can get you?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[9]);
         obj->state = 6;
         obj->state2 = 2;
      }

      break;

   case 8:
      // "Too bad. Anything else?"
      DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
      DrawText(68, 20, 30, 2, 0, gTextPointers[10]);
      obj->state = 6;
      obj->state2 = 2;
      break;

   case 9:

      switch (obj->state2) {
      case 0:
         // "Thank you.  Who shall I give it to?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[11]);
         SlideWindowTo(0x3a, -210, 90);
         DrawWindow(0x3b, 412, 100, 96, 136, 330, 290, WBS_CROSSED, OBJ.partyRows);
         ListParty(OBJ.partyTop, OBJ.partyRows, 0);
         DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
         DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gState.choices[3]);
         SlideWindowTo(0x3b, 10, 90);
         OBJ.partyChoice = OBJ.partyTop + GetWindowChoice(0x3b);
         DrawSmallEquipmentWindow(gCurrentParty[OBJ.partyChoice - 1]);
         DrawWindow(0x3c, 256, 200, 128, 36, 108, 188, WBS_CROSSED, 0);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawItemName1Byte(272, 210, 0, OBJ.itemToPurchase, ITEM1B_NAME_TIGHT);
         else
#endif
         DrawSjisText(272, 210, 19, 2, 0, gItemNamesSjis[OBJ.itemToPurchase]);
         DisplayCustomWindow(0x3c, 2, 1, 50, 1, 0);
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DISPLAY_ICON;
         obj_v1->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + OBJ.itemToPurchase;
         obj_v1->x1.n = 116;
         obj_v1->y1.n = 198;
         obj->state2++;

      // fallthrough
      case 1:
         DrawWindow(0x3e, 400, 0, 64, 63, 0, 0, WBS_CROSSED, 0);
         DrawWindow(0x3d, 324, 140, 64, 54, 350, 170, WBS_CROSSED, 0);
         DrawSjisText(328, 151, 10, 2, 0, sStatChangeBuffer);
         DisplayCustomWindow(0x3d, 2, 1, 5, 0, 0);
         OBJ.partyIdx2 = -1;
         obj->state2++;

      // fallthrough
      case 2:
         i = GetWindowChoice(0x3b) + OBJ.partyTop;
         if (i != OBJ.partyChoice && i < 13) {
            OBJ.partyChoice = i;
            DrawSmallEquipmentWindow(gCurrentParty[i - 1]);
         }
         if (OBJ.category != INVENTORY_ITEM) {
            SlideWindowTo(0x3d, 240, 170);
         }
         i = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (OBJ.partyIdx2 != i && i < 13) {
            OBJ.partyIdx2 = i;
            DrawWindow(0x3d, 324, 140, 64, 54, 240, 170, WBS_CROSSED, 0);
            UpdateStatChangeText(OBJ.itemToPurchase, OBJ.partyIdx2);
            DrawSjisText(328, 151, 10, 2, 0, sStatChangeBuffer);
         }
         if (i >= 13) {
            // Depot
            SlideWindowTo(0x3d, 350, 170);
         }
#ifdef PC_PORT
         /* See the partyIdx2 >= 0 guard note in state 31 -- avoid the gUnits[-1] wild read on PC. */
         if (OBJ.partyIdx2 >= 0)
            gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#else
         gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#endif
         SlideWindowTo(0x3e, 210, 128);
         SlideWindowTo(1, 108, 90);
         if (i >= 13) {
            gState.unitListPortraitId = 0;
            SlideWindowTo(0x3e, 210, 250);
            SlideWindowTo(1, 350, 90);
         }
         gWindowActiveIdx = 0x3b;
         if (gWindowChoice.raw == 0x3bff) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            SlideWindowTo(0x3d, 350, 170);
            CloseWindow(0x3d);
            SlideWindowTo(0x3e, 210, 250);
            SlideWindowTo(1, 350, 90);
            obj->state = 8;
            obj->state2 = 0;
         }
         if (gWindowChoice.s.windowId == 0x3b && gWindowChoice.s.choice != 0) {
            OBJ.partyIdx1 = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
            if (OBJ.partyIdx1 == 13) {
               // Depot
               obj->state = 10;
               obj->state2 = 0;
            } else {
               if (OBJ.category < INVENTORY_ITEM) {
                  if (gEquipmentTypeClassCapability[gItemEquipmentTypes[OBJ.itemToPurchase]]
                                                   [gUnits[OBJ.partyIdx1].class] &&
                      !(OBJ.category == INVENTORY_WEAPON && OBJ.partyIdx1 == UNIT_ASH &&
                        gPartyMembers[UNIT_ASH].weapon == ITEM_V_HEART)) {
                     gWindowActiveIdx = 0;
                     obj->state = 12;
                     obj->state2 = 0;
                  } else {
                     obj->state = 13;
                     obj->state2 = 0;
                  }
                  SlideWindowTo(0x3d, 350, 170);
                  CloseWindow(0x3d);
                  SlideWindowTo(0x3e, 240, 168);
               }
               if (OBJ.category == INVENTORY_ITEM) {
                  if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL) {
                     gPartyMembers[OBJ.partyIdx1].items[0] = OBJ.itemToPurchase;
                     obj->state = 11;
                     obj->state2 = 0;
                  } else if (gPartyMembers[OBJ.partyIdx1].items[1] == ITEM_NULL) {
                     gPartyMembers[OBJ.partyIdx1].items[1] = OBJ.itemToPurchase;
                     obj->state = 11;
                     obj->state2 = 0;
                  } else {
                     // "#32's hands are full. Is there someone else I should give it to?"
                     gStringTable[32] = gStringTable[OBJ.partyIdx1];
                     DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
                     DrawText(68, 20, 30, 2, 0, gTextPointers[12]);
                  }
               }
            }
         }

         break;
      }

      break;

   case 10:

      switch (obj->state2) {
      case 0:
         // "Okay, I'll put it in your supply wagon."
         gState.gold -= gItemCosts[OBJ.itemToPurchase];
         gState.depot[OBJ.itemToPurchase]++;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[13]);
         gWindowActiveIdx = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            obj->state = 19;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 11:

      switch (obj->state2) {
      case 0:
         // "Okay, here you go. Thanks very much."
         gState.gold -= gItemCosts[OBJ.itemToPurchase];
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[14]);
         DrawSmallEquipmentWindow(OBJ.partyIdx1);
         gWindowActiveIdx = 0;
         obj->state2++;
         break;

      case 1:
         if (PressedCircleOrX_2()) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            obj->state = 19;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 12:

      switch (obj->state2) {
      case 0:
         // "Do you want to equip yourself with it now?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[15]);
         // YES/NO
         gWindowActiveIdx = 0x37;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[16]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3701) {
            // "It suits you very well."
            gState.gold -= gItemCosts[OBJ.itemToPurchase];
            CloseWindow(0x37);
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 30, 2, 0, gTextPointers[17]);

            i = gItemEquipmentTypes[OBJ.itemToPurchase];
            newEquipment = OBJ.itemToPurchase;
            if (i <= EQUIPMENT_TYPE_V_HEART) {
               i = gItemCosts[gPartyMembers[OBJ.partyIdx1].weapon];
               OBJ.formerEquipment = gPartyMembers[OBJ.partyIdx1].weapon;
               gPartyMembers[OBJ.partyIdx1].weapon = newEquipment;
            } else if (i <= EQUIPMENT_TYPE_V_HELM) {
               i = gItemCosts[gPartyMembers[OBJ.partyIdx1].helmet];
               OBJ.formerEquipment = gPartyMembers[OBJ.partyIdx1].helmet;
               gPartyMembers[OBJ.partyIdx1].helmet = newEquipment;
            } else {
               i = gItemCosts[gPartyMembers[OBJ.partyIdx1].armor];
               OBJ.formerEquipment = gPartyMembers[OBJ.partyIdx1].armor;
               gPartyMembers[OBJ.partyIdx1].armor = newEquipment;
            }
            DrawSmallEquipmentWindow(OBJ.partyIdx1);
            if (i == 0) {
               // Priceless equipment can't be sold
               obj->state = 14;
               obj->state2 = 0;
            } else {
               obj->state = 17;
               obj->state2 = 0;
            }
         }
         if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            obj->state2++;
         }
         break;

      case 2:
         if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL) {
            gPartyMembers[OBJ.partyIdx1].items[0] = OBJ.itemToPurchase;
            obj->state = 11;
            obj->state2 = 0;
         } else if (gPartyMembers[OBJ.partyIdx1].items[1] == ITEM_NULL) {
            gPartyMembers[OBJ.partyIdx1].items[1] = OBJ.itemToPurchase;
            obj->state = 11;
            obj->state2 = 0;
         } else {
            // "#32's hands are full. Is there someone else I should give it to?"
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            gStringTable[32] = gStringTable[OBJ.partyIdx1];
            DrawText(68, 20, 30, 2, 0, gTextPointers[18]);
            obj->state = 9;
            obj->state2 = 1;
         }
         break;
      }

      break;

   case 13:

      switch (obj->state2) {
      case 0:
         // "#32 cannot use this. Shall I give it to him anyway?"
         gStringTable[32] = gStringTable[OBJ.partyIdx1];
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[19]);
         // YES/NO
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 30, 2, 0, gTextPointers[20]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         gWindowActiveIdx = 0x37;
         obj->state2++;

      // fallthrough
      case 1:
         if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            // "Thank you.  Who shall I give it to?"
            CloseWindow(0x37);
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 30, 2, 0, gTextPointers[21]);
            obj->state = 9;
            obj->state2 = 1;
         } else if (gWindowChoice.raw == 0x3701) {
            CloseWindow(0x37);
            obj->state = 12;
            obj->state2 = 2;
         }
         break;
      }

      break;

   case 14:

      switch (obj->state2) {
      case 0:
         if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL ||
             gPartyMembers[OBJ.partyIdx1].items[1] == ITEM_NULL) {
            obj->state2++;
         } else {
            obj->state = 16;
            obj->state2 = 0;
         }

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 15;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 15:

      switch (obj->state2) {
      case 0:
         // "Thank you. Here is your former equipment."
         if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL) {
            gPartyMembers[OBJ.partyIdx1].items[0] = OBJ.formerEquipment;
         } else {
            gPartyMembers[OBJ.partyIdx1].items[1] = OBJ.formerEquipment;
         }
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[22]);
         DrawSmallEquipmentWindow(OBJ.partyIdx1);
         obj->state2++;
         break;

      case 1:
         if (PressedCircleOrX_2()) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            obj->state = 19;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 16:

      switch (obj->state2) {
      case 0:
         // "I'll put your former equipment in the supply wagon. Thanks."
         gState.depot[OBJ.formerEquipment]++;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[23]);
         obj->state2++;
         break;

      case 1:
         if (PressedCircleOrX_2()) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            obj->state = 19;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 17:

      switch (obj->state2) {
      case 0:
         if (PressedCircleOrX_2()) {
            obj->state2++;
         }
         break;

      case 1:
         // "May I purchase your used equipment?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[24]);
         // YES/NO
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 30, 2, 0, gTextPointers[25]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         obj->state2++;
         break;

      case 2:
         if (gWindowChoice.raw == 0x3701) {
            obj->state = 18;
            obj->state2 = 0;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL ||
                gPartyMembers[OBJ.partyIdx1].items[1] == ITEM_NULL) {
               obj->state = 15;
               obj->state2 = 0;
            } else {
               obj->state = 16;
               obj->state2 = 0;
            }
         }
         break;
      }

      break;

   case 18:

      switch (obj->state2) {
      case 0:
         // "Thank you. Please accept this as payment in return."
         gState.gold += gItemCosts[OBJ.formerEquipment] / 2;
         CloseWindow(0x37);
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[26]);
         obj->state2++;
         break;

      case 1:
         if (PressedCircleOrX_2()) {
            SlideWindowTo(0x3b, 330, 90);
            CloseWindow(0x3b);
            SlideWindowTo(0x3c, 308, 190);
            CloseWindow(0x3c);
            ClearIcons();
            obj->state = 19;
            obj->state2 = 0;
            break;
         }
         break;
      }

      break;

   case 19:
      // "Is there anything else you need?"
      DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
      DrawText(68, 20, 30, 2, 0, gTextPointers[27]);
      obj->state = 6;
      obj->state2 = 2;
      break;

   case 30:
      // "Who is holding the item that you wish to sell?"
      SlideWindowTo(0x38, 12, 90);
      DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
      DrawText(68, 20, 30, 2, 0, gTextPointers[28]);
      DrawWindow(0x3b, 412, 100, 96, 136, 90, 90, WBS_CROSSED, OBJ.partyRows);
      ListParty(OBJ.partyTop, OBJ.partyRows, 0);
      DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
      DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gState.choices[3]);
      DrawWindow(0x3e, 400, 0, 64, 63, 0, 0, WBS_CROSSED, 0);
      OBJ.partyIdx2 = -1;
      obj->state++;

   // fallthrough
   case 31:
      gWindowActiveIdx = 0x3b;
      i = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
      if (OBJ.partyIdx2 != i && i < 13) {
         OBJ.partyIdx2 = i;
      }
#ifdef PC_PORT
      /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot (i >= 13,
       * so partyIdx2 is still -1). On PSX the garbage portrait id is harmless -- overwritten by the
       * `i >= 13` branch below -- but on PC the wild index can hit unmapped memory. Skip the read when
       * partyIdx2 is invalid; the branch below sets the portrait to 0 in exactly that case. */
      if (OBJ.partyIdx2 >= 0)
         gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#else
      gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#endif
      SlideWindowTo(0x3e, 210, 128);
      if (i >= 13) {
         gState.unitListPortraitId = 0;
         SlideWindowTo(0x3e, 210, 250);
      }
      if (gWindowChoice.raw == 0x3bff) {
         SlideWindowTo(0x3b, 90, 240);
         CloseWindow(0x3b);
         SlideWindowTo(0x3e, 210, 250);
         obj->state = 5;
      } else if (gWindowChoice.s.windowId == 0x3b && gWindowChoice.s.choice != 0) {
         SlideWindowTo(0x3b, 90, 240);
         CloseWindow(0x3b);
         OBJ.partyIdx1 = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (OBJ.partyIdx1 == 13) {
            SlideWindowTo(0x38, -80, 90);
            obj->state = 33;
            obj->state2 = 0;
         } else {
            // "We pay a fair rate for all goods we buy."
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(68, 20, 30, 2, 0, gTextPointers[29]);
            SlideWindowTo(0x38, -80, 90);
            obj->state++;
         }
      }
      break;

   case 32:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x3f, 0, 100, 200, 108, 10, 390, WBS_CROSSED, 5);
         ListPartyMemberInventory(OBJ.partyIdx1, 1);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawPartyItemList1Byte(1);
         else
#endif
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         gWindowActiveIdx = 0x3f;
         D_8012338C = 0x3f;
         DisplayCustomWindowWithSetChoice(0x3f, 5, 1, 40, 0, 0,
                                          gMenuMem_SellingFromParty[OBJ.partyIdx1]);
         SlideWindowTo(0x3f, 10, 90);
         obj->state2++;
         break;

      case 1:
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 2:
         gMenuMem_SellingFromParty[OBJ.partyIdx1] = gState.choices[4];
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gPartyMemberInventory[gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 29, 2, 0, gItemDescriptions2[i]);
         }
         if ((gPadStateNewPresses & PAD_X) || (gWindowChoice.raw == 0x3fff)) {
            SlideWindowTo(0x3f, 90, 240);
            CloseWindow(0x3f);
            D_8012338C = 0;
            obj->state = 30;
            obj->state2 = 0;
         } else if (gWindowChoice.s.windowId == 0x3f && gWindowChoice.s.choice != 0) {
            OBJ.itemToPurchase = gPartyMemberInventory[gHighlightedChoice - 1];
            OBJ.itemIdx = gHighlightedChoice - 4;
            if (OBJ.itemToPurchase != ITEM_NULL) {
               if (gHighlightedChoice < 4) {
                  // "I can't buy weapons or armor that you are currently equipped with."
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 30, 2, 0, gTextPointers[30]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else if (gItemCosts[OBJ.itemToPurchase] == 0) {
                  // "You mustn't sell such a thing!!"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 30, 2, 0, gTextPointers[31]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else {
                  // "I'll give you this much for it.  Do you want to make a deal?"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 30, 2, 0, gTextPointers[32]);
                  // YES/NO
                  DrawWindow(0x37, 256, 140, 64, 54, 240, 160, WBS_CROSSED, 2);
                  DrawText(260, 151, 10, 2, 0, gTextPointers[33]);
                  DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
                  gWindowActiveIdx = 0x37;
                  obj->state2 += 3;
               }
            }
         }
         break;

      case 3:
         if (PressedCircleOrX_2()) {
            obj->state2++;
         }
         break;

      case 4:
         // "Is there anything else you would like to sell?"
         DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[34]);
         gWindowActiveIdx = 0x3f;
         D_8012338C = 0x3f;
         obj->state2 -= 2;
         break;

      case 5:
         if (gWindowChoice.raw == 0x3701) {
            // "Thank you very much. Is there anything else you would like to sell?"
            gState.gold += gItemCosts[OBJ.itemToPurchase] / 2;
            if (gPartyMembers[OBJ.partyIdx1].items[0] != ITEM_NULL) {
               gPartyMembers[OBJ.partyIdx1].items[OBJ.itemIdx] = ITEM_NULL;
            } else {
               gPartyMembers[OBJ.partyIdx1].items[1] = ITEM_NULL;
            }
            ListPartyMemberInventory(OBJ.partyIdx1, 1);
#ifdef PC_FEAT
            if (PC_LangItemNames1Byte()) DrawPartyItemList1Byte(1);
            else
#endif
            DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(68, 20, 30, 2, 0, gTextPointers[35]);
            CloseWindow(0x37);
            gWindowActiveIdx = 0x3f;
            D_8012338C = 0x3f;
            obj->state2 -= 4;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            obj->state2--;
         }
         break;
      }

      break;

   case 33:

      switch (obj->state2) {
      case 0:
         // "What sort of things do you want to sell?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[36]);
         obj->state2++;

      // fallthrough
      case 1:
         // Weapon/Armor/Item/Quit
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x39, 412, 100, 80, 90, 400, 110, WBS_CROSSED, 4);
         DrawText(416, 111, 10, 2, 0, gTextPointers[37]);
         DisplayCustomWindowWithSetChoice(0x39, 2, 1, 30, 0, 0, gState.choices[1]);
         SlideWindowTo(0x39, 110, 110);
         gWindowActiveIdx = 0x39;
         obj->state2++;

      // fallthrough
      case 2:
         if (gWindowChoice.raw == 0x3904 || gWindowChoice.raw == 0x39ff) {
            SlideWindowTo(0x39, 325, 110);
            CloseWindow(0x39);
            obj->state = 30;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3901) {
            OBJ.depotCategory = INVENTORY_WEAPON;
            OBJ.depotTop = gMenuMem_SellingFromDepot[INVENTORY_WEAPON].top;
            obj->state = 34;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3902) {
            OBJ.depotCategory = INVENTORY_ARMOR;
            OBJ.depotTop = gMenuMem_SellingFromDepot[INVENTORY_ARMOR].top;
            obj->state = 34;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3903) {
            OBJ.depotCategory = INVENTORY_ITEM;
            OBJ.depotTop = gMenuMem_SellingFromDepot[INVENTORY_ITEM].top;
            obj->state = 34;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 34:

      switch (obj->state2) {
      case 0:
         SlideWindowTo(0x39, 325, 110);
         CloseWindow(0x39);
         obj->state2++;

      // fallthrough
      case 1:
         // "We pay a fair rate for all goods we buy."
         gState.choices[5] = gMenuMem_SellingFromDepot[OBJ.depotCategory].ofs;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x40, 0, 100, 200, 136, 90, 240, WBS_CROSSED, 7);
         ListDepotInventory(OBJ.depotCategory, OBJ.depotTop, 7, 0, 1);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawDepotList1Byte(OBJ.depotTop, 7, 1);
         else
#endif
         DrawSjisText(28, 110, 30, 2, 0, sInventoryBuffer);
         DisplayCustomWindowWithSetChoice(0x40, 5, 1, 50, 1, 0, gState.choices[5]);
         gWindowActiveIdx = 0x40;
         D_8012338C = 0x40;
         SlideWindowTo(0x40, 10, 90);
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[38]);
         obj->state2++;

      // fallthrough
      case 2:
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 3:
         gMenuMem_SellingFromDepot[OBJ.depotCategory].ofs = gState.choices[5];
         gMenuMem_SellingFromDepot[OBJ.depotCategory].top = OBJ.depotTop;
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 29, 2, 0, gItemDescriptions2[i]);
         }
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x40, 84, 250);
            CloseWindow(0x40);
            gWindowActiveIdx = 0;
            D_8012338C = 0;
            obj->state = 33;
            obj->state2 = 1;
         }
         if (gWindowChoice.s.windowId == 0x40 && gWindowChoice.s.choice != 0) {
            OBJ.itemToPurchase = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
            if (OBJ.itemToPurchase != 0) {
               if (gItemCosts[OBJ.itemToPurchase] == 0) {
                  // "You mustn't sell such a thing!!"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 30, 2, 0, gTextPointers[39]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else {
                  // "I'll give you this much for it.  Do you want to make a deal?"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 30, 2, 0, gTextPointers[40]);
                  // YES/NO
                  DrawWindow(0x37, 256, 140, 64, 54, 240, 160, WBS_CROSSED, 2);
                  DrawText(260, 151, 30, 2, 0, gTextPointers[41]);
                  DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
                  gWindowActiveIdx = 0x37;
                  obj->state2 += 3;
               }
            }
         }
         break;

      case 4:
         if (PressedCircleOrX_2()) {
            obj->state2++;
         }
         break;

      case 5:
         // "Is there anything else you would like to sell?"
         DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[42]);
         gWindowActiveIdx = 0x40;
         D_8012338C = 0x40;
         obj->state2 -= 2;
         break;

      case 6:
         if (gWindowChoice.raw == 0x3701) {
            // "Thank you very much. Is there anything else you would like to sell?"
            gState.gold += gItemCosts[OBJ.itemToPurchase] / 2;
            gState.depot[OBJ.itemToPurchase]--;
            DrawWindow(0x40, 0, 100, 200, 136, 84, 250, WBS_CROSSED, 7);
            ListDepotInventory(OBJ.depotCategory, OBJ.depotTop, 7, 0, 1);
#ifdef PC_FEAT
            if (PC_LangItemNames1Byte()) DrawDepotList1Byte(OBJ.depotTop, 7, 1);
            else
#endif
            DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(68, 20, 30, 2, 0, gTextPointers[43]);
            CloseWindow(0x37);
            gWindowActiveIdx = 0x40;
            D_8012338C = 0x40;
            obj->state2 -= 4;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            obj->state2--;
         }
         break;
      }

      break;

   case 99:

      switch (obj->state2) {
      case 0:
         // "Thanks. Please come again."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 30, 2, 0, gTextPointers[44]);
         OBJ.exitTimer = 60;
         obj->state2++;

      // fallthrough
      case 1:
         if (--OBJ.exitTimer == 0) {
            FadeOutScreen(2, 6);
            OBJ.exitTimer = 50;
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.exitTimer == 0) {
            gState.primary = STATE_20;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state5 = 0;
            return;
         }
         break;
      }

      break;

   case 50:
      // "Whose possessions do you want to transfer?"
      OBJ.partyTop = gMenuMem_TransferFrom.top;
      OBJ.partyNeedsRedraw = 1;
      SlideWindowTo(0x34, 4, 10);
      SlideWindowTo(0x35, 252, 10);
      D_801F6D8C = 0;
      D_801F6D88 = 0;
      DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
      DrawText(20, 20, 30, 2, 0, gTextPointers[45]);
      DrawWindow(0x3b, 412, 100, 96, 136, 90, 90, WBS_CROSSED, OBJ.partyRows);
      ListParty(OBJ.partyTop, OBJ.partyRows, 0);
      DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
      DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gMenuMem_TransferFrom.ofs);
      DrawWindow(0x3e, 400, 0, 64, 63, 0, 0, WBS_CROSSED, 0);
      OBJ.partyIdx2 = -1;
      obj->state++;
      gWindowActiveIdx = 0x3b;
      break;

   case 51:
      gWindowActiveIdx = 0x3b;
      gMenuMem_TransferFrom.ofs = GetWindowChoice(0x3b) - 1;
      gMenuMem_TransferFrom.top = OBJ.partyTop;
      i = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
      if (OBJ.partyIdx2 != i && i < 13) {
         OBJ.partyIdx2 = i;
      }
#ifdef PC_PORT
      /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot (i >= 13,
       * so partyIdx2 is still -1). On PSX the garbage portrait id is harmless -- overwritten by the
       * `i >= 13` branch below -- but on PC the wild index can hit unmapped memory. Skip the read when
       * partyIdx2 is invalid; the branch below sets the portrait to 0 in exactly that case. */
      if (OBJ.partyIdx2 >= 0)
         gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#else
      gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#endif
      SlideWindowTo(0x3e, 210, 128);
      if (i >= 13) {
         SlideWindowTo(0x3e, 210, 250);
         gState.unitListPortraitId = 0;
      }
      if (gWindowChoice.raw == 0x3bff) {
         CloseWindow(0x3b);
         SlideWindowTo(0x3e, 210, 250);
         obj->state = 90;
      } else if (gWindowChoice.s.windowId == 0x3b && gWindowChoice.s.choice != 0) {
         CloseWindow(0x3b);
         D_801F6D98 = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (D_801F6D98 == 13) {
            obj->state = 53;
            obj->state2 = 0;
         } else {
            // "Which possession do you want to transfer?"
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(20, 20, 30, 2, 0, gTextPointers[46]);
            obj->state++;
         }
      }
      break;

   case 52:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x3f, 0, 100, 200, 108, 10, 390, WBS_CROSSED, 5);
         ListPartyMemberInventory(D_801F6D98, 0);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawPartyItemList1Byte(0);
         else
#endif
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         gWindowActiveIdx = 0x3f;
         D_8012338C = 0x3f;
         DisplayCustomWindowWithSetChoice(0x3f, 5, 1, 40, 0, 0,
                                          gMenuMem_PartyInventory[D_801F6D98]);
         SlideWindowTo(0x3f, 10, 90);
         obj->state2++;
         break;

      case 1:
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 2:
         gMenuMem_PartyInventory[D_801F6D98] = gState.choices[4];
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gPartyMemberInventory[gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(20, 20, 35, 2, 0, gItemDescriptions2[i]);
         }
         if ((gPadStateNewPresses & PAD_X) || (gWindowChoice.raw == 0x3fff)) {
            SlideWindowTo(0x3f, 90, 240);
            CloseWindow(0x3f);
            D_8012338C = 0;
            obj->state = 50;
            obj->state2 = 0;
         } else {
            if (gWindowChoice.s.windowId == 0x3f && gWindowChoice.s.choice != 0) {
               D_801F6D94 = gPartyMemberInventory[gHighlightedChoice - 1];
               if (D_801F6D94 != ITEM_NULL) {
                  D_801F6D90 = gHighlightedChoice;
                  if (gHighlightedChoice == 4 && gPartyMembers[D_801F6D98].items[0] == ITEM_NULL) {
                     // First listed item could map to either since they're displayed contiguously
                     D_801F6D90 = gHighlightedChoice + 1;
                  }
                  if (D_801F6D90 < 4) {
                     D_801F6D88 = 1;
                  }
                  SlideWindowTo(0x3f, 90, 240);
                  CloseWindow(0x3f);
                  D_8012338C = 0;
                  gWindowActiveIdx = 0;
                  obj->state = 55;
                  obj->state2 = 0;
               }
            }
         }
         break;
      }

      break;

   case 53:

      switch (obj->state2) {
      case 0:
         // "Which type of possession do you want to transfer?"
         D_801F6D88 = 0;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[47]);
         obj->state2++;

      // fallthrough
      case 1:
         // Weapon/Armor/Item
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x39, 412, 100, 80, 72, 400, 110, WBS_CROSSED, 3);
         DrawText(416, 111, 30, 2, 0, gTextPointers[48]);
         DisplayCustomWindowWithSetChoice(0x39, 2, 1, 30, 0, 0, gState.choices[1]);
         SlideWindowTo(0x39, 110, 110);
         gWindowActiveIdx = 0x39;
         obj->state2++;

      // fallthrough
      case 2:
         if (gWindowChoice.raw == 0x39ff) {
            SlideWindowTo(0x39, 325, 110);
            CloseWindow(0x39);
            obj->state = 50;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3901) {
            OBJ.depotCategory = INVENTORY_WEAPON;
            OBJ.depotTop = 0;
            obj->state = 54;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3902) {
            OBJ.depotCategory = INVENTORY_ARMOR;
            OBJ.depotTop = 0;
            obj->state = 54;
            obj->state2 = 0;
         }
         if (gWindowChoice.raw == 0x3903) {
            OBJ.depotCategory = INVENTORY_ITEM;
            OBJ.depotTop = 0;
            obj->state = 54;
            obj->state2 = 0;
         }
      }

      break;

   case 54:

      switch (obj->state2) {
      case 0:
         SlideWindowTo(0x39, 325, 110);
         CloseWindow(0x39);
         obj->state2++;
         gState.choices[5] = gMenuMem_ShopOrDepot[OBJ.depotCategory].ofs;
         OBJ.depotTop = gMenuMem_ShopOrDepot[OBJ.depotCategory].top;

      // fallthrough
      case 1:
         // "Which possession do you want to transfer?"
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x40, 0, 100, 200, 136, 10, 240, WBS_CROSSED, 7);
         ListDepotInventory(OBJ.depotCategory, OBJ.depotTop, 7, 0, 0);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawDepotList1Byte(OBJ.depotTop, 7, 0);
         else
#endif
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         DisplayCustomWindowWithSetChoice(0x40, 5, 1, 50, 1, 0,
                                          gMenuMem_ShopOrDepot[OBJ.depotCategory].ofs);
         SlideWindowTo(0x40, 10, 90);
         gWindowActiveIdx = 0x40;
         D_8012338C = 0x40;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[49]);
         obj->state2++;

      // fallthrough
      case 2:
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 3:
         gMenuMem_ShopOrDepot[OBJ.depotCategory].ofs = gState.choices[5];
         gMenuMem_ShopOrDepot[OBJ.depotCategory].top = OBJ.depotTop;
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(20, 20, 35, 2, 0, gItemDescriptions2[i]);
         }
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x40, 90, 240);
            CloseWindow(0x40);
            gWindowActiveIdx = 0;
            D_8012338C = 0;
            obj->state = 53;
            obj->state2 = 0;
         }
         if (gWindowChoice.s.windowId == 0x40 && gWindowChoice.s.choice != 0) {
            D_801F6D94 = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
            if (D_801F6D94 != ITEM_NULL) {
               D_801F6D8C = 1;
               SlideWindowTo(0x40, 90, 240);
               CloseWindow(0x40);
               D_8012338C = 0;
               gWindowActiveIdx = 0;
               obj->state = 55;
               obj->state2 = 0;
            }
         }
         break;
      }

      break;

   case 55:

      switch (obj->state2) {
      case 0:
         // "Who do you want to transfer it to?"
         OBJ.partyTop = gMenuMem_TransferTo.top;
         OBJ.partyNeedsRedraw = 1;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[50]);
         DrawWindow(0x3b, 412, 100, 96, 136, 330, 290, WBS_CROSSED, OBJ.partyRows);
         ListParty(OBJ.partyTop, OBJ.partyRows, 0);
         DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
         DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gMenuMem_TransferTo.ofs);
         SlideWindowTo(0x3b, 10, 90);
         OBJ.partyChoice = OBJ.partyTop + GetWindowChoice(0x3b);
         DrawSmallEquipmentWindow(gCurrentParty[OBJ.partyChoice - 1]);
         DrawWindow(0x3c, 256, 200, 128, 36, 108, 188, WBS_CROSSED, 0);
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) DrawItemName1Byte(272, 210, 0, D_801F6D94, ITEM1B_NAME_TIGHT);
         else
#endif
         DrawSjisText(272, 210, 19, 2, 0, gItemNamesSjis[D_801F6D94]);
         DisplayCustomWindow(0x3c, 2, 1, 50, 1, 0);
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DISPLAY_ICON;
         obj_v1->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + D_801F6D94;
         obj_v1->x1.n = 116;
         obj_v1->y1.n = 198;
         obj->state2++;

      // fallthrough
      case 1:
         DrawWindow(0x3e, 400, 0, 64, 63, 0, 0, WBS_CROSSED, 0);
         DrawWindow(0x3d, 324, 140, 64, 54, 350, 170, WBS_CROSSED, 0);
         DrawSjisText(328, 151, 10, 2, 0, sStatChangeBuffer);
         DisplayCustomWindow(0x3d, 2, 1, 5, 0, 0);
         OBJ.partyIdx2 = -1;
         obj->state2++;
         gWindowActiveIdx = 0x3b;
         break;

      case 2:
         gMenuMem_TransferTo.ofs = GetWindowChoice(0x3b) - 1;
         gMenuMem_TransferTo.top = OBJ.partyTop;
         i = GetWindowChoice(0x3b) + OBJ.partyTop;
         if (i != OBJ.partyChoice && i < 13) {
            OBJ.partyChoice = i;
            DrawSmallEquipmentWindow(gCurrentParty[i - 1]);
         }
         if (gItemEquipmentTypes[D_801F6D94] != EQUIPMENT_TYPE_ITEM) {
            SlideWindowTo(0x3d, 240, 170);
         }
         i = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (OBJ.partyIdx2 != i && i < 13) {
            OBJ.partyIdx2 = i;
            DrawWindow(0x3d, 324, 140, 64, 54, 240, 170, WBS_CROSSED, 0);
            UpdateStatChangeText(D_801F6D94, OBJ.partyIdx2);
            DrawSjisText(328, 151, 10, 2, 0, sStatChangeBuffer);
         }
         if (i >= 13) {
            SlideWindowTo(0x3d, 350, 170);
         }
#ifdef PC_PORT
         /* See the partyIdx2 >= 0 guard note in state 31 -- avoid the gUnits[-1] wild read on PC. */
         if (OBJ.partyIdx2 >= 0)
            gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#else
         gState.unitListPortraitId = gUnitPortraitIds[gUnits[OBJ.partyIdx2].unitId];
#endif
         SlideWindowTo(0x3e, 210, 128);
         SlideWindowTo(1, 108, 90);
         if (i >= 13) {
            SlideWindowTo(0x3e, 210, 250);
            SlideWindowTo(1, 350, 90);
            gState.unitListPortraitId = 0;
         }
         gWindowActiveIdx = 0x3b;
         if (gWindowChoice.raw == 0x3bff) {
            obj->state = 80;
            obj->state2 = 0;
         } else if (gWindowChoice.s.windowId == 0x3b && gWindowChoice.s.choice != 0) {
            D_8012338C = 0;
            gWindowActiveIdx = 0;
            OBJ.partyIdx1 = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
            if (OBJ.partyIdx1 == D_801F6D98 &&
                (D_801F6D88 != 0 || D_801F6D8C != 0 ||
                 !gEquipmentTypeClassCapability[gItemEquipmentTypes[D_801F6D94]]
                                               [gUnits[OBJ.partyIdx1].class])) {
               // "You gave it back to yourself."
               DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
               DrawText(20, 20, 30, 2, 0, gTextPointers[51]);
               obj->state = 56;
               obj->state2 = 0;
            } else if (OBJ.partyIdx1 == 13) {
               if (D_801F6D88 == 0) {
                  obj->state = 58;
                  obj->state2 = 0;
               } else {
                  obj->state = 57;
                  obj->state2 = 0;
               }
            } else if (gEquipmentTypeClassCapability[gItemEquipmentTypes[D_801F6D94]]
                                                    [gUnits[OBJ.partyIdx1].class] &&
                       D_801F6D94 != ITEM_V_HEART &&
                       !(OBJ.partyIdx1 == UNIT_ASH &&
                         gPartyMembers[UNIT_ASH].weapon == ITEM_V_HEART &&
                         gItemEquipmentTypes[D_801F6D94] <= EQUIPMENT_TYPE_V_HEART)) {
               SlideWindowTo(0x3d, 350, 170);
               CloseWindow(0x3d);
               SlideWindowTo(0x3e, 240, 168);
               if (D_801F6D88 != 0) {
                  obj->state = 62;
                  obj->state2 = 0;
               } else {
                  if (OBJ.partyIdx1 == D_801F6D98) {
                     obj->state = 64;
                     obj->state2 = 0;
                  } else {
                     obj->state = 63;
                     obj->state2 = 0;
                  }
               }
            } else {
               if (OBJ.partyIdx1 == D_801F6D98 &&
                   gItemEquipmentTypes[D_801F6D94] <= EQUIPMENT_TYPE_V_HEART) {
                  // "You gave it back to yourself."
                  DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
                  DrawText(20, 20, 30, 2, 0, gTextPointers[51]);
                  obj->state = 56;
                  obj->state2 = 0;
               } else if (D_801F6D88 != 0) {
                  obj->state = 59;
                  obj->state2 = 0;
               } else {
                  if (gPartyMembers[OBJ.partyIdx1].items[0] != ITEM_NULL &&
                      gPartyMembers[OBJ.partyIdx1].items[1] != ITEM_NULL) {
                     obj->state = 61;
                     obj->state2 = 0;
                  } else {
                     obj->state = 60;
                     obj->state2 = 0;
                  }
               }
            }
         }
         break;
      }

      break;

   case 56:
      if (PressedCircleOrX_2()) {
         obj->state = 80;
         obj->state2 = 0;
      }
      break;

   case 57:

      switch (obj->state2) {
      case 0:
         // "You can't transfer equipped items to the supply wagon."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[52]);
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 58:

      switch (obj->state2) {
      case 0:
         // "Item was transferred to the supply wagon."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[53]);
         gState.depot[D_801F6D94]++;
         gPartyMembers[D_801F6D98].items[D_801F6D90 - 4] = ITEM_NULL;
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 59:

      switch (obj->state2) {
      case 0:
         // "You can't transfer this equipped item."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[54]);
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 60:

      switch (obj->state2) {
      case 0:
         // "Possession has been transferred."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[55]);
         if (D_801F6D8C != 0) {
            gState.depot[D_801F6D94]--;
         } else {
            gPartyMembers[D_801F6D98].items[D_801F6D90 - 4] = ITEM_NULL;
         }
         if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL) {
            gPartyMembers[OBJ.partyIdx1].items[0] = D_801F6D94;
         } else {
            gPartyMembers[OBJ.partyIdx1].items[1] = D_801F6D94;
         }
         DrawSmallEquipmentWindow(gCurrentParty[OBJ.partyChoice - 1]);
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 61:

      switch (obj->state2) {
      case 0:
         // "Too many possessions. Cannot transfer."
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[56]);
         obj->state2++;

      // fallthrough
      case 1:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 62:

      switch (obj->state2) {
      case 0:
         // "Do you want to exchange equipment?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 30, 2, 0, gTextPointers[57]);
         // YES/NO
         gWindowActiveIdx = 0x37;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[58]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3701) {
            // "Equipment exchange completed."
            CloseWindow(0x37);
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(20, 20, 30, 2, 0, gTextPointers[59]);
            i = gItemEquipmentTypes[D_801F6D94];
            if (i <= EQUIPMENT_TYPE_V_HEART) {
               gPartyMembers[D_801F6D98].weapon = gPartyMembers[OBJ.partyIdx1].weapon;
               gPartyMembers[OBJ.partyIdx1].weapon = D_801F6D94;
            } else if (i <= EQUIPMENT_TYPE_V_HELM) {
               gPartyMembers[D_801F6D98].helmet = gPartyMembers[OBJ.partyIdx1].helmet;
               gPartyMembers[OBJ.partyIdx1].helmet = D_801F6D94;
            } else {
               gPartyMembers[D_801F6D98].armor = gPartyMembers[OBJ.partyIdx1].armor;
               gPartyMembers[OBJ.partyIdx1].armor = D_801F6D94;
            }
            DrawSmallEquipmentWindow(OBJ.partyIdx1);
            obj->state2++;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            obj->state = 80;
            obj->state2 = 0;
            CloseWindow(0x37);
         }
         break;

      case 2:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 63:
   case 64:

      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         if (obj->state == 63) {
            // "Do you want to equip it?"
            DrawText(20, 20, 30, 2, 0, gTextPointers[60]);
         } else {
            // "Equipment exchange completed."
            DrawText(20, 20, 30, 2, 0, gTextPointers[61]);
         }
         // YES/NO
         gWindowActiveIdx = 0x37;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[62]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         obj->state2++;
         break;

      case 1:
         if (gWindowChoice.raw == 0x3701) {
            // "Equipment exchange completed."
            CloseWindow(0x37);
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(20, 20, 30, 2, 0, gTextPointers[63]);
            i = gItemEquipmentTypes[D_801F6D94];
            if (i <= EQUIPMENT_TYPE_V_HEART) {
               j = gPartyMembers[OBJ.partyIdx1].weapon;
               gPartyMembers[OBJ.partyIdx1].weapon = D_801F6D94;
            } else if (i < EQUIPMENT_TYPE_ARMOR) {
               j = gPartyMembers[OBJ.partyIdx1].helmet;
               gPartyMembers[OBJ.partyIdx1].helmet = D_801F6D94;
            } else {
               j = gPartyMembers[OBJ.partyIdx1].armor;
               gPartyMembers[OBJ.partyIdx1].armor = D_801F6D94;
            }
            if (D_801F6D8C != 0) {
               gState.depot[D_801F6D94]--;
               gState.depot[j]++;
            } else {
               if (D_801F6D90 == 4) {
                  gPartyMembers[D_801F6D98].items[0] = j;
               } else {
                  gPartyMembers[D_801F6D98].items[1] = j;
               }
            }
            DrawSmallEquipmentWindow(OBJ.partyIdx1);
            obj->state2++;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            if (obj->state == 64) {
               // "You gave it back to yourself."
               DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
               DrawText(20, 20, 30, 2, 0, gTextPointers[64]);
               obj->state = 56;
               obj->state2 = 0;
            } else {
               if (gPartyMembers[OBJ.partyIdx1].items[0] != ITEM_NULL &&
                   gPartyMembers[OBJ.partyIdx1].items[1] != ITEM_NULL) {
                  obj->state = 61;
                  obj->state2 = 0;
               } else {
                  obj->state = 60;
                  obj->state2 = 0;
               }
            }
         }
         break;

      case 2:
         if (PressedCircleOrX_2()) {
            obj->state = 80;
            obj->state2 = 0;
         }
         break;
      }

      break;

   case 80:
      D_8012338C = 0;
      gWindowActiveIdx = 0;
      SlideWindowTo(0x3b, 330, 90);
      CloseWindow(0x3b);
      SlideWindowTo(0x3c, 308, 190);
      CloseWindow(0x3c);
      ClearIcons();
      SlideWindowTo(0x3d, 350, 170);
      CloseWindow(0x3d);
      SlideWindowTo(0x3e, 210, 250);
      SlideWindowTo(1, 350, 90);
      obj->state = 50;
      obj->state2 = 0;
      break;

   case 90:
      D_801F6D44 = 40;
      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 91:

      switch (obj->state2) {
      case 0:
         if (--D_801F6D44 == 0) {
            FadeOutScreen(2, 6);
            D_801F6D44 = 50;
            obj->state2++;
         }
         break;

      case 1:
         if (--D_801F6D44 == 0) {
            if (gState.prevState == 6 || gState.prevState == 28) {
               gState.primary = STATE_28;
            }
            if (gState.prevState == 7 || gState.prevState == 20) {
               gState.primary = STATE_20;
            }
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         break;
      }

      break;
   }

   if (gWindowActiveIdx == 0x40) {
      i = 0;
      while (gDepotInventoryPtr[i] != ITEM_NULL) {
         i++;
      }
      j = i;
      if (i > 7) {
         i -= 7;
      } else {
         i = 0;
      }
      if ((gPadStateNewPresses & PAD_DOWN) && (i > OBJ.depotTop) && (gHighlightedChoice == 7)) {
         OBJ.depotTop++;
         OBJ.depotNeedsRedraw++;
      }
      if (OBJ.depotTop != 0) {
         if ((gPadStateNewPresses & PAD_UP) && (gHighlightedChoice == 1)) {
            OBJ.depotTop--;
            OBJ.depotNeedsRedraw++;
         }
      }
      if (OBJ.depotTop != 0) {
         if (PositionScrollIndicator(0x40, 0)) {
            RenderScrollIndicator(0, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
      if (j > OBJ.depotTop + 7) {
         if (PositionScrollIndicator(0x40, 1)) {
            RenderScrollIndicator(1, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
   }

   if (gWindowActiveIdx == 0x3a) {
      i = 0;
      while (gShopInventories[OBJ.shopId][OBJ.category][i] != ITEM_NULL) {
         i++;
      }
      j = i;
      if (i > OBJ.shopRows) {
         i -= OBJ.shopRows;
      } else {
         i = 0;
      }
      if ((gPadStateNewPresses & PAD_DOWN) && (i > OBJ.shopTop) &&
          (gHighlightedChoice == OBJ.shopRows)) {
         OBJ.shopTop++;
         OBJ.shopNeedsRedraw++;
      }
      if (OBJ.shopTop != 0) {
         if ((gPadStateNewPresses & PAD_UP) && (gHighlightedChoice == 1)) {
            OBJ.shopTop--;
            OBJ.shopNeedsRedraw++;
         }
      }
      if (OBJ.shopTop != 0) {
         if (PositionScrollIndicator(0x3a, 0)) {
            RenderScrollIndicator(0, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
      if (j > OBJ.shopTop + OBJ.shopRows) {
         if (PositionScrollIndicator(0x3a, 1)) {
            RenderScrollIndicator(1, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
   }

   if (gWindowActiveIdx == 0x3b) {
      i = 0;
      while (gCurrentParty[i] != 0) {
         i++;
      }
      j = i;
      if (i > OBJ.partyRows) {
         i -= OBJ.partyRows;
      } else {
         i = 0;
      }
      if ((gPadStateNewPresses & PAD_DOWN) && (i > OBJ.partyTop) &&
          (gHighlightedChoice == OBJ.partyRows)) {
         OBJ.partyTop++;
         OBJ.partyNeedsRedraw++;
      }
      if (OBJ.partyTop != 0) {
         if ((gPadStateNewPresses & PAD_UP) && (gHighlightedChoice == 1)) {
            OBJ.partyTop--;
            OBJ.partyNeedsRedraw++;
         }
      }
      if (OBJ.partyTop != 0) {
         if (PositionScrollIndicator(0x3b, 0)) {
            RenderScrollIndicator(0, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
      if (j > OBJ.partyTop + OBJ.partyRows) {
         if (PositionScrollIndicator(0x3b, 1)) {
            RenderScrollIndicator(1, gScrollIndicatorX, gScrollIndicatorY);
         }
      }
   }

   if (OBJ.depotNeedsRedraw) {
      OBJ.depotNeedsRedraw = 0;
      i = (obj->state < 51) ? 1 : 0;
      ListDepotInventory(OBJ.depotCategory, OBJ.depotTop, 7, 1, i);
#ifdef PC_FEAT
      if (PC_LangItemNames1Byte()) DrawDepotList1Byte(OBJ.depotTop, 7, i);
      else
#endif
      DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
   }
   if (OBJ.shopNeedsRedraw) {
      OBJ.shopNeedsRedraw = 0;
#ifdef PC_FEAT
      if (PC_LangItemNames1Byte()) {
         DrawShopList1Byte(OBJ.shopId, OBJ.category, OBJ.shopTop, OBJ.shopRows);
      } else
#endif
      {
         ListShopInventory(OBJ.shopId, OBJ.category, OBJ.shopTop, OBJ.shopRows, 1);
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
      }
   }
   if (OBJ.partyNeedsRedraw) {
      OBJ.partyNeedsRedraw = 0;
      DrawWindow(0x3b, 412, 100, 96, 136, 84, 250, WBS_CROSSED, OBJ.partyRows);
      ListParty(OBJ.partyTop, OBJ.partyRows, 1);
      DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
   }

   switch (OBJ.goldState) {
   case 0:
      OBJ.goldTimer = 50;
      OBJ.goldState++;

   // fallthrough
   case 1:
      if (--OBJ.goldTimer == 0) {
         D_801F6D40 = gState.gold;
#ifdef PC_FEAT
         /* exchange/91: the 1-byte item list needs the right area, so shrink the gold box (104->72),
          * draw the amount in the small font, and slide it right (208->241) to sit clear of the list. */
         if (PC_LangItemNames1Byte()) {
            DrawWindow(0x36, 256, 98, GOLD1B_W, 36, 408, 90, WBS_CROSSED, 0);
            DrawGold1Byte(D_801F6D40);
         } else
#endif
         {
            DrawWindow(0x36, 256, 98, 104, 36, 408, 90, WBS_CROSSED, 0);
            EmbedIntAsSjis(D_801F6D40, sGoldBuffer, 6);
            DrawSjisText(256, 109, 10, 2, 0, sGoldBuffer);
         }
         DisplayCustomWindow(0x36, 0, 1, 49, 0, 0);
#ifdef PC_FEAT
         SlideWindowTo(0x36, PC_LangItemNames1Byte() ? GOLD1B_X : 208, 90);
#else
         SlideWindowTo(0x36, 208, 90);
#endif
         OBJ.goldState++;
      }
      break;

   case 2:
      if (D_801F6D40 != gState.gold) {
         D_801F6D40 = gState.gold;
#ifdef PC_FEAT
         if (PC_LangItemNames1Byte()) {
            DrawGold1Byte(gState.gold);
         } else
#endif
         {
            EmbedIntAsSjis(gState.gold, sGoldBuffer, 6);
            DrawSjisText(256, 109, 10, 2, 0, sGoldBuffer);
         }
      }
      break;
   }
}

#undef OBJF
#define OBJF 009
void Objf009_ItemIconMgr(Object *obj) {
   s32 i, j;
   Object *shopOrDepot;
   Object *window;
   Object *p;
   s16 x, y;
   s16 dx, dy;
   s32 tmp;

   shopOrDepot = OBJ.shopOrDepot;

   for (i = 0; i < 7; i++) {
      gState.dynamicIcons[i].gfxIdx = GFX_NULL;
   }

   if (D_8012338C == 0x40) {
      p = &gObjectArray[0];
      for (j = 0; j < OBJ_DATA_CT; j++, p++) {
         if (p->functionIndex == OBJF_WINDOW_TBD_005 && p->d.objf005.windowId == 0x40 &&
             p->d.objf005.window != NULL) {

            window = p->d.objf005.window;
            dx = (window->d.sprite2.coords[2].x - window->d.sprite2.coords[0].x) / 7;
            dy = (window->d.sprite2.coords[2].y - window->d.sprite2.coords[0].y - 16) / 7;
            x = window->d.sprite2.coords[0].x + 9;
            y = window->d.sprite2.coords[0].y + 10;

            for (i = 0; i < 7; i++) {
               gState.dynamicIcons[i].x = x;
               gState.dynamicIcons[i].y = y;
               //? Maybe fake match due to unusual assignments
               tmp = gState.dynamicIcons[i].gfxIdx =
                   gDepotInventoryPtr[shopOrDepot->d.objf406.depotTop + i];
               if (tmp != 0) {
                  gState.dynamicIcons[i].gfxIdx += GFX_ITEM_ICONS_OFS;
               }
               x += dx;
               y += dy;
            }
            break;
         }
      }
   }

   if (D_8012338C == 0x3f) {
      p = &gObjectArray[0];
      for (j = 0; j < OBJ_DATA_CT; j++, p++) {
         if (p->functionIndex == OBJF_WINDOW_TBD_005 && p->d.objf005.windowId == 0x3f &&
             p->d.objf005.window != NULL) {

            window = p->d.objf005.window;
            dx = (window->d.sprite2.coords[2].x - window->d.sprite2.coords[0].x) / 5;
            dy = (window->d.sprite2.coords[2].y - window->d.sprite2.coords[0].y - 20) / 5;
            x = window->d.sprite2.coords[0].x + 9;
            y = window->d.sprite2.coords[0].y + 10;

            for (i = 0; i < 5; i++) {
               gState.dynamicIcons[i].x = x;
               gState.dynamicIcons[i].y = y;
               tmp = gState.dynamicIcons[i].gfxIdx = gPartyMemberInventory[i];
               if (tmp != 0) {
                  gState.dynamicIcons[i].gfxIdx += GFX_ITEM_ICONS_OFS;
               }
               x += dx;
               y += dy;
            }
            break;
         }
      }
   }

   if (D_8012338C == 0x3a) {
      p = &gObjectArray[0];
      for (j = 0; j < OBJ_DATA_CT; j++, p++) {
         if (p->functionIndex == OBJF_WINDOW_TBD_005 && p->d.objf005.windowId == 0x3a &&
             p->d.objf005.window != NULL) {

            window = p->d.objf005.window;
            dx = (window->d.sprite2.coords[2].x - window->d.sprite2.coords[0].x) / 7;
            dy = (window->d.sprite2.coords[2].y - window->d.sprite2.coords[0].y - 16) / 7;
            x = window->d.sprite2.coords[0].x + 9;
            y = window->d.sprite2.coords[0].y + 10;

            for (i = 0; i < 7; i++) {
               gState.dynamicIcons[i].x = x;
               gState.dynamicIcons[i].y = y;
               tmp = (gState.dynamicIcons[i].gfxIdx =
                          gShopInventories[shopOrDepot->d.objf406.shopId]
                                          [shopOrDepot->d.objf406.category]
                                          [shopOrDepot->d.objf406.shopTop + i]);
               if (tmp != 0) {
                  gState.dynamicIcons[i].gfxIdx += GFX_ITEM_ICONS_OFS;
               }
               x += dx;
               y += dy;
            }
            break;
         }
      }
   }
}

#undef OBJF
#define OBJF 577
void Objf577_DynamicIcon(Object *obj) {
   OBJ.gfxIdx = gState.dynamicIcons[OBJ.idx].gfxIdx;
   if (OBJ.gfxIdx != GFX_NULL) {
      obj->x1.n = gState.dynamicIcons[OBJ.idx].x;
      obj->y1.n = gState.dynamicIcons[OBJ.idx].y;
      obj->x3.n = obj->x1.n + 15;
      obj->y3.n = obj->y1.n + 15;
      AddObjPrim_Gui(gGraphicsPtr->ot, obj);
   }
}
