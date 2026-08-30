/* Map 32 (the rail/mine map): Objf650_Map32_CarRelease stages each released ore car's sweep
 * with field rendering suppressed, then rewrites the strip to bare rail (Map32_RemoveCar() on
 * reload); Objf673_Map32_Scn63_Cinematic is the scene-63 rail fly-through camera. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"
#include "battle.h"
#include "units.h"

static s16 sCarXPositions[4] = {CV(10.0), CV(20.0), CV(32.0), CV(42.0)};

// TODO: Integrate autogen
static MapTileModel sMapTileModels_801009bc[4] = {
#include "assets/801009bc.inc"
};

#undef OBJF
#define OBJF 673
void Objf673_Map32_Scn63_Cinematic(Object *obj) {
   Object *focus;

   focus = OBJ.focus;

   switch (obj->state) {
   case 0:
      AssignFromMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 1;
      obj->state++;
      obj->x1.n = CV(108.0);
      obj->z1.n = CV(4.0);
      obj->y1.n = CV(8.0);
      obj->x2.n = CV(60.0);
      obj->z2.n = CV(4.0);
      obj->y2.n = CV(2.5);
      PerformAudioCommand(AUDIO_CMD_PLAY_XA(84));
      focus = Obj_GetUnused();
      focus->functionIndex = OBJF_NOOP;
      OBJ.focus = focus;
      focus->x1.n = obj->x2.n;
      focus->z1.n = obj->z2.n;
      focus->y1.n = obj->y2.n;
      gState.focus = focus;
      SnapCameraLookAt(obj);
      break;

   case 1:
      EaseCameraLookAt(obj);
      focus->x1.n = obj->x2.n;
      focus->z1.n = obj->z2.n;
      focus->y1.n = obj->y2.n;
      obj->state++;

   // fallthrough
   case 2:
      obj->x1.n -= CV(0.5);
      obj->y1.n += CV(0.0625);
      EaseCameraLookAt(obj);
      focus->x1.n = obj->x2.n;
      focus->z1.n = obj->z2.n;
      focus->y1.n = obj->y2.n;

      if (++obj->mem >= 64) {
         obj->state++;
         obj->z3.n = CV(16.0);
         obj->state3 = 0;
      }
      break;

   case 3:
      obj->x1.n = obj->x2.n + (obj->z3.n * rcos(obj->state3) >> 12);
      obj->z1.n = obj->z2.n + (obj->z3.n * rsin(obj->state3) >> 12);
      obj->y1.n -= CV(0.015625);
      obj->x2.n -= CV(0.25);

      if (obj->x2.n < CV(7.0)) {
         obj->x2.n = CV(7.0);
      }

      switch (obj->state2) {
      case 0:
         obj->z3.n -= CV(0.25);
         if (obj->z3.n <= CV(5.5)) {
            OBJ.unused_0x3C = 0;
            obj->state2++;
         }
         break;

      case 1:
         obj->z3.n += CV(0.375);
         if (obj->z3.n >= CV(24.0)) {
            obj->state2++;
         }
         break;

      case 2:
         obj->z3.n -= CV(0.0625);
         if (obj->z3.n <= CV(16.5)) {
            obj->state2++;
         }
         break;
      }

      EaseCameraLookAt((Object *)obj);
      focus->x1.n = obj->x2.n;
      focus->z1.n = obj->z2.n;
      focus->y1.n = obj->y2.n;
      obj->state3 -= 8;

      if (++obj->mem >= 512) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 4:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->mem >= 64) {
         obj->state++;
      }
      break;

   case 5:
      AssignToMainCamera(&OBJ.camera);
      gState.focus = NULL;
      obj->state++;
      break;

   case 6:
      obj->functionIndex = OBJF_NULL;
      break;
   }

   gState.eventCameraRot = gCameraRotation.vy;
}

void Map32_RemoveCar(s32 idx) {
   extern void ShiftTile(MapTileModel *, s16, s16);

   s32 x;
   s32 ix, iz;
   MapTileModel *tileModel;
   MapTileModel *railTileModel;

   x = sCarXPositions[idx] / CV(1.0);

   for (ix = 2; ix < x; ix++) {
      for (iz = 0; iz <= 7; iz++) {
         // Replace tiles w/ empty rail, and shift into place
         tileModel = &gMapRowPointers[iz][ix];
         railTileModel = &gMapRowPointers[iz][ix % 2];
         *tileModel = *railTileModel;
         ShiftTile(tileModel, ix / 2 * 64, 0);
         gTerrainPtr[iz][ix].s.terrain = TERRAIN_NO_ENTRY;
         gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
      }
   }
}

#undef OBJF
#define OBJF 650
void Objf650_Map32_CarRelease(Object *obj) {
   extern void ShiftTile(MapTileModel *, s16, s16);

   static s32 audioCommands[5] = {AUDIO_CMD_PLAY_XA(85), AUDIO_CMD_PLAY_XA(85),
                                  AUDIO_CMD_PLAY_XA(85), AUDIO_CMD_PLAY_XA(86),
                                  AUDIO_CMD_PLAY_XA(86)};

   s32 i, j, k;
   s16 startX, endX;
   Object *obj_s0;
   Object *unitSprite;
   UnitStatus *unit;
   SVECTOR translation;
   SVECTOR rotation = {0, 0, 0};
   MapTileModel tileModels[4];
   MapTileModel *tileModel;
   MapTileModel *tileModel2;
   s16 savedCamPosX, savedCamPosY;
   s32 flag;

   startX = OBJ.startX / CV(1.0);
   endX = OBJ.endX / CV(1.0);

   switch (obj->state) {
   case 0:
      for (i = 0; i < ARRAY_COUNT(sMapTileModels_801009bc); i++) {
         UpdateMapTileLighting(&sMapTileModels_801009bc[i]);
      }
      if (gState.mapState.s.field_0x0 != 0) {
         Map32_RemoveCar(gState.mapState.s.field_0x0 - 1);
         OBJ.startX = sCarXPositions[gState.mapState.s.field_0x0 - 1];
         OBJ.carIdx = gState.mapState.s.field_0x0;
      } else {
         OBJ.startX = CV(2.0);
         OBJ.carIdx = 0;
         OBJ.endX = 0;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 > OBJ.carIdx && OBJ.carIdx <= 3) {
         AssignFromMainCamera(&OBJ.camera);
         obj->x1.n = sCarXPositions[OBJ.carIdx];
         obj->z1.n = CV(6.0);
         obj->y1.n = CV(4.0);

         obj_s0 = Obj_GetUnused();
         obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
         obj_s0->d.objf026.target = obj;
         obj_s0->d.objf026.zoom = 384;

         obj->state2 = 0;
         obj->state++;
         OBJ.endX = sCarXPositions[OBJ.carIdx];
         obj->x2.n = 0;
         obj->x3.n = -3;
         OBJ.camOffsetAccum = 0;

         startX = OBJ.startX / CV(1.0);
         endX = OBJ.endX / CV(1.0);
         OBJ.hasCasualties = 0;
         k = 0;
         for (i = startX; i < endX; i++) {
            for (j = 0; j < 8; j++) {
               if (gMapUnitsPtr[j][i].s.unitIdx != 0) {
                  k++;
               }
            }
         }
         if (k != 0) {
            OBJ.hasCasualties = 1;
         }
         if (OBJ.hasCasualties) {
            PerformAudioCommand(0x1000 + audioCommands[0]);
         } else {
            PerformAudioCommand(0x1000 + audioCommands[OBJ.carIdx]);
         }
         gPlayerControlSuppressed = 1;
      }
      break;

   case 2:
      switch (obj->state2) {
      case 0:
         if (++obj->state3 >= 32) {
            obj_s0 = Obj_GetUnused();
            obj_s0->functionIndex = OBJF_BOUNCE_ZOOM;
            if (OBJ.hasCasualties) {
               PerformAudioCommand(audioCommands[0]);
            } else {
               PerformAudioCommand(audioCommands[OBJ.carIdx]);
            }
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 1:
         if (++obj->state3 >= 4) {
            for (i = 0; i < 16; i++) {
               obj_s0 = CreatePositionedObj(obj, OBJF_MAP32_SMOKESTACK_PARTICLE);
               obj_s0->z2.n = rand() % CV(0.37890625) - CV(0.1875);
               obj_s0->x2.n = rand() % CV(0.015625) + CV(0.109375);
               obj_s0->y2.n = rand() % CV(0.015625) + CV(0.0078125);
               obj_s0->z3.n = obj_s0->z2.n >> 2;
               obj_s0->x3.n = -(rand() % CV(0.015625) + CV(0.125));
               obj_s0->z1.n = i * CV(0.125) + CV(4.0);
            }
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 2:
         if (++obj->state3 >= 30) {
            startX = OBJ.startX / CV(1.0);
            endX = OBJ.endX / CV(1.0);
            k = 0;
            for (i = startX; i < endX; i++) {
               for (j = 0; j < 8; j++) {
                  if (gMapUnitsPtr[j][i].s.unitIdx != 0) {
                     k++;
                  }
               }
            }
            if (k != 0) {
               PerformAudioCommand(AUDIO_CMD_FADE_OUT_128_1);
               obj->state2 += 2;
               obj->state3 = 0;
            } else {
               obj->state2++;
               obj->state3 = 0;
            }
         }
         break;

      case 3:
         if (++obj->state3 >= 15) {
            obj->state3 = 0;
            obj->state2 += 3;
         }
         break;

      case 4:
         if (++obj->state3 >= 0) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 5:
         startX = OBJ.startX / CV(1.0);
         endX = OBJ.endX / CV(1.0);

         switch (obj->state3) {
         case 0:
            OBJ.tileX = startX;
            obj->state3++;

         // fallthrough
         case 1:
            OBJ.tileZ = 0;
            obj->state3++;

         // fallthrough
         case 2:
            i = OBJ.tileX;
            j = OBJ.tileZ;
            if (gMapUnitsPtr[j][i].s.unitIdx != 0) {
               gState.msgFinished = 0;
               gTileStateGridPtr[j][i].action = TA_32;
               obj->state3++;
            } else {
               obj->state3 += 3;
            }
            break;

         case 3:
            if (gState.msgFinished) {
               obj->state3++;
            }
            break;

         case 4:
            obj->state3++;

         // fallthrough
         case 5:
            OBJ.tileZ++;
            if (OBJ.tileZ >= 8) {
               obj->state3++;
            } else {
               obj->state3 = 2;
            }
            break;

         case 6:
            OBJ.tileX++;
            if (OBJ.tileX >= endX) {
               OBJ.timer = 0;
               obj->state3++;
               obj_s0 = Obj_GetUnused();
               obj_s0->functionIndex = OBJF_FOCUS_CAMERA;
               obj_s0->d.objf026.target = obj;
               obj_s0->d.objf026.zoom = 384;
               PerformAudioCommand(0x1000 + audioCommands[OBJ.carIdx]); // Prepare XA
            } else {
               obj->state3 = 1;
            }
            break;

         case 7:
            if (++OBJ.timer >= 32) {
               PerformAudioCommand(audioCommands[OBJ.carIdx]); // Play XA
               for (i = 0; i < 16; i++) {
                  obj_s0 = CreatePositionedObj(obj, OBJF_MAP32_SMOKESTACK_PARTICLE);
                  obj_s0->z2.n = rand() % CV(0.37890625) - CV(0.1875);
                  obj_s0->x2.n = rand() % CV(0.015625) + CV(0.109375);
                  obj_s0->y2.n = rand() % CV(0.015625) + CV(0.0078125);
                  obj_s0->z3.n = obj_s0->z2.n >> 2;
                  obj_s0->x3.n = -(rand() % CV(0.015625) + CV(0.125));
                  obj_s0->z1.n = i * CV(0.125) + CV(4.0);
               }
               OBJ.timer = 0;
               obj->state3++;
            }
            break;

         case 8:
            if (++OBJ.timer >= 60) {
               obj->state3 = 0;
               obj->state2++;
            }
            break;
         } // switch (obj->state3) (via state:2->state2:5)

         break;

      case 6:
         startX = OBJ.startX / CV(1.0);
         endX = OBJ.endX / CV(1.0);

         for (i = startX; i < endX; i++) {
            for (j = 0; j < 8; j++) {
               tileModel = &gMapRowPointers[j][i];
               for (k = 0; k < ARRAY_COUNT(tileModel->gfx); k++) {
                  if (tileModel->gfx[k] >= GFX_MAP_TEXTURE_7 &&
                      tileModel->gfx[k] <= GFX_MAP_TEXTURE_18) {
                     tileModel->gfx[k] = GFX_TRANSPARENT;
                  }
                  if (tileModel->gfx[k] >= GFX_MAP_TEXTURE_23 &&
                      tileModel->gfx[k] <= GFX_MAP_TEXTURE_34) {
                     tileModel->gfx[k] = GFX_TRANSPARENT;
                  }
               }
            }
         }

         for (i = startX / 2; i < endX / 2; i++) {
            translation.vx = (i + 1) * 512;
            translation.vy = 0;
            translation.vz = -512;
            for (j = 0; j < ARRAY_COUNT(tileModels); j++) {
               tileModels[j] = sMapTileModels_801009bc[j];
               RotTransMapTile(&sMapTileModels_801009bc[j], &rotation, &translation,
                               &tileModels[j]);
            }
         }

         gState.fieldRenderingDisabled = 1;
         obj->state3 = 0;
         obj->state2 = 0;
         obj->state++;

         break;
      } // switch (obj->state2) (via state:2)

      break;

   case 3:
      gCameraZoom.vz += 8;
      startX = OBJ.startX / CV(1.0);
      endX = OBJ.endX / CV(1.0);

      k = endX + 16;
      if (k > 64) {
         k = 64;
      }

      for (i = gMapViewOriginX; i < startX; i++) {
         for (j = 1; j < 7; j++) {
            tileModel = &gMapRowPointers[j][i];
            RenderMapTile(gGraphicsPtr->ot, tileModel, GRID_COLOR_NONE);
         }
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[0][i], GRID_COLOR_NONE);
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[7][i], GRID_COLOR_NONE);
      }

      for (i = endX; i < k; i++) {
         for (j = 1; j < 7; j++) {
            tileModel = &gMapRowPointers[j][i];
            RenderMapTile(gGraphicsPtr->ot, tileModel, GRID_COLOR_NONE);
         }
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[0][i], GRID_COLOR_NONE);
         RenderEdgeMapTile(gGraphicsPtr->ot, &gMapRowPointers[7][i], GRID_COLOR_NONE);
      }

      for (i = (startX - 10) / 2; i < endX / 2; i++) {
         translation.vx = (i + 1) * 512;
         translation.vy = 0;
         translation.vz = -512;
         for (j = 0; j < ARRAY_COUNT(tileModels); j++) {
            tileModels[j] = sMapTileModels_801009bc[j];
            RotTransMapTile(&sMapTileModels_801009bc[j], &rotation, &translation, &tileModels[j]);
            RenderEdgeMapTile(gGraphicsPtr->ot, &tileModels[j], GRID_COLOR_NONE);
         }
      }

      savedCamPosY = gCameraPos.vy;
      savedCamPosX = gCameraPos.vx;
      PushMatrix();
      RotMatrix(&gCameraRotation, &gCameraMatrix);
      TransMatrix(&gCameraMatrix, &gCameraZoom);
      SetRotMatrix(&gCameraMatrix);
      SetTransMatrix(&gCameraMatrix);
      gCameraPos.vx += OBJ.camOffsetX;
      RotTrans(&gCameraPos, (VECTOR *)gCameraMatrix.t, &flag);
      SetTransMatrix(&gCameraMatrix);

      for (i = startX; i < endX; i++) {
         for (j = 0; j < 8; j++) {
            tileModel = &gMapRowPointers[j][i];
            RenderMapTile(gGraphicsPtr->ot, tileModel, GRID_COLOR_NONE);
         }
      }

      obj_s0 = Obj_GetUnused();
      obj_s0->functionIndex = OBJF_NOOP;

      for (i = 0; i < UNIT_CT; i++) {
         if (gUnits[i].idx != 0) {
            unit = &gUnits[i];
            unitSprite = unit->sprite;
            if (unitSprite->x1.n < OBJ.endX) {
               unitSprite->d.sprite.hidden = 1;
               CopyObject(unitSprite, obj_s0);
               obj_s0->d.sprite.hidden = 0;
               RenderUnitSprite(gGraphicsPtr->ot, obj_s0, 0);
            }
         }
      }

      obj_s0->functionIndex = OBJF_NULL;
      PopMatrix();
      gCameraPos.vx = savedCamPosX;
      OBJ.camOffsetAccum += obj->x2.n;
      obj->x2.n += obj->x3.n;
      OBJ.camOffsetX = OBJ.camOffsetAccum >> 3;
      if (OBJ.camOffsetX > -0x400) {
         break;
      }

      obj->state++;

   // fallthrough
   case 4:
      for (i = startX; i < endX; i++) {
         for (j = 0; j < 8; j++) {
            tileModel2 = &gMapRowPointers[j][i];
            gMapRowPointers[j][i] = gMapRowPointers[j][i % 2];
            ShiftTile(tileModel2, i / 2 * 64, 0);
            if (gMapUnitsPtr[j][i].s.unitIdx != 0) {
               gTileStateGridPtr[j][i].action = TA_22;
            }
            gTerrainPtr[j][i].s.terrain = TERRAIN_NO_ENTRY;
            gTerrainPtr[j][i].s.elevation = -gMapRowPointers[j][i].vertices[0].vy >> 4;
         }
      }
      gState.fieldRenderingDisabled = 0;
      obj->state++;

   // fallthrough
   case 5:
      EaseOutCamera(&OBJ.camera, 2);
      if (++obj->state3 >= 32) {
         AssignToMainCamera(&OBJ.camera);
         OBJ.camOffsetX = 0;
         OBJ.carIdx++;
         OBJ.startX = OBJ.endX;
         obj->state = 1;
         obj->state3 = 0;
         gPlayerControlSuppressed = 0;
         gState.field_0x96 = 0;
         if (OBJ.carIdx > 2) {
            PerformAudioCommand(AUDIO_CMD_FADE_OUT_4_1);
         }
      }
      break;
   }
}
