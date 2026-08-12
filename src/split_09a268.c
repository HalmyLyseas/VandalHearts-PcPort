/* Shared FX helpers + event-scene special effects (segment 0x9a268).
 *
 * The first ~260 lines are a toolbox used across the fx_* / map_effects_* units: object
 * spawn and unit-snap helpers (SnapToUnit, CreatePositionedObj, SphericalToVector), tree-wide
 * render primitives (RenderMaskEffect, RenderSphere, RenderLightningBolt), the camera-zoom
 * service object (Objf277_Zoom, spawned on every map by SetupMapExtras; its set/stop API
 * SetCameraZoomTarget_Unused/StopCameraZoom_Unused is dead in retail), the screen dim/undim
 * pair (DimScreen/UndimScreen, used by the save prompt), and a stripped debug-print pair
 * (PrintDigit / Noop_DebugPrintValue).
 *
 * The remainder is cutscene/event-entity effects, spawned almost exclusively by EVDATA*.DAT
 * scripts (event opcode 0x1d names an objf index directly -- see the interpreter in
 * split_03c94c.c): per-scene set pieces (Map44/Scn00, Map36/Scn74-75, Map48/Scn20,
 * Map43/Scn93, Map67/Scn34, Map61/Scn83, Map20, Map30), reusable particles (rays, sparks,
 * streaks, rings), entity-overlay transforms (hide the sprite, cross-fade masked clones),
 * the Dimensional Rift family, blend-mode toggles, and controller-2 debug handlers
 * (Objf674_DebugSounds, Objf714_DebugCamera). Handlers reachable from no spell table, no
 * event script and no code path are cut content, suffixed _Unused. */
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

Object *D_801233CC;

#undef OBJF
#define OBJF 277
void Objf277_Zoom(Object *obj) {
   //? Maybe for debugging? Spawned by SetupMapExtras() (assigned to D_801233CC); can be manipulated
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
   switch (D_801233CC->state) {
   case 0:
      D_801233CC->state++;

   // fallthrough
   default:
      D_801233CC->d.objf277.todo_x24 = param_1;
      D_801233CC->d.objf277.todo_x26 = param_2;
      D_801233CC->d.objf277.dstZoom = zoom;
      D_801233CC->d.objf277.smoothness = smoothness;
      break;
   }
}

void StopCameraZoom_Unused(void) { D_801233CC->state = 0; }

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

static s16 sSparkleAnimData_80101838[14] = {5, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                            3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                            1, GFX_NULL};

