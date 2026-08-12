/* Map 33's collapsing lava-pit platform, plus the shared exploding-tile debris object.
 *
 * The platform is five tiles in a plus shape. Map33_Save/RestorePlatform stash and
 * reinstate their tile models through gScratch1_801317c0, and Map33_LowerPlatform() drops
 * them N steps at once for reloads. Objf651_Map33_LavaPitPlatform (spawned by
 * SetupMapExtras()) is the driver: each time Objf553_EvaluateBattle33 bumps gState.mapState
 * it focuses the camera on the unit standing there and spawns one more drop.
 *
 * Objf750_751_Map33_LowerPlatform performs that drop, one handler at two indices: as 751 it
 * grabs the unit sprite standing on the platform; as the scene-65 variant 750 it uses the
 * entity sprite the event script handed it. It rocks the saved tile copies with a sine
 * wobble while re-rendering a clone of the rider's sprite, then commits one 16-unit step.
 *
 * Objf653_ExplodingTile (OBJF_EXPLODING_TILE) is unrelated to Map 33 and lives here only by
 * address adjacency: the generic tumbling-tile debris used by Map 13's bridge and Map 35's.
 * It arcs a rotating copy of a tile model, bounces it, trails OBJF_EXPLOSION, and either
 * settles or -- on water -- throws an OBJF_SPLASH_WITH_DROPLETS and drifts downstream. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

static BVectorZX sPlatformLocations[5] = {{11, 10}, {10, 11}, {12, 11}, {11, 12}, {11, 11}};

void Map33_SavePlatform(void) {
   MapTileModel *pSavedTile;
   MapTileModel *pMapTile;
   BVectorZX *pLoc;
   s32 i;

   pSavedTile = (MapTileModel *)gScratch1_801317c0;
   pLoc = &sPlatformLocations[0];

   for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
      pMapTile = &gMapRowPointers[pLoc[i].z][pLoc[i].x];
      *pSavedTile = *pMapTile;
      pSavedTile++;
   }
}

void Map33_RestorePlatform(void) {
   MapTileModel *pSavedTile;
   MapTileModel *pMapTile;
   s32 i;

   pSavedTile = (MapTileModel *)gScratch1_801317c0;

   for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
      pMapTile = &gMapRowPointers[sPlatformLocations[i].z][sPlatformLocations[i].x];
      *pMapTile = *pSavedTile;
      pSavedTile++;
   }
}

void Map33_LowerPlatform(s32 steps) {
   s32 i;
   MapTileModel *pSavedTile;

   Map33_SavePlatform();
   pSavedTile = (MapTileModel *)gScratch1_801317c0;

   for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
      AdjustFaceElevation(pSavedTile, 0, steps * 16);
      AdjustFaceElevation(pSavedTile, 1, steps * 16);
      if (i < 4) {
         pSavedTile->vertices[pSavedTile->faces[2][1]].vy += steps * 16;
         pSavedTile->vertices[pSavedTile->faces[2][3]].vy += steps * 16;
      }
      pSavedTile++;
   }

   // The saved copy is modified above, and those changes will be adopted here:
   Map33_RestorePlatform();
}

#undef OBJF
#define OBJF 651
void Objf651_Map33_LavaPitPlatform(Object *obj) {
   s32 i;
   Object *unitSprite;
   Object *obj_v1;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 != 0) {
         Map33_LowerPlatform(gState.mapState.s.field_0x0);
         for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
            UpdateTileElevation(sPlatformLocations[i].z, sPlatformLocations[i].x);
         }
         OBJ.step = gState.mapState.s.field_0x0;
      }
      obj->x2.n = CV(11.5);
      obj->z2.n = CV(11.5);
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 > OBJ.step && OBJ.step < 10) {
         obj->state2 = 0;
         obj->state++;
         obj->state3 = 0;
         obj->mem = 128;
         obj->y3.n = 4;
         obj->y2.n = 24;
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(39));
         gPlayerControlSuppressed = 1;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         gCameraRotation.vy &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         unitSprite = GetUnitSpriteAtPosition(obj->z2.s.hi, obj->x2.s.hi);
         obj_v1 = Obj_GetUnused();
         obj_v1->functionIndex = OBJF_FOCUS_CAMERA;
         obj_v1->d.objf026.target = unitSprite;
         *(s16 *)(&obj_v1->d.objf026.type) = 1; //?
         obj_v1->d.objf026.zoom = 512;
         obj->state3 = 0;
         obj->state2++;
         break;

      case 1:
         if (++obj->state3 >= 32) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 2:
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 3:
      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_MAP33_LOWER_PLATFORM;
      obj->state++;
      break;

   case 4:
      if (--obj->mem <= 0) {
         OBJ.step++;
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 5:
      obj->state++;

   // fallthrough
   case 6:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         AssignToMainCamera(&OBJ.camera);
         obj->state++;
      }
      break;

   case 7:
      obj->state = 1;
      gPlayerControlSuppressed = 0;
      gState.field_0x96 = 0;
      break;
   }
}

#undef OBJF
#define OBJF 750
void Objf750_751_Map33_LowerPlatform(Object *obj) {
   s32 i, j;
   MapTileModel *pMapTile;
   MapTileModel *pSavedTile;
   u8 *pMapTileFaceVerts;
   u8 *pSavedTileFaceVerts;
   s16 dx, dy, dz;
   Object *clonedSprite;
   Object *unitSprite;

   switch (obj->state) {
   case 0:
      obj->x2.n = CV(11.5);
      obj->z2.n = CV(11.5);
      obj->mem = 128;
      obj->y3.n = CV(0.015625);
      obj->y2.n = CV(0.09375);
      PerformAudioCommand(AUDIO_CMD_PLAY_XA(39));
      Map33_SavePlatform();
      obj->state++;
      break;

   case 1:
      dx = (obj->mem >> 4) * rcos(obj->state3 * 16) >> 12;
      dz = (obj->mem >> 5) * rcos(obj->state3 * 8) >> 12;
      OBJ.ydrop += obj->y2.n;
      if (OBJ.ydrop > CV(0.5)) {
         OBJ.ydrop = CV(0.5);
         obj->y2.n = -(obj->y2.n / 2);
      }
      obj->y2.n += obj->y3.n;
      dy = OBJ.ydrop >> 3;

      for (i = 0, pSavedTile = (MapTileModel *)gScratch1_801317c0;
           i < ARRAY_COUNT(sPlatformLocations); i++) {
         pMapTile = &gMapRowPointers[sPlatformLocations[i].z][sPlatformLocations[i].x];
         pMapTileFaceVerts = &pMapTile->faces[0][0];
         pSavedTileFaceVerts = &pSavedTile->faces[0][0];
         for (j = 0; j < 4; j++) {
            pMapTile->vertices[pMapTileFaceVerts[j]].vx =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vx + dx;
            pMapTile->vertices[pMapTileFaceVerts[j]].vz =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vz + dz;
            pMapTile->vertices[pMapTileFaceVerts[j]].vy =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vy + dy;
         }
         pMapTileFaceVerts = &pMapTile->faces[1][0];
         pSavedTileFaceVerts = &pSavedTile->faces[1][0];
         for (j = 0; j < 4; j++) {
            pMapTile->vertices[pMapTileFaceVerts[j]].vx =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vx + dx;
            pMapTile->vertices[pMapTileFaceVerts[j]].vz =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vz + dz;
            pMapTile->vertices[pMapTileFaceVerts[j]].vy =
                pSavedTile->vertices[pSavedTileFaceVerts[j]].vy + dy;
         }
         if (i < 4) {
            pMapTile->vertices[pMapTile->faces[2][1]].vx =
                pSavedTile->vertices[pSavedTile->faces[2][1]].vx + dx;
            pMapTile->vertices[pMapTile->faces[2][1]].vz =
                pSavedTile->vertices[pSavedTile->faces[2][1]].vz + dz;
            pMapTile->vertices[pMapTile->faces[2][1]].vy =
                pSavedTile->vertices[pSavedTile->faces[2][1]].vy + dy;
            pMapTile->vertices[pMapTile->faces[2][3]].vx =
                pSavedTile->vertices[pSavedTile->faces[2][3]].vx + dx;
            pMapTile->vertices[pMapTile->faces[2][3]].vz =
                pSavedTile->vertices[pSavedTile->faces[2][3]].vz + dz;
            pMapTile->vertices[pMapTile->faces[2][3]].vy =
                pSavedTile->vertices[pSavedTile->faces[2][3]].vy + dy;
         }
         pSavedTile++;
      }

      clonedSprite = Obj_GetUnused();
      clonedSprite->functionIndex = OBJF_NOOP;
      if (obj->functionIndex == OBJF_MAP33_SCN65_LOWER_PLATFORM) {
         unitSprite = OBJ.variant_0x24.entitySpriteParam;
      } else {
         unitSprite = GetUnitSpriteAtPosition(obj->z2.s.hi, obj->x2.s.hi);
      }
      CopyObject(unitSprite, clonedSprite);
      unitSprite->d.sprite.hidden = 1;
      clonedSprite->d.sprite.hidden = 0;
      clonedSprite->x1.n = unitSprite->x1.n + (dx << 3);
      clonedSprite->z1.n = unitSprite->z1.n + (dz << 3);
      clonedSprite->y1.n = unitSprite->y1.n - (dy << 3);
      RenderUnitSprite(gGraphicsPtr->ot, clonedSprite, 1);
      clonedSprite->functionIndex = OBJF_NULL;

      obj->state3 += 8;
      if (--obj->mem <= 0) {
         OBJ.variant_0x24.unk++;
         obj->state3 = 0;
         unitSprite->d.sprite.hidden = 0;
         obj->state++;
      }
      break;

   case 2:
      pSavedTile = (MapTileModel *)gScratch1_801317c0;
      for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
         AdjustFaceElevation(pSavedTile, 0, 16);
         AdjustFaceElevation(pSavedTile, 1, 16);
         if (i < 4) {
            pSavedTile->vertices[pSavedTile->faces[2][1]].vy += 16;
            pSavedTile->vertices[pSavedTile->faces[2][3]].vy += 16;
         }
         pSavedTile++;
      }
      Map33_RestorePlatform();
      for (i = 0; i < ARRAY_COUNT(sPlatformLocations); i++) {
         gTerrainPtr[sPlatformLocations[i].z][sPlatformLocations[i].x].s.elevation =
             -gMapRowPointers[sPlatformLocations[i].z][sPlatformLocations[i].x].vertices[0].vy >> 4;
      }
      obj->state++;

   // fallthrough
   case 3:
      obj->state++;
      break;

   case 4:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 653
void Objf653_ExplodingTile(Object *obj) {
   MapTileModel transformedModel;
   MapTileModel *pBaseModel;
   s16 y, z, x;
   s16 elevation;
   Object *obj_s1;
   s32 theta;
   SVECTOR *pRot;
   SVECTOR *pTrans;

   memset(&transformedModel, 0, sizeof(MapTileModel));
   pBaseModel = OBJ.tileModel;
   transformedModel = *pBaseModel;

   switch (obj->state) {
   case 0:
      pTrans = &OBJ.translation;
      obj->x1.n = pTrans->vx;
      obj->y1.n = pTrans->vy;
      obj->z1.n = pTrans->vz;
      // memcpy(&OBJ.savedTranslation, &OBJ.translation, sizeof(SVECTOR));
      ((u32 *)&OBJ.savedTranslation)[0] = ((u32 *)pTrans)[0];
      ((u32 *)&OBJ.savedTranslation)[1] = ((u32 *)pTrans)[1];
      obj->state++;

   // fallthrough
   case 1:
      pTrans = &OBJ.translation;
      // memcpy(&OBJ.translation, &OBJ.savedTranslation, sizeof(SVECTOR));
      ((u32 *)pTrans)[0] = ((u32 *)&OBJ.savedTranslation)[0];
      ((u32 *)pTrans)[1] = ((u32 *)&OBJ.savedTranslation)[1];
      obj->x1.n = pTrans->vx;
      obj->y1.n = pTrans->vy;
      obj->z1.n = pTrans->vz;
      theta = (rand() >> 2) % DEG(360);
      obj->x2.n = CV(0.0625) * rcos(theta) >> 12;
      obj->z2.n = CV(0.0625) * rsin(theta) >> 12;
      obj->y2.n = CV(0.625);
      obj->y3.n = CV(-0.046875);
      obj->x3.n = 0;
      obj->z3.n = 0;
      obj->state2 = 3;
      obj->mem = 0;
      obj->state++;

   // fallthrough
   case 2:
      if (--obj->state3 <= 0) {
         obj->state3 = 0;
         obj->state++;
      }
      pBaseModel = OBJ.tileModel;
      pTrans = &OBJ.translation;
      pRot = &OBJ.rotation;
      pTrans->vx = obj->x1.n;
      pTrans->vy = obj->y1.n;
      pTrans->vz = obj->z1.n;
      RotTransMapTile(pBaseModel, pRot, pTrans, &transformedModel);
      RenderMapTile(gGraphicsPtr->ot, &transformedModel, GRID_COLOR_NONE);
      break;

   case 3:
      pBaseModel = OBJ.tileModel;
      pTrans = &OBJ.translation;
      pRot = &OBJ.rotation;
      obj->state3++;

      if (obj->state2 != 0) {
         pRot->vy = obj->state3 << 8; //* DEG(22.5);
         pRot->vz = obj->state3 << 8;
      }

      x = obj->x1.n;
      y = obj->y1.n;
      z = obj->z1.n;

      obj->y1.n += obj->y2.n;
      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y2.n += obj->y3.n;

      if (obj->mem != 0) {
         elevation = obj->y1.n;
      } else {
         elevation = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi) - CV(0.5);
      }

      if (elevation > obj->y1.n) {
         if (OBJ_TERRAIN(obj).s.terrain != TERRAIN_WATER) {
            obj->y1.n = elevation;
            obj->y2.n = -(obj->y2.n >> 1);
            if (obj->state2 != 0) {
               obj->state2--;
            } else {
               obj->x2.n = 0;
               obj->y2.n = 0;
               obj->z2.n = 0;
               obj->y3.n = 0;
               obj->state3 = 224;
            }
         } else {
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_SPLASH_WITH_DROPLETS;
            obj_s1->x1.n = obj->x1.n;
            obj_s1->z1.n = obj->z1.n;
            obj_s1->y1.n = elevation;
            obj_s1->y2.n = -obj->y2.n;
            obj->y1.n = elevation;
            obj->y2.n = 0;
            obj->y3.n = 0;
            obj->x2.n = 0;
            obj->z2.n = OBJ.riverVelocity;
            obj->state2 = 0;
            obj->state3 = 224;
            obj->mem = 1;
            return;
         }
      }

      if (obj->state3 >= 256) {
         obj->functionIndex = OBJF_NULL;
      }

      if ((obj->x1.s.hi < gMapMinX || obj->x1.s.hi > gMapMaxX) ||
          (obj->z1.s.hi < gMapMinZ || obj->z1.s.hi > gMapMaxZ)) {
         obj->functionIndex = OBJF_NULL;
      } else {
         pTrans->vx = obj->x1.n;
         pTrans->vy = obj->y1.n;
         pTrans->vz = obj->z1.n;
         RotTransMapTile(pBaseModel, pRot, &OBJ.translation, &transformedModel);
         RenderMapTile(gGraphicsPtr->ot, &transformedModel, GRID_COLOR_NONE);
         if (obj->state2 != 0) {
            obj_s1 = CreatePositionedObj(obj, OBJF_EXPLOSION);
            obj_s1->mem = 1;
            obj_s1->x2.n = (x - obj->x1.n) >> 2;
            obj_s1->z2.n = (z - obj->z1.n) >> 2;
            obj_s1->y2.n = ((y - obj->y1.n) >> 2) + (rand() >> 2) % 16;
         }
      }
      break;
   }
}
