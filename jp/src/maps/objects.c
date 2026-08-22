#include "common.h"
#include "object.h"
#include "field.h"
#include "units.h"
#include "graphics.h"
#include "state.h"
#include "field.h"
#include "battle.h"
#include "window.h"
#include "audio.h"

void Objf035_MapObject_Tree(Object *);
void Objf036_MapObject_GraveMarker(Object *);
void Objf415_MapObject_Torch(Object *);
void Objf037_MapObject_Fountain(Object *);
void Objf038_MapObject_LampPost(Object *);
void Objf039_MapObject_Flag(Object *);
void Objf045_BloodSpurtParticleOffset(Object *);
void Objf591_MapObject_Boulder(Object *);
void Objf046_MapObject_Crate(Object *);
s32 ScanForCrateDestination(Object *);
s32 ScanForBoulderDestination(Object *);
void UpdateCrateElevation(Object *);
void Objf020_PushedBoulder(Object *);
void Objf048_Push(Object *);
void TiltChestLid(Object *, s16, u8);
void Objf040_MapObject_Chest(Object *);

#undef OBJF
#define OBJF 035
void Objf035_MapObject_Tree(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.boxIdx = 10;
      OBJ.gfxIdx = OBJ.param;
      OBJ.param = 0;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      OBJ_TILE_MODEL(obj).height = (-OBJ_TILE_MODEL(obj).vertices[0].vy >> 4) + 4;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, (gState.mapNum == 14));
   }
}

#undef OBJF
#define OBJF 036
void Objf036_MapObject_GraveMarker(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.boxIdx = 18;
      OBJ.gfxIdx = GFX_GRAVE_MARKER;
      OBJ.param = 0;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      OBJ_TILE_MODEL(obj).height = (-OBJ_TILE_MODEL(obj).vertices[0].vy >> 4) + 3;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, 0);
   }
}

#undef OBJF
#define OBJF 415
void Objf415_MapObject_Torch(Object *obj) {
   static s16 torchAnimData[] = {18, GFX_TORCH_1, 3, GFX_TORCH_2, 3, GFX_TORCH_3,
                                 3,  GFX_TORCH_4, 3, GFX_NULL,    3, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ.animData = torchAnimData;
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      OBJ_TILE_MODEL(obj).height = (-OBJ_TILE_MODEL(obj).vertices[0].vy >> 4) + 3;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   UpdateObjAnimation(obj);

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, 0);
   }
}

#undef OBJF
#define OBJF 037
void Objf037_MapObject_Fountain(Object *obj) {
   static s16 fountainAnimData[] = {12, GFX_FOUNTAIN_1, 1, GFX_FOUNTAIN_2,
                                    1,  GFX_NULL,       1, GFX_NULL};

   switch (obj->state) {
   case 0:
      OBJ.animData = fountainAnimData;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   UpdateObjAnimation(obj);

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, 0);
   }
}

#undef OBJF
#define OBJF 038
void Objf038_MapObject_LampPost(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.boxIdx = 13;
      OBJ.gfxIdx = GFX_LAMP_POST;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      OBJ_TILE_MODEL(obj).height = (-OBJ_TILE_MODEL(obj).vertices[0].vy >> 4) + 5;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, 0);
   }
}

#undef OBJF
#define OBJF 039
void Objf039_MapObject_Flag(Object *obj) {
   static s16 flagAnimData[] = {14, GFX_FLAG_1, 2, GFX_FLAG_2, 2, GFX_FLAG_3,
                                2,  GFX_FLAG_4, 2, GFX_NULL,   1, GFX_NULL};

   switch (obj->state) {
   case 0:
      OBJ.animData = flagAnimData;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_NO_ENTRY;
      OBJ_TILE_MODEL(obj).height = (-OBJ_TILE_MODEL(obj).vertices[0].vy >> 4) + 6;
      SetElevationFromTerrain(obj);
      obj->state++;
      break;
   }

   UpdateObjAnimation(obj);

   if (gOverheadMapState == 0) {
      AddObjPrim8(gGraphicsPtr->ot, obj, 0);
   }
}

void Objf045_BloodSpurtParticleOffset(Object *obj) {
   // Used during battle 32 to simulate train movement
   obj->x1.n -= CV(0.1875);
   Objf206_686_BloodSpurtParticle(obj);
}

