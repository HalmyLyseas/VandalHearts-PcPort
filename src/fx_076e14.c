/* Spell casting effects, part 5 (segment 0x76e14): FX1 drivers for the magic-restoration
 * family (Magic Charge / Mage Oil / Mage Gem) and Healing Circle/Wave, plus the shared
 * sparkle child. Dispatched via gSpellsEx (see fx_060c38.c). Objf384 (a skull rising
 * row-by-row over the target, sharing Evil Stream's GFX_SKULL asset) is cut content;
 * Objf_Unk_80087b58 is not even in gObjFunctionPointers -- an unreachable test handler. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

#undef OBJF
#define OBJF 330
void Objf330_MagicRestoration_FX1(Object *obj) {
   static const SVectorXZ deltas[4] = {{-1, 0}, {0, 1}, {0, -1}, {1, 0}};
   static const u8 indices[4][4] = {[CAM_DIR_SOUTH] = {0, 1, 2, 3},
                                    [CAM_DIR_EAST] = {2, 0, 3, 1},
                                    [CAM_DIR_NORTH] = {3, 2, 1, 0},
                                    [CAM_DIR_WEST] = {1, 3, 0, 2}};
   static s16 increment = 0x200;

   Object *obj_s6; // targetSprite, ringBottom
   Object *obj_s7; // ringTop, ray
   Object *casterSprite;
   Object *cam;
   Object *dataStore;
   Cylinder *dsCylinder;
   BVectorXZ *p;
   s32 i;
   s32 ringSize;
   s32 direction;
   s16 savedY;
   s32 dx_0, dz_0;
   s32 dx_1, dz_1;
   s32 dx_2, dz_2;
   s32 dx_3, dz_3;

   switch (obj->state) {
   case 0:

      switch (obj->state2) {
      case 0:
         SnapToUnit(obj);
         obj->y2.n = obj->y1.n;
         OBJ.startingTheta1 = 0;
         OBJ.startingTheta2 = DEG(90);
         OBJ.ringSize = CV(1.0);
         obj->state2++;

      // fallthrough
      case 1:
         p = (BVectorXZ *)gTargetCoords;
         obj_s6 = GetUnitSpriteAtPosition(p->z, p->x);
         casterSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
         if (obj_s6 == casterSprite) {
            obj->state2 = 0;
            obj->state++;
         } else {
            cam = CreatePositionedObj(obj, OBJF_CAMERA_TBD_026);
            cam->d.objf026.target = obj_s6;
            obj->x1.n = obj_s6->x1.n;
            obj->y2.n = obj->y1.n = obj_s6->y1.n;
            obj->z1.n = obj_s6->z1.n;
            obj->state3 = 8;
            obj->state2++;
         }
         break;

      case 2:
         if (--obj->state3 <= 0) {
            obj->state2 = 0;
            obj->state3 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 1:
      obj_s7 = Obj_GetUnused();
      obj_s7->functionIndex = OBJF_NOOP;
      OBJ.ringTop = obj_s7;
      direction = (gCameraRotation.vy & 0xfff) / DEG(90);
      obj_s7->d.sprite.gfxIdx = GFX_RING_TOP;
      obj_s7->d.sprite.clut = CLUT_BLUES;

      ringSize = OBJ.ringSize;
      dx_0 = deltas[indices[direction][0]].x * ringSize;
      dz_0 = deltas[indices[direction][0]].z * ringSize;
      dx_1 = deltas[indices[direction][1]].x * ringSize;
      dz_1 = deltas[indices[direction][1]].z * ringSize;
      dx_2 = deltas[indices[direction][2]].x * ringSize;
      dz_2 = deltas[indices[direction][2]].z * ringSize;
      dx_3 = deltas[indices[direction][3]].x * ringSize;
      dz_3 = deltas[indices[direction][3]].z * ringSize;

      obj_s7->d.sprite.coords[0].x = obj->x1.n + dx_0;
      obj_s7->d.sprite.coords[0].z = obj->z1.n + dz_0;
      obj_s7->d.sprite.coords[1].x = obj->x1.n + dx_1;
      obj_s7->d.sprite.coords[1].z = obj->z1.n + dz_1;
      obj_s7->d.sprite.coords[2].x = obj->x1.n + (dx_0 + dx_2) / 2;
      obj_s7->d.sprite.coords[2].z = obj->z1.n + (dz_0 + dz_2) / 2;
      obj_s7->d.sprite.coords[3].x = obj->x1.n + (dx_1 + dx_3) / 2;
      obj_s7->d.sprite.coords[3].z = obj->z1.n + (dz_1 + dz_3) / 2;

      obj_s6 = Obj_GetUnused();
      obj_s6->functionIndex = OBJF_NOOP;
      OBJ.ringBottom = obj_s6;
      obj_s6->d.sprite.gfxIdx = GFX_RING_BTM;
      obj_s6->d.sprite.clut = CLUT_BLUES;
      obj_s6->d.sprite.coords[2].x = obj->x1.n + dx_2;
      obj_s6->d.sprite.coords[2].z = obj->z1.n + dz_2;
      obj_s6->d.sprite.coords[3].x = obj->x1.n + dx_3;
      obj_s6->d.sprite.coords[3].z = obj->z1.n + dz_3;
      obj_s6->d.sprite.coords[0].x = obj->x1.n + (dx_0 + dx_2) / 2;
      obj_s6->d.sprite.coords[0].z = obj->z1.n + (dz_0 + dz_2) / 2;
      obj_s6->d.sprite.coords[1].x = obj->x1.n + (dx_1 + dx_3) / 2;
      obj_s6->d.sprite.coords[1].z = obj->z1.n + (dz_1 + dz_3) / 2;
      obj->state2 = 0;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 2:

      switch (obj->state2) {
      case 0:
         dataStore = Obj_GetUnused();
         dataStore->functionIndex = OBJF_NOOP;
         OBJ.dataStore = dataStore;
         dsCylinder = &dataStore->d.dataStore.cylinder;
         dsCylinder->top.vx = dsCylinder->bottom.vx = obj->x1.n;
         dsCylinder->top.vz = dsCylinder->bottom.vz = obj->z1.n;
         dsCylinder->bottom.vy = obj->y1.n;
         dsCylinder->top.vy = obj->y1.n + CV(4.0);
         dsCylinder->sideCt = 16;
         dsCylinder->semiTrans = 4;
         dsCylinder->gfxIdx = GFX_TILED_FLAMES;
         obj->mem = 0;
         dsCylinder->bottomRadius = 0;
         dsCylinder->topRadius = 0;
         dsCylinder->useColor = 1;
         dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = 0;
         dsCylinder->theta = 0;
         dsCylinder->clut = CLUT_PURPLES;
         obj->state2++;

      // fallthrough
      case 1:
         dataStore = OBJ.dataStore;
         dsCylinder = &dataStore->d.dataStore.cylinder;
         obj->mem += 0x10;
         dsCylinder->topRadius = dsCylinder->bottomRadius = obj->mem;
         dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = obj->mem >> 1;
         dsCylinder->theta += DEG(2.8125);
         RenderCylinder(dsCylinder);
         if (obj->mem >= 0x100) {
            obj->state++;
         }
         break;

      case 2:
         break;
      }

      break;

   case 3:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      obj->mem -= 2;
      if (obj->mem <= 0) {
         obj->mem = 0;
      }
      dsCylinder->color.r = dsCylinder->color.g = dsCylinder->color.b = obj->mem >> 1;
      dsCylinder->theta += DEG(2.8125);
      RenderCylinder(dsCylinder);

      obj->y1.n += CV(0.09375);
      obj_s7 = OBJ.ringTop;
      obj_s6 = OBJ.ringBottom;
      savedY = obj->y1.n;
      OBJ.theta1 = OBJ.startingTheta1;
      OBJ.theta2 = OBJ.startingTheta2;

      for (i = 0; i < 16; i++) {
         obj_s7->d.sprite.coords[0].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta1) >> 12);
         obj_s7->d.sprite.coords[1].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta2) >> 12);
         obj_s7->d.sprite.coords[2].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta2 + DEG(180)) >> 12);
         obj_s7->d.sprite.coords[3].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta1 + DEG(180)) >> 12);
         obj_s7->d.sprite.coords[2].y =
             (obj_s7->d.sprite.coords[2].y + obj_s7->d.sprite.coords[0].y) / 2;
         obj_s7->d.sprite.coords[3].y =
             (obj_s7->d.sprite.coords[3].y + obj_s7->d.sprite.coords[1].y) / 2;
         obj_s7->d.sprite.otOfs = -6;
         obj_s7->x1.n = obj_s7->d.sprite.coords[0].x;
         obj_s7->z1.n = obj_s7->d.sprite.coords[0].z;
         obj_s7->y1.n = obj_s7->d.sprite.coords[0].y;
         AddObjPrim5(gGraphicsPtr->ot, obj_s7);

         obj_s6->d.sprite.coords[0].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta1) >> 12);
         obj_s6->d.sprite.coords[1].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta2) >> 12);
         obj_s6->d.sprite.coords[2].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta2 + DEG(180)) >> 12);
         obj_s6->d.sprite.coords[3].y = obj->y1.n + (CV(0.25) * rsin(OBJ.theta1 + DEG(180)) >> 12);
         obj_s6->d.sprite.coords[0].y =
             (obj_s6->d.sprite.coords[2].y + obj_s6->d.sprite.coords[0].y) / 2;
         obj_s6->d.sprite.coords[1].y =
             (obj_s6->d.sprite.coords[3].y + obj_s6->d.sprite.coords[1].y) / 2;
         obj_s6->x1.n = obj_s6->d.sprite.coords[3].x;
         obj_s6->z1.n = obj_s6->d.sprite.coords[3].z;
         obj_s6->y1.n = obj_s6->d.sprite.coords[3].y;
         obj_s6->d.sprite.otOfs = 8;
         AddObjPrim5(gGraphicsPtr->ot, obj_s6);

         OBJ.theta1 += increment;
         OBJ.theta2 += increment;

         obj->y1.n -= CV(0.5);
         if (obj->y1.n < obj->y2.n) {
            break;
         }
      }

      for (i = 0; i < 2; i++) {
         obj_s7 = Obj_GetUnused();
         obj_s7->functionIndex = OBJF_VERTICAL_RAY;
         obj_s7->x2.n = obj->x1.n;
         obj_s7->z2.n = obj->z1.n;
         obj_s7->y2.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
         obj_s7->d.objf315.theta = rand() % DEG(360);
         obj_s7->d.objf315.radius = CV(0.75);
         obj_s7->d.objf315.top = obj->y2.n;
         obj_s7->d.objf315.height = rand() % CV(1.0);
         obj_s7->d.objf315.rotationSpeed = 0x40;
         obj_s7->d.objf315.speed = 0x18 + rand() % 0x30;
         obj_s7->d.objf315.clut = 3 + (rand() & 3);
         obj_s7->d.objf315.gfxIdx = GFX_COLOR_15;
         obj_s7->state = 1;
      }

      obj->y1.n = savedY;
      OBJ.startingTheta1 += DEG(33.75);
      OBJ.startingTheta2 += DEG(33.75);
      if (obj->y1.n > obj->y2.n + CV(6.0)) {
         obj->state++;
      }
      break;

   case 4:
      obj_s7 = OBJ.ringTop;
      obj_s7->functionIndex = OBJF_NULL;
      obj_s6 = OBJ.ringBottom;
      obj_s6->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      dataStore = OBJ.dataStore;
      dataStore->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

void HealingCircle_RenderBlueRing(Object *healingCircle, s32 outerRadius, s32 thickness, s32 t) {
   Object *sprite;
   s32 cx, cz;
   POLY_FT4 *poly;
   s32 i;
   u32 rgbCode;

   sprite = Obj_GetUnused();
   sprite->functionIndex = OBJF_NOOP;
   cx = healingCircle->x1.n;
   cz = healingCircle->z1.n;
   sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES;
   sprite->d.sprite.semiTrans = 2;
   sprite->d.sprite.clut = CLUT_BLUES;

   sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[2].y =
       sprite->d.sprite.coords[3].y = healingCircle->y1.n;

   sprite->d.sprite.coords[0].x = cx + ((outerRadius - thickness) * rcos(0) >> 12);
   sprite->d.sprite.coords[1].x = cx + (outerRadius * rcos(0) >> 12);
   sprite->d.sprite.coords[0].z = cz + ((outerRadius - thickness) * rsin(0) >> 12);
   sprite->d.sprite.coords[1].z = cz + (outerRadius * rsin(0) >> 12);

   rgbCode = POLY_RGB_CODE(GPU_CODE_POLY_FT4 | GPU_CODE_SEMI_TRANS, t, t, t);

   for (i = 0; i < 32; i++) {
      sprite->d.sprite.coords[2].x = cx + ((outerRadius - thickness) * rcos((i + 1) * 0x80) >> 12);
      sprite->d.sprite.coords[3].x = cx + (outerRadius * rcos((i + 1) * 0x80) >> 12);
      sprite->d.sprite.coords[2].z = cz + ((outerRadius - thickness) * rsin((i + 1) * 0x80) >> 12);
      sprite->d.sprite.coords[3].z = cz + (outerRadius * rsin((i + 1) * 0x80) >> 12);
      sprite->x1.n = cx;
      sprite->z1.n = cz;
      sprite->y1.n = healingCircle->y1.n;
      AddObjPrim3(gGraphicsPtr->ot, sprite);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      *(u32 *)&poly->r0 = rgbCode;
      sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[2].x;
      sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[3].x;
      sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[2].z;
      sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[3].z;
   }

   sprite->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 381
void Objf381_HealingCircle_FX1(Object *obj) {
   Object *sparkle;
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      p = (BVectorXZ *)gTargetCoords;
      GetUnitSpriteAtPosition(p->z, p->x);
      obj->state2 = 0x10;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      gLightColor.r -= 2;
      gLightColor.g -= 2;
      gLightColor.b -= 2;
      gCameraZoom.vz += 4;

      if (++obj->state3 >= 0x20) {
         obj->state++;
      }

      break;

   case 2:
      for (i = 0; i < 32; i++) {
         sparkle = Obj_GetUnused();
         sparkle->functionIndex = OBJF_SPARKLE;
         sparkle->x1.n = obj->x1.n;
         sparkle->z1.n = obj->z1.n;
         sparkle->y1.n = obj->y1.n;
         sparkle->x2.n = 0x20 * rcos(i * 0x80) >> 12;
         sparkle->z2.n = 0x20 * rsin(i * 0x80) >> 12;
         sparkle->state2 = 0x40;
      }
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 3:
      gCameraZoom.vz += 4;
      gCameraRotation.vx += 2;
      gCameraRotation.vy += 3;

      if (obj->state2 <= 0x200 - 4) {
         obj->state2 += (((0x200 - 4) - obj->state2) >> 4) + 4;
      } else {
         for (i = 0; i < 32; i++) {
            sparkle = Obj_GetUnused();
            sparkle->functionIndex = OBJF_SPARKLE;
            sparkle->x1.n = obj->x1.n;
            sparkle->z1.n = obj->z1.n;
            sparkle->y1.n = obj->y1.n;
            sparkle->x2.n = 0x20 * rcos(i * 0x80) >> 12;
            sparkle->z2.n = 0x20 * rsin(i * 0x80) >> 12;
            sparkle->state2 = 0x40;
         }
         obj->state2 = 0;
         return;
      }

      //? Last arg u8?
      HealingCircle_RenderBlueRing(obj, obj->state2, 0x10, ((0x200 - obj->state2) >> 1) & 0xff);

      if (++obj->state3 >= 0x80) {
         gSignal3 = 1;
         obj->state3 = 0;
         obj->state++;
      }

      break;

   case 4:
      gLightColor.r += 2;
      gLightColor.g += 2;
      gLightColor.b += 2;

      if (++obj->state3 >= 0x20) {
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 383
void Objf383_Sparkle(Object *obj) {
   static s16 sparkleAnimData[20] = {
       5, GFX_SPARKLE_1, 2, GFX_SPARKLE_2, 2, GFX_SPARKLE_3, 2, GFX_SPARKLE_4, 2, GFX_SPARKLE_5,
       2, GFX_SPARKLE_4, 2, GFX_SPARKLE_3, 2, GFX_SPARKLE_2, 2, GFX_NULL,      1, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->d.sprite.boxIdx = 3;
      obj->d.sprite.animData = sparkleAnimData;
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;

      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 384
/* A skull (Evil Stream's GFX_SKULL) restored row by row rising over the hidden target
 * sprite, bottom row smeared to the screen edge; gSignal3 on completion (FX1-shaped).
 * Cut content: no spell, event, or code dispatches 384. */
