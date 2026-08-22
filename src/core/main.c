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

#ifdef PC_FEAT
/* Language packs (platform/pc/src/pc_lang.c): string literals below wrapped in PC_LANGSTR are
 * offered to the active pack by CONTENT (matched by hash); with no pack, or no entry, the literal
 * itself is returned. The matching build expands the macro to the bare literal. */
extern u8 *PC_LangStr(const char *lit);
#define PC_LANGSTR(s) (PC_LangStr(s))
#else
#define PC_LANGSTR(s) (s)
#endif


void main(void);
void UpdateState(void);
void Objf582_MainMenu_Jpn(Object *obj);
void Objf583_LoadingIndicator(Object *obj);
void Objf006_Logo(Object *obj);
void State_Init(void);
void State_EventScene(void);
#ifdef PC_FEAT
void Objf584_DebugSceneSelect(Object *obj);
#else
void Objf584_Noop(void);
#endif

void main(void) {
   s32 i;
   u8 *p;

   //__main(); // <- inserted automatically by gcc
   p = (u8 *)&gState;

   for (i = 0; i < sizeof(State); i++) {
      *(p++) = 0;
   }

   while (1) {
#ifdef PC_PORT
      /* Overlay RETURN TO TITLE is deferred to here -- the only point where no game code is
       * mid-frame -- so the state flip can't race a live loader (see pc_balance.c). Must run
       * BEFORE UpdateState: the title state's entry then resets leftover objects before
       * Obj_Execute can run one. */
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
      SetXaCdSectorOffset(0x4c0d);
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
         DrawWindow(0x34, 0, 0, 200, 81, 60, 79, WBS_DRAGON, 0);
         DisplayCustomWindow(0x34, 0, 1, 1, 0, 0);
         DrawText(20, 24, 25, 2, 0, PC_LANGSTR("The days go by one\nafter the other..."));
      }
      if (gState.scene == 0) {
         DrawWindow(0x34, 0, 0, 184, 100, 68, 70, WBS_DRAGON, 0);
         DisplayCustomWindow(0x34, 0, 1, 1, 0, 0);
         DrawText(24, 24, 25, 3, 0, PC_LANGSTR("1254 AT\nGillbaris Island\n \"Castle ruins\""));
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

#ifdef PC_FEAT
/* exchange/107: the JP build's development jump-to-anywhere selector (three paged families --
 * events 0-94, world-map positions, towns), ported from jp/src/core/main.c with the SJIS page
 * literals translated to ASCII (the US font carries only 209 glyphs -- kanji don't exist on
 * this disc). Logic is UNCHANGED from the JP original; town pages use the same "#NN"
 * string-table escapes, which resolve to the US localization's own town names. Reached from
 * the retail debug menu's SELECT B entry (states/debug_menu.c, same gate); the menu itself
 * remains dev-only via VH_DEBUG_MENU=1 (pc_diag.c). US retail ships slot 584 as an empty stub
 * (the #else). */
void Objf584_DebugSceneSelect(Object *obj) {
   switch (obj->state) {
   case 0:
      gWindowChoiceHeight = 0x11;
      gWindowChoicesTopMargin = 10;
      gWindowActiveIdx = 0x34;
      DrawWindow(0x34, 0, 0, 120, 225, 180, 4, 0, 3);
      DrawSjisText(12, 11, 10, 2, 0, "\x82\x64\x82\x75\x82\x64\x82\x6d\x82\x73\n\x82\x76\x82\x6e\x82\x71\x82\x6b\x82\x63\x81\x40\x82\x6c\x82\x60\x82\x6f\n\x82\x73\x82\x6e\x82\x76\x82\x6d");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x4f\x81\x40\x82\x6c\x82\x81\x82\x87\x82\x89\x82\x83\x81\x40\x82\x93\x82\x94\x82\x8f\x82\x8e\x82\x85\n\x82\x50\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\n\x82\x51\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x83\x82\x81\x82\x90\x82\x94\x82\x89\x82\x96\x82\x85\n\x82\x52\x81\x40\x82\x66\x82\x95\x82\x89\x82\x8c\x82\x84\x81\x40\x82\x67\x82\x70\x81\x40\x81\x69\x82\x50\x81\x6a\n\x82\x53\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\n\x82\x54\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x83\x82\x88\x82\x95\x82\x92\x82\x83\x82\x88\n\x82\x55\x81\x40\x82\x66\x82\x95\x82\x89\x82\x8c\x82\x84\x81\x40\x82\x67\x82\x70\x81\x40\x81\x69\x82\x51\x81\x6a\n\x82\x56\x81\x40\x82\x6f\x82\x8f\x82\x93\x82\x94\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x56\n\x82\x57\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\n\x82\x58\x81\x40\x82\x66\x82\x8f\x82\x8c\x82\x85\x82\x8d\x81\x40\x82\x87\x82\x89\x82\x92\x82\x8c\n\x82\x50\x82\x4f\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x84\x82\x95\x82\x8e\x82\x87\x82\x85\x82\x8f\x82\x8e\n\x82\x50\x82\x50\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x53");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x50\x82\x51\x81\x40\x82\x6c\x82\x85\x82\x92\x82\x83\x82\x85\x82\x8e\x82\x81\x82\x92\x82\x99\n\x82\x50\x82\x52\x81\x40\x82\x64\x82\x8c\x82\x85\x82\x8e\x82\x89\x81\x66\x82\x93\x81\x40\x82\x90\x82\x81\x82\x93\x82\x94\n\x82\x50\x82\x53\x81\x40\x82\x6b\x82\x81\x82\x84\x82\x8f\x81\x40\x82\x88\x82\x8f\x82\x8d\x82\x85\x81\x40\x81\x69\x82\x50\x81\x6a\n\x82\x50\x82\x54\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x54\n\x82\x50\x82\x55\x81\x40\x82\x6b\x82\x81\x82\x84\x82\x8f\x81\x40\x82\x88\x82\x8f\x82\x8d\x82\x85\x81\x40\x81\x69\x82\x51\x81\x6a\n\x82\x50\x82\x56\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x55\n\x82\x50\x82\x57\x81\x40\x82\x71\x82\x85\x82\x84\x82\x92\x82\x81\x82\x8d\x81\x66\x82\x93\x81\x40\x82\x85\x82\x8e\x82\x84\n\x82\x50\x82\x58\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x56\n\x82\x51\x82\x4f\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x89\x82\x93\x82\x8c\x82\x81\x82\x8e\x82\x84\n\x82\x51\x82\x50\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x57\n\x82\x51\x82\x51\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x86\x82\x95\x82\x87\x82\x89\x82\x94\x82\x89\x82\x96\x82\x85\n\x82\x51\x82\x52\x81\x40\x82\x72\x82\x95\x82\x92\x82\x96\x82\x85\x82\x99\x81\x40\x82\x94\x82\x85\x82\x81\x82\x8d");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x51\x82\x53\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x58\n\x82\x51\x82\x54\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x83\x82\x81\x82\x8d\x82\x90\n\x82\x51\x82\x55\x81\x40\x82\x6b\x82\x85\x82\x85\x82\x8e\x81\x40\x81\x95\x81\x40\x82\x6a\x82\x85\x82\x89\x82\x94\x82\x88\n\x82\x51\x82\x56\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x4f\n\x82\x51\x82\x57\x81\x40\x82\x61\x82\x85\x82\x86\x82\x8f\x82\x92\x82\x85\x81\x40\x82\x92\x82\x81\x82\x89\x82\x84\n\x82\x51\x82\x58\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x50\n\x82\x52\x82\x4f\x81\x40\x82\x62\x82\x92\x82\x89\x82\x8d\x82\x93\x82\x8f\x82\x8e\x81\x40\x82\x81\x82\x87\x82\x81\x82\x89\x82\x8e\n\x82\x52\x82\x50\x81\x40\x82\x74\x82\x90\x82\x92\x82\x89\x82\x93\x82\x89\x82\x8e\x82\x87\n\x82\x52\x82\x51\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x51\n\x82\x52\x82\x52\x81\x40\x82\x61\x82\x85\x82\x8c\x82\x81\x82\x93\x82\x83\x82\x8f\n\x82\x52\x82\x53\x81\x40\x82\x60\x82\x93\x82\x88\x81\x40\x82\x96\x82\x81\x82\x8e\x82\x89\x82\x93\x82\x88\x82\x85\x82\x93\n\x82\x52\x82\x54\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x52");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x52\x82\x55\x81\x40\x82\x6c\x82\x99\x82\x93\x82\x94\x82\x85\x82\x92\x82\x99\x81\x40\x82\x8d\x82\x81\x82\x8e\n\x82\x52\x82\x56\x81\x40\x82\x72\x82\x81\x82\x8d\x82\x84\x82\x85\x82\x8c\x82\x81\x81\x40\x82\x88\x82\x95\x82\x94\n\x82\x52\x82\x57\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x53\n\x82\x52\x82\x58\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x92\x82\x85\x82\x94\x82\x95\x82\x92\x82\x8e\n\x82\x53\x82\x4f\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x54\n\x82\x53\x82\x50\x81\x40\x82\x71\x82\x85\x82\x95\x82\x8e\x82\x89\x82\x8f\x82\x8e\n\x82\x53\x82\x51\x81\x40\x82\x71\x82\x85\x82\x96\x82\x8f\x82\x8c\x82\x94\x81\x40\x82\x90\x82\x8c\x82\x81\x82\x8e\n\x82\x53\x82\x52\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x55\n\x82\x53\x82\x53\x81\x40\x82\x69\x82\x95\x82\x84\x82\x87\x82\x8d\x82\x85\x82\x8e\x82\x94\n\x82\x53\x82\x54\x81\x40\x81\x69\x82\x95\x82\x8e\x82\x95\x82\x93\x82\x85\x82\x84\x81\x6a\n\x82\x53\x82\x55\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x56\n\x82\x53\x82\x56\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x90\x82\x92\x82\x89\x82\x93\x82\x8f\x82\x8e");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x53\x82\x57\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x57\n\x82\x53\x82\x58\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x50\x82\x58\n\x82\x54\x82\x4f\x81\x40\x82\x69\x82\x8f\x82\x89\x82\x8e\x82\x89\x82\x8e\x82\x87\x81\x40\x82\x95\x82\x90\n\x82\x54\x82\x50\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x90\x82\x8c\x82\x81\x82\x8e\n\x82\x54\x82\x51\x81\x40\x82\x60\x82\x93\x82\x88\x81\x40\x81\x95\x81\x40\x82\x6a\x82\x8c\x82\x81\x82\x95\x82\x93\n\x82\x54\x82\x52\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x4f\n\x82\x54\x82\x53\x81\x40\x82\x6b\x82\x85\x82\x85\x82\x8e\x81\x40\x82\x83\x82\x81\x82\x95\x82\x87\x82\x88\x82\x94\n\x82\x54\x82\x54\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x50\n\x82\x54\x82\x55\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x94\x82\x92\x82\x95\x82\x94\x82\x88\n\x82\x54\x82\x56\x81\x40\x82\x6a\x82\x95\x82\x92\x82\x9a\x81\x40\x82\x8d\x82\x85\x82\x92\x82\x83\x82\x88\x82\x81\x82\x8e\x82\x94\n\x82\x54\x82\x57\x81\x40\x81\x69\x82\x95\x82\x8e\x82\x95\x82\x93\x82\x85\x82\x84\x81\x6a\n\x82\x54\x82\x58\x81\x40\x81\x69\x82\x95\x82\x8e\x82\x95\x82\x93\x82\x85\x82\x84\x81\x6a");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x55\x82\x4f\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x51\n\x82\x55\x82\x50\x81\x40\x82\x6a\x82\x95\x82\x92\x82\x9a\x81\x40\x82\x86\x82\x81\x82\x8c\x82\x8c\x82\x93\n\x82\x55\x82\x51\x81\x40\x82\x71\x82\x85\x82\x83\x82\x8f\x82\x8e\x82\x83\x82\x89\x82\x8c\x82\x85\x82\x84\n\x82\x55\x82\x52\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x52\n\x82\x55\x82\x53\x81\x40\x82\x6f\x82\x92\x82\x8f\x82\x96\x82\x8f\x82\x83\x82\x81\x82\x94\x82\x89\x82\x8f\x82\x8e\n\x82\x55\x82\x54\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x53\n\x82\x55\x82\x55\x81\x40\x82\x65\x82\x85\x82\x95\x82\x84\x81\x66\x82\x93\x81\x40\x82\x85\x82\x8e\x82\x84\n\x82\x55\x82\x56\x81\x40\x82\x6b\x82\x85\x82\x85\x82\x8e\x81\x66\x82\x93\x81\x40\x82\x92\x82\x85\x82\x87\x82\x92\x82\x85\x82\x94\n\x82\x55\x82\x57\x81\x40\x82\x73\x82\x92\x82\x85\x82\x81\x82\x93\x82\x8f\x82\x8e\n\x82\x55\x82\x58\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x54\n\x82\x56\x82\x4f\x81\x40\x82\x64\x82\x8c\x82\x85\x82\x8e\x82\x89\x81\x66\x82\x93\x81\x40\x82\x84\x82\x92\x82\x85\x82\x81\x82\x8d\n\x82\x56\x82\x50\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x55");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x56\x82\x51\x81\x40\x82\x72\x82\x90\x82\x85\x82\x83\x82\x95\x82\x8c\x82\x81\x82\x94\x82\x89\x82\x8f\x82\x8e\n\x82\x56\x82\x52\x81\x40\x82\x6f\x82\x81\x82\x84\x82\x97\x82\x89\x82\x8e\x81\x40\x82\x88\x82\x8f\x82\x8d\x82\x85\n\x82\x56\x82\x53\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x56\n\x82\x56\x82\x54\x81\x40\x82\x6f\x82\x8c\x82\x81\x82\x89\x82\x8e\x82\x93\x81\x40\x82\x92\x82\x89\x82\x84\x82\x84\x82\x8c\x82\x85\n\x82\x56\x82\x55\x81\x40\x82\x75\x82\x89\x82\x8c\x82\x8c\x82\x81\x82\x87\x82\x85\x81\x40\x82\x93\x82\x85\x82\x83\x82\x92\x82\x85\x82\x94\n\x82\x56\x82\x56\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x57\n\x82\x56\x82\x57\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x51\x82\x58\n\x82\x56\x82\x58\x81\x40\x82\x6f\x82\x81\x82\x84\x82\x97\x82\x89\x82\x8e\x81\x66\x82\x93\x81\x40\x82\x97\x82\x89\x82\x8c\x82\x8c\n\x82\x57\x82\x4f\x81\x40\x82\x73\x82\x88\x82\x85\x81\x40\x82\x82\x82\x95\x82\x92\x82\x89\x82\x81\x82\x8c\n\x82\x57\x82\x50\x81\x40\x82\x64\x82\x8c\x82\x85\x82\x8e\x82\x89\x81\x40\x81\x95\x81\x40\x82\x6b\x82\x89\x82\x8e\x82\x81\n\x82\x57\x82\x51\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\x82\x4f\n\x82\x57\x82\x52\x81\x40\x82\x6b\x82\x89\x82\x8e\x82\x81\x81\x40\x82\x96\x82\x81\x82\x8e\x82\x89\x82\x93\x82\x88\x82\x85\x82\x93");
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
         DrawSjisText(12, 11, 30, 2, 0, "\x82\x57\x82\x53\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\x82\x50\n\x82\x57\x82\x54\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\x82\x51\n\x82\x57\x82\x55\x81\x40\x82\x60\x82\x93\x82\x88\x81\x40\x82\x82\x82\x85\x82\x92\x82\x93\x82\x85\x82\x92\x82\x8b\n\x82\x57\x82\x56\x81\x40\x82\x6a\x82\x8c\x82\x81\x82\x95\x82\x93\x81\x40\x82\x92\x82\x85\x82\x83\x82\x81\x82\x8c\x82\x8c\x82\x93\n\x82\x57\x82\x57\x81\x40\x82\x6a\x82\x8c\x82\x81\x82\x95\x82\x93\x81\x66\x82\x93\x81\x40\x82\x94\x82\x92\x82\x95\x82\x94\x82\x88\n\x82\x57\x82\x58\x81\x40\x82\x64\x82\x96\x82\x85\x81\x40\x82\x8f\x82\x86\x81\x40\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\n\x82\x58\x82\x4f\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\x82\x52\n\x82\x58\x82\x50\x81\x40\x82\x61\x82\x85\x82\x86\x82\x8f\x82\x92\x82\x85\x81\x40\x82\x86\x82\x89\x82\x8e\x82\x81\x82\x8c\n\x82\x58\x82\x51\x81\x40\x82\x6f\x82\x92\x82\x85\x81\x7c\x82\x82\x82\x81\x82\x94\x82\x94\x82\x8c\x82\x85\x81\x40\x82\x52\x82\x53\n\x82\x58\x82\x52\x81\x40\x82\x63\x82\x8f\x82\x8c\x82\x86\x81\x66\x82\x93\x81\x40\x82\x85\x82\x8e\x82\x84\n\x82\x58\x82\x53\x81\x40\x82\x64\x82\x8c\x82\x85\x82\x8e\x82\x89\x81\x66\x82\x93\x81\x40\x82\x84\x82\x89\x82\x81\x82\x92\x82\x99");
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
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x50\x81\x7c\x82\x50\n\x82\x50\x81\x7c\x82\x51\n\x82\x50\x81\x7c\x82\x52\n\x82\x50\x81\x7c\x82\x53\n\x82\x50\x81\x7c\x82\x54\n\x82\x50\x81\x7c\x82\x55\n\x82\x50\x81\x7c\x82\x56\n\x82\x51\x81\x7c\x82\x50\n\x82\x51\x81\x7c\x82\x51\n\x82\x51\x81\x7c\x82\x52\n\x82\x51\x81\x7c\x82\x53\n\x82\x51\x81\x7c\x82\x54");
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
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x52\x81\x7c\x82\x50\n\x82\x52\x81\x7c\x82\x51\n\x82\x53\x81\x7c\x82\x50\n\x82\x53\x81\x7c\x82\x51\n\x82\x53\x81\x7c\x82\x52\n\x82\x54\x81\x7c\x82\x50\n\x82\x54\x81\x7c\x82\x51\n\x82\x54\x81\x7c\x82\x52\n\x82\x55\x81\x7c\x82\x50\n\x82\x55\x81\x7c\x82\x51\n\x82\x55\x81\x7c\x82\x52\n\x82\x55\x81\x7c\x82\x53");
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
         DrawSjisText(12, 11, 10, 2, 0, "\x82\x55\x81\x7c\x82\x54\n\x82\x55\x81\x7c\x82\x55\n\x82\x55\x81\x7c\x82\x56\n\x82\x56\x81\x7c\x82\x50\n\x82\x56\x81\x7c\x82\x51\n\x82\x56\x81\x7c\x82\x52\n\x82\x56\x81\x7c\x82\x53");
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
         DrawSjisText(12, 11, 20, 2, 0, "\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x50\x81\x6a\n\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x50\x81\x6a\x82\x61\n\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x51\x81\x6a\n\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x51\x81\x6a\x82\x61\n\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x52\x81\x6a\n\x82\x73\x81\x44\x82\x75\x82\x81\x82\x8c\x82\x8c\x82\x85\x82\x99\x81\x69\x82\x53\x81\x6a\n\x82\x6c\x81\x44\x82\x6f\x82\x8f\x82\x92\x82\x94\x81\x69\x82\x50\x81\x6a\n\x82\x6c\x81\x44\x82\x6f\x82\x8f\x82\x92\x82\x94\x81\x69\x82\x50\x81\x6a\x82\x61\n\x82\x6c\x81\x44\x82\x6f\x82\x8f\x82\x92\x82\x94\x81\x69\x82\x51\x81\x6a\n\x82\x6c\x81\x44\x82\x6f\x82\x8f\x82\x92\x82\x94\x81\x69\x82\x52\x81\x6a\n\x82\x6c\x81\x44\x82\x6f\x82\x8f\x82\x92\x82\x94\x81\x69\x82\x53\x81\x6a\n\x82\x78\x82\x95\x82\x9a\x82\x95\x81\x40\x82\x75\x82\x8c\x82\x87");
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
         DrawSjisText(12, 11, 20, 2, 0, "\x82\x78\x82\x95\x82\x9a\x82\x95\x81\x40\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x51\x81\x6a\n\x82\x71\x82\x85\x82\x8d\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x50\x81\x6a\n\x82\x71\x82\x85\x82\x8d\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x50\x81\x6a\x82\x61\n\x82\x71\x82\x85\x82\x8d\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x51\x81\x6a\n\x82\x71\x82\x85\x82\x8d\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x52\x81\x6a\n\x82\x73\x82\x85\x82\x92\x82\x81\x82\x93\x82\x95\n\x82\x6a\x82\x88\x82\x81\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x50\x81\x6a\n\x82\x6a\x82\x88\x82\x81\x81\x44\x82\x62\x82\x89\x82\x94\x82\x99\x81\x69\x82\x51\x81\x6a\n\x82\x6a\x82\x85\x82\x92\x82\x81\x82\x83\x82\x88\x82\x89\x81\x69\x82\x50\x81\x6a\n\x82\x6a\x82\x85\x82\x92\x82\x81\x82\x83\x82\x88\x82\x89\x81\x69\x82\x50\x81\x6a\x82\x61\n\x82\x6a\x82\x85\x82\x92\x82\x81\x82\x83\x82\x88\x82\x89\x81\x69\x82\x51\x81\x6a\n\x82\x6a\x82\x85\x82\x92\x82\x81\x82\x83\x82\x88\x82\x89\x81\x69\x82\x52\x81\x6a");
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
         DrawSjisText(12, 11, 20, 2, 0, "\x82\x6a\x82\x85\x82\x92\x82\x81\x82\x83\x82\x88\x82\x89\x81\x69\x82\x53\x81\x6a\n\x82\x72\x82\x8f\x82\x92\x82\x82\x82\x8f\x81\x69\x82\x50\x81\x6a\n\x82\x72\x82\x8f\x82\x92\x82\x82\x82\x8f\x81\x69\x82\x51\x81\x6a\n\x82\x65\x82\x92\x82\x8e\x82\x94\x81\x44\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x50\x81\x6a\n\x82\x65\x82\x92\x82\x8e\x82\x94\x81\x44\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x50\x81\x6a\x82\x61\n\x82\x65\x82\x92\x82\x8e\x82\x94\x81\x44\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x51\x81\x6a\n\x82\x65\x82\x92\x82\x8e\x82\x94\x81\x44\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x52\x81\x6a\n\x82\x65\x82\x92\x82\x8e\x82\x94\x81\x44\x82\x75\x82\x8c\x82\x87\x81\x69\x82\x53\x81\x6a\n\x82\x66\x82\x8c\x82\x81\x82\x93\x82\x87\x82\x8f\x82\x97\x81\x40\x82\x62\x82\x89\x82\x94\x82\x99");
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
#else
void Objf584_Noop(void) {}
#endif
