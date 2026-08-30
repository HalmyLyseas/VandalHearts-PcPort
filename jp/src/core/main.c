#include "common.h"
#include "cd_files.h"
#include "audio.h"
#include "card.h"
#include "state.h"
#include "object.h"
#include "units.h"
#include "window.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
void main(void);
void UpdateState(void);
void Objf582_MainMenu_Jpn(Object *obj);
void Objf583_LoadingIndicator(Object *obj);
void Objf006_Logo(Object *obj);
void State_Init(void);
void State_EventScene(void);
void Objf584_DebugSceneSelect(Object *obj);

void main(void) {
   //__main(); // <- inserted automatically by gcc
   while (1) {
#ifdef PC_PORT
      /* Overlay RETURN TO TITLE is deferred to here -- the only point where no game code is
       * mid-frame -- so the state flip can't race a live loader (see pc_balance.c). Must run BEFORE
       * UpdateState: the title state's entry resets leftover objects before Obj_Execute runs one. */
      { extern void PC_ApplyReturnToTitle(void); PC_ApplyReturnToTitle(); }
#endif
      UpdateState();
      UpdateEngine();
   }
}

void UpdateState(void) {
   Object *obj;

   switch (gState.primary) {
   case STATE_0:
      gState.suppressLoadingScreen = 1;
      gUnitDataPtr = gScratch3_80180210 + 0x2db80;
      gRegularSaveDataPtr = gScratch1_801317c0 + 0x5000;
      gInBattleSaveDataPtr = gScratch1_801317c0 + 0x6000;
      gCardFileListingPtr = gScratch1_801317c0 + 0xa000;
      gState.textSpeed = 1;
      Initialize();
      LoadItemIcons();
      InitAudio();
      SetXaCdSectorOffset(0x3f35);
      LoadSoundSet(0);
      FinishLoadingVab();
      LoadSoundSet(1);
      FinishLoadingVab();
      LoadSoundSet(2);
      FinishLoadingVab();
      StashOverlayCodeToVram();
      gState.primary++;
   case STATE_1:
      gState.field_0x96 = 0;
      gUnitDataPtr = gScratch3_80180210 + 0x2db80;
      ClearPortraitSet();
      State_Init();
      break;
   case STATE_3:
   case STATE_LOAD_IN_BATTLE_SAVE:
   case STATE_27:
   case STATE_30:
   case STATE_31:
      State_Battle();
      break;
   case STATE_4:
   case STATE_16:
   case STATE_17:
   case STATE_25:
      State_EventScene();
      break;
   case STATE_SHOP:
   case STATE_DEPOT:
      State_ShopOrDepot();
      break;
   case STATE_6:
   case STATE_28:
      State_WorldMap();
      break;
   case STATE_7:
   case STATE_20:
      State_Town();
      break;
   case STATE_TAVERN:
      State_Tavern();
      break;
   case STATE_DOJO:
   case STATE_TRIAL_COMPLETE:
      State_Dojo();
      break;
   case STATE_MOVIE:
      State_Movie();
      break;
   case STATE_12:
   case STATE_18:
   case STATE_19:
   case STATE_26:
      State_SetupScene();
      break;
   case STATE_CHAPTER_COMPLETE:
      State_ChapterComplete();
      break;
   case STATE_ENDING_SCREEN:
      State_EndingScreen();
      break;
   case STATE_SET_SCENE_STATE:
      State_SetSceneState();
      break;
   case STATE_FILE_SAVE_SCREEN:
      State_FileSaveScreen();
      break;
   case STATE_FILE_LOAD_SCREEN:
      State_FileLoadScreen();
      break;
   case STATE_TITLE_SCREEN:
      State_TitleScreen();
      break;
   case STATE_TITLE_LOAD_SCREEN:
      State_Title_FileLoadScreen();
      break;
   case STATE_LOAD_DEBUG_MENU:
      Obj_ResetFromIdx10();
      FetchOverlayCodeFromVram();
      gState.vsyncMode = 0;
      gClearSavedPadState = 1;
      gState.fieldRenderingDisabled = 1;
      LoadFullscreenImage(CDF_US_TITLE_TIM);
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FULLSCREEN_IMAGE;
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_DEBUG_MENU;
      gState.primary = STATE_DEBUG_MENU;
   case STATE_DEBUG_MENU:
      break;
   }
}

void Objf582_MainMenu_Jpn(Object *obj) {
   // Left-over debugging stuff?
   Object *dialog;
   s32 i;

   switch (obj->state) {
   case 0:
      FadeInScreen(2, 0xff);
      gWindowChoiceHeight = 17;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 128, 54, 180, 8, WBS_CROSSED, 2);
      DrawSjisText(12, 11, 20, 2, 0,
                   "\x8e\x6e\x82\xdf\x82\xa9\x82\xe7\x0a\x83\x8d\x81\x5b\x83\x68");
      DisplayBasicWindow(0x34);
      obj->state++;
      obj->state2 = 0;
      break;
   case 1:
      gState.gold = 0;
      gState.frameCounter = 0;

      for (i = 0; i < DEPOT_CT; i++) {
         gState.depot[i] = ITEM_NULL;
      }

      if (gWindowChoice.s.windowId == 0x34 && gWindowChoice.s.choice != 0) {
         if (gWindowChoice.raw == 0x3401) {
            ResetStateForNewGame();
            gState.primary = STATE_MOVIE;
            gState.movieIdxToPlay = MOV_LOGO_USA_STR;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.raw == 0x3402) {
            CloseWindow(0x34);
            dialog = Obj_GetUnused();
            dialog->functionIndex = OBJF_FILE_LOAD_MENU_DEBUG;
            obj->state++;
            gState.subObjDone = 0;
         }
      }
      break;
   case 2:
      if (gState.subObjDone != 0) {
         gState.primary = STATE_LOAD_DEBUG_MENU;
      }
      break;
   }
}

