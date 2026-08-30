/* Avalanche FX2/FX3 (Objf317_338 rockfall, Objf084 dust cloud), Roman Fire FX2/FX3
 * (Objf344_345) and the Objf339_349 rubble spawner, all dispatched via gSpellsEx
 * (docs/decomp/spell-fx-dispatch.md); the other pieces of both spells sit in sibling files. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

/* UNREFERENCED zero-arg stub (wrong signature for the function table) -- most likely a
 * deleted handler's remnant, kept for byte/ordering exactness. */
void Noop_8008bca8() {}

void Objf274_Noop(Object *obj) {}

#undef OBJF
#define OBJF 344
void Objf344_345_RomanFire_FX2_FX3(Object *obj) {
   OBJ.clut = CLUT_REDS;
   if (obj->functionIndex == OBJF_ROMAN_FIRE_FX3) {
      obj->functionIndex = OBJF_ENGULF_EXPLOSION_SLAY;
   } else if (obj->functionIndex == OBJF_ROMAN_FIRE_FX2) {
      obj->functionIndex = OBJF_ENGULF_EXPLOSION_DAMAGE;
   } else {
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
   }
}

#undef OBJF
#define OBJF 339
void Objf339_349_Rubble(Object *obj) {
   static s16 rockAnimData_Fast[12] = {7, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                       2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};
   static s16 rockAnimData_Slow[12] = {7, GFX_ROCK_1, 3, GFX_ROCK_2, 3, GFX_ROCK_3,
                                       3, GFX_ROCK_4, 3, GFX_NULL,   1, GFX_NULL};

   s32 halfSize;
   s32 rnd1, rnd2, rnd3;
   s16 elevation;
   SVECTOR vector;

   halfSize = obj->d.sprite.coords[0].x;
   gQuad_800fe63c[0].vx = -halfSize;
   gQuad_800fe63c[0].vy = -halfSize;
   gQuad_800fe63c[1].vx = halfSize;
   gQuad_800fe63c[1].vy = -halfSize;
   gQuad_800fe63c[2].vx = -halfSize;
   gQuad_800fe63c[2].vy = halfSize;
   gQuad_800fe63c[3].vx = halfSize;
   gQuad_800fe63c[3].vy = halfSize;

   switch (obj->state) {
   case 0:
      gQuad_800fe63c[0].vx = -2;
      gQuad_800fe63c[0].vy = -2;
      gQuad_800fe63c[1].vx = 2;
      gQuad_800fe63c[1].vy = -2;
      gQuad_800fe63c[2].vx = -2;
      gQuad_800fe63c[2].vy = 2;
      gQuad_800fe63c[3].vx = 2;
      gQuad_800fe63c[3].vy = 2;

      switch (obj->functionIndex) {
      case OBJF_AVALANCHE_RUBBLE:
         obj->d.sprite.gfxIdx = GFX_ROCK_1;
         obj->d.sprite.boxIdx = 7;
         obj->d.sprite.coords[0].x = 1 + (rand() >> 2) % 8;
         rnd1 = rand() % DEG(360);
         if ((rnd1 >> 2) % 2 != 0) {
            obj->d.sprite.animData = rockAnimData_Fast;
         } else {
            obj->d.sprite.animData = rockAnimData_Slow;
         }
         rnd2 = 0x100 + (0x20 * rsin(rand() % DEG(360)) >> 12);
         rnd3 = 0x180 + (0x80 * rsin(rand() % DEG(360)) >> 12);
         SphericalToVector(&vector, rnd2, rnd3, rnd1);
         obj->x2.n = obj->x1.n + vector.vx;
         obj->z2.n = obj->z1.n + vector.vz;
         obj->y2.n = obj->y1.n + vector.vy;
         obj->x3.n = (obj->x2.n - obj->x1.n) / 2;
         obj->z3.n = (obj->z2.n - obj->z1.n) / 2;
         obj->y3.n = (obj->y2.n - obj->y1.n) / 2;
         obj->y2.n = 0;
         obj->state2 = 0x20;
         AddObjPrim6(gGraphicsPtr->ot, obj, 0);
         obj->state++;
         break;

      case OBJF_RUBBLE:
         obj->d.sprite.gfxIdx = GFX_ROCK_1;
         obj->d.sprite.boxIdx = 7;
         obj->d.sprite.coords[0].x = 1 + (rand() >> 2) % 5;
         rnd1 = rand() % DEG(360);
         if ((rnd1 >> 2) % 2 != 0) {
            obj->d.sprite.animData = rockAnimData_Fast;
         } else {
            obj->d.sprite.animData = rockAnimData_Slow;
         }
         rnd2 = 0x20 + (0x20 * rsin(rand() % DEG(360)) >> 12);
         rnd3 = 0x300 + (0x80 * rsin(rand() % DEG(360)) >> 12);
         SphericalToVector(&vector, rnd2, rnd3, rnd1);
         obj->x2.n = obj->x1.n + vector.vx;
         obj->z2.n = obj->z1.n + vector.vz;
         obj->y2.n = obj->y1.n + vector.vy;
         obj->x3.n = (obj->x2.n - obj->x1.n) / 2;
         obj->z3.n = (obj->z2.n - obj->z1.n) / 2;
         obj->y3.n = (obj->y2.n - obj->y1.n) * 2;
         //?
         obj->y2.n = ~((5 - obj->d.sprite.coords[0].x) / 2) * 8;
         // obj->y2.n = (obj->d.sprite.coords[0].x / 2 - 3) * 8;
         obj->state2 = 0x60;
         AddObjPrim6(gGraphicsPtr->ot, obj, 0);
         obj->state++;
         break;
      }

      break;

   case 1:
      obj->x1.n = obj->x1.n + obj->x3.n;
      obj->z1.n = obj->z1.n + obj->z3.n;
      obj->y1.n = obj->y1.n + obj->y3.n;
      obj->y3.n = obj->y3.n + obj->y2.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      if (obj->y1.n < elevation) {
         obj->y1.n = elevation;
         obj->y3.n = -(obj->y3.n / 2);
         if (OBJ_TERRAIN(obj).s.terrain == TERRAIN_WATER) {
            CreatePositionedObj(obj, OBJF_RIPPLE);
            obj->functionIndex = OBJF_NULL;
            return;
         }
      }
      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 2:
      break;
   }
}

