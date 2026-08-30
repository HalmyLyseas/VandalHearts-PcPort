/* Map 28's gate (Objf663 button -> Objf662 door; Map28_RemoveDoor() on reload) and the shared
 * flame sprite Objf304_661_Flame: a static torch as OBJF_FLAME, or the orbiting Xeno flame that
 * Objf303_Map31_Scn61_XenoFlames rings around an entity sprite (one per 8.4375 degrees). */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

#undef OBJF
#define OBJF 662
void Objf662_Map28_OpenDoor(Object *obj) {
   s32 i;
   s32 ix, iz;
   Object *smoke;
   s32 tmp;

   gCameraRotation.vy -= DEG(0.703125);

   switch (obj->state) {
   case 0:
      obj->state++;
      obj->mem = 48;
      obj->y2.n = gCameraPos.vy;
      obj->z3.n = 3;
      obj->state2 = 1;

   // fallthrough
   case 1:
      if (obj->mem >= obj->state2) {
         ix = 13;
         for (iz = 13; iz <= 15; iz++) {
            AdjustFaceElevation(&gMapRowPointers[iz][ix], 0, obj->state2);
         }
         obj->y2.n -= obj->state2;
         obj->mem -= obj->state2;
      } else {
         ix = 13;
         for (iz = 13; iz <= 15; iz++) {
            AdjustFaceElevation(&gMapRowPointers[iz][ix], 0, obj->mem);
         }
         obj->y2.n -= obj->mem;
         obj->mem = 0;
         obj->state++;
      }

      if (--obj->z3.n <= 0) {
         for (i = 0; i < 7; i++) {
            smoke = Obj_GetUnused();
            smoke->functionIndex = OBJF_SMOKE;
            smoke->x1.n = CV(13.0);
            smoke->z1.n = CV(13.0) + i * CV(0.5);
            smoke->y1.n = CV(0.5);
            smoke->x2.n = -rand() % CV(0.25);
            smoke->z2.n = rand() % CV(0.25) - CV(0.125);
            smoke->y2.n = rand() % CV(0.25);

            smoke = Obj_GetUnused();
            smoke->functionIndex = OBJF_SMOKE;
            smoke->x1.n = CV(14.0);
            smoke->z1.n = CV(13.0) + i * CV(0.5);
            smoke->y1.n = CV(0.5);
            smoke->x2.n = rand() % CV(0.25);
            smoke->z2.n = rand() % CV(0.25) - CV(0.125);
            smoke->y2.n = rand() % CV(0.25);
         }
         obj->z3.n += (rand() % 5 + 3);
      }

      gCameraPos.vy = obj->y2.n + rand() % 4;

      for (i = 194; i < 197; i++) {
         gGfxSubTextures[GFX_MAP_TEXTURES_OFS + i][3] = obj->mem >> 1;
      }
      break;

   case 2:
      gCameraPos.vy = obj->y2.n + rand() % 2;
      if (++obj->mem >= 32) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 3:
      //?
      rand();
      tmp = obj->y2.n;
      gCameraPos.vy = tmp;
      if (++obj->mem >= 32) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 4:
      for (iz = 13; iz <= 15; iz++) {
         gTerrainPtr[iz][13].s.elevation = -gMapRowPointers[iz][13].vertices[0].vy >> 4;
      }
      obj->functionIndex = OBJF_NULL;
      gCameraPos.vy = obj->y2.n;
      break;

   case 5:
      break;
   }
}

void Map28_RemoveDoor(void) {
   s32 iz, ix;

   ix = 13;
   for (iz = 13; iz <= 15; iz++) {
      AdjustFaceElevation(&gMapRowPointers[iz][ix], 0, 48);
      gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
   }
}