#undef OBJF
#define OBJF 591
void Objf591_MapObject_Boulder(Object *obj) {
   // TBD: Are the static inlines necessary? GetCamRotY_U etc.
   // obj->x2: dx;    obj->y2: dy;    obj->z2: dz;
   // obj->x3: dstX;  obj->y3: dstY;  obj->z3: dstZ;
   s32 shouldDraw;
   s16 diff;
   Object *newObj;
   u8 unused[40];

   shouldDraw = !IsSpriteOutsideVisibleRange(obj);
   if (gOverheadMapState != 0) {
      shouldDraw = 1;
   }

   switch (obj->state) {
   case 0:
      OBJ.boxIdx = 15;
      OBJ.gfxIdx = GFX_BOULDER_1;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      obj->x3.n = obj->x1.n;
      obj->z3.n = obj->z1.n;
      OBJ.gridZ = obj->z1.s.hi;
      OBJ.gridX = obj->x1.s.hi;
      SetElevationFromTerrain(obj);
      OBJ_MAP_UNIT(obj).s.team = TEAM_BOULDER;
      obj->state++;

   // fallthrough
   case 1:
      if (OBJ_TILE_STATE(obj).action == TA_BOULDER_PUSHED) {
         if (gState.mapNum == 40 && gState.mapState.s.field_0x0 == 0) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(112));
            newObj = Obj_GetUnused();
            newObj->functionIndex = OBJF_AUDIO_CMD;
            newObj->d.objf581.cmd = AUDIO_CMD_PLAY_XA(112);
            newObj->d.objf581.delay = 10;
         } else {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(111));
         }
         obj->state++;
         obj->state2 = 0;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         OBJ.savedCamRotY = gCameraRotation.vy = GetCamRotY_U() & 0xfff;
         OBJ_MAP_UNIT(obj).s.team = TEAM_NULL;
         OBJ.dstCamRotY = GetBestViewOfTarget(obj->z3.s.hi, obj->x3.s.hi, 1);
         diff = OBJ.dstCamRotY - GetCamRotY_U();
         if (diff > 0) {
            if (diff > DEG(180)) {
               OBJ.dstCamRotY -= DEG(360);
            }
         } else if (diff < DEG(-180)) {
            OBJ.dstCamRotY += DEG(360);
         }

         OBJ_TILE_STATE(obj).action = TA_NONE;
         ScanForBoulderDestination(obj);

         obj->x2.n = 0;
         obj->z2.n = 0;
         if (obj->x3.s.hi > obj->x1.s.hi) {
            obj->x2.n = CV(0.25);
         }
         if (obj->x3.s.hi < obj->x1.s.hi) {
            obj->x2.n = CV(-0.25);
         }
         if (obj->z3.s.hi > obj->z1.s.hi) {
            obj->z2.n = CV(0.25);
         }
         if (obj->z3.s.hi < obj->z1.s.hi) {
            obj->z2.n = CV(-0.25);
         }
         obj->state2++;

      // fallthrough
      case 1:
         newObj = Obj_GetUnused();
         newObj->functionIndex = OBJF_CLOUD;
         newObj->x1.n = obj->x1.n;
         newObj->z1.n = obj->z1.n;
         newObj->y1.n = obj->y1.n;
         obj->x1.n += obj->x2.n;
         obj->z1.n += obj->z2.n;
         if (OBJ.gridZ != obj->z1.s.hi || OBJ.gridX != obj->x1.s.hi) {
            OBJ.gridZ = obj->z1.s.hi;
            OBJ.gridX = obj->x1.s.hi;
            if (OBJ_MAP_UNIT(obj).s.unitIdx != 0) {
               OBJ_TILE_STATE(obj).action = TA_BOULDER_HIT;
            }
         }
         if (obj->x1.n == obj->x3.n && obj->z1.n == obj->z3.n) {
            obj->state++;
         }
         break;
      } // switch (obj->state2) (via state:2)

      OffsetCamPosX((-(obj->x1.n >> 3) - GetCamPosX()) >> 2);
      OffsetCamPosZ((-(obj->z1.n >> 3) - GetCamPosZ()) >> 2);
      OffsetCamPosY(((obj->y1.n >> 3) - GetCamPosY()) >> 2);
      OffsetCamRotY((OBJ.dstCamRotY - GetCamRotY()) >> 3);
      break;

   case 3:
      newObj = Obj_GetUnused();
      if (OBJ_TERRAIN(obj).s.terrain == TERRAIN_WATER ||
          OBJ_TERRAIN(obj).s.terrain == TERRAIN_LAVA) {
         newObj->functionIndex = OBJF_PUSHED_OBJECT_SPLASH;
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(234));
      } else {
         newObj->functionIndex = OBJF_BOULDER_RUBBLE;
         if (!(gState.mapNum == 40 && gState.mapState.s.field_0x0 == 0)) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(111));
         }
      }
      newObj->x1.n = obj->x1.n;
      newObj->z1.n = obj->z1.n;
      newObj->y1.n = obj->y1.n;
      OBJ.hidden = 1;
      OBJ.timer = 70;
      obj->state++;

   // fallthrough
   case 4:
      if (gState.mapNum == 40 && gState.mapState.s.field_0x0 == 0) {
         // Destroyed barricade
         gState.mapState.s.field_0x0 = 1;
         gState.field_0x96 = 1;
         OBJ.timer = 120;
         obj->state++;
      }
      if (--OBJ.timer == 0) {
         OBJ.timer = 20;
         gSignal5 = 3;
         obj->state += 2;
      }
      break;

   case 5:
      if (--OBJ.timer == 0) {
         OBJ.timer = 20;
         gSignal5 = 3;
         obj->state++;
      }
      break;

   case 6:
      if (--OBJ.timer == 0) {
         gCameraRotation.vy = GetCamDir() * DEG(90) + DEG(45);
         gPlayerControlSuppressed = 0;
         obj->functionIndex = OBJF_NULL;
         return;
      }

      OffsetCamRotY((OBJ.savedCamRotY - GetCamRotY()) >> 2);
      break;
   }

   if (obj->state != 1) {
      obj->y1.n += (OBJ_TERRAIN(obj).s.elevation * CV(0.5) - obj->y1.n) >> 2;
   }

   if (shouldDraw) {
      AddObjPrim7(gGraphicsPtr->ot, obj, 0);
   }
}

typedef struct CubeSideOrdering {
   s16 west;
   s16 east;
   s16 north;
   s16 south;
} CubeSideOrdering;

