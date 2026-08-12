/* The treasure/item-reveal cluster: Objf292 blue sparkle, Objf290_294_761 the reveal
 * driver, Objf295 the item icon, Objf385 the mimic, plus Map32's smokestacks
 * (Objf301/300, strays kept by address contiguity). Spawned from the battle reward flow
 * and event scripts. Handlers reachable from no spell table, no event script and no code
 * path are cut content, suffixed _Unused. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

#undef OBJF
#define OBJF 292
void Objf292_BlueItemSparkle(Object *obj) {
   static s16 sparkleAnimData[14] = {7, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};
   Object *sprite;
   Quad quad;
   s32 i;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_LIT_SPHERE_8;
      sprite->d.sprite.boxIdx = 7;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->d.sprite.semiTrans = 4;
      sprite->d.sprite.animData = sparkleAnimData;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      OBJ.sprite = sprite;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe63c[i];
      }
      for (i = 0; i < obj->state2; i++) {
         gQuad_800fe63c[0].vx = -i * 2;
         gQuad_800fe63c[1].vx = i * 2;
         gQuad_800fe63c[2].vx = -i * 2;
         gQuad_800fe63c[3].vx = i * 2;
         gQuad_800fe63c[0].vy = -i * 2;
         gQuad_800fe63c[1].vy = -i * 2;
         gQuad_800fe63c[2].vy = i * 2;
         gQuad_800fe63c[3].vy = i * 2;
         UpdateObjAnimation(sprite);
         AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      }
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i] = quad[i];
      }
      if (++obj->state3 > 0x10) {
         obj->state++;
      }
      obj->state2 = abs(8 * rsin(obj->state3 * 0x10) >> 12) + 1;
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 293
void Objf293_Sparkle_Unused(Object *obj) {
   static s16 sparkleAnimData[14] = {5, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = sparkleAnimData;
      if (obj->state2 == 0) {
         obj->state2 = 0x20;
      }
      obj->state++;

   // fallthrough
   case 1:
      UpdateObjAnimation(obj);
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 290
void Objf290_294_761_RevealItem(Object *obj) {
   Object *obj_s5;
   s16 elevation;
   s32 halfSize;
   s32 i;
   Quad quad;

   switch (obj->state) {
   case 0:
      obj->y1.n += CV(0.5);
      obj->state3 = 0;
      OBJ.timer = 1;
      OBJ.theta = DEG(90) * 10;
      if (obj->functionIndex == OBJF_REVEAL_CHEST_ITEM) {
         CreatePositionedObj(obj, OBJF_CHEST_IMPACT);
      }
      obj->state++;

   // fallthrough
   case 1:
      if (++obj->state3 >= 0x10 || obj->functionIndex == OBJF_REVEAL_USED_ITEM) {
         obj->state3 = 0;
         obj->state2 = 0;
         obj->mem = 1;
         obj->state++;
         obj->y2.n = CV(0.25);
         if (obj->functionIndex == OBJF_REVEAL_HIDDEN_ITEM ||
             obj->functionIndex == OBJF_REVEAL_USED_ITEM) {
            obj->y1.n += CV(0.25);
         }
         obj->y3.n = -6;
      }
      break;

   case 2:
      if (--obj->mem <= 0) {
         obj_s5 = CreatePositionedObj(obj, OBJF_BLUE_ITEM_SPARKLE);
         obj_s5->x1.n += rand() % CV(0.75) - CV(0.375);
         obj_s5->z1.n += rand() % CV(0.75) - CV(0.375);
         obj_s5->y1.n += rand() % CV(0.75) - CV(0.375);
         obj->mem = rand() % 8 + 4;
      }

      switch (obj->state2) {
      case 0:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
         if (obj->functionIndex == OBJF_REVEAL_HIDDEN_ITEM ||
             obj->functionIndex == OBJF_REVEAL_USED_ITEM) {
            elevation += CV(0.75);
         }
         if (obj->y2.n < 0 && obj->y1.n < elevation + CV(1.0)) {
            OBJ.theta = 0;
            obj->y1.n = elevation + CV(1.0);
            obj->state2++;
         }
         if (--obj->state3 <= 0 && obj->functionIndex != OBJF_REVEAL_USED_ITEM) {
            for (i = 0; i < 8; i++) {
               obj_s5 = CreatePositionedObj(obj, OBJF_SPARKLE);
               obj_s5->x2.n = 12 * rcos(i * DEG(45)) >> 12;
               obj_s5->z2.n = 12 * rsin(i * DEG(45)) >> 12;
               obj_s5->y3.n = -2;
               obj_s5->y2.n = 8;
               obj_s5->d.sprite.clut = CLUT_REDS;
               obj_s5->state2 = 24;
            }
            obj->state3 = 6;
         }
         if (++OBJ.timer > 12) {
            OBJ.timer = 12;
         }
         break;

      case 1:
         if (++OBJ.timer > 12) {
            OBJ.timer = 12;
         }
         if (gSignal3 == 1) {
            obj->state2++;
         }
         break;

      case 2:
         if (--OBJ.timer == 0) {
            obj->state++;
         }
         break;
      }

      // Item icon
      obj_s5 = Obj_GetUnused();
      obj_s5->functionIndex = OBJF_NOOP;
      obj_s5->d.sprite.gfxIdx = OBJ.gfxIdx;
      obj_s5->d.sprite.boxIdx = 7;
      obj_s5->x1.n = obj->x1.n;
      obj_s5->z1.n = obj->z1.n;
      obj_s5->y1.n = obj->y1.n;

      halfSize = OBJ.timer;
      quad[0].vx = -halfSize;
      quad[1].vx = halfSize;
      quad[2].vx = -halfSize;
      quad[3].vx = halfSize;
      quad[0].vy = -halfSize;
      quad[1].vy = -halfSize;
      quad[2].vy = halfSize;
      quad[3].vy = halfSize;

      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i].vx = (quad[i].vx * rcos(OBJ.theta) - quad[i].vy * rsin(OBJ.theta)) >> 12;
         gQuad_800fe63c[i].vy = (quad[i].vx * rsin(OBJ.theta) + quad[i].vy * rcos(OBJ.theta)) >> 12;
         gQuad_800fe63c[i].vz = 0;
      }

      OBJ.theta += (0 - OBJ.theta) >> 2;
      AddObjPrim6(gGraphicsPtr->ot, obj_s5, 0);
      obj_s5->functionIndex = OBJF_NULL;
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF Unk8008d1f0
/* UNREACHABLE: not in gObjFunctionPointers[]. 128-frame camera-plane blue globe over the
 * target; un-hides the unit sprite on exit (written to follow a handler that hides it,
 * like the engulf family) and raises gSignal3 -- target/defeat-shaped, never wired up. */
