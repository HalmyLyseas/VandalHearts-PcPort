/* Spell casting effects, part 7 (segment 0x7851c): the multi-object "creature and weather"
 * spells -- Dark Hurricane, Dynamo Hum, Salamander, Rolling Fire, Roman Fire, Wyrmfang and
 * Avalanche chains -- plus Flame Breath (monster melee), the casting-pose FX, the treasure/
 * item-reveal cluster and Map32's smokestacks. Dispatched via gSpellsEx (see fx_060c38.c);
 * drivers own the gSignal3 completion handshake, children do not.
 *
 * This file carries the largest block of cut content in the corpus, all verified against
 * the retail gSpellsEx dump, the parsed EVDATA event scripts, and a whole-tree reference
 * search: eight orphaned table-resident handlers (suffixed _Unused: converging sparkle,
 * shrinking ground arc, casting-FX spawner, duplicate sparkle, red-X marker, rune spiral,
 * blue flame dome, morph-mesh node, plus the explosion-burst pair 397/398 and the
 * Salamander breath-head 377 whose breath phase no shipped head ever signals), three
 * handlers absent from gObjFunctionPointers entirely (Objf_Unk_*, unreachable), and a
 * zero-arg Noop stub. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

static s16 sFlameAnimData_800ff5f8[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

static s16 sFlameAnimData_800ff620[20] = {
    0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    0, GFX_NULL};

static s16 sFlameAnimData_800ff648[20] = {
    7, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
    2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};

// Used by Objf379_EvilStream_Rock
s16 gRockAnimData_800ff670[12] = {5, GFX_ROCK_1, 2, GFX_ROCK_2, 2, GFX_ROCK_3,
                                  2, GFX_ROCK_4, 2, GFX_NULL,   1, GFX_NULL};

static s16 sSmokeAnimData_800ff688[24] = {
    4, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4,  2, GFX_PUFF_5, 2, GFX_PUFF_6,
    2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   1, GFX_NULL};

#undef OBJF
#define OBJF 284
void Objf284_ConvergingSparkle_Unused(Object *obj) {
   static s16 sparkleAnimData[14] = {7, GFX_SPARKLE_1, 3, GFX_SPARKLE_2, 3, GFX_SPARKLE_3,
                                     3, GFX_SPARKLE_4, 3, GFX_SPARKLE_5, 3, GFX_NULL,
                                     1, GFX_NULL};

   s32 i;
   SVECTOR *p;
   Quad unused;

   switch (obj->state) {
   case 0:
      OBJ.animData = sparkleAnimData;
      OBJ.boxIdx = 7;
      obj->mem = 4 + rand() % 16;
      obj->x3.n = (obj->x2.n - obj->x1.n) / obj->mem;
      obj->z3.n = (obj->z2.n - obj->z1.n) / obj->mem;
      obj->y3.n = (obj->y2.n - obj->y1.n) / obj->mem;

      p = &OBJ.quad[0];
      for (i = 0; i < 4; i++) {
         p->vx = gQuad_800fe53c[i].vx;
         p->vy = gQuad_800fe53c[i].vy;
         p->vz = gQuad_800fe53c[i].vz;
         p++;
      }

      obj->state2 = 3;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.clut = 3 + obj->mem % 3;
      OBJ.quad[0].vx = -obj->state2;
      OBJ.quad[0].vy = -obj->state2;
      OBJ.quad[1].vx = obj->state2;
      OBJ.quad[1].vy = -obj->state2;
      OBJ.quad[2].vx = -obj->state2;
      OBJ.quad[2].vy = obj->state2;
      OBJ.quad[3].vx = obj->state2;
      OBJ.quad[3].vy = obj->state2;

      p = &OBJ.quad[0];
      for (i = 0; i < 4; i++) {
         gQuad_800fe63c[i].vx = p->vx;
         gQuad_800fe63c[i].vy = p->vy;
         gQuad_800fe63c[i].vz = 0;
         p++;
      }

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      obj->x1.n += (obj->x2.n - obj->x1.n) >> 3;
      obj->z1.n += (obj->z2.n - obj->z1.n) >> 3;
      obj->y1.n += (obj->y2.n - obj->y1.n) >> 3;

      if (--obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 286
void Objf286_ShrinkingGroundArc_Unused(Object *obj) {
   s32 radius;

   switch (obj->state) {
   case 0:
      obj->state2 = rand() % DEG(360);
      obj->mem = CV(0.75) + rand() % CV(0.25);
      obj->d.sprite.gfxIdx = GFX_COLOR_15;
      obj->state3 = 8;
      obj->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      radius = obj->mem;
      obj->d.sprite.coords[0].x = obj->x2.n + (radius * rcos(obj->state2) >> 12);
      obj->d.sprite.coords[0].z = obj->z2.n + (radius * rsin(obj->state2) >> 12);
      obj->d.sprite.coords[1].x = obj->x2.n + (radius * rcos(obj->state2 + 0x10) >> 12);
      obj->d.sprite.coords[1].z = obj->z2.n + (radius * rsin(obj->state2 + 0x10) >> 12);

      radius = obj->mem + ((0 - obj->mem) >> 2);
      obj->d.sprite.coords[2].x = obj->x2.n + (radius * rcos(obj->state2) >> 12);
      obj->d.sprite.coords[2].z = obj->z2.n + (radius * rsin(obj->state2) >> 12);
      obj->d.sprite.coords[3].x = obj->x2.n + (radius * rcos(obj->state2 + 0x10) >> 12);
      obj->d.sprite.coords[3].z = obj->z2.n + (radius * rsin(obj->state2 + 0x10) >> 12);

      obj->d.sprite.coords[0].y = obj->d.sprite.coords[1].y = obj->d.sprite.coords[2].y =
          obj->d.sprite.coords[3].y = obj->y2.n;

      obj->x1.n = obj->d.sprite.coords[0].x;
      obj->z1.n = obj->d.sprite.coords[0].z;
      obj->y1.n = obj->d.sprite.coords[0].y;
      obj->d.sprite.otOfs = -8;
      AddObjPrim5(gGraphicsPtr->ot, obj);

      obj->mem += (0 - obj->mem) >> 3;
      if (obj->mem <= 4) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 285
void Objf285_CastingFx(Object *obj) {
   Object *ray;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->y1.n += CV(0.5);
      obj->state++;
      obj->state2 = 4;
      obj->mem = 0xc0;
      PerformAudioCommand(AUDIO_CMD_PLAY_SFX(230));

   // fallthrough
   case 1:
      if (--obj->state2 <= 0) {
         for (i = 0; i < 3; i++) {
            ray = Obj_GetUnused();
            ray->functionIndex = OBJF_INWARD_RAY;
            ray->x2.n = obj->x1.n;
            ray->z2.n = obj->z1.n;
            ray->y2.n = obj->y1.n;
         }
         obj->state2 = (rand() >> 2) % 3;
      }
      if (gSignal3 == 1) {
         obj->mem = 0x20;
         obj->state++;
      }
      break;

   case 2:
      if (--obj->mem <= 0) {
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(231));
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 287
void Objf287_CastingFxSpawner_Unused(Object *obj) {
   Object *unitSprite;
   Object *castingFx;

   switch (obj->state) {
   case 0:
      unitSprite = SnapToUnit(obj);
      obj->y1.n += CV(0.5);
      OBJ.unitSprite = unitSprite;
      obj->state3 = 0;
      obj->state++;

      castingFx = Obj_GetUnused();
      castingFx->functionIndex = OBJF_CASTING_FX;
      castingFx->mem = 0x30;
      castingFx->x1.n = obj->x1.n;
      castingFx->z1.n = obj->z1.n;
      castingFx->y1.n = obj->y1.n;

   // fallthrough
   case 1:
      if (gSignal3 == 1) {
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 283
void Objf283_DarkHurricane_Cloud(Object *obj) {
   Object *parent;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_PUFF_1;
      OBJ.boxIdx = 4;
      OBJ.animData = sSmokeAnimData_800ff688;
      obj->mem = 0x100;
      obj->state2 = rand() % DEG(360);
      obj->state++;

   // fallthrough
   case 1:
      parent = OBJ.parent;
      obj->x1.n = parent->x1.n + (obj->mem * rcos(obj->state2) >> 12);
      obj->z1.n = parent->z1.n + (obj->mem * rsin(obj->state2) >> 12);
      obj->y1.n = parent->y1.n + CV(0.5) - (obj->mem / 2);
      obj->mem -= 0x10;
      obj->state2 += 0x40;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (obj->mem <= 0x20) {
         obj->functionIndex = OBJF_NULL;
      }
      if (parent->state == 99) {
         obj->state++;
      }
      break;

   case 2:
      parent = OBJ.parent;
      obj->x3.n = parent->x1.n + (obj->mem * rcos(obj->state2) >> 12);
      obj->z3.n = parent->z1.n + (obj->mem * rsin(obj->state2) >> 12);
      obj->y3.n = parent->y1.n + CV(0.5) - (obj->mem / 2);
      obj->x2.n = obj->x3.n - obj->x1.n;
      obj->z2.n = obj->z3.n - obj->z1.n;
      obj->y2.n = obj->y3.n - obj->y1.n;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 3:
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      if (++obj->mem >= 0x40) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 281
void Objf281_282_DarkHurricane_FX2_FX3(Object *obj) {
   Object *fxSprite;
   Object *targetSprite;
   Object *obj_v1;
   s16 theta;
   s32 i;
   Quad quad;

   switch (obj->state) {
   case 0:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_v1;

      targetSprite = SnapToUnit(obj);
      OBJ.targetSprite = targetSprite;

      obj->state3 = 0;
      obj->y1.n = CV(8.0);
      obj->y3.n = -8;
      OBJ.theta = 0;
      obj->state++;

   // fallthrough
   case 1:
      GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      targetSprite = OBJ.targetSprite;
      targetSprite->d.sprite.hidden = 1;
      fxSprite = OBJ.sprite;
      CopyObject(targetSprite, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      fxSprite->d.sprite.hidden = 0;
      obj->state3++;

      switch (obj->state2) {
      case 0:
         obj->state2++;

      // fallthrough
      case 1:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         OBJ.theta += (obj->y2.n * 8);
         theta = OBJ.theta;

         if (GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) > obj->y1.n) {
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_BOUNCE_ZOOM;
            obj_v1 = Obj_GetUnused();
            obj_v1->functionIndex = OBJF_DISPLAY_DAMAGE_2;
            obj_v1->x1.n = obj->x1.n;
            obj_v1->z1.n = obj->z1.n;
            obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
            obj->y2.n = -(obj->y2.n / 2);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         OBJ.theta += (0 - OBJ.theta) >> 2;
         theta = OBJ.theta;

         if (GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) > obj->y1.n) {
            obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
            obj->y2.n = 0;
            obj->y3.n = 0;
            fxSprite->functionIndex = OBJF_NULL;
            targetSprite->d.sprite.hidden = 0;
            obj->state3 = 0;

            if (obj->functionIndex == OBJF_DARK_HURRICANE_FX2) {
               obj->state += 1;
            } else if (obj->functionIndex == OBJF_DARK_HURRICANE_FX3) {
               obj->state += 2;
            }
         }

         break;
      }

      fxSprite->y1.n = obj->y1.n;
      fxSprite->x1.n = obj->x1.n;
      fxSprite->z1.n = obj->z1.n;

      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe53c[i];
      }

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx = (quad[i].vx * rcos(theta) - (quad[i].vy + 8) * rsin(theta)) >> 12;
         gQuad_800fe53c[i].vy = (quad[i].vx * rsin(theta) + (quad[i].vy + 8) * rcos(theta)) >> 12;
         gQuad_800fe53c[i].vz = 0;
      }

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i] = quad[i];
      }

      break;

   case 2:
      if (++obj->state3 >= 15) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;

   case 3:
      if (++obj->state3 >= 10) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 4:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_STRETCH_WARP_SPRITE;
      obj_v1->x1.n = obj->x1.n;
      obj_v1->z1.n = obj->z1.n;
      obj_v1->y1.n = obj->y1.n;
      obj->state++;

   // fallthrough
   case 5:
      if (++obj->state3 >= 15) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 280
void Objf280_DarkHurricane_Target(Object *obj) {
   Object *fxSprite; // copy of unit sprite to toss around
   Object *targetSprite;
   Object *obj_s1;
   Object *vortex;
   s32 i;
   s16 theta;
   Quad quad;

   switch (obj->state) {
   case 0:
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;
      OBJ.sprite = obj_s1;
      obj_s1 = OBJ.unitSprite = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->state3 = rand() % 0x20;
      obj->x1.n = obj_s1->x1.n;
      obj->z1.n = obj_s1->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->y3.n = -8;
      OBJ.theta = 0;
      obj->state++;

   // fallthrough
   case 1:
      targetSprite = OBJ.unitSprite;
      targetSprite->d.sprite.hidden = 1;
      fxSprite = OBJ.sprite;
      CopyObject(targetSprite, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      fxSprite->d.sprite.hidden = 0;
      obj->state3++;
      OBJ.theta += obj->y2.n * 8;
      theta = OBJ.theta;

      switch (obj->state2) {
      case 0:
         vortex = OBJ.parent;
         if (vortex->state == 99) {
            obj->y2.n = CV(-0.5);
            obj->state2++;
            obj->state3 = 0;
         } else {
            obj->y1.n += CV(0.03125);
            if (obj->y1.n >= CV(4.0)) {
               obj->y1.n = CV(4.0);
            }
            obj->x1.n = vortex->x1.n + (CV(0.5) * rcos(obj->state3 * 0x80) >> 12);
            obj->z1.n = vortex->z1.n + (CV(0.5) * rsin(obj->state3 * 0x80) >> 12);
            obj->y2.n = CV(1.75);
         }
         break;

      case 1:
         obj->y1.n += obj->y2.n;
         obj->y2.n += obj->y3.n;
         fxSprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
         break;
      }

      fxSprite->y1.n = obj->y1.n;
      fxSprite->x1.n = obj->x1.n;
      fxSprite->z1.n = obj->z1.n;

      for (i = 0; i < 4; i++) {
         quad[i] = gQuad_800fe53c[i];
      }

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i].vx =
             (quad[i].vx * rcos(theta) - (quad[i].vy + 8) * rsin(theta)) >> 1 >> 12;
         gQuad_800fe53c[i].vy =
             (quad[i].vx * rsin(theta) + (quad[i].vy + 8) * rcos(theta)) >> 1 >> 12;
         gQuad_800fe53c[i].vz = 0;
      }

      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);

      for (i = 0; i < 4; i++) {
         gQuad_800fe53c[i] = quad[i];
      }

      break;
   }
}

/* UNREACHABLE: not in gObjFunctionPointers[]. Salamander head + per-frame screen-space
 * red-spark wedge for 128 frames, then gSignal3 -- driver-shaped test code. */
