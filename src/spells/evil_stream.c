/* Evil Stream skull-swarm FX2/FX3 (Objf307_324, Objf379 rock; FX1 is in casting_main.c) and
 * EvilStream_RenderSkull, dispatched via gSpellsEx (docs/decomp/spell-fx-dispatch.md).
 * Objf325/326 (a CLUT-cycling fade tick, a rising red-sparkle pillar) are cut content. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "battle.h"
#include "field.h"

/* spells/life_orb.c -- same TU before the split; prototype keeps the retail call codegen. */
void RenderCylinder(Cylinder *cylinder);

void EvilStream_RenderSkull(Object *evilStream, s16 height) {
   Object *skullSprite;
   s16 halfSize;

   skullSprite = CreatePositionedObj(evilStream, OBJF_NOOP);
   skullSprite->y1.n += 0x80;
   skullSprite->d.sprite.gfxIdx = GFX_SKULL;
   skullSprite->d.sprite.boxIdx = 7;
   height >>= 3;
   halfSize = 0x10;
   gQuad_800fe63c[0].vx = -halfSize;
   gQuad_800fe63c[1].vx = halfSize;
   gQuad_800fe63c[2].vx = -halfSize;
   gQuad_800fe63c[3].vx = halfSize;
   gQuad_800fe63c[0].vy = -height / 2;
   gQuad_800fe63c[1].vy = gQuad_800fe63c[0].vy;
   gQuad_800fe63c[2].vy = height / 2;
   gQuad_800fe63c[3].vy = gQuad_800fe63c[2].vy;
   gQuad_800fe63c[0].vz = 0;
   gQuad_800fe63c[1].vz = 0;
   gQuad_800fe63c[2].vz = 0;
   gQuad_800fe63c[3].vz = 0;
   AddObjPrim6(gGraphicsPtr->ot, skullSprite, 0);
   skullSprite->functionIndex = OBJF_NULL;
}

void Objf325_ClutCycleFadeSprite_Unused(Object *obj) {
   POLY_FT4 *poly;

   obj->d.sprite.clut = 3 + obj->state2 % 3;
   AddObjPrim6(gGraphicsPtr->ot, obj, 0);
   if (obj->d.sprite.semiTrans != 0) {
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      setRGB0(poly, obj->state2, obj->state2, obj->state2);
   }
   if (--obj->state2 <= 0) {
      obj->functionIndex = OBJF_NULL;
   }
}

