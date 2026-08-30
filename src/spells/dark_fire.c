/* Dark Fire (Objf122/188) and Roman Fire FX1 (Objf080/081), dispatched via gSpellsEx
 * (docs/decomp/spell-fx-dispatch.md), plus the family's explosion/flame/smoke anim tables --
 * global because Salamander and Dynamo Hum read them too. Objf291 (chest impact) is a stray. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "audio.h"

// The values offset by 0x100 will be flipped.
static s16 sDarkFireSphereAnimData_800ff344[36] = {7, GFX_TILED_GRAY_SPARKLES_DYN_1, // Marker?
                                                   1, GFX_LIT_SPHERE_1,
                                                   1, GFX_LIT_SPHERE_2,
                                                   1, GFX_LIT_SPHERE_3,
                                                   1, GFX_LIT_SPHERE_4,
                                                   1, GFX_LIT_SPHERE_5,
                                                   1, GFX_LIT_SPHERE_6,
                                                   1, GFX_LIT_SPHERE_7,
                                                   1, GFX_LIT_SPHERE_8,
                                                   1, GFX_LIT_SPHERE_7 + 0x100,
                                                   1, GFX_LIT_SPHERE_6 + 0x100,
                                                   1, GFX_LIT_SPHERE_5 + 0x100,
                                                   1, GFX_LIT_SPHERE_4 + 0x100,
                                                   1, GFX_LIT_SPHERE_3 + 0x100,
                                                   1, GFX_LIT_SPHERE_2 + 0x100,
                                                   1, GFX_LIT_SPHERE_1 + 0x100,
                                                   1, GFX_NULL,
                                                   1, GFX_NULL};

s16 gSparkleAnimData_800ff38c[14] = {4, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};

static s16 sExplosionAnimData_800ff3a8[26] = {
    0, GFX_EXPLOSION_1, 2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,  2, GFX_EXPLOSION_4,
    2, GFX_EXPLOSION_5, 2, GFX_EXPLOSION_6,  2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,
    2, GFX_EXPLOSION_9, 2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
    1, GFX_NULL};

s16 gExplosionAnimData_800ff3dc[26] = {0, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                                       2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

static s16 sFlameAnimData_800ff410[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

static s16 sSmokeAnimData_800ff438[24] = {
    0, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4,  2, GFX_PUFF_5, 2, GFX_PUFF_6,
    2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   1, GFX_NULL};

s16 gLightningAnimData_800ff468[20] = {0, GFX_LIGHTNING_1, 2, GFX_LIGHTNING_2, 2, GFX_LIGHTNING_3,
                                       2, GFX_LIGHTNING_4, 2, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6,
                                       2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8, 2, GFX_NULL,
                                       1, GFX_NULL};

static s16 sSmokeAnimData_800ff490[8] = {0, GFX_PUFF_2, 1, GFX_PUFF_3, 1, GFX_NULL, 1, GFX_NULL};

static s16 *sAnimPointers_800ff4a0[4] = {gSparkleAnimData_800ff38c, sExplosionAnimData_800ff3a8,
                                         sFlameAnimData_800ff410, sSmokeAnimData_800ff438};

static s16 sImpactAnimData_800ff4b0[18] = {5, GFX_IMPACT_1, 1, GFX_IMPACT_2, 1, GFX_IMPACT_3,
                                           1, GFX_IMPACT_4, 1, GFX_IMPACT_5, 1, GFX_IMPACT_6,
                                           1, GFX_IMPACT_7, 1, GFX_NULL,     0, GFX_NULL};

static s16 sImpactAnimData_800ff4d4[18] = {0, GFX_IMPACT_1, 1, GFX_IMPACT_2, 1, GFX_IMPACT_3,
                                           1, GFX_IMPACT_4, 1, GFX_IMPACT_5, 1, GFX_IMPACT_6,
                                           1, GFX_IMPACT_7, 1, GFX_NULL,     0, GFX_NULL};

static s16 sFlameAnimData_800ff4f8[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

#undef OBJF
#define OBJF 122
void Objf122_DarkFire_FX1(Object *obj) {
   Object *obj_s2;
   Object *obj_s1;
   Object *obj_s0;
   Object *target;
   POLY_FT4 *poly;
   s32 i;
   s16 theta;
   s16 halfSize;

   switch (obj->state) {
   case 0:
      target = Obj_GetUnused();
      target->functionIndex = OBJF_NOOP;
      target->x1.n = gTargetX * CV(1.0) + CV(0.5);
      target->z1.n = gTargetZ * CV(1.0) + CV(0.5);
      target->y1.n = GetTerrainElevation(gTargetZ, gTargetX) + CV(1.0);
      OBJ.target = target;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = target;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.animData = sDarkFireSphereAnimData_800ff344;
      OBJ.sprite = obj_s1;

      for (i = 0; i < 0x40; i++) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_DARK_FIRE_RAY;
         obj_s1->d.objf188.target = target;
      }

      OBJ.savedLightRotZ = gLightRotation.vz;
      OBJ.savedLightRotY = gLightRotation.vy;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         if (gLightColor.g > 0x20) {
            gLightColor.g -= 4;
            gLightColor.b -= 4;
            gLightColor.r += 4;
         }
         gQuad_800fe63c[0].vx = -0x18;
         gQuad_800fe63c[0].vy = -0x18;
         gQuad_800fe63c[1].vx = 0x18;
         gQuad_800fe63c[1].vy = -0x18;
         gQuad_800fe63c[2].vx = -0x18;
         gQuad_800fe63c[2].vy = 0x18;
         gQuad_800fe63c[3].vx = 0x18;
         gQuad_800fe63c[3].vy = 0x18;
         break;

      case 1:
         theta = OBJ.theta - DEG(90);
         if (theta < 0) {
            theta = 0;
         }
         halfSize = 0x18 * (ONE - rsin(theta)) / ONE;
         gQuad_800fe63c[0].vx = -halfSize;
         gQuad_800fe63c[0].vy = -halfSize;
         gQuad_800fe63c[1].vx = halfSize;
         gQuad_800fe63c[1].vy = -halfSize;
         gQuad_800fe63c[2].vx = -halfSize;
         gQuad_800fe63c[2].vy = halfSize;
         gQuad_800fe63c[3].vx = halfSize;
         gQuad_800fe63c[3].vy = halfSize;
         break;
      }

      obj_s2 = OBJ.target;
      obj_s1 = OBJ.sprite;

      UpdateObjAnimation(obj_s1);
      if (obj_s1->d.sprite.gfxIdx >= 0x100) {
         obj_s1->d.sprite.gfxIdx -= 0x100;
         obj_s1->d.sprite.facingLeft = 1;
      } else {
         obj_s1->d.sprite.facingLeft = 0;
      }

      obj_s1->x1.n = obj_s2->x1.n;
      obj_s1->z1.n = obj_s2->z1.n;
      obj_s1->y1.n = GetTerrainElevation(obj_s2->z1.s.hi, obj_s2->x1.s.hi) + CV(1.0);
      obj_s1->d.sprite.semiTrans = 0;
      obj_s1->d.sprite.clut = CLUT_BLUES;

      if (obj_s1->d.sprite.gfxIdx != GFX_TILED_GRAY_SPARKLES_DYN_1) {
         AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, 0x30, 0x30, 0x30);
      }

      obj_s0 = Obj_GetUnused();
      CopyObject(obj_s1, obj_s0);
      obj_s0->d.sprite.gfxIdx = GFX_LIT_SPHERE_8;
      obj_s0->d.sprite.clut = CLUT_BLUES;
      obj_s0->d.sprite.semiTrans = 0;
      AddObjPrim6(gGraphicsPtr->ot, obj_s0, 0);
      obj_s0->functionIndex = OBJF_NULL;
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, 0x18, 0x18, 0x18);

      switch (obj->state2) {
      case 0:
         if (OBJ.timer == 128) {
            obj->state2++;
         }
         OBJ.timer += 8;
         gLightRotation.vz = (gLightRotation.vz + 0x80) % DEG(360);
         gLightRotation.vy = (gLightRotation.vy + 0x20) % DEG(360);
         break;

      case 1:
         if (theta > 0x280) {
            gLightColor.g += 4;
            gLightColor.b += 4;
            gLightColor.r -= 4;
            gLightRotation.vz += (OBJ.savedLightRotZ - gLightRotation.vz) >> 3;
            gLightRotation.vy += (OBJ.savedLightRotY - gLightRotation.vy) >> 3;
         } else {
            gLightRotation.vz = (gLightRotation.vz + 0x80) % DEG(360);
            gLightRotation.vy = (gLightRotation.vy + 0x20) % DEG(360);
         }
         if (theta == 0x220) {
            gSignal3 = 1;
         }
         if (theta >= DEG(90)) {
            obj->functionIndex = OBJF_NULL;
            obj_s1->functionIndex = OBJF_NULL;
            obj_s2->functionIndex = OBJF_NULL;
            gLightColor.r = 0x80;
            gLightColor.g = 0x80;
            gLightColor.b = 0x80;
            gLightRotation.vz = OBJ.savedLightRotZ;
            gLightRotation.vy = OBJ.savedLightRotY;
         }
         OBJ.theta += 0x10;
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF 188
void Objf188_DarkFire_Ray(Object *obj) {
   Object *target;
   Object *sprite;

   switch (obj->state) {
   case 0:
      target = OBJ.target;
      obj->y1.n = GetTerrainElevation(target->z1.s.hi, target->x1.s.hi) + CV(1.0);
      obj->x1.n = target->x1.n;
      obj->z1.n = target->z1.n;

      OBJ.maxRadius = CV(3.0) + rand() % CV(3.0);
      OBJ.thetaX = rand() % DEG(360);
      OBJ.thetaZ = rand() % DEG(360);
      OBJ.thetaY = rand() % DEG(360);
      OBJ.thetaXSpeed = 0x60 - rand() % 0xc1;
      OBJ.thetaZSpeed = 0x60 - rand() % 0xc1;
      OBJ.thetaYSpeed = 0x60 - rand() % 0xc1;

      obj->state++;

   // fallthrough
   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_COLOR_14;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.semiTrans = 1;

      OBJ.radius = OBJ.maxRadius * OBJ.growth / 0x20;

      sprite->d.sprite.coords[0].x = obj->x1.n;
      sprite->d.sprite.coords[0].z = obj->z1.n;
      sprite->d.sprite.coords[0].y = obj->y1.n;
      sprite->d.sprite.coords[1].x = obj->x1.n;
      sprite->d.sprite.coords[1].z = obj->z1.n;
      sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].x = obj->x1.n + OBJ.radius * rcos(OBJ.thetaX) / ONE;
      sprite->d.sprite.coords[2].z = obj->z1.n + OBJ.radius * rsin(OBJ.thetaZ) / ONE;
      sprite->d.sprite.coords[2].y = obj->y1.n + OBJ.radius * rsin(OBJ.thetaY) / ONE;
      sprite->d.sprite.coords[3].x = obj->x1.n + OBJ.radius * rcos(OBJ.thetaX + 8) / ONE;
      sprite->d.sprite.coords[3].z = obj->z1.n + OBJ.radius * rsin(OBJ.thetaZ + 8) / ONE;
      sprite->d.sprite.coords[3].y = obj->y1.n + OBJ.radius * rsin(OBJ.thetaY + 8) / ONE;

      AddObjPrim4(gGraphicsPtr->ot, sprite);
      OBJ.thetaX += OBJ.thetaXSpeed;
      OBJ.thetaZ += OBJ.thetaZSpeed;
      OBJ.thetaY += OBJ.thetaYSpeed;

      sprite->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.growth += 2;
         if (OBJ.growth == 0x20) {
            obj->state2++;
         }
         break;
      case 1:
         OBJ.timer++;
         if (OBJ.timer == 0x5c) {
            obj->state2++;
         }
         break;
      case 2:
         OBJ.growth -= 2;
         if (OBJ.growth <= 0) {
            obj->functionIndex = OBJF_NULL;
         }
         break;
      }

      break;
   }
}

#undef OBJF
#define OBJF Unk80080924
/* UNREACHABLE: declared but absent from gObjFunctionPointers[] -- retail leftover.
 * Mechanism: 8 smoke puffs bursting outward and arcing up over 10 frames; a near-
 * duplicate of Objf215_Cloud. Keeps its address name on purpose. */
