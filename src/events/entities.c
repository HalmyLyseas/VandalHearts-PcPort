/* The event-entity interpreter: the object behind every scripted cutscene actor.
 *
 * Objf409_EventEntity (~870 lines) runs one EVDATA*.DAT script per entity, bound by
 * SetupEventEntity in units/actor.c. state3 is the run state (0 create sprite, 1 fetch the
 * next s16 opcode/arg pair, 2 execute or block), and the opcodes form one flat switch --
 * motion/facing (3-8), waits (1/9/0xa), animation select (2, 0x30), control flow (0xc
 * relative branch, 0x11/0x12 resume other entities), and a long tail of one-shot effects:
 * audio, dialogue, camera, terrain (0x52/0x53 spawn the face-elevation objects in
 * maps/unpack.c), and 0x1d, which spawns an object by raw objf index -- the mechanism
 * behind most per-scene effects in events/fx_scenes.c and the spells_* and maps_* units.
 *
 * Objf590_BattleTurnTicker closes the file: a per-turn map script live only on the two
 * timed maps (13, the collapsing bridge; 33, Kira over the lava pit). */
#include "common.h"
#include "object.h"
#include "window.h"
#include "units.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "graphics.h"
#include "audio.h"

/* units/actor.c -- same TU before the split; the prototypes keep the retail narrow-arg
 * call codegen (definition-in-scope semantics). */
void MsgBox_ShowForSprite(Object *sprite, u8 lower, u8 omitTail);
void MsgBox_SetPortrait(s16 portraitId, u8 lower);
void MsgBox_Close(u8 lower);
void UpdateUnitSpriteOrientation(Object *sprite);
void StepEntitySpriteTowardDest(Object *sprite, Object *entity);
void ReserveSprite(u8 srcIdxWithinSheet, u8 dstStripIdx, u8 dstSubIdx);

