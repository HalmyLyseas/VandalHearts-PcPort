#include "common.h"
#include "window.h"
#include "card.h"
#include "object.h"
#include "state.h"
#include "audio.h"
#include "cd_files.h"


extern void DrawText(s32 x, s32 y, s32 maxCharsPerLine, s32 lineSpacing, s32 color, u8 *text);

#ifdef PC_FEAT
/* Adopts a loaded save's mode from its card-header marker before applying it. */
extern void PC_AdoptSaveMode(void);
#endif

static s8 *sText_InsertCard[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xf0\x83\x58\x83\x8d\x83\x62\x83\x67\x82\x50\x82\xc9", // メモリーカードをスロット１に
    "\x81\x40\x81\x40\x81\x40\x91\x7d\x93\xfc\x82\xb5\x82\xc4\x89\xba\x82\xb3\x82\xa2", // 　　　挿入して下さい
    NULL,
};

static s8 *sText_YesNo[] = {
    "\x82\x78\x82\x64\x82\x72", // ＹＥＳ
    "\x82\x6d\x82\x6e\x81\x40", // ＮＯ　
    NULL,
};

static s8 *sText_AskFormatCard[] = {
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xb3\x82\xea\x82\xc4\x82\xa2\x82\xc8\x82\xa2", // フォーマットされていない
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xc5\x82\xb7\x81\x42", // メモリーカードです。
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81\x48", // フォーマットしますか？
    NULL,
};

static s8 *sText_CannotDetectCard[] = {
    "\x83\x4a\x81\x5b\x83\x68\x82\xf0\x94\x46\x8e\xaf\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1", // カードを認識できません
    NULL,
};

static s8 *sText_Formatting[] = {
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xb5\x82\xc4\x82\xa2\x82\xdc\x82\xb7\x81\x45\x81\x45\x81\x45", // フォーマットしています・・・
    NULL,
};

static s8 *sText_FormattingCompleted[] = {
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xaa\x8f\x49\x97\xb9\x82\xb5\x82\xdc\x82\xb5\x82\xbd", // フォーマットが終了しました
    NULL,
};

static s8 *sText_FormattingUnsuccessful[] = {
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x82\xc5\x82\xb5\x82\xbd", // フォーマットできませんでした
    NULL,
};

static s8 *sText_NoEmptyBlocks[] = {
    "\x8b\xf3\x82\xab\x83\x75\x83\x8d\x83\x62\x83\x4e\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb9\x82\xf1", // 空きブロックがありません
    NULL,
};

static s8 *sText_SelectSaveLocation[] = {
    "\x83\x5a\x81\x5b\x83\x75\x82\xb7\x82\xe9\x8f\xea\x8f\x8a\x82\xf0\x91\x49\x82\xf1\x82\xc5\x89\xba\x82\xb3\x82\xa2", // セーブする場所を選んで下さい
    NULL,
};

static s8 *sText_Saving[] = {
    "\x83\x5a\x81\x5b\x83\x75\x82\xb5\x82\xc4\x82\xa2\x82\xdc\x82\xb7\x81\x45\x81\x45\x81\x45", // セーブしています・・・
    NULL,
};

static s8 *sText_SaveUnsuccessful[] = {
    "\x83\x5a\x81\x5b\x83\x75\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x82\xc5\x82\xb5\x82\xbd", // セーブできませんでした
    NULL,
};

static s8 *sText_SaveCompleted[] = {
    "\x83\x5a\x81\x5b\x83\x75\x82\xb5\x82\xdc\x82\xb5\x82\xbd", // セーブしました
    NULL,
};

static s8 *sText_AskDoInBattleSave[] = {
    "\x83\x52\x83\x93\x83\x65\x83\x42\x83\x6a\x83\x85\x81\x5b\x83\x5a\x81\x5b\x83\x75\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81\x48", // コンティニューセーブしますか？
    NULL,
};

static s8 *sText_AskSaveData[] = {
    "\x83\x5a\x81\x5b\x83\x75\x82\xb5\x82\xdc\x82\xb7\x82\xa9\x81\x48", // セーブしますか？
    NULL,
};

static s8 *sText_FileSaveCaptions[] = {
    "",
    "",
    "",
    NULL,
};

#ifdef PERMUTER
/* PC build only: [29] so the implicit NUL lives inside the object. Reading it is not a behavior
 * change -- hardware's byte at +28 is a real 0x00 (SLPM_860.07 @0x801042f4) -- it only stops
 * strcpy()/DrawText() walking off the C object. See docs/memory-safety.md, "AddressSanitizer". */
static s8 sEmptyFileCaption[29] = "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x96\xa2\x8e\x67\x97\x70\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40";
#else
static s8 sEmptyFileCaption[28] = "\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x96\xa2\x8e\x67\x97\x70\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40\x81\x40"; // 　　　　　未使用　　　　　　 (28B exact fit, no NUL — terminated by the next symbol's zero byte)
#endif

static s8 *sUnused_80102350[] = {
    NULL,
    NULL,
    NULL,
};

static s8 *sText_SelectDataToLoad[] = {
    "\x83\x8d\x81\x5b\x83\x68\x82\xb7\x82\xe9\x83\x66\x81\x5b\x83\x5e\x82\xf0\x91\x49\x82\xf1\x82\xc5\x89\xba\x82\xb3\x82\xa2", // ロードするデータを選んで下さい
    NULL,
};

static s8 *sUnused_80102364[] = {
    "", "", "", "", "", NULL,
};

static s8 *sText_Loading[] = {
    "\x83\x8d\x81\x5b\x83\x68\x82\xb5\x82\xc4\x82\xa2\x82\xdc\x82\xb7\x81\x45\x81\x45\x81\x45", // ロードしています・・・
    NULL,
};

static s8 *sText_LoadUnsuccessful[] = {
    "\x83\x8d\x81\x5b\x83\x68\x82\xc5\x82\xab\x82\xdc\x82\xb9\x82\xf1\x82\xc5\x82\xb5\x82\xbd", // ロードできませんでした
    NULL,
};

// Unused
static s8 *sText_LoadCompleted[] = {
    "\x83\x8d\x81\x5b\x83\x68\x82\xb5\x82\xdc\x82\xb5\x82\xbd", // ロードしました
    NULL,
};

static s8 *sText_CardNotFormatted[] = {
    "\x83\x74\x83\x48\x81\x5b\x83\x7d\x83\x62\x83\x67\x82\xb3\x82\xea\x82\xc4\x82\xa2\x82\xc8\x82\xa2", // フォーマットされていない
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xc5\x82\xb7", // メモリーカードです
    NULL,
};

static s8 *sText_CannotReadData[] = {
    "\x83\x66\x81\x5b\x83\x5e\x82\xaa\x93\xc7\x82\xdf\x82\xdc\x82\xb9\x82\xf1", // データが読めません
    NULL,
};

