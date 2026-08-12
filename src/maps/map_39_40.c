/* Map 39's rising temple and Map 40's barricade.
 *
 * Map 39's trick: the submerged sections are authored 28 tiles to the right, outside the
 * visible bounds. Map39_Setup() shifts and sinks that staging copy into place, and
 * Map39_SwapSection() swaps a rectangle of it back in as each section surfaces, re-deriving
 * terrain from the tile's texture. Objf656_Map39 is the driver: as
 * Objf559_EvaluateMap39_EscortLeena advances mapState it pans the camera and raises the next section
 * using the local area helpers, dressing the emerging tiles with
 * Objf672_Map39_SplashingTile, which fires OBJF_SPLASH sprites as a tile's highest vertex
 * breaks the surface. Objf754_Map39_Scn82 is the event script's cutscene version of the
 * same rise. Map39_SetupSectionSplashing_2 is a byte-identical duplicate used only for the
 * last section -- do not merge, it holds distinct bytes.
 *
 * Map 40: Objf703_Map40_Barricade waits on mapState and spawns
 * Objf738_Map40_LowerBarricade, which sinks the barricade while shaking the camera and
 * venting OBJF_SMOKE; Map40_FlattenBarricade() is the instant version. Objf738's states 3-5
 * are unreachable copy-paste from Objf662_Map28_OpenDoor -- they still address Map 28's
 * gate tiles -- kept byte-exact. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

s32 GetIdxOfHighestVtx(MapTileModel *tileModel) {
   s32 i, j;
   s32 highest;
   s32 faceCt;

   faceCt = tileModel->faceCt;
   highest = 0;

   for (i = 0; i < faceCt; i++) {
      for (j = 0; j < 4; j++) {
         if (tileModel->vertices[tileModel->faces[i][j]].vy < tileModel->vertices[highest].vy) {
            highest = tileModel->faces[i][j];
         }
      }
   }

   return highest;
}

#undef OBJF
#define OBJF 672
void Objf672_Map39_SplashingTile(Object *obj) {
   MapTileModel *tileModel;
   Object *splash;
   s16 y;

   tileModel = OBJ.tileModel;

   switch (obj->state) {
   case 0:
      obj->mem = GetIdxOfHighestVtx(OBJ.tileModel);
      obj->state++;
      break;

   case 1:
      y = -tileModel->vertices[obj->mem].vy << 4; //?
      if (y > obj->y1.n - CV(0.5)) {
         obj->state3 = rand() % 16;
         obj->state++;
      }
      break;

   case 2:
      if (--obj->state3 <= 0) {
         splash = CreatePositionedObj(obj, OBJF_SPLASH);
         // TODO replace
         splash->d.sprite.coords[0].x = CV(0.25) * rsin(obj->state2 * DEG(22.5)) >> 12;
         obj->state3 += (rand() % 6 + 6);
         obj->state2++;
      }
      if (obj->state2 >= 8) {
         obj->state++;
      }
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void SwapTiles(MapTileModel *one, MapTileModel *two) {
   MapTileModel swap;

   swap = *one;
   *one = *two;
   *two = swap;
}

void RenderArea(s32 x1, s32 z1, s32 x2, s32 z2) {
   s32 ix, iz;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[iz][ix], GRID_COLOR_NONE);
      }
   }
}

void AdjustAreaElevation(s32 x1, s32 z1, s32 x2, s32 z2, s16 amount) {
   s32 ix, iz;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         AdjustTileElevation(&gMapRowPointers[iz][ix], amount);
      }
   }
}

void ShiftTile(MapTileModel *tileModel, s16 dx, s16 dz) {
   s32 i;

   for (i = 0; i < ARRAY_COUNT(tileModel->vertices); i++) {
      tileModel->vertices[i].vx += dx;
      tileModel->vertices[i].vz += dz;
   }
}

void ShiftArea(s32 x1, s32 z1, s32 x2, s32 z2, s16 dx, s16 dz) {
   s32 ix, iz;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         ShiftTile(&gMapRowPointers[iz][ix], dx, dz);
      }
   }
}

s32 Map39_GetTileTerrainType(MapTileModel *tileModel) {
   s32 gfxIdx;
   s32 terrain;

   gfxIdx = tileModel->gfx[0];
   terrain = TERRAIN_NO_ENTRY;

   if (gfxIdx >= GFX_MAP_TEXTURE_5) {
      terrain = TERRAIN_BARREN;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_21) {
      terrain = TERRAIN_VILE_BOG;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_25) {
      terrain = TERRAIN_PRAIRIE;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_51) {
      terrain = TERRAIN_WATER;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_54) {
      terrain = TERRAIN_BOUNDARY;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_55) {
      terrain = TERRAIN_THICKET;
   }
   if (gfxIdx >= GFX_MAP_TEXTURE_56) {
      terrain = TERRAIN_PLAINS;
   }
   return terrain;
}

void Map39_UpdateTerrainType(s16 x, s16 z) {
   gTerrainPtr[z][x].s.terrain = Map39_GetTileTerrainType(&gMapRowPointers[z][x]);
}

void Map39_SwapSection(s32 x1, s32 z1, s32 x2, s32 z2) {
   // The temple and platform tiles are stored beyond the visible bounds, and this is used to swap
   // in sections as they are revealed.

   s32 ix, iz;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         SwapTiles(&gMapRowPointers[iz][ix + 28], &gMapRowPointers[iz][ix]);
         gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
         Map39_UpdateTerrainType(ix, iz);
         UpdateTileElevation(iz, ix);
      }
   }
}

void UpdateAreaElevation(s32 x1, s32 z1, s32 x2, s32 z2) {
   s32 ix, iz;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         gTerrainPtr[iz][ix].s.elevation = -gMapRowPointers[iz][ix].vertices[0].vy >> 4;
         UpdateTileElevation(iz, ix);
      }
   }
}

void Map39_SetupSectionSplashing(s32 x1, s32 z1, s32 x2, s32 z2) {
   s32 ix, iz;
   Object *splash;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         if (Map39_GetTileTerrainType(&gMapRowPointers[iz][ix]) != TERRAIN_WATER) {
            splash = Obj_GetUnused();
            splash->functionIndex = OBJF_MAP39_SPLASHING_TILE;
            splash->x1.n = ix * CV(1.0) + CV(0.5);
            splash->z1.n = iz * CV(1.0) + CV(0.5);
            splash->y1.n = CV(0.5);
            splash->d.objf672.tileModel = &gMapRowPointers[iz][ix];
         }
      }
   }
}

void Map39_SetupSectionSplashing_2(s32 x1, s32 z1, s32 x2, s32 z2) {
   // Duplicate of previous function

   s32 ix, iz;
   Object *splash;

   for (ix = x1; ix < x2; ix++) {
      for (iz = z1; iz < z2; iz++) {
         if (Map39_GetTileTerrainType(&gMapRowPointers[iz][ix]) != TERRAIN_WATER) {
            splash = Obj_GetUnused();
            splash->functionIndex = OBJF_MAP39_SPLASHING_TILE;
            splash->x1.n = ix * CV(1.0) + CV(0.5);
            splash->z1.n = iz * CV(1.0) + CV(0.5);
            splash->y1.n = CV(0.5);
            splash->d.objf672.tileModel = &gMapRowPointers[iz][ix];
         }
      }
   }
}

#undef OBJF
#define OBJF 754
void Objf754_Map39_Scn82(Object *obj) {
   Object *focus;

   focus = OBJ.focus;

   switch (obj->state) {
   case 0:
      AdjustAreaElevation(28, 0, 56, 14, 256);
      ShiftArea(28, 0, 56, 14, CV(-28.0) >> 3, 0);
      focus = Obj_GetUnused();
      focus->functionIndex = OBJF_NOOP;
      OBJ.focus = focus;
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(32));
      obj->state++;

   // fallthrough
   case 1:
      obj->state2 = 0;
      obj->state++;
      gCameraRotation.vy |= 0xf000;
      gCameraRotation.vx &= 0xfff;
      AssignFromMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 1;
      focus->x1.n = CV(16.0);
      focus->y1.n = CV(1.0);
      focus->z1.n = CV(11.0);
      gState.focus = focus;
      break;

   case 2:

      switch (obj->state2) {
      case 0:
         gCameraRotation.vy += (DEG(45) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (512 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 24) {
            obj->x1.n = CV(16.0);
            obj->z1.n = CV(11.0);
            obj->y1.n = CV(1.0);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         Map39_SwapSection(19, 5, 24, 14);
         Map39_SwapSection(15, 7, 19, 14);
         Map39_SetupSectionSplashing(19, 5, 24, 14);
         Map39_SetupSectionSplashing(15, 7, 19, 14);
         obj->state3 = 0;
         obj->state2 = 0;
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(32));
         obj->state++;
         break;
      }

      break;

   case 3:
      RenderArea(47, 5, 52, 14);
      RenderArea(43, 7, 47, 14);

      switch (obj->state2) {
      case 0:
         AdjustAreaElevation(19, 5, 24, 14, -1);
         AdjustAreaElevation(15, 7, 19, 14, -1);
         if (++obj->state3 >= 192) {
            obj->x1.n += CV(3.0);
            obj->z1.n -= CV(2.0);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         AdjustAreaElevation(19, 5, 24, 14, -1);
         AdjustAreaElevation(15, 7, 19, 14, -1);
         gCameraRotation.vy += (DEG(-45) - gCameraRotation.vy) >> 4;
         if (++obj->state3 >= 64) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 4);
            obj->mem = rand() % 8;
         }
         if (++obj->state3 >= 32) {
            UpdateAreaElevation(19, 5, 24, 14);
            UpdateAreaElevation(15, 7, 19, 14);
            obj->state++;
            obj->state2 = 0;
            obj->state3 = 0;
         }
         break;
      }

      break;

   case 4:
      obj->state++;

   // fallthrough
   case 5:
      RenderArea(28, 0, 35, 14);

      switch (obj->state2) {
      case 0:
         focus->x1.n = CV(2.0);
         focus->y1.n = CV(2.0);
         focus->z1.n = CV(7.0);
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 8);
            obj->mem = rand() % 8;
         }
         gCameraRotation.vy += (DEG(157.5) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (512 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(45) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 48) {
            Map39_SwapSection(0, 0, 4, 4);
            Map39_SwapSection(0, 4, 7, 14);
            Map39_SetupSectionSplashing(0, 0, 7, 14);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 8);
            obj->mem = rand() % 8;
         }
         AdjustAreaElevation(0, 0, 4, 4, -1);
         AdjustAreaElevation(0, 4, 7, 14, -1);
         gCameraRotation.vy -= DEG(0.52734375);
         gCameraRotation.vx = DEG(45) + (DEG(22.5) * rsin(obj->state3 * 16) >> 12);
         if (++obj->state3 >= 256) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         if (++obj->state3 >= 128) {
            UpdateAreaElevation(0, 0, 4, 4);
            UpdateAreaElevation(0, 4, 7, 14);
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 6:
      obj->state++;

   // fallthrough
   case 7:
      obj->state++;
      gState.focus = NULL;
      gCameraRotation.vy &= 0xfff;
      gCameraRotation.vx &= 0xfff;

   // fallthrough
   case 8:
      EaseOutCamera(&OBJ.camera, 3);
      focus->x1.n = OBJ.camera.posX;
      focus->y1.n = OBJ.camera.posY;
      focus->z1.n = OBJ.camera.posZ;
      if (++obj->state3 >= 32) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 9:
      AssignToMainCamera(&OBJ.camera);
      obj->functionIndex = OBJF_NULL;
      break;
   }

   gState.eventCameraRot = gCameraRotation.vy;
}

s32 s_unused_80123344;

#undef OBJF
#define OBJF 656
void Objf656_Map39(Object *obj) {
   s32 amount;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 <= 1) {
         gState.mapState.s.field_0x0 = 1;
         obj->state = 12;
      } else {
         amount = -256;
         if (gState.mapState.s.field_0x0 >= 2) {
            AdjustAreaElevation(37, 0, 40, 4, amount);
            AdjustAreaElevation(40, 0, 43, 10, amount);
            AdjustAreaElevation(43, 0, 47, 7, amount);
            AdjustAreaElevation(47, 0, 52, 5, amount);
            Map39_SwapSection(9, 0, 12, 4);
            Map39_SwapSection(12, 0, 15, 10);
            Map39_SwapSection(15, 0, 19, 7);
            Map39_SwapSection(19, 0, 24, 5);
            obj->state = 18;
         }
         if (gState.mapState.s.field_0x0 >= 3) {
            AdjustAreaElevation(35, 4, 40, 11, -256);
            AdjustAreaElevation(39, 11, 43, 12, -256);
            Map39_SwapSection(7, 4, 12, 11);
            Map39_SwapSection(11, 11, 15, 12);
            obj->functionIndex = OBJF_NULL;
            return;
         }
         if (gState.mapState.s.field_0x0 == 0) {
            obj->state++;
         }
      }

      break;

   case 1:
      AdjustAreaElevation(28, 0, 56, 14, 256);
      obj->state++;

   // fallthrough
   case 2:
      ShiftArea(28, 0, 56, 14, CV(-28.0) >> 3, 0);
      obj->state++;

   // fallthrough
   case 3:
      if (gState.mapState.s.field_0x0 != 0) {
         obj->state2 = 0;
         obj->state++;
         gCameraRotation.vy &= 0xfff;
         gCameraRotation.vx &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 1;
      }
      break;

   case 4:

      switch (obj->state2) {
      case 0:
         PanCamera(CV(16.0), CV(1.0), CV(11.0), 2);
         gCameraRotation.vy += (DEG(45) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (512 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 24) {
            obj->x1.n = CV(16.0);
            obj->z1.n = CV(11.0);
            obj->y1.n = CV(1.0);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
         Map39_SwapSection(19, 5, 24, 14);
         Map39_SwapSection(15, 7, 19, 14);
         Map39_SetupSectionSplashing(19, 5, 24, 14);
         Map39_SetupSectionSplashing(15, 7, 19, 14);
         obj->state3 = 0;
         obj->state2 = 0;
         obj->state++;
         break;
      }

      break;

   case 5:
      RenderArea(47, 5, 52, 14);
      RenderArea(43, 7, 47, 14);

      switch (obj->state2) {
      case 0:
         AdjustAreaElevation(19, 5, 24, 14, -1);
         AdjustAreaElevation(15, 7, 19, 14, -1);
         if (++obj->state3 >= 192) {
            obj->x1.n += CV(3.0);
            obj->z1.n -= CV(2.0);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         AdjustAreaElevation(19, 5, 24, 14, -1);
         AdjustAreaElevation(15, 7, 19, 14, -1);
         gCameraRotation.vy += (DEG(-45) - gCameraRotation.vy) >> 4;
         PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
         if (++obj->state3 >= 64) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 4);
            obj->mem = rand() % 8;
         }
         if (++obj->state3 >= 32) {
            UpdateAreaElevation(19, 5, 24, 14);
            UpdateAreaElevation(15, 7, 19, 14);
            obj->state++;
            obj->state2 = 0;
            obj->state3 = 0;
         }
         break;
      }

      break;

   case 6:
      obj->state++;

   // fallthrough
   case 7:
      RenderArea(28, 0, 35, 14);

      switch (obj->state2) {
      case 0:
         PanCamera(CV(2.0), CV(2.0), CV(7.0), 3);
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 8);
            obj->mem = rand() % 8;
         }
         gCameraRotation.vy += (DEG(157.5) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (512 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(45) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 48) {
            Map39_SwapSection(0, 0, 4, 4);
            Map39_SwapSection(0, 4, 7, 14);
            Map39_SetupSectionSplashing(0, 0, 7, 14);
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         PanCamera(CV(2.0), CV(2.0), CV(7.0), 3);
         if (--obj->mem <= 0) {
            gCameraPos.vy -= (rand() % 8);
            obj->mem = rand() % 8;
         }
         AdjustAreaElevation(0, 0, 4, 4, -1);
         AdjustAreaElevation(0, 4, 7, 14, -1);
         gCameraRotation.vy -= DEG(0.52734375);
         gCameraRotation.vx = DEG(45) + (DEG(22.5) * rsin(obj->state3 * 16) >> 12);
         if (++obj->state3 >= 256) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 2:
         PanCamera(CV(2.0), CV(2.0), CV(7.0), 3);
         if (++obj->state3 >= 128) {
            UpdateAreaElevation(0, 0, 4, 4);
            UpdateAreaElevation(0, 4, 7, 14);
            obj->state3 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      }

      break;

   case 8:
      obj->state++;

   // fallthrough
   case 9:
      obj->state++;

   // fallthrough
   case 10:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 11:
      AssignToMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 0;
      obj->state++;
      break;

   case 12:
      if (gState.mapState.s.field_0x0 >= 2) {
         obj->state2 = 0;
         obj->state++;
         gCameraRotation.vy &= 0xfff;
         gCameraRotation.vx &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(32));
         gPlayerControlSuppressed = 1;
      }
      break;

   case 13:

      switch (obj->state2) {
      case 0:
         PanCamera(CV(16.0), CV(1.0), CV(3.0), 3);
         gCameraRotation.vy += (DEG(270) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (640 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(45) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 48) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         Map39_SwapSection(9, 0, 12, 4);
         Map39_SwapSection(12, 0, 15, 10);
         Map39_SwapSection(15, 0, 19, 7);
         Map39_SwapSection(19, 0, 24, 5);
         Map39_SetupSectionSplashing(9, 0, 12, 4);
         Map39_SetupSectionSplashing(12, 0, 15, 10);
         Map39_SetupSectionSplashing(15, 0, 19, 7);
         Map39_SetupSectionSplashing(19, 0, 24, 5);
         obj->state3 = 0;
         obj->state++;
         break;
      }

      break;

   case 14:
      RenderArea(37, 0, 40, 4);
      RenderArea(40, 0, 43, 10);
      RenderArea(43, 0, 47, 7);
      RenderArea(47, 0, 52, 5);
      gCameraRotation.vy -= DEG(0.52734375);
      gCameraRotation.vx = DEG(45) + (DEG(22.5) * rsin(obj->state3 * 16) >> 12);
      AdjustAreaElevation(9, 0, 12, 4, -1);
      AdjustAreaElevation(12, 0, 15, 10, -1);
      AdjustAreaElevation(15, 0, 19, 7, -1);
      AdjustAreaElevation(19, 0, 24, 5, -1);
      obj->state3++;
      if (obj->state3 == 30) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(32));
      }
      if (obj->state3 >= 256) {
         obj->state++;
         obj->state3 = 0;
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_1);
      }
      break;

   case 15:
      RenderArea(37, 0, 40, 4);
      RenderArea(40, 0, 43, 10);
      RenderArea(43, 0, 47, 7);
      RenderArea(47, 0, 52, 5);
      if (++obj->state3 >= 128) {
         UpdateAreaElevation(9, 0, 12, 4);
         UpdateAreaElevation(12, 0, 15, 10);
         UpdateAreaElevation(15, 0, 19, 7);
         UpdateAreaElevation(19, 0, 24, 5);
         obj->state3 = 0;
         obj->state++;
         OBJ.camera.rotY = ShortestTurnAngle(gCameraRotation.vy, OBJ.camera.rotY);
      }
      gCameraRotation.vy -= DEG(0.52734375);
      break;

   case 16:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 17:
      AssignToMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 0;
      gState.field_0x96 = 0;
      obj->state++;
      break;

   case 18:
      if (gState.mapState.s.field_0x0 >= 3) {
         s_unused_80123344 = 0;
         obj->state2 = 0;
         obj->state++;
         gCameraRotation.vy &= 0xfff;
         gCameraRotation.vx &= 0xfff;
         AssignFromMainCamera(&OBJ.camera);
         AdjustAreaElevation(35, 4, 40, 11, -64);
         AdjustAreaElevation(39, 11, 43, 12, -64);
         PerformAudioCommand(AUDIO_CMD_PREPARE_XA(32));
         gPlayerControlSuppressed = 1;
      }
      break;

   case 19:

      switch (obj->state2) {
      case 0:
         PanCamera(CV(9.0), CV(2.0), CV(7.0), 3);
         gCameraRotation.vy += (DEG(157.5) - gCameraRotation.vy) >> 2;
         gCameraZoom.vz += (640 - gCameraZoom.vz) >> 2;
         gCameraRotation.vx += (DEG(45) - gCameraRotation.vx) >> 2;
         if (++obj->state3 >= 48) {
            obj->state2++;
            obj->state3 = 0;
         }
         break;

      case 1:
         Map39_SwapSection(7, 4, 12, 11);
         Map39_SwapSection(11, 11, 15, 12);
         Map39_SetupSectionSplashing_2(7, 4, 12, 11);
         Map39_SetupSectionSplashing_2(11, 11, 15, 12);
         obj->state3 = 0;
         obj->state++;
         break;
      }

      break;

   case 20:
      RenderArea(35, 4, 40, 11);
      RenderArea(39, 11, 43, 12);
      if (obj->state3 < 192) {
         AdjustAreaElevation(7, 4, 12, 11, -1);
         AdjustAreaElevation(11, 11, 15, 12, -1);
         obj->state3++;
      }
      gCameraRotation.vy -= DEG(0.52734375);
      gCameraRotation.vx = DEG(45);
      if (obj->state3 == 20) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(32));
      }
      if (obj->state3 == 192) {
         obj->state++;
         obj->state3 = 0;
         PerformAudioCommand(AUDIO_CMD_FADE_OUT_8_1);
      }
      break;

   case 21:
      RenderArea(35, 4, 40, 11);
      RenderArea(39, 11, 43, 12);
      if (++obj->state3 >= 128) {
         UpdateAreaElevation(7, 4, 12, 11);
         UpdateAreaElevation(11, 11, 15, 12);
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 22:
      EaseOutCamera(&OBJ.camera, 3);
      if (++obj->state3 >= 32) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 23:
      AssignToMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 0;
      gState.field_0x96 = 0;
      obj->state++;
      break;

   case 24:
      break;
   }
}

void Map39_Setup(void) {
   // Submerged tiles start at x+28 (out-of-view)
   ShiftArea(28, 0, 56, 14, CV(-28.0) >> 3, 0);
   AdjustAreaElevation(28, 0, 56, 14, 256);
   AdjustAreaElevation(47, 5, 52, 14, -256);
   AdjustAreaElevation(43, 7, 47, 14, -256);
   AdjustAreaElevation(28, 0, 35, 14, -256);
   Map39_SwapSection(19, 5, 24, 14);
   Map39_SwapSection(15, 7, 19, 14);
   Map39_SwapSection(0, 0, 4, 4);
   Map39_SwapSection(0, 4, 7, 14);
}

#undef OBJF
#define OBJF 703
void Objf703_Map40_Barricade(Object *obj) {
   Object *lowerBarricade;

   switch (obj->state) {
   case 0:
      if (gState.mapState.s.field_0x0 == 0) {
         obj->state++;
      } else {
         Map40_FlattenBarricade();
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 1:
      if (gState.mapState.s.field_0x0 != 0) {
         lowerBarricade = Obj_GetUnused();
         lowerBarricade->functionIndex = OBJF_MAP40_LOWER_BARRICADE;
         obj->state++;
      }
      break;

   case 2:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 738
/* States 3-5 below are unreachable copy-paste from Objf662_Map28_OpenDoor -- they still
 * address Map 28's gate tiles, not Map 40's barricade. Kept byte-exact. */