#undef OBJF
#define OBJF 409
void Objf409_EventEntity(Object *obj) {
   // obj->state3: runState
   Object *sprite;
   Object *obj1;
   Object *obj2;
   s16 argument;
   s16 *pNextCommand;
   s16 *pCurrentCommand;
   s32 tmp;
   s32 i, j;
   u8 **animSet;

   sprite = OBJ.sprite;

   // Handle run state:
   switch (obj->state3) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.coords[0].z = rand() % 256 - 128;
      sprite->d.sprite.stripIdx = OBJ.stripIdxA;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->x3.n = obj->x1.n;
      sprite->z3.n = obj->z1.n;
      OBJ.sprite = sprite;
      obj->state3++;

   // fallthrough
   HandleRunState1:
   case 1:
      pNextCommand = OBJ.pNextCommand;
      obj->mem = *pNextCommand;
      OBJ.pNextCommand = pNextCommand + 2;
      obj->state3++;
      OBJ.commandState = 0;

   // fallthrough
   case 2:
      pNextCommand = OBJ.pNextCommand;
      argument = pNextCommand[-1];

      switch (obj->mem) {
      case 1:
         // Yield until given location
         if (gState.eventResumeLocation >= argument) {
            obj->state3 = 1;
            goto HandleRunState1;
         }
         // Continue waiting for resume
         break;

      case 2:
         // Play base-set animation
         if ((OBJ.animIdx != argument * 2) || OBJ.usingAltAnimSet == 1) {
            // Need to switch
            OBJ.animIdx = argument * 2;
            OBJ.usingAltAnimSet = 0;
            sprite->d.sprite.animInitialized = 0;
            sprite->d.sprite.animFinished = 0;
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 3:
         sprite->x3.n = argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 4:
         sprite->z3.n = argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 5:
         sprite->x3.n = argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 6:
         sprite->z3.n = argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 7:
         sprite->x2.n = argument;
         sprite->z2.n = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 8:
         sprite->d.sprite.direction = argument << 10;
         obj->state3 = 1;
         goto HandleRunState1;

      case 9:
         if (sprite->d.sprite.finishedMoving) {
            obj->state3 = 1;
            goto HandleRunState1;
         }
         break;

      case 0xa:
         if (sprite->d.sprite.animFinished) {
            obj->state3 = 1;
            goto HandleRunState1;
         }
         break;

      case 0xb:

         switch (OBJ.commandState) {
         case 0:
            OBJ.timer = argument;
            OBJ.commandState++;
            break;
         case 1:
            if (--OBJ.timer == 0) {
               obj->state3 = 1;
               goto HandleRunState1;
            }
            break;
         }

         break;

      case 0xc:
         //? Branch (relative to current command)
         pCurrentCommand = OBJ.pNextCommand - 2;
         OBJ.pNextCommand = pCurrentCommand + argument * 2;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0xd:
         gState.focus = sprite;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0xe:
         gState.focus = NULL;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0xf:

         switch (OBJ.commandState) {
         case 0:
            StartUnitSpritesDecoder(sprite->d.sprite.stripIdx);
            OBJ.commandState++;
            break;
         case 1:
            if (!gDecodingSprites) {
               obj->state3 = 1;
               goto HandleRunState1;
            }
            break;
         }

         break;

      case 0x10:
         if (gState.eventResumeLocation >= argument) {
            OBJ.pNextCommand += 2;
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x11:
      case 0x12:
         // Resume suspended entities (up to given "location"), effectively used to branch between
         // entity scripts
         if (gState.eventResumeLocation < argument) {
            gState.eventResumeLocation = argument;
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x13:
         sprite->x1.n = sprite->x3.n;
         sprite->z1.n = sprite->z3.n;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x14:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_EVENT_ZOOM;
         obj1->d.objf410.zoom = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x15:
         sprite->x1.n = argument;
         sprite->x3.n = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x16:
         sprite->z1.n = argument;
         sprite->z3.n = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x17:
         sprite->x3.n = sprite->x1.n + argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x18:
         sprite->z3.n = sprite->z1.n + argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x19:
         sprite->x3.n = sprite->x1.n + argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1a:
         sprite->z3.n = sprite->z1.n + argument;
         sprite->d.sprite.finishedMoving = 0;
         OBJ.maintainDirection = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1b:
         sprite->d.sprite.hidden = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1c:
         sprite->d.sprite.hidden = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1d:
         // Spawn an arbitrary obj
         obj1 = Obj_GetUnused();
         obj1->functionIndex = argument;
         obj1->d.entitySpawn.entitySpriteParam = sprite;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1e:
         MsgBox_ShowForSprite(sprite, argument, 0);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x1f:
         MsgBox_Close(argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x20:
         MsgBox_SetText(1, argument, 0x100);
         gState.msgBoxFinished = 0;
         gState.msgBoxPagePaused = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x21:
         MsgBox_SetText(2, argument, 0x100);
         gState.msgBoxFinished = 0;
         gState.msgBoxPagePaused = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x22:
         if (gState.msgBoxFinished) {
            gState.msgBoxFinished = 0;
            obj->state3 = 1;
            goto HandleRunState1;
         }
         break;

      case 0x23:
         if (gState.msgBoxPagePaused != 0) {
            gState.msgBoxPagePaused = 0;
            obj->state3 = 1;
            goto HandleRunState1;
         }
         break;

      case 0x24:
         gState.eventCameraRot = gCameraRotation.vy = (argument << 10) | DEG(45);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x25:
         gState.eventCameraRot = tmp = argument << 10;
         gState.eventCameraRot += DEG(45);

         i = gState.eventCameraRot - gCameraRotation.vy;
         if (i > DEG(180)) {
            gState.eventCameraRot -= DEG(360);
         }
         if (i < DEG(-180)) {
            gState.eventCameraRot += DEG(360);
         }

         obj->state3 = 1;
         goto HandleRunState1;

      case 0x26:
         gState.eventCameraPan.x = argument;
         gCameraPos.vx = -(argument >> 3);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x27:
         gState.eventCameraPan.y = argument;
         gCameraPos.vy = (argument + gState.eventCameraHeight) >> 3;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x28:
         gState.eventCameraPan.z = argument;
         gCameraPos.vz = -(argument >> 3);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x29:
         gState.eventCameraPan.x = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2a:
         gState.eventCameraPan.y = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2b:
         gState.eventCameraPan.z = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2c:
         obj1 = gState.focus;
         gState.eventCameraPan.x = obj1->x1.n;
         gState.eventCameraPan.y = obj1->y1.n;
         gState.eventCameraPan.z = obj1->z1.n;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2d:
         gCameraZoom.vz = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2e:
         OBJ.stripIdxA = argument + 2;
         sprite->d.sprite.stripIdx = argument + 2;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x2f:
         if (argument == 0) {
            sprite->d.sprite.stripIdx = OBJ.stripIdxA;
         } else {
            sprite->d.sprite.stripIdx = OBJ.stripIdxB;
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x30:
         // Play alt-set animation
         if ((OBJ.animIdx != argument * 2) || !OBJ.usingAltAnimSet) {
            // Need to switch
            OBJ.animIdx = argument * 2;
            OBJ.usingAltAnimSet = 1;
            sprite->d.sprite.animInitialized = 0;
            sprite->d.sprite.animFinished = 0;
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x31:
      case 0x32:
         MsgBox_SetPortrait(argument, obj->mem == 0x32);
         obj->state3 = 1;
         goto HandleRunState1;

      //?
      case 0x33:
         i = 0;
         goto ReserveSpriteInSubIdxI;
      case 0x34:
         i = 1;
         goto ReserveSpriteInSubIdxI;
      case 0x35:
         i = 2;
         goto ReserveSpriteInSubIdxI;
      case 0x36:
         i = 3;
         goto ReserveSpriteInSubIdxI;
      case 0x37:
         i = 4;
         goto ReserveSpriteInSubIdxI;
      case 0x38:
         i = 5;
         goto ReserveSpriteInSubIdxI;
      case 0x39:
         i = 6;
         goto ReserveSpriteInSubIdxI;
      case 0x3a:
         i = 7;
         goto ReserveSpriteInSubIdxI;
      case 0x3b:
         i = 8;
         goto ReserveSpriteInSubIdxI;
      case 0x3c:
         i = 9;
      ReserveSpriteInSubIdxI: //?: Could use something like Duff's device instead?
         ReserveSprite(argument, sprite->d.sprite.stripIdx, i);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x3d:
         PerformAudioCommand(argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x3e:
         PerformAudioCommand(AUDIO_CMD_STOP_SEQ);
         LoadSeqSet(argument);
         FinishLoadingSeq();
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x3f:
      case 0x55:

         switch (OBJ.commandState) {
         case 0:
            OBJ.timer = 35;
            obj2 = Obj_GetUnused();
            obj2->functionIndex = OBJF_BLOOD_SPURT;
            obj2->d.objf205.unitSprite = sprite;
            if (obj->mem == 0x55) {
               obj->state3 = 1;
               goto HandleRunState1;
            } else {
               OBJ.commandState++;
            }
            break;

         case 1:
            if (--OBJ.timer == 0) {
               obj2 = Obj_GetUnused();
               obj2->functionIndex = OBJF_SLAY_UNIT;
               obj2->d.objf131.unitSprite = sprite;
               obj->state3 = 1;
               goto HandleRunState1;
            }
            break;
         }

         break;

      case 0x40:

         switch (OBJ.commandState) {
         case 0:
            OBJ.timer = 50;
            OBJ.commandState++;

         // fallthrough
         case 1:
            if (--OBJ.timer == 0) {
               gState.primary = STATE_SET_SCENE_STATE;
               gState.secondary = 0;
               gState.state3 = 0;
               gState.state4 = 0;
            }
            break;
         }

         break;

      case 0x41:
         gState.eventCameraHeight = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x42:
         MsgBox_ShowForSprite(sprite, argument, 1);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x43:
         FadeOutScreen(2, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x44:
         FadeInScreen(2, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x45:
         FadeOutScreen(1, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x46:
         FadeInScreen(1, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x47:
         OBJ.elevationType = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x48:

         switch (OBJ.commandState) {
         case 0:
            gWindowChoicesTopMargin = 10;
            gWindowChoiceHeight = 17;
            DrawWindow(0x34, 0, 170, 240, 54, 40, 93, WBS_CROSSED, 2);
            DrawText(12, 181, 25, 2, 0, gState.currentTextPointers[argument]);
            DisplayBasicWindow(0x34);
            gWindowActiveIdx = 0x34;
            OBJ.commandState++;
            break;

         case 1:
            if (gWindowChoice.raw == 0x3401) {
               CloseWindow(0x34);
               gState.eventChoice = 0;
               OBJ.pNextCommand += 2;
               OBJ.commandState++;
            }
            if (gWindowChoice.raw == 0x3402) {
               CloseWindow(0x34);
               gState.eventChoice = 1;
               OBJ.commandState++;
            }
            break;

         case 2:
            obj->state3 = 1;
            goto HandleRunState1;
         }

         break;

      case 0x49:
         gWindowChoicesTopMargin = 10;
         DrawWindow(0x34, 0, 170, 240, 36, 40, 97, WBS_CROSSED, 0);
         DrawText(12, 180, 25, 2, 0, gState.currentTextPointers[argument]);
         DisplayBasicWindow(0x34);
         gWindowActiveIdx = 0x34;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x4a:
         if (PressedCircleOrX()) {
            obj->state3 = 1;
            CloseWindow(0x34);
            obj->state3 = 1;
            goto HandleRunState1;
         }
         break;

      case 0x4b:
         gMapMinX = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x4c:
         gMapMinZ = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x4d:
         gMapSizeX = argument;
         gMapMarginX = 0;
         gMapMaxX = gMapMinX + gMapSizeX - 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x4e:
         gMapSizeZ = argument;
         gMapMarginZ = 0;
         gMapMaxZ = gMapMinZ + gMapSizeZ - 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x4f:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_STRETCH_WARP_SPRITE;
         obj2->d.objf062.sprite = sprite;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x50:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_STRETCH_WARP_SPRITE;
         obj2->d.objf062.sprite = sprite;
         obj2->mem = 1; // Reversed (warp in)
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x51:
         CloseWindow(0x34);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x52:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_ADJUST_FACE_ELEVATION;
         obj2->state2 = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x53:
         obj2 = Obj_GetUnused();
         obj2->functionIndex = OBJF_SLIDING_FACE;
         obj2->state2 = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x54:
         Obj_ResetByFunction(argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x56:
         gState.preciseSprites = 1;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x57:
         gState.preciseSprites = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x58:
         DrawWindow(0x43, 0, 100, 296, 64, 12, 161, WBS_ROUNDED, 0);
         DisplayCustomWindow(0x43, 0, 1, 1, 0, 25);
         DisplayCustomWindow(0x44, 0, 1, 1, 0, 25);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x59:
         gCameraPos.vy = (sprite->y1.n + CV(1.0) + gState.eventCameraHeight) >> 3;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5a:
         (gState.screenEffect)->state = 5;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5b:
         (gState.screenEffect)->state = 0;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5c:
         SetScreenEffectOrdering(argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5d:
         (gState.screenEffect)->state2 = argument; // semiTransRate
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5e:
         (gState.screenEffect)->d.objf369.color.r = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x5f:
         (gState.screenEffect)->d.objf369.color.g = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x60:
         (gState.screenEffect)->d.objf369.color.b = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x65:
         (gState.screenEffect)->d.objf369.rd = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x66:
         (gState.screenEffect)->d.objf369.gd = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x67:
         (gState.screenEffect)->d.objf369.bd = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x68:
         (gState.screenEffect)->d.objf369.rmax = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x69:
         (gState.screenEffect)->d.objf369.gmax = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6a:
         (gState.screenEffect)->d.objf369.bmax = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6b:
         gLightColor.r = gLightColor.g = gLightColor.b = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6c:
         gLightColor.r += argument;
         gLightColor.g = gLightColor.b = gLightColor.r;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6d:
         (gState.screenEffect)->d.objf369.color.r = argument;
         (gState.screenEffect)->d.objf369.color.g = argument;
         (gState.screenEffect)->d.objf369.color.b = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6e:
         (gState.screenEffect)->d.objf369.rd = argument;
         (gState.screenEffect)->d.objf369.gd = argument;
         (gState.screenEffect)->d.objf369.bd = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x6f:
         (gState.screenEffect)->d.objf369.rmax = argument;
         (gState.screenEffect)->d.objf369.gmax = argument;
         (gState.screenEffect)->d.objf369.bmax = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x70:
      case 0x71:
         (gState.screenEffect)->d.objf369.semiTrans = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x72:
         for (i = 0; i < 30; i++) {
            for (j = 0; j < 65; j++) {
               gPathGrid0[i][j] = PATH_STEP_UNSET;
            }
         }
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x73:
         gCameraRotation.vx = argument;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x74:
         gScreenFade = Obj_GetUnused();
         gScreenFade->functionIndex = OBJF_FADE;
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x75:
         // Set up fade
         Event_FadeOutScreen(1, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x76:
         // Set up fade
         Event_FadeInScreen(1, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x77:
         // Set up fade
         Event_FadeOutScreen(2, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x78:
         // Set up fade
         Event_FadeInScreen(2, argument);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x79:
         Obj_ResetByFunction(OBJF_FADE);
         obj->state3 = 1;
         goto HandleRunState1;

      case 0x7a:
         MsgBox_SetText2(1, argument, 0x100);
         gState.msgBoxFinished = 0;
         gState.msgBoxPagePaused = 0;
         obj->state3 = 1;
         goto HandleRunState1;
      } // switch (command) (via runState:2)

      break;
   } // switch (runState)

   // UpdateSprite_80051464:
   StepEntitySpriteTowardDest(sprite, obj);
   UpdateUnitSpriteOrientation(sprite);
   if (!OBJ.usingAltAnimSet) {
      animSet = OBJ.baseAnimSet;
   } else {
      animSet = OBJ.altAnimSet;
   }
#ifdef PC_PORT
   /* PC_PORT (Stage 2.3): OBJ.baseAnimSet/altAnimSet can be NULL (the gEvtEntities data-gen
    * residual, see milestone_cutscene_units_fixed) -- animSet[idx] then reads through NULL.
    * The 2.2 fault handler read 0; mirror that (animData = NULL), which UpdateUnitSpriteAnimation
    * below already tolerates (its own PC_PORT zero-table guard) -- bit-identical to the validated
    * build. NULL site: Objf409_EventEntity+0x1f5e. See exchange/56. */
   sprite->d.sprite.animData =
       animSet ? animSet[OBJ.animIdx + sprite->d.sprite.facingFront] : (u8 *)0;
#else
   sprite->d.sprite.animData = animSet[OBJ.animIdx + sprite->d.sprite.facingFront];
#endif
   UpdateUnitSpriteAnimation(sprite);
#ifdef PC_DEBUG_SPRITE_LOG
   { extern void PC_DebugEvtEntityLog(int, int, int, const void *, const void *, const void *,
                                      int, int, const void *, int, int, int, int, int, int, int);
     extern s16 gEvtEntityData[];
     s16 *pc = OBJ.pNextCommand;
     long cmdOff = (pc >= gEvtEntityData && pc < gEvtEntityData + 4096) ? (long)(pc - gEvtEntityData) : -1;
     int cmdArg = (cmdOff > 0) ? pc[-1] : 0;
     PC_DebugEvtEntityLog((int)(obj - gObjectArray), sprite->x1.s.hi, sprite->z1.s.hi,
                          (const void *)OBJ.baseAnimSet, (const void *)OBJ.altAnimSet,
                          (const void *)animSet, OBJ.animIdx, sprite->d.sprite.facingFront,
                          (const void *)sprite->d.sprite.animData, sprite->d.sprite.gfxIdx,
                          OBJ.usingAltAnimSet, obj->state3, obj->mem, OBJ.commandState,
                          (int)cmdOff, cmdArg); }
#endif
   RenderUnitSprite(gGraphicsPtr->ot, sprite, OBJ.elevationType + 1);
}

void Objf590_BattleTurnTicker(Object *obj) {
   switch (obj->state) {
   case 0:
      gState.field_0x96 = 1;

      switch (gState.mapNum) {
      case 13:
         // Bridge crumbling in sections
         gState.mapState.s.field_0x0++;
         obj->state++;
         break;
      case 33:
         // Kira being gradually lowered into lava pit
         gState.mapState.s.field_0x0++;
         obj->state++;
         break;
      default:
         gState.field_0x96 = 0;
         obj->state++;
         break;
      }

      break;

   case 1:
      if (gState.field_0x96 == 0) {
         gState.field_0x98 = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}