#undef OBJF
#define OBJF 307
void Objf307_324_EvilStream_FX2_FX3(Object *obj) {
   Object *obj_s1;
   Object *fxSprite;
   s32 rnd;
   Cylinder *cylinder;

   switch (obj->state) {
   case 0:
      obj_s1 = SnapToUnit(obj);
      fxSprite = Obj_GetUnused();
      CopyObject(obj_s1, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.hidden = 1;
      OBJ.sprite = fxSprite;
      OBJ.targetSprite = obj_s1;
      obj->mem = 1;

      // TODO: union?
      *(s32 *)&gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1] =
          *(s32 *)&gGfxSubTextures[GFX_TILED_RED_SPARKLES];

      cylinder = &OBJ.cylinder;
      cylinder->top.vx = cylinder->bottom.vx = obj->x1.n;
      cylinder->top.vz = cylinder->bottom.vz = obj->z1.n;
      cylinder->bottom.vy = obj->y1.n;
      cylinder->top.vy = cylinder->bottom.vy + 0x400;
      cylinder->sideCt = 16;
      cylinder->semiTrans = 2;
      cylinder->gfxIdx = GFX_TILED_RED_SPARKLES_DYN_1;
      cylinder->topRadius = 0;
      cylinder->bottomRadius = 0;
      cylinder->useColor = 1;
      cylinder->color.r = cylinder->color.g = cylinder->color.b = 0;
      cylinder->clut = CLUT_REDS;

      obj->state2 = 0x80;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 1:
      cylinder = &OBJ.cylinder;

      switch (obj->mem) {
      case 0:
         cylinder->topRadius += (CV(0.75) - cylinder->topRadius) >> 4;
         cylinder->bottomRadius = cylinder->topRadius;
         cylinder->color.b += (0x40 - cylinder->color.b) >> 3;
         cylinder->color.r += (0x40 - cylinder->color.r) >> 3;
         cylinder->color.g += (0x40 - cylinder->color.g) >> 3;
         gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = -0x60 - obj->state2 * 8 % 0x20;
         RenderCylinder(cylinder);

         gLightColor.r -= (gLightColor.r - 0x10) >> 4;
         gLightColor.g -= (gLightColor.g - 0x10) >> 4;
         gLightColor.b -= (gLightColor.b - 0x10) >> 4;

         if (obj->state2 % 2 == 0) {
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_EVIL_STREAM_ROCK;
            rnd = (rand() >> 2) % DEG(360);
            obj_s1->x1.n = obj->x1.n + (CV(0.5) * rcos(rnd) >> 12);
            obj_s1->z1.n = obj->z1.n + (CV(0.5) * rsin(rnd) >> 12);
            obj_s1->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
            obj_s1->y2.n = 0x20 + (rand() >> 2) % 0x20; // y velocity
            obj_s1->d.objf379.maxHeight = obj_s1->y1.n + CV(8.0);
         }
         if (--obj->state2 <= 32) {
            obj->mem++;
         }
         break;

      case 1:
         cylinder->topRadius += (0 - cylinder->topRadius) >> 4;
         cylinder->bottomRadius = cylinder->topRadius;
         cylinder->color.b += (0xff - cylinder->color.b) >> 3;
         cylinder->color.r += (0xff - cylinder->color.r) >> 3;
         cylinder->color.g += (0xff - cylinder->color.g) >> 3;
         gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = -0x60 - obj->state2 * 8 % 0x20;
         cylinder->clut = 3 + obj->state2 % 3;
         RenderCylinder(cylinder);
         if (--obj->state2 <= 0) {
            obj->mem++;
         }
         break;

      case 2:
         obj->state++;
         break;
      }

      fxSprite = OBJ.sprite;
      obj_s1 = GetUnitSpriteAtPosition(obj->z1.s.hi, obj->x1.s.hi);
      CopyObject(obj_s1, fxSprite);
      fxSprite->functionIndex = OBJF_NOOP;
      obj_s1->d.sprite.hidden = 1;
      fxSprite->d.sprite.hidden = 0;
      fxSprite->x1.n = obj_s1->x1.n + rand() % 0x41 - 0x20;
      fxSprite->y1.n = obj_s1->y1.n;
      RenderUnitSprite(gGraphicsPtr->ot, fxSprite, 0);
      if (obj->state2 >= 0x20 && obj->state2 <= 0x80) {
         EvilStream_RenderSkull(obj, obj->state2 << 3);
      }
      break;

   case 2:
      fxSprite = OBJ.sprite;
      fxSprite->functionIndex = OBJF_NULL;
      obj_s1 = OBJ.targetSprite;
      obj_s1->d.sprite.hidden = 0;
      if (obj->functionIndex == OBJF_EVIL_STREAM_FX3) {
         obj_s1 = CreatePositionedObj(obj, OBJF_ENGULF_FLAME_SLAY);
         obj_s1->d.objf134.clut = CLUT_REDS;
      } else { // OBJF_EVIL_STREAM_FX2:
         obj_s1 = CreatePositionedObj(obj, OBJF_ENGULF_FLAME_DAMAGE);
         obj_s1->d.objf132.clut = CLUT_REDS;
      }
      obj->functionIndex = OBJF_NULL;
      gLightColor.r = gLightColor.g = gLightColor.b = 0x80;
      break;
   }
}

