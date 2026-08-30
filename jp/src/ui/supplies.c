#include "common.h"
#include "object.h"
#include "graphics.h"
#include "state.h"
#include "cd_files.h"
#include "units.h"
#include "window.h"

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
static u8 sAttackUpStr[] = "\x8d\x55\x8c\x82\x97\xcd\n"  /* 攻撃力\n */
                           "\x81\x7b\x81\x40\x81\x40";   /* ＋　　  */

static u8 sAttackDownStr[] = "\x8d\x55\x8c\x82\x97\xcd\n"
                             "\x81\x7c\x81\x40\x81\x40";

static u8 sDefenseUpStr[] = "\x96\x68\x8c\xe4\x97\xcd\n" /* 防御力\n */
                            "\x81\x7b\x81\x40\x81\x40";

static u8 sDefenseDownStr[] = "\x96\x68\x8c\xe4\x97\xcd\n"
                              "\x81\x7c\x81\x40\x81\x40";

#ifdef PERMUTER
/* PC build only: the copy loops below move a fixed 13 bytes, but the JP string is only 11 chars +
 * NUL = 12, shorter than the loop's US-sized length. [13] makes the 13th byte part of the object;
 * it reads 0x00, matching hardware's byte at +12 (SLPM_860.07 @0x801f8b88). */
static u8 sIncompatibleStr[13] = "\x91\x95\x94\xf5\n"      /* 装備\n    */
                                 "\x81\x40\x95\x73\x89\xc2"; /* 　不可  */
#else
static u8 sIncompatibleStr[] = "\x91\x95\x94\xf5\n"      /* 装備\n    */
                               "\x81\x40\x95\x73\x89\xc2"; /* 　不可  */
