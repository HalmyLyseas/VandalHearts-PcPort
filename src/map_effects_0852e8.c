/* Entry point and shared geometry toolkit for the map_effects_*.c corpus.
 *
 * SetupMapExtras() is the hub: called once per field/battle init from split_0496f8.c, right
 * after the OBJF_SETUP_MAP_OBJECTS pass over setup_map_objects.c's per-map chest/scenery
 * tables. It switches on gState.mapNum and spawns that map's scripted-scenery driver
 * (drawbridges, buttons, floodgates, the sand pit, the lava-pit platform, the car release,
 * the barricade...) plus ambient objects -- rain, chimney smoke, Map 38's seven torch
 * flames -- and the camera-zoom service object kept in gCameraZoomObj. InEventScene() picks the
 * cutscene vs. battle variant and gates the battle-only drivers.
 *
 * The rest is the toolkit every other map_effects file builds on: tile-model elevation
 * edits (AdjustTileElevation, AdjustFaceElevation[2], SetFaceElevation), the matrix
 * versions (RotateMapTile, RotTransMapTile), and the direct camera pokes (PositionCamera,
 * PanCamera).
 *
 * Local handlers: Objf399_Map11, the Map 11 drawbridge-lever cutaway (pans to the lever,
 * sinks it, and hands mapState 1->2 to Objf362_DrawbridgeButton), and
 * Objf753_IncrementMapState0, a one-shot event-script helper. Unreferenced leftovers kept
 * byte-exact: func_80095128, RotateMapTile, AdjustAreaElevation_FromOrigin. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "audio.h"

// TODO: Rename elevation stuff to disambiguate kind

s32 InEventScene(void) {
   if (gState.primary == STATE_4 || gState.primary == STATE_25 || gState.primary == STATE_16 ||
       gState.primary == STATE_17) {
      return 1;
   } else {
      return 0;
   }
}

void SetupMapExtras(void) {
   extern Object *gCameraZoomObj;
   Object *obj;
   s32 i;

   obj = Obj_GetUnused();
   obj->functionIndex = OBJF_CAMERA_TBD_277;
   gCameraZoomObj = obj;

   if (gState.mapNum == 11) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP11;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_HEAVY_RAINFALL;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_DRAWBRIDGE_BUTTON;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_CHIMNEY_SMOKE;
      obj->x1.n = CV(18.375);
      obj->z1.n = CV(2.375);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_CHIMNEY_SMOKE;
      obj->x1.n = CV(13.375);
      obj->z1.n = CV(10.625);
   }

   if (gState.mapNum == 13) {
      if (InEventScene() == 1) {
         obj = Obj_GetUnused();
         obj->functionIndex = OBJF_MAP13_BRIDGE_EXPLOSION_IN_SCENE;
      } else {
         obj = Obj_GetUnused();
         obj->functionIndex = OBJF_MAP13_BRIDGE_EXPLOSION_IN_BATTLE;
      }
   }

   if (!InEventScene() && gState.mapNum == 14) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP14_SAND;
   }

   if (gState.mapNum == 15 && !InEventScene()) {
      Map15_RaisePlank(7, 11);
      Map15_RaisePlank(14, 11);
   }

   if (gState.mapNum == 17) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP17_BUTTON;
   }

   if (gState.mapNum == 17) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP17_FLOODGATE;
   }

   if (gState.mapNum == 19) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP19;
   }

   if (gState.mapNum == 26) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP26;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_DRAWBRIDGE_BUTTON;
   }

   if (gState.mapNum == 27) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP27_BUTTONS;
   }

   if (gState.mapNum == 28) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP28_BUTTON;

      for (i = 194; i < 197; i++) {
         gGfxSubTextures[GFX_MAP_TEXTURES_OFS + i][3] = 24;
      }

      if (gState.primary == STATE_4 && gState.scene == 50) {
         gState.mapState.s.field_0x0 = 1;
      }
   }

   if (gState.mapNum == 29) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP29;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_DRAWBRIDGE_BUTTON;
   }

   if (gState.mapNum == 32) {
      obj = Obj_GetUnused();
      obj->x1.n = CV(58.0);
      obj->z1.n = CV(4.0);
      obj->y1.n = CV(6.75);
      obj->functionIndex = OBJF_MAP32_SMOKESTACK;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP32_CAR_RELEASE;
   }

   if (gState.mapNum == 33) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP33_LAVA_PIT_PLATFORM;
   }

   if (gState.mapNum == 35) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP35_BUTTON;
   }

   if (gState.mapNum == 36 && !InEventScene()) {
      // Leena shielding villager
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP36_LEENA;
      obj->x1.n = CV(7.5);
      obj->z1.n = CV(6.5);
      gMapUnitsPtr[6][7].s.team = TEAM_NEUTRAL;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP36_VILLAGER;
      obj->x1.n = CV(7.5);
      obj->z1.n = CV(7.5);
      gMapUnitsPtr[7][7].s.team = TEAM_NEUTRAL;
   }

   if (gState.mapNum == 39 && !InEventScene()) {
      // Leena raising submerged platforms
      Map39_Setup();
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP39;
   }

   if (gState.mapNum == 40) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP40_BARRICADE;
      gGfxSubTextures[GFX_MAP_TEXTURE_232][3] = 40;
      gGfxSubTextures[GFX_MAP_TEXTURE_233][3] = 40;
   }

   if (gState.mapNum == 38) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP38_FLOODGATE;

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(20.5);
      obj->z1.n = CV(11.5);
      obj->y1.n = CV(1.5);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(18.5);
      obj->z1.n = CV(12.5);
      obj->y1.n = CV(1.5);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(17.5);
      obj->z1.n = CV(12.5);
      obj->y1.n = CV(1.5);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(13.0);
      obj->z1.n = CV(2.0);
      obj->y1.n = CV(3.0);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(11.0);
      obj->z1.n = CV(3.0);
      obj->y1.n = CV(3.0);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(3.0);
      obj->z1.n = CV(13.0);
      obj->y1.n = CV(3.0);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_FLAME;
      obj->x1.n = CV(1.0);
      obj->z1.n = CV(14.0);
      obj->y1.n = CV(3.0);
   }

   if (gState.mapNum == 42) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_CHIMNEY_SMOKE;
      obj->x1.n = CV(2.375);
      obj->z1.n = CV(9.625);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_CHIMNEY_SMOKE;
      obj->x1.n = CV(19.375);
      obj->z1.n = CV(3.375);

      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_CHIMNEY_SMOKE;
      obj->x1.n = CV(17.625);
      obj->z1.n = CV(14.625);
   }
}

void PositionCamera(s16 x, s16 y, s16 z) {
   gCameraPos.vx = -(x >> 3);
   gCameraPos.vy = y >> 3;
   gCameraPos.vz = -(z >> 3);
}

s32 func_80095128(s32 param_1, s16 x, s16 y, s16 z) {
   /* UNREFERENCED, and the computed vector is discarded -- an emptied helper that now
    * just returns its first argument. Keeps its address name. */
   SVECTOR vec;

   vec.vx = -(x >> 3) + 448;
   vec.vy = y >> 3;
   vec.vz = -(z >> 3) + 448;

   return param_1;
}