static s8 *sText_NoSavedData[] = {
    "\x83\x5a\x81\x5b\x83\x75\x83\x66\x81\x5b\x83\x5e\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb9\x82\xf1", // セーブデータがありません
    NULL,
};

static s8 *sText_FromCardOrBattleStart[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xa9\x82\xe7\x83\x8d\x81\x5b\x83\x68", // メモリーカードからロード
    "\x81\x40\x82\xb1\x82\xcc\x83\x7d\x83\x62\x83\x76\x82\xcc\x8d\xc5\x8f\x89\x82\xa9\x82\xe7\x81\x40", // 　このマップの最初から　
    NULL,
};

static s8 *sText_FileLoadCaptions[] = {
    "",
    "",
    "",
#ifdef PC_PORT
    /* This array is genuinely 4 wide (the title-screen Load path reads index 3); with 3 entries
     * the read runs off the end and DrawSjisText dereferences garbage at 64-bit. See
     * docs/width-bugs.md, "Found only by building and running". */
    "\x83\x52\x83\x93\x83\x65\x83\x42\x83\x6a\x83\x85\x81\x5b\x83\x8d\x81\x5b\x83\x68",
#endif
};

// Unused
static s8 *sText_InBattleSaveOrBattleStart[] = {
    "\x83\x52\x83\x93\x83\x65\x83\x42\x83\x6a\x83\x85\x81\x5b\x83\x8d\x81\x5b\x83\x68", // コンティニューロード
    "\x82\xb1\x82\xcc\x83\x7d\x83\x62\x83\x76\x82\xcc\x8d\xc5\x8f\x89\x82\xa9\x82\xe7", // このマップの最初から
};

static s8 *sText_MainMenuChoices[] = {
    "\x83\x58\x83\x5e\x81\x5b\x83\x67", // スタート
    "\x83\x8d\x81\x5b\x83\x68\x81\x40", // ロード　
    "\x90\xdd\x92\xe8\x81\x40\x81\x40", // 設定　　
    NULL,
};

static s8 *sText_CheckingCard[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xf0\x83\x60\x83\x46\x83\x62\x83\x4e\x92\x86\x81\x45\x81\x45", // メモリーカードをチェック中・・
    NULL,
};

static s8 *sText_CardNotPresent[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xaa\x82\xa0\x82\xe8\x82\xdc\x82\xb9\x82\xf1", // メモリーカードがありません
    NULL,
};

static s8 *sText_CannotReadCard[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xaa\x93\xc7\x82\xdf\x82\xdc\x82\xb9\x82\xf1", // メモリーカードが読めません
    NULL,
};

static s8 *sText_AskStartWithoutCard[] = {
    "\x83\x81\x83\x82\x83\x8a\x81\x5b\x83\x4a\x81\x5b\x83\x68\x82\xc8\x82\xb5\x82\xc5", // メモリーカードなしで
    "\x83\x51\x81\x5b\x83\x80\x82\xf0\x82\xcd\x82\xb6\x82\xdf\x82\xdc\x82\xb7\x82\xa9", // ゲームをはじめますか
    NULL,
};

static s8 sUnused_801230c8[] = "";

static s8 *sText_Options[] = {
    "\x95\xb6\x8e\x9a\x83\x58\x83\x73\x81\x5b\x83\x68", // 文字スピード
    "\x83\x54\x83\x45\x83\x93\x83\x68", // サウンド
    NULL,
};

static s8 *sText_TextSpeedOptions[] = {
    "\x91\xac\x82\xa2", // 速い
    "\x95\x81\x92\xca", // 普通
    "\x92\x78\x82\xa2", // 遅い
    NULL,
};

static s8 *sText_SoundOptions[] = {
    "\x83\x58\x83\x65\x83\x8c\x83\x49", // ステレオ
    "\x83\x82\x83\x6d\x83\x89\x83\x8b", // モノラル
    NULL,
};

// Unused
static s8 *sText_CameraOptions[] = {
    "\x83\x5f\x83\x43\x83\x69\x83\x7e\x83\x62\x83\x4e", // ダイナミック
    "\x83\x74\x83\x42\x83\x62\x83\x4e\x83\x58", // フィックス
    NULL,
};

// Unused (JP replaces the US language menu with raw data)
static u8 sUnused_LanguageData[16] = {
    0x2b, 0x01, 0x00, 0x00, 0x2c, 0x01, 0x00, 0x00,
    0x23, 0x0a, 0x24, 0x0a, 0x25, 0x0a, 0x24, 0x05,
};

void DrawTextWindow(s8 **lines, s32 lineCount, s32 windowId, s32 x, s32 y, s32 dispX_, s32 dispY_,
                    s32 borderStyle, s32 numChoices, s32 centered) {
   s32 i;
   s32 textWidth;
   s32 paddedWidth;
   s32 padding;
   s32 dispX, dispY;
   s32 halfPadding;

   if (borderStyle == WBS_CROSSED) {
      textWidth = strlen(lines[0]) / 2;
      textWidth *= 12;
      paddedWidth = textWidth / 8 * 8 + 16;
      padding = paddedWidth - textWidth;
      halfPadding = padding / 8 * 4;
      dispX = dispX_;
      dispY = dispY_;
      if (centered) {
         dispX = SCREEN_HALF_WIDTH - (paddedWidth + 16) / 2;
      }

      DrawWindow(windowId, x, y, paddedWidth + 16, lineCount * 18 + 18, dispX, dispY, borderStyle,
                 numChoices);
      for (i = 0; i < lineCount; i++) {
         DrawText(halfPadding + x + 8, (i * 18) + y + 11, 20, 0, 0, lines[i]);
      }
   } else {
      textWidth = strlen(lines[0]) / 2;
      textWidth *= 12;
      paddedWidth = textWidth / 8 * 8 + 16;
      padding = paddedWidth - textWidth;
      halfPadding = padding / 8 * 4;
      dispX = dispX_;
      dispY = dispY_;
      if (centered) {
         dispX = SCREEN_HALF_WIDTH - (paddedWidth + 32) / 2;
      }

      DrawWindow(windowId, x, y, paddedWidth + 32, lineCount * 18 + 36, dispX, dispY, borderStyle,
                 numChoices);
      for (i = 0; i < lineCount; i++) {
         DrawText(halfPadding + x + 16, (i * 18) + y + 20, 20, 0, 0, lines[i]);
      }
   }
}

void ResetCardFileListing(void) {
   s32 i;

   for (i = 0; i < 4; i++) {
      gCardFileListingPtr->slotOccupied[i] = 0;
   }
   for (i = 0; i < 3; i++) {
      strcpy(gCardFileListingPtr->captions[i], sEmptyFileCaption);
   }
}

