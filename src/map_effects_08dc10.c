/* Map 27's prison cell doors -- the whole file is one map's puzzle.
 *
 * Objf665_Map27_Buttons (spawned by SetupMapExtras()) owns four lever tiles and five cell
 * doors, paired through buttonDoorIdx; doors 2 and 3 open together. On reload it applies
 * the already-pressed levers instantly (Map27_DepressButton + Map27_RemoveCellDoor). In
 * play it scans gState.mapState.bytes[i] -- set by Objf443_EvaluateBattle27 -- then takes
 * the camera, runs the shared OBJF_BUTTON_DEPRESS cutaway on that lever, pans to the door
 * and spawns Objf664_Map27_OpenCellDoor, which raises the door's two tiles out of the floor
 * and retextures them walkable. It then returns to the scan state, so several levers can be
 * played out one after another over the course of the battle. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

void Map27_DepressButton(s32 x, s32 z) {
   AdjustFaceElevation(&gMapRowPointers[z][x], 0, 16);
   gTerrainPtr[z][x].s.elevation = -gMapRowPointers[z][x].vertices[0].vy >> 4;
}

void Map27_RemoveCellDoor(s32 x, s32 z) {
   MapTileModel *tileModel;

   tileModel = &gMapRowPointers[z][x];
   AdjustFaceElevation(tileModel, 0, 64);
   tileModel->vertices[tileModel->faces[7][0]].vy += 64;
   tileModel->vertices[tileModel->faces[7][1]].vy += 64;
   gTerrainPtr[z][x].s.elevation = -gMapRowPointers[z][x].vertices[0].vy >> 4;
   gTerrainPtr[z][x].s.terrain = TERRAIN_PLAINS;
   tileModel->gfx[0] = GFX_MAP_TEXTURE_25;

   tileModel = &gMapRowPointers[z + 1][x];
   AdjustFaceElevation(tileModel, 0, 64);
   tileModel->vertices[tileModel->faces[7][0]].vy += 64;
   tileModel->vertices[tileModel->faces[7][1]].vy += 64;
   tileModel->gfx[0] = GFX_MAP_TEXTURE_25;
   gTerrainPtr[z + 1][x].s.elevation = -gMapRowPointers[z][x].vertices[0].vy >> 4;
   gTerrainPtr[z + 1][x].s.terrain = TERRAIN_PLAINS;
}

#undef OBJF
#define OBJF 664
void Objf664_Map27_OpenCellDoor(Object *obj) {
   s32 tileZ, tileX;
   MapTileModel *tileModel;

   tileModel = &OBJ_TILE_MODEL(obj);
   tileX = obj->x1.s.hi;
   tileZ = obj->z1.s.hi;

   switch (obj->state) {
   case 0:
      obj->mem = 64;
      obj->state2 = 2;
      obj->state3 = 1;
      obj->state++;

   // fallthrough
   case 1:
      if (obj->mem > obj->state2) {
         AdjustFaceElevation(&gMapRowPointers[tileZ][tileX], 0, obj->state2);
         AdjustFaceElevation(&gMapRowPointers[tileZ + 1][tileX], 0, obj->state2);

         tileModel = &gMapRowPointers[tileZ][tileX];
         tileModel->vertices[tileModel->faces[7][0]].vy += obj->state2;
         tileModel->vertices[tileModel->faces[7][1]].vy += obj->state2;

         tileModel = &gMapRowPointers[tileZ + 1][tileX];
         tileModel->vertices[tileModel->faces[7][0]].vy += obj->state2;
         tileModel->vertices[tileModel->faces[7][1]].vy += obj->state2;

         obj->mem -= obj->state2;
      } else {
         SetFaceElevation(&gMapRowPointers[tileZ][tileX], 0, -16);
         SetFaceElevation(&gMapRowPointers[tileZ][tileX], 1, -16);
         SetFaceElevation(&gMapRowPointers[tileZ + 1][tileX], 0, -16);
         SetFaceElevation(&gMapRowPointers[tileZ + 1][tileX], 1, -16);
         obj->mem = 0;
         obj->state++;
      }
      UpdateMapTileLighting(tileModel);
      gTerrainPtr[tileZ][tileX].s.elevation = -gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4;
      gTerrainPtr[tileZ][tileX].s.terrain = TERRAIN_PLAINS;
      gTerrainPtr[tileZ + 1][tileX].s.elevation =
          -gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4;
      gTerrainPtr[tileZ + 1][tileX].s.terrain = TERRAIN_PLAINS;
      break;

   case 2:
      if (++obj->mem >= 32) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 665
void Objf665_Map27_Buttons(Object *obj) {
   static BVectorXZ buttonLocations[4] = {{22, 14}, {9, 9}, {15, 12}, {1, 1}};
   static u8 buttonDoorIdx[4] = {0, 1, 2, 4};
   static BVectorXZ doorLocations[5] = {{11, 1}, {19, 13}, {3, 1}, {11, 13}, {7, 9}};

   s32 i;
   s32 doorIdx;
   Object *child;
   BVectorXZ *p;

   switch (obj->state) {
   case 0:
      for (i = 0; i < ARRAY_COUNT(buttonLocations); i++) {
         if (gState.mapState.bytes[i] != 0) {
            Map27_DepressButton(buttonLocations[i].x, buttonLocations[i].z);
            doorIdx = buttonDoorIdx[i];
            Map27_RemoveCellDoor(doorLocations[doorIdx].x, doorLocations[doorIdx].z);
            if (doorIdx == 2) {
               // Doors 2 and 3 open together
               p = &doorLocations[3];
               Map27_RemoveCellDoor(p->x, p->z);
            }
            OBJ.buttonPressed[i] = 1;
         }
      }
      obj->state++;

   // fallthrough
   case 1:
      for (i = 0; i < ARRAY_COUNT(buttonLocations); i++) {
         if (!OBJ.buttonPressed[i] && gState.mapState.bytes[i] != 0) {
            // Button i currently being pressed. (mapState set by Objf443_EvaluateBattle27)
            obj->mem = i;
            obj->state++;
            obj->state2 = 0;
            break;
         }
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         AssignFromMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 1;
         obj->z3.n = buttonDoorIdx[obj->mem];
         obj->state2++;
         break;

      case 1:
         child = Obj_GetUnused();
         child->functionIndex = OBJF_BUTTON_DEPRESS;
         child->x1.s.hi = buttonLocations[obj->mem].x;
         child->z1.s.hi = buttonLocations[obj->mem].z;
         OBJ.child = child;
         obj->state2++;
         break;

      case 2:
         child = OBJ.child;
         if (child->state == 99) {
            obj->state3 = 0;
            obj->state2++;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(40));
         }
         break;

      case 3:
         PanCamera(doorLocations[obj->z3.n].x * CV(1.0) + CV(0.5),
                   GetTerrainElevation(doorLocations[obj->z3.n].z, doorLocations[obj->z3.n].x),
                   doorLocations[obj->z3.n].z * CV(1.0) + CV(0.5), 3);
         if (++obj->state3 >= 48) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(40));
            obj->state2++;
         }
         break;

      case 4:
         child = Obj_GetUnused();
         child->functionIndex = OBJF_MAP27_OPEN_CELL_DOOR;
         child->x1.s.hi = doorLocations[obj->z3.n].x;
         child->z1.s.hi = doorLocations[obj->z3.n].z;
         OBJ.child = child;
         obj->state2++;
         break;

      case 5:
         child = OBJ.child;
         if (child->functionIndex != OBJF_MAP27_OPEN_CELL_DOOR) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 6:
         if (obj->z3.n == 2) {
            obj->z3.n++;
            obj->state2 = 3;
         } else {
            obj->state2++;
         }
         break;

      case 7:
         EaseOutCamera(&OBJ.camera, 2);
         if (++obj->state3 >= 48) {
            AssignToMainCamera(&OBJ.camera);
            obj->state2++;
         }
         break;

      case 8:
         gPlayerControlSuppressed = 0;
         gState.signal = 0;
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 3:
      OBJ.buttonPressed[obj->mem] = 1;
      obj->state = 1;
      break;
   }
}