void Objf583_LoadingIndicator(Object *obj) {
   Object *spr = Obj_GetUnused();

   spr->d.sprite.gfxIdx = GFX_NOW_LOADING;
   spr->x1.n = 120;
   spr->y1.n = 95;
   spr->x3.n = spr->x1.n + 80;
   spr->y3.n = spr->y1.n + 50;
   AddObjPrim_Gui(gGraphicsPtr->ot, spr);

   spr->d.sprite.gfxIdx = GFX_VANDAL_HEARTS;
   spr->x1.n = 156;
   spr->y1.n = 184;
   spr->x3.n = spr->x1.n + 128;
   spr->y3.n = spr->y1.n + 32;
   AddObjPrim_Gui(gGraphicsPtr->ot, spr);
}

void Objf006_Logo(Object *obj) {
   obj->d.sprite.gfxIdx = GFX_VANDAL_HEARTS;
   obj->d.sprite.otOfs = 2;
   obj->d.sprite.clut = CLUT_BLUES;
   obj->x1.n = 12;
   obj->y1.n = 50;
   obj->x3.n = obj->x1.n + 128;
   obj->y3.n = obj->y1.n + 32;
   AddObjPrim_Gui(gGraphicsPtr->ot, obj);
}

void State_Init(void) {
   FetchOverlayCodeFromVram();
   LoadPortraits();
   gClearSavedPadState = 0;
   gIsEnemyTurn = 0;
   gPlayerControlSuppressed = 0;
   gMapCursorSuppressed = 0;
   gSignal1 = 0;
   gSignal2 = 0;
   gMapCursorX = 0;
   gMapCursorZ = 0;
   gState.inEvent = 0;
   gState.previewingRange = 0;
   gSavedPadState = 0;
   gSavedPad2State = 0;
   gSavedPadStateNewPresses = 0;
   gSavedPad2StateNewPresses = 0;
   gPadState = 0;
   gPad2State = 0;
   gPadStateNewPresses = 0;
   gPad2StateNewPresses = 0;
   gWindowChoice.raw = 0;
   gWindowActivatedChoice.raw = 0;
   gMapDataPtr = gScratch3_80180210;
   gGraphicsPtr = &gGraphicBuffers[0];
   ClearUnits();
   Obj_ResetAll();
   LoadFWD();
   SetupGfx();

   gTempGfxObj = Obj_GetFirstUnused();
   gTempGfxObj->functionIndex = OBJF_NOOP;

   gTempObj = Obj_GetFirstUnused();
   gTempObj->functionIndex = OBJF_NOOP_407;

   gTempObj = Obj_GetFirstUnused();
   gTempObj->functionIndex = OBJF_EVENT_CAMERA;

   gTempObj = Obj_GetFirstUnused();
   gTempObj->functionIndex = OBJF_MENU_CHOICE;

   gTempObj = Obj_GetFirstUnused();
   gTempObj->functionIndex = OBJF_SCREEN_EFFECT;
   gState.screenEffect = gTempObj;

   gDecodingSprites = 0;
   gState.fieldRenderingDisabled = 1;
   gState.vsyncMode = 2;
   gOverheadMapState = 0;

   if (gState.debug) {
      gState.primary = STATE_LOAD_DEBUG_MENU;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
   } else {
      gState.primary = STATE_MOVIE;
      gState.secondary = 0;
      gState.state3 = 0;
      gState.state4 = 0;
      gState.movieIdxToPlay = MOV_LOGO_USA_STR;
   }

   gState.suppressLoadingScreen = 0;
   SetDispMask(1);
}

