/* Battle/field odds and ends (segment 0x27dd4): LoadUnits (each unit's 0x3000-byte sprite strip
 * into VRAM, bracketed by SaveItemIcons/RestoreItemIcons, whose stride is read THROUGH
 * sUnitsWithDefeatSpeech[8] -- a retail data overlap), overlay parking, and shared predicates. */
#include "common.h"
#include "object.h"
#include "battle.h"
#include "units.h"
#include "state.h"
#include "field.h"
#include "window.h"
#include "graphics.h"
#include "cd_files.h"

typedef struct ItemIconStripInfo {
   RECT rect;
   u8 unk_0x8;
   u8 unk_0x9;
} ItemIconStripInfo;

static ItemIconStripInfo sItemIconStripInfo[10] = {
    {{700, 352, 4, 144}, 128, 4}, {{764, 352, 4, 144}, 128, 4}, {{828, 0, 4, 240}, 128, 7},
    {{828, 256, 4, 240}, 128, 7}, {{892, 0, 4, 240}, 128, 7},   {{892, 256, 4, 240}, 128, 7},
    {{956, 0, 4, 240}, 128, 7},   {{956, 256, 4, 240}, 128, 7}, {{1020, 0, 4, 240}, 128, 7},
    {{1020, 256, 4, 240}, 128, 7}};

static u8 sUnitsWithDefeatSpeech[] = {
    UNIT_ASH,    UNIT_CLINT,  UNIT_DIEGO,  UNIT_ELENI,  UNIT_HUXLEY, UNIT_KIRA,  UNIT_GROG,
    UNIT_DOLAN,  UNIT_AMON,   UNIT_SARA,   UNIT_ZOHAR,  UNIT_DARIUS, UNIT_CLIVE, UNIT_LEENA,
    UNIT_ZOOT,   32,          UNIT_HASSAN, UNIT_MAGNUS, UNIT_LANDO,  UNIT_DUMAS, UNIT_KANE,
    UNIT_SABINA, UNIT_DALLAS, UNIT_KURTZ,  UNIT_XENO,   UNIT_DOLF,   0};
/* 0, 14, UNIT_ASH, UNIT_GROG, UNIT_AMON, UNIT_SARA, UNIT_ZOOT, UNIT_HASSAN, 16, UNIT_MAGNUS,
 * UNIT_LANDO, UNIT_DUMAS, UNIT_KANE, UNIT_KURTZ, UNIT_SABINA, 26, 28, 44, 41, 47, 48, 42, 45, 0,
 * 0};*/

//?: How to reproduce overlap with sUnitsWithDefeatSpeech[]?
// static s16 sItemIconStripIntervalBytes = 0xa09;
// static s16 sItemIconStripIntervalBytes[] = {0xa09};

void SaveItemIcons(void) {
   s32 i;
   RECT rect;
   u8 *p = gScratch1_801317c0;

   for (i = 0; i < ARRAY_COUNT(sItemIconStripInfo); i++) {
      rect.x = sItemIconStripInfo[i].rect.x;
      rect.y = sItemIconStripInfo[i].rect.y;
      rect.w = sItemIconStripInfo[i].rect.w;
      rect.h = sItemIconStripInfo[i].rect.h;
      StoreImage(&rect, p);
      // FIXME:
      // p += sItemIconStripIntervalBytes;
      p += *(s16 *)&sUnitsWithDefeatSpeech[8];
   }
   DrawSync(0);
}

void RestoreItemIcons(void) {
   s32 i;
   RECT rect;
   u8 *p = gScratch1_801317c0;

   for (i = 0; i < ARRAY_COUNT(sItemIconStripInfo); i++) {
      rect.x = sItemIconStripInfo[i].rect.x;
      rect.y = sItemIconStripInfo[i].rect.y;
      rect.w = sItemIconStripInfo[i].rect.w;
      rect.h = sItemIconStripInfo[i].rect.h;
      LoadImage(&rect, p);
      // FIXME:
      // p += sItemIconStripIntervalBytes;
      p += *(s16 *)&sUnitsWithDefeatSpeech[8];
   }
   DrawSync(0);
}