#undef OBJF
#define OBJF 341
void Objf341_342_353_FileSaveMenu(Object *obj) {
   extern s8 *gCardSlotCaptions[3];

   s32 i;
   s32 tmp; //?

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         obj->x1.n = 50;
         obj->y1.n = 50;
         gClearSavedPadState = 1;
         OBJ.savedWindowId = gWindowActiveIdx;
         obj->state2++;

      // fallthrough
      case 1:

         switch (obj->functionIndex) {
         case OBJF_FILE_SAVE_MENU:
            obj->state2 = 0;
            obj->state += 2;
            break;

         case OBJF_FILE_SAVE_MENU_IBS:
            obj->state2 = 0;
            obj->state += 3;
            break;

         default:
         case OBJF_FILE_SAVE_MENU_UNK:
            obj->state2++;
            break;
         }

         break;

      case 2:
         if (++obj->state3 >= 128) {
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 1:

      switch (obj->state2) {
      case 0:
         if (obj->functionIndex == OBJF_FILE_SAVE_MENU_UNK) {
            obj->state2++;
         } else {
            obj->state++;
         }
         break;

      case 1:
         DrawTextWindow(sText_AskSaveData, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 0);
         DisplayBasicWindow(0x3c);
         DimScreen();
         obj->state2++;

      // fallthrough
      case 2:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 9;
         DrawTextWindow(sText_YesNo, 2, 61, 0, 54, obj->x1.n + 180, obj->y1.n, 0, 2, 0);
         DisplayBasicWindow(0x3d);
         gWindowActiveIdx = 0x3d;
         obj->state2++;
         break;

      case 3:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state2 = 0;
            obj->state += 2;
            break;

         case 2:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state3 = 1;
            obj->state2 = 0;
            UndimScreen();
            obj->state = 101;
            break;
         }

         break;
      }

      break;

   case 2:

      switch (obj->state2) {
      case 0:
         FadeInScreen(2, 10);
         obj->state2++;
         break;

      case 1:
         if (++obj->state3 >= 25) {
            obj->state2++;
         }
         break;

      case 2:
         obj->state3 = 0;
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 3:
      OBJ.error = Card_ReadFileListing();
      if (OBJ.error == 0) {
         for (i = 0; i < 3; i++) {
            if (gCardFileListingPtr->slotOccupied[i]) {
               gCardSlotCaptions[i] = gCardFileListingPtr->captions[i];
            } else {
               gCardSlotCaptions[i] = sEmptyFileCaption;
            }
         }
         obj->state = 14;
      } else {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 4:
      if (OBJ.error == -3) {
         obj->state++;
      } else {
         obj->state += 2;
      }
      break;

   case 5:

      switch (obj->state2) {
      case 0:
         DrawTextWindow(sText_InsertCard, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 1:
         if (gPadStateNewPresses & PAD_CIRCLE) {
            OBJ.error = Card_CheckState();
            if (OBJ.error == -3) {
               obj->state2 = 0;
            }
            CloseWindow(0x3c);
            obj->state = 3;
            obj->state2 = 0;
         } else if (gPadStateNewPresses & PAD_X) {
            CloseWindow(0x3c);
            obj->state3 = 1;
            obj->state2 = 0;
            obj->state = 99;
         }
         break;
      }

      break;

   case 6:
      obj->state++;

   // fallthrough
   case 7:
      if (OBJ.error == -2) {
         if (++obj->mem >= 5) {
            obj->state++;
         }
      } else {
         obj->state += 2;
         break;
      }

   // fallthrough
   case 8:

      switch (obj->state2) {
      case 0:
         DrawTextWindow(sText_CannotDetectCard, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 1:
         if (gPadStateNewPresses & PAD_X) {
            obj->state3 = 1;
            obj->state = 99;
            obj->state2 = 0;
            CloseWindow(0x3c);
         } else if (gPadStateNewPresses & PAD_CIRCLE) {
            obj->state2 = 0;
            CloseWindow(0x3c);
            obj->state = 3;
         }
         break;
      }

      break;

   case 9:
      if (OBJ.error == -4) {
         obj->state++;
      } else {
         obj->state += 2;
      }
      break;

   case 10:

      switch (obj->state2) {
      case 0:
         obj->mem = 0;
         obj->state2++;
         break;

      case 1:
         DrawTextWindow(sText_AskFormatCard, 3, 0x3c, 0, 0, obj->x1.n, obj->y1.n - 20, 0, 0, 1);
         obj->state2++;
         break;

      case 2:
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 3:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 9;
         DrawTextWindow(sText_YesNo, 2, 0x3d, 52, 160, obj->x1.n, obj->y1.n + 50, 0, 2, 1);
         DisplayBasicWindow(0x3d);
         gWindowActiveIdx = 0x3d;
         obj->state2++;
         break;

      case 4:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
            obj->state2++;
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            break;

         case 255:
         case 2:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state3 = 1;
            obj->state = 99;
            break;
         }

         break;

      case 5:
         DrawTextWindow(sText_Formatting, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;

      // fallthrough
      case 6:
         OBJ.error = Card_Format();
         obj->state2++;
         break;

      case 7:
         CloseWindow(0x3c);
         if (OBJ.error == 0) {
            obj->state2 += 3;
         } else {
            obj->state2++;
         }
         break;

      case 8:
         DrawTextWindow(sText_FormattingUnsuccessful, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 9:
         if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
            CloseWindow(0x3c);
            obj->state3 = 1;
            obj->state = 99;
         }
         break;

      case 10:
         DrawTextWindow(sText_FormattingCompleted, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         obj->mem = 0;
         break;

      case 11:
         if (++obj->mem >= 64) {
            obj->state2++;
         }
         break;

      case 12:
         CloseWindow(0x3c);
         obj->state2 = 0;
         obj->state = 3;
         break;
      }

      break;

   case 11:

      switch (obj->state2) {
      case 0:
         //@c6c
         tmp = OBJ.error;
         if (OBJ.error == 0) {
            obj->state++;
         } else {
            obj->state2++;
         }
         break;

      case 1:
         OBJ.error = Card_CountFreeBlocks();
         if (OBJ.error < 2) {
            obj->state2++;
         } else {
            obj->state2 += 3;
         }
         break;

      case 2:
         DrawTextWindow(sText_NoEmptyBlocks, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         gWindowActiveIdx = 0;
         obj->state2++;
         break;

      case 3:
         if (gPadStateNewPresses & PAD_CIRCLE) {
            CloseWindow(0x3c);
            obj->state2 = 0;
            obj->state3 = 0;
            obj->state = 3;
         } else if (gPadStateNewPresses & PAD_X) {
            CloseWindow(0x3c);
            obj->state2 = 0;
            obj->state3 = 1;
            obj->state = 99;
         }
         break;

      case 4:
         ResetCardFileListing();
         for (i = 0; i < 3; i++) {
            gCardSlotCaptions[i] = sEmptyFileCaption;
         }
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 12:
      obj->state2 = 0;
      obj->state += 2;
      break;

   case 13:
      obj->state++;
      break;

   case 14:

      //@db0
      switch (obj->functionIndex) {
      case OBJF_FILE_SAVE_MENU:
         obj->state++;
         break;

      case OBJF_FILE_SAVE_MENU_IBS:
         obj->state += 2;
         break;

         // case OBJF_FILE_SAVE_MENU_UNK:
         //    obj->state++;
         //    break;

      default:
         if (obj->functionIndex == OBJF_FILE_SAVE_MENU_UNK) {
            obj->state++;
         } else {
            obj->state = 99;
            obj->state3 = 1;
         }
         break;
      }

      break;

   case 15:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 9;
         DrawTextWindow(sText_SelectSaveLocation, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0,
                        0);
         DisplayBasicWindow(0x3c);
         for (i = 0; i < 3; i++) {
            sText_FileSaveCaptions[i] = gCardSlotCaptions[i];
         }
         DrawTextWindow(sText_FileSaveCaptions, 3, 0x3d, 0, 54, obj->x1.n, obj->y1.n + 50, 0, 3, 1);
         DisplayBasicWindow(0x3d);
         obj->state2++;
         break;

      case 1:
         gClearSavedPadState = 1;
         gWindowActiveIdx = 0x3d;
         obj->state2++;
         break;

      case 2:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
         case 2:
         case 3:
            OBJ.choice = gWindowChoice.u.choice;
            gWindowActiveIdx = 0x3c;
            obj->state2++;
            break;

         case 255:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state3 = 1;
            obj->state = 99;
            break;
         }

         break;

      case 3:
         CloseWindow(0x3c);
         gWindowActiveIdx = 0;
         Card_UpdateCaption(OBJ.choice - 1);
         gCardFileListingPtr->slotOccupied[OBJ.choice - 1] = 1;
         for (i = 0; i < 3; i++) {
            if (gCardFileListingPtr->slotOccupied[i]) {
               gCardSlotCaptions[i] = gCardFileListingPtr->captions[i];
            } else {
               gCardSlotCaptions[i] = sEmptyFileCaption;
            }
         }
         DrawTextWindow(sText_Saving, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0, 0);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         obj->mem = 0;
         break;

      case 4:
         if (++obj->mem >= 3) {
            obj->state2++;
            obj->mem = 0;
         }
         break;

      case 5:
         OBJ.error = Card_WriteFileListing();
         if (OBJ.error != 0) {
            obj->mem = 0;
            obj->state2++;
         } else {
            OBJ.error = Card_WriteRegularSave(OBJ.choice - 1);
            obj->mem = 0;
            obj->state2++;
         }
         break;

      case 6:
         CloseWindow(0x3c);
         if (OBJ.error == 0) {
            obj->state2 += 2;
         } else {
            obj->state2++;
         }
         break;

      case 7:
         DrawTextWindow(sText_SaveUnsuccessful, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0, 0);
         DisplayBasicWindow(0x3c);
         obj->state2 = 9;
         obj->state3 = 1;
         break;

      case 8:
         CloseWindow(0x3d);
         tmp = OBJ.choice - 1;
         gWindowChoicesTopMargin = tmp * 18 + 9;
         for (i = 0; i < 3; i++) {
            sText_FileSaveCaptions[i] = gCardSlotCaptions[i];
         }
         DrawTextWindow(sText_FileSaveCaptions, 3, 0x3d, 0, 54, obj->x1.n, obj->y1.n + 50, 0, 3, 1);
         DisplayBasicWindow(0x3d);
         DrawTextWindow(sText_SaveCompleted, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0, 0);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         obj->state3 = 2;
         obj->mem = 0;

      // fallthrough
      case 9:
         if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state = 99;
         }
         break;
      }

      break;

   case 16:

      switch (obj->state2) {
      case 0:
         obj->mem = 0;
         obj->state2++;

      // fallthrough
      case 1:
         DrawTextWindow(sText_AskDoInBattleSave, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         obj->state2++;

      // fallthrough
      case 2:
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 3:
         DrawTextWindow(sText_YesNo, 2, 0x3d, 0, 72, obj->x1.n, obj->y1.n + 50, 0, 2, 0);
         DisplayBasicWindow(0x3d);
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 36;
         gWindowActiveIdx = 0x3d;
         obj->state2++;
         break;

      case 4:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
            obj->state2++;
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            break;

         case 2:
         case 255:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state3 = 1;
            obj->state = 99;
            break;
         }

         break;

      case 5:
         gCardFileListingPtr->slotOccupied[3] = 1;
         OBJ.error = Card_WriteFileListing();
         if (OBJ.error == 0) {
            OBJ.error = Card_WriteInBattleSave();
         }
         obj->state2++;
         break;

      case 6:
         if (OBJ.error == 0) {
            DrawTextWindow(sText_SaveCompleted, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            obj->state3 = 2;
         } else {
            DrawTextWindow(sText_SaveUnsuccessful, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            obj->state3 = 1;
         }
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 7:
         if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
            CloseWindow(0x3c);
            obj->state = 99;
         }
         break;
      }

      break;

   case 90:
   case 99:
      obj->state2 = 0;

      switch (obj->functionIndex) {
      case OBJF_FILE_SAVE_MENU:
         FadeOutScreen(2, 6);
         obj->state++;
         break;

      case OBJF_FILE_SAVE_MENU_UNK:
         if (obj->state3 != 0) {
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state = 1;
         } else {
            obj->state += 2;
         }
         break;

      default:
      case OBJF_FILE_SAVE_MENU_IBS:
         obj->state += 2;
         break;
      }

      break;

   case 100:
      if (++obj->state2 >= 45) {
         obj->state++;
      }
      break;

   case 101:
      gWindowActiveIdx = OBJ.savedWindowId;
      gClearSavedPadState = 0;
      obj->functionIndex = OBJF_NULL;
      gState.subObjDone = obj->state3;
      break;
   }
}

#undef OBJF
#define OBJF 343
void Objf343_Etc_FileLoadMenu(Object *obj) {
   // 343, 360, 367, 373, 374, 376

   s32 i;
   s32 numChoices;
   s32 currentChoice;
   s32 textWidth;
   s32 paddedWidth;
   s32 padding;
   s32 halfPadding;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         obj->x1.n = 50;
         obj->y1.n = 50;
         OBJ.savedSeqId = GetCurrentSeqId();
         gClearSavedPadState = 1;
         OBJ.savedWindowId = gWindowActiveIdx;
         if (obj->functionIndex == OBJF_FILE_LOAD_MENU_IBS ||
             obj->functionIndex == OBJF_FILE_LOAD_MENU_DEFEAT) {
            obj->state++;
         } else {
            FadeOutScreen(2, 255);
            obj->state2++;
         }
         break;

      case 1:
         FadeInScreen(2, 10);
         obj->state2++;
         break;

      case 2:
         if (++obj->mem >= 30) {
            obj->mem = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 1:

      switch (obj->state2) {
      case 0:
         if (obj->functionIndex == OBJF_FILE_LOAD_MENU_IBS ||
             obj->functionIndex == OBJF_FILE_LOAD_MENU_DEFEAT) {
            obj->state2++;
         } else {
            obj->state++;
         }
         break;

      case 1:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 9;
         if (obj->functionIndex == OBJF_FILE_LOAD_MENU_DEFEAT) {
            DrawTextWindow(sText_FromCardOrBattleStart, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n + 76, 0,
                           2, 1);
         } else {
            DrawTextWindow(sText_FromCardOrBattleStart, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n + 30, 0,
                           2, 1);
         }
         DisplayBasicWindow(0x3c);
         gWindowActiveIdx = 0x3c;
         obj->state2++;
         break;

      case 2:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
            obj->state2 = 0;
            obj->state++;
            CloseWindow(0x3c);
            gWindowActiveIdx = 0;
            break;

         case 2:
            CloseWindow(0x3c);
            gWindowActiveIdx = 0;
            ReloadBattle();
            obj->state3 = 0;
            obj->state = 99;
            break;

         case 255:
            if (obj->functionIndex != OBJF_FILE_LOAD_MENU_DEFEAT) {
               CloseWindow(0x3c);
               gWindowActiveIdx = 0;
               obj->state = 99;
               obj->state3 = 1;
            }
            break;
         }

         break;
      }

      break;

   case 2:

      switch (obj->state2) {
      case 0:
         obj->state2++;

      // fallthrough
      case 1:
         OBJ.error = Card_ReadFileListing();
         obj->state2++;
         obj->mem = 0;
         obj->state3 = 0;
         break;

      case 2:
         if (OBJ.error == -3) {
            obj->state2++;
         } else {
            obj->state2 += 3;
         }
         break;

      case 3:
         DrawTextWindow(sText_InsertCard, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 4:
         if (gPadStateNewPresses & PAD_CIRCLE) {
            CloseWindow(0x3c);
            OBJ.error = Card_ReadFileListing();
            if (OBJ.error == -3) {
               obj->state3 = 0;
               obj->state2 = 1;
            } else {
               obj->state2++;
            }
         } else if (gPadStateNewPresses & PAD_X) {
            CloseWindow(0x3c);
            if (obj->functionIndex == OBJF_FILE_LOAD_MENU_DEFEAT ||
                obj->functionIndex == OBJF_FILE_LOAD_MENU_IBS) {
               obj->state = 1;
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
            } else {
               obj->state3 = 1;
               obj->state = 99;
            }
         }
         break;

      case 5:
         if (OBJ.error == 0) {
            obj->state++;
            obj->state2 = 0;
         } else {
            obj->state3 = 1;
            obj->state = 90;
         }
         break;
      }

      break;

   case 3:
      for (i = 0; i < 4; i++) {
         if (gCardFileListingPtr->slotOccupied[i]) {
            OBJ.slotOccupied[i] = 1;
         }
      }

      switch (obj->functionIndex) {
      case OBJF_FILE_LOAD_MENU_343:
      case OBJF_FILE_LOAD_MENU_367:
      default:
         OBJ.numChoices = 3;
         break;
      case OBJF_FILE_LOAD_MENU_DEBUG:
      case OBJF_FILE_LOAD_MENU:
      case OBJF_FILE_LOAD_MENU_IBS:
      case OBJF_FILE_LOAD_MENU_DEFEAT:
         OBJ.numChoices = 4;
         break;
      }
      obj->state++;

   // fallthrough
   case 4:

      switch (obj->state2) {
      case 0:
         OBJ.error = Card_ReadFileListing();
         if (OBJ.error != 0) {
            obj->state3 = 1;
            obj->state = 90;
         } else {
            for (i = 0; i < 3; i++) {
               if (gCardFileListingPtr->slotOccupied[i]) {
                  sText_FileLoadCaptions[i] = gCardFileListingPtr->captions[i];
               } else {
                  sText_FileLoadCaptions[i] = sEmptyFileCaption;
               }
            }
            gWindowChoiceHeight = 18;
            gWindowChoicesTopMargin = 9;
            DrawTextWindow(sText_SelectDataToLoad, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0,
                           0);
            DisplayBasicWindow(0x3c);
            numChoices = OBJ.numChoices;
            paddedWidth = strlen(sText_FileLoadCaptions[0]) / 2;
            textWidth = paddedWidth * 12;
            paddedWidth = textWidth / 8 * 8 + 16;
            padding = paddedWidth - textWidth;
            halfPadding = padding / 8 * 4;

            gWindowChoiceHeight = 18;
            gWindowChoicesTopMargin = 9;
            DrawWindow(0x3d, 0, 54, paddedWidth + 16, numChoices * 18 + 18,
                       SCREEN_HALF_WIDTH - (paddedWidth + 16) / 2, obj->y1.n + 50, WBS_CROSSED,
                       numChoices);
            for (i = 0; i < numChoices; i++) {
               if (i < 4 && !OBJ.slotOccupied[i]) {
                  DrawText(halfPadding + 8, (i * 18) + 63, 20, 0, 1, sText_FileLoadCaptions[i]);
               } else {
                  DrawText(halfPadding + 8, (i * 18) + 63, 20, 0, 0, sText_FileLoadCaptions[i]);
               }
            }
            obj->state2++;
         }
         break;

      case 1:
         gClearSavedPadState = 1;
         DisplayBasicWindow(0x3d);
         gWindowActiveIdx = 0x3d;
         obj->state2++;
         break;

      case 2:
         currentChoice = gWindowChoice.u.choice;

         switch (currentChoice) {
         case 0:
            break;

         case 1:
         case 2:
         case 3:
         case 4:
            if (!OBJ.slotOccupied[currentChoice - 1]) {
               break;
            }

         // fallthrough
         case 5:
            OBJ.choice = currentChoice;
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->state2++;
            break;

         case 255:
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            if (obj->functionIndex == OBJF_FILE_LOAD_MENU) {
               obj->state3 = 1;
               obj->state = 99;
            } else {
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
               obj->state = 1;
            }
            break;
         }

         break;

      case 3:
         OBJ.error = Card_ReadFileListing();
         if (OBJ.error != 0) {
            obj->state3 = 1;
            obj->state = 90;
         } else {
            for (i = 0; i < 3; i++) {
               if (gCardFileListingPtr->slotOccupied[i]) {
                  sText_FileLoadCaptions[i] = gCardFileListingPtr->captions[i];
               } else {
                  sText_FileLoadCaptions[i] = sEmptyFileCaption;
               }
            }
            gWindowChoicesTopMargin = (OBJ.choice - 1) * 18 + 9;
            numChoices = OBJ.numChoices;
            paddedWidth = strlen(sText_FileLoadCaptions[0]) / 2;
            textWidth = paddedWidth * 12;
            paddedWidth = textWidth / 8 * 8 + 16;
            padding = paddedWidth - textWidth;
            halfPadding = padding / 8 * 4;
            DrawWindow(0x3d, 0, 54, paddedWidth + 16, numChoices * 18 + 18,
                       SCREEN_HALF_WIDTH - (paddedWidth + 16) / 2, obj->y1.n + 50, WBS_CROSSED,
                       numChoices);
            for (i = 0; i < numChoices; i++) {
               if (i < 4 && !OBJ.slotOccupied[i]) {
                  DrawText(halfPadding + 8, (i * 18) + 63, 20, 0, 1, sText_FileLoadCaptions[i]);
               } else {
                  DrawText(halfPadding + 8, (i * 18) + 63, 20, 0, 0, sText_FileLoadCaptions[i]);
               }
            }
            DisplayBasicWindow(0x3d);
            DrawTextWindow(sText_Loading, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0, 0);
            DisplayBasicWindow(0x3c);
            gWindowActiveIdx = 0x3c;
            obj->state2++;
            obj->state3 = 0;
            obj->mem = 0;
         }
         break;

      case 4:

         switch (OBJ.choice) {
         case 0:
         case 1:
         case 2:
         case 3:

            switch (obj->state3) {
            case 0:
               obj->mem = 0;
               PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
               obj->state3++;

            // fallthrough
            default:
            case 1:
               if (++obj->mem >= 32) {
                  #ifdef PC_FEAT
                  PC_AdoptSaveMode();   /* GAP 4: mode = the card's marker, before the save is applied */
#endif
                  OBJ.error = Card_LoadRegularSave(OBJ.choice - 1);
                  CloseWindow(0x3c);
                  obj->state2++;
                  obj->mem = 0;
                  obj->state3 = 0;
               }
               break;
            }

            break;

         case 4:

            switch (obj->state3) {
            case 0:
               PerformAudioCommand(AUDIO_CMD_FADE_OUT_32_4);
               obj->state3++;
               obj->mem = 0;
               break;

            case 1:
               if (++obj->mem >= 32) {
                  #ifdef PC_FEAT
                  PC_AdoptSaveMode();   /* GAP 4: mode = the card's marker, before the save is applied */
#endif
                  OBJ.error = Card_LoadInBattleSave();
                  CloseWindow(0x3c);
                  obj->state2++;
               }
               break;

            default:
               CloseWindow(0x3c);
               obj->state2++;
               break;
            }

            break;

         case 5:
            OBJ.error = ReloadBattle();
            CloseWindow(0x3d);
            CloseWindow(0x3c);
            obj->state2++;
            break;
         }

         break;

      case 5:
         if (OBJ.error == 0) {
            CloseWindow(0x3d);
            return;
         }

         obj->state3 = 1;
         obj->state2++;

      // fallthrough
      case 6:
         DrawTextWindow(sText_LoadUnsuccessful, 1, 0x3c, 0, 0, obj->x1.n - 20, obj->y1.n, 0, 0, 0);
         DisplayBasicWindow(0x3c);
         obj->state3 = 1;
         obj->state2 = 8;
         break;

      case 7:
         obj->state3 = 0;
         obj->state2++;
         obj->mem = 0;

      // fallthrough
      case 8:
         if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
            CloseWindow(0x3c);
            CloseWindow(0x3d);
            obj->mem = 0;
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state = 1;
         }
         break;
      }

      break;

   case 90:
      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 91:

      switch (obj->state2) {
      case 0:

         switch (OBJ.error) {
         case -4:
            DrawTextWindow(sText_CardNotFormatted, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n - 20, 0, 0,
                           1);
            break;

         case 2:
         case -2:
            DrawTextWindow(sText_CannotReadData, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            break;

         default:
         case 1:
            DrawTextWindow(sText_NoSavedData, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            break;
         }

         DisplayBasicWindow(0x3c);
         obj->state2++;
         break;

      case 1:
         if (gPadStateNewPresses & PAD_CIRCLE) {
            CloseWindow(0x3c);
            obj->state2++;
         } else if (gPadStateNewPresses & PAD_X) {
            if (obj->functionIndex != OBJF_FILE_LOAD_MENU_DEFEAT) {
               CloseWindow(0x3c);
               obj->state = 99;
               obj->state3 = 1;
            } else {
               CloseWindow(0x3c);
               obj->state2++;
            }
         }
         break;

      case 2:
         obj->mem = 0;
         obj->state3 = 0;
         obj->state2 = 0;
         obj->state = 1;
         break;

      default:
         obj->state = 99;
         break;
      }

      break;

   case 99:
      obj->state2 = 0;
      if (obj->functionIndex == OBJF_FILE_LOAD_MENU_IBS ||
          obj->functionIndex == OBJF_FILE_LOAD_MENU_DEFEAT) {
         obj->state += 2;
      } else {
         FadeOutScreen(2, 6);
         obj->state++;
      }
      break;

   case 100:
      if (++obj->state2 >= 45) {
         obj->state++;
      }
      break;

   case 101:
      if (GetCurrentSeqId() == 0) {
         PerformAudioCommand(OBJ.savedSeqId);
      }
      gWindowActiveIdx = OBJ.savedWindowId;
      gClearSavedPadState = 0;
      gState.subObjDone = obj->state3;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void Noop_800b5118(void){};

void ShowFileLoadScreen_Unused(void) {
   Object *obj;

   switch (gState.secondary) {
   case 0:
      Obj_ResetFromIdx10();
      LoadFullscreenImage(CDF_US_LOAD_TIM);
      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
      gState.secondary++;
      break;

   case 1:
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FILE_LOAD_MENU_DEFEAT;
      gState.subObjDone = 0;
      gState.secondary++;
      break;

   case 2:
      if (gState.subObjDone != 0) {
         gState.primary = gState.state6;
         gState.secondary = 0;
         gState.state3 = 0;
         gState.state4 = 0;
      }
      break;
   }
}

void State_TitleScreen(void) {
   Object *obj;

   switch (gState.secondary) {
   case 0:
      if (gState.state7 == 0) {
         LoadFWD();
      }
      Obj_ResetFromIdx10();
      FetchOverlayCodeFromVram();
      gState.vsyncMode = 0;
      gClearSavedPadState = 1;
      gState.fieldRenderingDisabled = 1;
      LoadFullscreenImage(CDF_US_TITLE_TIM);
      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
      gState.secondary++;
      gState.suppressLoadingScreen = 0;
      break;

   case 1:
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAIN_MENU;
      gState.subObjDone = 0;
      gState.secondary++;
      gState.state3 = 0;
      break;

   case 2:
      if (gPadState == 0) {
         // If no input, increment idle counter
         gState.state3++;
      } else {
         // Reset idle counter
         gState.state3 = 0;
      }
      if (gState.state3 >= 1500) {
         // No input for 25 seconds
         if (gState.state7 == 0) {
            // Enter demo mode
            gState.primary = STATE_27;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
            gState.mapNum = 8;
            gState.suppressLoadingScreen = 0;
         } else {
            // If demo battle was shown last, show intro movie
            gState.movieIdxToPlay = MOV_TITLE_WS_STR;
            gState.primary = STATE_MOVIE;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
            gState.suppressLoadingScreen = 0;
         }
      }
      break;
   }
}

#undef OBJF
#define OBJF 796
void Objf796_MainMenu(Object *obj) {
   static s16 cursorMemory = 0;

   s32 i;

   if (obj->state > 4) {
      // Disable idle counter in submenus
      gState.state3 = 0;
   }

   switch (obj->state) {
   case 0:
      obj->x1.n = 45;
      obj->y1.n = 120;
      OBJ.menuMem_main = cursorMemory;
      OBJ.menuMem_options = 0;
      FadeOutScreen(2, 255);
      obj->state++;
      break;

   case 1:
      FadeInScreen(2, 10);
      obj->state++;
      break;

   case 2:
      if (++obj->state3 >= 30) {
         obj->state++;
         obj->state3 = 0;
      }
      break;

   case 3:
      gWindowChoiceHeight = 18;
      gWindowChoicesTopMargin = 18;
      DrawTextWindow(sText_MainMenuChoices, 3, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 1, 3, 1);
      DisplayBasicWindowWithSetChoice(0x3c, OBJ.menuMem_main);
      gWindowActiveIdx = 0x3c;
      obj->state++;
      break;

   case 4:

      switch (gWindowChoice.u.choice) {
      case 0:
         break;

      case 1:
         OBJ.menuMem_main = 0;
         cursorMemory = 0;
         obj->state = 15;
         CloseWindow(0x3c);
         break;

      case 2:
         OBJ.menuMem_main = 1;
         cursorMemory = 1;
         obj->state = 20;
         CloseWindow(0x3c);
         break;

      case 3:
         OBJ.menuMem_main = 2;
         cursorMemory = 2;
         obj->state = 25;
         CloseWindow(0x3c);
         break;

      case 255:
         break;
      }

      break;

   case 15:

      switch (obj->state2) {
      case 0:
         DrawTextWindow(sText_CheckingCard, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
         DisplayBasicWindow(0x3c);
         OBJ.error = Card_ReadFileListing();
         obj->state2++;
         obj->mem = 0;
         obj->state3 = 0;
         break;

      case 1:
         if (++obj->state3 >= 60) {
            if (OBJ.error == 0) {
               CloseWindow(0x3c);
               obj->state2 = 0;
               obj->state3 = 0;
               obj->state++;
            } else {
               if (++obj->mem >= 4) {
                  obj->mem = 0;
                  obj->state3 = 0;
                  obj->state2++;
               } else {
                  OBJ.error = Card_ReadFileListing();
               }
            }
         }
         break;

      case 2:
         if (OBJ.error == -3) {
            obj->state3 = 0;
            obj->state2++;
         } else {
            obj->state2 += 2;
         }
         break;

      case 3:

         switch (obj->state3) {
         case 0:
            CloseWindow(0x3c);
            DrawTextWindow(sText_InsertCard, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            obj->state3++;
            break;

         case 1:
            if (gPadStateNewPresses & PAD_CIRCLE) {
               CloseWindow(0x3c);
               obj->mem = 0;
               obj->state3++;
            } else if (gPadStateNewPresses & PAD_X) {
               CloseWindow(0x3c);
               obj->state3 = 0;
               obj->state2 = 0;
               obj->state = 3;
            }
            break;

         case 2:
            OBJ.error = Card_ReadFileListing();
            if (OBJ.error != -3) {
               obj->state2++;
               obj->mem = 0;
               obj->state3 = 0;
            } else {
               if (++obj->mem >= 4) {
                  obj->state3++;
               }
            }
            break;

         case 3:
            DrawTextWindow(sText_CardNotPresent, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            obj->state3++;

         // fallthrough
         case 4:
            if (gPadStateNewPresses & PAD_CIRCLE) {
               CloseWindow(0x3c);
               obj->state2 = 10;
               obj->mem = 0;
               obj->state3 = 0;
            } else if (gPadStateNewPresses & PAD_X) {
               CloseWindow(0x3c);
               obj->state3 = 0;
               obj->state2 = 0;
               obj->state = 3;
            }
            break;
         }

         break;

      case 4:
         if (OBJ.error == -4) {
            obj->state2++;
            obj->state3 = 0;
         } else {
            obj->state2 += 2;
         }
         break;

      case 5:

         switch (obj->state3) {
         case 0:
            CloseWindow(0x3c);
            DrawTextWindow(sText_AskFormatCard, 3, 0x3c, 0, 0, obj->x1.n, obj->y1.n - 58, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            gWindowChoiceHeight = 18;
            gWindowChoicesTopMargin = 9;
            DrawTextWindow(sText_YesNo, 2, 0x3d, 0, 72, obj->x1.n + 50, obj->y1.n + 30, 0, 2, 1);
            DisplayBasicWindow(0x3d);
            gWindowActiveIdx = 0x3d;
            obj->state3++;
            break;

         case 1:

            switch (gWindowChoice.u.choice) {
            case 0:
               break;

            case 1:
               CloseWindow(0x3c);
               CloseWindow(0x3d);
               obj->state3++;
               gWindowActiveIdx = 0;
               break;

            case 2:
            case 255:
               CloseWindow(0x3c);
               CloseWindow(0x3d);
               gWindowActiveIdx = 0;
               obj->state2 = 10;
               obj->mem = 0;
               obj->state3 = 0;
               break;
            }

            break;

         case 2:
            DrawTextWindow(sText_Formatting, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 0);
            DisplayBasicWindow(0x3c);
            obj->state3++;

         // fallthrough
         case 3:
            OBJ.error = Card_Format();
            obj->state3++;

         // fallthrough
         case 4:
            CloseWindow(0x3c);
            if (OBJ.error == 0) {
               obj->state3 += 3;
            } else {
               obj->state3++;
            }
            break;

         case 5:
            DrawTextWindow(sText_FormattingUnsuccessful, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0,
                           1);
            DisplayBasicWindow(0x3c);
            obj->state3++;
            break;

         case 6:
            if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
               CloseWindow(0x3c);
               obj->state2 = 10;
               obj->mem = 0;
               obj->state3 = 0;
            }
            break;

         case 7:
            DrawTextWindow(sText_FormattingCompleted, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            obj->state3++;
            break;

         case 8:
            if (gPadStateNewPresses & (PAD_CIRCLE | PAD_X)) {
               CloseWindow(0x3c);
               obj->state++;
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
            }
            break;
         }

         break;

      case 6:
         if (OBJ.error != 1) {
            obj->state2 += 2;
         } else {
            obj->mem = 0;
            obj->state2++;
         }
         break;

      case 7:

         switch (obj->state3) {
         case 0:
            OBJ.numFreeBlocks = Card_CountFreeBlocks();
            if (OBJ.numFreeBlocks < 0) {
               if (++obj->mem >= 4) {
                  OBJ.error = OBJ.numFreeBlocks;
                  obj->state3++;
               }
            } else {
               obj->state3++;
            }
            break;

         case 1:
            if (OBJ.numFreeBlocks < 2) {
               obj->state3++;
            } else {
               CloseWindow(0x3c);
               obj->state++;
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
            }
            break;

         case 2:
            CloseWindow(0x3c);
            DrawTextWindow(sText_NoEmptyBlocks, 1, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            obj->state3++;
            break;

         case 3:
            if (gPadStateNewPresses & PAD_CIRCLE) {
               CloseWindow(0x3c);
               obj->state3 = 0;
               obj->state2 = 10;
            } else if (gPadStateNewPresses & PAD_X) {
               CloseWindow(0x3c);
               obj->state3 = 0;
               obj->state2 = 0;
               obj->state = 3;
            }
            break;
         }

         break;

      case 8:
         obj->mem = 0;
         obj->state3 = 0;
         obj->state2++;

      // fallthrough
      case 9:

         switch (obj->state3) {
         case 0:
            CloseWindow(0x3c);
            DrawTextWindow(sText_CannotReadCard, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 0, 0, 1);
            DisplayBasicWindow(0x3c);
            obj->state3++;

         // fallthrough
         case 1:
            if (gPadStateNewPresses & PAD_CIRCLE) {
               CloseWindow(0x3c);
               obj->state3 = 0;
               obj->state2++;
            } else if (gPadStateNewPresses & PAD_X) {
               CloseWindow(0x3c);
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
               obj->state = 3;
            }
            break;
         }

         break;

      case 10:

         switch (obj->state3) {
         case 0:
            DrawTextWindow(sText_AskStartWithoutCard, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n - 50, 0,
                           0, 1);
            DisplayBasicWindow(0x3c);
            gWindowChoicesTopMargin = 9;
            gWindowChoiceHeight = 18;
            DrawTextWindow(sText_YesNo, 2, 0x3d, 0, 72, obj->x1.n + 50, obj->y1.n + 30, 0, 2, 1);
            DisplayBasicWindow(0x3d);
            gWindowActiveIdx = 0x3d;
            obj->state3++;

         // fallthrough
         case 1:

            switch (gWindowChoice.u.choice) {
            case 0:
               break;

            case 1:
               CloseWindow(0x3c);
               CloseWindow(0x3d);
               gWindowActiveIdx = 0;
               obj->state++;
               break;

            case 2:
            case 255:
               CloseWindow(0x3c);
               CloseWindow(0x3d);
               gWindowActiveIdx = 0;
               obj->state = 3;
               obj->mem = 0;
               obj->state3 = 0;
               obj->state2 = 0;
               break;
            }

            break;
         }

         break;
      }

      break;

   case 16:
      ResetStateForNewGame();
      for (i = 0; i < PARTY_CT; i++) {
         gPartyMembers[i].inParty = 0;
         gPartyMembers[i].advChosePathB = 0;
         gPartyMembers[i].advLevelFirst = 0;
         gPartyMembers[i].advLevelSecond = 0;
      }
      gState.gold = 0;
      gState.frameCounter = 0;
      gState.primary = STATE_MOVIE;
      gState.movieIdxToPlay = MOV_1BU_WS_STR;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
      obj->functionIndex = OBJF_NULL;
      break;

   case 20:
      FadeOutScreen(2, 16);
      obj->state++;
      obj->state3 = 0;
      break;

   case 21:
      if (++obj->state3 >= 16) {
         gState.primary = STATE_TITLE_LOAD_SCREEN;
         gState.secondary = 0;
         gState.state3 = 0;
         gState.state4 = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 25:
      obj->state2 = 0;
      OBJ.menuMem_options = 0;
      obj->state++;
      break;

   case 26:

      switch (obj->state2) {
      case 0:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 18;
         DrawTextWindow(sText_Options, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 1, 2, 1);
         DisplayBasicWindowWithSetChoice(0x3c, OBJ.menuMem_options);
         gWindowActiveIdx = 0x3c;
         obj->state2++;
         break;

      case 1:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
            CloseWindow(0x3c);
            OBJ.menuMem_options = 0;
            obj->state2 = 10;
            gWindowActiveIdx = 0;
            break;

         case 2:
            CloseWindow(0x3c);
            OBJ.menuMem_options = 1;
            obj->state2 = 15;
            gWindowActiveIdx = 0;
            break;

         case 255:
            CloseWindow(0x3c);
            obj->state = 3;
            break;
         }

         break;

      case 10:
         gWindowChoiceHeight = 18;
         gWindowChoicesTopMargin = 18;
         DrawTextWindow(sText_TextSpeedOptions, 3, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 1, 3, 1);
         DisplayBasicWindowWithSetChoice(0x3c, 2 - gState.textSpeed);
         gWindowActiveIdx = 0x3c;
         obj->state2++;
         break;

      case 11:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
         case 2:
         case 3:
            gState.textSpeed = 3 - gWindowChoice.u.choice;
            gWindowActiveIdx = 0;
            CloseWindow(0x3c);
            obj->state2 = 0;
            break;

         case 255:
            CloseWindow(0x3c);
            obj->state2 = 0;
            break;
         }

         break;

      case 15:
         gWindowChoicesTopMargin = 18;
         gWindowChoiceHeight = 18;
         DrawTextWindow(sText_SoundOptions, 2, 0x3c, 0, 0, obj->x1.n, obj->y1.n, 1, 2, 1);
         DisplayBasicWindowWithSetChoice(0x3c, gState.mono);
         gWindowActiveIdx = 0x3c;
         obj->state2++;
         break;

      case 16:

         switch (gWindowChoice.u.choice) {
         case 0:
            break;

         case 1:
         case 2:
            gState.mono = gWindowChoice.u.choice - 1;
            PerformAudioCommand(!gState.mono ? AUDIO_CMD_STEREO : AUDIO_CMD_MONO);
            CloseWindow(0x3c);
            gWindowActiveIdx = 0;
            obj->state2 = 0;
            break;

         case 255:
            CloseWindow(0x3c);
            obj->state2 = 0;
            break;
         }

         break;
      }

      break;
   }
}

void State_Title_FileLoadScreen(void) {
   Object *obj;

   Noop_DebugPrintValue(20, 20, gState.primary);

   switch (gState.secondary) {
   case 0:
      Obj_ResetFromIdx10();
      LoadFullscreenImage(CDF_US_LOAD_TIM);
      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = OBJF_FULLSCREEN_IMAGE;
      gState.secondary++;
      break;

   case 1:
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FILE_LOAD_MENU;
      gState.subObjDone = 0;
      gState.secondary++;
      break;

   case 2:
      if (gState.subObjDone != 0) {
         gState.primary = STATE_TITLE_SCREEN;
         gState.secondary = 0;
         gState.state3 = 0;
         gState.state4 = 0;
         gState.suppressLoadingScreen = 1;
      }
      break;
   }
}