#undef OBJF
#define OBJF 393
void Objf393_Map44_Scn00_ExplosionRays(Object *obj) {
   SVECTOR local_58;
   SVECTOR local_50;
   SVECTOR local_48;
   SVECTOR local_40;
   Object *target;
   Object *sprite;
   s32 i;
   s32 ct;
   s16 theta1;
   s16 theta2;
   s16 tmp_0x80;
   s16 tmp_0x800;

   switch (obj->state) {
   case 0:
      target = OBJ.entitySprite;
      obj->x1.n = target->x1.n;
      obj->z1.n = target->z1.n;
      obj->y1.n = target->y1.n + CV(0.5);
      obj->state++;
      break;

   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_COLOR_15;
      sprite->d.sprite.semiTrans = 4;
      sprite->d.sprite.clut = CLUT_BLUES;
      obj->x2.n = 16;
      obj->state++;

   // fallthrough
   case 2:
      gCameraZoom.vz += 3;
      sprite = OBJ.sprite;
      obj->x2.n = obj->state3 + 16;
      obj->mem += obj->x2.n;
      theta2 = obj->mem;
      theta1 = 300 - (theta2 >> 5);
      tmp_0x80 = 0x80;
      tmp_0x800 = 0x800;
      ct = obj->state3 / 16 + 8;

      for (i = 0; i < ct; i++) {
         SphericalToVector(&local_58, tmp_0x80, theta1 + DEG(2.8125), theta2);
         SphericalToVector(&local_50, tmp_0x80, theta1, theta2);
         SphericalToVector(&local_48, tmp_0x800, theta1 + DEG(2.8125), theta2);
         SphericalToVector(&local_40, tmp_0x800, theta1, theta2);

         sprite->d.sprite.coords[0].x = obj->x1.n + local_58.vx;
         sprite->d.sprite.coords[0].z = obj->z1.n + local_58.vz;
         sprite->d.sprite.coords[0].y = obj->y1.n + local_58.vy;
         sprite->d.sprite.coords[1].x = obj->x1.n + local_50.vx;
         sprite->d.sprite.coords[1].z = obj->z1.n + local_50.vz;
         sprite->d.sprite.coords[1].y = obj->y1.n + local_58.vy;
         sprite->d.sprite.coords[2].x = obj->x1.n + local_48.vx;
         sprite->d.sprite.coords[2].z = obj->z1.n + local_48.vz;
         sprite->d.sprite.coords[2].y = obj->y1.n + local_48.vy;
         sprite->d.sprite.coords[3].x = obj->x1.n + local_40.vx;
         sprite->d.sprite.coords[3].z = obj->z1.n + local_40.vz;
         sprite->d.sprite.coords[3].y = obj->y1.n + local_40.vy;

         AddObjPrim4(gGraphicsPtr->ot, sprite);
         theta2 += DEG(360) / ct;
      }
      if (obj->state3 == 128) {
         obj->state++;
      }
      break;

   case 3:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 276
/* Expanding tiled-flames hemisphere, red twin of Objf348_BlueFlameDome_Unused. Unfinished:
 * state 2 (its own gSignal3 teardown) is unreachable, the flame-wall anim and a local clut
 * table are dead, and the dome grows without bound. Nothing dispatches 276. */
void Objf276_RedFlameDome_Unused(Object *obj) {
   static s16 animData[20] = {
       0, GFX_FLAME_WALL_1, 2, GFX_FLAME_WALL_2, 2, GFX_FLAME_WALL_3, 2, GFX_FLAME_WALL_4,
       2, GFX_FLAME_WALL_5, 2, GFX_FLAME_WALL_6, 2, GFX_FLAME_WALL_7, 2, GFX_FLAME_WALL_8,
       2, GFX_NULL,         1, GFX_NULL,
   };

   s16 cluts_unused[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};
   Object *obj_a0;
   Cylinder *dsCylinder;
   s32 i;
   s16 a, b;
   s32 r;

   switch (obj->state) {
   case 0:
      obj_a0 = OBJ.variant_0x24.targetParam;
      obj->x1.n = obj_a0->x1.n;
      obj->z1.n = obj_a0->z1.n;
      obj->y1.n = obj_a0->y1.n;
      OBJ.animData = animData;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_a0;
      dsCylinder = &obj_a0->d.dataStore.cylinder;
      dsCylinder->top.vx = dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->top.vz = dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->sideCt = 16;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_TILED_FLAMES;
      obj->mem = CV(0.25);
      dsCylinder->bottomRadius = CV(0.25);
      dsCylinder->topRadius = CV(0.25);
      dsCylinder->useColor = 1;
      dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = 128;
      dsCylinder->theta = 0;
      dsCylinder->clut = CLUT_REDS;
      OBJ.variant_0x24.unk = 5;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2++;
      obj_a0 = OBJ.dataStore;
      dsCylinder = &obj_a0->d.dataStore.cylinder;
      dsCylinder->theta += obj->state2 * 2;
      UpdateObjAnimation(obj);

      for (i = 0; i < 8; i++) {
         a = (i + 1) * DEG(11.25);
         b = i * DEG(11.25);
         r = obj->mem;
         dsCylinder->semiTrans = 4;
         dsCylinder->bottom.vy = obj->y1.n + (r * rsin(b) >> 12);
         dsCylinder->top.vy = obj->y1.n + (r * rsin(a) >> 12);
         dsCylinder->bottomRadius = r * rcos(b) >> 12;
         dsCylinder->topRadius = r * rcos(a) >> 12;
         RenderCylinder(dsCylinder);
      }
      obj->mem += CV(0.015625);
      break;

   case 2:
      obj_a0 = OBJ.dataStore;
      obj_a0->functionIndex = OBJF_NULL;
      obj_a0 = OBJ.unused_0x58; //?
      obj_a0->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 275
void Objf275_ConvergingExplosion_Unused(Object *obj) {
   static s16 animData[] = {4, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                            2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                            2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                            2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                            0, GFX_NULL};

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_EXPLOSION_1;
      OBJ.animData = animData;
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      obj->x1.n += (obj->x2.n - obj->x1.n) >> 2;
      obj->z1.n += (obj->z2.n - obj->z1.n) >> 2;
      obj->y1.n += (obj->y2.n - obj->y1.n) >> 2;
      if (OBJ.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 273
void Objf273_OutwardRay(Object *obj) {
   s32 rnd;
   s32 r;

   switch (obj->state) {
   case 0:
      OBJ.theta1 = rand() % DEG(33.75);
      OBJ.theta2 = rand() % DEG(360);
      OBJ.unused_0x3A = rand() % 64 + 32;
      OBJ.todo_x38 = CV(0.5);
      obj->x3.n = 32;
      obj->z3.n = 8;

      rnd = (rand() >> 3) % 4096;
      if (rnd >= 512) {
         obj->mem = 3;
      } else {
         obj->mem = 2;
      }

      OBJ.todo_x54 = -32 - rand() % 64;

      switch (obj->state3) {
      case 0:
         OBJ.gfxIdx = GFX_COLOR_15;
         OBJ.semiTrans = 2;
         OBJ.clut = CLUT_BLUES;
         break;
      case 1:
         OBJ.gfxIdx = GFX_COLOR_12;
         OBJ.semiTrans = 0;
         OBJ.clut = CLUT_PURPLES;
         break;
      }

      obj->state2 = 0;

      r = (OBJ.todo_x38 * rcos(OBJ.theta1)) >> 12;
      OBJ.coords[1].x = obj->x2.n + (r * rcos(OBJ.theta2) >> 12);
      OBJ.coords[1].z = obj->z2.n + (r * rsin(OBJ.theta2) >> 12);
      OBJ.coords[3].x = obj->x2.n + (r * rcos(OBJ.theta2 + DEG(1.40625)) >> 12);
      OBJ.coords[3].z = obj->z2.n + (r * rsin(OBJ.theta2 + DEG(1.40625)) >> 12);
      OBJ.coords[1].y = OBJ.coords[3].y = obj->y2.n + (OBJ.todo_x38 * rsin(OBJ.theta1) >> 12);

      obj->state++;

   // fallthrough
   case 1:
      OBJ.todo_x38 += obj->x3.n;
      obj->x3.n += obj->z3.n;

      r = (OBJ.todo_x38 * rcos(OBJ.theta1)) >> 12;
      OBJ.coords[0].x = obj->x2.n + (r * rcos(OBJ.theta2) >> 12);
      OBJ.coords[0].z = obj->z2.n + (r * rsin(OBJ.theta2) >> 12);
      OBJ.coords[2].x = obj->x2.n + (r * rcos(OBJ.theta2 + DEG(1.40625)) >> 12);
      OBJ.coords[2].z = obj->z2.n + (r * rsin(OBJ.theta2 + DEG(1.40625)) >> 12);
      OBJ.coords[0].y = OBJ.coords[2].y = obj->y2.n + (OBJ.todo_x38 * rsin(OBJ.theta1) >> 12);

      obj->x1.n = OBJ.coords[0].x;
      obj->z1.n = OBJ.coords[0].z;
      obj->y1.n = OBJ.coords[0].y;

      AddObjPrim5(gGraphicsPtr->ot, obj);

      OBJ.coords[1].x = OBJ.coords[0].x;
      OBJ.coords[1].z = OBJ.coords[0].z;
      OBJ.coords[3].x = OBJ.coords[2].x;
      OBJ.coords[3].z = OBJ.coords[2].z;
      OBJ.coords[1].y = OBJ.coords[3].y = OBJ.coords[0].y;

      if (OBJ.todo_x38 >= CV(4.5)) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 271
void Objf271_Map36_Scn74_LeenaCastingShield(Object *obj) {
   Object *obj_s5;
   Object *forcefield;
   Cylinder *dsCylinder;
   s32 i;
   s32 theta1;
   s32 theta2;
   s32 iVar6;

   switch (obj->state) {
   case 0:
      obj_s5 = OBJ.entitySprite;
      obj->x1.n = obj_s5->x1.n;
      obj->z1.n = obj_s5->z1.n;
      obj->y1.n = obj_s5->y1.n;

      obj_s5 = Obj_GetUnused();
      obj_s5->functionIndex = OBJF_NOOP;
      dsCylinder = &obj_s5->d.dataStore.cylinder;
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n;
      dsCylinder->sideCt = 32;
      dsCylinder->semiTrans = 2;
      dsCylinder->gfxIdx = GFX_COLOR_11;
      dsCylinder->clut = CLUT_BLUES;
      dsCylinder->useColor = 1;
      dsCylinder->color.r = 255;
      dsCylinder->color.g = 255;
      dsCylinder->color.b = 255;
      OBJ.dataStore = obj_s5;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 1:
      iVar6 = (CV(1.0) - obj->state3) * 4;
      theta1 = obj->mem;
      iVar6 += CV(0.8125);
      theta2 = theta1 + DEG(2.8125);
      if (theta2 >= DEG(90)) {
         theta2 = DEG(90);
      }

      obj_s5 = OBJ.dataStore;
      dsCylinder = &obj_s5->d.dataStore.cylinder;
      dsCylinder->theta = obj->state3 << 7; //* 128
      dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = (1024 - theta1) >> 2;

      for (i = 0; i < 2; i++) {
         dsCylinder->bottomRadius = iVar6 * rcos(theta1) >> 12;
         dsCylinder->topRadius = iVar6 * rcos(theta2) >> 12;
         dsCylinder->bottom.vy = obj->y1.n + (iVar6 * rsin(theta1) >> 12);
         dsCylinder->top.vy = obj->y1.n + (iVar6 * rsin(theta2) >> 12);
         RenderCylinder(dsCylinder);
         theta1 += DEG(2.8125);
         theta2 += DEG(2.8125);
         if (theta1 >= DEG(90)) {
            theta1 = DEG(90);
         }
         if (theta2 >= DEG(90)) {
            theta1 = DEG(90);
         }
      }

      theta1 = 0;
      theta2 = DEG(11.25);
      dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = (1024 - obj->mem) >> 2;

      for (i = 0; i < 8; i++) {
         dsCylinder->bottomRadius = iVar6 * rcos(theta1) >> 12;
         dsCylinder->topRadius = iVar6 * rcos(theta2) >> 12;
         dsCylinder->bottom.vy = obj->y1.n + (iVar6 * rsin(theta1) >> 12);
         dsCylinder->top.vy = obj->y1.n + (iVar6 * rsin(theta2) >> 12);
         RenderCylinder(dsCylinder);
         theta1 += DEG(11.25);
         theta2 += DEG(11.25);
      }

      obj->mem += 64;
      if (obj->mem >= 1024) {
         obj->mem = 0;
      }

      obj->state3 += 8;
      if (obj->state3 == 192) {
         forcefield = Obj_GetUnused();
         forcefield->functionIndex = OBJF_LEENA_FORCEFIELD;
         forcefield->d.objf675.targetSprite = OBJ.entitySprite;
      }
      if (obj->state3 >= 208) {
         obj_s5->functionIndex = OBJF_NULL;
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

static s16 sCluts_801230a4[4] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};

#undef OBJF
#define OBJF 270
/* Spinning splash-textured column ringed by 8 chained lightning tendrils (Objf269) plus
 * streak/spark emitters. Unfinished: also spawns 8 UNLINKED tendrils whose link is NULL
 * (would deref 0), never terminates, and nothing dispatches 270. */
void Objf270_LightningPillar_Unused(Object *obj) {
   static s16 splashAnimData[20] = {
       0, GFX_SPLASH_1, 1, GFX_SPLASH_2, 1, GFX_SPLASH_3, 1, GFX_SPLASH_4, 1, GFX_SPLASH_5,
       1, GFX_SPLASH_6, 1, GFX_SPLASH_7, 1, GFX_SPLASH_8, 1, GFX_NULL,     1, GFX_NULL};

   s32 i;
   Object *link;   // s2
   Object *sprite; // s1
   Object *obj_s0;
   Cylinder *dsCylinder;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(0.5);

      link = obj;
      for (i = 0; i < 8; i++) {
         CreatePositionedObj(obj, OBJF_LIGHTNING_TENDRIL_UNUSED);
         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_LIGHTNING_TENDRIL_UNUSED;
         obj_s0->d.objf269.link = link;
         obj_s0->d.objf269.parent = obj;
         link = obj_s0;
      }

      gCameraZoom.vz = 256;
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.animData = splashAnimData;
      gCameraRotation.vy += 16;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;

      if (--obj->mem <= 0) {
         obj_s0 = CreatePositionedObj(obj, OBJF_STREAK_PARTICLE);
         obj_s0->x1.n += rand() % CV(0.5);
         obj_s0->z1.n += rand() % CV(0.5);
         obj_s0->y1.n += rand() % CV(0.5);
         obj_s0->d.objf099.theta1 = 0;
         obj_s0->d.objf099.theta2 = 0;
         obj_s0->d.objf099.todo_x28 = rand() % 64 + 128;
         obj_s0->d.objf099.clut = sCluts_801230a4[rand() % 4];
         obj->mem += rand() % 4;
      }

      if (--obj->mem <= 0) {
         obj_s0 = CreatePositionedObj(obj, OBJF_SPARK_PARTICLE);
         obj_s0->x1.n += rand() % CV(0.5);
         obj_s0->z1.n += rand() % CV(0.5);
         obj_s0->y1.n += rand() % CV(0.5);
         obj_s0->y2.n = rand() % 8 + 8;
         obj_s0->mem = 64;
         obj->mem += rand() % 2;
      }

      UpdateObjAnimation(sprite);

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      dsCylinder = &obj_s0->d.dataStore.cylinder;
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n + CV(1.0);
      dsCylinder->sideCt = 16;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = sprite->d.sprite.gfxIdx;
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->theta = sprite->state2;
      sprite->state2 += 32;
      dsCylinder->clut = CLUT_PURPLES;
      RenderCylinder(dsCylinder);
      obj_s0->functionIndex = OBJF_NULL;

      OBJ.todo_x24 = 0;
      OBJ.todo_x26 = 0;
      break;
   }
}

#undef OBJF
#define OBJF 269
void Objf269_LightningTendril_Unused(Object *obj) {
   static s16 lightningAnimData[20] = {0, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3,
                                       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
                                       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_NULL,
                                       1, GFX_NULL};

   Object *link;
   Object *sprite;
   s16 radius;
   s16 theta;

   link = OBJ.link;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_COLOR_15;
      sprite->d.sprite.clut = CLUT_REDS;
      sprite->d.sprite.semiTrans = 2;
      sprite->d.sprite.animData = lightningAnimData;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      obj->mem++;
      radius = abs(192 * rsin(obj->mem * 16) >> 12);
      sprite = OBJ.sprite;
      OBJ.theta2 = OBJ.theta1 = rand() % DEG(180) - DEG(90);
      UpdateObjAnimation(sprite);
      theta = OBJ.theta2;
      obj->x1.n = link->x1.n;
      obj->z1.n = link->z1.n + (radius * rcos(theta) >> 12);
      obj->y1.n = link->y1.n + (radius * rsin(theta) >> 12);
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[1].x = link->x1.n;
      sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[3].x = obj->x1.n;
      sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z = link->z1.n;
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[3].z = obj->z1.n;
      sprite->d.sprite.coords[0].y = link->y1.n - CV(0.5);
      sprite->d.sprite.coords[1].y = link->y1.n;
      sprite->d.sprite.coords[2].y = obj->y1.n - CV(0.5);
      sprite->d.sprite.coords[3].y = obj->y1.n;
      AddObjPrim3(gGraphicsPtr->ot, sprite);
      break;
   }
}

#undef OBJF
#define OBJF 099
void Objf099_StreakParticle(Object *obj) {
   Object *sprite;
   SVECTOR svec;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      sprite->d.sprite.gfxIdx = GFX_POISONED;
      sprite->d.sprite.clut = OBJ.clut;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;
      obj->mem = OBJ.todo_x30;
      obj->state++;

   // fallthrough
   case 1:
      SphericalToVector(&svec, OBJ.todo_x28, OBJ.theta1, OBJ.theta2);
      obj->x1.n += svec.vx;
      obj->z1.n += svec.vz;
      obj->y1.n += svec.vy;
      sprite = OBJ.sprite;
      sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[3].x = obj->x1.n;
      sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[2].x = sprite->x1.n;
      sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[3].z = obj->z1.n;
      sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[2].z = sprite->z1.n;
      sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[3].y = obj->y1.n + CV(0.0625);
      sprite->d.sprite.coords[0].y = sprite->y1.n;
      sprite->d.sprite.coords[2].y = sprite->y1.n + CV(0.0625);
      AddObjPrim3(gGraphicsPtr->ot, sprite);

      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      if (--obj->mem <= 0) {
         obj->state++;
      }
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 088
void Objf088_SparkParticle(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_COLOR_12;
      OBJ.clut = CLUT_BLUES;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;

      OBJ.coords[0].x = OBJ.coords[1].x = obj->x1.n;
      OBJ.coords[2].x = OBJ.coords[3].x = obj->x1.n;
      OBJ.coords[0].z = OBJ.coords[2].z = obj->z1.n;
      OBJ.coords[1].z = OBJ.coords[3].z = obj->z1.n + CV(0.03125);
      OBJ.coords[0].y = OBJ.coords[1].y = obj->y1.n;
      OBJ.coords[2].y = OBJ.coords[3].y = obj->y1.n + CV(0.03125);

      AddObjPrim3(gGraphicsPtr->ot, obj);

      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void RenderLightningBolt(Object *sprite) {
   s32 dx, dz, dy;
   s32 i;
   s16 rnd;
   Object *sprite_s0;
   s16 tmp;

   dx = (sprite->x2.n - sprite->x1.n) / 8;
   dy = (sprite->y2.n - sprite->y1.n) / 8;
   dz = (sprite->z2.n - sprite->z1.n) / 8;

   sprite_s0 = Obj_GetUnused();
   sprite_s0->functionIndex = OBJF_NOOP;
   sprite_s0->d.sprite.coords[0].y = sprite_s0->d.sprite.coords[1].y = sprite->y1.n;
   sprite_s0->d.sprite.coords[0].z = sprite->z1.n + CV(0.25);
   sprite_s0->d.sprite.coords[1].z = sprite->z1.n - CV(0.25);
   sprite_s0->d.sprite.coords[0].x = sprite_s0->d.sprite.coords[1].x = sprite->x1.n;
   sprite_s0->d.sprite.gfxIdx = sprite->d.sprite.gfxIdx;
   sprite_s0->d.sprite.clut = sprite->d.sprite.clut;
   sprite_s0->d.sprite.semiTrans = sprite->d.sprite.semiTrans;
   if (sprite_s0->d.sprite.gfxIdx == GFX_NULL
#ifdef PC_PORT
       /* Some callers pass a NON-sprite object (e.g. Objf319_Map67_Scn34_RiftArcs) whose d.sprite.gfxIdx
        * aliases their own struct. On PSX that offset reads 0 (GFX_NULL), so this fallback selects the
        * lightning gfx. On LP64 a leading pointer in the caller's struct occupies 0x24..0x2B and shifts
        * the read into the pointer, yielding a garbage index (a fixed-arena pointer's low half) that
        * slips past the ==GFX_NULL test -> OOB gGfxTPageIds[] read -> tpage 0 -> the effect samples
        * VRAM page 0 (the framebuffer) = a solid garbage "blob". Treat any out-of-range index as
        * GFX_NULL, restoring the intended GFX_LIGHTNING_5. */
       || (unsigned int)sprite_s0->d.sprite.gfxIdx >= (unsigned int)GFX_CT
#endif
   ) {
      sprite_s0->d.sprite.gfxIdx = GFX_LIGHTNING_5;
   }
   if (sprite_s0->d.sprite.clut == CLUT_NULL) {
      sprite_s0->d.sprite.clut = CLUT_BLUES;
   }
   for (i = 1; i < 8; i++) {
      sprite_s0->d.sprite.coords[2].y = sprite_s0->d.sprite.coords[3].y = sprite->y1.n + dy * i;
      rnd = rand() % CV(1.0);
      tmp = 63;
      sprite_s0->d.sprite.coords[2].z = sprite->z1.n + dz * i + (rnd - tmp);
      tmp = 191;
      sprite_s0->d.sprite.coords[3].z = sprite->z1.n + dz * i + (rnd - tmp);
      sprite_s0->d.sprite.coords[2].x = sprite_s0->d.sprite.coords[3].x = sprite->x1.n + dx * i;
      AddObjPrim4(gGraphicsPtr->ot, sprite_s0);
      sprite_s0->d.sprite.coords[0].y = sprite_s0->d.sprite.coords[1].y =
          sprite_s0->d.sprite.coords[2].y;
      sprite_s0->d.sprite.coords[0].z = sprite_s0->d.sprite.coords[2].z;
      sprite_s0->d.sprite.coords[1].z = sprite_s0->d.sprite.coords[3].z;
      sprite_s0->d.sprite.coords[0].x = sprite_s0->d.sprite.coords[2].x;
      sprite_s0->d.sprite.coords[1].x = sprite_s0->d.sprite.coords[3].x;
   }
   sprite_s0->d.sprite.coords[2].y = sprite_s0->d.sprite.coords[3].y = sprite->y2.n;
   sprite_s0->d.sprite.coords[2].z = sprite->z2.n + CV(0.25);
   sprite_s0->d.sprite.coords[3].z = sprite->z2.n - CV(0.25);
   sprite_s0->d.sprite.coords[2].x = sprite_s0->d.sprite.coords[3].x = sprite->x2.n;
   AddObjPrim4(gGraphicsPtr->ot, sprite_s0);
   sprite_s0->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 340
void Objf340_Map48_Scn20_LightningFan(Object *obj) {
   Object *obj_s0;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         obj->x1.n = CV(5.5);
         obj->z1.n = CV(2.5);
         obj->y1.n = CV(0.5);
         obj->state2++;

      // fallthrough
      case 1:
         obj->mem += 2;
         if (obj->mem >= 96) {
            obj->state2 = 0;
            obj->mem = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 1:
      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_STREAK_PARTICLE;
      obj_s0->x1.n = obj->x1.n + CV(2.0);
      obj_s0->y1.n = obj->y1.n;
      obj_s0->z1.n = obj->z1.n + (rand() % 1024 - CV(2.0));
      obj_s0->d.objf099.theta1 = DEG(180);
      obj_s0->d.objf099.theta2 = DEG(0);
      obj_s0->d.objf099.todo_x28 = rand() % 64 + 256;
      obj_s0->d.objf099.clut = sCluts_801230a4[rand() % 4];
      obj_s0->d.objf099.todo_x30 = 16;

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;
      obj_s0->x1.n = obj->x1.n;
      obj_s0->z1.n = obj->z1.n;
      obj_s0->y1.n = obj->y1.n + CV(0.5);
      obj_s0->x2.n = obj->x1.n - obj->state3;
      obj_s0->z2.n = obj->z1.n;
      obj_s0->y2.n = obj->y1.n + CV(0.5);
      obj_s0->d.sprite.clut = CLUT_BLUES;
      RenderLightningBolt(obj_s0);
      obj_s0->z2.n = obj->z1.n + (obj->state3 >> 2);
      obj_s0->d.sprite.clut = CLUT_GREENS;
      RenderLightningBolt(obj_s0);
      obj_s0->z2.n = obj->z1.n - (obj->state3 >> 2);
      obj_s0->d.sprite.clut = CLUT_PURPLES;
      RenderLightningBolt(obj_s0);
      obj_s0->functionIndex = OBJF_NULL;
      obj->state3 += (1024 - obj->state3) >> 4;
      break;

   case 2:
      gLightColor.r = 128;
      gLightColor.g = 128;
      gLightColor.b = 128;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 305
void Objf305_328_MagicStoneFx(Object *obj) {
   Object *obj_a0;
   Object *sprite;
   s32 i;
   s32 iVar7;
   u32 tmp;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      obj_a0 = OBJ.variant_0x24.entitySpriteParam;
      obj->x1.n = obj_a0->x1.n;
      obj->z1.n = obj_a0->z1.n;
      obj->y1.n = obj_a0->y1.n;
      OBJ.variant_0x24.timers.ray = 5;

      sprite = CreatePositionedObj(obj, OBJF_NOOP);
      sprite->y1.n += CV(0.5625);
      sprite->d.sprite.gfxIdx = GFX_LARGE_RED_CIRCLE;
      sprite->d.sprite.boxIdx = 7;
      sprite->d.sprite.clut = CLUT_PURPLES;
      sprite->d.sprite.semiTrans = 4;
      OBJ.sprite = sprite;

      obj->state++;

   // fallthrough
   case 1:
      obj->state2++;
      UpdateObjAnimation(obj);
      obj->mem += 4;
      if (--OBJ.variant_0x24.timers.ray <= 0) {
         for (i = 0; i < 3; i++) {
            tmp = rand() % 2; //
            obj_a0 = CreatePositionedObj(obj, OBJF_OUTWARD_RAY);
            obj_a0->x2.n = obj->x1.n;
            obj_a0->z2.n = obj->z1.n;
            obj_a0->y2.n = obj->y1.n + CV(0.5);
            if (obj->functionIndex == OBJF_MAP43_SCN93_MAGIC_STONE_FX) {
               obj_a0->state3 = 1;
            }
         }
         OBJ.variant_0x24.timers.ray = rand() % 3;
      }
      if (--OBJ.variant_0x24.timers.explosion <= 0) {
         if (obj->functionIndex == OBJF_MAP43_SCN93_MAGIC_STONE_FX) {
            CreatePositionedObj(obj, OBJF_MAGIC_STONE_EXPLOSION);
         }
         OBJ.variant_0x24.timers.explosion = rand() % 16 + 8;
      }
      sprite = OBJ.sprite;
      i = rsin(obj->state2 * 32); //
      iVar7 = abs(24 * i >> 12);
      sprite->d.sprite.gfxIdx = GFX_GLOBE_1 + iVar7 / 3;
      if (sprite->d.sprite.gfxIdx >= GFX_GLOBE_8) {
         sprite->d.sprite.gfxIdx = GFX_GLOBE_8;
      }
      if (obj->functionIndex == OBJF_MAP43_SCN93_MAGIC_STONE_FX) {
         tmp = 16;
      } else {
         tmp = 0;
      }
      for (i = 0; i < iVar7; i += 2) {
         gQuad_800fe63c[0].vx = -i - tmp;
         gQuad_800fe63c[0].vy = -i - tmp;
         gQuad_800fe63c[0].vz = 0;
         gQuad_800fe63c[1].vx = i + tmp;
         gQuad_800fe63c[1].vy = -i - tmp;
         gQuad_800fe63c[1].vz = 0;
         gQuad_800fe63c[2].vx = -i - tmp;
         gQuad_800fe63c[2].vy = i + tmp;
         gQuad_800fe63c[2].vz = 0;
         gQuad_800fe63c[3].vx = i + tmp;
         gQuad_800fe63c[3].vy = i + tmp;
         gQuad_800fe63c[3].vz = 0;
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         poly->r0 = (iVar7 - i) * 4;
         poly->g0 = (iVar7 - i) * 4;
         poly->b0 = (iVar7 - i) * 4;
      }
      break;

   case 2:
      obj_a0 = OBJ.unused_0x5C; //?
      obj_a0->functionIndex = OBJF_NULL;
      obj_a0 = OBJ.sprite;
      obj_a0->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

static s16 sCluts_80101900[] = {CLUT_REDS, CLUT_BLUES, CLUT_PURPLES, CLUT_GREENS};

#undef OBJF
#define OBJF 690
void Objf690_MagicStoneExplosion(Object *obj) {
   static s16 animData[35] = {GFX_EXPLOSION_1,  2, 4, GFX_EXPLOSION_2,  2, 4, GFX_EXPLOSION_3, 2, 4,
                              GFX_EXPLOSION_4,  2, 3, GFX_EXPLOSION_5,  2, 3, GFX_EXPLOSION_6, 2, 3,
                              GFX_EXPLOSION_7,  2, 3, GFX_EXPLOSION_8,  2, 3, GFX_EXPLOSION_9, 2, 3,
                              GFX_EXPLOSION_10, 2, 3, GFX_EXPLOSION_11, 2, 3, GFX_NULL,        0};

   s32 r;
   s32 theta;
   s32 dx, dy, dz;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = animData;
      obj->d.sprite.clut = CLUT_REDS;
      r = rand() % CV(0.5) + CV(1.0);
      theta = rand() % DEG(360);
      dx = r * rcos(theta) >> 12;
      dz = r * rsin(theta) >> 12;
      dy = r * rsin(rand() % 512) >> 12;
      obj->x2.n = obj->x1.n + dx;
      obj->z2.n = obj->z1.n + dz;
      obj->y2.n = obj->y1.n + dy;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += (obj->x2.n - obj->x1.n) >> 3;
      obj->z1.n += (obj->z2.n - obj->z1.n) >> 3;
      obj->y1.n += (obj->y2.n - obj->y1.n) >> 3;
      UpdateMultisizeObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 691
void Objf691_Map43_Scn93_CameraShake(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->y1.n = gCameraPos.vy;
      obj->state++;

   // fallthrough
   case 1:
      gCameraPos.vy = obj->y1.n + rand() % 8;
      break;
   }
}

void RenderSphere(Object *sphere) {
   s32 iVar5;
   s32 iVar4;
   s32 i;
   Object *dataStore;
   Cylinder *dsCylinder;
   s32 theta1;
   s32 theta2;

   iVar5 = sphere->mem;
   iVar4 = sphere->state3;

   dataStore = Obj_GetUnused();
   dataStore->functionIndex = OBJF_NOOP;
   dsCylinder = &dataStore->d.dataStore.cylinder;
   dsCylinder->bottom.vx = sphere->x1.n;
   dsCylinder->bottom.vz = sphere->z1.n;
   dsCylinder->bottom.vy = sphere->y1.n;
   dsCylinder->top.vx = sphere->x1.n;
   dsCylinder->top.vz = sphere->z1.n;
   dsCylinder->top.vy = sphere->y1.n;
   dsCylinder->sideCt = 16;
   dsCylinder->semiTrans = sphere->d.sprite.semiTrans;
   dsCylinder->gfxIdx = sphere->d.sprite.gfxIdx;
   dsCylinder->clut = sphere->d.sprite.clut;
   dsCylinder->theta = sphere->state2;

   for (i = 0; i < 4; i++) {
      theta1 = i * DEG(22.5);
      theta2 = (i + 1) * DEG(22.5);
      dsCylinder->bottomRadius = iVar5 * rcos(theta1) >> 12;
      dsCylinder->topRadius = iVar5 * rcos(theta2) >> 12;
      dsCylinder->bottom.vy = sphere->y1.n + (iVar4 * rsin(theta1) >> 12);
      dsCylinder->top.vy = sphere->y1.n + (iVar4 * rsin(theta2) >> 12);
      RenderCylinder(dsCylinder);
   }

   for (i = 0; i < 4; i++) {
      theta1 = i * DEG(-22.5);
      theta2 = (i + 1) * DEG(-22.5);
      dsCylinder->bottomRadius = iVar5 * rcos(theta1) >> 12;
      dsCylinder->topRadius = iVar5 * rcos(theta2) >> 12;
      dsCylinder->bottom.vy = sphere->y1.n + (iVar4 * rsin(theta1) >> 12);
      dsCylinder->top.vy = sphere->y1.n + (iVar4 * rsin(theta2) >> 12);
      RenderCylinder(dsCylinder);
   }

   dataStore->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 318
/* Meant to be a sparkle SPHERE over the target (RenderSphere), but state3 -- the
 * vertical radius -- is never set, so it degenerates to flat expanding ground rings.
 * gSignal3 on completion (event-FX shape); no EVDATA, spell, or code dispatches 318. */
void Objf318_ExpandingSparkleRings_Unused(Object *obj) {
   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(1.0);
      obj->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES;
      obj->d.sprite.semiTrans = 4;
      obj->d.sprite.clut = CLUT_BLUES;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2 += DEG(2.8125);
      if (++obj->mem >= CV(2.0)) {
         obj->state++;
      }
      RenderSphere(obj);
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 742
void Objf742_Map67_Scn34_RiftArcs_Separate(Object *obj) {
   s32 i;
   Object *p;

   p = &gObjectArray[0];

   for (i = 0; i < ARRAY_COUNT(gObjectArray); i++) {
      if (p->functionIndex == OBJF_MAP67_SCN34_RIFT_ARCS) {
         if (p->state == 1 && p->state2 == 2) {
            p->state2++;
         }
         break;
      }
      p++;
   }

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 319
void Objf319_Map67_Scn34_RiftArcs(Object *obj) {
   Object *obj_s2;
   Object *obj_s0;
   Object *obj_s3;

   switch (obj->state) {
   case 0:
      obj_s2 = OBJ.entitySprite;
      obj->x1.n = obj_s2->x1.n;
      obj->z1.n = obj_s2->x1.n;
      obj->y1.n = obj_s2->y1.n + CV(1.0);

      obj_s2 = CreatePositionedObj(obj, OBJF_MAP67_SCN34_BOLT_ENDPOINT);
      obj_s2->x3.n = obj->x1.n - CV(1.5);
      OBJ.todo_x5c = obj_s2;

      obj_s0 = CreatePositionedObj(obj, OBJF_NOOP);
      obj_s0->x1.n -= CV(3.0);
      OBJ.todo_x58 = obj_s0;
      obj_s0->d.sprite.clut = CLUT_PURPLES;
      obj_s0->d.sprite.semiTrans = 2;
      OBJ.semiTrans = 2;

      obj_s2 = CreatePositionedObj(obj_s0, OBJF_MAP67_SCN34_BOLT_ENDPOINT);
      obj_s2->x3.n = obj_s0->x1.n + CV(1.5);
      OBJ.todo_x50 = obj_s2;
      obj->state++;

   // fallthrough
   case 1:
      obj_s2 = OBJ.todo_x5c;
      obj->x2.n = obj_s2->x1.n;
      obj->z2.n = obj_s2->z1.n;
      obj->y2.n = obj_s2->y1.n;
      RenderLightningBolt(obj);
      RenderLightningBolt(obj);
      RenderLightningBolt(obj);
      obj_s0 = OBJ.todo_x58;
      obj_s3 = OBJ.todo_x50;
      obj_s0->x2.n = obj_s3->x1.n;
      obj_s0->z2.n = obj_s3->z1.n;
      obj_s0->y2.n = obj_s3->y1.n;
      RenderLightningBolt(obj_s0);
      RenderLightningBolt(obj_s0);

      switch (obj->state2) {
      case 0:
         obj->state2++;

      // fallthrough
      case 1:
         obj->state2++;

      // fallthrough
      case 2:
         if (--obj->state3 <= 0) {
            obj_s2 = CreatePositionedObj(obj, OBJF_FLICKERING_EXPAND_RING);
            obj_s2->x1.n -= CV(1.5);
            obj_s2->mem = 16;
            obj->state3 = rand() % 3 + 8;
         }
         break;

      case 3:
         obj_s2 = OBJ.todo_x5c;
         obj_s3 = OBJ.todo_x50;
         obj_s2->x2.n = obj_s3->x2.n = obj->x1.n - CV(1.5) - obj->mem;
         if (--obj->state3 <= 0) {
            obj_s0 = CreatePositionedObj(obj_s3, OBJF_FLICKERING_EXPAND_RING);
            obj_s0->mem = 16;
            obj->state3 = rand() % 3 + 8;
         }
         obj->mem += 2;
         if (obj->mem >= 384) {
            obj_s2->functionIndex = OBJF_NULL;
            obj_s3->functionIndex = OBJF_NULL;
            obj->state++;
            obj->mem = 384;
         }
         break;

      case 4:
         obj->state++;
         break;
      }

      break;

   case 2:
      FadeOutScreen(0, 0xff);
      obj->state++;
      break;

   case 3:
      obj->state++;
      break;

   case 4:
      FadeInScreen(0, 8);
      obj->state++;
      break;

   case 5:
      obj_s0 = OBJ.todo_x58;
      obj_s0->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 320
void Objf320_Map67_Scn34_BoltEndpoint(Object *obj) {
   s32 i;
   Object *obj_s4;
   s32 r;
   s32 a, b;

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += (obj->x3.n - obj->x1.n) >> 2;
      if (++obj->mem >= 32) {
         obj->state++;
      }
      break;

   case 2:
      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 3:
      rsin(obj->mem << 4);
      obj->x1.n = obj->x2.n;
      obj->z1.n = obj->z2.n;
      obj->y1.n = obj->y2.n;
      obj->mem++;

      for (i = 0; i < 5; i++) {
         obj_s4 = CreatePositionedObj(obj, OBJF_SPARK_PARTICLE);
         a = rand() % DEG(360);
         b = rand() % DEG(360);
         r = rand() % CV(0.125) + CV(0.0625);
         obj_s4->x2.n = r * rcos(a) >> 12;
         obj_s4->z2.n = r * rsin(a) >> 12;
         obj_s4->y2.n = r * rsin(b) >> 12;
         obj_s4->mem = 12;
      }
      break;
   }
}

#undef OBJF
#define OBJF 728
void Objf728_FlickeringExpandRing(Object *obj) {
   s16 halfSize;

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_RING;
      obj->d.sprite.clut = CLUT_BLUES;
      obj->state++;

   // fallthrough
   case 1:
      halfSize = obj->state2;
      obj->state2 += 8;
      obj->d.sprite.coords[0].x = obj->d.sprite.coords[1].x = obj->d.sprite.coords[2].x =
          obj->d.sprite.coords[3].x = obj->x1.n;
      obj->d.sprite.coords[0].z = obj->d.sprite.coords[2].z = obj->z1.n - halfSize;
      obj->d.sprite.coords[1].z = obj->d.sprite.coords[3].z = obj->z1.n + halfSize;
      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->y1.n - halfSize;
      obj->d.sprite.coords[2].y = obj->d.sprite.coords[3].y = obj->y1.n + halfSize;
      obj->mem++;
      obj->d.sprite.hidden = obj->mem % 2;
      AddObjPrim4(gGraphicsPtr->ot, obj);
      if (obj->state2 >= 256) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 323
void Objf323_713_SummonRuneColumn(Object *obj) {
   // 323: Spawned by Objf333 (unused?), EVDATA29.DAT (Map20/Magnus)
   // 713: Spawned by EVDATA55.DAT (Map30/Xeno)

   Object *obj_s2;
   Cylinder *dsCylinder;
   s32 r;
   s16 r2;
   s16 theta;
   u8 unused[80];

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_SUMMON_CREST;
         obj_s2->d.objf204.unitSprite = OBJ.entitySprite;
         obj_s2->d.objf204.maskClut = CLUT_MASK;
         obj_s2->d.objf204.clut = CLUT_REDS;
         obj_s2->d.objf204.vramSrcX = 384 << 2;
         obj_s2->d.objf204.vramSrcY = 384;

         obj->state3 = 32;
         obj->state2++;

      // fallthrough
      case 1:
         if (--obj->state3 <= 0) {
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 1:
      obj_s2 = OBJ.entitySprite;
      obj->x1.n = obj_s2->x1.n;
      obj->z1.n = obj_s2->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);

      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_s2;
      dsCylinder = &obj_s2->d.dataStore.cylinder;
      dsCylinder->bottom.vx = dsCylinder->top.vx = obj->x1.n;
      dsCylinder->bottom.vz = dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + CV(5.0);
      dsCylinder->sideCt = 32;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_COLORS;
      dsCylinder->topRadius = CV(0.0);
      dsCylinder->bottomRadius = CV(0.0);
      if (obj->functionIndex == OBJF_SUMMON_RUNE_COLUMN_323) {
         dsCylinder->clut = CLUT_PURPLES;
      } else if (obj->functionIndex == OBJF_SUMMON_RUNE_COLUMN_713) {
         dsCylinder->clut = CLUT_GREENS;
      }
      dsCylinder->useColor = 1;
      dsCylinder->color.r = 0;
      dsCylinder->color.g = 0;
      dsCylinder->color.b = 0;
      obj->state++;

   // fallthrough
   case 2:
      obj_s2 = OBJ.dataStore;
      dsCylinder = &obj_s2->d.dataStore.cylinder;

      dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b =
          abs(rsin(obj->mem * 8) * 224 >> 12);
      r = rsin(obj->mem * 8) * CV(1.5) >> 12;
      if (r > CV(1.0)) {
         r = CV(1.0);
      }
      dsCylinder->bottomRadius = dsCylinder->topRadius = r;
      dsCylinder->top.vy = dsCylinder->bottom.vy + CV(4.0) + obj->mem * 2;
      RenderCylinderWithRotatedTexture(dsCylinder);

      obj->mem += 2;
      if (obj->mem >= 256) {
         obj->state++;
      }
      if (--obj->state2 <= 0) {
         if (obj->mem <= 192) {
            r2 = CV(0.5);
            obj_s2 = Obj_GetUnused();
            obj_s2->functionIndex = OBJF_RISING_GLYPH;
            theta = rand() % DEG(360);
            obj_s2->x1.n = obj->x1.n + (r2 * rsin(theta) >> 12);
            obj_s2->z1.n = obj->z1.n + (r2 * rcos(theta) >> 12);
            obj_s2->y1.n = obj->y1.n + CV(0.5) + (rand() % CV(0.5));
            obj_s2->d.sprite.gfxIdx = GFX_RUNE_1 + (rand() % 12);
            obj_s2->y2.n = rand() % CV(0.125) + CV(0.125);
            obj_s2->y3.n = CV(0.03125);
            obj->state2 = rand() % 8;
         }
      }
      break;

   case 3:
      obj_s2 = OBJ.dataStore;
      obj_s2->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 679
void Objf679_EntityFlasher(Object *obj) {
   // Spawned by: EVDATA00.DAT (flashes the magic stone),
   //             EVDATA29.DAT (flashes Magnus)
   Object *entitySprite;
   Object *sprite;
   POLY_FT4 *poly;
   s16 prevQuadIdx;
   u8 intensity;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.entitySprite;
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      CopyObject(entitySprite, sprite);
      sprite->functionIndex = OBJF_NOOP;
      entitySprite->d.sprite.hidden = 1;
      sprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 1);
      OBJ.sprite = sprite;
      obj->state++;
      break;

   case 1:
      entitySprite = OBJ.entitySprite;
      sprite = OBJ.sprite;

      switch (obj->state2) {
      case 0:
         obj->mem = abs(223 * rsin(obj->state3 * 64) >> 12) + 32;
         if (!entitySprite->d.sprite.hidden) {
            entitySprite->d.sprite.hidden = 1;
            obj->state2++;
         }
         break;

      case 1:
         obj->mem += (128 - obj->mem) >> 3;
         if (++OBJ.timer >= 16) {
            obj->mem = 128;
            obj->state++;
            entitySprite->d.sprite.hidden = 0;
         }
         break;
      }

      CopyObject(entitySprite, sprite);
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.hidden = 0;

      prevQuadIdx = gQuadIndex;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 1);
      if (gQuadIndex != prevQuadIdx) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         intensity = obj->mem;
         setRGB0(poly, intensity, intensity, intensity);
      }

      obj->state3++;
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 680
void Objf680_LitDummySprite(Object *obj) {
   Object *entitySprite;
   Object *sprite;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.entitySprite;
      sprite = Obj_GetUnused();
      CopyObject(entitySprite, sprite);
      sprite->functionIndex = OBJF_NOOP;
      entitySprite->d.sprite.hidden = 1;
      sprite->d.sprite.hidden = 0;
      obj->state++;
      RenderUnitSprite(gGraphicsPtr->ot, sprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, gLightColor.r, gLightColor.g, gLightColor.b);
      OBJ.sprite = sprite;
      break;

   case 1:
      entitySprite = OBJ.entitySprite;
      sprite = OBJ.sprite;
      RenderUnitSprite(gGraphicsPtr->ot, sprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, gLightColor.r, gLightColor.g, gLightColor.b);
      if (!entitySprite->d.sprite.hidden) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 682
void Objf682_RaiseFaces_Unused(Object *obj) {
   // Unused map effect?

   AdjustFaceElevation(&gMapRowPointers[5][20], 1, 64);
   AdjustFaceElevation(&gMapRowPointers[6][20], 1, 64);
   AdjustFaceElevation(&gMapRowPointers[7][20], 1, 64);
   obj->functionIndex = OBJF_NULL;
}

/* UNREFERENCED (no caller, not in gObjFunctionPointers[]): the animated twin of
 * Objf682_RaiseFaces_Unused -- same three hard-coded tiles, +1/frame for 64 frames. */
void func_800ADF0C(Object *obj) {
   // Unused map effect?

   switch (obj->state) {
   case 0:
      obj->mem = 64;
      obj->state++;

   // fallthrough
   case 1:
      AdjustFaceElevation(&gMapRowPointers[5][20], 1, 1);
      AdjustFaceElevation(&gMapRowPointers[6][20], 1, 1);
      AdjustFaceElevation(&gMapRowPointers[7][20], 1, 1);
      if (--obj->mem <= 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 689
void Objf689_EntityFlashBurstRays(Object *obj) {
   // Spawned by: EVDATA29.DAT, EVDATA86.DAT; Entity burst?

   u8 local_30[5] = {4, 4, 8, 4, 2};
   s32 i, j;
   s32 x, z, y;
   SVectorXZY *pCoord;
   Object *obj_a0;
   Object *entitySprite;
   Object *sprite;
   s32 bVar1;
   s32 r, theta1, theta2;
   POLY_FT4 *poly;
   // s32 intensity;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n + CV(0.5);

      sprite = Obj_GetUnused();
      CopyObject(entitySprite, sprite);
      entitySprite->d.sprite.hidden = 1;
      sprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
      OBJ.sprite = sprite;

      pCoord = &OBJ.coords[0];
      for (i = 0; i < 3; i++) {
         r = rand() % CV(1.0) + CV(4.0);
         theta1 = rand() % DEG(180);
         theta2 = rand() % DEG(360);
         x = r * rcos(theta2) >> 12;
         z = r * rsin(theta2) >> 12;
         y = r * rsin(theta1) >> 12;
         pCoord->x = x;
         pCoord->z = z;
         pCoord->y = y;
         pCoord++;
      }

      obj->state++;
      break;

   case 1:
      RenderUnitSprite(gGraphicsPtr->ot, OBJ.sprite, 0);

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_NOOP;
      obj_a0->d.sprite.gfxIdx = GFX_COLOR_15;
      obj_a0->d.sprite.clut = CLUT_REDS;
      obj_a0->d.sprite.semiTrans = 2;

      bVar1 = local_30[obj->mem];
      pCoord = &OBJ.coords[0];
      for (j = 0; j < 3; j++) {
         x = pCoord->x;
         z = pCoord->z;
         y = pCoord->y;
         for (i = 1; i < 4; i++) {
            obj_a0->d.sprite.coords[0].x = obj_a0->d.sprite.coords[2].x = obj->x1.n;
            obj_a0->d.sprite.coords[0].z = obj_a0->d.sprite.coords[2].z = obj->z1.n;
            obj_a0->d.sprite.coords[0].y = obj->y1.n;
            obj_a0->d.sprite.coords[2].y = obj->y1.n;
            obj_a0->d.sprite.coords[1].x = obj_a0->d.sprite.coords[3].x = obj->x1.n + x;
            obj_a0->d.sprite.coords[1].z = obj_a0->d.sprite.coords[3].z = obj->z1.n + z;
            obj_a0->d.sprite.coords[1].y = obj->y1.n + y + (i * bVar1);
            obj_a0->d.sprite.coords[3].y = obj->y1.n + y - (i * bVar1);
            obj_a0->x1.n = obj->x1.n;
            obj_a0->z1.n = obj->z1.n;
            obj_a0->y1.n = obj->y1.n - CV(0.5);
            //? Never drawn?
         }
         pCoord++;
      }
      obj_a0->functionIndex = OBJF_NULL;

      obj->mem = 0;
      obj->state3 = 0;
      obj->state++;
      break;

   case 2:
      entitySprite = OBJ.entitySprite;
      sprite = OBJ.sprite;
      sprite->d.sprite.semiTrans = 2;
      sprite->d.sprite.clut = CLUT_MASK;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      i = 128 - obj->mem;
      setRGB0(poly, i, i, i);
      sprite->d.sprite.semiTrans = 0;

      entitySprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, entitySprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      i = obj->mem;
      setRGB0(poly, i, i, i);
      entitySprite->d.sprite.hidden = 1;
      obj->mem += (128 - obj->mem) >> 2;

      if (obj->state3 == 0) {
         for (i = 0; i < 32; i++) {
            entitySprite = CreatePositionedObj(obj, OBJF_OUTWARD_RAY);
            entitySprite->x2.n = obj->x1.n;
            entitySprite->z2.n = obj->z1.n;
            entitySprite->y2.n = obj->y1.n;
         }
      }
      if (++obj->state3 >= 32) {
         entitySprite = OBJ.entitySprite;
         entitySprite->d.sprite.hidden = 0;
         obj->state++;
      }
      break;

   case 3:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 693
void Objf693_EntityRedStripePulse_Unused(Object *obj) {
   Object *entitySprite;
   Object *sprite;
   MaskEffectPreset maskEffect;
   u8 intensity;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      entitySprite->d.sprite.hidden = 1;
      obj->state++;
      obj->mem = 256;

   // fallthrough
   case 1:
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      maskEffect.srcGfxIdx = GFX_TILED_LINES;
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.semiTrans = 2;
      maskEffect.clut = CLUT_REDS;
      intensity = 255 * rsin(obj->mem * 16) >> 12;
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b = intensity;
      RenderMaskEffect(entitySprite, &maskEffect);

      sprite = Obj_GetUnused();
      CopyObject(entitySprite, sprite);
      sprite->d.sprite.hidden = 0;
      sprite->d.sprite.clut = CLUT_MASK;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 128 - intensity, 128 - intensity, 128 - intensity);
      sprite->functionIndex = OBJF_NULL;

      if (--obj->mem <= 0) {
         entitySprite->d.sprite.hidden = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 694
void Objf694_Map61_Scn83_AshGlow(Object *obj) {
   Object *entitySprite;
   Object *sprite;
   POLY_FT4 *poly;
   s32 intensity;

   switch (obj->state) {
   case 0:
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      sprite = Obj_GetUnused();
      CopyObject(entitySprite, sprite);
      sprite->functionIndex = OBJF_NOOP;
      entitySprite->d.sprite.hidden = 1;
      sprite->d.sprite.hidden = 0;
      obj->mem = 0;
      obj->state3 = 0;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
      obj->state++;
      break;

   case 1:
      if (obj->state3 >= 1280) {
         obj->state3 = 1280;
         obj->state++;
      }
      obj->mem = obj->state3 / 10;

      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      sprite = Obj_GetUnused();
      CopyObject(entitySprite, sprite);
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.hidden = 0;
      sprite->d.sprite.semiTrans = 2;
      sprite->d.sprite.clut = CLUT_MASK;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      intensity = 128 - obj->mem;
      setRGB0(poly, intensity, intensity, intensity);

      entitySprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, entitySprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      intensity = obj->mem;
      setRGB0(poly, intensity, intensity, intensity);
      entitySprite->d.sprite.hidden = 1;

      sprite->functionIndex = OBJF_NULL;
      obj->state3 += 12;
      break;

   case 2:
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      entitySprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, entitySprite, 0);
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 695
void Objf695_696_EntityBlendFade_Unused(Object *obj) {
   // Unused entity spawn?

   s32 i;
   Object *entitySprite;
   Object *sprite_s2;
   Object *sprite_s4;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.variant_0x24.entitySpriteParam;
      OBJ.entitySprite = entitySprite;
      OBJ.variant_0x24.unk = 0;
      entitySprite->d.sprite.hidden = 1;
      if (obj->functionIndex == OBJF_ENTITY_BLEND_FADE_UNUSED) {
         OBJ.intensity = 128;
         obj->mem = -4;
      } else {
         OBJ.intensity = 0;
         obj->mem = 4;
      }
      obj->state3 = 128;
      obj->state++;

   // fallthrough
   case 1:
      entitySprite = OBJ.entitySprite;
      sprite_s2 = Obj_GetUnused();
      CopyObject(entitySprite, sprite_s2);
      sprite_s2->d.sprite.hidden = 0;
      sprite_s2->x1.n = entitySprite->x1.n;
      sprite_s2->z1.n = entitySprite->z1.n;
      sprite_s2->y1.n = entitySprite->y1.n;
      sprite_s2->d.sprite.semiTrans = 4;

      sprite_s4 = Obj_GetUnused();
      CopyObject(sprite_s2, sprite_s4);
      sprite_s4->d.sprite.semiTrans = 2;

      for (i = 0; i < 4; i++) {
         RenderUnitSprite(gGraphicsPtr->ot, sprite_s2, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, OBJ.intensity, OBJ.intensity, OBJ.intensity);
      }

      OBJ.intensity += obj->mem;
      obj->state3 -= abs(obj->mem);
      if (obj->state3 <= 0) {
         if (obj->functionIndex != OBJF_ENTITY_BLEND_FADE_UNUSED) {
            entitySprite->d.sprite.hidden = 0;
         }
         obj->functionIndex = OBJF_NULL;
      }
      sprite_s2->functionIndex = OBJF_NULL;
      sprite_s4->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 697
void Objf697_Map43_Scn93_FlameSphere(Object *obj) {
   Object *sphere;
   Object *obj_v1;

   switch (obj->state) {
   case 0:
      obj_v1 = OBJ.entitySprite; // Dolf
      obj->x1.n = obj_v1->x1.n;
      obj->z1.n = obj_v1->z1.n;
      obj->y1.n = obj_v1->y1.n;
      sphere = Obj_GetUnused();
      sphere->functionIndex = OBJF_NOOP;
      OBJ.sphere = sphere;
      obj->state++;

   // fallthrough
   case 1:
      sphere = OBJ.sphere;
      sphere->x1.n = obj->x1.n;
      sphere->z1.n = obj->z1.n;
      sphere->y1.n = obj->y1.n + CV(0.5);
      sphere->mem = CV(0.25);
      sphere->state3 = CV(0.25);
      sphere->d.sprite.gfxIdx = GFX_TILED_DIAMONDS;
      sphere->d.sprite.clut = CLUT_REDS;
      sphere->d.sprite.semiTrans = 1;
      obj->state++;

   // fallthrough
   case 2:
      sphere = OBJ.sphere;
      sphere->state2 += DEG(2.8125);
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_INWARD_RAY;
      obj_v1->x2.n = obj->x1.n;
      obj_v1->z2.n = obj->z1.n;
      obj_v1->y2.n = obj->y1.n + CV(0.5);
      obj_v1->d.objf314.todo_x38 = 256;
      RenderSphere(sphere);
      break;
   }
}

#undef OBJF
#define OBJF 698
void Objf698_Map61_Scn83_EleniSparkleRings(Object *obj) {
   static s16 sparkleAnimData[] = {5, GFX_SPARKLE_1, 2, GFX_SPARKLE_2, 2, GFX_SPARKLE_3,
                                   2, GFX_SPARKLE_4, 2, GFX_SPARKLE_5, 2, GFX_NULL,
                                   1, GFX_NULL};

   Object *entitySprite;
   Object *sparkleSprite;
   // Object *particle;
   s32 iVar6, iVar3;
   s32 theta;
   s32 i;
   SVECTOR svector;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n;

      // Target for homing particles:
      obj->x2.n = obj->x1.n + CV(3.0);
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;

      sparkleSprite = Obj_GetUnused();
      sparkleSprite->functionIndex = OBJF_NOOP;
      sparkleSprite->d.sprite.animData = sparkleAnimData;
      OBJ.sparkleSprite = sparkleSprite;

      obj->mem = DEG(0);
      if (OBJ.firstLaunchTimer == 0) {
         OBJ.firstLaunchTimer = 256;
      }
      obj->state++;

   // fallthrough
   case 1:
      sparkleSprite = OBJ.sparkleSprite;
      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n;

      iVar6 = (CV(0.1875) * rsin(obj->mem) >> 12) + CV(0.75);
      UpdateObjAnimation(sparkleSprite);

      theta = obj->mem;
      iVar3 = DEG(45) * rsin(theta) >> 12;

      switch (obj->state3) {
      case 0:
         if (--OBJ.firstLaunchTimer <= 0) {
            obj->state3++;
         }
         break;

      case 1:
         if (--OBJ.launchTimer <= 0) {
            OBJ.launchNum++;
            if (OBJ.launchNum >= 16) {
               obj->state3++;
            }
            OBJ.launchTimer = 4;
         }
         break;

      case 2:
         obj->state++;
         break;
      }

      for (i = 0; i < 16; i++) {
         SphericalToVector(&svector, iVar6, iVar3 * rsin(theta) >> 12, theta);
         sparkleSprite->d.sprite.clut = CLUT_REDS;
         sparkleSprite->x1.n = obj->x1.n + svector.vx;
         sparkleSprite->z1.n = obj->z1.n + svector.vz;
         sparkleSprite->y1.n = obj->y1.n + svector.vy + CV(0.5);
         if (i < OBJ.launchNum) {
            sparkleSprite->d.sprite.hidden = 1;
            if (i == OBJ.launchNum - 1 && OBJ.launchTimer == 4) {
               Object *particle = CreatePositionedObj(sparkleSprite, OBJF_HOMING_PARTICLE);
               particle->x2.n = obj->x2.n; // target
               particle->z2.n = obj->z2.n;
               particle->y2.n = obj->y2.n;
               particle->state2 = 2; // inverse speed
               particle->mem = 32;   // lifetime
               particle->d.sprite.animData = sSparkleAnimData_80101838;
               particle->d.sprite.clut = CLUT_REDS;
            }
         } else {
            sparkleSprite->d.sprite.hidden = 0;
         }
         AddObjPrim6(gGraphicsPtr->ot, sparkleSprite, 0);

         SphericalToVector(&svector, iVar6, -(iVar3 * rsin(theta) >> 12), theta);
         sparkleSprite->d.sprite.clut = CLUT_BLUES;
         sparkleSprite->x1.n = obj->x1.n + svector.vx;
         sparkleSprite->z1.n = obj->z1.n + svector.vz;
         sparkleSprite->y1.n = obj->y1.n + svector.vy + CV(0.5);
         if (i < OBJ.launchNum) {
            sparkleSprite->d.sprite.hidden = 1;
            if (i == OBJ.launchNum - 1 && OBJ.launchTimer == 4) {
               Object *particle = CreatePositionedObj(sparkleSprite, OBJF_HOMING_PARTICLE);
               particle->x2.n = obj->x2.n; // target
               particle->z2.n = obj->z2.n;
               particle->y2.n = obj->y2.n;
               particle->state2 = 2; // inverse speed
               particle->mem = 32;   // lifetime
               particle->d.sprite.animData = sSparkleAnimData_80101838;
               particle->d.sprite.clut = CLUT_BLUES;
            }
         } else {
            sparkleSprite->d.sprite.hidden = 0;
         }
         AddObjPrim6(gGraphicsPtr->ot, sparkleSprite, 0);

         theta += DEG(22.5);
      }

      obj->mem += DEG(4.21875);
      break;

   case 2:
      sparkleSprite = OBJ.sparkleSprite;
      sparkleSprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 699
void Objf699_Map61_Scn83_EleniSpell(Object *obj) {
   MaskEffectPreset maskEffect;
   Object *entitySprite;
   Object *sparkleRings;

   switch (obj->state) {
   case 0:
      sparkleRings = Obj_GetUnused();
      sparkleRings->functionIndex = OBJF_MAP61_SCN83_ELENI_SPARKLE_RINGS;
      sparkleRings->d.objf698.entitySprite = obj->d.entitySpawn.entitySpriteParam;
      sparkleRings->d.objf698.firstLaunchTimer = 512;
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      entitySprite->d.sprite.hidden = 1;
      obj->state++;

   // fallthrough
   case 1:
      entitySprite = obj->d.entitySpawn.entitySpriteParam;
      maskEffect.srcGfxIdx = GFX_TILED_FLAMES;
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.width = 0;
      maskEffect.height = obj->state3;
      maskEffect.semiTrans = 2;
      maskEffect.clut = CLUT_BLUES;
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b =
          abs(255 * rsin(obj->state3 * 16) >> 12);
      RenderMaskEffect(entitySprite, &maskEffect);

      entitySprite->d.sprite.hidden = 0;
      RenderUnitSprite(gGraphicsPtr->ot, entitySprite, 0);
      entitySprite->d.sprite.hidden = 1;

      if (++obj->state3 >= 512) {
         entitySprite->d.sprite.hidden = 0;
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 2:
      break;
   }
}

#undef OBJF
#define OBJF 700
void Objf700_DynamoHum_ColoredBolt(Object *obj) {
   static s16 animData[] = {7, GFX_LIGHTNING_1, 1, GFX_LIGHTNING_2, 1, GFX_LIGHTNING_3,
                            1, GFX_LIGHTNING_4, 1, GFX_LIGHTNING_5, 1, GFX_LIGHTNING_6,
                            1, GFX_LIGHTNING_7, 1, GFX_LIGHTNING_8, 1, GFX_NULL,
                            0, GFX_NULL};
   static Quad quad = {{-16, -128, 0, 0}, {16, -128, 0, 0}, {-16, 0, 0, 0}, {16, 0, 0, 0}};

   s32 i;
   u8 intensity;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = animData;
      *(u32 *)&obj->d.sprite.hidden = 0; //?
      obj->d.sprite.clut = sCluts_80101900[rand() % 4];
      obj->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i] = quad[i];
      }
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 192, 192, 192);
      if (obj->d.sprite.animFinished) {
         obj->state2 = 192;
         obj->state++;
      }
      break;

   case 2:
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i] = quad[i];
      }
      intensity = obj->state2;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, intensity, intensity, intensity);
      obj->state2 -= 16;
      if (obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

static s16 sExplosionAnimData_801019b4[] = {
    3, GFX_EXPLOSION_1, 2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,  2, GFX_EXPLOSION_4,
    2, GFX_EXPLOSION_5, 2, GFX_EXPLOSION_6,  2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,
    2, GFX_EXPLOSION_9, 2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
    0, GFX_NULL};

static s16 sFlashingPatternsGfx_801019e8[] = {
    GFX_TILED_CLOUDS, GFX_TILED_VEINS,       GFX_TILED_LINES, GFX_TILED_TBD_177,
    GFX_TILED_FLAMES, GFX_TILED_ICE,         GFX_TILED_STONE, GFX_TILED_DIAMONDS,
    GFX_TILED_MAGMA,  GFX_TILED_RED_SPARKLES};

#undef OBJF
#define OBJF 705
void Objf705_732_743_744_Transformation(Object *obj) {
   // 705: Unused?
   // 732: Objf587_BattleEnemyEvent/Objf014_BattleUnit (Kane/Dolf mid-battle transformations)
   // 743: EVDATA34.DAT, EVDATA29.DAT (Magnus)
   // 744: EVDATA86.DAT (Ash)

   Object *obj_s0;
   Object *obj_s1;
   s32 radius, theta;
   MaskEffectPreset maskEffect;

   if (obj->functionIndex == OBJF_MID_BATTLE_TRANSFORMATION) {
      // 732 is not spawned by an event entity
      obj_s1 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      if (obj_s1 != NULL) {
         obj->d.entitySpawn.entitySpriteParam = obj_s1;
      }
   }

   switch (obj->state) {
   case 0:
      obj_s1 = obj->d.entitySpawn.entitySpriteParam;
      obj_s1->d.sprite.hidden = 1;
      obj->state++;
      if (obj->state3 == 0) {
         obj->state3 = 256;
      }
      break;

   case 1:
      if (obj->mem % 2 == 0) {
         obj_s0 = obj->d.entitySpawn.entitySpriteParam;
         GetUnitSpriteAtPosition(obj_s0->z1.s.hi, obj_s0->x1.s.hi); // unused return

         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_HOMING_PARTICLE;
         obj_s1->x1.n = obj_s0->x1.n;
         obj_s1->z1.n = obj_s0->z1.n;
         obj_s1->y1.n = obj_s0->y1.n + CV(0.5);
         obj_s1->d.sprite.semiTrans = 2;

         radius = rand() % CV(1.0) + CV(0.5);
         theta = rand() % DEG(360);
         obj_s1->x2.n = obj_s1->x1.n + (radius * rcos(theta) >> 12);
         obj_s1->z2.n = obj_s1->z1.n + (radius * rsin(theta) >> 12);
         obj_s1->y2.n = obj_s1->y1.n;
         obj_s1->d.sprite.animData = sExplosionAnimData_801019b4;
         obj_s1->d.sprite.clut = sCluts_801230a4[rand() % 2];
         obj_s1->mem = 32;
         obj_s1->state2 = 2;
      }
      obj->mem++;
      break;
   }

   obj_s1 = obj->d.entitySpawn.entitySpriteParam;
   if (obj_s1 != NULL) {
      maskEffect.srcGfxIdx = sFlashingPatternsGfx_801019e8[obj->mem % 6];
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.width = 0;
      maskEffect.height = obj->mem % 64;
      maskEffect.semiTrans = 0;
      maskEffect.clut = sCluts_80101900[obj->mem % 4];
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b =
          128 + abs(127 * rsin(obj->mem * 16) >> 12);
      RenderMaskEffect(obj_s1, &maskEffect);
   }

   if (obj->functionIndex == OBJF_MID_BATTLE_TRANSFORMATION) {
      if (gState.D_801405A4 != 0) {
         if (obj_s1 != NULL) {
            obj_s1->d.sprite.hidden = 0;
         }
         obj->functionIndex = OBJF_NULL;
      }
   } else if (--obj->state3 <= 0 || !obj_s1->d.sprite.hidden) {
      if (obj_s1 != NULL) {
         obj_s1->d.sprite.hidden = 0;
      }
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 745
void Objf745_MapUnitTransformation_Unused(Object *obj) {
   // Unused?

   Object *obj_s3;
   s32 radius, theta;
   MaskEffectPreset maskEffect;

   switch (obj->state) {
   case 0:
      obj_s3 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x1.n = obj_s3->x1.n;
      obj->z1.n = obj_s3->z1.n;
      obj->y1.n = obj_s3->y1.n + CV(0.5);
      obj_s3->d.sprite.hidden = 1;
      obj->state++;
      if (obj->state3 == 0) {
         obj->state3 = 256;
      }
      break;

   case 1:
      if (obj->mem % 2 == 0) {
         obj_s3 = Obj_GetUnused();
         obj_s3->functionIndex = OBJF_HOMING_PARTICLE;
         obj_s3->x1.n = obj->x1.n;
         obj_s3->z1.n = obj->z1.n;
         obj_s3->y1.n = obj->y1.n;
         obj_s3->d.sprite.semiTrans = 2;

         radius = rand() % CV(1.0) + CV(0.5);
         theta = rand() % DEG(360);
         obj_s3->x2.n = obj_s3->x1.n + (radius * rcos(theta) >> 12);
         obj_s3->z2.n = obj_s3->z1.n + (radius * rsin(theta) >> 12);
         obj_s3->y2.n = obj_s3->y1.n;
         obj_s3->d.sprite.animData = sExplosionAnimData_801019b4;
         obj_s3->d.sprite.clut = sCluts_801230a4[rand() % 2];
         obj_s3->mem = 32;
         obj_s3->state2 = 2;
      }
      obj->mem++;
      break;
   }

   if (OBJ_MAP_UNIT(obj).s.unitIdx != 0) {
      obj_s3 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      maskEffect.srcGfxIdx = sFlashingPatternsGfx_801019e8[obj->mem % 6];
      maskEffect.dstGfxIdx = GFX_MASK_EFFECT_1;
      maskEffect.width = 0;
      maskEffect.height = obj->mem % 64;
      maskEffect.semiTrans = 0;
      maskEffect.clut = sCluts_80101900[obj->mem % 4];
      maskEffect.color.r = maskEffect.color.g = maskEffect.color.b =
          128 + abs(127 * rsin(obj->mem * 16) >> 12);
      RenderMaskEffect(obj_s3, &maskEffect);
   }

   if (--obj->state3 <= 0 || !obj_s3->d.sprite.hidden) {
      obj_s3->d.sprite.hidden = 0;
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 707
void Objf707_RisingGlyph(Object *obj) {
   s16 halfHeight, halfWidth;

   switch (obj->state) {
   case 0:
      if (OBJ.gfxIdx == GFX_NULL) {
         OBJ.gfxIdx = GFX_EXPLOSION_1;
      }
      OBJ.boxIdx = 7;
      OBJ.clut = CLUT_REDS;
      OBJ.halfWidth = 8;
      OBJ.halfHeight = 6;
      obj->mem = 24;
      obj->state++;

   // fallthrough
   case 1:
      halfWidth = OBJ.halfWidth;
      halfHeight = OBJ.halfHeight;
      gQuad_800fe63c[0].vx = gQuad_800fe63c[2].vx = -halfWidth;
      gQuad_800fe63c[1].vx = gQuad_800fe63c[3].vx = halfWidth;
      gQuad_800fe63c[0].vy = gQuad_800fe63c[1].vy = -halfHeight;
      gQuad_800fe63c[2].vy = gQuad_800fe63c[3].vy = halfHeight;
      gQuad_800fe63c[0].vz = gQuad_800fe63c[1].vz = 0;
      gQuad_800fe63c[2].vz = gQuad_800fe63c[3].vz = 0;

      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      OBJ.halfWidth--;
      OBJ.halfHeight++;
      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;
      if (OBJ.halfWidth <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 714
void Objf714_DebugCamera(Object *obj) {
   Noop_DebugPrintValue(1, 3, gState.eventCameraPan.x);
   Noop_DebugPrintValue(1, 4, gState.eventCameraPan.y);
   Noop_DebugPrintValue(1, 5, gState.eventCameraPan.z);

   if (gPad2State & PAD_UP) {
      gState.eventCameraPan.x += CV(0.0625);
   }
   if (gPad2State & PAD_DOWN) {
      gState.eventCameraPan.x -= CV(0.0625);
   }
   if (gPad2State & PAD_RIGHT) {
      gState.eventCameraPan.z += CV(0.0625);
   }
   if (gPad2State & PAD_LEFT) {
      gState.eventCameraPan.z -= CV(0.0625);
   }
   if (gPad2State & PAD_L1) {
      gState.eventCameraPan.y += CV(0.0625);
   }
   if (gPad2State & PAD_L2) {
      gState.eventCameraPan.y -= CV(0.0625);
   }
}

s16 D_8012334C;
s16 D_80123350;
s16 D_80123354;

#undef OBJF
#define OBJF 719
void Objf719_DimensionalRift(Object *obj) {
   Quad quad1;
   Quad quad2;
   Quad quad3;
   s16 halfSize;
   s32 i;
   Object *sphere;
   Object *entitySprite;
   SVectorXZY *pCoord;

   halfSize = CV(1.0);

   for (i = 0; i < 4; i++) {
      quad1[i].vy = 0;
   }

   switch (obj->state) {
   case 0:
      OBJ.entitySprite = OBJ.entitySpriteParam;
      OBJ.entitySpriteParam = NULL;
      OBJ.gfxIdx = GFX_MASK_EFFECT_1;
      OBJ.clut = CLUT_BLUES;
      OBJ.semiTrans = 2;
      D_8012334C = 0;
      D_80123350 = 0;
      D_80123354 = DEG(180);

      sphere = CreatePositionedObj(obj, OBJF_NOOP);
      OBJ.sphere = sphere;
      sphere->d.sprite.gfxIdx = GFX_TILED_LINES;
      sphere->d.sprite.semiTrans = 2;
      sphere->d.sprite.clut = CLUT_BLUES;

      obj->state++;

   // fallthrough
   case 1:
      ApplyMaskEffect(496 << 2, 384, 64, 64, 384 << 2, 256, D_8012334C % 32, D_80123350 % 32,
                      GFX_MASK_EFFECT_1, 0);

      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n + CV(1.0);

      sphere = OBJ.sphere;
      sphere->x1.n = obj->x1.n;
      sphere->z1.n = obj->z1.n;
      sphere->y1.n = obj->y1.n;

      OBJ.gfxIdx = GFX_MASK_EFFECT_1;

      switch (obj->state2) {
      case 0:
         halfSize = obj->state3;
         obj->state3 += obj->mem;
         if (obj->state3 >= CV(1.0)) {
            obj->state3 = CV(1.0);
            obj->state2++;
         }
         break;

      case 1:
         halfSize = obj->state3;
         break;

      case 2:
         halfSize = obj->state3;
         obj->state3 -= 2;
         if (obj->state3 <= 0) {
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      quad1[0].vx = quad1[2].vx = -halfSize;
      quad1[1].vx = quad1[3].vx = halfSize;
      quad1[0].vz = quad1[1].vz = -halfSize;
      quad1[2].vz = quad1[3].vz = halfSize;

      RotateQuadYZ(quad1, quad3, D_8012334C);
      RotateQuadXZ(quad3, quad2, D_80123350);

      pCoord = &OBJ.coords[0];
      for (i = 0; i < 4; i++) {
         pCoord->x = obj->x1.n + quad2[i].vx;
         pCoord->z = obj->z1.n + quad2[i].vz;
         pCoord->y = obj->y1.n + quad2[i].vy;
         pCoord++;
      }

      OBJ.clut = CLUT_REDS;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      RotateQuadYZ(quad1, quad3, D_8012334C);
      RotateQuadXZ(quad3, quad2, D_80123354);

      pCoord = &OBJ.coords[0];
      for (i = 0; i < 4; i++) {
         pCoord->x = obj->x1.n + quad2[i].vx;
         pCoord->z = obj->z1.n + quad2[i].vz;
         pCoord->y = obj->y1.n + quad2[i].vy;
         pCoord++;
      }

      OBJ.clut = CLUT_BLUES;
      AddObjPrim4(gGraphicsPtr->ot, obj);

      OBJ.coords[0].x = OBJ.coords[2].x = obj->x1.n - halfSize;
      OBJ.coords[1].x = OBJ.coords[3].x = obj->x1.n + halfSize;
      OBJ.coords[0].z = OBJ.coords[1].z = obj->z1.n - halfSize;
      OBJ.coords[2].z = OBJ.coords[3].z = obj->z1.n + halfSize;
      OBJ.coords[0].y = OBJ.coords[1].y = OBJ.coords[2].y = OBJ.coords[3].y = obj->y1.n;

      ApplyMaskEffect(448 << 2, 384, 64, 64, 416 << 2, 384, D_8012334C % 32, D_80123350 % 32,
                      GFX_MASK_EFFECT_2, 0);
      OBJ.gfxIdx = GFX_MASK_EFFECT_2;
      OBJ.semiTrans = 0;
      AddObjPrim4(gGraphicsPtr->ot, obj);
      D_8012334C += 16;
      D_80123350 += 8;
      D_80123354 -= DEG(1.40625);

      sphere = OBJ.sphere;
      sphere->mem = halfSize;
      sphere->state3 = halfSize + ((halfSize >> 1) * rsin(D_8012334C * 8) >> 12);
      sphere->state2 = D_8012334C;
      sphere->d.sprite.clut = CLUT_BLUES;
      RenderSphere(sphere);
      break;

   case 2:
      sphere = OBJ.sphere;
      sphere->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 720
void Objf720_Map61_Scn83_XenoCastingCylinder(Object *obj) {
   Object *obj_a0;
   Cylinder *dsCylinder;

   switch (obj->state) {
   case 0:
      obj_a0 = OBJ.entitySprite;
      obj->x1.n = obj_a0->x1.n;
      obj->z1.n = obj_a0->z1.n;
      obj->y1.n = obj_a0->y1.n;

      obj_a0 = Obj_GetUnused();
      obj_a0->functionIndex = OBJF_NOOP;
      OBJ.dataStore = obj_a0;
      dsCylinder = &obj_a0->d.dataStore.cylinder;
      dsCylinder->bottom.vx = dsCylinder->top.vx = obj->x1.n;
      dsCylinder->bottom.vz = dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + CV(5.0);
      dsCylinder->sideCt = 32;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_COLORS;
      dsCylinder->topRadius = CV(0.0);
      dsCylinder->bottomRadius = CV(0.0);
      dsCylinder->clut = CLUT_BLUES;

      obj->state++;

   // fallthrough
   case 1:
      obj_a0 = OBJ.dataStore;
      dsCylinder = &obj_a0->d.dataStore.cylinder;

      switch (obj->state2) {
      case 0:
         dsCylinder->bottomRadius = dsCylinder->topRadius = obj->mem;
         obj->mem += CV(0.0625);
         if (obj->mem >= CV(1.0)) {
            obj->mem = CV(0.0);
            obj->state2++;
         }
         break;

      case 1:
         if (--obj->mem <= CV(0.0)) {
            obj_a0 = CreatePositionedObj(obj, OBJF_MAP61_SCN83_XENO_CASTING_CYLINDER_CREST);
            obj_a0->d.objf721.gfxIdx = GFX_CREST;
            if (obj->state3 % 2 != 0) {
               obj_a0->d.objf721.clut = CLUT_REDS;
               obj_a0->state2 = 64;
            } else {
               obj_a0->d.objf721.clut = CLUT_BLUES;
               obj_a0->state2 = -64;
            }
            obj_a0->d.objf721.semiTrans = 4;
            obj->mem = CV(0.0625);
            if (++obj->state3 >= 13) {
               obj->mem = CV(0.3125);
               obj->state2++;
            }
         }
         break;

      case 2:
         if (++obj->mem >= CV(1.0)) {
            obj->state2++;
            break;
         }
         break;

      case 3:
         dsCylinder->bottomRadius = dsCylinder->topRadius = obj->mem;
         obj->mem -= CV(0.0625);
         if (obj->mem <= CV(0.0)) {
            obj->state++;
         }
         break;
      }

      RenderCylinderWithRotatedTexture(dsCylinder);
      break;

   case 2:
      obj_a0 = OBJ.dataStore;
      obj_a0->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 721
void Objf721_Map61_Scn83_XenoCastingCylinder_Crest(Object *obj) {
   Quad quad1;
   Quad quad2;
   Object *sprite;
   s16 theta;
   s16 halfSize;
   s32 i;
   SVectorXZY *pCoord;

   switch (obj->state) {
   case 0:
      sprite = CreatePositionedObj(obj, OBJF_NOOP);
      sprite->d.sprite.gfxIdx = OBJ.gfxIdx;
      sprite->d.sprite.clut = OBJ.clut;
      sprite->d.sprite.semiTrans = OBJ.semiTrans;
      OBJ.sprite = sprite;

      obj->mem = CV(0.25);
      obj->state3 = DEG(0);
      obj->state++;

   // fallthrough
   case 1:
      theta = obj->state3;
      obj->state3 += obj->state2;
      sprite = OBJ.sprite;
      halfSize = obj->mem;
      obj->mem += CV(0.0625);
      if (obj->mem >= CV(1.0)) {
         obj->mem = CV(1.0);
      }
      quad1[0].vx = quad1[2].vx = -halfSize;
      quad1[1].vx = quad1[3].vx = halfSize;
      quad1[0].vy = quad1[1].vy = -halfSize;
      quad1[2].vy = quad1[3].vy = halfSize;

      for (i = 0; i < 4; i++) {
         quad1[i].vz = 0;
      }

      RotateQuadXY(quad1, quad2, theta);

      for (i = 0; i < 4; i++) {
         quad2[i].vz = 0;
      }

      pCoord = &sprite->d.sprite.coords[0];
      for (i = 0; i < 4; i++) {
         pCoord->x = sprite->x1.n + quad2[i].vx;
         pCoord->y = sprite->y1.n + quad2[i].vz;
         pCoord->z = sprite->z1.n + quad2[i].vy;
         pCoord++;
      }

      sprite->y1.n += CV(0.09375);
      AddObjPrim4(gGraphicsPtr->ot, sprite);
      AddObjPrim4(gGraphicsPtr->ot, sprite);
      AddObjPrim4(gGraphicsPtr->ot, sprite);

      if (sprite->y1.n >= CV(6.0)) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 722
void Objf722_DimensionalRift_Sparkles(Object *obj) {
   // Colored sparkles that converge into rift; code to spawn exists in 724/741, but it's disabled;

   Object *particle;
   s32 i;
   s32 theta;

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->state3 <= 0) {
         for (i = 0; i < 4; i++) {
            particle = Obj_GetUnused();
            particle->functionIndex = OBJF_HOMING_PARTICLE;
            particle->x2.n = obj->x1.n;
            particle->z2.n = obj->z1.n;
            particle->y2.n = obj->y1.n;
            particle->d.sprite.animData = sSparkleAnimData_80101838;
            particle->d.sprite.clut = sCluts_80101900[(rand() >> 4) % 4];
            theta = rand() % DEG(360);
            particle->x1.n = obj->x1.n + (CV(2.0) * rsin(theta) >> 12);
            particle->z1.n = obj->z1.n + (CV(2.0) * rcos(theta) >> 12);
            theta = rand() % DEG(360);
            particle->y1.n = obj->y1.n + (CV(2.0) * rsin(theta) >> 12);
            particle->state2 = 3;
            particle->mem = 12;
         }
         obj->state3 = 2;
      }
      if (++obj->state2 >= 256) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 723
void Objf723_HomingParticle(Object *obj) {
   s32 shift;

   shift = obj->state2;
   obj->x1.n += (obj->x2.n - obj->x1.n) >> shift;
   obj->z1.n += (obj->z2.n - obj->z1.n) >> shift;
   obj->y1.n += (obj->y2.n - obj->y1.n) >> shift;
   UpdateObjAnimation(obj);
   if (obj->d.sprite.animFinished) {
      obj->d.sprite.hidden = 1;
   }
   AddObjPrim6(gGraphicsPtr->ot, obj, 0);
   if (--obj->mem <= 0) {
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 724
void Objf724_741_DimensionalRift_Open(Object *obj) {
   // 724: Spawned by EVDATA34.DAT, EVDATA39.DAT, EVDATA83.DAT
   // 741: Spawned by EVDATA40.DAT, EVDATA93.DAT

   Object *entitySprite;
   Object *rift;

   switch (obj->state) {
   case 0:
      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n + CV(1.0);
      obj->state = 3;
      break;

   case 1:
      OBJ.sparkles = CreatePositionedObj(obj, OBJF_DIMENSIONAL_RIFT_SPARKLES);
      obj->state++;

   // fallthrough
   case 2:
      if (++obj->mem >= 128) {
         obj->state++;
      }
      break;

   case 3:
      rift = CreatePositionedObj(obj, OBJF_DIMENSIONAL_RIFT);
      rift->d.objf719.entitySpriteParam = OBJ.entitySprite;
      rift->mem = 2;
      if (obj->functionIndex == OBJF_DIMENSIONAL_RIFT_OPEN_741) {
         rift->mem = 64;
      }
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 731
void Objf731_DimensionalRift_Close(Object *obj) {
   s32 i;
   Object *p;

   p = &gObjectArray[0];

   for (i = 0; i < ARRAY_COUNT(gObjectArray); i++) {
      if (p->functionIndex == OBJF_DIMENSIONAL_RIFT) {
         if (p->state == 1) {
            p->state2++;
         }
         break;
      }
      p++;
   }

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 725
void Objf725_CastingRays(Object *obj) {
   Object *obj_v1;
   s32 i;

   switch (obj->state) {
   case 0:
      obj_v1 = obj->d.entitySpawn.entitySpriteParam;
      obj->x1.n = obj_v1->x1.n;
      obj->z1.n = obj_v1->z1.n;
      obj->y1.n = obj_v1->y1.n + CV(0.5);
      gSignal3 = 0;
      obj->state2 = 4;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->state2 <= 0) {
         for (i = 0; i < 3; i++) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_INWARD_RAY;
            obj_v1->x2.n = obj->x1.n;
            obj_v1->z2.n = obj->z1.n;
            obj_v1->y2.n = obj->y1.n;
         }
         obj->state2 = (rand() >> 2) % 3;
      }
      if (gSignal3 == 1) {
         gSignal3 = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 726
void Objf726_CastingRays_Stop(Object *obj) {
   gSignal3 = 1;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 727
void Objf727_RollingThunderOnEntity_Unused(Object *obj) {
   // Unused? Maybe to reuse Rolling Thunder FX for event entity?

   Object *fx;

   fx = Obj_GetUnused();
   fx->functionIndex = OBJF_ROLLING_THUNDER_FX1;
   fx->mem = 1;
   fx->d.objf197.variant_0x24.unitSpriteParam = obj->d.entitySpawn.entitySpriteParam;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 265
void Objf265_266_729_ShrinkWarpSprite(Object *obj) {
   // Spawned in scenes 34/39/83 to suck sprites into dimensional rift.

   static SVectorXZY riftLocations[4] = {{CV(7.5), CV(10.5), CV(2.0)},
                                         {CV(8.5), CV(8.5), CV(3.5)},
                                         {CV(8.5), CV(3.5), CV(2.0)},
                                         {CV(7.0), CV(10.0), CV(2.5)}};

   s32 riftIdx;
   s32 i;
   Object *sprite;
   Object *entitySprite;
   Quad savedQuad;
   Quad *quadp;

   switch (obj->state) {
   case 0:
      switch (obj->functionIndex) {
      default:
      case 267: //?
         riftIdx = 0;
         break;
      case OBJF_MAP23_SCN39_SHRINK_WARP_SPRITE:
         riftIdx = 1;
         break;
      case OBJF_MAP61_SCN83_SHRINK_WARP_SPRITE:
         riftIdx = 2;
         break;
      }

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      entitySprite = OBJ.entitySprite;
      CopyObject(entitySprite, sprite);
      entitySprite->d.sprite.hidden = 1;
      sprite->d.sprite.hidden = 0;
      OBJ.sprite = sprite;
      sprite->x2.n = riftLocations[riftIdx].x;
      sprite->z2.n = riftLocations[riftIdx].z;
      sprite->y2.n = riftLocations[riftIdx].y;

      obj->state3 = 256;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      sprite->x1.n += (sprite->x2.n - sprite->x1.n) >> 3;
      sprite->z1.n += (sprite->z2.n - sprite->z1.n) >> 3;
      sprite->y1.n += (sprite->y2.n - sprite->y1.n) >> 3;

      entitySprite = OBJ.entitySprite;
      quadp = gSpriteBoxQuads[entitySprite->d.sprite.boxIdx];
      for (i = 0; i < 4; i++) {
         savedQuad[i] = (*quadp)[i];
         (*quadp)[i].vx = ((*quadp)[i].vx * obj->state3) >> 8;
         (*quadp)[i].vy = ((*quadp)[i].vy * obj->state3) >> 8;
      }

      sprite->d.sprite.boxIdx = 7;
      RenderUnitSprite(gGraphicsPtr->ot, sprite, 0);

      for (i = 0; i < 4; i++) {
         (*quadp)[i] = savedQuad[i];
      }

      obj->state3 -= 4;
      if (obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 730
void Objf730_EnableAdditiveBlending(Object *obj) {
   Object *entitySprite;

   entitySprite = obj->d.entitySpawn.entitySpriteParam;
   entitySprite->d.sprite.semiTrans = 2;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 794
void Objf794_DisableBlending(Object *obj) {
   Object *entitySprite;

   entitySprite = obj->d.entitySpawn.entitySpriteParam;
   entitySprite->d.sprite.semiTrans = 0;
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 734
void Objf734_MeteorImpact_Unused(Object *obj) {
   static s16 rockAnimData[] = {4, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

   Object *unitSprite;
   Object *obj_s1;
   Object *obj_s2;
   Object *zoom;
   s16 prevQuadIdx;
   POLY_FT4 *poly;
   s32 i;
   s32 theta;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.unitSprite = unitSprite;

      obj_s1 = CreatePositionedObj(obj, OBJF_NOOP);
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->y1.n += CV(6.0);
      obj_s1->d.sprite.animData = rockAnimData;
      obj_s1->y3.n = CV(-0.25);
      obj_s1->y2.n = CV(-0.375);
      obj_s1->d.sprite.boxIdx = 3;
      obj_s1->d.sprite.clut = CLUT_REDS;
      OBJ.sprite = obj_s1;
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = OBJ.sprite;
      unitSprite = OBJ.unitSprite;
      UpdateObjAnimation(obj_s1);
      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);
      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, 220, 128, 128);
      }
      for (i = 0; i < 1; i++) {
         obj_s2 = CreatePositionedObj(obj_s1, OBJF_EXPLOSION);
         theta = rand() % DEG(360);
         obj_s2->x2.n = CV(0.0234375) * rcos(theta) >> 12;
         obj_s2->z2.n = CV(0.0234375) * rsin(theta) >> 12;
         obj_s2->y2.n = -(obj_s1->y2.n >> 3);
         // The explosion object assigns the actual animData based on this initial value
         obj_s2->d.sprite.animData = 1;
      }

      obj_s1->y1.n += obj_s1->y2.n;
      if (obj_s1->y1.n <= unitSprite->y1.n) {
         obj_s1->functionIndex = OBJF_NULL;
         obj->state++;
      }
      break;

   case 2:
      zoom = Obj_GetUnused();
      zoom->functionIndex = OBJF_BOUNCE_ZOOM;
      theta = (rand() >> 2) % DEG(360);
      for (i = 0; i < 8; i++) {
         obj_s1 = CreatePositionedObj(obj, OBJF_FLAMING_ROCK);
         obj_s1->x2.n = CV(0.1875) * rcos(theta) >> 12;
         obj_s1->z2.n = CV(0.1875) * rsin(theta) >> 12;
         obj_s1->y2.n = CV(0.375) + (CV(0.125) * rsin(rand() % DEG(360)) >> 12);
         obj_s1->y3.n = CV(-0.046875);
         obj_s1->x3.n = 0;
         obj_s1->z3.n = 0;
         theta += DEG(45);
      }
      obj_s1 = CreatePositionedObj(obj, OBJF_EXPANDING_EXPLOSION_RING_UNUSED);
      obj_s1->y1.n += CV(0.5);
      obj->state3 = 128;
      obj->state++;
      break;

   case 3:
      if (obj->state3 % 2) {
         obj_s1 = CreatePositionedObj(obj, OBJF_EXPLOSION_RING_EMITTER);
         obj_s1->y1.n += CV(0.5);
         obj_s1->d.objf141.clut = CLUT_REDS;
         obj_s1->d.objf141.semiTrans = 2;
         obj_s1->d.objf141.todo_x2c = 1;
         obj_s1->d.objf141.todo_x28 = 0x20;
         obj_s1->d.objf141.todo_x2a = 0xaa;
         obj_s1->d.objf141.radius = CV(0.4375);
      }
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 756
void Objf756_Map36_Scn75_Cinematic(Object *obj) {
   switch (obj->state) {
   case 0:
      gState.enableMapScaling = 1;
      obj->x1.n = CV(16.0);
      obj->z1.n = CV(16.0);
      obj->state++;

   // fallthrough
   case 1:
      gCameraRotation.vx += 4;
      gCameraRotation.vy += 8;
      gState.eventCameraRot = gCameraRotation.vy;
      gState.eventCameraPan.x += (CV(11.0) - gState.eventCameraPan.x) >> 3;
      gState.eventCameraPan.z += (CV(11.0) - gState.eventCameraPan.z) >> 3;
      obj->x1.n -= CV(0.0625);
      obj->z1.n -= CV(0.0625);
      gMapScale.vx = obj->x1.n;
      gMapScale.vz = obj->z1.n;
      obj->y1.n = obj->x1.n;
      gMapScale.vy = obj->y1.n;
      obj->state3++;
      if (gCameraRotation.vx == DEG(90)) {
         obj->state++;
      }
      break;

   case 2:
      gCameraRotation.vy += 8;
      gState.eventCameraRot = gCameraRotation.vy;
      if (++obj->state3 >= 256) {
         obj->state++;
      }
      break;

   case 3:
      gMapScale.vx += (FIXED(1.0) - gMapScale.vx) >> 4;
      gMapScale.vy += (FIXED(1.0) - gMapScale.vy) >> 4;
      gMapScale.vz += (FIXED(1.0) - gMapScale.vz) >> 4;
      if (++obj->state2 >= 128) {
         gState.enableMapScaling = 0;
         gMapScale.vx = FIXED(1.0);
         gMapScale.vy = FIXED(1.0);
         gMapScale.vz = FIXED(1.0);
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 253
void Objf253_SpawnGraveMarker(Object *obj) {
   CreatePositionedObj(obj->d.entitySpawn.entitySpriteParam, OBJF_MAP_OBJECT_GRAVE_MARKER);
   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 530
void Objf530_Map61_Scn83_VandalHeartForcefield(Object *obj) {
   extern SVECTOR D_80124EB4[6];

   Object *obj_s0;
   s32 i;
   s16 theta;
   u8 intensity;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      gState.mapState.s.field_0x0 = 0;
      obj_s0 = OBJ.entitySprite;
      obj->y1.n = obj_s0->y1.n;
      obj->x1.n = obj_s0->x1.n;
      obj->z1.n = obj_s0->z1.n;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         if (gState.mapState.s.field_0x0 != 0) {
            obj->mem = 30;
            obj->state2++;
         }
         break;

      case 1:
         if (++obj->mem >= 180) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }

   obj_s0 = Obj_GetUnused();
   obj_s0->functionIndex = OBJF_NOOP;
   obj_s0->d.sprite.gfxIdx = GFX_COLORS;
   obj_s0->d.sprite.clut = CLUT_14;
   obj_s0->d.sprite.semiTrans = 2;
   theta = OBJ.theta;

   for (i = 0; i < 6; i++) {
      D_80124EB4[i].vx = CV(0.5) * rsin(theta + i * (DEG(360) / 6)) / ONE;
      D_80124EB4[i].vz = CV(0.5) * rcos(theta + i * (DEG(360) / 6)) / ONE;
   }

   if (obj->state2 != 0) {
      s32 tmp = 256 * rsin(obj->mem * 4096 / 360) / ONE;
      if (tmp > 255) {
         tmp = 255;
      }
      intensity = tmp;
   }

   for (i = 0; i < 5; i++) {
      obj_s0->d.sprite.coords[0].x = D_80124EB4[i].vx + obj->x1.n;
      obj_s0->d.sprite.coords[0].z = D_80124EB4[i].vz + obj->z1.n;
      obj_s0->d.sprite.coords[0].y = obj->y1.n + CV(1.25);
      obj_s0->d.sprite.coords[1].x = D_80124EB4[i + 1].vx + obj->x1.n;
      obj_s0->d.sprite.coords[1].z = D_80124EB4[i + 1].vz + obj->z1.n;
      obj_s0->d.sprite.coords[1].y = obj->y1.n + CV(1.25);
      obj_s0->d.sprite.coords[2].x = D_80124EB4[i].vx + obj->x1.n;
      obj_s0->d.sprite.coords[2].z = D_80124EB4[i].vz + obj->z1.n;
      obj_s0->d.sprite.coords[2].y = obj->y1.n;
      obj_s0->d.sprite.coords[3].x = D_80124EB4[i + 1].vx + obj->x1.n;
      obj_s0->d.sprite.coords[3].z = D_80124EB4[i + 1].vz + obj->z1.n;
      obj_s0->d.sprite.coords[3].y = obj->y1.n;

      AddObjPrim4(gGraphicsPtr->ot, obj_s0);
      if (obj->state2 != 0) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, intensity, intensity, intensity);
      }
   }

   obj_s0->d.sprite.coords[0].x = D_80124EB4[5].vx + obj->x1.n;
   obj_s0->d.sprite.coords[0].z = D_80124EB4[5].vz + obj->z1.n;
   obj_s0->d.sprite.coords[0].y = obj->y1.n + CV(1.25);
   obj_s0->d.sprite.coords[1].x = D_80124EB4[0].vx + obj->x1.n;
   obj_s0->d.sprite.coords[1].z = D_80124EB4[0].vz + obj->z1.n;
   obj_s0->d.sprite.coords[1].y = obj->y1.n + CV(1.25);
   obj_s0->d.sprite.coords[2].x = D_80124EB4[5].vx + obj->x1.n;
   obj_s0->d.sprite.coords[2].z = D_80124EB4[5].vz + obj->z1.n;
   obj_s0->d.sprite.coords[2].y = obj->y1.n;
   obj_s0->d.sprite.coords[3].x = D_80124EB4[0].vx + obj->x1.n;
   obj_s0->d.sprite.coords[3].z = D_80124EB4[0].vz + obj->z1.n;
   obj_s0->d.sprite.coords[3].y = obj->y1.n;

   AddObjPrim4(gGraphicsPtr->ot, obj_s0);
   if (obj->state2 != 0) {
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, intensity, intensity, intensity);
   }

   for (i = 0; i < 5; i++) {
      obj_s0->d.sprite.coords[0].x = obj_s0->d.sprite.coords[1].x = obj->x1.n;
      obj_s0->d.sprite.coords[0].z = obj_s0->d.sprite.coords[1].z = obj->z1.n;
      obj_s0->d.sprite.coords[0].y = obj_s0->d.sprite.coords[1].y = obj->y1.n + CV(1.5);
      obj_s0->d.sprite.coords[2].x = D_80124EB4[i].vx + obj->x1.n;
      obj_s0->d.sprite.coords[2].z = D_80124EB4[i].vz + obj->z1.n;
      obj_s0->d.sprite.coords[2].y = obj->y1.n + CV(1.25);
      obj_s0->d.sprite.coords[3].x = D_80124EB4[i + 1].vx + obj->x1.n;
      obj_s0->d.sprite.coords[3].z = D_80124EB4[i + 1].vz + obj->z1.n;
      obj_s0->d.sprite.coords[3].y = obj->y1.n + CV(1.25);

      AddObjPrim4(gGraphicsPtr->ot, obj_s0);
      if (obj->state2 != 0) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, intensity, intensity, intensity);
      }
   }

   obj_s0->d.sprite.coords[0].x = obj_s0->d.sprite.coords[1].x = obj->x1.n;
   obj_s0->d.sprite.coords[0].z = obj_s0->d.sprite.coords[1].z = obj->z1.n;
   obj_s0->d.sprite.coords[0].y = obj_s0->d.sprite.coords[1].y = obj->y1.n + CV(1.5);
   obj_s0->d.sprite.coords[2].x = D_80124EB4[5].vx + obj->x1.n;
   obj_s0->d.sprite.coords[2].z = D_80124EB4[5].vz + obj->z1.n;
   obj_s0->d.sprite.coords[2].y = obj->y1.n + CV(1.25);
   obj_s0->d.sprite.coords[3].x = D_80124EB4[0].vx + obj->x1.n;
   obj_s0->d.sprite.coords[3].z = D_80124EB4[0].vz + obj->z1.n;
   obj_s0->d.sprite.coords[3].y = obj->y1.n + CV(1.25);

   AddObjPrim4(gGraphicsPtr->ot, obj_s0);
   if (obj->state2 != 0) {
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, intensity, intensity, intensity);
   }

   obj_s0->functionIndex = OBJF_NULL;
   OBJ.theta += DEG(2.8125);
}

#undef OBJF
#define OBJF 540
void Objf540_to_545_Map14_Scn15_CloudSpawner(Object *obj) {
   Object *entitySprite;
   Object *cloud;

   entitySprite = obj->d.entitySpawn.entitySpriteParam;
   cloud = Obj_GetUnused();
   cloud->functionIndex = OBJF_CLOUD;
   cloud->y1.n = entitySprite->y1.n;
   cloud->x1.n = entitySprite->x1.n;
   cloud->z1.n = entitySprite->z1.n;
}

#undef OBJF
#define OBJF 535
void Objf535_536_FadeLight(Object *obj) {
   // 535: EVDATA85.DAT, EVDATA86.DAT
   // 536: EVDATA86.DAT

   switch (obj->state) {
   case 0:
      obj->state2 = (obj->functionIndex == OBJF_FADE_LIGHT_BRIGHTEN) ? 10 : 20;
      obj->state++;
      break;
   }

   switch (obj->state2) {
   case 10:
      gLightColor.r += 2;
      if (gLightColor.r >= 128) {
         gLightColor.r = 128;
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 20:
      gLightColor.r -= 2;
      if (gLightColor.r == 0) {
         gLightColor.r = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }

   gLightColor.g = gLightColor.b = gLightColor.r;
}

#undef OBJF
#define OBJF 256
void Objf256_258_Map36_LeenaAndVillager(Object *obj) {
   static TextureWindow texw[] = {{144, 0, 0, 0}, {192, 0, 0, 0}};

   Object *sprite;
   Object *forcefield;
   s16 prevQuadIdx;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_OBSTACLE;

      switch (obj->functionIndex) {
      case OBJF_MAP36_VILLAGER:
         obj->y2.n = 48; // vram y ofs
         break;

      case OBJF_MAP36_LEENA:
         sprite = CreatePositionedObj(obj, OBJF_NOOP);
         sprite->z1.n += CV(0.5);
         forcefield = CreatePositionedObj(sprite, OBJF_LEENA_FORCEFIELD);
         forcefield->d.objf675.targetSprite = sprite;
         sprite->d.objf675.targetSprite = obj; //?
         break;

      default:
         obj->functionIndex = OBJF_NULL;
         break;
      }

      OBJ.tpage = GetTPage(0, 0, 964, 256);
      OBJ.clut_unused = CLUT_58;
      obj->state++;
      OBJ.gfxIdx = GFX_COLOR_15;
      OBJ.clut = CLUT_58;
      OBJ.boxIdx = 0;
      OBJ.texwIdx = 0;

   // fallthrough
   case 1:

      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         OBJ.texwIdx = 1;
         OBJ.facingLeft = 0;
         break;
      case CAM_DIR_EAST:
         OBJ.texwIdx = 1;
         OBJ.facingLeft = 1;
         break;
      case CAM_DIR_NORTH:
         OBJ.texwIdx = 0;
         OBJ.facingLeft = 1;
         break;
      default:
         OBJ.texwIdx = 0;
         OBJ.facingLeft = 0;
         break;
      }

      prevQuadIdx = gQuadIndex;
      AddObjPrim6(gGraphicsPtr->ot, obj, 1);
      if (prevQuadIdx != gQuadIndex) {
         u8 x, y;
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         poly->tpage = OBJ.tpage;
         x = texw[OBJ.texwIdx].x;
         y = texw[OBJ.texwIdx].y + obj->y2.n;
         setUVWH(poly, x, y, 48, 48);
      }
      break;
   }
}