#undef OBJF
#define OBJF 326
void Objf326_RisingSparklePillar_Unused(Object *obj) {
   Object *sprite;
   Object *dataStore;
   Cylinder *dsCylinder;
   s32 i;
   s32 radius;

   switch (obj->state) {
   case 0:
      sprite = Obj_GetUnused();
      sprite->functionIndex = OBJF_NOOP;
      sprite->d.sprite.gfxIdx = GFX_TILED_RED_SPARKLES_DYN_1;
      sprite->d.sprite.clut = CLUT_REDS;
      sprite->d.sprite.semiTrans = 2;
      OBJ.sprite = sprite;

      OBJ.height = 0;
      OBJ.speed = 0x20;
      OBJ.radius = 2;

      // TODO: union?
      *(s32 *)&gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1] =
          *(s32 *)&gGfxSubTextures[GFX_TILED_RED_SPARKLES];

      obj->x2.n = obj->x1.n;
      obj->z2.n = obj->z1.n;
      obj->y2.n = obj->y1.n;

      dataStore = Obj_GetUnused();
      dataStore->functionIndex = OBJF_NOOP;
      OBJ.dataStore = dataStore;

      dsCylinder = &dataStore->d.dataStore.cylinder;
      dsCylinder->sideCt = 16;
      dsCylinder->semiTrans = 2;
      dsCylinder->gfxIdx = GFX_TILED_RED_SPARKLES_DYN_1;
      dsCylinder->topRadius = 2;
      dsCylinder->bottomRadius = 2;
      dsCylinder->bottom.vx = obj->x1.n;
      dsCylinder->bottom.vz = obj->z1.n;
      dsCylinder->bottom.vy = obj->y1.n;
      dsCylinder->top.vx = obj->x1.n;
      dsCylinder->top.vz = obj->z1.n;
      dsCylinder->top.vy = obj->y1.n;
      dsCylinder->useColor = 1;

      obj->state++;

   // fallthrough
   case 1:
      dataStore = OBJ.dataStore;
      dsCylinder = &dataStore->d.dataStore.cylinder;
      OBJ.radius += (CV(0.75) - OBJ.radius) >> 4;
      if (OBJ.height < CV(4.0)) {
         OBJ.height += OBJ.speed;
      }
      dsCylinder->top.vy = obj->y1.n + OBJ.height;
      dsCylinder->bottomRadius = dsCylinder->topRadius = OBJ.radius;
      dsCylinder->color.r = 0x40;
      dsCylinder->color.g = 0x40;
      dsCylinder->color.b = 0x40;
      RenderCylinder(dsCylinder);

      gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = -0x60 - obj->state2 * 16 % 0x20;
      sprite = OBJ.sprite;

      for (i = 0; i < 16; i++) {
         radius = OBJ.radius;
         gGfxSubTextures[GFX_TILED_RED_SPARKLES_DYN_1][1] = -0x60 - obj->state2 * i % 0x20;

         sprite->d.sprite.coords[0].x = sprite->d.sprite.coords[2].x =
             obj->x1.n + (radius * rcos(i * 0x100) >> 12);
         sprite->d.sprite.coords[1].x = sprite->d.sprite.coords[3].x =
             obj->x1.n + (radius * rcos((i + 1) * 0x100) >> 12);
         sprite->d.sprite.coords[0].z = sprite->d.sprite.coords[2].z =
             obj->z1.n + (radius * rsin(i * 0x100) >> 12);
         sprite->d.sprite.coords[1].z = sprite->d.sprite.coords[3].z =
             obj->z1.n + (radius * rsin((i + 1) * 0x100) >> 12);
         sprite->d.sprite.coords[0].y = sprite->d.sprite.coords[1].y = obj->y1.n + OBJ.height;
         sprite->d.sprite.coords[2].y = sprite->d.sprite.coords[3].y = obj->y1.n;

         sprite->x1.n = sprite->d.sprite.coords[0].x;
         sprite->z1.n = sprite->d.sprite.coords[0].z;
         sprite->y1.n = sprite->d.sprite.coords[0].y;
      }

      if (--obj->state2 <= 0) {
         sprite->functionIndex = OBJF_NULL;
         obj->functionIndex = OBJF_NULL;
      }

      break;
   }
}

#undef OBJF
#define OBJF 379
void Objf379_EvilStream_Rock(Object *obj) {
   extern s16 gRockAnimData_800ff670[12];

   switch (obj->state) {
   case 0:
      OBJ.boxIdx = 4;
      OBJ.animData = gRockAnimData_800ff670;
      obj->state++;

   // fallthrough
   case 1:
      obj->y1.n += obj->y2.n;
      if (obj->y1.n > OBJ.maxHeight) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }

   UpdateObjAnimation(obj);
   AddObjPrim6(gGraphicsPtr->ot, obj, 0);
}

void Objf329_Noop(Object *obj) {}
