/* The shared button-press cutaway, Map 26/29's drawbridge levers, and Map 14's sand pit --
 * plus the camera save/restore/ease helpers the whole corpus uses.
 *
 * Buttons: DepressButton() sinks a lever tile instantly; Objf346_ButtonDepress
 * (OBJF_BUTTON_DEPRESS) is the animated cutaway -- rotate and pan the camera onto the tile,
 * sink it over four frames, then park in state 99 so the parent can notice before it frees
 * itself. Maps 17, 26, 27, 28, 29, 35 and 38 all spawn it. Objf347_Map26 and Objf352_Map29
 * are those maps' lever drivers (mapState 1 -> cutaway -> mapState 2 for
 * Objf362_DrawbridgeButton -> ease the camera back at 3); Map 11's is Objf399_Map11.
 *
 * Map 14 sand: Objf670_Map14_Sand raises and lowers a diamond dune via
 * Objf668/666_Map14_Raise/LowerSandMound, which walk outward ring by ring spawning one
 * Objf669/667_Map14_*SandTile per tile. Objf752_Map14_Scn15_SandMoundSpawner is the event
 * script's scene-15 entry to the same mound. Objf708_709_Map14_Unused is a leftover pair of
 * mapState pokes with no spawn site.
 *
 * Shared camera: AssignFrom/ToMainCamera, EaseOutShort/EaseOutInt and EaseOutCamera -- the
 * save/restore/ease-back idiom every scripted map effect wraps its cutaway in. */
#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "audio.h"

void DepressButton(s32 x, s32 z) {
   AdjustFaceElevation(&gMapRowPointers[z][x], 0, 16);
   gTerrainPtr[z][x].s.elevation = -gMapRowPointers[z][x].vertices[0].vy >> 4;
}