#undef OBJF
#define OBJF 046
void Objf046_MapObject_Crate(Object *obj) {
   static CubeSideOrdering sideOrdering[4] = {
       {-1, 0, 2, 0}, {2, 0, 0, -1}, {0, -1, 0, 2}, {0, 2, -1, 0}};

   // obj->x2: dx;    obj->y2: dy;    obj->z2: dz;
   // obj->x3: dstX;  obj->y3: dstY;  obj->z3: dstZ;
   Object *face;
   Object *obj1;
   s32 dir;
   s16 x1, x2, z1, z2;
   s32 shouldDraw;
   POLY_FT4 *poly;
   s32 i;

   dir = (gCameraRotation.vy & 0xfff) >> 10;
   face = gTempGfxObj;
   //?
   *(s16 *)(&face->d.sprite.hidden) = 0;
   face->d.sprite.gfxIdx = GFX_CRATE;
   face->d.sprite.clut = CLUT_NULL;
   face->d.sprite.otOfs = -6;

   x1 = obj->x1.n + CV(0.5);
   x2 = obj->x1.n - CV(0.5);
   z1 = obj->z1.n + CV(0.5);
   z2 = obj->z1.n - CV(0.5);

   // Top face:
   face->d.sprite.coords[0].x = x2;
   face->d.sprite.coords[2].x = x2;
   face->d.sprite.coords[1].x = x1 - 1;
   face->d.sprite.coords[3].x = x1 - 1;
   face->d.sprite.coords[0].z = z2;
   face->d.sprite.coords[2].z = z1 - 1;
   face->d.sprite.coords[1].z = z2;
   face->d.sprite.coords[3].z = z1 - 1;
   face->d.sprite.coords[0].y = obj->y1.n + CV(1.0);
   face->d.sprite.coords[1].y = obj->y1.n + CV(1.0);
   face->d.sprite.coords[2].y = obj->y1.n + CV(1.0);
   face->d.sprite.coords[3].y = obj->y1.n + CV(1.0);
   face->y1.n = obj->y1.n + CV(1.0);
   face->x1.n = obj->x1.n;
   face->z1.n = obj->z1.n;

   shouldDraw = !IsSpriteOutsideVisibleRange(obj);
   if (gOverheadMapState != 0) {
      shouldDraw = 1;
   }

   if (shouldDraw) {
      AddObjPrim5(gGraphicsPtr->ot, face);
      poly = &gGraphicsPtr->quads[gQuadIndex - 1];
      if (gShowBlueMovementGrid &&
          gBlueMovementGridPtr[obj->z1.s.hi][obj->x1.s.hi] != PATH_STEP_UNSET &&
          gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi] == OBJ.stack) {
         poly->r0 = 40;
         poly->g0 = 40;
         poly->b0 = gGridColorOscillation;
      } else if (gYellowTargetGridPtr[obj->z1.s.hi][obj->x1.s.hi] != PATH_STEP_UNSET) {
         poly->r0 = gGridColorOscillation;
         poly->g0 = gGridColorOscillation;
         poly->b0 = 0;
      } else if (gRedAttackGridPtr[obj->z1.s.hi][obj->x1.s.hi] != PATH_STEP_UNSET) {
         poly->r0 = gGridColorOscillation;
         poly->g0 = 40;
         poly->b0 = 40;
      }

      // West face:
      face->d.sprite.coords[0].x = x2 + 0xff;
      face->d.sprite.coords[2].x = x2 + 0xff;
      face->d.sprite.coords[1].y = obj->y1.n;
      face->d.sprite.coords[3].y = obj->y1.n;
      if (sideOrdering[dir].west != 0) {
         face->d.sprite.otOfs = sideOrdering[dir].west - 7;
         AddObjPrim5(gGraphicsPtr->ot, face);
      }

      // East face:
      face->d.sprite.coords[0].x = x2;
      face->d.sprite.coords[2].x = x2;
      face->d.sprite.coords[1].x = x2;
      face->d.sprite.coords[3].x = x2;
      if (sideOrdering[dir].east != 0) {
         face->d.sprite.otOfs = sideOrdering[dir].east - 7;
         AddObjPrim5(gGraphicsPtr->ot, face);
      }

      // North face:
      face->d.sprite.coords[1].x = x2 + 0xff;
      face->d.sprite.coords[3].x = x2 + 0xff;
      face->d.sprite.coords[2].z = z2;
      face->d.sprite.coords[3].z = z2;
      face->d.sprite.coords[0].y = obj->y1.n;
      face->d.sprite.coords[1].y = obj->y1.n;
      face->d.sprite.coords[2].y = obj->y1.n + CV(1.0);
      face->d.sprite.coords[3].y = obj->y1.n + CV(1.0);
      if (sideOrdering[dir].north != 0) {
         face->d.sprite.otOfs = sideOrdering[dir].north - 7;
         AddObjPrim5(gGraphicsPtr->ot, face);
      }

      // South face:
      face->d.sprite.coords[0].z = z2 + 0xff;
      face->d.sprite.coords[2].z = z2 + 0xff;
      face->d.sprite.coords[1].z = z2 + 0xff;
      face->d.sprite.coords[3].z = z2 + 0xff;
      if (sideOrdering[dir].south != 0) {
         face->d.sprite.otOfs = sideOrdering[dir].south - 7;
         AddObjPrim5(gGraphicsPtr->ot, face);
      }
   }

   switch (obj->state) {
   case 0:
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      UpdateCrateElevation(obj);
      obj->y1.n += gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi] * CV(1.0);
      OBJ.stack = ++gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi];
      OBJ_TERRAIN(obj).s.elevation += 2;
      OBJ.terrain = OBJ_TERRAIN(obj).s.terrain;
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_PLAINS;
      obj->state++;

   // fallthrough
   case 1:
      if (OBJ_TILE_STATE(obj).action != TA_CRATE_PUSHED ||
          OBJ_TILE_STATE(obj).cachedByte >= OBJ.stack) {
         if (OBJ_TILE_STATE(obj).action == TA_CRATE_PUSHED_PRE &&
             OBJ_TILE_STATE(obj).cachedByte == OBJ.stack) {
            if (ScanForCrateDestination(obj) == 0) {
               // Can't be pushed
               OBJ_TILE_STATE(obj).action = TA_NONE;
            } else {
               OBJ_TILE_STATE(obj).action = TA_CRATE_PUSHED;
               gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi]--;
               OBJ_TERRAIN(obj).s.elevation -= 2;
               obj->state += 2;
               obj->state2 = 0;
            }
         }
      } else {
         obj->state++;
         obj->state2 = 0;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         OBJ.elevation = obj->y1.n - CV(1.0);
         obj->y2.n = 0;
         obj->state2++;
         break;

      case 1:
         obj->y1.n -= obj->y2.n;
         obj->y2.n += CV(0.125);
         if (obj->y1.n <= OBJ.elevation) {
            obj->y1.n = OBJ.elevation;
            OBJ.stack--;
            obj->state = 1;
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(235));
            if (OBJ.stack == 1) {
               obj1 = Obj_GetUnused();
               obj1->functionIndex = OBJF_DUST_CLOUD_SPAWNER;
               obj1->x1.n = obj->x1.n;
               obj1->z1.n = obj->z1.n;
               obj1->y1.n = obj->y1.n;
            }
         }
         break;
      }

      break;

   case 3:

      switch (obj->state2) {
      case 0:
         if (OBJ.terrain != TERRAIN_PLAINS) {
            if (gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi] == 0) {
               OBJ_TERRAIN(obj).s.terrain = OBJ.terrain;
            } else {
               obj1 = gObjectArray;
               for (i = 0; i < OBJ_DATA_CT; i++) {
                  if (obj1->functionIndex == OBJF_MAP_OBJECT_CRATE && obj1 != obj) {
                     break;
                  }
                  obj1++;
               }
               obj1->d.objf046.terrain = OBJ.terrain;
            }
         }

         OBJ_TILE_STATE(obj).action = TA_NONE;
         ScanForCrateDestination(obj);

         obj->x2.n = 0;
         obj->z2.n = 0;
         if (obj->x3.s.hi > obj->x1.s.hi) {
            obj->x2.n = CV(0.25);
         }
         if (obj->x3.s.hi < obj->x1.s.hi) {
            obj->x2.n = CV(-0.25);
         }
         if (obj->z3.s.hi > obj->z1.s.hi) {
            obj->z2.n = CV(0.25);
         }
         if (obj->z3.s.hi < obj->z1.s.hi) {
            obj->z2.n = CV(-0.25);
         }
         obj->state2++;

      // fallthrough
      case 1:
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_CLOUD;
         obj1->x1.n = obj->x1.n;
         obj1->z1.n = obj->z1.n;
         obj1->y1.n = obj->y1.n;

         obj->x1.n += obj->x2.n;
         obj->z1.n += obj->z2.n;

         if (obj->x1.n == obj->x3.n && obj->z1.n == obj->z3.n) {
            if (obj->y3.n == obj->y1.n) {
               obj->state += 2;
            } else {
               obj->state++;
               obj->state2 = 0;
            }
         }
         break;
      } // switch (obj->state2) (via state:3)

      break;

   case 4:

      switch (obj->state2) {
      case 0:
         OBJ.elevation = obj->y3.n;
         obj->y2.n = CV(0.125);
         obj->state2++;

      // fallthrough
      case 1:
         obj->y1.n -= obj->y2.n;
         obj->y2.n += CV(0.125);
         if (obj->y1.n <= OBJ.elevation) {
            obj->y1.n = OBJ.elevation;
            obj->state++;
            PerformAudioCommand(AUDIO_CMD_PLAY_SFX(235));
            obj1 = Obj_GetUnused();
            obj1->functionIndex = OBJF_DUST_CLOUD_SPAWNER;
            obj1->x1.n = obj->x1.n;
            obj1->z1.n = obj->z1.n;
            obj1->y1.n = obj->y1.n;
         }
         break;
      }

      break;

   case 5:
      if (OBJ_TERRAIN(obj).s.terrain == TERRAIN_WATER ||
          OBJ_TERRAIN(obj).s.terrain == TERRAIN_LAVA) {
         PerformAudioCommand(AUDIO_CMD_PLAY_SFX(234));
         obj1 = Obj_GetUnused();
         obj1->functionIndex = OBJF_PUSHED_OBJECT_SPLASH;
         obj1->x1.n = obj->x1.n;
         obj1->z1.n = obj->z1.n;
         obj1->y1.n = obj->y1.n;
         obj1->y2.n = CV(1.0);
         obj->functionIndex = OBJF_NULL;
      } else {
         OBJ.terrain = OBJ_TERRAIN(obj).s.terrain;
         OBJ_TERRAIN(obj).s.terrain = TERRAIN_PLAINS;
         OBJ.stack = ++gCrateGrid_Ptr[obj->z1.s.hi][obj->x1.s.hi];
         OBJ_TERRAIN(obj).s.elevation += 2;
         obj->state = 1;
      }
      break;
   }
}