#endif

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
extern s32 gDisplayedGold; // gold as displayed last
extern u8 gSupplyMenuTimer;  // timer
extern s16 gItemListWindowId; // wnd
extern u8 gDepotItemIsEquipped;  // is equipped
extern u8 gDepotTransferFromDepot;  // transferring from depot
extern u8 gDepotTransferSrcSlot;  // choice (slot+1) of party member item to transfer
extern u8 gDepotTransferItem;  // item to transfer
extern u8 gDepotTransferSrcMember;  // partyIdx of transfer src

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
         // SJIS 9590: 武
         *pDst++ = 0x95;
         *pDst++ = 0x90;
         break;
      case 1:
         // SJIS 93aa: 頭
         *pDst++ = 0x93;
         *pDst++ = 0xaa;
         break;
      case 2:
         // SJIS 91cc: 体
         *pDst++ = 0x91;
         *pDst++ = 0xcc;
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
      for (j = 0; j < 4; j++) {
         *pDst++ = 0x81;
         *pDst++ = 0x40;
      }
      pDst -= 8;
      pSrc = gStringTable[gCurrentParty[top + i]];
      for (j = 0; j < 4; j++) {
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
      gSupplyMenuTimer = 30;
      obj->state++;

   // fallthrough
   case 1:
      if (--gSupplyMenuTimer == 0) {
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
      DrawText(68, 20, 25, 2, 0, gTextPointers[1]);
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
         DrawWindow(0x39, 400, 0, 72, 90, 400, 110, WBS_CROSSED, 4);
         DrawText(404, 11, 10, 2, 0, gTextPointers[3]);
         DisplayCustomWindowWithSetChoice(0x39, 2, 1, 30, 0, 0, gState.choices[1]);
         SlideWindowTo(0x39, 110, 110);
         obj->state2++;

      // fallthrough
      case 1:
         // "What sort of thing are you looking for?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 20, 2, 0, gTextPointers[4]);
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
      DrawText(68, 20, 19, 2, 0, gTextPointers[5]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[6]);
         obj->state2++;

      // fallthrough
      case 1:
         gState.choices[2] = gMenuMem_ShopOrDepot[OBJ.category].ofs;
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x3a, 0, 100, 200, 136, 84, 250, WBS_CROSSED, OBJ.shopRows);
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
            DrawText(68, 20, 19, 2, 0, gItemDescriptions2[i]);
         }
         gWindowActiveIdx = 0x3a;
         gItemListWindowId = 0x3a;
         SlideWindowTo(0x3a, 10, 90);
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x3a, 84, 250);
            CloseWindow(0x3a);
            gWindowActiveIdx = 0;
            gItemListWindowId = 0;
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[7]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[9]);
         obj->state = 6;
         obj->state2 = 2;
      }

      break;

   case 8:
      // "Too bad. Anything else?"
      DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
      DrawText(68, 20, 19, 2, 0, gTextPointers[10]);
      obj->state = 6;
      obj->state2 = 2;
      break;

   case 9:

      switch (obj->state2) {
      case 0:
         // "Thank you.  Who shall I give it to?"
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 19, 2, 0, gTextPointers[11]);
         SlideWindowTo(0x3a, -210, 90);
         DrawWindow(0x3b, 412, 100, 96, 136, 330, 290, WBS_CROSSED, OBJ.partyRows);
         ListParty(OBJ.partyTop, OBJ.partyRows, 0);
         DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
         DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gState.choices[3]);
         SlideWindowTo(0x3b, 10, 90);
         OBJ.partyChoice = OBJ.partyTop + GetWindowChoice(0x3b);
         DrawSmallEquipmentWindow(gCurrentParty[OBJ.partyChoice - 1]);
         DrawWindow(0x3c, 256, 200, 128, 36, 108, 188, WBS_CROSSED, 0);
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
         /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot
          * (partyIdx2 still -1); harmless on PSX (the i >= 13 branch below overwrites it) but a
          * wild index on a host. Skip the read when invalid; that branch sets 0 anyway. */
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
                     DrawText(68, 20, 20, 2, 0, gTextPointers[12]);
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
         DrawText(68, 20, 20, 2, 0, gTextPointers[13]);
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
         DrawText(68, 20, 20, 2, 0, gTextPointers[14]);
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
         DrawText(68, 20, 20, 2, 0, gTextPointers[15]);
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
            DrawText(68, 20, 20, 2, 0, gTextPointers[17]);

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
            DrawText(68, 20, 20, 2, 0, gTextPointers[18]);
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
         DrawText(68, 20, 20, 2, 0, gTextPointers[19]);
         // YES/NO
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[20]);
         DisplayCustomWindow(0x37, 2, 1, 5, 0, 0);
         gWindowActiveIdx = 0x37;
         obj->state2++;

      // fallthrough
      case 1:
         if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            // "Thank you.  Who shall I give it to?"
            CloseWindow(0x37);
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(68, 20, 20, 2, 0, gTextPointers[21]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[22]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[23]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[24]);
         // YES/NO
         DrawWindow(0x37, 256, 140, 64, 54, 210, 126, WBS_CROSSED, 2);
         DrawText(260, 151, 10, 2, 0, gTextPointers[25]);
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[26]);
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
      DrawText(68, 20, 20, 2, 0, gTextPointers[27]);
      obj->state = 6;
      obj->state2 = 2;
      break;

   case 30:
      // "Who is holding the item that you wish to sell?"
      SlideWindowTo(0x38, 12, 90);
      DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
      DrawText(68, 20, 25, 2, 0, gTextPointers[28]);
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
      /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot
       * (partyIdx2 still -1); harmless on PSX (the i >= 13 branch below overwrites it) but a
       * wild index on a host. Skip the read when invalid; that branch sets 0 anyway. */
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
            DrawText(68, 20, 25, 2, 0, gTextPointers[29]);
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
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         gWindowActiveIdx = 0x3f;
         gItemListWindowId = 0x3f;
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
            DrawText(68, 20, 19, 2, 0, gItemDescriptions2[i]);
         }
         if ((gPadStateNewPresses & PAD_X) || (gWindowChoice.raw == 0x3fff)) {
            SlideWindowTo(0x3f, 90, 240);
            CloseWindow(0x3f);
            gItemListWindowId = 0;
            obj->state = 30;
            obj->state2 = 0;
         } else if (gWindowChoice.s.windowId == 0x3f && gWindowChoice.s.choice != 0) {
            OBJ.itemToPurchase = gPartyMemberInventory[gHighlightedChoice - 1];
            OBJ.itemIdx = gHighlightedChoice - 4;
            if (OBJ.itemToPurchase != ITEM_NULL) {
               if (gHighlightedChoice < 4) {
                  // "I can't buy weapons or armor that you are currently equipped with."
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 25, 2, 0, gTextPointers[30]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else if (gItemCosts[OBJ.itemToPurchase] == 0) {
                  // "You mustn't sell such a thing!!"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 25, 2, 0, gTextPointers[31]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else {
                  // "I'll give you this much for it.  Do you want to make a deal?"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 25, 2, 0, gTextPointers[32]);
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
         DrawText(68, 20, 25, 2, 0, gTextPointers[34]);
         gWindowActiveIdx = 0x3f;
         gItemListWindowId = 0x3f;
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
            DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(68, 20, 25, 2, 0, gTextPointers[35]);
            CloseWindow(0x37);
            gWindowActiveIdx = 0x3f;
            gItemListWindowId = 0x3f;
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[36]);
         obj->state2++;

      // fallthrough
      case 1:
         // Weapon/Armor/Item/Quit
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x39, 412, 100, 72, 90, 400, 110, WBS_CROSSED, 4);
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
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         DisplayCustomWindowWithSetChoice(0x40, 5, 1, 50, 1, 0, gState.choices[5]);
         gWindowActiveIdx = 0x40;
         gItemListWindowId = 0x40;
         SlideWindowTo(0x40, 10, 90);
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(68, 20, 19, 2, 0, gTextPointers[38]);
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
            DrawText(68, 20, 19, 2, 0, gItemDescriptions2[i]);
         }
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x40, 84, 250);
            CloseWindow(0x40);
            gWindowActiveIdx = 0;
            gItemListWindowId = 0;
            obj->state = 33;
            obj->state2 = 1;
         }
         if (gWindowChoice.s.windowId == 0x40 && gWindowChoice.s.choice != 0) {
            OBJ.itemToPurchase = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
            if (OBJ.itemToPurchase != 0) {
               if (gItemCosts[OBJ.itemToPurchase] == 0) {
                  // "You mustn't sell such a thing!!"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 25, 2, 0, gTextPointers[39]);
                  gWindowActiveIdx = 0;
                  obj->state2++;
               } else {
                  // "I'll give you this much for it.  Do you want to make a deal?"
                  DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
                  DrawText(68, 20, 25, 2, 0, gTextPointers[40]);
                  // YES/NO
                  DrawWindow(0x37, 256, 140, 64, 54, 240, 160, WBS_CROSSED, 2);
                  DrawText(260, 151, 10, 2, 0, gTextPointers[41]);
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
         DrawText(68, 20, 25, 2, 0, gTextPointers[42]);
         gWindowActiveIdx = 0x40;
         gItemListWindowId = 0x40;
         obj->state2 -= 2;
         break;

      case 6:
         if (gWindowChoice.raw == 0x3701) {
            // "Thank you very much. Is there anything else you would like to sell?"
            gState.gold += gItemCosts[OBJ.itemToPurchase] / 2;
            gState.depot[OBJ.itemToPurchase]--;
            DrawWindow(0x40, 0, 100, 200, 136, 84, 250, WBS_CROSSED, 7);
            ListDepotInventory(OBJ.depotCategory, OBJ.depotTop, 7, 0, 1);
            DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(68, 20, 25, 2, 0, gTextPointers[43]);
            CloseWindow(0x37);
            gWindowActiveIdx = 0x40;
            gItemListWindowId = 0x40;
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
         DrawText(68, 20, 19, 2, 0, gTextPointers[44]);
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
      gDepotTransferFromDepot = 0;
      gDepotItemIsEquipped = 0;
      DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
      DrawText(20, 20, 25, 2, 0, gTextPointers[45]);
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
      /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot
       * (partyIdx2 still -1); harmless on PSX (the i >= 13 branch below overwrites it) but a
       * wild index on a host. Skip the read when invalid; that branch sets 0 anyway. */
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
         gDepotTransferSrcMember = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (gDepotTransferSrcMember == 13) {
            obj->state = 53;
            obj->state2 = 0;
         } else {
            // "Which possession do you want to transfer?"
            DrawWindow(0x34, 0, 0, 312, 90, -400, -400, WBS_DRAGON, 0);
            DrawText(20, 20, 25, 2, 0, gTextPointers[46]);
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
         ListPartyMemberInventory(gDepotTransferSrcMember, 0);
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         gWindowActiveIdx = 0x3f;
         gItemListWindowId = 0x3f;
         DisplayCustomWindowWithSetChoice(0x3f, 5, 1, 40, 0, 0,
                                          gMenuMem_PartyInventory[gDepotTransferSrcMember]);
         SlideWindowTo(0x3f, 10, 90);
         obj->state2++;
         break;

      case 1:
         OBJ.selectionModified = 0;
         obj->state2++;

      // fallthrough
      case 2:
         gMenuMem_PartyInventory[gDepotTransferSrcMember] = gState.choices[4];
         if ((gPadStateNewPresses & PAD_DOWN) || (gPadStateNewPresses & PAD_UP)) {
            OBJ.selectionModified = 1;
         }
         i = gPartyMemberInventory[gHighlightedChoice - 1];
         if (OBJ.selectionModified && OBJ.item != i) {
            OBJ.item = i;
            DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
            DrawText(20, 20, 19, 2, 0, gItemDescriptions2[i]);
         }
         if ((gPadStateNewPresses & PAD_X) || (gWindowChoice.raw == 0x3fff)) {
            SlideWindowTo(0x3f, 90, 240);
            CloseWindow(0x3f);
            gItemListWindowId = 0;
            obj->state = 50;
            obj->state2 = 0;
         } else {
            if (gWindowChoice.s.windowId == 0x3f && gWindowChoice.s.choice != 0) {
               gDepotTransferItem = gPartyMemberInventory[gHighlightedChoice - 1];
               if (gDepotTransferItem != ITEM_NULL) {
                  gDepotTransferSrcSlot = gHighlightedChoice;
                  if (gHighlightedChoice == 4 && gPartyMembers[gDepotTransferSrcMember].items[0] == ITEM_NULL) {
                     // First listed item could map to either since they're displayed contiguously
                     gDepotTransferSrcSlot = gHighlightedChoice + 1;
                  }
                  if (gDepotTransferSrcSlot < 4) {
                     gDepotItemIsEquipped = 1;
                  }
                  SlideWindowTo(0x3f, 90, 240);
                  CloseWindow(0x3f);
                  gItemListWindowId = 0;
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
         gDepotItemIsEquipped = 0;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 19, 2, 0, gTextPointers[47]);
         obj->state2++;

      // fallthrough
      case 1:
         // Weapon/Armor/Item
         gWindowChoiceHeight = 17;
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x39, 412, 100, 72, 72, 400, 110, WBS_CROSSED, 3);
         DrawText(416, 111, 10, 2, 0, gTextPointers[48]);
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
         DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
         DisplayCustomWindowWithSetChoice(0x40, 5, 1, 50, 1, 0,
                                          gMenuMem_ShopOrDepot[OBJ.depotCategory].ofs);
         SlideWindowTo(0x40, 10, 90);
         gWindowActiveIdx = 0x40;
         gItemListWindowId = 0x40;
         DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
         DrawText(20, 20, 19, 2, 0, gTextPointers[49]);
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
            DrawText(20, 20, 19, 2, 0, gItemDescriptions2[i]);
         }
         if (gPadStateNewPresses & PAD_X) {
            SlideWindowTo(0x40, 90, 240);
            CloseWindow(0x40);
            gWindowActiveIdx = 0;
            gItemListWindowId = 0;
            obj->state = 53;
            obj->state2 = 0;
         }
         if (gWindowChoice.s.windowId == 0x40 && gWindowChoice.s.choice != 0) {
            gDepotTransferItem = gDepotInventoryPtr[OBJ.depotTop + gHighlightedChoice - 1];
            if (gDepotTransferItem != ITEM_NULL) {
               gDepotTransferFromDepot = 1;
               SlideWindowTo(0x40, 90, 240);
               CloseWindow(0x40);
               gItemListWindowId = 0;
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
         DrawText(20, 20, 19, 2, 0, gTextPointers[50]);
         DrawWindow(0x3b, 412, 100, 96, 136, 330, 290, WBS_CROSSED, OBJ.partyRows);
         ListParty(OBJ.partyTop, OBJ.partyRows, 0);
         DrawSjisText(420, 110, 19, 2, 0, gPartyListBuffer);
         DisplayCustomWindowWithSetChoice(0x3b, 5, 1, 50, 1, 0, gMenuMem_TransferTo.ofs);
         SlideWindowTo(0x3b, 10, 90);
         OBJ.partyChoice = OBJ.partyTop + GetWindowChoice(0x3b);
         DrawSmallEquipmentWindow(gCurrentParty[OBJ.partyChoice - 1]);
         DrawWindow(0x3c, 256, 200, 128, 36, 108, 188, WBS_CROSSED, 0);
         DrawSjisText(272, 210, 19, 2, 0, gItemNamesSjis[gDepotTransferItem]);
         DisplayCustomWindow(0x3c, 2, 1, 50, 1, 0);
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DISPLAY_ICON;
         obj_v1->d.sprite.gfxIdx = GFX_ITEM_ICONS_OFS + gDepotTransferItem;
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
         if (gItemEquipmentTypes[gDepotTransferItem] != EQUIPMENT_TYPE_ITEM) {
            SlideWindowTo(0x3d, 240, 170);
         }
         i = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
         if (OBJ.partyIdx2 != i && i < 13) {
            OBJ.partyIdx2 = i;
            DrawWindow(0x3d, 324, 140, 64, 54, 240, 170, WBS_CROSSED, 0);
            UpdateStatChangeText(gDepotTransferItem, OBJ.partyIdx2);
            DrawSjisText(328, 151, 10, 2, 0, sStatChangeBuffer);
         }
         if (i >= 13) {
            SlideWindowTo(0x3d, 350, 170);
         }
         #ifdef PC_PORT
         /* Retail reads gUnits[-1] when the first highlighted target is a convoy/non-unit slot
          * (partyIdx2 still -1); harmless on PSX (the i >= 13 branch below overwrites it) but a
          * wild index on a host. Skip the read when invalid; that branch sets 0 anyway. */
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
            gItemListWindowId = 0;
            gWindowActiveIdx = 0;
            OBJ.partyIdx1 = gCurrentParty[OBJ.partyTop + gHighlightedChoice - 1];
            if (OBJ.partyIdx1 == gDepotTransferSrcMember &&
                (gDepotItemIsEquipped != 0 || gDepotTransferFromDepot != 0 ||
                 !gEquipmentTypeClassCapability[gItemEquipmentTypes[gDepotTransferItem]]
                                               [gUnits[OBJ.partyIdx1].class])) {
               // "You gave it back to yourself."
               DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
               DrawText(20, 20, 19, 2, 0, gTextPointers[51]);
               obj->state = 56;
               obj->state2 = 0;
            } else if (OBJ.partyIdx1 == 13) {
               if (gDepotItemIsEquipped == 0) {
                  obj->state = 58;
                  obj->state2 = 0;
               } else {
                  obj->state = 57;
                  obj->state2 = 0;
               }
            } else if (gEquipmentTypeClassCapability[gItemEquipmentTypes[gDepotTransferItem]]
                                                    [gUnits[OBJ.partyIdx1].class] &&
                       gDepotTransferItem != ITEM_V_HEART &&
                       !(OBJ.partyIdx1 == UNIT_ASH &&
                         gPartyMembers[UNIT_ASH].weapon == ITEM_V_HEART &&
                         gItemEquipmentTypes[gDepotTransferItem] <= EQUIPMENT_TYPE_V_HEART)) {
               SlideWindowTo(0x3d, 350, 170);
               CloseWindow(0x3d);
               SlideWindowTo(0x3e, 240, 168);
               if (gDepotItemIsEquipped != 0) {
                  obj->state = 62;
                  obj->state2 = 0;
               } else {
                  if (OBJ.partyIdx1 == gDepotTransferSrcMember) {
                     obj->state = 64;
                     obj->state2 = 0;
                  } else {
                     obj->state = 63;
                     obj->state2 = 0;
                  }
               }
            } else {
               if (OBJ.partyIdx1 == gDepotTransferSrcMember &&
                   gItemEquipmentTypes[gDepotTransferItem] <= EQUIPMENT_TYPE_V_HEART) {
                  // "You gave it back to yourself."
                  DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
                  DrawText(20, 20, 19, 2, 0, gTextPointers[51]);
                  obj->state = 56;
                  obj->state2 = 0;
               } else if (gDepotItemIsEquipped != 0) {
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[52]);
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[53]);
         gState.depot[gDepotTransferItem]++;
         gPartyMembers[gDepotTransferSrcMember].items[gDepotTransferSrcSlot - 4] = ITEM_NULL;
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[54]);
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[55]);
         if (gDepotTransferFromDepot != 0) {
            gState.depot[gDepotTransferItem]--;
         } else {
            gPartyMembers[gDepotTransferSrcMember].items[gDepotTransferSrcSlot - 4] = ITEM_NULL;
         }
         if (gPartyMembers[OBJ.partyIdx1].items[0] == ITEM_NULL) {
            gPartyMembers[OBJ.partyIdx1].items[0] = gDepotTransferItem;
         } else {
            gPartyMembers[OBJ.partyIdx1].items[1] = gDepotTransferItem;
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[56]);
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
         DrawText(20, 20, 20, 2, 0, gTextPointers[57]);
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
            DrawText(20, 20, 20, 2, 0, gTextPointers[59]);
            i = gItemEquipmentTypes[gDepotTransferItem];
            if (i <= EQUIPMENT_TYPE_V_HEART) {
               gPartyMembers[gDepotTransferSrcMember].weapon = gPartyMembers[OBJ.partyIdx1].weapon;
               gPartyMembers[OBJ.partyIdx1].weapon = gDepotTransferItem;
            } else if (i <= EQUIPMENT_TYPE_V_HELM) {
               gPartyMembers[gDepotTransferSrcMember].helmet = gPartyMembers[OBJ.partyIdx1].helmet;
               gPartyMembers[OBJ.partyIdx1].helmet = gDepotTransferItem;
            } else {
               gPartyMembers[gDepotTransferSrcMember].armor = gPartyMembers[OBJ.partyIdx1].armor;
               gPartyMembers[OBJ.partyIdx1].armor = gDepotTransferItem;
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
            DrawText(20, 20, 20, 2, 0, gTextPointers[60]);
         } else {
            // "Equipment exchange completed."
            DrawText(20, 20, 20, 2, 0, gTextPointers[61]);
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
            DrawText(20, 20, 20, 2, 0, gTextPointers[63]);
            i = gItemEquipmentTypes[gDepotTransferItem];
            if (i <= EQUIPMENT_TYPE_V_HEART) {
               j = gPartyMembers[OBJ.partyIdx1].weapon;
               gPartyMembers[OBJ.partyIdx1].weapon = gDepotTransferItem;
            } else if (i < EQUIPMENT_TYPE_ARMOR) {
               j = gPartyMembers[OBJ.partyIdx1].helmet;
               gPartyMembers[OBJ.partyIdx1].helmet = gDepotTransferItem;
            } else {
               j = gPartyMembers[OBJ.partyIdx1].armor;
               gPartyMembers[OBJ.partyIdx1].armor = gDepotTransferItem;
            }
            if (gDepotTransferFromDepot != 0) {
               gState.depot[gDepotTransferItem]--;
               gState.depot[j]++;
            } else {
               if (gDepotTransferSrcSlot == 4) {
                  gPartyMembers[gDepotTransferSrcMember].items[0] = j;
               } else {
                  gPartyMembers[gDepotTransferSrcMember].items[1] = j;
               }
            }
            DrawSmallEquipmentWindow(OBJ.partyIdx1);
            obj->state2++;
         } else if (gWindowChoice.raw == 0x3702 || gWindowChoice.raw == 0x37ff) {
            CloseWindow(0x37);
            if (obj->state == 64) {
               // "You gave it back to yourself."
               DrawWindow(0x34, 0, 0, 312, 90, 4, 10, WBS_DRAGON, 0);
               DrawText(20, 20, 19, 2, 0, gTextPointers[64]);
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
      gItemListWindowId = 0;
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
      gSupplyMenuTimer = 40;
      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 91:

      switch (obj->state2) {
      case 0:
         if (--gSupplyMenuTimer == 0) {
            FadeOutScreen(2, 6);
            gSupplyMenuTimer = 50;
            obj->state2++;
         }
         break;

      case 1:
         if (--gSupplyMenuTimer == 0) {
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
      DrawSjisText(28, 110, 19, 2, 0, sInventoryBuffer);
   }
   if (OBJ.shopNeedsRedraw) {
      OBJ.shopNeedsRedraw = 0;
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
         gDisplayedGold = gState.gold;
         {
            DrawWindow(0x36, 256, 98, 104, 36, 408, 90, WBS_CROSSED, 0);
            EmbedIntAsSjis(gDisplayedGold, sGoldBuffer, 6);
            DrawSjisText(256, 109, 10, 2, 0, sGoldBuffer);
         }
         DisplayCustomWindow(0x36, 0, 1, 49, 0, 0);
         SlideWindowTo(0x36, 208, 90);
         OBJ.goldState++;
      }
      break;

   case 2:
      if (gDisplayedGold != gState.gold) {
         gDisplayedGold = gState.gold;
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

   if (gItemListWindowId == 0x40) {
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

   if (gItemListWindowId == 0x3f) {
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

   if (gItemListWindowId == 0x3a) {
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
