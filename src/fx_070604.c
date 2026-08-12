/* Spell casting effects, part 6 (segment 0x70604): Dark Fire, Roman Fire, Mood Ring and
 * Dagger Storm, dispatched via gSpellsEx (see fx_060c38.c), plus shared explosion sprites.
 * Cut content herein (no spell/event/code dispatch): Objf082/083 (orbiting ember pair,
 * homing explosion spark) and Objf321 (expanding explosion ring -- its only spawner,
 * Objf734 in split_09a268.c, is itself unreachable). Objf_Unk_80080924 and the two
 * func_800* helpers are not referenced at all and keep their address names. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "audio.h"

// TODO: Figure out if this data should be split or continued. Some of this is accessed pretty far
// apart, so it will probably need to be global.
//
// From the ordering and xref locations, and the interspersing of probable static data, I'm guessing
// these were defined (probably as global) as they were needed, and then occasionally re-used later.
// For now, I'll continue defining them as static (function-scoped where reasonable), making global
// as needed, and not worry about precise placement until later when I begin finer file splits.

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

static s16 sExplosionAnimData_800ff520[26] = {
    3, GFX_EXPLOSION_1, 2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,  2, GFX_EXPLOSION_4,
    2, GFX_EXPLOSION_5, 2, GFX_EXPLOSION_6,  2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,
    2, GFX_EXPLOSION_9, 2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
    0, GFX_NULL};

static s16 sExplosionAnimData_800ff554[26] = {
    4, GFX_EXPLOSION_1, 1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,  1, GFX_EXPLOSION_4,
    1, GFX_EXPLOSION_5, 1, GFX_EXPLOSION_6,  1, GFX_EXPLOSION_7,  1, GFX_EXPLOSION_8,
    1, GFX_EXPLOSION_9, 1, GFX_EXPLOSION_10, 1, GFX_EXPLOSION_11, 1, GFX_NULL,
    0};

static s16 sLightningAnimData_800ff588[36] = {
    0, GFX_LIGHTNING_1, 1, GFX_LIGHTNING_2, 1, GFX_LIGHTNING_3, 1, GFX_LIGHTNING_4,
    1, GFX_LIGHTNING_5, 1, GFX_LIGHTNING_6, 1, GFX_LIGHTNING_7, 1, GFX_LIGHTNING_8,
    1, GFX_LIGHTNING_1, 1, GFX_LIGHTNING_2, 1, GFX_LIGHTNING_3, 1, GFX_LIGHTNING_4,
    1, GFX_LIGHTNING_5, 2, GFX_LIGHTNING_6, 2, GFX_LIGHTNING_7, 2, GFX_LIGHTNING_8,
    2, GFX_NULL,        0, GFX_NULL};

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
      obj_s1->functionIndex = OBJF_CAMERA_TBD_026;
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

      OBJ.todo_x24 = CV(3.0) + rand() % CV(3.0);
      OBJ.todo_x26 = rand() % DEG(360);
      OBJ.todo_x28 = rand() % DEG(360);
      OBJ.todo_x2a = rand() % DEG(360);
      OBJ.todo_x2c = 0x60 - rand() % 0xc1;
      OBJ.todo_x2e = 0x60 - rand() % 0xc1;
      OBJ.todo_x30 = 0x60 - rand() % 0xc1;

      obj->state++;

   // fallthrough
   case 1:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_COLOR_14;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.semiTrans = 1;

      OBJ.todo_x34 = OBJ.todo_x24 * OBJ.todo_x36 / 0x20;

      sprite->d.sprite.coords[0].x = obj->x1.n;
      sprite->d.sprite.coords[0].z = obj->z1.n;
      sprite->d.sprite.coords[0].y = obj->y1.n;
      sprite->d.sprite.coords[1].x = obj->x1.n;
      sprite->d.sprite.coords[1].z = obj->z1.n;
      sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].x = obj->x1.n + OBJ.todo_x34 * rcos(OBJ.todo_x26) / ONE;
      sprite->d.sprite.coords[2].z = obj->z1.n + OBJ.todo_x34 * rsin(OBJ.todo_x28) / ONE;
      sprite->d.sprite.coords[2].y = obj->y1.n + OBJ.todo_x34 * rsin(OBJ.todo_x2a) / ONE;
      sprite->d.sprite.coords[3].x = obj->x1.n + OBJ.todo_x34 * rcos(OBJ.todo_x26 + 8) / ONE;
      sprite->d.sprite.coords[3].z = obj->z1.n + OBJ.todo_x34 * rsin(OBJ.todo_x28 + 8) / ONE;
      sprite->d.sprite.coords[3].y = obj->y1.n + OBJ.todo_x34 * rsin(OBJ.todo_x2a + 8) / ONE;

      AddObjPrim4(gGraphicsPtr->ot, sprite);
      OBJ.todo_x26 += OBJ.todo_x2c;
      OBJ.todo_x28 += OBJ.todo_x2e;
      OBJ.todo_x2a += OBJ.todo_x30;

      sprite->functionIndex = OBJF_NULL;

      switch (obj->state2) {
      case 0:
         OBJ.todo_x36 += 2;
         if (OBJ.todo_x36 == 0x20) {
            obj->state2++;
         }
         break;
      case 1:
         OBJ.todo_x32++;
         if (OBJ.todo_x32 == 0x5c) {
            obj->state2++;
         }
         break;
      case 2:
         OBJ.todo_x36 -= 2;
         if (OBJ.todo_x36 <= 0) {
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
      obj_s1->functionIndex = OBJF_CAMERA_TBD_026;
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
         if (obj_s1->functionIndex == OBJF_CAMERA_TBD_026) {
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

#undef OBJF
#define OBJF 094
void Objf094_MoodRing_FX1(Object *obj) {
   Object *unitSprite;
   Object *obj_s1; // ring, cam
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_CAMERA_TBD_026;
      obj_s1->d.objf026.target = unitSprite;
      obj_s1->d.objf026.type = 0;
      obj_s1->d.objf026.zoom = 0x200;

      obj->x2.n = unitSprite->x1.n;
      obj->z2.n = unitSprite->z1.n;
      obj->y2.n = unitSprite->y1.n;

      p = (BVectorXZ *)gTargetCoords;
      for (i = 0; i < 8; i++) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_MOOD_RING_RING;
         obj_s1->x1.n = obj->x2.n + (CV(8.0) * rcos(i * DEG(45)) >> 12);
         obj_s1->z1.n = obj->z2.n + (CV(8.0) * rsin(i * DEG(45)) >> 12);
         obj_s1->y1.n = obj->y2.n + CV(1.0);
         obj_s1->d.objf095.theta = i * DEG(45);
         obj_s1->d.objf095.radius = CV(8.0);
         obj_s1->x2.n = obj->x2.n;
         obj_s1->z2.n = obj->z2.n;
         obj_s1->y2.n = obj->y2.n;
         obj_s1->d.objf095.parent = obj;
         obj_s1->state2 = 56;

         if (p->x != 0xff) {
            unitSprite = GetUnitSpriteAtPosition(p->z, p->x);
            obj_s1->x3.n = unitSprite->x1.n;
            obj_s1->y3.n = unitSprite->y1.n + CV(0.5);
            obj_s1->z3.n = unitSprite->z1.n;
            p++;
         } else {
            obj_s1->x3.n = obj_s1->x1.n;
            obj_s1->y3.n = obj_s1->y1.n + CV(0.5);
            obj_s1->z3.n = obj_s1->z1.n;
         }
      }

      obj->state2 = 56;
      obj->state++;
      break;

   case 1:
      if (--obj->state2 < 0) {
         obj->state2 = 3;
         obj->state++;
      }
      break;

   case 2:
      OBJ.launchingOutward = 1;
      if (--obj->state2 < 0) {
         obj->state++;
      }
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

void MoodRing_RenderRing(Object *obj, s16 halfSize, s16 theta, s16 clut) {
   SVECTOR vector;
   Object *top;
   Object *bottom;
   s32 a;

   if ((gCameraRotation.vy & 0xfff) < DEG(90) || (gCameraRotation.vy & 0xfff) > DEG(225)) {
      halfSize = -halfSize;
   }

   a = abs(halfSize) * 362 >> 8;
   func_800A9E78(&vector, a, theta, DEG(45));

   bottom = Obj_GetUnused();
   bottom->functionIndex = OBJF_NOOP;
   bottom->d.sprite.boxIdx = 3;
   bottom->d.sprite.gfxIdx = GFX_RING_BTM;
   bottom->d.sprite.clut = clut;

   top = Obj_GetUnused();
   top->functionIndex = OBJF_NOOP;
   top->d.sprite.boxIdx = 3;
   top->d.sprite.gfxIdx = GFX_RING_TOP;
   top->d.sprite.clut = clut;

   bottom->d.sprite.coords[0].x = top->d.sprite.coords[2].x = obj->x1.n - halfSize;
   bottom->d.sprite.coords[0].z = top->d.sprite.coords[2].z = obj->z1.n + halfSize;
   bottom->d.sprite.coords[0].y = top->d.sprite.coords[2].y = obj->y1.n;
   bottom->d.sprite.coords[1].x = top->d.sprite.coords[3].x = obj->x1.n + halfSize;
   bottom->d.sprite.coords[1].z = top->d.sprite.coords[3].z = obj->z1.n - halfSize;
   bottom->d.sprite.coords[1].y = top->d.sprite.coords[3].y = obj->y1.n;
   bottom->d.sprite.coords[2].x = bottom->d.sprite.coords[0].x + vector.vx;
   bottom->d.sprite.coords[2].z = bottom->d.sprite.coords[0].z + vector.vz;
   bottom->d.sprite.coords[2].y = bottom->d.sprite.coords[0].y + vector.vy;
   bottom->d.sprite.coords[3].x = bottom->d.sprite.coords[1].x + vector.vx;
   bottom->d.sprite.coords[3].z = bottom->d.sprite.coords[1].z + vector.vz;
   bottom->d.sprite.coords[3].y = bottom->d.sprite.coords[1].y + vector.vy;
   bottom->x1.n = bottom->d.sprite.coords[3].x;
   bottom->z1.n = bottom->d.sprite.coords[3].z;
   bottom->y1.n = bottom->d.sprite.coords[3].y;
   AddObjPrim3(gGraphicsPtr->ot, bottom, 0); //? Extra arg

   top->d.sprite.coords[0].x = top->d.sprite.coords[2].x - vector.vx;
   top->d.sprite.coords[0].z = top->d.sprite.coords[2].z - vector.vz;
   top->d.sprite.coords[0].y = top->d.sprite.coords[2].y - vector.vy;
   top->d.sprite.coords[1].x = top->d.sprite.coords[3].x - vector.vx;
   top->d.sprite.coords[1].z = top->d.sprite.coords[3].z - vector.vz;
   top->d.sprite.coords[1].y = top->d.sprite.coords[3].y - vector.vy;
   top->x1.n = top->d.sprite.coords[0].x;
   top->z1.n = top->d.sprite.coords[0].z;
   top->y1.n = top->d.sprite.coords[0].y;
   AddObjPrim3(gGraphicsPtr->ot, top, 0); //? Extra arg

   top->functionIndex = OBJF_NULL;
   bottom->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 095
void Objf095_MoodRing_Ring(Object *obj) {
   Object *parent;
   s16 a;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx_unused = GFX_RING; // Just guessing
      OBJ.boxIdx_unused = 3;        // for these fields
      OBJ.unused_0x44 = 0x40;
      obj->state2 = 0x200;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.theta += 0x40;
      OBJ.radius -= (OBJ.radius >> 4);
      a = obj->x2.n + (rcos(OBJ.theta) * OBJ.radius >> 12);
      obj->x1.n = a;
      a = obj->z2.n + (rsin(OBJ.theta) * OBJ.radius >> 12);
      obj->z1.n = a;
      MoodRing_RenderRing(obj, obj->state2, 0, 3 + obj->state3 % 3);
      obj->state2 -= (obj->state2 - 0x20) >> 4;
      obj->state3++;
      parent = OBJ.parent;
      if (parent->d.objf094.launchingOutward == 1) {
         obj->state3 = 0x10;
         obj->state++;
      }
      break;

   case 2:
      obj->state2 = 0x80;
      obj->x1.n -= (obj->x1.n - obj->x3.n) >> 2;
      obj->z1.n -= (obj->z1.n - obj->z3.n) >> 2;
      obj->y1.n -= (obj->y1.n - obj->y3.n) >> 2;
      MoodRing_RenderRing(obj, obj->state2, 0, 3 + obj->state3 % 3);
      if (--obj->state3 == 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 097
void Objf097_MoodRing_FX3(Object *obj) {
   OBJ.fatal = 1;
   obj->functionIndex = OBJF_MOOD_RING_FX2;
}

#undef OBJF
#define OBJF 096
void Objf096_MoodRing_FX2(Object *obj) {
   Object *targetSprite;

   switch (obj->state) {
   case 0:
      targetSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.targetSprite = targetSprite;
      SnapToUnit(obj);
      obj->y2.n = obj->y1.n;
      obj->state2 = 0x80;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.targetSprite;
      obj->y1.n = targetSprite->y1.n + CV(0.75);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->y1.n = targetSprite->y1.n + CV(0.5);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->y1.n = targetSprite->y1.n + CV(0.25);
      MoodRing_RenderRing(obj, obj->state2 + 0x60, 0, 3 + obj->state3 % 3);
      obj->state3++;
      obj->state2 -= 0x10;
      if (obj->state2 <= 0) {
         if (OBJ.fatal) {
            CreatePositionedObj(obj, OBJF_SLAY_FX3);
         } else {
            CreatePositionedObj(obj, OBJF_DAMAGE_FX2);
         }
         obj->state++;
      }
      break;

   case 2:
      obj->state2++;
      if (obj->state2 >= 0x20) {
         obj->state++;
      }
      break;

   case 3:
      obj->state++;

   // fallthrough
   case 4:
   case 5:
      obj->state++;
      break;

   case 6:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 090
void Objf090_DaggerStorm_FX2(Object *obj) {
   Object *obj_s0;
   Object *dagger;
   SVECTOR svector;
   VECTOR vector1, vector2;
   s32 i;
   s32 a, b, c;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->state3 = 0x20;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->state3 < 0) {
         obj_s0 = Obj_GetUnused();
         if (obj_s0 != NULL) {
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(228));
            obj_s0->functionIndex = OBJF_DISPLAY_DAMAGE_2;
            obj_s0->x1.s.hi = obj->x1.s.hi;
            obj_s0->z1.s.hi = obj->z1.s.hi;
         }
         obj->state3 = 0x20;
         obj->state++;
      }

      if (obj->state3 % 8 == 0) {
         for (i = 0; i < 16; i++) {
            a = OBJ.todo_x24 = rand() % 0x400;
            b = OBJ.todo_x26 = rand() % 0x1000;
            c = OBJ.todo_x28 = 0x200 + rand() % 0x200;

            dagger = Obj_GetUnused();
            if (dagger == NULL) {
               return;
            }
            dagger->functionIndex = OBJF_DAGGER_STORM_DAGGER;
            if (obj->state3 >= 25) {
               dagger->state2 = 0;
            } else {
               dagger->state2 = 2;
            }
            dagger->d.objf091.todo_x28 = 0xd4;
            dagger->x2.n = obj->x1.n;
            dagger->y2.n = obj->y1.n + CV(0.5);
            dagger->z2.n = obj->z1.n;
            dagger->d.objf091.todo_x24 = a;
            dagger->d.objf091.todo_x26 = b;
            dagger->d.objf091.todo_x28 = c;
            func_800A9E78(&svector, OBJ.todo_x28, OBJ.todo_x24, OBJ.todo_x26);
            dagger->x1.n = obj->x1.n + svector.vx;
            dagger->y1.n = obj->y1.n + svector.vy;
            dagger->z1.n = obj->z1.n + svector.vz;
            vector1.vx = dagger->x2.n - dagger->x1.n;
            vector1.vy = dagger->y2.n - dagger->y1.n;
            vector1.vz = dagger->z2.n - dagger->z1.n;
            VectorNormalS(&vector1, &svector);
            dagger->x3.n = svector.vx * 0x60 >> 12;
            dagger->y3.n = svector.vy * 0x60 >> 12;
            dagger->z3.n = svector.vz * 0x60 >> 12;
            vector1.vx *= vector1.vx;
            vector1.vy *= vector1.vy;
            vector1.vz *= vector1.vz;
            vector2.vx = dagger->x3.n * dagger->x3.n;
            vector2.vy = dagger->y3.n * dagger->y3.n;
            vector2.vz = dagger->z3.n * dagger->z3.n;
            dagger->state3 = csqrt(vector1.vx + vector1.vy + vector1.vz) /
                             csqrt(vector2.vx + vector2.vy + vector2.vz);
            dagger->x3.n = (dagger->x2.n - dagger->x1.n) / dagger->state3;
            dagger->y3.n = (dagger->y2.n - dagger->y1.n) / dagger->state3;
            dagger->z3.n = (dagger->z2.n - dagger->z1.n) / dagger->state3;
            dagger->d.objf091.positions[1].y = 0;
            dagger->d.objf091.positions[1].x = 0;
            dagger->d.objf091.positions[1].z = 0;
            func_800A9E78(&svector, OBJ.todo_x28, OBJ.todo_x24, OBJ.todo_x26 + 0x40);
            dagger->d.objf091.positions[3].y = obj->y1.n + svector.vy - dagger->y1.n;
            dagger->d.objf091.positions[3].x = obj->x1.n + svector.vx - dagger->x1.n;
            dagger->d.objf091.positions[3].z = obj->z1.n + svector.vz - dagger->z1.n;
            func_800A9E78(&svector, OBJ.todo_x28 + 0xa0, OBJ.todo_x24, OBJ.todo_x26);
            dagger->d.objf091.positions[0].y = obj->y1.n + svector.vy - dagger->y1.n;
            dagger->d.objf091.positions[0].x = obj->x1.n + svector.vx - dagger->x1.n;
            dagger->d.objf091.positions[0].z = obj->z1.n + svector.vz - dagger->z1.n;
            func_800A9E78(&svector, OBJ.todo_x28 + 0xa0, OBJ.todo_x24, OBJ.todo_x26 + 0x40);
            dagger->d.objf091.positions[2].y = obj->y1.n + svector.vy - dagger->y1.n;
            dagger->d.objf091.positions[2].x = obj->x1.n + svector.vx - dagger->x1.n;
            dagger->d.objf091.positions[2].z = obj->z1.n + svector.vz - dagger->z1.n;
         }
      }
      break;

   case 2:
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 3:
      if (OBJ.fatal) {
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(232));
         obj_s0 = Obj_GetUnused();
         if (obj_s0 != NULL) {
            obj_s0->functionIndex = OBJF_STRETCH_WARP_SPRITE;
            obj_s0->x1.n = obj->x1.n;
            obj_s0->z1.n = obj->z1.n;
            obj_s0->y1.n = obj->y1.n;
            obj->state3 = 0x20;
            obj->state++;
         }
      } else {
         obj->state++;
      }
      break;

   case 4:
      if (--obj->state3 < 0) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

/* UNREFERENCED (no caller, not in gObjFunctionPointers[]): draw-for-state3-frames-
 * then-die tick; reads as the predecessor of Objf337_DaggerStorm_BloodSplatter. */