void LoadUnits(void) {
   s32 encodedIdx, i, j;
   s32 m;
   s32 fileSubUnitCt;
   u8 *p;
   s32 offset;
   u8 *pStripsStart;
   void *pSheets[6];
   u32 *pSheetSizes;
   u32 *pSheet;
   u32 *pSheetSize;
   u32 *pDstIter;
   u32 *pSrcIter;
   s32 copiedWords;
   s32 providedId, requestedId;
   RECT rect;

   const s32 BYTES_PER_STRIP = 0x3000;
   const s32 UNITS_PER_FILE = 6;

   // Sprite strip locations in VRAM
   RECT stripRects[23] = {{640, 352, 0, 0}, {640, 400, 0, 0}, {640, 448, 0, 0}, {768, 0, 0, 0},
                          {768, 48, 0, 0},  {768, 96, 0, 0},  {768, 144, 0, 0}, {768, 192, 0, 0},
                          {768, 256, 0, 0}, {768, 304, 0, 0}, {768, 352, 0, 0}, {768, 400, 0, 0},
                          {768, 448, 0, 0}, {896, 0, 0, 0},   {896, 48, 0, 0},  {896, 96, 0, 0},
                          {896, 144, 0, 0}, {896, 192, 0, 0}, {896, 256, 0, 0}, {896, 304, 0, 0},
                          {896, 352, 0, 0}, {896, 400, 0, 0}, {896, 448, 0, 0}};

   // UNIT_DB_CT is also 144, but this is probably coincidental.
   s32 unitFiles[144] = {
       CDF_UNIT_00_DAT, CDF_UNIT_01_DAT, CDF_UNIT_02_DAT, CDF_UNIT_03_DAT, CDF_UNIT_04_DAT,
       CDF_UNIT_05_DAT, CDF_UNIT_06_DAT, CDF_UNIT_07_DAT, CDF_UNIT_08_DAT, CDF_UNIT_09_DAT,
       CDF_UNIT_0A_DAT, CDF_UNIT_0B_DAT, CDF_UNIT_0C_DAT, CDF_UNIT_0D_DAT, CDF_UNIT_0E_DAT,
       CDF_UNIT_0F_DAT, CDF_UNIT_10_DAT, CDF_UNIT_11_DAT, CDF_UNIT_12_DAT, CDF_UNIT_13_DAT,
       CDF_UNIT_14_DAT, CDF_UNIT_15_DAT, CDF_UNIT_16_DAT, CDF_UNIT_17_DAT, CDF_UNIT_18_DAT,
       CDF_UNIT_19_DAT, CDF_UNIT_1A_DAT, CDF_UNIT_1B_DAT, CDF_UNIT_1C_DAT, CDF_UNIT_1D_DAT,
       CDF_UNIT_1E_DAT, CDF_UNIT_1F_DAT, CDF_UNIT_20_DAT, CDF_UNIT_21_DAT, CDF_UNIT_22_DAT,
       CDF_UNIT_23_DAT, CDF_UNIT_24_DAT, CDF_UNIT_25_DAT, CDF_UNIT_26_DAT, CDF_UNIT_27_DAT,
       CDF_UNIT_28_DAT, CDF_UNIT_29_DAT, CDF_UNIT_2A_DAT, CDF_UNIT_2B_DAT, CDF_UNIT_2C_DAT,
       CDF_UNIT_2D_DAT, CDF_UNIT_2E_DAT, CDF_UNIT_2F_DAT, CDF_UNIT_30_DAT, CDF_UNIT_31_DAT,
       CDF_UNIT_32_DAT, CDF_UNIT_33_DAT, CDF_UNIT_34_DAT, CDF_UNIT_35_DAT, CDF_UNIT_36_DAT,
       CDF_UNIT_37_DAT, CDF_UNIT_38_DAT, CDF_UNIT_39_DAT, CDF_UNIT_3A_DAT, CDF_UNIT_3B_DAT,
       CDF_UNIT_3C_DAT, CDF_UNIT_3D_DAT, CDF_UNIT_3E_DAT, CDF_UNIT_3F_DAT, CDF_UNIT_40_DAT,
       CDF_UNIT_41_DAT, CDF_UNIT_42_DAT, CDF_UNIT_43_DAT, CDF_UNIT_44_DAT, CDF_UNIT_45_DAT,
       CDF_UNIT_46_DAT, CDF_UNIT_47_DAT, CDF_UNIT_48_DAT, CDF_UNIT_49_DAT, CDF_UNIT_4A_DAT,
       CDF_UNIT_4B_DAT, CDF_UNIT_4C_DAT, CDF_UNIT_4D_DAT, CDF_UNIT_4E_DAT, CDF_UNIT_4F_DAT,
       CDF_UNIT_50_DAT, CDF_UNIT_51_DAT, CDF_UNIT_52_DAT, CDF_UNIT_53_DAT, CDF_UNIT_54_DAT,
       CDF_UNIT_55_DAT, CDF_UNIT_56_DAT, CDF_UNIT_57_DAT, CDF_UNIT_58_DAT, CDF_UNIT_59_DAT,
       CDF_UNIT_5A_DAT, CDF_UNIT_5B_DAT, CDF_UNIT_5C_DAT, CDF_UNIT_5D_DAT, CDF_UNIT_5E_DAT,
       CDF_UNIT_5F_DAT, CDF_UNIT_60_DAT, CDF_UNIT_61_DAT, CDF_UNIT_62_DAT, CDF_UNIT_63_DAT,
       CDF_UNIT_64_DAT, CDF_UNIT_65_DAT, CDF_UNIT_66_DAT, CDF_UNIT_67_DAT, CDF_UNIT_68_DAT,
       CDF_UNIT_69_DAT, CDF_UNIT_6A_DAT, CDF_UNIT_6B_DAT, CDF_UNIT_6C_DAT, CDF_UNIT_6D_DAT,
       CDF_UNIT_6E_DAT, CDF_UNIT_6F_DAT, CDF_UNIT_70_DAT, CDF_UNIT_71_DAT, CDF_UNIT_72_DAT,
       CDF_UNIT_73_DAT, CDF_UNIT_74_DAT, CDF_UNIT_75_DAT, CDF_UNIT_76_DAT, CDF_UNIT_77_DAT,
       CDF_UNIT_78_DAT, CDF_UNIT_79_DAT, CDF_UNIT_7A_DAT, CDF_UNIT_7B_DAT, CDF_UNIT_7C_DAT,
       CDF_UNIT_7D_DAT, CDF_UNIT_7E_DAT, CDF_UNIT_7F_DAT, CDF_UNIT_80_DAT, CDF_UNIT_81_DAT,
       CDF_UNIT_82_DAT, CDF_UNIT_83_DAT, CDF_UNIT_84_DAT, CDF_UNIT_85_DAT, CDF_UNIT_86_DAT,
       CDF_UNIT_87_DAT, CDF_UNIT_88_DAT, CDF_UNIT_89_DAT, CDF_UNIT_8A_DAT, CDF_UNIT_8B_DAT,
       CDF_UNIT_8C_DAT, CDF_UNIT_8D_DAT, CDF_UNIT_8E_DAT, CDF_UNIT_8F_DAT};

   // Equipment icons are stored in the gaps between sprite strips and will be overwritten by any
   // loaded sprite strips
   SaveItemIcons();

   if (gState.mapNum == 39) {
      gUnitDataPtr = gScratch3_80180210 + 0x3a300;
   } else {
      gUnitDataPtr = gScratch3_80180210 + 0x2db80;
   }

   p = gUnitDataPtr;
   encodedIdx = 0;
   rect.w = 512 >> 2;
   rect.h = 48;

   // Iterate through 144 available unit files; Each unit file contains data for 6 "sub-units", each
   // with its own ID; gCurrentUnitSet is a set of 20 such IDs to load.

   for (i = 0; i < ARRAY_COUNT(unitFiles); i++) {
      // Check if any of this unit file's 6 provided IDs are in the requested set (gCurrentUnitSet)
      m = 0;
      for (j = 0; j < ARRAY_COUNT(gCurrentUnitSet); j++) {
         requestedId = gCurrentUnitSet[j];

         for (providedId = i * UNITS_PER_FILE + 1;
              providedId < i * UNITS_PER_FILE + 1 + UNITS_PER_FILE; providedId++) {
            if (requestedId == providedId) {
               m++;
               break;
            }
         }

         if (m) {
            break;
         }
      }

      if (m) {
         // Unit file contains a requested ID
         LoadCdFile(unitFiles[i], 1);

         fileSubUnitCt = gScratch3_80180210[2] + gScratch3_80180210[3];
         pSheetSizes = (u32 *)&gScratch3_80180210[4];
         offset = fileSubUnitCt * 4 + 4;

         for (j = 0; j < fileSubUnitCt; j++) {
            pSheets[j] = &gScratch3_80180210[offset];
            offset += *pSheetSizes++;
         }

         // Data for sprite strips starts after the (compressed) sprite sheets data
         pStripsStart = &gScratch3_80180210[offset];

         for (j = 0; j < ARRAY_COUNT(gCurrentUnitSet); j++) {
            if (gCurrentUnitSet[j] == 0) {
               if (gUnitSetEncodedSpriteDataIdx[j] != -1) {
                  gEncodedUnitSpriteData[encodedIdx] = NULL;
                  gUnitSetEncodedSpriteDataIdx[j] = -1;

                  encodedIdx++;
                  if (encodedIdx == 20) {
                     goto Finish;
                  }

                  continue; //-> to next j
               }
            }

            for (m = 0; m < UNITS_PER_FILE; m++) {
               providedId = i * UNITS_PER_FILE + m + 1;
               if (providedId == 0x361) {
                  break;
               }

               if (gCurrentUnitSet[j] == providedId) {
                  rect.x = stripRects[j].x;
                  rect.y = stripRects[j].y;
                  LoadImage(&rect, &pStripsStart[m * BYTES_PER_STRIP]);
                  gEncodedUnitSpriteData[encodedIdx] = p;
                  gUnitSetEncodedSpriteDataIdx[j] = encodedIdx;

                  pSheet = pSheets[m];
                  pSrcIter = pSheet;
                  pDstIter = (u32 *)p;
                  pSheetSize = &pSheet[0];

                  for (copiedWords = 0; copiedWords < (*pSheetSize / sizeof(u32)); copiedWords++) {
                     *pDstIter++ = *pSrcIter++;
                  }

                  encodedIdx++;
                  p += *pSheetSize;

                  if (encodedIdx == 20) {
                     goto Finish;
                  }
                  break; //-> to next j
               }
            } // for (m)
         }    // for (j)

         DrawSync(0);
      }
   } // for (i)

Finish:
   DrawSync(0);
   RestoreItemIcons();
}