void PanCamera(s16 x, s16 y, s16 z, s32 smoothness) {
   gCameraPos.vx += (-(x >> 3) - gCameraPos.vx) >> smoothness;
   gCameraPos.vz += (-(z >> 3) - gCameraPos.vz) >> smoothness;
   gCameraPos.vy += ((y >> 3) - gCameraPos.vy) >> smoothness;
}

void AdjustTileElevation(MapTileModel *tileModel, s16 amount) {
   s32 i;

   for (i = 0; i < ARRAY_COUNT(tileModel->vertices); i++) {
      tileModel->vertices[i].vy += amount;
   }
}

void AdjustFaceElevation2(MapTileModel *tileModel, s16 faceIdx, s16 amount) {
   s32 i, j;
   s32 isDupeVtx;
   u8 *pFaceVerts;

   pFaceVerts = &tileModel->faces[faceIdx][0];

   for (i = 0; i < 4; i++) {
      // Prevent moving the same vertex multiple times, presumably for triangle or edge type faces.
      isDupeVtx = 0;

      for (j = 0; j < i; j++) {
         if (pFaceVerts[i] == pFaceVerts[j]) {
            isDupeVtx = 1;
         }
      }

      if (!isDupeVtx) {
         tileModel->vertices[pFaceVerts[i]].vy += amount;
      }
   }
}

void AdjustFaceElevation(MapTileModel *tileModel, s16 faceIdx, s16 amount) {
   s32 i;
   u8 *pFaceVerts;

   pFaceVerts = &tileModel->faces[faceIdx][0];

   for (i = 0; i < 4; i++) {
      tileModel->vertices[pFaceVerts[i]].vy += amount;
   }
}

void SetFaceElevation(MapTileModel *tileModel, s16 faceIdx, s16 elevation) {
   s32 i;
   u8 *pFaceVerts;

   pFaceVerts = &tileModel->faces[faceIdx][0];

   for (i = 0; i < 4; i++) {
      tileModel->vertices[pFaceVerts[i]].vy = elevation;
   }
}