void func_80082E48(Object *obj) {
   if (--obj->state3 != 0) {
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
   } else {
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 337
void Objf337_DaggerStorm_BloodSplatter(Object *obj) {
   s32 elevation;

   switch (obj->state) {
   case 0:
      obj->state3 = 0x20;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->y2.n -= 4;

      elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      if (elevation > obj->y1.n) {
         obj->x2.n = 0;
         obj->z2.n = 0;
         obj->y1.n = elevation;
      }

      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 091
void Objf091_DaggerStorm_Dagger(Object *obj) {
   Object *obj_a1;

   switch (obj->state) {
   case 0:
      obj_a1 = Obj_GetUnused();
      if (obj_a1 == NULL) {
         break;
      }
      obj_a1->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_a1;
      obj_a1->d.sprite.gfxIdx = GFX_DAGGER;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->state2 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->x1.n += obj->x3.n;
      obj->z1.n += obj->z3.n;
      obj->y1.n += obj->y3.n;

      if (abs(obj->x2.n - obj->x1.n) <= CV(0.5) && abs(obj->y2.n - obj->y1.n) <= CV(0.5) &&
          abs(obj->z2.n - obj->z1.n) <= CV(0.5)) {

         obj_a1 = Obj_GetUnused();
         if (obj_a1 != NULL) {
            obj_a1->functionIndex = OBJF_BOUNCE_ZOOM;
            obj_a1->d.objf024.soft = 1;
         }

         if (rand() % 8 == 0) {
            obj_a1 = Obj_GetUnused();
            if (obj_a1 != NULL) {
               obj_a1->functionIndex = OBJF_DAGGER_STORM_BLOOD_SPLATTER;
               obj_a1->x1.n = obj->x1.n;
               obj_a1->z1.n = obj->z1.n;
               obj_a1->y1.n = obj->y1.n;
               obj_a1->x2.n = (obj->x1.n - obj->x2.n) >> 2;
               obj_a1->z2.n = (obj->z1.n - obj->z2.n) >> 2;
               obj_a1->y2.n = (obj->y1.n - obj->y2.n) >> 1;
               obj_a1->d.sprite.gfxIdx = GFX_EXPLOSION_11;
               obj_a1->d.sprite.boxIdx = 5;
               obj_a1->d.sprite.clut = CLUT_REDS;
            }
         }

         obj_a1 = GetUnitSpriteAtPosition(obj->z2.s.hi, obj->x2.s.hi);
         OBJ.targetSprite = obj_a1;

         obj->x2.n = obj->x1.n - obj->x2.n;
         obj->z2.n = obj->z1.n - obj->z2.n;
         obj->y2.n = obj->y1.n - obj->y2.n;

         obj->state2 = 2;
         obj->state++;
      }

      break;

   case 3:
      obj_a1 = OBJ.targetSprite;
      obj->x1.n = obj_a1->x1.n + obj->x2.n;
      obj->z1.n = obj_a1->z1.n + obj->z2.n;
      obj->y1.n = obj_a1->y1.n + obj->y2.n;

      if (--obj->state2 > 0) {
         obj->x1.n -= obj->x3.n;
         obj->z1.n -= obj->z3.n;
         obj->y1.n -= obj->y3.n;
      }
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 4:
      obj_a1 = OBJ.sprite;
      obj_a1->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }

   if (obj->functionIndex != OBJF_NULL) {
      obj_a1 = OBJ.sprite;
      obj_a1->d.sprite.coords[0].x = OBJ.positions[0].x + obj->x1.n;
      obj_a1->d.sprite.coords[0].y = OBJ.positions[0].y + obj->y1.n;
      obj_a1->d.sprite.coords[0].z = OBJ.positions[0].z + obj->z1.n;
      obj_a1->d.sprite.coords[1].x = OBJ.positions[1].x + obj->x1.n;
      obj_a1->d.sprite.coords[1].y = OBJ.positions[1].y + obj->y1.n;
      obj_a1->d.sprite.coords[1].z = OBJ.positions[1].z + obj->z1.n;
      obj_a1->d.sprite.coords[2].x = OBJ.positions[2].x + obj->x1.n;
      obj_a1->d.sprite.coords[2].y = OBJ.positions[2].y + obj->y1.n;
      obj_a1->d.sprite.coords[2].z = OBJ.positions[2].z + obj->z1.n;
      obj_a1->d.sprite.coords[3].x = OBJ.positions[3].x + obj->x1.n;
      obj_a1->d.sprite.coords[3].y = OBJ.positions[3].y + obj->y1.n;
      obj_a1->d.sprite.coords[3].z = OBJ.positions[3].z + obj->z1.n;
      obj_a1->x1.n = obj->x1.n + (OBJ.positions[2].x << 2);
      obj_a1->z1.n = obj->z1.n + (OBJ.positions[2].z << 2);
      obj_a1->y1.n = obj->y1.n + (OBJ.positions[2].y << 2);
      obj_a1->d.sprite.otOfs = -16;
      AddObjPrim5(gGraphicsPtr->ot, obj_a1);
   }
}

void Objf093_DaggerStorm_FX3(Object *obj) {
   obj->functionIndex = OBJF_DAGGER_STORM_FX2;
   obj->d.objf090.fatal = 1;
}

void Objf092_DaggerStorm_FX1(Object *obj) {
   obj->functionIndex = OBJF_NULL;
   gSignal3 = 1;
}

#undef OBJF
#define OBJF 082
void Objf082_OrbitingEmberPair_Unused(Object *obj) {
   Object *obj_s3;
   Object *sprite;
   SVECTOR vector;

   switch (obj->state) {
   case 0:
      obj_s3 = OBJ.todo_x58;
      obj->state3 = obj_s3->functionIndex;

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;

      OBJ.radius = CV(0.25);
      OBJ.theta1 = DEG(90);
      OBJ.theta2 = DEG(90);
      OBJ.theta3 = DEG(90);
      OBJ.theta4 = DEG(90);

      obj->state2 = 0x100;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      obj_s3 = OBJ.todo_x58;

      if (obj_s3->functionIndex != obj->state3) {
         obj->state++;
      } else {
         sprite->d.sprite.gfxIdx = GFX_EXPLOSION_1;
         sprite->d.sprite.boxIdx = 5;

         vector.vx = (OBJ.radius * rcos(OBJ.theta2) >> 12);
         vector.vz = (OBJ.radius * rsin(OBJ.theta2) >> 12);
         sprite->x1.n = obj_s3->x1.n + vector.vx;
         sprite->y1.n = obj_s3->y1.n + vector.vz;
         sprite->z1.n = obj_s3->z1.n + vector.vz;
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

         vector.vx = (OBJ.radius * rcos(OBJ.theta4) >> 12);
         vector.vz = (OBJ.radius * rsin(OBJ.theta4) >> 12);
         sprite->x1.n = obj_s3->x1.n + vector.vx;
         sprite->y1.n = obj_s3->y1.n - vector.vz;
         sprite->z1.n = obj_s3->z1.n + vector.vz;

         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         OBJ.theta1 = (OBJ.theta1 + DEG(11.25)) & 0xfff;
         OBJ.theta2 = (OBJ.theta2 + DEG(11.25)) & 0xfff;
         OBJ.theta3 = (OBJ.theta3 - DEG(11.25)) & 0xfff;
         OBJ.theta4 = (OBJ.theta4 - DEG(11.25)) & 0xfff;
      }
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void Objf083_HomingExplosionSpark_Unused(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_COLOR_15;
      obj->d.sprite.boxIdx = 5;
      obj->state++;

   // fallthrough
   case 1:
      obj->x2.n = (obj->x3.n - obj->x1.n) >> 2;
      obj->y2.n = (obj->y3.n - obj->y1.n) >> 2;
      obj->z2.n = (obj->z3.n - obj->z1.n) >> 2;
      obj->x1.n += obj->x2.n;
      obj->y1.n += obj->y2.n;
      obj->z1.n += obj->z2.n;

      obj->d.sprite.gfxIdx = GFX_EXPLOSION_1 + ((0x20 - obj->state2) >> 4);
      obj->d.sprite.boxIdx = 5 - (obj->state2 >> 4);
      obj->d.sprite.clut = 3 + (obj->state2 % 2);

      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }

   if (obj->functionIndex != OBJF_NULL) {
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
   }
}

void Objf309_Explosion(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      obj->d.sprite.boxIdx = 5;
      if (obj->d.sprite.animData == NULL) {
         obj->d.sprite.animData = sExplosionAnimData_800ff520;
      } else {
         obj->d.sprite.animData = sExplosionAnimData_800ff554;
      }
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
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 321
/* 8 counter-rotating explosion sprites on a linearly expanding ring. Only spawner is
 * Objf734 (split_09a268.c) -- itself dispatched by nothing in retail, so this whole
 * flaming-rock-impact chain is cut content. */
void Objf321_ExpandingExplosionRing_Unused(Object *obj) {
   Object *sprite;
   s32 i;
   s16 *p;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = sExplosionAnimData_800ff520;
      sprite->d.sprite.clut = CLUT_REDS;
      sprite->d.sprite.boxIdx = 5;
      OBJ.sprite = sprite;
      OBJ.thetas[3] = DEG(90);
      OBJ.thetas[2] = DEG(90);
      OBJ.thetas[5] = DEG(180);
      OBJ.thetas[4] = DEG(180);
      OBJ.thetas[7] = DEG(270);
      OBJ.thetas[6] = DEG(270);
      OBJ.radius = 0;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      UpdateObjAnimation(sprite);
      p = &OBJ.thetas[0];
      for (i = 0; i < 8; i++) {
         sprite->z1.n = obj->z1.n + (OBJ.radius * rsin(*p) >> 12);
         sprite->x1.n = obj->x1.n + (OBJ.radius * rcos(*p) >> 12);
         sprite->y1.n = obj->y1.n;
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
         p++;
      }
      p = &OBJ.thetas[0];
      for (i = 0; i < 4; i++) {
         p[0] += 0x300;
         p[1] -= 0x300;
         p += 2;
      }
      OBJ.radius += 24;
      if (sprite->d.sprite.animFinished) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}