s32 PressedCircleOrX_2(void) {
   if ((gPadStateNewPresses & PAD_CIRCLE) || (gPadStateNewPresses & PAD_X)) {
      return 1;
   } else {
      return 0;
   }
}

/* Test-and-clear of gState.msgBoxPagePaused, which core/text.c raises when the message box
 * fills a page or finishes -- the dojo/town/tavern state machines advance on it. */
s32 ConsumeMsgBoxPagePause(void) {
   if (gState.msgBoxPagePaused != 0) {
      gState.msgBoxPagePaused = 0;
      return 1;
   } else {
      return 0;
   }
}

s32 MsgBox_IsFinished(void) { return gState.msgBoxFinished != 0; }

s32 PressedCircleOrX(void) {
   if ((gPadStateNewPresses & PAD_CIRCLE) || (gPadStateNewPresses & PAD_X)) {
      return 1;
   } else {
      return 0;
   }
}

extern u8 additional_VRAM[]; // Linker symbol generated by splat for the 'additional' code segment

/* Parks the .additional overlay code segment (RAM) into four spare VRAM rects at boot
 * (LoadImage = RAM->VRAM); FetchOverlayCodeFromVram brings it back before any state that
 * runs that code. Renamed from Swap{In,Out}... whose directions read backwards. */