#undef OBJF
#define OBJF 663
void Objf663_Map28_Button(Object *obj) {
   Object *child;

   switch (obj->state) {
   case 0:
      obj->x1.n = CV(13.5);
      obj->z1.n = CV(14.5);
      obj->y1.n = GetTerrainElevation(14, 13);
      obj->state++;
      if (gState.mapState.s.field_0x0 != 0) {
         Map28_RemoveDoor();
         DepressButton(2, 19);
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 1:
      if (gState.mapState.s.field_0x0 != 0) {
         obj->state2 = 0;
         obj->state++;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         AssignFromMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 1;
         obj->state2++;
         break;

      case 1:
         child = Obj_GetUnused();
         child->functionIndex = OBJF_BUTTON_DEPRESS;
         child->x1.s.hi = 2;
         child->z1.s.hi = 19;
         OBJ.child = child;
         obj->state2++;
         break;

      case 2:
         child = OBJ.child;
         if (child->state == 99) {
            obj->state3 = 0;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(42));
            obj->state2++;
         }
         break;

      case 3:
         PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
         if (++obj->state3 >= 48) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(42));
            obj->state2++;
         }
         break;

      case 4:
         child = Obj_GetUnused();
         child->functionIndex = OBJF_MAP28_OPEN_DOOR;
         OBJ.child = child;
         obj->state2++;
         break;

      case 5:
         child = OBJ.child;
         if (child->functionIndex != OBJF_MAP28_OPEN_DOOR) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 6:
         obj->state2++;

      // fallthrough
      case 7:
         EaseOutCamera(&OBJ.camera, 2);
         if (++obj->state3 >= 32) {
            AssignToMainCamera(&OBJ.camera);
            obj->state2++;
         }
         break;

      case 8:
         gPlayerControlSuppressed = 0;
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 3:
      gState.mapState.s.field_0x0 = 1;
      gState.signal = 0;
      obj->state++;
      break;

   case 4:
      break;
   }
}