void Objf_Unk_80080924(Object *obj) {
   // Very similar to Objf215_Cloud
   extern s16 gSmokeAnimData_800ff1b0[24];
   Object *sprite;
   s16 halfSize, randomAngle;
   s16 theta;

   switch (obj->state) {
   case 0:
      theta = OBJ.theta;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = gSmokeAnimData_800ff1b0;
      sprite->d.sprite.animInitialized = 0;
      OBJ.sprite = sprite;

      randomAngle = rand() % DEG(360);
      OBJ.position1.x = rsin(randomAngle) >> 6;
      OBJ.position1.z = rcos(randomAngle) >> 6;
      OBJ.position1.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position2.x = rsin(randomAngle) >> 6;
      OBJ.position2.z = rcos(randomAngle) >> 6;
      OBJ.position2.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position3.x = rsin(randomAngle) >> 6;
      OBJ.position3.z = rcos(randomAngle) >> 6;
      OBJ.position3.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position4.x = rsin(randomAngle) >> 6;
      OBJ.position4.z = rcos(randomAngle) >> 6;
      OBJ.position4.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position5.x = rsin(randomAngle) >> 6;
      OBJ.position5.z = rcos(randomAngle) >> 6;
      OBJ.position5.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position6.x = rsin(randomAngle) >> 6;
      OBJ.position6.z = rcos(randomAngle) >> 6;
      OBJ.position6.y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position7_x = rsin(randomAngle) >> 6;
      OBJ.position7_z = rcos(randomAngle) >> 6;
      OBJ.position7_y = (rand() % 0x60) + 0x20;

      randomAngle = rand() % DEG(360);
      OBJ.position8_x = rsin(randomAngle) >> 6;
      OBJ.position8_z = rcos(randomAngle) >> 6;
      OBJ.position8_y = (rand() % 0x60) + 0x20;

      OBJ.halfSize = (rand() % theta) + theta;
      obj->state++;

   // fallthrough
   case 1:
      halfSize = OBJ.halfSize;
      gQuad_800fe63c[0].vx = -halfSize;
      gQuad_800fe63c[0].vy = -halfSize;
      gQuad_800fe63c[1].vx = halfSize;
      gQuad_800fe63c[1].vy = -halfSize;
      gQuad_800fe63c[2].vx = -halfSize;
      gQuad_800fe63c[2].vy = halfSize;
      gQuad_800fe63c[3].vx = halfSize;
      gQuad_800fe63c[3].vy = halfSize;

      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);

      sprite->x1.n =
          obj->x1.n + OBJ.position1.x + (OBJ.position1.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position1.z + (OBJ.position1.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position1.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position2.x + (OBJ.position2.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position2.z + (OBJ.position2.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position2.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position3.x + (OBJ.position3.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position3.z + (OBJ.position3.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position3.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position4.x + (OBJ.position4.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position4.z + (OBJ.position4.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position4.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position5.x + (OBJ.position5.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position5.z + (OBJ.position5.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position5.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position6.x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position6.z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position6.y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position7_x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position7_z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position7_y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      sprite->x1.n =
          obj->x1.n + OBJ.position8_x + (OBJ.position6.x * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->z1.n =
          obj->z1.n + OBJ.position8_z + (OBJ.position6.z * (ONE - rcos(OBJ.theta)) / ONE);
      sprite->y1.n = obj->y1.n + (OBJ.position8_y * rsin(OBJ.theta)) / ONE;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      OBJ.theta += DEG(9);
      if (OBJ.theta >= DEG(90)) {
         obj->functionIndex = OBJF_NULL;
         sprite->functionIndex = OBJF_NULL;
      }
      break;
   }
}

/* UNREFERENCED (no caller anywhere): sweeps the display-env origin to the right half
 * and back with two PutDispEnv calls -- a forced display refresh, likely debug. */
void func_800815B0(void) {
   SetDefDispEnv(&gGraphicsPtr->dispEnv, SCREEN_WIDTH, gGraphicsPtr->dispEnv.disp.y, SCREEN_WIDTH,
                 SCREEN_HEIGHT);
   PutDispEnv(&gGraphicsPtr->dispEnv);
   SetDefDispEnv(&gGraphicsPtr->dispEnv, 0, gGraphicsPtr->dispEnv.disp.y, SCREEN_WIDTH,
                 SCREEN_HEIGHT);
   PutDispEnv(&gGraphicsPtr->dispEnv);
}

#undef OBJF
#define OBJF 291
void Objf291_ChestImpact(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = sImpactAnimData_800ff4d4;
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
   }
}

#undef OBJF
#define OBJF 080
void Objf080_RomanFire_FX1(Object *obj) {
   Object *target;
   Object *obj_s1;
   s32 i;

   switch (obj->state) {
   case 0:
      target = Obj_GetUnused();
      target->functionIndex = OBJF_NOOP;
      target->x1.n = gMapCursorX * CV(1.0) + CV(0.5);
      target->z1.n = gMapCursorZ * CV(1.0) + CV(0.5);
      target->y1.n = GetTerrainElevation(gMapCursorZ, gMapCursorX);
      OBJ.target = target;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_FOCUS_CAMERA;
      obj_s1->d.objf026.target = target;
      obj_s1->d.objf026.type = 0;
      obj_s1->d.objf026.zoom = gCameraZoom.vz;
      OBJ.cam = obj_s1;

      obj->x1.n = target->x1.n;
      obj->z1.n = target->z1.n;
      obj->y1.n = target->y1.n;
      obj->y2.n = CV(0.375);
      obj->y3.n = CV(-0.03125);
      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.gfxIdx = GFX_TBD_15;
      obj_s1->d.sprite.animData = gSparkleAnimData_800ff38c;
      OBJ.sprite = obj_s1;
      obj->state2 = 8;
      obj->state++;
      break;

   case 2:
      if (--obj->state2 == 0) {
         obj->state2 = 1;
         obj->state++;
      }
      break;

   case 3:
      if (obj->state2 == 1) {
         obj_s1 = OBJ.cam;
         if (obj_s1->functionIndex == OBJF_FOCUS_CAMERA) {
            obj_s1->functionIndex = OBJF_NULL;
            obj->state2 = 0;
         }
      }

      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;
      //?
      gCameraPos.vy += ((obj->y1.n >> 3) - gCameraPos.vy) >> 3;
      gCameraPos.vy = obj->y1.n >> 3;
      obj_s1 = OBJ.sprite;
      obj_s1->x1.n = obj->x1.n;
      obj_s1->z1.n = obj->z1.n;
      obj_s1->y1.n = obj->y1.n;

      if (obj->y2.n < 0) {
         obj->state2 = 0x40;
         obj->state++;
      }

      if (obj->y1.n < GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.25)) {
         obj->state = 5;
         obj->state2 = 8;
      }

      UpdateObjAnimation(obj_s1);
      AddObjPrim6(gGraphicsPtr->ot, obj_s1, 0);
      gCameraRotation.vy += 16;
      gCameraRotation.vx += 4;
      break;

   case 4:
      for (i = 0; i < 8; i++) {
         obj_s1 = CreatePositionedObj(obj, OBJF_ROMAN_FIRE_FLAME);
         obj_s1->functionIndex = OBJF_ROMAN_FIRE_FLAME;
         // obj_s1->x2.n = 0x20 * rcos(i * 0x200 + obj->y1.n * 4) >> 12;
         obj_s1->x2.n = rcos(i * DEG(45) + obj->y1.n * 4) >> 7;
         obj_s1->z2.n = rsin(i * DEG(45) + obj->y1.n * 4) >> 7;
         obj_s1->y2.n = 0x20;
         obj_s1->y3.n = -8;
         obj_s1->d.sprite.animData = sExplosionAnimData_800ff3a8;
         obj_s1->d.sprite.boxIdx = 5;
      }
      obj->state = 3;
      gCameraRotation.vy += 16;
      gCameraRotation.vx += 4;
      gCameraZoom.vz += 4;
      break;

   case 5:
      gCameraRotation.vy += 16;
      gCameraRotation.vx += 4;
      gCameraZoom.vz += 4;
      if (--obj->state2 <= 0) {
         obj->state++;
      }
      break;

   case 6:
      obj_s1 = OBJ.sprite;
      target = OBJ.target;
      obj_s1->functionIndex = OBJF_NULL;
      target->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 081
void Objf081_RomanFire_Flame(Object *obj) {
   s32 elevation;

   obj->x1.n += obj->x2.n;
   obj->z1.n += obj->z2.n;
   obj->y1.n += obj->y2.n;
   obj->y2.n += obj->y3.n;

   if (obj->d.sprite.animData != NULL) {
      UpdateObjAnimation(obj);
   }

   AddObjPrim6(gGraphicsPtr->ot, obj, 0);

   elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
   if (obj->y1.n <= elevation) {
      obj->y1.n = elevation;
      obj->state++;
      obj->y2.n = -(obj->y2.n >> 2);

      switch (obj->state) {
      case 0:
      case 1:
      case 2:
         obj->d.sprite.animInitialized = 0;
         obj->d.sprite.animData = sSmokeAnimData_800ff438;
         break;
      case 3:
      case 4:
         break;
      case 5:
         obj->functionIndex = OBJF_NULL;
         break;
      }
   }
}