void StashOverlayCodeToVram(void) {
   gTempRect.w = 64;
   gTempRect.h = 256;
   gTempRect.x = 512;
   gTempRect.y = 256;
   LoadImage(&gTempRect, &additional_VRAM[0]);
   gTempRect.h = 128;
   gTempRect.x = 576;
   gTempRect.y = 384;
   // 64 * 256 * 2
   LoadImage(&gTempRect, &additional_VRAM[0x8000]);
   gTempRect.w = 60;
   gTempRect.h = 144;
   gTempRect.x = 896;
   gTempRect.y = 352;
   // (64 * 256 * 2) + (64 * 128 * 2)
   LoadImage(&gTempRect, &additional_VRAM[0xc000]);
   gTempRect.x = 960;
   // (64 * 256 * 2) + (64 * 128 * 2) + (60 * 144 * 2)
   LoadImage(&gTempRect, &additional_VRAM[0x10380]);
   DrawSync(0);
}

void FetchOverlayCodeFromVram(void) {
   gTempRect.w = 64;
   gTempRect.h = 256;
   gTempRect.x = 512;
   gTempRect.y = 256;
   StoreImage(&gTempRect, &additional_VRAM[0]);
   gTempRect.h = 128;
   gTempRect.x = 576;
   gTempRect.y = 384;
   // 64 * 256 * 2
   StoreImage(&gTempRect, &additional_VRAM[0x8000]);
   gTempRect.w = 60;
   gTempRect.h = 144;
   gTempRect.x = 896;
   gTempRect.y = 352;
   // (64 * 256 * 2) + (64 * 128 * 2)
   StoreImage(&gTempRect, &additional_VRAM[0xc000]);
   gTempRect.x = 960;
   // (64 * 256 * 2) + (64 * 128 * 2) + (60 * 144 * 2)
   StoreImage(&gTempRect, &additional_VRAM[0x10380]);
   DrawSync(0);
}