void RotateMapTile(MapTileModel *src, SVECTOR *rot, MapTileModel *dst) {
   // Unused?
   s32 i;
   MATRIX mtx;
   VECTOR vec;
   s32 flag;

   PushMatrix();
   RotMatrix(rot, &mtx);
   vec.vx = 0;
   vec.vz = 0;
   vec.vy = 0;
   TransMatrix(&mtx, &vec);
   SetTransMatrix(&mtx);
   SetRotMatrix(&mtx);

   for (i = 0; i < ARRAY_COUNT(src->vertices); i++) {
      RotTrans(&src->vertices[i], &vec, &flag);
      dst->vertices[i].vx = vec.vx;
      dst->vertices[i].vz = vec.vz;
      dst->vertices[i].vy = vec.vy;
   }

   PopMatrix();
}

void RotTransMapTile(MapTileModel *src, SVECTOR *rot, SVECTOR *tr, MapTileModel *dst) {
   s32 i;
   MATRIX mtx;
   VECTOR vec;
   s32 flag;

   PushMatrix();
   RotMatrix(rot, &mtx);
   vec.vx = tr->vx >> 3;
   vec.vz = tr->vz >> 3;
   vec.vy = -(tr->vy >> 3);
   TransMatrix(&mtx, &vec);
   SetTransMatrix(&mtx);
   SetRotMatrix(&mtx);

   for (i = 0; i < ARRAY_COUNT(src->vertices); i++) {
      RotTrans(&src->vertices[i], &vec, &flag);
      dst->vertices[i].vx = vec.vx;
      dst->vertices[i].vz = vec.vz;
      dst->vertices[i].vy = vec.vy;
   }

   PopMatrix();
}

void AdjustAreaElevation_FromOrigin(s32 x, s32 z, s16 amount) {
   // Unused?
   s32 iz, ix;

   for (ix = 0; ix < x; ix++) {
      for (iz = 0; iz < z; iz++) {
         AdjustTileElevation(&gMapRowPointers[iz][ix], amount);
      }
   }
}

#undef OBJF
#define OBJF 399
void Objf399_Map11(Object *obj) {
   switch (obj->state) {
   case 0:
      gTerrainPtr[3][8].s.terrain = TERRAIN_NO_ENTRY;
      gTerrainPtr[4][8].s.terrain = TERRAIN_NO_ENTRY;
      gMapRowPointers[3][8].height = 9;
      gMapRowPointers[4][8].height = 9;
      obj->state++;

      if (gState.mapState.s.field_0x0 != 0) {
         obj->functionIndex = OBJF_NULL;
         return;
      }

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 1) {
         obj->state2 = 48;
         gCameraRotation.vy &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 1;
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(33));
         obj->state++;
      }
      break;

   case 2:
      OBJ.dstRotY = GetBestViewOfTarget(11, 1, 1);
      gCameraRotation.vy &= 0xfff;
      OBJ.dstRotY = ShortestTurnAngle(gCameraRotation.vy, DEG(135));
      obj->state++;

   // fallthrough
   case 3:
      if (--obj->state2 <= 0) {
         gCameraRotation.vy = OBJ.dstRotY;
         gCameraRotation.vx = DEG(33.75);
         obj->state3 = 2;
         obj->state++;
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(33));
      }
      PanCamera(CV(11.5), CV(2.0), CV(1.0), 2);
      gCameraRotation.vy += (OBJ.dstRotY - gCameraRotation.vy) >> 2;
      gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
      gCameraZoom.vz += (DEG(33.75) - gCameraZoom.vz) >> 2;
      break;

   case 4:
      AdjustFaceElevation(&gMapRowPointers[1][11], 0, 8);
      gTerrainPtr[1][11].s.elevation = -gMapRowPointers[1][11].vertices[0].vy >> 4;
      gCameraPos.vy += 8;

      if (--obj->state3 <= 0) {
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 5:
      if (--obj->state3 <= 0) {
         gState.mapState.s.field_0x0 = 2;
         obj->state++;
      }
      break;

   case 6:
      if (gState.mapState.s.field_0x0 == 3) {
         gCameraRotation.vy &= 0xfff;
         OBJ.camera.rotY = ShortestTurnAngle(gCameraRotation.vy, OBJ.camera.rotY);
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 7:
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      EaseOutCamera(&OBJ.camera, 2);
      break;

   case 8:
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
         AssignToMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 0;
         gState.signal = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 753
void Objf753_IncrementMapState0(Object *obj) {
   gState.mapState.s.field_0x0++;
   obj->functionIndex = OBJF_NULL;
}