void State_EventScene(void) {
   s32 i;

   switch (gState.secondary) {
   case 0:
      gState.suppressLoadingScreen = 1;
      FadeOutScreen(2, 0xff);
      ResetGeomOffset();
      gState.vsyncMode = 2;
      gState.preciseSprites = 0;
      Obj_ResetFromIdx10();
      ClearUnits();
      gIsEnemyTurn = 0;
      gCameraRotation.vx = 0x180;
      gClearSavedPadState = 1;
      gPlayerControlSuppressed = 1;
      gState.eventCameraPan.x = 0;
      gState.eventCameraPan.y = 0;
      gState.eventCameraPan.z = 0;
      gState.eventCameraHeight = 0;
      gState.focus = NULL;

      for (i = 0; i < 20; i++) {
         gState.mapState.bytes[i] = 0;
      }

      if (gState.scene == 94) {
         DrawWindow(0x34, 0, 0, 136, 63, 92, 88, WBS_DRAGON, 0);
         DisplayCustomWindow(0x34, 0, 1, 1, 0, 0);
         /* そして私は・・・ */
         DrawText(20, 24, 25, 2, 0, "\x82\xbb\x82\xb5\x82\xc4\x8e\x84\x82\xcd\x81\x45\x81\x45\x81\x45");
      }
      if (gState.scene == 0) {
         DrawWindow(0x34, 0, 0, 144, 100, 88, 70, WBS_DRAGON, 0);
         DisplayCustomWindow(0x34, 0, 1, 1, 0, 0);
         /* 神光紀１２５４年\n　ギルバレス島\n　「城塞遺跡」 */
         DrawText(24, 24, 25, 3, 0,
                  "\x90\x5f\x8c\xf5\x8b\x49\x82\x50\x82\x51\x82\x54\x82\x53\x94\x4e\x0a"
                  "\x81\x40\x83\x4d\x83\x8b\x83\x6f\x83\x8c\x83\x58\x93\x87\x0a"
                  "\x81\x40\x81\x75\x8f\xe9\x8d\xc7\x88\xe2\x90\xd5\x81\x76");
      }
      if ((gState.scene != 0) && (gState.scene != 1) && (gState.scene != 94) &&
          (gState.scene != 7) && (gState.scene != 20) && (gState.scene != 3) &&
          (gState.scene != 6) && (gState.scene != 19) && (gState.scene != 28) &&
          (gState.scene != 31) && (gState.scene != 34) && (gState.scene != 35) &&
          (gState.scene != 51) && (gState.scene != 68) && (gState.scene != 70) &&
          (gState.scene != 80) && (gState.scene != 87) && (gState.scene != 89) &&
          (gState.primary == STATE_4 || gState.primary == STATE_25)) {
         gState.suppressLoadingScreen = 0;
      }
   case 1:
   case 2:
   case 3:
      gState.secondary++;
      break;
   case 4:
      if (gState.primary == STATE_25) {
         LoadFWD();
         gState.primary = STATE_4;
      }
      if (gState.primary == STATE_4) {
         SetupPartySprites();
         for (i = 24; i < 50; i++) {
            gState.portraitsToLoad[i] = gPortraitsDb.sceneSets[gState.scene][i - 24];
         }
         LoadPortraits();
         LoadUnits();
         SetupSprites();
      }
      if (gState.scene == 29) {
         LoadSoundSet(10);
         FinishLoadingVab();
      }
      if (gState.scene == 39) {
         LoadSoundSet(5);
         FinishLoadingVab();
         LoadMap();
      }
      if (gState.scene == 55) {
         LoadSoundSet(3);
         FinishLoadingVab();
      }
      if (gState.scene == 88) {
         LoadSoundSet(9);
         FinishLoadingVab();
         LoadMap();
      }
      if (gState.scene == 93) {
         LoadSoundSet(7);
         FinishLoadingVab();
      }
      if (gState.scene == 51) {
         LoadSoundSet(11);
         FinishLoadingVab();
      }
      if (gState.scene == 52) {
         LoadSoundSet(2);
         FinishLoadingVab();
      }
      if (gState.scene == 61) {
         LoadSoundSet(12);
         FinishLoadingVab();
         LoadMap();
      }
      if (gState.scene == 94) {
         LoadSoundSet(11);
         FinishLoadingVab();
      }
      if (gState.scene == 90) {
         LoadSoundSet(2);
         FinishLoadingVab();
      }
      if (gState.scene == 63) {
         LoadSoundSet(2);
         FinishLoadingVab();
      }
      if (gState.primary != STATE_16) {
         LoadFCOM4XX();
         LoadMapTextures();
         LoadMap();
      }
      SetupField();
      SetupLight();
      gState.fieldRenderingDisabled = 0;
      gState.primary = STATE_4;
      gState.secondary++;
      gState.suppressLoadingScreen = 0;
      break;
   case 5:
      gState.secondary++;
      break;
   case 6:
      Obj_ResetFromIdx10();
      SetupMap();
      gTempObj = Obj_GetUnused();
      gTempObj->functionIndex = gState.state7;
      FadeInScreen(2, 6);
      gState.secondary++;
   case 7:
      break;
   }
}

/* JP-only debug scene selector, spawned from the debug menu (states/debug_menu); the US build ships
 * slot 584 as an empty stub. Three paged families -- events, world-map destinations, towns; an even
 * state2 draws a page and the following odd state2 reads the choice; LEFT/RIGHT page, X backs out. */
