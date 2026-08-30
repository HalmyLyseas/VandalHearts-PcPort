/* Dagger Storm spell effects (Objf090/337/091/093/092 and a rain helper), the generic explosion
 * burst Objf309, and strays kept by address contiguity. Dispatched via gSpellsEx
 * (docs/decomp/spell-fx-dispatch.md); _Unused handlers are cut content, kept byte-exact. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "audio.h"

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
            a = OBJ.theta1 = rand() % 0x400;
            b = OBJ.theta2 = rand() % 0x1000;
            c = OBJ.radius = 0x200 + rand() % 0x200;

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
            dagger->d.objf091.radius = 0xd4;
            dagger->x2.n = obj->x1.n;
            dagger->y2.n = obj->y1.n + CV(0.5);
            dagger->z2.n = obj->z1.n;
            dagger->d.objf091.theta1 = a;
            dagger->d.objf091.theta2 = b;
            dagger->d.objf091.radius = c;
            SphericalToVector(&svector, OBJ.radius, OBJ.theta1, OBJ.theta2);
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
            SphericalToVector(&svector, OBJ.radius, OBJ.theta1, OBJ.theta2 + 0x40);
            dagger->d.objf091.positions[3].y = obj->y1.n + svector.vy - dagger->y1.n;
            dagger->d.objf091.positions[3].x = obj->x1.n + svector.vx - dagger->x1.n;
            dagger->d.objf091.positions[3].z = obj->z1.n + svector.vz - dagger->z1.n;
            SphericalToVector(&svector, OBJ.radius + 0xa0, OBJ.theta1, OBJ.theta2);
            dagger->d.objf091.positions[0].y = obj->y1.n + svector.vy - dagger->y1.n;
            dagger->d.objf091.positions[0].x = obj->x1.n + svector.vx - dagger->x1.n;
            dagger->d.objf091.positions[0].z = obj->z1.n + svector.vz - dagger->z1.n;
            SphericalToVector(&svector, OBJ.radius + 0xa0, OBJ.theta1, OBJ.theta2 + 0x40);
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
      obj_s3 = OBJ.anchor;
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
      obj_s3 = OBJ.anchor;

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
 * Objf734 (events/fx_scenes.c) -- itself dispatched by nothing in retail, so this whole
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
