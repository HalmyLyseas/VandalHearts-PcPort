/* Map 17's floodgate and Map 19's paired elevators.
 *
 * Map 17: Objf366_Map17_Button (spawned by SetupMapExtras()) waits on gState.mapState --
 * set by Objf433_EvaluateMap17_ProtectDolan -- runs the shared OBJF_BUTTON_DEPRESS cutaway, then
 * hands off to Objf365_Map17_Floodgate, which slides the gate tiles' faces open, sprays
 * Objf296_Map17_SprayParticle and spawns Objf368_Map17_DrainingWater to animate the channel
 * emptying. Map17_LowerFloodgate() and Map17_RemoveWater() are the instant
 * "already happened" versions applied when the map is reloaded after the fact.
 *
 * Map 19: Objf357_Map19 creates the two OBJF_MAP19_ELEVATOR platforms and, when
 * Objf435_EvaluateMap19_ReachExit sets mapState to 1 or 2 (which side was examined), spawns
 * Objf355_356_Map19_Elevator -- one handler at both indices, branching on its own
 * functionIndex to decide which platform rises and which sinks. Objf358_Map19_Elevator is
 * the per-platform tile-face motion.
 *
 * Objf354_Map19_DebugElevators has no spawn site and reads controller 2 -- a developer
 * toggle kept byte-exact. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

//...
void Map17_RemoveWater(void);
//...

void Map17_LowerFloodgate(void) {
   s32 ix;

   for (ix = 6; ix <= 7; ix++) {
      AdjustFaceElevation(&gMapRowPointers[15][ix], 1, 48);
      AdjustFaceElevation(&gMapRowPointers[15][ix], 2, 48);
      AdjustFaceElevation(&gMapRowPointers[15][ix], 3, 16);
      gTerrainPtr[15][ix].s.elevation = -gMapRowPointers[15][ix].vertices[0].vy >> 4;
      UpdateTileElevation(15, ix);
   }
}

#undef OBJF
#define OBJF 366
void Objf366_Map17_Button(Object *obj) {
   Object *buttonDepress;

   switch (obj->state) {
   case 0:
      obj->x1.s.hi = 10;
      obj->x1.s.lo = CV(0.5);
      obj->z1.s.hi = 16;
      obj->z1.s.lo = CV(0.5);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);

      if (gState.mapState.s.field_0x0 != 0) {
         Map17_RemoveWater();
         DepressButton(10, 16);
         Map17_LowerFloodgate();
         obj->functionIndex = OBJF_NULL;
         return;
      }

      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 1) {
         obj->state2 = 48;
         AssignFromMainCamera(&OBJ.camera);
         OBJ.unused_0x24 = 0x200;

         buttonDepress = Obj_GetUnused();
         buttonDepress->functionIndex = OBJF_BUTTON_DEPRESS;
         OBJ.buttonDepress = buttonDepress;
         buttonDepress->x1.n = obj->x1.n;
         buttonDepress->z1.n = obj->z1.n;
         buttonDepress->y1.n = obj->y1.n;

         gPlayerControlSuppressed = 1;
         obj->state += 2;
      }
      break;

   case 3:
      buttonDepress = OBJ.buttonDepress;
      if (buttonDepress->state == 99) {
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(35));
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 4:
      if (--obj->state3 <= 0) {
         gState.mapState.s.field_0x0 = 2;
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 5:
      if (++obj->state3 == 16) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(35));
      }
      if (gState.mapState.s.field_0x0 == 4) {
         obj->state3 = 48;
         obj->state++;
      }
      break;

   case 6:
      if (--obj->state3 <= 0) {
         obj->state3 = 8;
         obj->state++;
      }
      EaseOutCamera(&OBJ.camera, 2);
      break;

   case 7:
      if (--obj->state3 <= 0) {
         obj->functionIndex = OBJF_NULL;
         AssignToMainCamera(&OBJ.camera);
         gState.signal = 0;
         gPlayerControlSuppressed = 0;
      }
      break;
   }
}

#undef OBJF
#define OBJF 296
void Objf296_Map17_SprayParticle(Object *obj) {
   Quad unusedQuad = {{-2, -2, 0, 0}, {2, -2, 0, 0}, {-2, 2, 0, 0}, {2, 2, 0, 0}};

   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_DOTS;
      obj->d.sprite.boxIdx = 5;
      obj->d.sprite.clut = 3;
      obj->d.sprite.semiTrans = 4;

      obj->state3 = rand() % DEG(90) + DEG(225);
      obj->x2.n = CV(0.25) * rcos(obj->state3) >> 12;
      obj->z2.n = CV(0.21875) * rsin(obj->state3) >> 12;
      obj->y2.n = CV(0.25) + (CV(0.0625) * rsin(rand() % DEG(360)) >> 12);
      obj->y3.n = CV(-0.015625);

      obj->state2 = 0;
      obj->state++;

   // fallthrough
   case 1:
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      obj->x1.n += obj->x2.n;
      obj->z1.n += obj->z2.n;
      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;

      if (++obj->state2 >= 32) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 365
void Objf365_Map17_Floodgate(Object *obj) {
   s32 i;
   MapTileModel *tileModel;
   Object *drainage;
   Object *particle;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 != 0) {
         obj->functionIndex = OBJF_NULL;
         return;
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 2) {
         obj->state++;
      }
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         obj->state3 = 0;
         obj->x1.n = CV(6.5);
         obj->z1.n = CV(15.5);
         obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
         OBJ.dstCamRotY = DEG(-45);

         tileModel = &gMapRowPointers[15][6];
         OBJ.face1Init = OBJ.face1Curr = tileModel->vertices[tileModel->faces[1][0]].vy << 3;
         OBJ.face2Init = OBJ.face2Curr = tileModel->vertices[tileModel->faces[2][0]].vy << 3;
         OBJ.face3Init = OBJ.face3Curr = tileModel->vertices[tileModel->faces[3][0]].vy << 3;

         obj->y3.n = 8;
         obj->state2++;

      // fallthrough
      case 1:
         PanCamera(CV(6.5), obj->y1.n, CV(15.5), 3);
         gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 2;

         if (++obj->state3 >= 48) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 2:
         OBJ.face1Curr += obj->y3.n;
         OBJ.face2Curr += obj->y3.n;

         tileModel = &gMapRowPointers[15][6];
         SetFaceElevation(tileModel, 1, OBJ.face1Curr >> 3);
         SetFaceElevation(tileModel, 2, OBJ.face2Curr >> 3);

         tileModel = &gMapRowPointers[15][7];
         SetFaceElevation(tileModel, 1, OBJ.face1Curr >> 3);
         SetFaceElevation(tileModel, 2, OBJ.face2Curr >> 3);

         if (++obj->state3 >= 32) {
            drainage = Obj_GetUnused();
            drainage->functionIndex = OBJF_MAP17_DRAINING_WATER;
            obj->y3.n = 1;
            obj->state2++;
         }
         break;

      case 3:
         OBJ.face1Curr += obj->y3.n;
         OBJ.face2Curr += obj->y3.n;
         OBJ.face3Curr += obj->y3.n;

         tileModel = &gMapRowPointers[15][6];
         SetFaceElevation(tileModel, 1, OBJ.face1Curr >> 3);
         SetFaceElevation(tileModel, 2, OBJ.face2Curr >> 3);
         SetFaceElevation(tileModel, 3, OBJ.face3Curr >> 3);

         tileModel = &gMapRowPointers[15][7];
         SetFaceElevation(tileModel, 1, OBJ.face1Curr >> 3);
         SetFaceElevation(tileModel, 2, OBJ.face2Curr >> 3);
         SetFaceElevation(tileModel, 3, OBJ.face3Curr >> 3);

         if (((OBJ.face1Curr - OBJ.face1Init) >> 3) >= 48) {
            tileModel = &gMapRowPointers[15][6];
            SetFaceElevation(tileModel, 1, (OBJ.face1Init >> 3) + 48);
            SetFaceElevation(tileModel, 2, (OBJ.face2Init >> 3) + 48);
            SetFaceElevation(tileModel, 3, (OBJ.face3Init >> 3) + 16);

            tileModel = &gMapRowPointers[15][7];
            SetFaceElevation(tileModel, 1, (OBJ.face1Init >> 3) + 48);
            SetFaceElevation(tileModel, 2, (OBJ.face2Init >> 3) + 48);
            SetFaceElevation(tileModel, 3, (OBJ.face3Init >> 3) + 16);

            UpdateTileElevation(15, 6);
            UpdateTileElevation(15, 7);
            obj->state2++;
         }

         if (--obj->mem <= 0) {
            for (i = 0; i < 4; i++) {
               particle = Obj_GetUnused();
               particle->functionIndex = OBJF_MAP17_SPRAY_PARTICLE;
               particle->x1.n = i * CV(0.25) + CV(6.0);
               particle->z1.n = i * CV(0.25) + CV(15.0);
               particle->y1.n = GetTerrainElevation(14, 6 + i / 2);

               particle = Obj_GetUnused();
               particle->functionIndex = OBJF_MAP17_SPRAY_PARTICLE;
               particle->x1.n = i * CV(0.25) + CV(7.0);
               particle->z1.n = CV(15.0);
               particle->y1.n = GetTerrainElevation(14, 6 + i * 2);
            }
            obj->mem = (rand() >> 2) % 3 + 3;
         }
         break;

      case 4:
         obj->state3 = 0;
         obj->state++;
         break;
      } // switch (obj->state2) (via state:2)

      break;

   case 3:
      gTerrainPtr[15][6].s.elevation = -gMapRowPointers[15][6].vertices[0].vy >> 4;
      gTerrainPtr[15][7].s.elevation = -gMapRowPointers[15][7].vertices[0].vy >> 4;
      obj->state++;
      break;

   case 4:
      if (gState.mapState.s.field_0x0 == 3) {
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 5:
      if (--obj->state3 <= 0) {
         gState.mapState.s.field_0x0 = 4;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void Map17_RemoveWater(void) {
   s32 iz, ix;

   for (iz = 0; iz <= 1; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         AdjustFaceElevation(&gMapRowPointers[iz][ix], 0, 16);
      }
   }
   for (iz = 4; iz <= 14; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         AdjustFaceElevation(&gMapRowPointers[iz][ix], 0, 16);
      }
   }
   for (iz = 2; iz <= 3; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         AdjustFaceElevation(&gMapRowPointers[iz][ix], 1, 16);
      }
   }
   for (iz = 0; iz <= 1; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         gMapRowPointers[iz][ix].gfx[0] = GFX_MAP_TEXTURE_192;
         gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
         gTerrainPtr[iz][ix].s.terrain = TERRAIN_PLAINS;
      }
   }
   for (iz = 4; iz <= 14; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         gMapRowPointers[iz][ix].gfx[0] = GFX_MAP_TEXTURE_192;
         gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
         gTerrainPtr[iz][ix].s.terrain = TERRAIN_PLAINS;
      }
   }
   for (iz = 2; iz <= 3; iz++) {
      for (ix = 6; ix <= 7; ix++) {
         gMapRowPointers[iz][ix].gfx[1] = GFX_MAP_TEXTURE_192;
      }
   }
}

#undef OBJF
#define OBJF 368
void Objf368_Map17_DrainingWater(Object *obj) {
   static Quad quad = {{-128, 0, 128, 0}, {128, 0, 128, 0}, {-128, 0, -128, 0}, {128, 0, -128, 0}};

   s32 iz, ix, i, j;
   s16 elevation;
   Object *sprite;
   SVectorXZY *pCoord;
   POLY_FT4 *poly;
   u8 fade;
   s8 unused[32];

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      obj->state++;
      obj->state2 = 1;
      obj->state3 = 128;
      obj->mem = 4;
      break;

   case 2:
      Map17_RemoveWater();
      obj->state++;

   // fallthrough
   case 3:
      obj->mem = obj->state3 >> 5;

      if (obj->state3 != 0) {
         sprite = Obj_GetUnused();
         sprite->functionIndex = OBJF_NOOP;
         sprite->d.sprite.gfxIdx = GFX_MAP_TEXTURE_51; // Flowing water
         elevation = GetTerrainElevation(0, 6);

         for (iz = 0; iz <= 14; iz++) {
            for (ix = 6; ix <= 7; ix++) {
               pCoord = &sprite->d.sprite.coords[0];
               for (i = 0; i < 4; i++) {
                  pCoord->x = ix * CV(1.0) + quad[i].vx + CV(0.5);
                  pCoord->z = iz * CV(1.0) + quad[i].vz + CV(0.5);
                  pCoord->y = elevation + obj->state3;
                  pCoord++;
               }

               pCoord = &sprite->d.sprite.coords[0];
               sprite->x1.n = (pCoord[0].x + pCoord[1].x + pCoord[2].x + pCoord[3].x) / 4;
               sprite->z1.n = (pCoord[0].z + pCoord[1].z + pCoord[2].z + pCoord[3].z) / 4;
               sprite->y1.n = (pCoord[0].y + pCoord[1].y + pCoord[2].y + pCoord[3].y) / 4;

               sprite->d.sprite.semiTrans = 1;

               for (j = 0; j < obj->mem; j++) {
                  sprite->d.sprite.gfxIdx = GFX_MAP_TEXTURE_51;
                  AddObjPrim5(gGraphicsPtr->ot, sprite);
                  poly = &gGraphicsPtr->quads[gQuadIndex - 1];
                  fade = 0x100 - obj->state3;
                  setRGB0(poly, fade, fade, fade);
               }

               for (j = obj->mem; j < 2; j++) {
                  sprite->d.sprite.semiTrans = 2;
                  sprite->d.sprite.gfxIdx = GFX_MAP_TEXTURE_51;
                  poly = &gGraphicsPtr->quads[gQuadIndex];
                  AddObjPrim5(gGraphicsPtr->ot, sprite);

                  fade = obj->state3;
                  if (fade < 0x20) {
                     fade = 0x20;
                  }
                  setRGB0(poly, fade, fade, fade);
               }
            }
         }

         sprite->functionIndex = OBJF_NULL;
      }

      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 4:
      obj->state++;
      break;

   case 5:
      gState.mapState.s.field_0x0 = 3;
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 358
void Objf358_Map19_Elevator(Object *obj) {
   MapTileModel *tileModel;
   s8 sign;
   s16 diff;

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      if (obj->state2 != 1) {
         return;
      }
      obj->state++;

   // fallthrough
   case 2:
      obj->state3 = 0x100;
      OBJ.riseAccum0 = 0;
      OBJ.riseAccum1 = 0;
      OBJ.riseAccum2 = 0;
      OBJ.riseSpeed0 = 0x100;
      OBJ.riseSpeed1 = 0x100;
      OBJ.riseSpeed2 = 0x100;
      OBJ.riseApplied0 = 0;
      OBJ.riseApplied1 = 0;
      OBJ.riseApplied2 = 0;
      obj->state++;

   // fallthrough
   case 3:
      tileModel = &OBJ_TILE_MODEL(obj);
      sign = (obj->mem != 0) ? +1 : -1;

      OBJ.riseAccum0 += OBJ.riseSpeed0;
      diff = (OBJ.riseAccum0 >> 8) - (OBJ.riseApplied0 >> 8);
      if (diff > 0) {
         AdjustFaceElevation(tileModel, 0, diff * sign);
         OBJ.riseApplied0 += diff * 0x100;
      }

      OBJ.riseAccum1 += OBJ.riseSpeed1;
      diff = (OBJ.riseAccum1 >> 8) - (OBJ.riseApplied1 >> 8);
      if (diff > 0) {
         AdjustFaceElevation(tileModel, 1, diff * sign);
         OBJ.riseApplied1 += diff * 0x100;
      }

      OBJ.riseAccum2 += OBJ.riseSpeed2;
      diff = (OBJ.riseAccum2 >> 8) - (OBJ.riseApplied2 >> 8);
      if (diff > 0) {
         tileModel->vertices[tileModel->faces[2][0]].vy += diff * sign;
         tileModel->vertices[tileModel->faces[2][1]].vy += diff * sign;
         tileModel->vertices[tileModel->faces[2][2]].vy += diff * sign;
         OBJ.riseApplied2 += diff * 0x100;
         OBJ_TERRAIN(obj).s.elevation = -tileModel->vertices[0].vy >> 4;
      }

      if (OBJ.riseAccum0 >= 0x3000) {
         obj->state++;
      }
      break;

   case 4:
      OBJ_TERRAIN(obj).s.elevation = -OBJ_TILE_MODEL(obj).vertices[0].vy >> 4;
      obj->state2 = 0;
      obj->state++;
      break;

   case 5:
      if (obj->mem != 0) {
         obj->mem = 0;
      } else {
         obj->mem = 1;
      }
      obj->state = 1;
      break;

   case 6:
      break;
   }
}

#undef OBJF
#define OBJF 355
void Objf355_356_Map19_Elevator(Object *obj) {
   Object *elevatorA;
   Object *elevatorB;

   switch (obj->state) {
   case 0:
      if (obj->functionIndex == OBJF_MAP19_ELEVATOR_356) {
         OBJ.elevator2 = OBJ.elevator2Param;
         OBJ.elevator1 = OBJ.variant_0x24.elevator1Param;
      } else if (obj->functionIndex == OBJF_MAP19_ELEVATOR_355) {
         Obj_GetUnused(); //?
         OBJ.elevator2 = OBJ.elevator2Param;
         OBJ.elevator1 = OBJ.variant_0x24.elevator1Param;
      }
      AssignFromMainCamera(&OBJ.camera);
      elevatorB = OBJ.elevator1;
      elevatorA = OBJ.elevator2;
      if (elevatorB->z1.n < elevatorA->z1.n) {
         OBJ.variant_0x24.dstCamRotY.a = DEG(45);
         OBJ.variant_0x24.dstCamRotY.b = DEG(135);
      } else {
         OBJ.variant_0x24.dstCamRotY.a = DEG(135);
         OBJ.variant_0x24.dstCamRotY.b = DEG(45);
      }
      gPlayerControlSuppressed = 1;
      obj->state3 = 0;
      obj->state2 = 0;
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(38));
      obj->state++;
      break;

   case 1:
      elevatorB = OBJ.elevator1;

      switch (obj->state3) {
      case 0:
         PanCamera(elevatorB->x1.n + CV(0.5), elevatorB->y1.n + CV(0.5), elevatorB->z1.n + CV(0.5),
                   2);
         gCameraRotation.vy += (OBJ.variant_0x24.dstCamRotY.a - gCameraRotation.vy) >> 2;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraZoom.vz += (384 - gCameraZoom.vz) >> 2;
         if (++obj->state2 >= 48) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(38));
            obj->state3++;
         }
         break;

      case 1:
         gCameraRotation.vy = OBJ.variant_0x24.dstCamRotY.a;
         gCameraRotation.vx = DEG(33.75);
         gCameraZoom.vz = 384;
         obj->state3++;
         break;

      case 2:
         elevatorB = OBJ.elevator1;
         if (elevatorB->state2 == 0) {
            elevatorB->state2 = 1;
            obj->state2 = 0;
            obj->state3 = 0;
            obj->state++;
         }
         break;
      } // switch (obj->state3) (via state:1)

      break;

   case 2:
      elevatorB = OBJ.elevator1;
      if (elevatorB->state2 == 0) {
         obj->state++;
      }
      break;

   case 3:
      elevatorB = OBJ.elevator2;

      switch (obj->state3) {
      case 0:
         obj->state2 = 0;
         obj->state3++;

      // fallthrough
      case 1:
         if (++obj->state2 >= 32) {
            obj->state2 = 0;
            obj->state3++;
         }
         break;

      case 2:
         gCameraZoom.vz += (768 - gCameraZoom.vz) >> 3;
         if (++obj->state2 >= 32) {
            obj->state2 = 0;
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(38));
            obj->state3++;
         }
         break;

      case 3:
         PanCamera(elevatorB->x1.n + CV(0.5), elevatorB->y1.n + CV(0.5), elevatorB->z1.n + CV(0.5),
                   4);
         gCameraRotation.vy += (OBJ.variant_0x24.dstCamRotY.b - gCameraRotation.vy) >> 4;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 4;
         gCameraZoom.vz += (384 - gCameraZoom.vz) >> 4;
         if (++obj->state2 >= 64) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(38));
            obj->state3++;
         }
         break;

      case 4:
         gCameraRotation.vy = OBJ.variant_0x24.dstCamRotY.b;
         gCameraRotation.vx = DEG(33.75);
         gCameraZoom.vz = 384;
         obj->state3++;
         break;

      case 5:
         elevatorB = OBJ.elevator2;
         if (elevatorB->state2 == 0) {
            elevatorB->state2 = 1;
            obj->state2 = 0;
            obj->state3 = 0;
            obj->state++;
         }
      } // switch (obj->state3) (via state:3)

      break;

   case 4:
      elevatorB = OBJ.elevator2;
      if (elevatorB->state2 == 0) {
         obj->state++;
      }
      break;

   case 5:

      switch (obj->state3) {
      case 0:
         obj->state2 = 0;
         obj->state3++;

      // fallthrough
      case 1:
         if (++obj->state2 >= 32) {
            obj->state2 = 0;
            obj->state3++;
         }
         break;

      case 2:
         EaseOutCamera(&OBJ.camera, 2);
         if (++obj->state2 >= 48) {
            obj->state3++;
         }
         break;

      case 3:
         AssignToMainCamera(&OBJ.camera);
         obj->state3++;
         break;

      case 4:
         obj->functionIndex = OBJF_NULL;
         gPlayerControlSuppressed = 0;
         gState.mapState.s.field_0x0 = 0;
         gState.signal = 0;
         break;
      } // switch (obj->state3) (via state:5)

      break;
   }
}

#undef OBJF
#define OBJF 357
void Objf357_Map19(Object *obj) {
   Object *obj_s3;
   Object *obj_v1;
   Object *obj_s1;
   Object *obj_s0;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x1 != 0) {
         AdjustFaceElevation2(&gMapRowPointers[3][19], 0, -48);
         AdjustFaceElevation2(&gMapRowPointers[3][19], 1, -48);
         AdjustFaceElevation2(&gMapRowPointers[3][19], 2, -48);
         UpdateTileElevation(3, 19);
         AdjustFaceElevation2(&gMapRowPointers[12][19], 0, 48);
         AdjustFaceElevation2(&gMapRowPointers[12][19], 1, 48);
         AdjustFaceElevation2(&gMapRowPointers[12][19], 2, 48);
         UpdateTileElevation(12, 19);
      }

      obj_s3 = Obj_GetUnused();
      obj_s3->functionIndex = OBJF_MAP19_ELEVATOR;
      obj_s3->z1.s.hi = 3;
      obj_s3->x1.s.hi = 19;
      obj_s3->y1.n = CV(2.0);
      OBJ.elevator1 = obj_s3;

      obj_v1 = Obj_GetUnused();
      obj_v1->functionIndex = OBJF_MAP19_ELEVATOR;
      obj_v1->z1.s.hi = 12;
      obj_v1->x1.s.hi = 19;
      obj_v1->y1.n = CV(2.0);
      OBJ.elevator2 = obj_v1;

      if (gState.mapState.s.field_0x1 == 0) {
         obj_s3->mem = 0;
         obj_v1->mem = 1;
         gState.mapState.s.field_0x2 = 1;
      } else {
         obj_s3->mem = 1;
         obj_v1->mem = 0;
         gState.mapState.s.field_0x1 = 1;
         gState.mapState.s.field_0x2 = 0;
      }

      obj->state++;

   // fallthrough
   case 1:
      obj_s1 = OBJ.elevator1;
      obj_s0 = OBJ.elevator2;

      // Set to 1 or 2 by Objf435_EvaluateMap19_ReachExit, depending on which side is examined.
      if (gState.mapState.s.field_0x0 == 1) {
         obj_s3 = Obj_GetUnused();
         obj_s3->functionIndex = OBJF_MAP19_ELEVATOR_356;
         obj_s3->d.objf356.variant_0x24.elevator1Param = obj_s1;
         obj_s3->d.objf356.elevator2Param = obj_s0;
         obj->state++;
      } else if (gState.mapState.s.field_0x0 == 2) {
         obj_s3 = Obj_GetUnused();
         obj_s3->functionIndex = OBJF_MAP19_ELEVATOR_355;
         obj_s3->d.objf356.variant_0x24.elevator1Param = obj_s0;
         obj_s3->d.objf356.elevator2Param = obj_s1;
         obj->state++;
      }

      break;

   case 2:
      if (gState.mapState.s.field_0x0 == 0) {
         gState.mapState.s.field_0x1++;
         gState.mapState.s.field_0x1 &= 1;
         gState.mapState.s.field_0x2++;
         gState.mapState.s.field_0x2 &= 1;
         obj->state = 1;
      }
      break;
   }
}

#undef OBJF
#define OBJF 354
void Objf354_Map19_DebugElevators(Object *obj) {
   // Unused? (Name chosen based on location and behavior.)

   switch (obj->state) {
   case 0:
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 0 && (gSavedPad2StateNewPresses & PAD_L2)) {
         gState.mapState.s.field_0x0 = 1;
         obj->state++;
      } else if (gState.mapState.s.field_0x0 == 0 && (gSavedPad2StateNewPresses & PAD_R2)) {
         gState.mapState.s.field_0x0 = 2;
         obj->state++;
      }
      break;

   case 2:
      if (gState.mapState.s.field_0x0 == 0) {
         obj->state = 1;
      }
      break;
   }
}