s32 ScanForCrateDestination(Object *obj) {
   s32 dx, dz;
   s32 unitHeight;

   switch (OBJ_TILE_STATE(obj).cachedShort >> 10) {
   case DIR_SOUTH:
      dz = 1;
      dx = 0;
      break;
   case DIR_WEST:
      dz = 0;
      dx = 1;
      break;
   case DIR_NORTH:
      dz = -1;
      dx = 0;
      break;
   case DIR_EAST:
      dz = 0;
      dx = -1;
      break;
   }

   obj->x3.n = obj->x1.n;
   obj->z3.n = obj->z1.n;
   obj->x3.s.hi = dx + obj->x3.s.hi;
   obj->z3.s.hi = dz + obj->z3.s.hi;

   if (OBJ_TARGET_MAP_UNIT(obj).raw != 0) {
      unitHeight = 0x100;
   } else {
      unitHeight = 0;
   }

   if (OBJ_TARGET_TERRAIN(obj).s.terrain == TERRAIN_NO_ENTRY) {
      obj->x3.s.hi -= dx;
      obj->z3.s.hi -= dz;
      obj->y3.n = obj->y1.n;
      return 0;
   } else {
      if (obj->y1.n > -(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                          (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8) + unitHeight) {
         obj->y3.n = -(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                     (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8);
      } else if (obj->y1.n < -(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                                 (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8) + unitHeight) {
         obj->x3.s.hi -= dx;
         obj->z3.s.hi -= dz;
         obj->y3.n = obj->y1.n;
         return 0;
      } else if (obj->y1.n <= (-(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                               (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8))) {
         obj->y3.n = -(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                     (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8);
      } else {
         obj->y3.n = -(gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].vertices[0].vy << 3) +
                     (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] << 8);
      }
      return 1;
   }
}

s32 ScanForBoulderDestination(Object *obj) {
   s32 dx, dz;
   s16 elevation;
   s32 boulderHeight;
   s32 hasRoom;

   switch (OBJ_TILE_STATE(obj).cachedShort >> 10) {
   case DIR_SOUTH:
      dz = 1;
      dx = 0;
      break;
   case DIR_WEST:
      dz = 0;
      dx = 1;
      break;
   case DIR_NORTH:
      dz = -1;
      dx = 0;
      break;
   case DIR_EAST:
      dz = 0;
      dx = -1;
      break;
   }

   obj->x3.n = obj->x1.n;
   obj->z3.n = obj->z1.n;
   elevation = obj->y1.n;
   hasRoom = 0;

   while (1) {
      obj->x3.s.hi = dx + obj->x3.s.hi;
      obj->z3.s.hi = dz + obj->z3.s.hi;

      if (OBJ_TARGET_MAP_UNIT(obj).s.team == TEAM_BOULDER) {
         boulderHeight = 0x100;
      } else {
         boulderHeight = 0;
      }

      if (elevation < ((gMapRowPointers[obj->z3.s.hi][obj->x3.s.hi].height * 0x80) +
                       (gCrateGrid_Ptr[obj->z3.s.hi][obj->x3.s.hi] * 0x100) + boulderHeight)) {
         obj->x3.s.hi -= dx;
         obj->z3.s.hi -= dz;
         return hasRoom;
      }

      if (OBJ_TARGET_MAP_UNIT(obj).s.team != TEAM_NULL && OBJ_TARGET_MAP_UNIT(obj).s.unitIdx == 0) {
         obj->x3.s.hi -= dx;
         obj->z3.s.hi -= dz;
         return hasRoom;
      }

      if (OBJ_TARGET_TERRAIN(obj).s.terrain == TERRAIN_WATER ||
          OBJ_TARGET_TERRAIN(obj).s.terrain == TERRAIN_LAVA) {
         return 1;
      }

      elevation = OBJ_TARGET_TERRAIN(obj).s.elevation * CV(0.5);
      hasRoom = 1;
   }
}

void UpdateCrateElevation(Object *obj) { obj->y1.n = (-OBJ_TILE_MODEL(obj).vertices[0].vy << 3); }

#undef OBJF
#define OBJF 020
void Objf020_PushedBoulder(Object *obj) {
   Object *unitSprite;

   unitSprite = OBJ.unitSprite;

   switch (obj->state) {
   case 0:
      gPlayerControlSuppressed = 1;
      if (gState.mapNum == 40 && gState.mapState.s.field_0x0 == 0) {
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(112));
      } else {
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(111));
      }
      gSignal5 = 0;
      OBJ.savedCamX = gCameraPos.vx;
      OBJ.savedCamZ = gCameraPos.vz;
      OBJ.savedCamY = gCameraPos.vy;
      OBJ.savedCamRotX = gCameraRotation.vx;
      OBJ.savedZoom = gCameraZoom.vz;
      OBJ.timer = 30;
      obj->state++;

   // fallthrough
   case 1:
      if (--OBJ.timer != 0) {
         gCameraPos.vx += (-(unitSprite->x1.n >> 3) - gCameraPos.vx) >> 2;
         gCameraPos.vz += (-(unitSprite->z1.n >> 3) - gCameraPos.vz) >> 2;
         gCameraPos.vy += ((unitSprite->y1.n >> 3) - gCameraPos.vy) >> 2;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraZoom.vz += (250 - gCameraZoom.vz) >> 2;
      } else {
         gSignal5 = 1;
         obj->state++;
      }
      break;

   case 2:
      if (gSignal5 == 2) {
         obj->state++;
      }
      break;

   case 3:
      if (gSignal5 != 3) {
         gCameraZoom.vz += (512 - gCameraZoom.vz) >> 3;
         OBJ.timer = 20;
      } else {
         obj->state++;
      }
      break;

   case 4:
      if (--OBJ.timer != 0) {
         gCameraRotation.vx += (OBJ.savedCamRotX - gCameraRotation.vx) >> 1;
         gCameraZoom.vz += (OBJ.savedZoom - gCameraZoom.vz) >> 1;
         gCameraPos.vx += (OBJ.savedCamX - gCameraPos.vx) >> 2;
         gCameraPos.vz += (OBJ.savedCamZ - gCameraPos.vz) >> 2;
         gCameraPos.vy += (OBJ.savedCamY - gCameraPos.vy) >> 2;
      } else {
         gCameraZoom.vz = OBJ.savedZoom;
         gCameraRotation.vx = ((gCameraRotation.vx - 64) >> 8) * 0x100 + 0x80;
         gSignal2 = 99;
         obj->functionIndex = OBJF_NULL;
      }
   }
}

#undef OBJF
#define OBJF 048
void Objf048_Push(Object *obj) {
   Object *sprite;
   Object *obj1;
   s16 dir;
   s16 fInner, fOuter, nfInner, nfOuter;
   s16 targetZ, targetX;
   s32 i;
   s32 j;
   s32 stack;
   s32 tmp2;
   // s16 crateElevation;
   // s16 srcElevation;
   s32 targetElevation;

   sprite = OBJ.unit->battler;
   sprite = sprite->d.objf014.sprite;

   switch (obj->state) {
   case 0:
      obj->x1.n = sprite->x1.n;
      obj->z1.n = sprite->z1.n;
      obj->y1.n = sprite->y1.n + CV(0.5);
      gSignal4 = sprite->d.sprite.direction;
      obj->state++;

   // fallthrough
   case 1:

      switch ((gCameraRotation.vy & 0xfff) >> 10) {
      case CAM_DIR_SOUTH:
         dir = DEG(0);
         break;
      case CAM_DIR_EAST:
         dir = DEG(270);
         break;
      case CAM_DIR_NORTH:
         dir = DEG(180);
         break;
      case CAM_DIR_WEST:
         dir = DEG(90);
         break;
      }

      if (gPadStateNewPresses & PAD_UP) {
         gSignal4 = dir + DEG(0);
      }
      if (gPadStateNewPresses & PAD_RIGHT) {
         gSignal4 = dir + DEG(90);
      }
      if (gPadStateNewPresses & PAD_DOWN) {
         gSignal4 = dir + DEG(180);
      }
      if (gPadStateNewPresses & PAD_LEFT) {
         gSignal4 = dir + DEG(270);
      }
      gSignal4 &= 0xfff;

      if (gPadStateNewPresses & PAD_X) {
         gSignal2 = 2;
         obj->functionIndex = OBJF_NULL;
         return;
      }

      OBJ.osc += DEG(45);

      fInner = (rcos(OBJ.osc & 0xfff) * 50 >> 12) + CV(0.75);
      fOuter = (rcos(OBJ.osc & 0xfff) * 130 >> 12) + CV(2.0);
      nfInner = (rcos(OBJ.osc & 0xfff) * 25 >> 12) + CV(0.75);
      nfOuter = (rcos(OBJ.osc & 0xfff) * 50 >> 12) + CV(1.5);
      obj1 = Obj_GetUnused();

      RenderDirectionArrow(obj, obj1, DEG(0), gSignal4 == DEG(0), fInner, fOuter, nfInner, nfOuter);
      RenderDirectionArrow(obj, obj1, DEG(90), gSignal4 == DEG(90), fInner, fOuter, nfInner,
                           nfOuter);
      RenderDirectionArrow(obj, obj1, DEG(180), gSignal4 == DEG(180), fInner, fOuter, nfInner,
                           nfOuter);
      RenderDirectionArrow(obj, obj1, DEG(270), gSignal4 == DEG(270), fInner, fOuter, nfInner,
                           nfOuter);

      targetX = sprite->x1.s.hi;
      targetZ = sprite->z1.s.hi;

      switch (gSignal4 >> 10) {
      case DIR_SOUTH:
         targetZ++;
         break;
      case DIR_WEST:
         targetX++;
         break;
      case DIR_NORTH:
         targetZ--;
         break;
      case DIR_EAST:
         targetX--;
         break;
      }

      OBJ.direction = gSignal4;
      OBJ.targetX = targetX;
      OBJ.targetZ = targetZ;

      if ((gPadStateNewPresses & PAD_CIRCLE) &&
          gMapUnitsPtr[targetZ][targetX].s.team == TEAM_BOULDER) {

         i = OBJ_TERRAIN(sprite).s.elevation - gTerrainPtr[targetZ][targetX].s.elevation;
         if ((i + 1 >= 0) && (i + 1 < 3)) {
            gTileStateGridPtr[targetZ][targetX].cachedShort = gSignal4;

            obj1 = gObjectArray;
            for (i = 0; i < OBJ_DATA_CT; i++) {
               if (obj1->functionIndex == OBJF_MAP_OBJECT_BOULDER && obj1->x1.s.hi == targetX &&
                   obj1->z1.s.hi == targetZ) {
                  if (ScanForBoulderDestination(obj1)) {
                     gSignal2 = 1;
                     obj->state = 5;
                     return;
                  }
                  break;
               }
               obj1++;
            }
         }
      } // CIRCLE (Boulders)

      if ((gPadStateNewPresses & PAD_CIRCLE) && gCrateGrid_Ptr[targetZ][targetX] != 0 &&
          gCrateGrid_Ptr[targetZ][targetX] != 0x7f) {
         // TODO: Clean-up.
         //@2a68
         tmp2 = OBJ_TERRAIN(sprite).s.elevation + 2;
         targetElevation = gTerrainPtr[targetZ][targetX].s.elevation;

         if (OBJ_TERRAIN(sprite).s.elevation >=
             targetElevation - gCrateGrid_Ptr[targetZ][targetX] * 2 - 1) {

            if (tmp2 - 1 <= targetElevation) {
               j = targetElevation - gCrateGrid_Ptr[targetZ][targetX] * 2;
               tmp2 = tmp2 - 1;
               stack = 0;

               do {
                  j += 2; //@2ae4
                  stack++;
               } while (j < tmp2);

               //@2af4
               OBJ.stack = stack;
               gTileStateGridPtr[targetZ][targetX].cachedByte = stack;
               gTileStateGridPtr[targetZ][targetX].cachedShort = gSignal4;

               obj1 = gObjectArray;
               for (i = 0; i < OBJ_DATA_CT; i++) {
                  if (obj1->functionIndex == OBJF_MAP_OBJECT_CRATE &&
                      stack == obj1->d.objf046.stack &&
                      (obj1->x1.s.hi == targetX && obj1->z1.s.hi == targetZ)) {

                     if (!ScanForCrateDestination(obj1)) {
                        return;
                     }

                     break;
                  }
                  obj1++;
               }

               gSignal2 = 1;
               obj->state++;
            }
         }
      } // CIRCLE (Crates)
      break;

   case 2:
      OBJ_TILE_STATE(sprite).action = TA_PUSHING_CRATE;
      gSignal3 = 0;
      obj->state++;

   // fallthrough
   case 3:
      if (gSignal3 != 0) {
         gTileStateGridPtr[OBJ.targetZ][OBJ.targetX].action = TA_CRATE_PUSHED_PRE;
         gTileStateGridPtr[OBJ.targetZ][OBJ.targetX].cachedByte = OBJ.stack;
         gTileStateGridPtr[OBJ.targetZ][OBJ.targetX].cachedShort = OBJ.direction;
         OBJ.timer = 20;
         obj->state++;
      }
      break;

   case 4:
      if (--OBJ.timer == 0) {
         gSignal2 = 99;
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 5:
      OBJ_TILE_STATE(sprite).action = TA_PUSHING_BOULDER;
      gSignal3 = 0;
      obj->state++;

   // fallthrough
   case 6:
      if (gSignal3 != 0) {
         gTileStateGridPtr[OBJ.targetZ][OBJ.targetX].action = TA_BOULDER_PUSHED;
         gTileStateGridPtr[OBJ.targetZ][OBJ.targetX].cachedShort = OBJ.direction;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void TiltChestLid(Object *chest, s16 angle, u8 facing) {
   if (facing == 0) {
      chest->d.objf040.lidFrontZ =
          chest->d.objf040.lidBackZ + (CV(0.875) * rcos(angle & 0xfff) >> 12);
      chest->d.objf040.lidFrontY =
          chest->d.objf040.lidBackY + (CV(0.875) * rcos((angle + DEG(90)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease1Z =
          chest->d.objf040.lidBackZ + (CV(0.375) * rcos((angle - DEG(56.25)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease1Y =
          chest->d.objf040.lidBackY + (CV(0.375) * rcos((angle + DEG(33.75)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease2Z =
          chest->d.objf040.lidBackZ + (CV(0.75) * rcos((angle - DEG(22.5)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease2Y =
          chest->d.objf040.lidBackY + (CV(0.75) * rcos((angle + DEG(67.5)) & 0xfff) >> 12);
   } else {
      chest->d.objf040.lidBackZ =
          chest->d.objf040.lidFrontZ - (CV(0.875) * rcos(angle & 0xfff) >> 12);
      chest->d.objf040.lidBackY =
          chest->d.objf040.lidFrontY + (CV(0.875) * rcos((angle + DEG(90)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease2Z =
          chest->d.objf040.lidFrontZ - (CV(0.375) * rcos((angle - DEG(56.25)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease2Y =
          chest->d.objf040.lidFrontY + (CV(0.375) * rcos((angle + DEG(33.75)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease1Z =
          chest->d.objf040.lidFrontZ - (CV(0.75) * rcos((angle - DEG(22.5)) & 0xfff) >> 12);
      chest->d.objf040.lidCrease1Y =
          chest->d.objf040.lidFrontY + (CV(0.75) * rcos((angle + DEG(67.5)) & 0xfff) >> 12);
   }
}

#undef OBJF
#define OBJF 040
void Objf040_MapObject_Chest(Object *obj) {
   static s16 chestGfx[10] = {GFX_CHEST_4, GFX_CHEST_4, GFX_CHEST_2, GFX_CHEST_2, GFX_CHEST_3,
                              GFX_CHEST_3, GFX_CHEST_2, GFX_CHEST_2, GFX_CHEST_1, GFX_CHEST_1};
   // static u8 mimicLevels[40];
   // static u8 mimicExpMulti[40];
   static u8 mimicData[80] = {
       10, 10, 10, 10, 10, 10, 10, 10, 10, 10, 1, 1, 1, 1, 1,  1, 1,  1, 1, 1,
       1,  1,  1,  1,  1,  1,  1,  18, 1,  1,  1, 1, 1, 1, 21, 1, 23, 1, 1, 1,

       0,  0,  0,  0,  0,  0,  0,  0,  0,  0,  1, 1, 1, 1, 1,  1, 1,  1, 1, 1,
       1,  1,  1,  1,  1,  1,  1,  8,  1,  1,  1, 1, 1, 1, 12, 1, 15, 1, 1, 0,
   };

   Object *face;
   Object *newObj;
   s32 i;
   s16 idx;
   u8 *pMimicExpMulti; // Starts halfway into mimicData

   face = gTempGfxObj;
   //?: Maybe a remnant of mapObj.param = 0?
   *(s16 *)(&face->d.sprite.hidden) = 0;
   idx = 0;
   if (gRedAttackGridPtr[obj->z1.s.hi][obj->x1.s.hi] == PATH_STEP_UNSET) {
      face->d.sprite.clut = CLUT_NULL;
   } else {
      face->d.sprite.clut = CLUT_REDS;
   }
   face->d.sprite.otOfs = 2;

   switch (obj->state) {
   case 0:
      OBJ.item = OBJ.item;
      OBJ_MAP_UNIT(obj).s.team = TEAM_CHEST;
      OBJ.terrain = OBJ_TERRAIN(obj).s.terrain;
      OBJ_TERRAIN(obj).s.terrain = TERRAIN_OBSTACLE;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.lo = CV(0.5);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      OBJ.lidCrease1Z = obj->z1.n - CV(0.25);
      OBJ.lidCrease2Z = obj->z1.n + CV(0.25);
      OBJ.lidBackZ = obj->z1.n - CV(0.4375);
      OBJ.lidFrontZ = obj->z1.n + CV(0.4375);
      OBJ.lidCrease1Y = obj->y1.n + CV(0.8125);
      OBJ.lidCrease2Y = obj->y1.n + CV(0.8125);
      OBJ.lidBackY = obj->y1.n + CV(0.5);
      OBJ.lidFrontY = obj->y1.n + CV(0.5);
      obj->state++;
      break;

   case 1:
      if (OBJ_TILE_STATE(obj).action == TA_CHEST_1) {
         OBJ_TERRAIN(obj).s.terrain = OBJ.terrain;
         OBJ_MAP_UNIT(obj).s.team = TEAM_NULL;
         OBJ.lidAngleVel = DEG(-16.875);
         obj->mem = 3;
         gSignal3 = 0;
         newObj = Obj_GetUnused();
         newObj->x1.n = obj->x1.n;
         newObj->z1.n = obj->z1.n;
         newObj->y1.n = obj->y1.n;
         if (OBJ.item != 0) {
            newObj->functionIndex = OBJF_REVEAL_CHEST_ITEM;
            newObj->d.objf290.gfxIdx = GFX_ITEM_ICONS_OFS + OBJ.item;
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(36));
         } else {
            newObj->functionIndex = OBJF_REVEAL_MIMIC;
            gState.subObjDone = 0;
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(132));
         }
         obj->state++;
      }
      if (OBJ_TILE_STATE(obj).action == TA_CHEST_2) {
         OBJ_TILE_STATE(obj).action = TA_NONE;
         OBJ_TILE_STATE(obj).cachedByte = OBJ.item;
      }
      if (OBJ_TILE_STATE(obj).action == TA_CHEST_3) {
         OBJ_TILE_STATE(obj).action = TA_NONE;
         OBJ.facing = OBJ_TILE_STATE(obj).cachedByte;
         if (OBJ.facing == 2 && gMapUnitsPtr[gTargetZ - 1][gTargetX].s.team == TEAM_NULL) {
            OBJ.facing = 0;
         }
      }
      break;

   case 2:
      if (--obj->mem == 0) {
         obj->mem = 75;
         obj->state++;
      }
      break;

   case 3:
      if (--obj->mem != 0) {
         OBJ.lidAngleVel -= DEG(1.40625);
         OBJ.lidAngle += OBJ.lidAngleVel;
         if (OBJ.lidAngle < DEG(-135)) {
            OBJ.lidAngle = DEG(-135);
            OBJ.lidAngleVel = (-OBJ.lidAngleVel >> 1);
         }
         TiltChestLid(obj, OBJ.lidAngle, OBJ.facing);
         break;
      }
      if (OBJ.item == 0) {
         // Spawn a mimic
         obj->state = 6;
         break;
      }
      if (gTileStateGridPtr[gTargetZ][gTargetX].cachedByte == 0) {
         ShowReceivedItemDialog(OBJ.item, 0x3c, 0);
      } else {
         ShowDepotReceivedItemDialog(OBJ.item, 0x3c, 0);
      }
      obj->mem = 50;
      obj->state++;
      break;

   case 4:
      if ((--obj->mem == 0) || (gPadStateNewPresses & PAD_CIRCLE)) {
         CloseWindow(0x3c);
         obj->state = 5;
      }
      break;

   case 5:
      gSignal3 = 1;
      OBJ_TILE_STATE(obj).action = TA_NONE;
      obj->functionIndex = OBJF_NULL;
      break;

   case 6:
      obj->mem = 30;
      i = 0;
      while (gSpriteStripUnitIds[i] != UNIT_ID_MIMIC) {
         i++;
      }
      pMimicExpMulti = &mimicData[40 + gState.mapNum];
      SetupBattleUnit(i, obj->z1.s.hi, obj->x1.s.hi, mimicData[gState.mapNum], TEAM_ENEMY,
                      OBJ_TILE_STATE(obj).cachedShort, 99, *pMimicExpMulti, 99);
      obj->state++;
      break;

   case 7:
      if (--obj->mem == 0) {
         OBJ_TILE_STATE(obj).action = TA_NONE;
         gState.subObjDone = 1;
         gSignal3 = 1;
         obj->functionIndex = OBJF_NULL;
      }
      return;
   }

   if (!IsSpriteOutsideVisibleRange(obj)) {
      face->d.sprite.coords[0].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[3].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[0].z = obj->z1.n - CV(0.4375);
      face->d.sprite.coords[1].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[2].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[3].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[0].y = obj->y1.n + CV(0.5);
      face->d.sprite.coords[1].y = face->d.sprite.coords[0].y;
      face->d.sprite.coords[2].y = obj->y1.n;
      face->d.sprite.coords[3].y = face->d.sprite.coords[2].y;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[0]

      face->d.sprite.coords[0].z = obj->z1.n + CV(0.4375);
      face->d.sprite.coords[1].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[2].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[3].z = face->d.sprite.coords[0].z;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[1]

      face->d.sprite.coords[0].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[1].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[3].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[0].z = obj->z1.n - CV(0.4375);
      face->d.sprite.coords[2].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[1].z = obj->z1.n + CV(0.4375);
      face->d.sprite.coords[3].z = face->d.sprite.coords[1].z;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[2]

      face->d.sprite.coords[0].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[1].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[3].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[0].z = obj->z1.n - CV(0.4375);
      face->d.sprite.coords[2].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[1].z = obj->z1.n + CV(0.4375);
      face->d.sprite.coords[3].z = face->d.sprite.coords[1].z;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[3]

      face->d.sprite.coords[0].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[3].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[0].z = OBJ.lidCrease1Z;
      face->d.sprite.coords[1].z = OBJ.lidCrease1Z;
      face->d.sprite.coords[2].z = OBJ.lidBackZ;
      face->d.sprite.coords[3].z = OBJ.lidBackZ;
      face->d.sprite.coords[0].y = OBJ.lidCrease1Y;
      face->d.sprite.coords[1].y = OBJ.lidCrease1Y;
      face->d.sprite.coords[2].y = OBJ.lidBackY;
      face->d.sprite.coords[3].y = OBJ.lidBackY;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[4]

      face->d.sprite.coords[0].z = OBJ.lidCrease2Z;
      face->d.sprite.coords[1].z = OBJ.lidCrease2Z;
      face->d.sprite.coords[2].z = OBJ.lidFrontZ;
      face->d.sprite.coords[3].z = OBJ.lidFrontZ;
      face->d.sprite.coords[0].y = OBJ.lidCrease2Y;
      face->d.sprite.coords[1].y = OBJ.lidCrease2Y;
      face->d.sprite.coords[2].y = OBJ.lidFrontY;
      face->d.sprite.coords[3].y = OBJ.lidFrontY;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[5]

      face->d.sprite.coords[0].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[3].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[0].z = OBJ.lidCrease1Z;
      face->d.sprite.coords[1].z = OBJ.lidCrease2Z;
      face->d.sprite.coords[2].z = OBJ.lidBackZ;
      face->d.sprite.coords[3].z = OBJ.lidFrontZ;
      face->d.sprite.coords[0].y = OBJ.lidCrease1Y;
      face->d.sprite.coords[1].y = OBJ.lidCrease2Y;
      face->d.sprite.coords[2].y = OBJ.lidBackY;
      face->d.sprite.coords[3].y = OBJ.lidFrontY;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[6]

      face->d.sprite.coords[0].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[3].x = face->d.sprite.coords[0].x;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[7]

      face->d.sprite.coords[0].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[3].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[0].z = OBJ.lidCrease2Z;
      face->d.sprite.coords[1].z = OBJ.lidCrease2Z;
      face->d.sprite.coords[2].z = OBJ.lidCrease1Z;
      face->d.sprite.coords[3].z = OBJ.lidCrease1Z;
      face->d.sprite.coords[0].y = OBJ.lidCrease2Y;
      face->d.sprite.coords[1].y = OBJ.lidCrease2Y;
      face->d.sprite.coords[2].y = OBJ.lidCrease1Y;
      face->d.sprite.coords[3].y = OBJ.lidCrease1Y;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[8]

      face->d.sprite.coords[0].x = obj->x1.n - CV(0.4375);
      face->d.sprite.coords[2].x = face->d.sprite.coords[0].x;
      face->d.sprite.coords[1].x = obj->x1.n + CV(0.4375);
      face->d.sprite.coords[3].x = face->d.sprite.coords[1].x;
      face->d.sprite.coords[0].z = obj->z1.n - CV(0.4375);
      face->d.sprite.coords[1].z = face->d.sprite.coords[0].z;
      face->d.sprite.coords[2].z = obj->z1.n + CV(0.4375);
      face->d.sprite.coords[3].z = face->d.sprite.coords[2].z;
      face->d.sprite.coords[0].y = obj->y1.n;
      face->d.sprite.coords[1].y = face->d.sprite.coords[0].y;
      face->d.sprite.coords[2].y = face->d.sprite.coords[0].y;
      face->d.sprite.coords[3].y = face->d.sprite.coords[0].y;
      face->d.sprite.otOfs = 2;
      face->d.sprite.gfxIdx = chestGfx[idx++];
      AddObjPrim4(gGraphicsPtr->ot, face); //[9]
   }
}