#undef OBJF
#define OBJF 317
void Objf317_338_Avalanche_FX2_FX3(Object *obj) {
   Object *obj_v1;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(0.5);

      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_UNIT_STRUCK;
      obj_v1->x1.n = obj->x1.n;
      obj_v1->z1.n = obj->z1.n;
      obj_v1->y1.n = obj->y1.n;

      OBJ.availableSlots = Obj_CountUnused();
      OBJ.rubbleAmount = (OBJ.availableSlots - 80) / 8;
      obj->state++;
      obj->state3 = 1;

   // fallthrough
   case 1:
      if (--obj->state3 <= 0) {
         obj->state3 = 4;
         obj->state++;
         obj->state2 = 1;
      }
      break;

   case 2:
      if (--obj->state2 <= 0) {
         for (i = 0; i < OBJ.rubbleAmount; i++) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_AVALANCHE_RUBBLE;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->z1.n = obj->z1.n;
            obj_v1->y1.n = obj->y1.n;
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_RUBBLE;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->z1.n = obj->z1.n;
            obj_v1->y1.n = obj->y1.n;
         }
         for (i = 0; i < 8; i++) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_AVALANCHE_DUST_CLOUD;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->z1.n = obj->z1.n;
         }
         if (--obj->state3 <= 0) {
            obj->state++;
            obj->state3 = 0x40;
         } else {
            obj->state2 = 1;
         }
      }
      break;

   case 3:
      if (--obj->state3 <= 0) {
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_DISPLAY_DAMAGE_2;
         obj_v1->x1.s.hi = obj->x1.s.hi;
         obj_v1->z1.s.hi = obj->z1.s.hi;
         obj->state++;
         obj->state3 = 0x20;
      }
      break;

   case 4:
      if (--obj->state3 <= 0) {
         if (obj->functionIndex == OBJF_AVALANCHE_FX3) {
            obj->state++;
         } else {
            obj->functionIndex = OBJF_NULL;
            gSignal3 = 1;
         }
      }
      break;

   case 5:
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(232));
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_STRETCH_WARP_SPRITE;
      obj_v1->x1.n = obj->x1.n;
      obj_v1->z1.n = obj->z1.n;
      obj_v1->y1.n = obj->y1.n;
      obj->state3 = 0x20;
      obj->state++;
      break;

   case 6:
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 084
void Objf084_Avalanche_DustCloud(Object *obj) {
   extern s16 gSmokeAnimData_800ff688[24];
   s16 randomAngle;

   switch (obj->state) {
   case 0:
      obj->state2 = 0x10;
      obj->d.sprite.gfxIdx = GFX_PUFF_1;
      obj->d.sprite.animData = gSmokeAnimData_800ff688;
      obj->d.sprite.boxIdx = 3 + (rand() >> 2) % 3;
      randomAngle = (rand() >> 2) & 0xfff;
      obj->x2.n = 0x20 * rsin(randomAngle) >> 12;
      obj->z2.n = 0x20 * rcos(randomAngle) >> 12;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 1);
      if (--obj->state2 <= 0) {
         obj->state++;
      }
      break;

   case 2:
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->y2.n = 0x20;
      obj->y3.n = 4;
      obj->state++;
      obj->state2 = 0x30;

   // fallthrough
   case 3:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