void Objf584_DebugSceneSelect(Object *obj) {
   switch (obj->state) {
   case 0:
      gWindowChoiceHeight = 0x11;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 120, 225, 180, 4, 0, 3);
      /* イベント/ワールドマップ/町 */
      DrawText(12, 11, 10, 2, 0, "\x83\x43\x83\x78\x83\x93\x83\x67\x0a\x83\x8f\x81"
          "\x5b\x83\x8b\x83\x68\x83\x7d\x83\x62\x83\x76\x0a"
          "\x92\xac");
      DisplayBasicWindow(0x34);
      obj->state++;
      break;

   case 1:
      if (gPadStateNewPresses & PAD_X) {
         gState.primary = 1;
         break;
      }
      if (gWindowChoice.raw == 0x3401) {
         obj->state = 2;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3402) {
         obj->state = 3;
         obj->state2 = 0;
      }
      if (gWindowChoice.raw == 0x3403) {
         obj->state = 4;
         obj->state2 = 0;
      }
      break;

   case 2:
      if (gPadStateNewPresses & PAD_X) {
         obj->state = 0;
         obj->state2 = 0;
         break;
      }
      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ０魔石発見/１バトル１前/２捕らえた男の正体/３兵団本部（１）/４バトル２前/５教会内/６兵団本部（２）/７バトル７後/８バトル３前/９ゴーレム使いの娘/１０地下牢にて/１１バトル４前 */
         DrawText(12, 11, 30, 2, 0, "\x82\x4f\x96\x82\x90\xce\x94\xad\x8c\xa9\x0a\x82"
            "\x50\x83\x6f\x83\x67\x83\x8b\x82\x50\x91\x4f\x0a"
            "\x82\x51\x95\xdf\x82\xe7\x82\xa6\x82\xbd\x92\x6a"
            "\x82\xcc\x90\xb3\x91\xcc\x0a\x82\x52\x95\xba\x92"
            "\x63\x96\x7b\x95\x94\x81\x69\x82\x50\x81\x6a\x0a"
            "\x82\x53\x83\x6f\x83\x67\x83\x8b\x82\x51\x91\x4f"
            "\x0a\x82\x54\x8b\xb3\x89\xef\x93\xe0\x0a\x82\x55"
            "\x95\xba\x92\x63\x96\x7b\x95\x94\x81\x69\x82\x51"
            "\x81\x6a\x0a\x82\x56\x83\x6f\x83\x67\x83\x8b\x82"
            "\x56\x8c\xe3\x0a\x82\x57\x83\x6f\x83\x67\x83\x8b"
            "\x82\x52\x91\x4f\x0a\x82\x58\x83\x53\x81\x5b\x83"
            "\x8c\x83\x80\x8e\x67\x82\xa2\x82\xcc\x96\xba\x0a"
            "\x82\x50\x82\x4f\x92\x6e\x89\xba\x98\x53\x82\xc9"
            "\x82\xc4\x0a\x82\x50\x82\x50\x83\x6f\x83\x67\x83"
            "\x8b\x82\x53\x91\x4f");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 1:
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice - 1;
         break;

      case 2:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* １２謎の女傭兵/１３エリナの過去/１４ラドーの家（１）/１５バトル５前/１６ラドーの家（２）/１７バトル６前/１８レッドラムの最後/１９バトル７前/２０島での出来事/２１バトル８前/２２逃亡者の正体/２３調査体の目的 */
         DrawText(12, 11, 30, 2, 0, "\x82\x50\x82\x51\x93\xe4\x82\xcc\x8f\x97\x97\x62"
            "\x95\xba\x0a\x82\x50\x82\x52\x83\x47\x83\x8a\x83"
            "\x69\x82\xcc\x89\xdf\x8b\x8e\x0a\x82\x50\x82\x53"
            "\x83\x89\x83\x68\x81\x5b\x82\xcc\x89\xc6\x81\x69"
            "\x82\x50\x81\x6a\x0a\x82\x50\x82\x54\x83\x6f\x83"
            "\x67\x83\x8b\x82\x54\x91\x4f\x0a\x82\x50\x82\x55"
            "\x83\x89\x83\x68\x81\x5b\x82\xcc\x89\xc6\x81\x69"
            "\x82\x51\x81\x6a\x0a\x82\x50\x82\x56\x83\x6f\x83"
            "\x67\x83\x8b\x82\x55\x91\x4f\x0a\x82\x50\x82\x57"
            "\x83\x8c\x83\x62\x83\x68\x83\x89\x83\x80\x82\xcc"
            "\x8d\xc5\x8c\xe3\x0a\x82\x50\x82\x58\x83\x6f\x83"
            "\x67\x83\x8b\x82\x56\x91\x4f\x0a\x82\x51\x82\x4f"
            "\x93\x87\x82\xc5\x82\xcc\x8f\x6f\x97\x88\x8e\x96"
            "\x0a\x82\x51\x82\x50\x83\x6f\x83\x67\x83\x8b\x82"
            "\x57\x91\x4f\x0a\x82\x51\x82\x51\x93\xa6\x96\x53"
            "\x8e\xd2\x82\xcc\x90\xb3\x91\xcc\x0a\x82\x51\x82"
            "\x52\x92\xb2\x8d\xb8\x91\xcc\x82\xcc\x96\xda\x93"
            "\x49");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 3:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 11;
         break;

      case 4:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ２４バトル９前/２５キャンプにて/２６リーンとキース/２７バトル１０前/２８突入直前/２９バトル１１前/３０クリムゾン再び/３１決起/３２バトル１２前/３３ベラスコ救出/３４アッシュ消滅/３５バトル１３前 */
         DrawText(12, 11, 30, 2, 0, "\x82\x51\x82\x53\x83\x6f\x83\x67\x83\x8b\x82\x58"
            "\x91\x4f\x0a\x82\x51\x82\x54\x83\x4c\x83\x83\x83"
            "\x93\x83\x76\x82\xc9\x82\xc4\x0a\x82\x51\x82\x55"
            "\x83\x8a\x81\x5b\x83\x93\x82\xc6\x83\x4c\x81\x5b"
            "\x83\x58\x0a\x82\x51\x82\x56\x83\x6f\x83\x67\x83"
            "\x8b\x82\x50\x82\x4f\x91\x4f\x0a\x82\x51\x82\x57"
            "\x93\xcb\x93\xfc\x92\xbc\x91\x4f\x0a\x82\x51\x82"
            "\x58\x83\x6f\x83\x67\x83\x8b\x82\x50\x82\x50\x91"
            "\x4f\x0a\x82\x52\x82\x4f\x83\x4e\x83\x8a\x83\x80"
            "\x83\x5d\x83\x93\x8d\xc4\x82\xd1\x0a\x82\x52\x82"
            "\x50\x8c\x88\x8b\x4e\x0a\x82\x52\x82\x51\x83\x6f"
            "\x83\x67\x83\x8b\x82\x50\x82\x51\x91\x4f\x0a\x82"
            "\x52\x82\x52\x83\x78\x83\x89\x83\x58\x83\x52\x8b"
            "\x7e\x8f\x6f\x0a\x82\x52\x82\x53\x83\x41\x83\x62"
            "\x83\x56\x83\x85\x8f\xc1\x96\xc5\x0a\x82\x52\x82"
            "\x54\x83\x6f\x83\x67\x83\x8b\x82\x50\x82\x52\x91"
            "\x4f");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 5:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 23;
         break;

      case 6:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ３６謎の男/３７サムデラの庵/３８バトル１４前/３９帰還/４０バトル１５前/４１再開/４２反乱計画の立案/４３バトル１６前/４４裁定の炎/４５欠番/４６バトル１７前/４７要塞刑務所にて */
         DrawText(12, 11, 30, 2, 0, "\x82\x52\x82\x55\x93\xe4\x82\xcc\x92\x6a\x0a\x82"
            "\x52\x82\x56\x83\x54\x83\x80\x83\x66\x83\x89\x82"
            "\xcc\x88\xc1\x0a\x82\x52\x82\x57\x83\x6f\x83\x67"
            "\x83\x8b\x82\x50\x82\x53\x91\x4f\x0a\x82\x52\x82"
            "\x58\x8b\x41\x8a\xd2\x0a\x82\x53\x82\x4f\x83\x6f"
            "\x83\x67\x83\x8b\x82\x50\x82\x54\x91\x4f\x0a\x82"
            "\x53\x82\x50\x8d\xc4\x8a\x4a\x0a\x82\x53\x82\x51"
            "\x94\xbd\x97\x90\x8c\x76\x89\xe6\x82\xcc\x97\xa7"
            "\x88\xc4\x0a\x82\x53\x82\x52\x83\x6f\x83\x67\x83"
            "\x8b\x82\x50\x82\x55\x91\x4f\x0a\x82\x53\x82\x53"
            "\x8d\xd9\x92\xe8\x82\xcc\x89\x8a\x0a\x82\x53\x82"
            "\x54\x8c\x87\x94\xd4\x0a\x82\x53\x82\x55\x83\x6f"
            "\x83\x67\x83\x8b\x82\x50\x82\x56\x91\x4f\x0a\x82"
            "\x53\x82\x56\x97\x76\x8d\xc7\x8c\x59\x96\xb1\x8f"
            "\x8a\x82\xc9\x82\xc4");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 7:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 35;
         break;

      case 8:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ４８バトル１８前/４９バトル１９前/５０合流/５１方針決定/５２アッシュとクラウス/５３バトル２０前/５４リーン拘束/５５バトル２１前/５６師弟の真実/５７大商人クルス/５８欠番/５９欠番 */
         DrawText(12, 11, 30, 2, 0, "\x82\x53\x82\x57\x83\x6f\x83\x67\x83\x8b\x82\x50"
            "\x82\x57\x91\x4f\x0a\x82\x53\x82\x58\x83\x6f\x83"
            "\x67\x83\x8b\x82\x50\x82\x58\x91\x4f\x0a\x82\x54"
            "\x82\x4f\x8d\x87\x97\xac\x0a\x82\x54\x82\x50\x95"
            "\xfb\x90\x6a\x8c\x88\x92\xe8\x0a\x82\x54\x82\x51"
            "\x83\x41\x83\x62\x83\x56\x83\x85\x82\xc6\x83\x4e"
            "\x83\x89\x83\x45\x83\x58\x0a\x82\x54\x82\x52\x83"
            "\x6f\x83\x67\x83\x8b\x82\x51\x82\x4f\x91\x4f\x0a"
            "\x82\x54\x82\x53\x83\x8a\x81\x5b\x83\x93\x8d\x53"
            "\x91\xa9\x0a\x82\x54\x82\x54\x83\x6f\x83\x67\x83"
            "\x8b\x82\x51\x82\x50\x91\x4f\x0a\x82\x54\x82\x55"
            "\x8e\x74\x92\xed\x82\xcc\x90\x5e\x8e\xc0\x0a\x82"
            "\x54\x82\x56\x91\xe5\x8f\xa4\x90\x6c\x83\x4e\x83"
            "\x8b\x83\x58\x0a\x82\x54\x82\x57\x8c\x87\x94\xd4"
            "\x0a\x82\x54\x82\x58\x8c\x87\x94\xd4");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 9:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 47;
         break;

      case 10:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ６０バトル２２前/６１クルス倒れる/６２親子の和解/６３バトル２３前/６４挑発/６５バトル２４前/６６恩讐の果て/６７リーンの悔悟/６８謀反/６９バトル２５前/７０エリナの悪夢/７１バトル２６前 */
         DrawText(12, 11, 30, 2, 0, "\x82\x55\x82\x4f\x83\x6f\x83\x67\x83\x8b\x82\x51"
            "\x82\x51\x91\x4f\x0a\x82\x55\x82\x50\x83\x4e\x83"
            "\x8b\x83\x58\x93\x7c\x82\xea\x82\xe9\x0a\x82\x55"
            "\x82\x51\x90\x65\x8e\x71\x82\xcc\x98\x61\x89\xf0"
            "\x0a\x82\x55\x82\x52\x83\x6f\x83\x67\x83\x8b\x82"
            "\x51\x82\x52\x91\x4f\x0a\x82\x55\x82\x53\x92\xa7"
            "\x94\xad\x0a\x82\x55\x82\x54\x83\x6f\x83\x67\x83"
            "\x8b\x82\x51\x82\x53\x91\x4f\x0a\x82\x55\x82\x55"
            "\x89\xb6\x8f\x51\x82\xcc\x89\xca\x82\xc4\x0a\x82"
            "\x55\x82\x56\x83\x8a\x81\x5b\x83\x93\x82\xcc\x89"
            "\xf7\x8c\xe5\x0a\x82\x55\x82\x57\x96\x64\x94\xbd"
            "\x0a\x82\x55\x82\x58\x83\x6f\x83\x67\x83\x8b\x82"
            "\x51\x82\x54\x91\x4f\x0a\x82\x56\x82\x4f\x83\x47"
            "\x83\x8a\x83\x69\x82\xcc\x88\xab\x96\xb2\x0a\x82"
            "\x56\x82\x50\x83\x6f\x83\x67\x83\x8b\x82\x51\x82"
            "\x55\x91\x4f");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 11:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 59;
         break;

      case 12:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 12);
         /* ７２憶測/７３パドウィンの家/７４バトル２７前/７５平地の謎/７６村の秘密/７７バトル２８前/７８バトル２９前/７９パドウィンの遺言/８０埋葬が終わって/８１エリナとリィナ/８２バトル３０前/８３リィナ消滅 */
         DrawText(12, 11, 30, 2, 0, "\x82\x56\x82\x51\x89\xaf\x91\xaa\x0a\x82\x56\x82"
            "\x52\x83\x70\x83\x68\x83\x45\x83\x42\x83\x93\x82"
            "\xcc\x89\xc6\x0a\x82\x56\x82\x53\x83\x6f\x83\x67"
            "\x83\x8b\x82\x51\x82\x56\x91\x4f\x0a\x82\x56\x82"
            "\x54\x95\xbd\x92\x6e\x82\xcc\x93\xe4\x0a\x82\x56"
            "\x82\x55\x91\xba\x82\xcc\x94\xe9\x96\xa7\x0a\x82"
            "\x56\x82\x56\x83\x6f\x83\x67\x83\x8b\x82\x51\x82"
            "\x57\x91\x4f\x0a\x82\x56\x82\x57\x83\x6f\x83\x67"
            "\x83\x8b\x82\x51\x82\x58\x91\x4f\x0a\x82\x56\x82"
            "\x58\x83\x70\x83\x68\x83\x45\x83\x42\x83\x93\x82"
            "\xcc\x88\xe2\x8c\xbe\x0a\x82\x57\x82\x4f\x96\x84"
            "\x91\x92\x82\xaa\x8f\x49\x82\xed\x82\xc1\x82\xc4"
            "\x0a\x82\x57\x82\x50\x83\x47\x83\x8a\x83\x69\x82"
            "\xc6\x83\x8a\x83\x42\x83\x69\x0a\x82\x57\x82\x51"
            "\x83\x6f\x83\x67\x83\x8b\x82\x52\x82\x4f\x91\x4f"
            "\x0a\x82\x57\x82\x52\x83\x8a\x83\x42\x83\x69\x8f"
            "\xc1\x96\xc5");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 13:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 71;
         break;

      case 14:
         DrawWindow(0x34, 0, 0, 224, 225, 50, 4, 0, 11);
         /* ８４バトル３１前/８５バトル３２前/８６アッシュ暴走/８７クラウスの回想/８８クラウスの告白/８９決戦前夜/９０バトル３３前/９１決戦直前/９２バトル３４前/９３ドルフの最後/９４エリナの日記 */
         DrawText(12, 11, 30, 2, 0, "\x82\x57\x82\x53\x83\x6f\x83\x67\x83\x8b\x82\x52"
            "\x82\x50\x91\x4f\x0a\x82\x57\x82\x54\x83\x6f\x83"
            "\x67\x83\x8b\x82\x52\x82\x51\x91\x4f\x0a\x82\x57"
            "\x82\x55\x83\x41\x83\x62\x83\x56\x83\x85\x96\x5c"
            "\x91\x96\x0a\x82\x57\x82\x56\x83\x4e\x83\x89\x83"
            "\x45\x83\x58\x82\xcc\x89\xf1\x91\x7a\x0a\x82\x57"
            "\x82\x57\x83\x4e\x83\x89\x83\x45\x83\x58\x82\xcc"
            "\x8d\x90\x94\x92\x0a\x82\x57\x82\x58\x8c\x88\x90"
            "\xed\x91\x4f\x96\xe9\x0a\x82\x58\x82\x4f\x83\x6f"
            "\x83\x67\x83\x8b\x82\x52\x82\x52\x91\x4f\x0a\x82"
            "\x58\x82\x50\x8c\x88\x90\xed\x92\xbc\x91\x4f\x0a"
            "\x82\x58\x82\x51\x83\x6f\x83\x67\x83\x8b\x82\x52"
            "\x82\x53\x91\x4f\x0a\x82\x58\x82\x52\x83\x68\x83"
            "\x8b\x83\x74\x82\xcc\x8d\xc5\x8c\xe3\x0a\x82\x58"
            "\x82\x53\x83\x47\x83\x8a\x83\x69\x82\xcc\x93\xfa"
            "\x8b\x4c");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 15:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 0x1a;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.scene = gWindowChoice.s.choice + 83;
         break;

      }
      break;

   case 3:
      if (gPadStateNewPresses & PAD_X) {
         obj->state = 0;
         obj->state2 = 0;
         break;
      }
      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 120, 225, 180, 4, 0, 12);
         /* １−１/１−２/１−３/１−４/１−５/１−６/１−７/２−１/２−２/２−３/２−４/２−５/ */
         DrawText(12, 11, 10, 2, 0, "\x82\x50\x81\x7c\x82\x50\x0a\x82\x50\x81\x7c\x82"
            "\x51\x0a\x82\x50\x81\x7c\x82\x52\x0a\x82\x50\x81"
            "\x7c\x82\x53\x0a\x82\x50\x81\x7c\x82\x54\x0a\x82"
            "\x50\x81\x7c\x82\x55\x0a\x82\x50\x81\x7c\x82\x56"
            "\x0a\x82\x51\x81\x7c\x82\x50\x0a\x82\x51\x81\x7c"
            "\x82\x51\x0a\x82\x51\x81\x7c\x82\x52\x0a\x82\x51"
            "\x81\x7c\x82\x53\x0a\x82\x51\x81\x7c\x82\x54\x0a");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 1:
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId == 0x34) {
            if (gWindowChoice.s.choice != 0) {
               gState.primary = 6;
               gState.secondary = 0;
               gState.state3 = 0;
               gState.state4 = 0;
            }
         }
         if (gWindowChoice.raw == 0x3401) {
            gState.worldMapState = 11;
         }
         if (gWindowChoice.raw == 0x3402) {
            gState.worldMapState = 12;
         }
         if (gWindowChoice.raw == 0x3403) {
            gState.worldMapState = 13;
         }
         if (gWindowChoice.raw == 0x3404) {
            gState.worldMapState = 14;
         }
         if (gWindowChoice.raw == 0x3405) {
            gState.worldMapState = 15;
         }
         if (gWindowChoice.raw == 0x3406) {
            gState.worldMapState = 16;
         }
         if (gWindowChoice.raw == 0x3407) {
            gState.worldMapState = 17;
         }
         if (gWindowChoice.raw == 0x3408) {
            gState.worldMapState = 21;
         }
         if (gWindowChoice.raw == 0x3409) {
            gState.worldMapState = 22;
         }
         if (gWindowChoice.raw == 0x340a) {
            gState.worldMapState = 23;
         }
         if (gWindowChoice.raw == 0x340b) {
            gState.worldMapState = 24;
         }
         if (gWindowChoice.raw == 0x340c) {
            gState.worldMapState = 25;
         }
         break;

      case 2:
         DrawWindow(0x34, 0, 0, 120, 225, 180, 4, 0, 12);
         /* ３−１/３−２/４−１/４−２/４−３/５−１/５−２/５−３/６−１/６−２/６−３/６−４/ */
         DrawText(12, 11, 10, 2, 0, "\x82\x52\x81\x7c\x82\x50\x0a\x82\x52\x81\x7c\x82"
            "\x51\x0a\x82\x53\x81\x7c\x82\x50\x0a\x82\x53\x81"
            "\x7c\x82\x51\x0a\x82\x53\x81\x7c\x82\x52\x0a\x82"
            "\x54\x81\x7c\x82\x50\x0a\x82\x54\x81\x7c\x82\x51"
            "\x0a\x82\x54\x81\x7c\x82\x52\x0a\x82\x55\x81\x7c"
            "\x82\x50\x0a\x82\x55\x81\x7c\x82\x51\x0a\x82\x55"
            "\x81\x7c\x82\x52\x0a\x82\x55\x81\x7c\x82\x53\x0a");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 3:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId == 0x34) {
            if (gWindowChoice.s.choice != 0) {
               gState.primary = 6;
               gState.secondary = 0;
               gState.state3 = 0;
               gState.state4 = 0;
            }
         }
         if (gWindowChoice.raw == 0x3401) {
            gState.worldMapState = 31;
         }
         if (gWindowChoice.raw == 0x3402) {
            gState.worldMapState = 32;
         }
         if (gWindowChoice.raw == 0x3403) {
            gState.worldMapState = 41;
         }
         if (gWindowChoice.raw == 0x3404) {
            gState.worldMapState = 42;
         }
         if (gWindowChoice.raw == 0x3405) {
            gState.worldMapState = 43;
         }
         if (gWindowChoice.raw == 0x3406) {
            gState.worldMapState = 51;
         }
         if (gWindowChoice.raw == 0x3407) {
            gState.worldMapState = 52;
         }
         if (gWindowChoice.raw == 0x3408) {
            gState.worldMapState = 53;
         }
         if (gWindowChoice.raw == 0x3409) {
            gState.worldMapState = 61;
         }
         if (gWindowChoice.raw == 0x340a) {
            gState.worldMapState = 62;
         }
         if (gWindowChoice.raw == 0x340b) {
            gState.worldMapState = 63;
         }
         if (gWindowChoice.raw == 0x340c) {
            gState.worldMapState = 64;
         }
         break;

      case 4:
         DrawWindow(0x34, 0, 0, 120, 225, 180, 4, 0, 7);
         /* ６−５/６−６/６−７/７−１/７−２/７−３/７−４ */
         DrawText(12, 11, 10, 2, 0, "\x82\x55\x81\x7c\x82\x54\x0a\x82\x55\x81\x7c\x82"
            "\x55\x0a\x82\x55\x81\x7c\x82\x56\x0a\x82\x56\x81"
            "\x7c\x82\x50\x0a\x82\x56\x81\x7c\x82\x51\x0a\x82"
            "\x56\x81\x7c\x82\x52\x0a\x82\x56\x81\x7c\x82\x53");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 5:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gWindowChoice.s.windowId == 0x34) {
            if (gWindowChoice.s.choice != 0) {
               gState.primary = 6;
               gState.secondary = 0;
               gState.state3 = 0;
               gState.state4 = 0;
            }
         }
         if (gWindowChoice.raw == 0x3401) {
            gState.worldMapState = 65;
         }
         if (gWindowChoice.raw == 0x3402) {
            gState.worldMapState = 66;
         }
         if (gWindowChoice.raw == 0x3403) {
            gState.worldMapState = 67;
         }
         if (gWindowChoice.raw == 0x3404) {
            gState.worldMapState = 71;
         }
         if (gWindowChoice.raw == 0x3405) {
            gState.worldMapState = 72;
         }
         if (gWindowChoice.raw == 0x3406) {
            gState.worldMapState = 73;
         }
         if (gWindowChoice.raw == 0x3407) {
            gState.worldMapState = 74;
         }
         break;

      }
      break;

   case 4:
      if (gPadStateNewPresses & PAD_X) {
         obj->state = 0;
         obj->state2 = 0;
         break;
      }
      switch (obj->state2) {
      case 0:
         DrawWindow(0x34, 0, 0, 160, 225, 150, 4, 0, 12);
         /* #33（１）/#33（１）Ｂ/#33（２）/#33（２）Ｂ/#33（３）/#33（４）/#37（１）/#37（１）Ｂ/#37（２）/#37（３）/#37（４）/#38 */
         DrawText(12, 11, 20, 2, 0, "\x23\x33\x33\x81\x69\x82\x50\x81\x6a\x0a\x23\x33"
            "\x33\x81\x69\x82\x50\x81\x6a\x82\x61\x0a\x23\x33"
            "\x33\x81\x69\x82\x51\x81\x6a\x0a\x23\x33\x33\x81"
            "\x69\x82\x51\x81\x6a\x82\x61\x0a\x23\x33\x33\x81"
            "\x69\x82\x52\x81\x6a\x0a\x23\x33\x33\x81\x69\x82"
            "\x53\x81\x6a\x0a\x23\x33\x37\x81\x69\x82\x50\x81"
            "\x6a\x0a\x23\x33\x37\x81\x69\x82\x50\x81\x6a\x82"
            "\x61\x0a\x23\x33\x37\x81\x69\x82\x51\x81\x6a\x0a"
            "\x23\x33\x37\x81\x69\x82\x52\x81\x6a\x0a\x23\x33"
            "\x37\x81\x69\x82\x53\x81\x6a\x0a\x23\x33\x38");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 1:
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 7;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.townState = gWindowChoice.s.choice - 1;
         break;

      case 2:
         DrawWindow(0x34, 0, 0, 160, 225, 150, 4, 0, 12);
         /* #38（２）/#43（１）/#43（１）Ｂ/#43（２）/#43（３）/#44/#48（１）/#48（２）/#50（１）/#50（１）Ｂ/#50（２）/#50（３） */
         DrawText(12, 11, 20, 2, 0, "\x23\x33\x38\x81\x69\x82\x51\x81\x6a\x0a\x23\x34"
            "\x33\x81\x69\x82\x50\x81\x6a\x0a\x23\x34\x33\x81"
            "\x69\x82\x50\x81\x6a\x82\x61\x0a\x23\x34\x33\x81"
            "\x69\x82\x51\x81\x6a\x0a\x23\x34\x33\x81\x69\x82"
            "\x52\x81\x6a\x0a\x23\x34\x34\x0a\x23\x34\x38\x81"
            "\x69\x82\x50\x81\x6a\x0a\x23\x34\x38\x81\x69\x82"
            "\x51\x81\x6a\x0a\x23\x35\x30\x81\x69\x82\x50\x81"
            "\x6a\x0a\x23\x35\x30\x81\x69\x82\x50\x81\x6a\x82"
            "\x61\x0a\x23\x35\x30\x81\x69\x82\x51\x81\x6a\x0a"
            "\x23\x35\x30\x81\x69\x82\x52\x81\x6a");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 3:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gPadStateNewPresses & PAD_RIGHT) {
            obj->state2++;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 7;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.townState = gWindowChoice.s.choice + 11;
         break;

      case 4:
         DrawWindow(0x34, 0, 0, 160, 225, 150, 4, 0, 9);
         /* #50（４）/#54（１）/#54（２）/#56（１）/#56（１）Ｂ/#56（２）/#56（３）/#56（４）/#60 */
         DrawText(12, 11, 20, 2, 0, "\x23\x35\x30\x81\x69\x82\x53\x81\x6a\x0a\x23\x35"
            "\x34\x81\x69\x82\x50\x81\x6a\x0a\x23\x35\x34\x81"
            "\x69\x82\x51\x81\x6a\x0a\x23\x35\x36\x81\x69\x82"
            "\x50\x81\x6a\x0a\x23\x35\x36\x81\x69\x82\x50\x81"
            "\x6a\x82\x61\x0a\x23\x35\x36\x81\x69\x82\x51\x81"
            "\x6a\x0a\x23\x35\x36\x81\x69\x82\x52\x81\x6a\x0a"
            "\x23\x35\x36\x81\x69\x82\x53\x81\x6a\x0a\x23\x36"
            "\x30");
         DisplayBasicWindow(0x34);
         obj->state2++;

      // fallthrough

      case 5:
         if (gPadStateNewPresses & PAD_LEFT) {
            obj->state2 -= 3;
            break;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice != 0) {
            gState.primary = 7;
            gState.secondary = 0;
            gState.state3 = 0;
            gState.state4 = 0;
         }
         if (gWindowChoice.s.windowId != 0x34) {
            break;
         }
         if (gWindowChoice.s.choice == 0) {
            break;
         }
         gState.townState = gWindowChoice.s.choice + 23;
         break;

      }
      break;

   }
}
