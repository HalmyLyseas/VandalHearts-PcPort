/* Shared FX helper toolbox used across the spells_*, maps_* and fx_* units: object spawn
 * and unit-snap helpers (SnapToUnit, CreatePositionedObj, SphericalToVector), the
 * RenderMaskEffect primitive (RenderSphere/RenderLightningBolt live in fx_event_scenes.c,
 * address-locked there), the camera-zoom service object (Objf277_Zoom, spawned on every
 * map by SetupMapExtras; its set/stop API SetCameraZoomTarget_Unused/StopCameraZoom_Unused
 * is dead in retail), the Ice Storm camera shake (Objf279, a spell stray), the screen
 * dim/undim pair (DimScreen/UndimScreen, used by the save prompt), a stripped debug-print
 * pair (PrintDigit / Noop_DebugPrintValue), and two controller-2 debug stubs
 * (Objf391_Unused, Objf674_DebugSounds). Handlers reachable from no spell table, no event
 * script and no code path are cut content, suffixed _Unused. */
#include "common.h"
#include "object.h"
#include "units.h"
#include "state.h"
#include "graphics.h"
#include "field.h"

s32 D_801233A8;

#undef OBJF
#define OBJF 391
/* Pad-2/L2 state toggle over D_801233A8, which nothing reads -- a vestigial debug stub
 * (same controller-2 idiom as its neighbours Objf674/714). No dispatcher references 391. */