#undef OBJF
#define OBJF 304
void Objf304_661_Flame(Object *obj) {
   static s16 flameAnimData_Fast[20] = {
       7, GFX_FLAME_1, 1, GFX_FLAME_2, 1, GFX_FLAME_3, 1, GFX_FLAME_4, 1, GFX_FLAME_5,
       1, GFX_FLAME_6, 1, GFX_FLAME_7, 1, GFX_FLAME_8, 1, GFX_NULL,    1, GFX_NULL};
   static s16 flameAnimData_Med[20] = {
       7, GFX_FLAME_1, 2, GFX_FLAME_2, 2, GFX_FLAME_3, 2, GFX_FLAME_4, 2, GFX_FLAME_5,
       2, GFX_FLAME_6, 2, GFX_FLAME_7, 2, GFX_FLAME_8, 2, GFX_NULL,    1, GFX_NULL};
   static s16 flameAnimData_Slow[20] = {
       7, GFX_FLAME_1, 3, GFX_FLAME_2, 3, GFX_FLAME_3, 3, GFX_FLAME_4, 3, GFX_FLAME_5,
       3, GFX_FLAME_6, 3, GFX_FLAME_7, 3, GFX_FLAME_8, 3, GFX_NULL,    1, GFX_NULL};
   static Quad quad = {{-16, -64, 0, 0}, {16, -64, 0, 0}, {-16, 0, 0, 0}, {16, 0, 0, 0}};
   static Quad quad_unused = {{-8, -32, 0, 0}, {8, -32, 0, 0}, {-8, 0, 0, 0}, {8, 0, 0, 0}};
   static s16 smokeAnimData_unused[24] = {
       4, GFX_PUFF_1, 2, GFX_PUFF_2, 2, GFX_PUFF_3, 2, GFX_PUFF_4,  2, GFX_PUFF_5, 2, GFX_PUFF_6,
       2, GFX_PUFF_7, 2, GFX_PUFF_8, 2, GFX_PUFF_9, 2, GFX_PUFF_10, 2, GFX_NULL,   1, GFX_NULL};

   void *animations[3] = {flameAnimData_Fast, flameAnimData_Med, flameAnimData_Slow};
   s32 i, skip;
   s32 prevQuadIdx;
   POLY_FT4 *poly;
   Quad *qswap;
   u8 fade;

   switch (obj->state) {
   case 0:
      obj->mem = rand() % 5;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->mem <= 0) {
         obj->state++;
      }
      break;

   case 2:
      if (obj->functionIndex == OBJF_MAP31_SCN61_XENO_FLAME) {
         OBJ.animData = animations[rand() % 3];
         OBJ.quadp = &quad;
         skip = rand() % 5;
         for (i = 0; i < skip; i++) {
            UpdateObjAnimation(obj);
         }
      } else {
         OBJ.animData = animations[rand() % 3];
         OBJ.quadp = &quad;
         OBJ.semiTrans = 2;
      }
      obj->state++;
      break;

   default:
   case 3:
      if (obj->functionIndex == OBJF_MAP31_SCN61_XENO_FLAME) {
         if (gState.mapState.s.field_0x0 == 1) {
            OBJ.radius--;
            if (OBJ.radius <= CV(3.0))
               OBJ.radius = CV(3.0);
         }
         obj->x1.n = obj->x2.n + (OBJ.radius * rcos(OBJ.theta) >> 12);
         obj->z1.n = obj->z2.n + (OBJ.radius * rsin(OBJ.theta) >> 12);
         obj->y1.n = obj->y2.n;
         if (gState.mapState.s.field_0x0 == 2) {
            obj->mem = 128;
            obj->state++;
         }
      }
      qswap = gSpriteBoxQuads[7];
      gSpriteBoxQuads[7] = OBJ.quadp;
      UpdateObjAnimation(obj);
      AddObjPrim7(gGraphicsPtr->ot, obj, 0);
      gSpriteBoxQuads[7] = qswap;
      break;

   case 4:
      prevQuadIdx = gQuadIndex;
      qswap = gSpriteBoxQuads[7];
      gSpriteBoxQuads[7] = OBJ.quadp;
      UpdateObjAnimation(obj);
      AddObjPrim7(gGraphicsPtr->ot, obj, 0);
      gSpriteBoxQuads[7] = qswap;
      if (prevQuadIdx != gQuadIndex) {
         poly = &gGraphicsPtr->quads[gQuadIndex - 1];
         fade = obj->mem;
         setRGB0(poly, fade, fade, fade);
      }
      obj->mem -= 4;
      if (obj->mem <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 303
void Objf303_Map31_Scn61_XenoFlames(Object *obj) {
   Object *entitySprite;
   Object *flame;
   s32 i;
   s32 rnd;

   switch (obj->state) {
   case 0:
      OBJ.theta = DEG(0);
      obj->state++;
      entitySprite = OBJ.entitySprite;
      obj->x1.n = entitySprite->x1.n;
      obj->z1.n = entitySprite->z1.n;
      obj->y1.n = entitySprite->y1.n;
      gState.mapState.s.field_0x0 = 0;
      obj->y3.n = gCameraPos.vy;

   // fallthrough
   case 1:
      for (i = 0; i < 1; i++) {
         flame = Obj_GetUnused();
         flame->functionIndex = OBJF_MAP31_SCN61_XENO_FLAME;
         flame->x1.n = obj->x1.n + (CV(4.0) * rcos(OBJ.theta + DEG(180)) >> 12);
         flame->z1.n = obj->z1.n + (CV(4.0) * rsin(OBJ.theta + DEG(180)) >> 12);
         flame->y1.n = GetTerrainElevation(flame->z1.s.hi, flame->x1.s.hi);
         flame->x2.n = obj->x1.n;
         flame->z2.n = obj->z1.n;
         flame->y2.n = obj->y1.n;
         flame->d.objf304.semiTrans = 2;
         flame->d.objf304.radius = CV(4.0);
         flame->d.objf304.theta = OBJ.theta + DEG(180);
      }
      OBJ.theta += DEG(8.4375);
      rnd = rand();
      gCameraPos.vy = obj->y3.n - 12;
      gCameraPos.vy += rnd % 24;
      if (OBJ.theta > DEG(360)) {
         gCameraPos.vy = obj->y3.n;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}