void Objf738_Map40_LowerBarricade(Object *obj) {
   s32 i, j;
   Object *cloud;
   s32 tmp;

   switch (obj->state) {
   case 0:
      obj->state3 = 80;
      obj->y2.n = gCameraPos.vy;
      obj->state++;

   // fallthrough
   case 1:
      for (i = 16; i <= 17; i++) {
         for (j = 5; j <= 8; j++) {
            AdjustFaceElevation(&gMapRowPointers[j][i], 0, 1);
         }
         gGfxSubTextures[GFX_MAP_TEXTURE_232][3] = obj->state3 >> 1;
         gGfxSubTextures[GFX_MAP_TEXTURE_233][3] = obj->state3 >> 1;
      }
      if (--obj->z3.n <= 0) {
         for (i = 5; i <= 8; i++) {
            cloud = Obj_GetUnused();
            cloud->functionIndex = OBJF_SMOKE;
            cloud->x1.n = CV(16.0);
            cloud->z1.n = i * CV(1.0) + CV(0.5);
            cloud->y1.n = CV(1.5);
            cloud->x2.n = -rand() % CV(0.25);
            cloud->z2.n = rand() % CV(0.25) - CV(0.125);
            cloud->y2.n = rand() % CV(0.25);

            cloud = Obj_GetUnused();
            cloud->functionIndex = OBJF_SMOKE;
            cloud->x1.n = CV(18.0);
            cloud->z1.n = i * CV(1.0) + CV(0.5);
            cloud->y1.n = CV(1.5);
            cloud->x2.n = rand() % CV(0.25);
            cloud->z2.n = rand() % CV(0.25) - CV(0.125);
            cloud->y2.n = rand() % CV(0.25);
         }
         obj->z3.n += (rand() % 5 + 3);
      }
      gCameraPos.vy = obj->y2.n + rand() % 4;
      if (--obj->state3 <= 0) {
         for (i = 16; i <= 17; i++) {
            for (j = 5; j <= 8; j++) {
               UpdateTileElevation(j, i);
            }
         }
         gGfxSubTextures[GFX_MAP_TEXTURE_232][3] = 0;
         gGfxSubTextures[GFX_MAP_TEXTURE_233][3] = 0;
         gState.field_0x96 = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;

   case 3:
      gCameraPos.vy = obj->y2.n + rand() % 2;
      if (++obj->mem >= 32) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 4:
      rand(); //?
      tmp = obj->y2.n;
      gCameraPos.vy = tmp;
      if (++obj->mem >= 32) {
         obj->mem = 0;
         obj->state++;
      }
      break;

   case 5:
      for (j = 13; j <= 15; j++) {
         gTerrainPtr[j][13].s.elevation = -gMapRowPointers[j][13].vertices[0].vy >> 4;
      }
      obj->functionIndex = OBJF_NULL;
      gCameraPos.vy = obj->y2.n;
      break;

   case 6:
      break;
   }
}

void Map40_FlattenBarricade(void) {
   s32 i, j;

   for (i = 16; i <= 17; i++) {
      for (j = 5; j <= 8; j++) {
         AdjustFaceElevation(&gMapRowPointers[j][i], 0, 80);
         UpdateTileElevation(j, i);
      }
   }
}
