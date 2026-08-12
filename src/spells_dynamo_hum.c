/* Dynamo Hum spell effects (Objf395 electric orb, Objf396, Objf394 driver) and Flame
 * Breath, the monster melee attack (Objf375/382). Dispatched via gSpellsEx (see
 * spells_casting_main.c); drivers own the gSignal3 completion handshake, children do not.
 * Objf397/398 (explosion-burst pair) are cut content -- reachable from no spell table,
 * event script, or code path -- suffixed _Unused, kept byte-exact. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

#undef OBJF
#define OBJF 395
void Objf395_DynamoHum_ElectricOrb(Object *obj) {
   extern s16 gExplosionAnimData_800ff3dc[26];
   static u8 cluts[5] = {CLUT_REDS, CLUT_BLUES, CLUT_GRAYS, CLUT_PURPLES, CLUT_GREENS};

   Object *parent;
   Object *ray;
   s32 halfSize;
   s32 i;

   parent = OBJ.parent;

   halfSize = ((OBJ.size - (0xa0 * rsin(OBJ.theta) >> 12)) >> 5) + 4;
   gQuad_800fe63c[0].vx = gQuad_800fe63c[2].vx = -halfSize;
   gQuad_800fe63c[1].vx = gQuad_800fe63c[3].vx = halfSize;
   gQuad_800fe63c[0].vy = gQuad_800fe63c[1].vy = -halfSize;
   gQuad_800fe63c[2].vy = gQuad_800fe63c[3].vy = halfSize;
   gQuad_800fe63c[2].vz = gQuad_800fe63c[3].vz = gQuad_800fe63c[0].vz = gQuad_800fe63c[1].vz = 0;

   obj->mem++;
   obj->mem %= 5;
   OBJ.clut = cluts[obj->mem];

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_EXPLOSION_1;
      OBJ.boxIdx = 7;
      OBJ.size = 0x100;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n = obj->x2.n + (obj->state3 * rcos(obj->state2) >> 12);
      obj->z1.n = obj->z2.n + (obj->state3 * rsin(obj->state2) >> 12);
      obj->y1.n = obj->y2.n;
      obj->state2 += 0x20;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      obj->state3 = OBJ.size + (0xa0 * rsin(OBJ.theta) >> 12);
      OBJ.theta += 0x80;
      OBJ.size -= 1;
      if (obj->state3 <= 8) {
         OBJ.animData = gExplosionAnimData_800ff3dc;
         obj->state = 2;
         obj->state3 = 0;
      }
      for (i = 0; i < 3; i++) {
         ray = CreatePositionedObj(obj, OBJF_OUTWARD_RAY);
         ray->x2.n = obj->x1.n;
         ray->z2.n = obj->z1.n;
         ray->y2.n = obj->y1.n;
      }
      if (parent->state == 99) {
         obj->state = 99;
      }
      break;

   case 2:
      if (parent->state == 99) {
         obj->state = 99;
         break;
      }
      if (OBJ.animFinished) {
         UpdateObjAnimation(obj);
         AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      }
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 396
void Objf396_DynamoHum_OrbElectricity(Object *obj) {
   extern s16 gLightningAnimData_800ff468[20];

   Object *link1;
   Object *link2;
   Object *electricity;
   Object **p;
   s32 i;
   s32 length;
   s32 dx, dy, dz;
   s16 sVar5;
   s32 rnd;
   VECTOR vector;
   SVECTOR normalized;

   switch (obj->state) {
   case 0:
      link1 = OBJ.links[0];
      link2 = OBJ.links[1];

      if (obj->state2 != 0) {
         for (i = 0, p = &OBJ.links[0]; i < 2; i++) {
            electricity = Obj_GetUnused();
            electricity->functionIndex = OBJF_DYNAMO_HUM_ORB_ELECTRICITY;
            electricity->d.objf396.links[0] = *p;
            electricity->d.objf396.links[1] = obj;
            if (obj->state2 == 1) {
               electricity->state3 = 1;
            } else {
               electricity->state2 = obj->state2 - 1;
            }
            electricity->x3.n = obj->x3.n;
            p++;
         }
      }

      OBJ.gfxIdx = GFX_LIGHTNING_1;
      OBJ.clut = CLUT_REDS;

      if (obj->state3 != 0) {
         OBJ.animData = gLightningAnimData_800ff468;
         obj->state3 = 0;
      }

      dx = (link1->x1.n - link2->x1.n);
      dz = (link1->z1.n - link2->z1.n);
      dy = (link1->y1.n - link2->y1.n);
      i = SquareRoot0(dx * dx + dy * dy + dz * dz);
      OBJ.length = i;
      OBJ.bowDir = obj->x3.n;

      if (obj->x3.n != 0) {
         obj->x3.n = 0;
         obj->z3.n = 0;
         obj->y3.n = -1;
      } else {
         obj->x3.n = 0;
         obj->z3.n = 0;
         obj->y3.n = 1;
      }

      obj->state++;

   // fallthrough
   case 1:
      link1 = OBJ.links[0];
      link2 = OBJ.links[1];

      if (link1->state == 99 || link2->state == 99) {
         obj->state = 99;
         break;
      }

      obj->x1.n = (link1->x1.n + link2->x1.n) / 2;
      obj->z1.n = (link1->z1.n + link2->z1.n) / 2;
      obj->y1.n = (link1->y1.n + link2->y1.n) / 2;
      obj->x1.n += rand() % 0x20 - 0x10;
      obj->z1.n += rand() % 0x20 - 0x10;
      obj->y1.n += rand() % 0x20 - 0x10;

      dx = (link1->x1.n - link2->x1.n);
      dz = (link1->z1.n - link2->z1.n);
      dy = (link1->y1.n - link2->y1.n);
      length = SquareRoot0(dx * dx + dy * dy + dz * dz);

      if (length < OBJ.length) {
         sVar5 = (OBJ.length - length) / 2;

         switch (OBJ.bowDir) {
         case 0:
            obj->y1.n += sVar5 * obj->y3.n;
            obj->x1.n += sVar5 * obj->x3.n;
            obj->z1.n += sVar5 * obj->z3.n;
            break;

         case 1:
            obj->y1.n += sVar5 * obj->y3.n;
            obj->x1.n += sVar5 * obj->x3.n;
            obj->z1.n += sVar5 * obj->z3.n;
            break;

         case 2:
         case 3:
            sVar5 = (OBJ.length - length) / 4;
            vector.vx = link2->x1.n - link1->x1.n;
            vector.vz = link2->z1.n - link1->z1.n;
            vector.vy = 0;
            VectorNormalS(&vector, &normalized);

            if (OBJ.bowDir == 2) {
               obj->x3.n = -(normalized.vz >> 11);
               obj->z3.n = normalized.vx >> 11;
               obj->y3.n = 0;
            } else {
               obj->x3.n = normalized.vz >> 11;
               obj->z3.n = -(normalized.vx >> 11);
               obj->y3.n = 0;
            }

            obj->y1.n += sVar5 * obj->y3.n;
            obj->x1.n += sVar5 * obj->x3.n;
            obj->z1.n += sVar5 * obj->z3.n;
            break;
         }
      }

      if (OBJ.animData != NULL) {
         UpdateObjAnimation(obj);
         p = &OBJ.links[0];
         for (i = 0; i < ARRAY_COUNT(OBJ.links); i++) {
            link1 = *p;
            OBJ.coords[0].x = OBJ.coords[1].x = link1->x1.n;
            OBJ.coords[2].x = OBJ.coords[3].x = obj->x1.n;
            OBJ.coords[0].z = OBJ.coords[1].z = link1->z1.n;
            OBJ.coords[2].z = OBJ.coords[3].z = obj->z1.n;
            OBJ.coords[1].y = link1->y1.n + CV(0.125);
            OBJ.coords[0].y = link1->y1.n - CV(0.125);
            OBJ.coords[3].y = obj->y1.n + CV(0.125);
            OBJ.coords[2].y = obj->y1.n - CV(0.125);
            AddObjPrim3(gGraphicsPtr->ot, obj);
            p++;
         }
      }

      if (link1->state == 2 || link2->state == 2) {
         obj->state = 2;
         obj->state2 = 0x100;
      }
      break;

   case 2:
      link1 = OBJ.links[0];
      link2 = OBJ.links[1];

      if (link1->state == 99 || link2->state == 99) {
         obj->state = 99;
         break;
      }

      if (link1->state != 2) {
         link1 = link2;
      }

      rnd = (rand() >> 2) % DEG(360);
      obj->x1.n = link1->x1.n + (obj->state2 * rcos(rnd) >> 12);
      obj->z1.n = link1->z1.n + (obj->state2 * rsin(rnd) >> 12);
      obj->y1.n = link1->y1.n;
      OBJ.coords[0].x = OBJ.coords[1].x = link1->x1.n;
      OBJ.coords[2].x = OBJ.coords[3].x = obj->x1.n;
      OBJ.coords[0].z = OBJ.coords[1].z = link1->z1.n;
      OBJ.coords[2].z = OBJ.coords[3].z = obj->z1.n;
      OBJ.coords[1].y = link1->y1.n + CV(0.125);
      OBJ.coords[0].y = link1->y1.n - CV(0.125);
      OBJ.coords[3].y = obj->y1.n + CV(0.125);
      OBJ.coords[2].y = obj->y1.n - CV(0.125);
      AddObjPrim3(gGraphicsPtr->ot, obj);

      obj->state2 += 4;
      if (link1->state == 2 || link2->state == 2) {
         obj->state = 2;
      }
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 394
void Objf394_DynamoHum_FX1(Object *obj) {
   Object *obj_s1;
   Object *orb1;
   Object *orb2;
   s32 rnd;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(1.0);

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_DYNAMO_HUM_ELECTRIC_ORB;
      OBJ.orb = obj_s1;
      obj_s1->x2.n = obj->x1.n;
      obj_s1->z2.n = obj->z1.n;
      obj_s1->y2.n = obj->y1.n;
      rnd = (rand() >> 2) % DEG(360);
      obj_s1->state2 = rnd;
      obj_s1->state3 = 0x100;
      obj_s1->d.objf395.parent = obj;
      orb1 = obj_s1;

      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_DYNAMO_HUM_ELECTRIC_ORB;
      obj_s1->x2.n = obj->x1.n;
      obj_s1->z2.n = obj->z1.n;
      obj_s1->y2.n = obj->y1.n;
      obj_s1->state2 = rnd + DEG(180);
      obj_s1->state3 = 0x100;
      obj_s1->d.objf395.parent = obj;
      orb2 = obj_s1;

      for (i = 0; i < 4; i++) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_DYNAMO_HUM_ORB_ELECTRICITY;
         obj_s1->d.objf396.links[0] = orb1;
         obj_s1->d.objf396.links[1] = orb2;
         obj_s1->state3 = 0;
         obj_s1->x3.n = i;
         obj_s1->state2 = 2;
      }

      obj->state3 = 0xc0;
      obj->state++;
      break;

   case 1:
      obj_s1 = OBJ.orb;

      switch (obj->state2) {
      case 0:
         if (obj->state3 == 0x20) {
            FadeOutScreen(1, 8);
            obj->state2++;
            obj->mem = 0;
         }
         break;
      case 1:
         break;
      case 2:
         break;
      }

      if (obj_s1->state == 2 && --obj->mem <= 0) {
         i = rand() % DEG(360);
         obj_s1 = CreatePositionedObj(obj, OBJF_DYNAMO_HUM_COLORED_BOLT);
         obj_s1->x1.n += CV(1.5) * rcos(i) >> 12;
         obj_s1->z1.n += CV(1.5) * rsin(i) >> 12;
         obj_s1->y1.n = GetTerrainElevation(obj_s1->z1.s.hi, obj_s1->x1.s.hi);
         obj->mem += rand() % 0x10;
      }
      if (--obj->state3 <= 0) {
         obj->state = 99;
         obj->state3 = 8;
         FadeInScreen(1, 0x80);
      }
      break;

   case 99:
      if (--obj->state3 <= 0) {
         gLightColor.r = 0x80;
         gLightColor.g = 0x80;
         gLightColor.b = 0x80;
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 398
void Objf398_ExplosionBurstParticle_Unused(Object *obj) {
   static s16 explosionAnimData[26] = {3, GFX_EXPLOSION_1,  1, GFX_EXPLOSION_2,  1, GFX_EXPLOSION_3,
                                       1, GFX_EXPLOSION_4,  1, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = explosionAnimData;
      if ((rand() >> 2) % 2 != 0) {
         obj->d.sprite.semiTrans = 2;
      }
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += (obj->x2.n - obj->x1.n) >> 4;
      obj->z1.n += (obj->z2.n - obj->z1.n) >> 4;
      obj->y1.n += (obj->y2.n - obj->y1.n) >> 4;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->d.sprite.animFinished) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 397
void Objf397_ExplosionBurst_Unused(Object *obj) {
   Object *obj_s2;
   s32 rnd;

   switch (obj->state) {
   case 0:
      gCameraZoom.vz = 512;
      SnapToUnit(obj);
      obj->d.sprite.clut = 3 + (rand() >> 2) % 3;
      obj->state3 = 0x80;
      obj->state++;

   // fallthrough
   case 1:
      gCameraRotation.vy += 4;
      if (obj->state3 % 3 == 0) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_EXPLOSION_BURST_PARTICLE_UNUSED;
         obj_s2->x1.n = obj->x1.n;
         obj_s2->z1.n = obj->z1.n;
         obj_s2->y1.n = obj->y1.n;
         rnd = (rand() >> 2) % DEG(360);
         obj_s2->x2.n = CV(0.5) * rcos(rnd) >> 12;
         obj_s2->z2.n = CV(0.5) * rsin(rnd) >> 12;
         rnd = (rand() >> 2) % 0x20;
         obj_s2->y2.n = (rand() % CV(1.0) + CV(2.5)) * rsin(rnd + 0x3f0) >> 12;
         obj_s2->d.sprite.clut = obj->d.sprite.clut;
         obj_s2->x2.n += obj->x1.n;
         obj_s2->z2.n += obj->z1.n;
         obj_s2->y2.n += obj->y1.n;
      }
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 375
void Objf375_FlameBreath_Particle(Object *obj) {
   static s16 animData[26] = {GFX_EXPLOSION_4,  2, 4, GFX_EXPLOSION_5,  2, 4, GFX_EXPLOSION_6, 2, 4,
                              GFX_EXPLOSION_7,  2, 4, GFX_EXPLOSION_8,  2, 3, GFX_EXPLOSION_9, 2, 3,
                              GFX_EXPLOSION_10, 2, 3, GFX_EXPLOSION_11, 2, 3, GFX_NULL,        0};
   static s16 animData_unused[20] = {3, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                     2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                     2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                     0, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = animData;
      obj->d.sprite.semiTrans = 0;
      obj->state++;

   // fallthrough
   case 1:
      UpdateMultisizeObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->x2.n += (obj->x3.n - obj->x2.n) >> 3;
      obj->z2.n += (obj->z3.n - obj->z2.n) >> 3;
      obj->y2.n += (obj->y3.n - obj->y2.n) >> 3;
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 382
void Objf382_FlameBreath(Object *obj) {
   static s16 thetas[4] = {DEG(90), DEG(0), DEG(270), DEG(180)};
   static s16 radii[4] = {CV(0.75), CV(0.75), CV(0.9375), CV(0.9375)};
   static s16 yOffsets[4] = {CV(0.625), CV(0.625), CV(0.125), CV(0.125)};

   s16 dir;
   Object *obj_s2;

   GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);

   dir = OBJ.direction + (gCameraRotation.vy & DEG(270));
   dir /= DEG(90);

   switch (obj->state) {
   case 0:
      if (++obj->state3 >= 24) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 1:
      SnapToUnit(obj);
      obj->y1.n += CV(0.5);
      OBJ.gfxIdx = GFX_SALAMANDER_S;
      OBJ.boxIdx = 3;
      if (OBJ.clut == CLUT_NULL) {
         OBJ.clut = CLUT_REDS;
      }
      obj_s2 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.theta = thetas[((obj_s2->d.sprite.direction & 0xfff) >> 10)];
      OBJ.unused_0x40 = (obj_s2->d.sprite.direction & 0xfff) >> 10;
      obj->x3.n = 40 * rcos(OBJ.theta) >> 12;
      obj->z3.n = 40 * rsin(OBJ.theta) >> 12;
      obj->y3.n = 0x10;
      obj->y2.n = obj->y1.n;
      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->x1.n += radii[dir] * rcos(OBJ.theta) >> 12;
      obj->z1.n += radii[dir] * rsin(OBJ.theta) >> 12;
      OBJ.yTheta = 0;
      obj->state3 = 0x30;
      obj->state++;

   // fallthrough
   case 2:
      obj_s2 = GetUnitSpriteAtPosition(obj->z2.s.hi, obj->x2.s.hi);
      obj->y1.n = obj_s2->y1.n + yOffsets[dir];
      obj->y3.n = 24 * rsin(0x300 * rsin(OBJ.yTheta) >> 12) >> 12;
      OBJ.yTheta += 0x20;
      if (obj->state3 % 2 == 0 && obj->state3 >= 16) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_FLAME_BREATH_PARTICLE;
         obj_s2->x1.n = obj->x1.n;
         obj_s2->z1.n = obj->z1.n;
         obj_s2->y1.n = obj->y1.n;
         obj_s2->d.sprite.clut = OBJ.clut;
         obj_s2->d.sprite.boxIdx = OBJ.boxIdx;
         obj_s2->x2.n = obj->x3.n + ((rand() >> 2) % 13 - 6);
         obj_s2->z2.n = obj->z3.n + ((rand() >> 2) % 13 - 6);
         obj_s2->y2.n = obj->y3.n;
         obj_s2->y3.n = obj_s2->y2.n >> 2;
         obj_s2->x3.n = obj_s2->x2.n >> 2;
         obj_s2->z3.n = obj_s2->z2.n >> 2;
      }
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