void Objf_Unk_8008d1f0(Object *obj) {
   Object *unitSprite;
   SVectorXZY *p;
   s32 i;
   s32 camRotY;

   switch (obj->state) {
   case 0:
      unitSprite = SnapToUnit(obj);
      OBJ.unitSprite = unitSprite;
      OBJ.gfxIdx = GFX_GLOBE_5;
      OBJ.boxIdx = 7;
      OBJ.clut = CLUT_BLUES;
      OBJ.semiTrans = 2;
      obj->state2 = 0;
      OBJ.coords[0].y = OBJ.coords[1].y = -0x10;
      OBJ.coords[2].y = OBJ.coords[3].y = 0x10;
      OBJ.coords[0].x = OBJ.coords[2].x = -0x10;
      OBJ.coords[1].x = OBJ.coords[3].x = 0x10;
      obj->mem = 0x400;
      obj->state++;

   // fallthrough
   case 1:
      p = &OBJ.coords[0];
      i = 0;
      camRotY = gCameraRotation.vy;
      for (; i < 4; i++) {
         gQuad_800fe63c[i].vx = p->x;
         gQuad_800fe63c[i].vy = (p->y * rcos(camRotY) - p->z * rsin(camRotY)) >> 12;
         gQuad_800fe63c[i].vz = (p->y * rsin(camRotY) + p->z * rcos(camRotY)) >> 12;
         p++;
      }
      AddObjPrim6(gGraphicsPtr->ot, obj, 1);
      if (++obj->state2 >= 0x80) {
         obj->state++;
      }
      break;

   case 2:
      unitSprite = OBJ.unitSprite;
      unitSprite->d.sprite.hidden = 0;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 295
void Objf295_Smoke(Object *obj) {
   static s16 animData[32] = {GFX_PUFF_1,  2, 3, GFX_PUFF_2, 2, 3, GFX_PUFF_3, 2, 3,
                              GFX_PUFF_4,  2, 3, GFX_PUFF_5, 2, 3, GFX_PUFF_6, 2, 3,
                              GFX_PUFF_7,  2, 3, GFX_PUFF_8, 2, 3, GFX_PUFF_9, 2, 3,
                              GFX_PUFF_10, 2, 3, GFX_NULL,   0};

   s32 i, ct;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = animData;
      obj->d.sprite.semiTrans = 1;
      ct = rand() % 3;
      for (i = 0; i < ct; i++) {
         UpdateMultisizeObjAnimation(obj);
      }
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->x2.n += (0 - obj->x2.n) >> 2;
      obj->z2.n += (0 - obj->z2.n) >> 2;
      obj->y2.n += (0 - obj->y2.n) >> 2;
      UpdateMultisizeObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 385
void Objf385_RevealMimic(Object *obj) {
   s32 i;
   Object *smoke;
   SVECTOR vector;

   switch (obj->state) {
   case 0:
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(0.25);
      obj->state2 = 0x30;
      obj->state++;

   // fallthrough
   case 1:
      for (i = 0; i < 3; i++) {
         smoke = CreatePositionedObj(obj, OBJF_SMOKE);
         SphericalToVector(&vector, rand() % obj->state2 + obj->state2, rand() % DEG(45),
                       rand() % DEG(360));
         smoke->x2.n = vector.vx;
         smoke->z2.n = vector.vz;
         smoke->y2.n = vector.vy;
      }
      if (++obj->state2 >= 0x20) {
         obj->state2 = 0x30;
      }
      if (gState.subObjDone != 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 301
void Objf301_Map32_SmokestackParticle(Object *obj) {
   static s16 animData[24] = {7, GFX_PUFF_1, 2, GFX_PUFF_2,  2, GFX_PUFF_3, 2, GFX_PUFF_4,
                              2, GFX_PUFF_5, 2, GFX_PUFF_6,  2, GFX_PUFF_7, 2, GFX_PUFF_8,
                              2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   0, GFX_NULL};
   static Quad quad = {{-24, -24, 0, 0}, {24, -24, 0, 0}, {-24, 24, 0, 0}, {24, 24, 0, 0}};

   Quad *qswap;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = animData;
      obj->d.sprite.semiTrans = 1;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->x2.n += obj->x3.n;
      obj->z2.n += obj->z3.n;
      obj->y2.n += obj->y3.n;

      qswap = gSpriteBoxQuads[7];
      gSpriteBoxQuads[7] = &quad;
      UpdateObjAnimation(obj);
      if ((obj->x1.s.hi < gMapViewOriginX) || (obj->x1.s.hi > gMapSizeX + gMapViewOriginX - 1) ||
          (obj->z1.s.hi < gMapViewOriginZ) || (obj->z1.s.hi > gMapSizeZ + gMapViewOriginZ - 1)) {
         obj->d.sprite.hidden = 1;
      } else {
         obj->d.sprite.hidden = 0;
      }
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      gSpriteBoxQuads[7] = qswap;

      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 300
void Objf300_Map32_Smokestack(Object *obj) {
   Object *smoke;

   switch (obj->state) {
   case 0:
      obj->state++;
      obj->mem = 3;
      obj->state3 = 0;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         smoke = CreatePositionedObj(obj, OBJF_MAP32_SMOKESTACK_PARTICLE);
         smoke->z2.n = rand() % 9 - 4;
         smoke->x2.n = -(rand() % 4 + 12);
         smoke->y2.n = rand() % 4 + 12;
         smoke->z3.n = 0;
         smoke->x3.n = -(rand() % 2 + 5);
         obj->mem = rand() % 3 + 1;
      }
      break;
   }
}