void Objf391_DebugStub_Unused(Object *obj) {
   switch (obj->state) {
   case 0:
      D_801233A8 = 0;
      obj->state++;

   // fallthrough
   case 1:
      if (gSavedPad2StateNewPresses & PAD_L2) {
         obj->state = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 674
void Objf674_DebugSounds(Object *obj) {
   static s32 unitId = 1;

   switch (obj->state) {
   case 0:
      obj->d.dataStore.shorts[0] = 1; //
      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 1:
      if (gSavedPad2State & PAD_R1) {
         unitId++;
      }
      if (gSavedPad2State & PAD_R2) {
         unitId--;
      }
      if (gSavedPad2StateNewPresses & PAD_L2) {
         obj->state2++;
         obj->state2 %= 2;
      }

      switch (obj->state2) {
      case 0:
         if (gSavedPad2StateNewPresses & PAD_UP) {
            gUnitSoundDelays_Attacking1[unitId][0]++;
         }
         if (gSavedPad2StateNewPresses & PAD_DOWN) {
            // Note: Using "-= 1" instead of the decrement causes a mismatch.
            gUnitSoundDelays_Attacking1[unitId][0]--;
         }
         if (gSavedPad2StateNewPresses & PAD_RIGHT) {
            gUnitSoundDelays_Attacking1[unitId][1]++;
         }
         if (gSavedPad2StateNewPresses & PAD_LEFT) {
            gUnitSoundDelays_Attacking1[unitId][1]--;
         }
         break;

      case 1:
         if (gSavedPad2StateNewPresses & PAD_UP) {
            gUnitSoundDelays_Attacking2[unitId][0]++;
         }
         if (gSavedPad2StateNewPresses & PAD_DOWN) {
            gUnitSoundDelays_Attacking2[unitId][0]--;
         }
         if (gSavedPad2StateNewPresses & PAD_RIGHT) {
            gUnitSoundDelays_Attacking2[unitId][1]++;
         }
         if (gSavedPad2StateNewPresses & PAD_LEFT) {
            gUnitSoundDelays_Attacking2[unitId][1]--;
         }
         break;
      }

      break;
   }
}

Object *SnapToUnit(Object *obj) {
   Object *unitSprite;

   unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
   obj->x1.n = unitSprite->x1.n;
   obj->z1.n = unitSprite->z1.n;
   obj->y1.n = unitSprite->y1.n;

   return unitSprite;
}

SVECTOR *SphericalToVector(SVECTOR *vec, s32 param_2, s32 theta1, s32 theta2) {
   s32 r;
   SVECTOR local_20;

   local_20.vy = param_2 * rsin(theta1) >> 12;
   r = param_2 * rcos(theta1) >> 12;
   local_20.vx = r * rcos(theta2) >> 12;
   local_20.vz = r * rsin(theta2) >> 12;

   *vec = local_20;
   return vec;
}

Object *CreatePositionedObj(Object *target, s16 objf) {
   Object *obj;

   obj = Obj_GetUnused();
   obj->functionIndex = objf;
   obj->x1.n = target->x1.n;
   obj->z1.n = target->z1.n;
   obj->y1.n = target->y1.n;
   return obj;
}

void PrintDigit(s32 x, s32 y, s32 digit) {
   // Unused?

   POLY_FT4 *poly;

   x *= 8;
   y *= 8;

   poly = &gGraphicsPtr->quads[gQuadIndex++];
   setcode(poly, GPU_CODE_POLY_FT4);
   setXYWH(poly, x, y, 8, 8);
   setUVWH(poly, digit * 8 + 16, 0, 8, 8);

   setTPage(poly, 0, 1, 640, 256);
   poly->clut = gClutIds[17];
   setRGB0(poly, 128, 128, 128);
   AddPrim(&gGraphicsPtr->ot[OT_SIZE - 1], poly);
}

void Noop_DebugPrintValue(s32 param_1, s32 param_2, s32 param_3) {
   /* All three call sites pass (col, row, value) and PrintDigit above is its orphaned
    * glyph blitter: a debug value-printer emptied for retail. */
}

void Objf688_Noop(Object *obj) {}

Object *gCameraZoomObj;

#undef OBJF
#define OBJF 277
void Objf277_Zoom(Object *obj) {
   //? Maybe for debugging? Spawned by SetupMapExtras() (assigned to gCameraZoomObj); can be manipulated
   // via SetCameraZoomTarget_Unused() / StopCameraZoom_Unused(), but those appear to be unused.

   s32 smoothness;

   switch (obj->state) {
   case 0:
      break;
   case 1:
      smoothness = OBJ.smoothness & 0x7;
      gCameraZoom.vz += (OBJ.dstZoom - gCameraZoom.vz) >> smoothness;
      break;
   }
}

void SetCameraZoomTarget_Unused(s16 param_1, s16 param_2, s16 zoom, s16 smoothness) {
   switch (gCameraZoomObj->state) {
   case 0:
      gCameraZoomObj->state++;

   // fallthrough
   default:
      gCameraZoomObj->d.objf277.panX_unused = param_1;
      gCameraZoomObj->d.objf277.panZ_unused = param_2;
      gCameraZoomObj->d.objf277.dstZoom = zoom;
      gCameraZoomObj->d.objf277.smoothness = smoothness;
      break;
   }
}

void StopCameraZoom_Unused(void) { gCameraZoomObj->state = 0; }

#undef OBJF
#define OBJF 279
void Objf279_IceStorm_Camera(Object *obj) {
   static s16 zoomLevels[6] = {256, 384, 400, 480, 512, 768};
   static s16 angles[6] = {DEG(11.25), DEG(22.5), DEG(33.75), DEG(45), DEG(56.25), DEG(67.5)};

   Object *targetSprite;
   s32 rotY;
   s16 diff;

   targetSprite = OBJ.targetSprite;

   switch (obj->state) {
   case 0:
      gCameraRotation.vy &= 0xfff;
      rotY = GetBestViewOfTarget(targetSprite->z1.s.hi, targetSprite->x1.s.hi, 1);
      diff = rotY - gCameraRotation.vy;
      if (diff > DEG(0)) {
         if (diff > DEG(180)) {
            rotY -= DEG(360);
         }
      } else if (diff < DEG(-180)) {
         rotY += DEG(360);
      }
      OBJ.dstCamRotY = rotY;
      OBJ.dstZoom = zoomLevels[rand() % 5 + 1];
      OBJ.dstCamRotX = angles[rand() % 5 + 1];
      OBJ.delayType = rand() % 3 + 2;

      switch (OBJ.delayType) {
      case 2:
         OBJ.timer = 35;
         break;
      case 3:
         OBJ.timer = 50;
         break;
      case 4:
         OBJ.timer = 65;
         break;
      }

      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer != 0) {
         PanCamera(targetSprite->x1.n, targetSprite->y1.n + CV(0.5), targetSprite->z1.n, 3);
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 3;
         gCameraRotation.vx += (OBJ.dstCamRotX - gCameraRotation.vx) >> 3;
         gCameraZoom.vz += (OBJ.dstZoom - gCameraZoom.vz) >> 3;
      } else {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void DimScreen(void) {
   gState.screenEffect->state = 6;
   gState.screenEffect->d.objf369.color.r = gState.screenEffect->d.objf369.color.g =
       gState.screenEffect->d.objf369.color.b = 64;
   SetScreenEffectOrdering(-10);
}

void UndimScreen(void) {
   // for un-dim?
   gState.screenEffect->state = 7;
   gState.screenEffect->d.objf369.color.r = gState.screenEffect->d.objf369.color.g =
       gState.screenEffect->d.objf369.color.b = 0;
   SetScreenEffectOrdering(-10);
   gState.screenEffect->state2 = 2;
   SetScreenEffectOrdering(0);
}

void RenderMaskEffect(Object *unitSprite, MaskEffectPreset *preset) {
   Object *clonedSprite;
   s32 spriteX, spriteY, spriteW, spriteH;
   POLY_FT4 *poly;
   s32 x, y;
   s32 cell;

   clonedSprite = Obj_GetUnused();
   CopyObject(unitSprite, clonedSprite);
   clonedSprite->functionIndex = OBJF_NOOP;
   clonedSprite->d.sprite.hidden = 0;
   GetUnitSpriteVramRect(unitSprite, &spriteX, &spriteY, &spriteW, &spriteH);
   cell = gGfxTPageCells[preset->srcGfxIdx];
   x = (cell & 0xf) * 256 + gGfxSubTextures[preset->srcGfxIdx][0];
   y = (cell >> 4) * 256 + gGfxSubTextures[preset->srcGfxIdx][1];
   ApplyMaskEffect(spriteX, spriteY, spriteW + 1, spriteH + 1, x, y, preset->width, preset->height,
                   preset->dstGfxIdx, 0);
   clonedSprite->d.sprite.gfxIdx = GFX_MASK_EFFECT_1;
   clonedSprite->d.sprite.clut = preset->clut;
   clonedSprite->d.sprite.semiTrans = preset->semiTrans;

   if (unitSprite->d.sprite.gfxIdx >= 21 && unitSprite->d.sprite.gfxIdx <= 30) {
      clonedSprite->d.sprite.boxIdx = 1;
   } else {
      clonedSprite->d.sprite.boxIdx = 0;
   }

   AddObjPrim6(gGraphicsPtr->ot, clonedSprite, 0);
   poly = &gGraphicsPtr->quads[gQuadIndex - 1];
   setRGB0(poly, preset->color.r, preset->color.g, preset->color.b);
   clonedSprite->functionIndex = OBJF_NULL;
}