void Objf384_SkullRise_Unused(Object *obj) {
   Object *unitSprite;
   Object *sprite;
   POLY_FT4 *poly;

   switch (obj->state) {
   case 0:
      unitSprite = SnapToUnit(obj);
      unitSprite->d.sprite.hidden = 1;
      obj->state2 = 0;
      obj->state3 = 0x20;
      obj->state++;
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][0] = gGfxSubTextures[GFX_SKULL][0];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = gGfxSubTextures[GFX_SKULL][1];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][2] = gGfxSubTextures[GFX_SKULL][2];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][3] = gGfxSubTextures[GFX_SKULL][3] - 0x10;
      gGfxTPageIds[GFX_TILED_RED_SPARKLES_DYN_1] = gGfxTPageIds[GFX_SKULL];
      gQuad_800fe63c[0].vx = -0x10;
      gQuad_800fe63c[0].vy = -0x10;
      gQuad_800fe63c[1].vx = 0x10;
      gQuad_800fe63c[1].vy = -0x10;
      gQuad_800fe63c[2].vx = -0x10;
      gQuad_800fe63c[2].vy = 0;
      gQuad_800fe63c[3].vx = 0x10;
      gQuad_800fe63c[3].vy = 0;

   // fallthrough
   case 1:
      sprite = Obj_GetUnused();
      gQuad_800fe63c[0].vx = -0x10;
      gQuad_800fe63c[0].vy = -0x10;
      gQuad_800fe63c[1].vx = 0x10;
      gQuad_800fe63c[1].vy = -0x10;
      gQuad_800fe63c[2].vx = -0x10;
      gQuad_800fe63c[2].vy = 0x10 - obj->state3;
      gQuad_800fe63c[3].vx = 0x10;
      gQuad_800fe63c[3].vy = 0x10 - obj->state3;
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][0] = gGfxSubTextures[GFX_SKULL][0];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = gGfxSubTextures[GFX_SKULL][1];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][2] = gGfxSubTextures[GFX_SKULL][2];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][3] =
          gGfxSubTextures[GFX_SKULL][3] - obj->state3;
      sprite->d.sprite.boxIdx = 7;
      sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES_DYN_1;
      sprite->d.sprite.clut = CLUT_2;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n + (obj->state3 << 2);
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      gQuad_800fe63c[0].vx = -0x10;
      gQuad_800fe63c[0].vy = 0x10 - obj->state3;
      gQuad_800fe63c[1].vx = 0x10;
      gQuad_800fe63c[1].vy = 0x10 - obj->state3;
      gQuad_800fe63c[2].vx = -0x10;
      gQuad_800fe63c[2].vy = 0x10;
      gQuad_800fe63c[3].vx = 0x10;
      gQuad_800fe63c[3].vy = 0x10;
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][0] = gGfxSubTextures[GFX_SKULL][0];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] =
          gGfxSubTextures[GFX_SKULL][1] + (0x20 - obj->state3);
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][2] = gGfxSubTextures[GFX_SKULL][2];
      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][3] = 0;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      poly->y3 = 0xf0;
      poly->y2 = 0xf0;
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      unitSprite->d.sprite.hidden = 0;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

void Objf711_712_Noop(void) {}

#undef OBJF
#define OBJF Unk80087b58
/* UNREACHABLE: declared but absent from gObjFunctionPointers[] -- a test handler.
 * Mechanism: sine-pulsed flat-grey cylinder on the target for 512 frames, then
 * gSignal3. Keeps its address name on purpose. */
void Objf_Unk_80087b58(Object *obj) {
   Object *dataStore;
   Cylinder *dsCylinder;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;

      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->sideCt = 32;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_COLOR_13;
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->useColor = 1;
      dsCylinder->color.r = 0x40;
      dsCylinder->color.g = 0x40;
      dsCylinder->color.b = 0x40;

      obj->state++;
      obj->state2 = 0;

   // fallthrough
   case 1:
      dataStore = OBJ.dataStore;
      obj->state3 += 0x10;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->topRadius = 1;
      dsCylinder->topRadius = dsCylinder->bottomRadius =
          CV(1.0) + (CV(1.0) * rsin(obj->state3) >> 12);
      RenderCylinder(dsCylinder);
      RenderCylinder(dsCylinder);
      if (++obj->state2 >= 0x200) {
         obj->state++;
      }
      break;

   case 2:
      dataStore = OBJ.dataStore;
      dataStore->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}
