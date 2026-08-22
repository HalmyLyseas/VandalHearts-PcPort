#ifndef STATE_H
#define STATE_H

#include "common.h"

typedef enum PrimaryState {
   STATE_0 = 0,
   STATE_1 = 1,
   STATE_3 = 3,
   STATE_4 = 4,
   STATE_SHOP = 5,
   STATE_6 = 6,
   STATE_7 = 7,
   STATE_TAVERN = 8,
   STATE_DOJO = 9,
   STATE_MOVIE = 10,
   STATE_12 = 12,
   STATE_CHAPTER_COMPLETE = 13,
   STATE_ENDING_SCREEN = 14,
   STATE_SET_SCENE_STATE = 15,
   STATE_16 = 16,
   STATE_17 = 17,
   STATE_18 = 18,
   STATE_19 = 19,
   STATE_20 = 20,
   STATE_DEPOT = 21,
   STATE_FILE_SAVE_SCREEN = 22,
   STATE_LOAD_IN_BATTLE_SAVE = 23,
   STATE_FILE_LOAD_SCREEN = 24,
   STATE_25 = 25,
   STATE_26 = 26,
   STATE_27 = 27,
   STATE_28 = 28,
   STATE_TRIAL_COMPLETE = 29,
   STATE_30 = 30,
   STATE_31 = 31,
   STATE_TITLE_SCREEN = 51,
   STATE_TITLE_LOAD_SCREEN = 52,
   STATE_LOAD_DEBUG_MENU = 98,
   STATE_DEBUG_MENU = 99,
} PrimaryState;

typedef enum CameraMode { CAMERA_MODE_DYNAMIC = 0, CAMERA_MODE_FIXED = 1 } CameraMode;

typedef struct DynamicIcon {
   s16 gfxIdx;
   s16 x;
   s16 y;
} DynamicIcon;

typedef struct State {
   u8 worldMapState;
   u8 townState;
   u8 scene;
   u8 field_0x3;
   u8 tavernVisit;
   u8 chapterOutro;
   u8 movieIdxToPlay;
   u8 worldMapDestination;
   u8 prevState;
   u8 saveLocation;
   u8 field_0xa;
   u8 field_0xb;
   s32 frameCounter;
   u8 chapter;
   u8 section;
   u8 subObjDone;
   u8 field_0x13;
   DynamicIcon dynamicIcons[7];
   BigInt experience;
   u8 expScalingLevel;
   u8 field_0x4f;
   s32 debug;
   s32 useDefaultUnits;
   s32 primary;
   s32 secondary;
   s32 state3;
   s32 state4;
   s32 state5;
   s32 state6;
   s32 state7; //<- Sometimes used to communicate objf# of scene loader
   s32 state8;
   s32 transformFxDone;

   u32 battleEval;
   u8 needEval;

   // MapState still TBD, but these 20 bytes are saved to card file, so starting w/ this:
   union {
      struct {
         u8 field_0x0;
         u8 field_0x1;
         u8 field_0x2;
         u8 field_0x3;
         u8 field_0x4;
         u8 field_0x5;
         u8 field_0x6;
         u8 field_0x7;
         u8 field_0x8;
         u8 field_0x9;
         u8 field_0xa;
         u8 foundHiddenItem1;
         u8 foundHiddenItem2;
         u8 field_0xd;
         u8 field_0xe;
         u8 field_0xf;
         u8 field_0x10;
         u8 field_0x11;
         u8 field_0x12;
         u8 field_0x13;
      } s;
      u8 bytes[20];
   } mapState;

   u8 signal;
   u8 field_0x96;
   u8 preciseSprites;

   u8 field_0x98;
   u8 msgFinished;
   u8 searchX;
   u8 searchZ;

   u8 droppedItem;
   u8 field_0x9d;
   s16 turn;
   u8 activeItem;
   u8 activeItemSlot;
   s16 unitListPortraitId;
   u8 mapNum; // <- also serves as battleNum when <= 43
   u8 fieldRenderingDisabled;
   u8 vsyncMode;
   u8 field_0xa7;
   s16 depot[DEPOT_CT];
   s32 gold;
   u8 field_0x1d8;
   u8 msgTextWaitTimer[5];
   s16 portraitsToLoad[75];
   s16 currentPortraits[75];
   u8 inEvent;
   u8 field_0x30b;
   s16 eventResumeLocation;
   SVectorXYZ eventCameraPan;
   s16 eventCameraRot;
   s16 eventCameraHeight;
   struct Object *focus;
   u8 msgBoxFinished;
   u8 msgBoxPagePaused;
   u8 field_0x31e;
   u8 field_0x31f;
   s8 **currentTextPointers;
   u8 previewingRange;
   u8 zoom;
   s16 unitMarkerSpin;
   u8 lastSelectedUnit;
   u8 mono;
   u8 cameraMode;
   u8 mapCursorOutOfRange;
   u8 shielding;
   u8 demoBattleOver;
   u8 vsyncNoWait;
   u8 suppressLoadingScreen;
   u8 eventChoice;
   u8 choices[6];
   u8 enableMapScaling;
   u8 textSpeed;
   u8 field_0x339;
   u8 field_0x33a;
   u8 field_0x33b;
   struct Object *screenEffect;
   u8 field_0x340;
   u8 field_0x341;
   u8 field_0x342;
   u8 field_0x343;
} State;

extern State gState;

#endif