s32 HasDefeatSpeech(UnitStatus *unit) {
   s32 i = 0;

   if (gState.mapNum == 8) {
      // No defeat speeches on demo map
      return 0;
   }

   while (sUnitsWithDefeatSpeech[i] != UNIT_NULL) {
      if (unit->name == sUnitsWithDefeatSpeech[i]) {
         if (gState.mapNum <= 7) {
            // On Trials of Toroah, only party members should have defeat speeches.
            if (unit->name <= UNIT_END_OF_PARTY) {
               return 1;
            }
         } else {
            return 1;
         }
      }
      i++;
   }

   return 0;
}

s32 IsSpriteOutsideVisibleRange(Object *sprite) {
   if ((sprite->x1.s.hi >= gMapViewOriginX) && (sprite->x1.s.hi <= gMapSizeX + gMapViewOriginX - 1) &&
       (sprite->z1.s.hi >= gMapViewOriginZ) && (sprite->z1.s.hi <= gMapSizeZ + gMapViewOriginZ - 1)) {
      return 0;
   } else {
      return 1;
   }
}

s32 UnitIsRocky(UnitStatus *unit) {
   // "Rocky" units will burst into rubble instead of blood.
   if (unit->unitId == UNIT_ID_GOLD_GOLEM || unit->unitId == UNIT_ID_EVILSTATUE ||
       unit->unitId == UNIT_ID_MAGE_TOWER || unit->unitId == UNIT_ID_DEATH_DEV ||
       unit->unitId == UNIT_ID_CLAY_GOLEM || unit->unitId == UNIT_ID_DARK_GOLEM ||
       unit->unitId == UNIT_ID_GUARDIAN || unit->unitId == UNIT_ID_BASILISK ||
       unit->unitId == UNIT_ID_MEGA_GUARD || unit->unitId == UNIT_ID_M_CANNON) {
      return 1;
   } else {
      return 0;
   }
}