void Objf_Unk_80089298(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 randomAngle;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      obj->d.sprite.gfxIdx = GFX_SALAMANDER_S;
      obj->d.sprite.boxIdx = 4;
      obj->d.sprite.clut = CLUT_REDS;
      obj->state2 = 0x80;
      obj->state++;

   // fallthrough
   case 1:
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      obj->x2.n = (poly->x0 + poly->x1) / 2;
      obj->y2.n = (poly->y0 + poly->y2) / 2;

      sprite = Obj_GetUnused();
      sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES;
      sprite->d.sprite.boxIdx = 4;
      sprite->d.sprite.clut = CLUT_BLUES;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      sprite->d.sprite.semiTrans = 1;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      poly->x0 = poly->x1 = obj->x2.n;
      poly->y0 = poly->y1 = obj->y2.n;

      randomAngle = (rand() >> 2) % DEG(360);
      poly->x2 = poly->x0 + (0x100 * rcos(randomAngle) >> 12);
      poly->y2 = poly->y0 + (0x100 * rsin(randomAngle) >> 12);
      poly->x3 = poly->x0 + (0x100 * rcos(randomAngle + 0x20) >> 12);
      poly->y3 = poly->y0 + (0x100 * rsin(randomAngle + 0x20) >> 12);

      if (--obj->state2 <= 0) {
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
#define OBJF 389
void Objf389_DarkHurricane_Vortex(Object *obj) {
   Object *parent;
   Object *sprite;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      sprite->d.sprite.boxIdx = 5;
      OBJ.sprite = sprite;

      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;
      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj->state2 = parent->state2;
      obj->x1.n = obj->x2.n + (0x80 * rcos(obj->state3 * 0x10) >> 12);
      obj->z1.n = obj->z2.n + (0x80 * rcos(obj->state3 * 0x10) >> 12);
      obj->y1.n = obj->y2.n;
      obj->state3++;

      sprite = OBJ.sprite;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      OBJ.theta += 0x40;

      if (parent->state == 99) {
         obj->state3 = 0;
         obj->state = 99;
      }
      break;

   case 99:
      if (++obj->state3 >= 2) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 390
void Objf390_DarkHurricane_VortexLayer(Object *obj) {
   Object *parent;
   Object *sprite;
   s32 i;
   s32 startingTheta;
   s32 parentTheta;
   s32 radius;
   s32 parentRadius;
   POLY_FT4 *poly;
   s32 increment;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_EXPLOSION_1;
      sprite->d.sprite.boxIdx = 5;
      OBJ.sprite = sprite;

      OBJ.height = 0;
      OBJ.radius2 = 0;
      OBJ.theta2 = (rand() >> 2) % DEG(360);
      OBJ.maxRadius = OBJ.radius;
      OBJ.radius = 0;
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:

      switch (obj->state2) {
      case 0:
         OBJ.height += 2;
         OBJ.radius2 = 0x10 + (0x30 * rcos(obj->state3 * 0x40) >> 12);
         OBJ.radius += 8;
         if (OBJ.radius > OBJ.maxRadius) {
            OBJ.radius = OBJ.maxRadius;
         }
         if (OBJ.height >= CV(0.375)) {
            OBJ.height = CV(0.375);
         }
         if (parent->state2 == 1) {
            obj->state2++;
            OBJ.radius = OBJ.maxRadius;
         }
         break;

      case 1:
         obj->state3++;
         OBJ.radius2 = 0x10 + (0x30 * rcos(obj->state3 * 0x40) >> 12);
         OBJ.height = 0x60 + (0x20 * rsin(obj->state3 * 0x20) >> 12);
         if (parent->state2 == 2) {
            obj->state2++;
         }
         break;

      case 2:
         OBJ.height += 2;
         OBJ.radius += (0 - OBJ.radius) >> 3;
         OBJ.radius2 += (0 - OBJ.radius2) >> 2;
         if (parent->state2 == 3) {
            obj->state2++;
         }
         break;
      }

      obj->x1.n = parent->x1.n + (OBJ.radius2 * rcos(OBJ.theta2) >> 12);
      obj->z1.n = parent->z1.n + (OBJ.radius2 * rsin(OBJ.theta2) >> 12);
      obj->y1.n = parent->y1.n + OBJ.height;

      OBJ.theta2 += DEG(2.8125);

      sprite = OBJ.sprite;
      startingTheta = OBJ.theta = parent->d.objf389.theta;
      parentTheta = startingTheta;
      radius = OBJ.radius;
      parentRadius = parent->d.objf389.radius;

      sprite->d.sprite.gfxIdx = GFX_TILED_LINES;
      sprite->d.sprite.clut = CLUT_GRAYS;
      sprite->d.sprite.semiTrans = 1;

      sprite->d.sprite.coords[0].x = obj->x1.n + (radius * rcos(startingTheta) >> 12);
      sprite->d.sprite.coords[0].z = obj->z1.n + (radius * rsin(startingTheta) >> 12);
      // Connect to previous layer (parent)
      sprite->d.sprite.coords[2].x = parent->x1.n + (parentRadius * rcos(parentTheta) >> 12);
      sprite->d.sprite.coords[2].z = parent->z1.n + (parentRadius * rsin(parentTheta) >> 12);
      sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = parent->y1.n;

      increment = DEG(360) / 16;

      for (i = 0; i < 16; i++) {
         sprite->d.sprite.coords[1].x =
             obj->x1.n + (radius * rcos(startingTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[1].z =
             obj->z1.n + (radius * rsin(startingTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[3].x =
             parent->x1.n + (parentRadius * rcos(parentTheta + increment * (i + 1)) >> 12);
         sprite->d.sprite.coords[3].z =
             parent->z1.n + (parentRadius * rsin(parentTheta + increment * (i + 1)) >> 12);

         sprite->x1.n = sprite->d.sprite.coords[3].x;
         sprite->z1.n = sprite->d.sprite.coords[3].z;
         sprite->y1.n = sprite->d.sprite.coords[3].y;

         AddObjPrim3(gGraphicsPtr->ot, sprite);
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         setRGB0(poly, 0x64, 0x64, 0x64);

         sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[1].x;
         sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[1].z;
         sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[3].x;
         sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[3].z;
      }

      if (parent->state == 99) {
         obj->state = 99;
      }
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 388
void Objf388_DarkHurricane_FX1(Object *obj) {
   Object *obj_s2;
   Object *obj_a0;
   BVectorXZ *p;
   s32 i;

   switch (obj->state) {
   case 0:
      obj->z1.s.hi = gTargetZ;
      obj->x1.s.hi = gTargetX;
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);

      obj_s2 = Obj_GetUnused();
      obj_s2->functionIndex = OBJF_DARK_HURRICANE_VORTEX;
      obj_s2->x1.n = obj->x1.n;
      obj_s2->z1.n = obj->z1.n;
      obj_s2->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj_s2->d.objf389.parent = obj;
      OBJ.vortex = obj_s2;

      p = (BVectorXZ *)gTargetCoords;
      obj_a0 = GetUnitSpriteAtPosition(p->z, p->x);
      obj->x2.n = obj_a0->x1.n;
      obj->z2.n = obj_a0->z1.n;
      obj->y2.n = obj_a0->y1.n + CV(0.5);
      while (p->x != 0xff) {
         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_DARK_HURRICANE_TARGET;
         obj_a0->z1.s.hi = p->z;
         obj_a0->x1.s.hi = p->x;
         obj_a0->d.objf280.parent = obj_s2;
         p++;
      }

      for (i = 0; i < 8; i++) {
         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_DARK_HURRICANE_VORTEX_LAYER;
         obj_a0->d.objf390.parent = obj_s2;
         obj_a0->d.objf390.radius = (i + 1) * 0x40;
         obj_a0->d.objf390.unused_0x38 = i * 0x20;
         obj_s2 = obj_a0;
      }

      obj->state++;
      obj->state3 = 0x100;
      obj->mem = 0;
      obj->state2 = 0;
      break;

   case 1:
      gCameraZoom.vz += (400 - gCameraZoom.vz) >> 3;
      gCameraRotation.vy += 0x10;
      PanCamera(obj->x2.n, obj->y2.n, obj->z2.n, 2);

      if (--obj->mem <= 0) {
         obj_s2 = Obj_GetUnused();
         obj_s2->functionIndex = OBJF_DARK_HURRICANE_CLOUD;
         obj_s2->d.objf283.parent = OBJ.vortex;
         obj->mem = rand() % 3;
      }

      obj->state3--;

      switch (obj->state2) {
      case 0:
         if (obj->state3 <= 0xc0) {
            obj->state2++;
         }
         break;
      case 1:
         if (obj->state3 <= 0x20) {
            obj->state2++;
         }
         break;
      case 2:
         if (obj->state3 <= 8) {
            obj->state2++;
         }
         break;
      }

      if (obj->state3 <= 0) {
         obj->state3 = 0;
         obj->state = 99;
      }
      break;

   case 99:
      if (++obj->state3 >= 2) {
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 392
/* Non-drawing 9x9 lattice vertex: flat layout -> neighbour relaxation -> sphere mapping,
 * driven by a controller object whose handler does not exist in retail. Cut content. */
void Objf392_MorphMeshNode_Unused(Object *obj) {
   Object *obj_s1;
   Object *obj1;
   Object *obj2;
   Object *obj3;
   Object *obj4;
   s32 radius;
   s8 unused[24];

   // Not enough context to identify which objf to use for this.
   obj_s1 = OBJ.todo_x5c;

   if (obj_s1->d.objfUnkUsedBy392.todo_x28 == 99) {
      obj->functionIndex = OBJF_NULL;
      return;
   }

   obj1 = OBJ.todo_x24;
   obj2 = OBJ.todo_x28;
   obj3 = OBJ.todo_x2c;
   obj4 = OBJ.todo_x30;

   switch (obj_s1->d.objfUnkUsedBy392.todo_x24) {
   case 0:
      obj->x1.n = obj->x3.n * 12 - 0x30;
      obj->y1.n = obj->y3.n * 12 - 0x30;
      if (obj->y1.n == 0) {
         obj->state3 = 0x200 + (8 - obj->x1.n) * 0x71;
      } else if (obj->y1.n == 8) {
         obj->state3 = -0x200 - (8 - obj->x1.n) * 0x71;
      }
      if (obj->x1.n == 0) {
         obj->state3 = obj->y1.n * 0x71 + 0x600;
      } else if (obj->x1.n == 8) {
         obj->state3 = -0x188;
      }
      break;

   case 1:
      if (obj1 != NULL && obj2 != NULL) {
         obj->y1.n += ((obj1->y1.n + obj2->y1.n) / 2 - obj->y1.n) >> 2;
      }
      if (obj3 != NULL && obj4 != NULL) {
         obj->x1.n += ((obj3->x1.n + obj4->x1.n) / 2 - obj->x1.n) >> 2;
      }
      if (obj3 == NULL || obj4 == NULL || obj1 == NULL || obj2 == NULL) {
         obj->x1.n += 4 * rcos(0x800 * rsin(obj_s1->d.objfUnkUsedBy392.theta) >> 12) >> 12;
         obj->y1.n += 4 * rsin(0x800 * rsin(obj_s1->d.objfUnkUsedBy392.theta) >> 12) >> 12;
      }
      break;

   case 2:
      if (obj->y3.n == 0 || obj->y3.n == 8) {
         radius = 0;
      } else {
         radius = 0x100 * rsin(obj->y3.n * 0x100) >> 12;
      }
      obj->x1.n = -(radius * rcos(obj->x3.n * 0x200) >> 12);
      obj->z1.n = -(radius * rsin(obj->x3.n * 0x200) >> 12);
      obj->y1.n = (8 - obj->y3.n) * 0x40;
      break;
   }
}

#undef OBJF
#define OBJF Unk8008a364
/* UNREACHABLE: not in gObjFunctionPointers[]. Camera-spinning disc that re-samples the
 * framebuffer through its own quad (GetTPage(2, ...) + flipped verts); never terminates
 * -- an abandoned mirror/reflection experiment. */
void Objf_Unk_8008a364(Object *obj) {
   Object *sprite;
   POLY_FT4 *poly;
   s32 clipX, clipY;
   s32 swap;
   u8 unused[32];

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n + CV(2.0);
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_LARGE_RED_CIRCLE;
      sprite->d.sprite.boxIdx = 3;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      clipX = gGraphicsPtr->drawEnv.clip.x;
      clipY = gGraphicsPtr->drawEnv.clip.y;
      gCameraRotation.vy += 1;

      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      poly->u0 = poly->x2;
      poly->v0 = poly->y2;
      poly->u1 = poly->x3;
      poly->v1 = poly->y3;
      poly->u2 = poly->x2 + 0xe0;
      poly->v2 = 0xff;
      poly->u3 = poly->x3 + 0x20;
      poly->v3 = 0xff;

      swap = poly->x0;
      poly->x0 = poly->x2;
      poly->x2 = swap;
      swap = poly->x1;
      poly->x1 = poly->x3;
      poly->x3 = swap;
      swap = poly->y0;
      poly->y0 = poly->y2;
      poly->y2 = swap;
      swap = poly->y1;
      poly->y1 = poly->y3;
      poly->y3 = swap;

      poly->tpage = GetTPage(2, 0, clipX, clipY);
      break;
   }
}

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
      OBJ.todo_x2a = obj->x3.n;

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

         switch (OBJ.todo_x2a) {
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

            if (OBJ.todo_x2a == 2) {
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
         func_800A9E78(&vector, rnd2, rnd3, rnd1);
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
         func_800A9E78(&vector, rnd2, rnd3, rnd1);
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
   s16 randomAngle;

   switch (obj->state) {
   case 0:
      obj->state2 = 0x10;
      obj->d.sprite.gfxIdx = GFX_PUFF_1;
      obj->d.sprite.animData = sSmokeAnimData_800ff688;
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
         func_800A9E78(&vector, rand() % obj->state2 + obj->state2, rand() % DEG(45),
                       rand() % DEG(360));
         smoke->x2.n = vector.vx;
         smoke->z2.n = vector.vz;
         smoke->y2.n = vector.vy;
      }
      if (++obj->state2 >= 0x20) {
         obj->state2 = 0x30;
      }
      if (gState.D_8014053E != 0) {
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
      if ((obj->x1.s.hi < D_80122E28) || (obj->x1.s.hi > gMapSizeX + D_80122E28 - 1) ||
          (obj->z1.s.hi < D_80122E2C) || (obj->z1.s.hi > gMapSizeZ + D_80122E2C - 1)) {
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

#undef OBJF
#define OBJF 332
void Objf332_RollingFire_FX1(Object *obj) {
   extern s16 gFlameAnimData_800ff8a4[];

   Object *dataStore;
   Cylinder *dsCylinder;

   switch (obj->state) {
   case 0:
      obj->x1.n = gTargetX * CV(1.0) + CV(0.5);
      obj->z1.n = gTargetZ * CV(1.0) + CV(0.5);
      obj->y1.n = GetTerrainElevation(gTargetZ, gTargetX);
      obj->d.sprite.animData = gFlameAnimData_800ff8a4;

      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->sideCt = 8;
      dsCylinder->semiTrans = 4;
      dsCylinder->gfxIdx = GFX_FLAME_1;
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->useColor = 1;
      dsCylinder->color.r = 0x80;
      dsCylinder->color.g = 0x80;
      dsCylinder->color.b = 0x80;

      obj->state2 = 0x200;
      obj->y2.n = 0x60;
      obj->y3.n = -4;
      obj->x2.n = 0;
      obj->x3.n = 0x10;
      obj->state++;

   // fallthrough
   case 1:
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
      if (++obj->mem >= 4) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 2:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      UpdateObjAnimation(obj);
      dsCylinder->gfxIdx = OBJ.gfxIdx;
      if (obj->mem >= 0x80) {
         dsCylinder->color.r = obj->mem - 1;
         dsCylinder->color.g = obj->mem - 1;
         dsCylinder->color.b = obj->mem - 1;
      }
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + (CV(6.0) * rsin(obj->mem * 8) >> 12);
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottomRadius = CV(2.0) * rsin(obj->mem * 8) >> 12;
      dsCylinder->topRadius =
          dsCylinder->bottomRadius * 2 + (CV(2.0) * rsin(obj->mem * 0x20) >> 12);
      dsCylinder->theta = obj->x2.n;
      obj->x2.n += obj->x3.n;
      obj->x3.n += 1;
      dsCylinder->semiTrans = 4;
      RenderCylinder(dsCylinder);
      dsCylinder->top.vy = obj->y1.n + (CV(4.0) * rsin(obj->mem * 8) >> 12);
      dsCylinder->top.vx = obj->x1.n + (CV(0.5) * rcos(obj->mem * 0x20) >> 12);
      dsCylinder->top.vz = obj->z1.n + (CV(0.5) * rsin(obj->mem * 0x20) >> 12);
      dsCylinder->semiTrans = 2;
      RenderCylinder(dsCylinder);
      dsCylinder->theta = -dsCylinder->theta;
      dsCylinder->bottomRadius -= (dsCylinder->bottomRadius >> 1);
      dsCylinder->topRadius -= (dsCylinder->topRadius >> 1);
      RenderCylinder(dsCylinder);
      obj->state2 += obj->y2.n;
      obj->y2.n += obj->y3.n;
      obj->mem += 2;
      gCameraZoom.vz += (384 - gCameraZoom.vz) >> 4;
      gCameraRotation.vy -= 6;
      if (obj->mem >= 0x100) {
         obj->state++;
      }
      break;

   case 3:
      dataStore = OBJ.dataStore;
      dataStore->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      gSignal3 = 1;
      break;
   }
}

#undef OBJF
#define OBJF 333
/* Sparkle spiralling inward onto a unit (~248 frames), laying rune-textured Objf323
 * ribbon segments every frame, then gSignal3. Cut: nothing dispatches 333 (Objf323's
 * other user is EVDATA29.DAT / Map20 -- see its comment in split_09a268.c). */
void Objf333_RuneSpiral_Unused(Object *obj) {
   extern s16 gSparkleAnimData_800ff38c[14];
   Object *obj_s3;
   s16 a, b;

   switch (obj->state) {
   case 0:
      obj_s3 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      obj->x2.n = obj_s3->x1.n;
      obj->z2.n = obj_s3->z1.n;
      obj->y2.n = obj_s3->y1.n;
      obj->d.sprite.animData = gSparkleAnimData_800ff38c;
      // TODO: Object
      obj->d.sprite.coords[0].x = 0x400;
      obj->d.sprite.coords[0].z = -4;
      obj->d.sprite.coords[0].y = 0;
      obj->d.sprite.coords[1].x = 0x40;
      obj->x1.n = obj->x2.n + (CV(4.0) * rcos(0) >> 12);
      obj->z1.n = obj->z2.n + (CV(4.0) * rsin(0) >> 12);
      obj->y1.n = obj->y2.n + CV(1.0);
      obj->state++;

   // fallthrough
   case 1:
      obj_s3 = Obj_GetUnused();
      obj_s3->functionIndex = OBJF_FX_TBD_323;
      obj_s3->d.objf323.gfxIdx = GFX_RUNE_1 + rand() % 10;
      obj_s3->d.objf323.semiTrans = 2;
      obj_s3->state2 = 0xff;
      obj_s3->state3 = 8;
      obj_s3->d.objf323.coords[1].x = obj->x1.n;
      obj_s3->d.objf323.coords[3].x = obj->x1.n;
      obj_s3->d.objf323.coords[1].z = obj->z1.n;
      obj_s3->d.objf323.coords[3].z = obj->z1.n;
      obj_s3->d.objf323.coords[1].y = obj->y1.n;
      obj_s3->d.objf323.coords[3].y = obj->y1.n - CV(0.25);
      a = obj->d.sprite.coords[0].x + obj->d.sprite.coords[0].z;
      obj->d.sprite.coords[0].x = a;
      b = obj->d.sprite.coords[0].y + obj->d.sprite.coords[1].x;
      obj->d.sprite.coords[0].y = b;
      obj->x1.n = obj->x2.n + (a * rcos(b) >> 12);
      obj->z1.n = obj->z2.n + (a * rsin(b) >> 12);
      obj->y1.n = obj->y2.n;
      UpdateObjAnimation(obj);
      obj->d.sprite.boxIdx = 5;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      obj_s3->d.objf323.coords[0].x = obj->x1.n;
      obj_s3->d.objf323.coords[2].x = obj->x1.n;
      obj_s3->d.objf323.coords[0].z = obj->z1.n;
      obj_s3->d.objf323.coords[2].z = obj->z1.n;
      obj_s3->d.objf323.coords[0].y = obj->y1.n;
      obj_s3->d.objf323.coords[2].y = obj->y1.n - CV(0.25);
      obj_s3->x1.n = obj_s3->d.objf323.coords[0].x;
      obj_s3->z1.n = obj_s3->d.objf323.coords[0].z;
      obj_s3->y1.n = obj_s3->d.objf323.coords[0].y;
      if (a < 0x20) {
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
#define OBJF 348
void Objf348_BlueFlameDome_Unused(Object *obj) {
   Object *dataStore;
   Object *sprite;
   Cylinder *dsCylinder;
   s32 i;
   s16 iVar6;
   s16 iVar2;
   s16 uVar1;
   s32 half;
   s16 theta_0xc0;

   gGfxSubTextures[GFX_TILED_FLAMES_DYN_1][1] =
       gGfxSubTextures[GFX_TILED_FLAMES][1] + (obj->mem * 2 % 0x20);

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->bottom.vx = dsCylinder->top.vx = obj->x1.n;
      dsCylinder->bottom.vz = dsCylinder->top.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vy = obj->y1.n + CV(4.0);
      dsCylinder->sideCt = 16;
      dsCylinder->topRadius = CV(1.0);
      dsCylinder->bottomRadius = CV(1.0);
      dsCylinder->gfxIdx = GFX_TILED_FLAMES_DYN_1;
      dsCylinder->semiTrans = 2;
      dsCylinder->useColor = 1;
      dsCylinder->clut = CLUT_BLUES;
      dsCylinder->color.r = 0x80;
      dsCylinder->color.g = 0x80;
      dsCylinder->color.b = 0x80;
      dsCylinder->theta = 0;

      for (i = 0; i < 8; i++) {
         OBJ.todo_x24[i] = (rand() >> 2) % 0x100 + 0x200;
         OBJ.todo_x34[i] = 0;
         OBJ.todo_x44[i] = (rand() >> 2) % 0x180;
      }

      obj->state++;

   // fallthrough
   case 1:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->theta = obj->mem << 2;
      dsCylinder->color.r = (0x100 - obj->mem) >> 1;
      dsCylinder->color.g = (0x100 - obj->mem) >> 1;
      dsCylinder->color.b = (0x100 - obj->mem) >> 1;

      //@6750
      uVar1 = obj->mem;
      iVar2 = (obj->mem * (0x100 - obj->mem)) >> 4;
      iVar6 = obj->mem;

      half = iVar6 >> 1;
      if (half >= 0x80) {
         half = 0x80;
      }

      for (i = 0; i < 8; i++) {
         dsCylinder->bottom.vy = obj->y1.n + (iVar2 * rsin(i * half) >> 12);
         dsCylinder->top.vy = obj->y1.n + (iVar2 * rsin((i + 1) * half) >> 12);
         dsCylinder->bottomRadius = uVar1 * rcos(i * half) >> 12;
         dsCylinder->topRadius = uVar1 * rcos((i + 1) * half) >> 12;
         RenderCylinder(dsCylinder);
      }

      sprite = Obj_GetUnused();
      sprite->d.sprite.gfxIdx = GFX_TILED_FLAMES;
      sprite->d.sprite.semiTrans = 4;
      theta_0xc0 = 0xc0;

      for (i = 0; i < 8; i++) {
         OBJ.todo_x34[i] += (OBJ.todo_x24[i] - OBJ.todo_x34[i]) >> 3;
         iVar6 = OBJ.todo_x34[i];

         sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[0].x =
             obj->x1.n + (iVar6 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[0].z =
             obj->z1.n + (iVar6 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y =
             obj->y1.n + (iVar6 * rsin(theta_0xc0) >> 12);

         iVar6 = uVar1 - 0x80;
         if (iVar6 < 0) {
            iVar6 = 0;
         }

         sprite->d.sprite.coords[3].x = obj->x1.n + (iVar6 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[3].z = obj->z1.n + (iVar6 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[3].y = obj->y1.n;
         sprite->d.sprite.coords[2].x = obj->x1.n + (uVar1 * rcos(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[2].z = obj->z1.n + (uVar1 * rsin(i * DEG(45)) >> 12);
         sprite->d.sprite.coords[2].y = obj->y1.n;
         AddObjPrim4(gGraphicsPtr->ot, sprite);
      }

      if (++obj->mem >= 0x100) {
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

#undef OBJF
#define OBJF 334
void Objf334_Salamander_FX1(Object *obj) {
   Object *previous;
   Object *current;
   s32 i;

   switch (obj->state) {
   case 0:
      SnapToUnit(obj);
      previous = CreatePositionedObj(obj, OBJF_SALAMANDER_HEAD);
      previous->d.objf335.link = obj;
      previous->d.objf335.parent = obj;

      for (i = 0; i < 32; i++) {
         current = Obj_GetUnused();
         current->functionIndex = OBJF_SALAMANDER_SEGMENT;
         current->x1.n = obj->x1.n;
         current->z1.n = obj->z1.n;
         current->y1.n = obj->y1.n;
         current->d.objf336.link = previous;
         current->d.objf336.theta1 = 0;
         current->d.objf336.theta2 = 0;
         current->d.objf336.todo_x2c = 0;
         current->d.objf336.todo_x2e = i * 0x111;
         current->d.objf336.parent = obj;
         current->mem = 0;
         previous = current;
      }

      OBJ.todo_x24 = 0;
      obj->state++;

   // fallthrough
   case 1:
      obj->mem = 0;
      obj->state++;
      break;

   case 2:
      if (gLightColor.r <= 0xd0) {
         gLightColor.r++;
         // gLightColor.g -= 1; // No match; needs to be decrement op.
         gLightColor.g--;
         gLightColor.b--;
      }
      if (obj->mem == 1) {
         obj->state++;
      }
      break;

   case 3:
      gLightColor.r = (0x80 - gLightColor.r) >> 2;
      gLightColor.g = (0x80 - gLightColor.g) >> 2;
      gLightColor.b = (0x80 - gLightColor.b) >> 2;
      if (++obj->mem >= 24) {
         gLightColor.b = 0x80;
         gLightColor.g = 0x80;
         gLightColor.r = 0x80;
         obj->state3 = 1;
         obj->functionIndex = OBJF_NULL;
         gSignal3 = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 335
void Objf335_Salamander_Head(Object *obj) {
   Quad quad = {{-32, -32, 0, 0}, {32, -32, 0, 0}, {-32, 32, 0, 0}, {32, 32, 0, 0}};
   s16 headGfx[8] = {GFX_SALAMANDER_N, GFX_SALAMANDER_NE, GFX_SALAMANDER_E, GFX_SALAMANDER_SE,
                     GFX_SALAMANDER_S, GFX_SALAMANDER_SE, GFX_SALAMANDER_E, GFX_SALAMANDER_NE};

   Object *parent;
   Object *sprite;
   s16 unaff_s1;
   s32 i;
   s16 dir;
   s16 theta;

   parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_8;
      sprite->d.sprite.boxIdx = 3;

      obj->x2.n = obj->x1.n;
      obj->y2.n = obj->y1.n;
      obj->z2.n = obj->z1.n;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) + CV(2.0);
      obj->state3 = 0x50;
      obj->z3.n = 0;
      OBJ.unused_0x4E = 0;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      OBJ.position4.x = OBJ.position3.x;
      OBJ.position3.x = OBJ.position2.x;
      OBJ.position2.x = OBJ.position1.x;
      OBJ.position1.x = obj->x1.n;
      OBJ.position4.y = OBJ.position3.y;
      OBJ.position3.y = OBJ.position2.y;
      OBJ.position2.y = OBJ.position1.y;
      OBJ.position1.y = obj->y1.n;
      OBJ.position4.z = OBJ.position3.z;
      OBJ.position3.z = OBJ.position2.z;
      OBJ.position2.z = OBJ.position1.z;
      OBJ.position1.z = obj->z1.n;

      switch (obj->z3.n) {
      case 0:
         unaff_s1 = 0x40 + (0x30 * rsin(OBJ.theta1 * 2) >> 12);
         OBJ.theta1 += obj->state3;
         OBJ.theta2 = CV(1.0) * rsin(OBJ.theta1) >> 12;

         if (++obj->y3.n >= 0x100) {
            obj->y3.n = 0;
            obj->x3.n = 0x100;
            gCameraRotation.vy &= 0xfff;
            obj->z3.n++;
         }

         obj->x1.n -= (unaff_s1 * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (unaff_s1 * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);

         gCameraRotation.vy += 0x10;
         gCameraZoom.vz = 460;
         break;

      case 1:
         OBJ.theta1 = 0;
         OBJ.theta2 += obj->x3.n;
         gCameraRotation.vy += (0 - gCameraRotation.vy) >> 2;
         unaff_s1 = 0xa0;

         if (++obj->y3.n >= 0x40) {
            obj->y3.n = 0;
            obj->z3.n++;
         }

         i = unaff_s1 * rcos(OBJ.theta2) >> 12;
         obj->x1.n -= (i * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (i * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);
         break;

      case 2:
         obj->y3.n = 0;
         obj->mem = 99;
         parent->mem++;
         obj->state++;
         break;

      case 3:
         OBJ.theta2 = 0;
         unaff_s1 = 0x40;

         if (++obj->y3.n >= 0x80) {
            obj->y3.n = 0;
            obj->z3.n++;
         }

         obj->x1.n -= (unaff_s1 * rcos(OBJ.theta1) >> 12);
         obj->z1.n -= (unaff_s1 * rsin(OBJ.theta1) >> 12);
         obj->y1.n -= (unaff_s1 * rsin(OBJ.theta2) >> 12);
         break;

      case 4:
      default:
         unaff_s1 = 0x40;
         obj->y3.n = 0;
         obj->mem = 99;
         parent->mem++;
         obj->state++;
         break;
      }

      if (OBJ.theta1 >= DEG(360)) {
         obj->state3 = -obj->state3;
      } else if (OBJ.theta1 <= 0) {
         obj->state3 = -obj->state3;
      }

      OBJ.todo_x2a = OBJ.theta2;
      OBJ.todo_x28 = OBJ.theta1;
      OBJ.todo_x2c = unaff_s1 + 0xa0;
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);

      switch (obj->z3.n) {
      case 0:
         dir = (((gCameraRotation.vy - OBJ.theta1) & 0xfff) / DEG(45) - 1) & 7;
         sprite->d.sprite.gfxIdx = headGfx[dir];
         sprite->d.sprite.boxIdx = 7;
         sprite->d.sprite.semiTrans = 2;

         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i] = quad[i];
         }

         if (dir > 4) {
            sprite->d.sprite.facingLeft = 1;
         } else {
            sprite->d.sprite.facingLeft = 0;
         }

         break;

      case 1:
      case 2:
         sprite->d.sprite.gfxIdx = GFX_SALAMANDER_E;
         dir = (((gCameraRotation.vy - OBJ.theta1) & 0xfff) / DEG(45) - 1) & 7;

         if (dir > 4) {
            sprite->d.sprite.facingLeft = 1;
         } else {
            sprite->d.sprite.facingLeft = 0;
         }

         sprite->d.sprite.boxIdx = 7;

         theta = OBJ.theta2;
         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i].vx = (quad[i].vx * rcos(theta) - quad[i].vy * rsin(theta)) >> 12;
            gQuad_800fe63c[i].vy = (quad[i].vx * rsin(theta) + quad[i].vy * rcos(theta)) >> 12;
         }
         break;

      case 3:
         break;
      }

      sprite->d.sprite.semiTrans = 2;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      sprite->d.sprite.semiTrans = 0;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      break;

   case 2:
      gCameraZoom.vz += (256 - gCameraZoom.vz) >> 2;
      if (++obj->y3.n >= 0x10) {
         sprite = OBJ.sprite;
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 336
void Objf336_Salamander_Segment(Object *obj) {
   static s16 animData[20] = {7, GFX_FLAME_1, 1, GFX_FLAME_2, 1, GFX_FLAME_3, 1, GFX_FLAME_4,
                              1, GFX_FLAME_5, 1, GFX_FLAME_6, 1, GFX_FLAME_7, 1, GFX_FLAME_8,
                              1, GFX_NULL,    1, GFX_NULL};
   static Quad quad = {{-16, -48, 0, 0}, {16, -48, 0, 0}, {-16, 16, 0, 0}, {16, 16, 0, 0}};

   SVECTOR vector_unused;
   Quad quad_unused = {{-32, -32, 0, 0}, {32, -32, 0, 0}, {-32, 32, 0, 0}, {32, 32, 0, 0}};

   Object *fx1;
   Object *sprite;
   Object *link;
   Object *flamingRock;
   s32 ct;
   s32 i;
   s32 randomAngle;
   s16 theta;
   s32 dx, dy, dz;

   fx1 = OBJ.parent;

   switch (obj->state) {
   case 0:
      obj->y1.n += CV(2.0);

      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_5;
      sprite->d.sprite.boxIdx = 3;
      sprite->d.sprite.animData = animData;

      ct = (rand() >> 2) % 0x10;
      for (i = 0; i < ct; i++) {
         UpdateObjAnimation(sprite);
      }

      sprite->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      link = OBJ.link;

      if (obj->mem < 9) {
         obj->mem = link->mem;
      }

      OBJ.position4.x = OBJ.position3.x;
      OBJ.position3.x = OBJ.position2.x;
      OBJ.position2.x = OBJ.position1.x;
      OBJ.position1.x = obj->x1.n;
      OBJ.position4.y = OBJ.position3.y;
      OBJ.position3.y = OBJ.position2.y;
      OBJ.position2.y = OBJ.position1.y;
      OBJ.position1.y = obj->y1.n;
      OBJ.position4.z = OBJ.position3.z;
      OBJ.position3.z = OBJ.position2.z;
      OBJ.position2.z = OBJ.position1.z;
      OBJ.position1.z = obj->z1.n;

      OBJ.todo_x2c = 0x40 + (0x20 * rsin(OBJ.todo_x2e) >> 12);
      OBJ.todo_x2e = (OBJ.todo_x2e + 0x40) & 0xfff;
      obj->x1.n = link->d.objf336.position1.x;
      obj->z1.n = link->d.objf336.position1.z;
      obj->y1.n = link->d.objf336.position1.y;
      OBJ.todo_x2c = link->d.objf336.todo_x2c;
      OBJ.theta2 = 0;
      OBJ.todo_x2a = link->d.objf336.todo_x2a;
      OBJ.todo_x28 = link->d.objf336.todo_x28 + OBJ.theta1;
      vector_unused.vy = OBJ.todo_x2c * rsin(OBJ.todo_x2a) >> 12;
      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;

      if (obj->mem != 9) {
         dx = link->x1.n - obj->x1.n;
         dz = link->z1.n - obj->z1.n;
         dy = link->y1.n - obj->y1.n;
         theta = ratan2(dy, SquareRoot0(dx * dx + dz * dz));
         for (i = 0; i < 4; i++) {
            gQuad_800fe63c[i].vx = (quad[i].vx * rcos(theta) - quad[i].vy * rsin(theta)) >> 12;
            gQuad_800fe63c[i].vy = (quad[i].vx * rsin(theta) + quad[i].vy * rcos(theta)) >> 12;
         }
         UpdateObjAnimation(sprite);
      }

      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);

      if (obj->mem == 99) {
         if (fx1->d.objf334.todo_x24 < 2) {
            fx1->d.objf334.todo_x24++;
         } else {
            if (Obj_CountUnused() > 100) {
               randomAngle = rand() % DEG(360);
               flamingRock = CreatePositionedObj(obj, OBJF_FLAMING_ROCK);
               flamingRock->y2.n = (rand() & 0x3f) + 0x10;
               flamingRock->x2.n = (0x40 * rsin(randomAngle) >> 12);
               flamingRock->z2.n = (0x40 * rcos(randomAngle) >> 12);
               flamingRock->y3.n = -12;
               fx1->d.objf334.todo_x24 = 0;
            }
         }
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
#define OBJF 377
/* An objf335/336-compatible Salamander chain link (identical struct layout + chaining
 * math) drawing the directional head; when its link reports mem == 2 it emits Flame
 * Breath particles -- but no shipped head handler ever writes mem = 2, and nothing
 * spawns 377: a cut breath-phase variant of the Salamander. */
void Objf377_SalamanderBreathHead_Unused(Object *obj) {
   SVECTOR vector;
   s16 headGfx[8] = {GFX_SALAMANDER_S, GFX_SALAMANDER_SE, GFX_SALAMANDER_E, GFX_SALAMANDER_NE,
                     GFX_SALAMANDER_N, GFX_SALAMANDER_NE, GFX_SALAMANDER_E, GFX_SALAMANDER_SE};
   Quad quad = {{-32, -32, 0, 0}, {32, -32, 0, 0}, {-32, 32, 0, 0}, {32, 32, 0, 0}};

   Object *sprite;
   Object *obj_s4; //? objf unknown - treating as objf335
   Object *obj_s1;
   s16 dir;
   s32 i;
   s32 iVar3, x, z;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      OBJ.sprite = sprite;
      sprite->d.sprite.gfxIdx = GFX_GLOBE_5;
      sprite->d.sprite.boxIdx = 3;
      obj->state++;

   // fallthrough
   case 1:
      sprite = OBJ.sprite;
      obj_s4 = OBJ.todo_x5c;

      dir = (((gCameraRotation.vy - OBJ.theta3) & 0xfff) / DEG(45)) & 7;
      sprite->d.sprite.gfxIdx = headGfx[dir];

      if (dir > 4) {
         sprite->d.sprite.facingLeft = 1;
      } else {
         sprite->d.sprite.facingLeft = 0;
      }

      if (obj->mem == 0) {
         if (obj_s4->mem != 2) {
            sprite->d.sprite.boxIdx = 7;
            for (i = 0; i < 4; i++) {
               gQuad_800fe63c[i] = quad[i];
            }
            OBJ.radius = CV(0.5) + (CV(0.125) * rsin(OBJ.theta5) >> 12);
            OBJ.theta5 = (OBJ.theta5 + DEG(5.625)) & 0xfff;
            OBJ.theta2 = obj_s4->d.objf335.theta2;
            gCameraZoom.vz = 512;
            PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
            gCameraRotation.vy += (16 * rsin(OBJ.theta4 >> 1) >> 12);
         } else {
            obj_s1 = CreatePositionedObj(obj, OBJF_FLAME_BREATH_PARTICLE);
            obj_s1->d.sprite.boxIdx = 3;
            obj_s1->x2.n = CV(0.28125) * rcos(OBJ.theta3) >> 12;
            obj_s1->z2.n = CV(0.28125) * rsin(OBJ.theta3) >> 12;
            obj_s1->x2.n += rand() % 13 - 6;
            obj_s1->z2.n += rand() % 13 - 6;
            obj_s1->y2.n = CV(0.15625) * rsin(DEG(67.5) * rsin(obj->state2) >> 12) >> 12;
            obj_s1->x3.n = obj_s1->x2.n >> 2;
            obj_s1->z3.n = obj_s1->z2.n >> 2;
            obj_s1->y3.n = obj_s1->y2.n >> 2;
            obj->state2 += 0x20;
            gCameraRotation.vy += DEG(1.40625) * rsin(OBJ.theta4 >> 1) >> 12;
            PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);
            gCameraZoom.vz = 512;
         }
      }

      OBJ.theta3 = obj_s4->d.objf335.todo_x28 + OBJ.theta1;
      OBJ.theta4 = obj_s4->d.objf335.todo_x2a + OBJ.theta2;

      iVar3 = OBJ.radius * rsin(OBJ.theta4) >> 12;
      vector.vy = iVar3;
      iVar3 = OBJ.radius * rcos(OBJ.theta4) >> 12;
      x = iVar3 * rcos(OBJ.theta3) >> 12;
      z = iVar3 * rsin(OBJ.theta3) >> 12;

      vector.vx = x;
      vector.vz = z;

      obj->x1.n = obj_s4->x1.n + (vector.vx);
      obj->z1.n = obj_s4->z1.n + (vector.vz);
      obj->y1.n = obj_s4->y1.n + (vector.vy);

      sprite->x1.n = obj->x1.n;
      sprite->z1.n = obj->z1.n;
      sprite->y1.n = obj->y1.n;
      AddObjPrim6(gGraphicsPtr->ot, sprite, 0);
      break;

   case 2:
      sprite = OBJ.sprite;
      sprite->functionIndex = OBJF_NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 331
void Objf331_RedXMark_Unused(Object *obj) {
   Object *parent = OBJ.parent;

   switch (obj->state) {
   case 0:
      OBJ.gfxIdx = GFX_RED_X_BTM_RIGHT;
      OBJ.boxIdx = 4;
      obj->state++;

   // fallthrough
   case 1:
      if (obj->state2 == 1) {
         obj->y1.n = parent->y1.n + (CV(1.5) * rsin(DEG(-45)) >> 12);
         obj->x1.n = parent->x1.n + CV(0.5);
         obj->z1.n = parent->z1.n;
      } else {
         obj->y1.n = parent->y1.n + (CV(1.5) * rsin(DEG(-135)) >> 12);
         obj->x1.n = parent->x1.n - CV(0.5);
         obj->z1.n = parent->z1.n;
      }

      AddObjPrim6(gGraphicsPtr->ot, obj, 0);
      break;
   }
}

#undef OBJF
#define OBJF 747
void Objf747_748_Wyrmfang_Flames(Object *obj) {
   Object *flame;

   if (obj->functionIndex == OBJF_WYRMFANG_FLAMES_CW) {
      OBJ.theta += DEG(8.4375);
      OBJ.radius += 8;
   } else {
      OBJ.theta -= DEG(8.4375);
      OBJ.radius += 8;
   }
   if (OBJ.radius > CV(6.0)) {
      OBJ.radius = 0;
   }

   OBJ.todo_x2c = (OBJ.todo_x2c + 280) & 0x7ff;
   OBJ.amplitude = 1200 * rcos((OBJ.todo_x2c - DEG(90)) & 0xfff) >> 12;

   switch (obj->state) {
   case 0:
      OBJ.timer = 1;
      if (obj->functionIndex == OBJF_WYRMFANG_FLAMES_CW) {
         OBJ.radius = 0;
      } else {
         OBJ.radius = CV(2.0);
      }
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer == 0) {
         OBJ.timer = 1;
         flame = CreatePositionedObj(obj, OBJF_WYRMFANG_FLAME);
         flame->d.objf749.theta = OBJ.theta;
         flame->d.objf749.radius = OBJ.radius;
         flame->d.objf749.amplitude = OBJ.amplitude;
      }
      break;
   }
}

// Used by: Objf332_RollingFire_FX1, Objf749_Wyrmfang_Flame
s16 gFlameAnimData_800ff8a4[20] = {0, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4,
                                   2, GFX_FLAME_5, 2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8,
                                   2, GFX_NULL,    1, GFX_NULL};

#undef OBJF
#define OBJF 749
void Objf749_Wyrmfang_Flame(Object *obj) {
   Object *sprite;

   sprite = OBJ.sprite;
   if (sprite != NULL) {
      UpdateObjAnimation(sprite);
      AddObjPrim3(gGraphicsPtr->ot, sprite);
   }

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.animData = gFlameAnimData_800ff8a4;
      sprite->d.sprite.semiTrans = 1;
      OBJ.sprite = sprite;
      sprite->d.sprite.coords[0].x =
          obj->x1.n + (rcos((OBJ.theta - DEG(11.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[0].z =
          obj->z1.n + (rcos((OBJ.theta + DEG(78.75)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[2].x = sprite->d.sprite.coords[0].x;
      sprite->d.sprite.coords[2].z = sprite->d.sprite.coords[0].z;
      sprite->d.sprite.coords[1].x =
          obj->x1.n + (rcos((OBJ.theta + DEG(11.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[1].z =
          obj->z1.n + (rcos((OBJ.theta + DEG(101.25)) & 0xfff) * OBJ.radius >> 12);
      sprite->d.sprite.coords[3].x = sprite->d.sprite.coords[1].x;
      sprite->d.sprite.coords[3].z = sprite->d.sprite.coords[1].z;
      sprite->d.sprite.coords[0].y = obj->y1.n;
      sprite->d.sprite.coords[1].y = obj->y1.n;
      sprite->d.sprite.coords[2].y = obj->y1.n;
      sprite->d.sprite.coords[3].y = obj->y1.n;
      sprite->x1.n = sprite->d.sprite.coords[2].x;
      sprite->z1.n = sprite->d.sprite.coords[2].z;
      sprite->y1.n = sprite->d.sprite.coords[2].y;
      obj->state++;

   // fallthrough
   case 1:
      OBJ.yTheta += DEG(4.21875);
      if (OBJ.yTheta > DEG(180)) {
         OBJ.yTheta = 0;
         obj->state++;
      }
      sprite->d.sprite.coords[0].y =
          obj->y1.n + (rcos((OBJ.yTheta - DEG(90)) & 0xfff) * OBJ.amplitude >> 12);
      sprite->d.sprite.coords[1].y = sprite->d.sprite.coords[0].y;
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      sprite->functionIndex = OBJF_NULL;
      break;
   }
}