#undef OBJF
#define OBJF 346
void Objf346_ButtonDepress(Object *obj) {
   switch (obj->state) {
   case 0:
      OBJ.dstCamRotY = GetBestViewOfTarget(obj->z1.s.hi, obj->x1.s.hi, 1);
      gCameraRotation.vy &= 0xfff;
      OBJ.dstCamRotY = ShortestTurnAngle(gCameraRotation.vy, DEG(135));
      obj->state2 = 48;
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      PerformAudioCommand(AUDIO_CMD_PREPARE_XA(33));
      obj->state++;
      break;

   case 1:
      if (--obj->state2 <= 0) {
         gCameraRotation.vy = OBJ.dstCamRotY;
         gCameraRotation.vx = DEG(33.75);
         obj->state3 = 4;
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(33));
         obj->state++;
      }
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 3);
      gCameraRotation.vy += (OBJ.dstCamRotY - gCameraRotation.vy) >> 3;
      gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 3;
      gCameraZoom.vz += (384 - gCameraZoom.vz) >> 3;
      break;

   case 2:
      AdjustFaceElevation(&OBJ_TILE_MODEL(obj), 0, 4);
      OBJ_TERRAIN(obj).s.elevation = -OBJ_TILE_MODEL(obj).vertices[0].vy >> 4;
      if (--obj->state3 <= 0) {
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 3:
      if (--obj->state3 <= 0) {
         obj->state = 99;
      }
      break;

   case 99:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 347
void Objf347_Map26(Object *obj) {
   Object *buttonDepress;

   switch (obj->state) {
   case 0:
      obj->state++;
      if (gState.mapState.s.field_0x0 != 0) {
         obj->functionIndex = OBJF_NULL;
      }

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 1) {
         obj->state2 = 48;
         AssignFromMainCamera(&OBJ.camera);

         buttonDepress = Obj_GetUnused();
         buttonDepress->functionIndex = OBJF_BUTTON_DEPRESS;
         buttonDepress->x1.n = CV(35.0);
         buttonDepress->z1.n = CV(8.0);
         OBJ.buttonDepress = buttonDepress;

         gPlayerControlSuppressed = 1;
         obj->state++;
      }
      break;

   case 2:
      buttonDepress = OBJ.buttonDepress;
      if (buttonDepress->state == 99) {
         obj->state++;
      }
      break;

   case 3:
      gState.mapState.s.field_0x0 = 2;
      obj->state++;
      break;

   case 4:
      if (gState.mapState.s.field_0x0 == 3) {
         gCameraRotation.vy &= 0xfff;
         OBJ.camera.posX = ShortestTurnAngle(gCameraRotation.vy, OBJ.camera.posX);
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 5:
      EaseOutCamera(&OBJ.camera, 2);
      if (--obj->state3 <= 0) {
         AssignToMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 0;
         gState.signal = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 352
void Objf352_Map29(Object *obj) {
   Object *buttonDepress;

   switch (obj->state) {
   case 0:
      obj->state++;
      if (gState.mapState.s.field_0x0 != 0) {
         obj->functionIndex = OBJF_NULL;
      }
      break;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 == 1) {
         obj->state2 = 48;
         AssignFromMainCamera(&OBJ.camera);

         buttonDepress = Obj_GetUnused();
         buttonDepress->functionIndex = OBJF_BUTTON_DEPRESS;
         buttonDepress->x1.n = CV(18.0);
         buttonDepress->z1.n = CV(12.0);
         OBJ.buttonDepress = buttonDepress;

         gPlayerControlSuppressed = 1;
         obj->state++;
      }
      break;

   case 2:
      buttonDepress = OBJ.buttonDepress;
      if (buttonDepress->state == 99) {
         obj->state++;
      }
      break;

   case 3:
      gState.mapState.s.field_0x0 = 2;
      obj->state++;
      break;

   case 4:
      if (gState.mapState.s.field_0x0 == 3) {
         gCameraRotation.vy &= 0xfff;
         OBJ.camera.posX = ShortestTurnAngle(gCameraRotation.vy, OBJ.camera.posX);
         obj->state3 = 32;
         obj->state++;
      }
      break;

   case 5:
      EaseOutCamera(&OBJ.camera, 2);
      if (--obj->state3 <= 0) {
         AssignToMainCamera(&OBJ.camera);
         gPlayerControlSuppressed = 0;
         gState.signal = 0;
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 667
void Objf667_Map14_LowerSandTile(Object *obj) {
   extern void UpdateTileElevation(u8, u8);

   s32 tileZ, tileX;
   s32 i;
   MapTileModel *tileModel;

   tileModel = &OBJ_TILE_MODEL(obj);
   tileX = obj->x1.s.hi;
   tileZ = obj->z1.s.hi;

   switch (obj->state) {
   case 0:
      obj->state3 = 0;
      obj->state++;

   // fallthrough
   case 1:
      if (obj->mem > obj->state2) {
         AdjustFaceElevation(&gMapRowPointers[tileZ][tileX], 0, obj->state2);
         obj->mem -= obj->state2;
      } else {
         AdjustFaceElevation(&gMapRowPointers[tileZ][tileX], 0, obj->mem);
         obj->mem = 0;
         obj->state++;
      }
      UpdateMapTileLighting(tileModel);
      gTerrainPtr[tileZ][tileX].s.elevation = -gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4;
      break;

   case 2:
      for (i = 1; i < 5; i++) {
         tileModel->gfx[i] = GFX_NULL;
      }
      UpdateTileElevation(tileZ, tileX);
      UpdateMapTileLighting(tileModel);
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 669
void Objf669_Map14_RaiseSandTile(Object *obj) {
   extern void UpdateTileElevation(u8, u8);

   s32 tileZ, tileX;
   s32 i;
   MapTileModel *tileModel;

   tileModel = &OBJ_TILE_MODEL(obj);
   tileX = obj->x1.s.hi;
   tileZ = obj->z1.s.hi;

   switch (obj->state) {
   case 0:
      obj->state3 = 0;
      obj->state++;
      for (i = 1; i < 5; i++) {
         tileModel->gfx[i] = GFX_MAP_TEXTURE_1; // Flowing sand
      }

   // fallthrough
   case 1:
      if (obj->mem < obj->state2) {
         AdjustFaceElevation(&gMapRowPointers[tileZ][tileX], 0, obj->state2);
         obj->mem -= obj->state2;
      } else {
         AdjustFaceElevation(&gMapRowPointers[tileZ][tileX], 0, obj->mem);
         obj->mem = 0;
         obj->state++;
      }
      UpdateMapTileLighting(tileModel);
      gTerrainPtr[tileZ][tileX].s.elevation = -gMapRowPointers[tileZ][tileX].vertices[0].vy >> 4;
      break;

   case 2:
      UpdateTileElevation(tileZ, tileX);
      UpdateMapTileLighting(tileModel);
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void Map14_AdjustSandTile(s32 x, s32 z, s32 amount) {
   extern void UpdateTileElevation(u8, u8);

   s32 i;
   MapTileModel *tileModel;

   if ((x >= 0 && x < 18) && (z >= 0 && z < 18)) {
      tileModel = &gMapRowPointers[z][x];
      AdjustFaceElevation(tileModel, 0, amount);
      UpdateMapTileLighting(tileModel);
      gTerrainPtr[z][x].s.elevation = -gMapRowPointers[z][x].vertices[0].vy >> 4;

      for (i = 1; i < 5; i++) {
         tileModel->gfx[i] = GFX_MAP_TEXTURE_1; // Flowing sand
      }
      UpdateTileElevation(z, x);
   }
}

void Map14_RaiseSandTile(s32 x, s32 z, s32 amount) {
   Object *obj;

   if ((x >= 0 && x < 18) && (z >= 0 && z < 18)) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP14_RAISE_SAND_TILE;
      obj->x1.s.hi = x;
      obj->z1.s.hi = z;
      obj->mem = amount;
      obj->state2 = -2;
   }
}

void Map14_LowerSandTile(s32 x, s32 z, s32 amount) {
   Object *obj;

   if ((x >= 0 && x < 18) && (z >= 0 && z < 18)) {
      obj = Obj_GetUnused();
      obj->functionIndex = OBJF_MAP14_LOWER_SAND_TILE;
      obj->x1.s.hi = x;
      obj->z1.s.hi = z;
      obj->mem = amount;
      obj->state2 = 2;
   }
}

#undef OBJF
#define OBJF 668
void Objf668_Map14_RaiseSandMound(Object *obj) {
   s32 tileZ, tileX;
   s32 i;
   s32 count, offset;
   s32 amount;
   s32 ix, iz;

   tileZ = obj->z1.s.hi;
   tileX = obj->x1.s.hi;

   switch (obj->state) {
   case 0:
      Map14_RaiseSandTile(tileX, tileZ, obj->mem * -16);
      obj->mem--;
      OBJ.count = 1;
      OBJ.offset = 1;
      obj->state++;
      obj->state3 = 0;
      break;

   case 1:
      if (++obj->state3 >= 8) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 2:
      tileZ = obj->z1.s.hi;
      tileX = obj->x1.s.hi;
      amount = obj->mem;
      if (amount <= 0) {
         obj->state += 2;
         return;
      }

      count = OBJ.count;
      offset = OBJ.offset;

      // 1
      iz = tileZ;
      ix = tileX - offset;

      for (i = 0; i < count; i++) {
         Map14_RaiseSandTile(ix, iz, amount * -16);
         iz++;
         ix++;
      }

      // 2
      iz = tileZ + offset;
      ix = tileX;

      for (i = 0; i < count; i++) {
         Map14_RaiseSandTile(ix, iz, amount * -16);
         iz--;
         ix++;
      }

      // 3
      iz = tileZ;
      ix = tileX + offset;

      for (i = 0; i < count; i++) {
         Map14_RaiseSandTile(ix, iz, amount * -16);
         iz--;
         ix--;
      }

      // 4
      iz = tileZ - offset;
      ix = tileX;

      for (i = 0; i < count; i++) {
         Map14_RaiseSandTile(ix, iz, amount * -16);
         iz++;
         ix--;
      }

      obj->state++;
      break;

   case 3:
      if (++obj->state3 >= 8) {
         OBJ.count++;
         OBJ.offset++;
         obj->mem--;
         obj->state = 2;
         obj->state3 = 0;
      }
      break;

   case 4:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void Map14_SetupSandMound(s32 x, s32 z, s32 size) {
   s32 i;
   s32 count, offset;
   s32 ix, iz;

   count = 1;
   offset = 1;

   Map14_AdjustSandTile(x, z, size * -16);

   while (--size > 0) {
      // 1
      iz = z;
      ix = x - offset;

      for (i = 0; i < count; i++) {
         Map14_AdjustSandTile(ix, iz, size * -16);
         iz++;
         ix++;
      }

      // 2
      iz = z + offset;
      ix = x;

      for (i = 0; i < count; i++) {
         Map14_AdjustSandTile(ix, iz, size * -16);
         iz--;
         ix++;
      }

      // 3
      iz = z;
      ix = x + offset;

      for (i = 0; i < count; i++) {
         Map14_AdjustSandTile(ix, iz, size * -16);
         iz--;
         ix--;
      }

      // 4
      iz = z - offset;
      ix = x;

      for (i = 0; i < count; i++) {
         Map14_AdjustSandTile(ix, iz, size * -16);
         iz++;
         ix--;
      }

      count++;
      offset++;
   }
}

#undef OBJF
#define OBJF 666
void Objf666_Map14_LowerSandMound(Object *obj) {
   s32 tileZ, tileX;
   s32 i;
   s32 count, offset;
   s32 amount;
   s32 ix, iz;

   tileZ = obj->z1.s.hi;
   tileX = obj->x1.s.hi;
   amount = obj->mem;

   switch (obj->state) {
   case 0:
      Map14_LowerSandTile(tileX, tileZ, amount * 16);
      obj->mem--;
      OBJ.count = 1;
      OBJ.offset = 1;
      obj->state++;
      obj->state3 = 0;
      break;

   case 1:
      if (++obj->state3 >= 8) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 2:
      tileZ = obj->z1.s.hi;
      tileX = obj->x1.s.hi;
      amount = obj->mem;
      if (amount <= 0) {
         obj->state += 2;
         return;
      }

      count = OBJ.count;
      offset = OBJ.offset;

      // 1
      iz = tileZ;
      ix = tileX - offset;

      for (i = 0; i < count; i++) {
         Map14_LowerSandTile(ix, iz, amount * 16);
         iz++;
         ix++;
      }

      // 2
      iz = tileZ + offset;
      ix = tileX;

      for (i = 0; i < count; i++) {
         Map14_LowerSandTile(ix, iz, amount * 16);
         iz--;
         ix++;
      }

      // 3
      iz = tileZ;
      ix = tileX + offset;

      for (i = 0; i < count; i++) {
         Map14_LowerSandTile(ix, iz, amount * 16);
         iz--;
         ix--;
      }

      // 4
      iz = tileZ - offset;
      ix = tileX;

      for (i = 0; i < count; i++) {
         Map14_LowerSandTile(ix, iz, amount * 16);
         iz++;
         ix--;
      }

      obj->state++;
      break;

   case 3:
      if (++obj->state3 >= 8) {
         OBJ.count++;
         OBJ.offset++;
         obj->mem--;
         obj->state = 2;
         obj->state3 = 0;
      }
      break;

   case 4:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

void AssignFromMainCamera(Camera *camera) {
   camera->rotX = gCameraRotation.vx;
   camera->rotY = gCameraRotation.vy;
   camera->rotZ = gCameraRotation.vz;
   camera->zoom = gCameraZoom.vz;
   camera->posX = gCameraPos.vx;
   camera->posY = gCameraPos.vy;
   camera->posZ = gCameraPos.vz;
}

void AssignToMainCamera(Camera *camera) {
   gCameraRotation.vx = camera->rotX;
   gCameraRotation.vy = camera->rotY;
   gCameraRotation.vz = camera->rotZ;
   gCameraZoom.vz = camera->zoom;
   gCameraPos.vx = camera->posX;
   gCameraPos.vy = camera->posY;
   gCameraPos.vz = camera->posZ;
}

void EaseOutShort(s16 *p, s16 target, s16 smoothness, s16 minStep) {
   s16 delta;
   s16 step;

   delta = target - *p;
   step = delta >> smoothness;

   if (abs(step) > minStep) {
      *p += step;
   } else if (abs(delta) > minStep) {
      *p += (step < 0) ? -minStep : +minStep;
   } else {
      *p += delta;
   }
}

void EaseOutInt(s32 *p, s32 target, s16 smoothness, s32 minStep) {
   s32 delta;
   s32 step;

   delta = target - *p;
   step = delta >> smoothness;

   if (abs(step) > minStep) {
      *p += step;
   } else if (abs(delta) > minStep) {
      *p += (step < 0) ? -minStep : +minStep;
   } else {
      *p += delta;
   }
}

void EaseOutCamera(Camera *camera, s16 smoothness) {
   EaseOutShort(&gCameraRotation.vx, camera->rotX, smoothness, 2);
   EaseOutShort(&gCameraRotation.vy, camera->rotY, smoothness, 2);
   EaseOutShort(&gCameraRotation.vz, camera->rotZ, smoothness, 2);
   EaseOutInt(&gCameraZoom.vz, camera->zoom, smoothness, 2);
   EaseOutShort(&gCameraPos.vx, camera->posX, smoothness, 2);
   EaseOutShort(&gCameraPos.vy, camera->posY, smoothness, 2);
   EaseOutShort(&gCameraPos.vz, camera->posZ, smoothness, 2);
}

#undef OBJF
#define OBJF 670
void Objf670_Map14_Sand(Object *obj) {
   Object *mound;

   switch (obj->state) {
   case 0:
      obj->state2 = 0;
      if (gState.mapState.s.field_0x0 == 0) {
         Map14_SetupSandMound(6, 8, 6);
         obj->z1.s.hi = 8;
         obj->x1.s.hi = 6;
         obj->state = 3;
      } else {
         obj->z1.s.hi = gState.mapState.s.field_0x1;
         obj->x1.s.hi = gState.mapState.s.field_0x2;
         Map14_SetupSandMound(obj->x1.s.hi, obj->z1.s.hi, 6);
         obj->state = 3;
      }
      break;

   case 1:

      switch (obj->state2) {
      case 0:
         if (gState.mapState.s.field_0x0 == 1) {
            obj->z1.s.hi = gState.mapState.s.field_0x1;
            obj->x1.s.hi = gState.mapState.s.field_0x2;
            obj->state2++;
         }
         break;

      case 1:
         obj->state2++;

      // fallthrough
      case 2:
         obj->state2++;

      // fallthrough
      case 3:
         mound = Obj_GetUnused();
         mound->functionIndex = OBJF_MAP14_RAISE_SAND_MOUND;
         mound->z1.s.hi = obj->z1.s.hi;
         mound->x1.s.hi = obj->x1.s.hi;
         mound->mem = 6; // size
         OBJ.mound = mound;
         obj->state2++;

      // fallthrough
      case 4:
         mound = OBJ.mound;
         if (mound->functionIndex != OBJF_MAP14_RAISE_SAND_MOUND) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 5:
         obj->state2++;

      // fallthrough
      case 6:
         gState.field_0x96 = 0;
         obj->state2 = 0;
         obj->state++;
         break;
      } // switch (obj->state2) (via state:1)

      break;

   case 2:
      obj->state++;
      break;

   case 3:

      switch (obj->state2) {
      case 0:
         if (gState.mapState.s.field_0x0 == 2) {
            obj->state2++;
         }
         break;

      case 1:
         obj->state3 = 32;
         obj->state2++;
         obj->state3 = 0;

      // fallthrough
      case 2:
         obj->state2++;

      // fallthrough
      case 3:
         mound = Obj_GetUnused();
         mound->functionIndex = OBJF_MAP14_LOWER_SAND_MOUND;
         mound->z1.s.hi = obj->z1.s.hi;
         mound->x1.s.hi = obj->x1.s.hi;
         mound->mem = 6;
         OBJ.mound = mound;
         obj->state2++;

      // fallthrough
      case 4:
         mound = OBJ.mound;
         if (mound->functionIndex != OBJF_MAP14_LOWER_SAND_MOUND) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 5:
         obj->state2++;

      // fallthrough
      case 6:
         if (--obj->state3 <= 0) {
            gState.field_0x96 = 0;
            obj->state2 = 0;
            obj->state++;
         }
         break;
      } // switch (obj->state2) (via state:3)

      break;

   case 4:
      obj->state = 1;
      break;
   }
}

#undef OBJF
#define OBJF 708
void Objf708_709_Map14_Unused(Object *obj) {
   Object *sand;

   switch (obj->functionIndex) {
   case OBJF_MAP14_UNUSED_708:
      gState.mapState.s.field_0x0 = 2;
      break;
   case OBJF_MAP14_UNUSED_709:
      sand = Obj_GetUnused();
      sand->functionIndex = OBJF_MAP14_SAND;
      gState.mapState.s.field_0x0 = 1;
      gState.mapState.s.field_0x1 = 6;
      gState.mapState.s.field_0x2 = 8;
      break;
   }

   obj->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 752
void Objf752_Map14_Scn15_SandMoundSpawner(Object *obj) {
   Object *entitySprite;
   Object *mound;

   entitySprite = obj->d.entitySpawn.entitySpriteParam;
   mound = Obj_GetUnused();
   mound->functionIndex = OBJF_MAP14_RAISE_SAND_MOUND;
   mound->z1.s.hi = entitySprite->z1.s.hi;
   mound->x1.s.hi = entitySprite->x1.s.hi;
   mound->mem = 6;
   obj->functionIndex = OBJF_NULL;
}
