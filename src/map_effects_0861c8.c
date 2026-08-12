#include "common.h"
#include "object.h"
#include "graphics.h"
#include "field.h"
#include "state.h"
#include "units.h"
#include "battle.h"
#include "audio.h"

//...
void SnapCameraLookAt(Object *obj);
//...

// The tile models so far are just boxes; that should be ok to embed, but anything more complex
// should be kept separate and #include'd instead.
static MapTileModel sMapTileModel_800ffd50 = {
    .vertices =
        {
            [0] = {-32, 0, 16, 0},   [1] = {0, 16, 16, 0},    [2] = {0, 16, -16, 0},
            [3] = {-32, 16, -16, 0}, [4] = {-32, 16, 16, 0},  [5] = {0, 0, 16, 0},
            [6] = {0, 0, -16, 0},    [7] = {-32, 0, -16, 0},  [8] = {-64, 16, -16, 0},
            [9] = {-64, 16, 16, 0},  [10] = {-64, 0, -16, 0}, [11] = {-64, 0, 16, 0},
            [12] = {-96, 0, -16, 0}, [13] = {-96, 0, 16, 0},  [14] = {-96, 16, -16, 0},
            [15] = {-96, 16, 16, 0}, [16] = {0, 0, 0, 0},     [17] = {0, 0, 0, 0},
            [18] = {0, 0, 0, 0},     [19] = {0, 0, 0, 0},     [20] = {0, 0, 0, 0},
            [21] = {0, 0, 0, 0},
        },
    .gfx =
        {
            [0] = 152,
            [1] = 147,
            [2] = 142,
            [3] = 142,
            [4] = 147,
            [5] = 152,
            [6] = 218,
            [7] = 218,
            [8] = 218,
            [9] = 218,
            [10] = 218,
            [11] = 218,
            [12] = 218,
            [13] = 218,
            [14] = 0,
            [15] = 0,
            [16] = 0,
            [17] = 0,
        },
    .faces =
        {
            [0] = {7, 0, 5, 6},
            [1] = {10, 11, 0, 7},
            [2] = {12, 13, 11, 10},
            [3] = {2, 1, 4, 3},
            [4] = {3, 4, 9, 8},
            [5] = {8, 9, 15, 14},
            [6] = {6, 5, 1, 2},
            [7] = {5, 0, 4, 1},
            [8] = {0, 11, 9, 4},
            [9] = {11, 13, 15, 9},
            [10] = {13, 12, 14, 15},
            [11] = {12, 10, 8, 14},
            [12] = {10, 7, 3, 8},
            [13] = {7, 6, 2, 3},
            [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},
            [16] = {0, 0, 0, 0},
            [17] = {0, 0, 0, 0},
        },
    .shades = {0},
    .faceCt = 14,
    .height = 16,
};

static u8 sUnkBytes_800ffe80[264] = {
    0x0e, 0x04, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x01, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x7e, 0x00, 0x7e, 0x00, 0x07, 0x00, 0x0e, 0x05, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00,
    0x02, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x08, 0x00, 0x0e, 0x06, 0x06, 0x00,
    0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x03, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x7e, 0x00,
    0x09, 0x00, 0x0f, 0x04, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x04, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x0f, 0x05, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07,
    0x0a, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00, 0x0f, 0x06,
    0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x06, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x06, 0x00, 0x10, 0x04, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x04, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x04, 0x00, 0x10, 0x05, 0x06, 0x00, 0x04, 0x05,
    0x06, 0x07, 0x0a, 0x00, 0x05, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00, 0x05, 0x00,
    0x10, 0x06, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x06, 0x00, 0x7e, 0x00, 0x00, 0x00,
    0x00, 0x00, 0x00, 0x00, 0x06, 0x00, 0x11, 0x04, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00,
    0x07, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x01, 0x00, 0x11, 0x05, 0x06, 0x00,
    0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x08, 0x00, 0x00, 0x00, 0x7e, 0x00, 0x00, 0x00, 0x00, 0x00,
    0x02, 0x00, 0x11, 0x06, 0x06, 0x00, 0x04, 0x05, 0x06, 0x07, 0x0a, 0x00, 0x09, 0x00, 0x7e, 0x00,
    0x7e, 0x00, 0x00, 0x00, 0x00, 0x00, 0x03, 0x00};

static MapTileModel sMapTileModel_800fff88 = {
    .vertices =
        {
            [0] = {-16, -16, 16, 0}, [1] = {-16, 0, -16, 0},   [2] = {16, 0, 16, 0},
            [3] = {16, 0, -16, 0},   [4] = {-16, -16, -16, 0}, [5] = {-16, 0, 16, 0},
            [6] = {16, -16, 16, 0},  [7] = {16, -16, -16, 0},  [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},      [10] = {0, 0, 0, 0},      [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},     [13] = {0, 0, 0, 0},      [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},     [16] = {0, -64, 48, 0},   [17] = {0, -64, 48, 0},
            [18] = {-8, -64, 32, 0}, [19] = {8, -64, 32, 0},   [20] = {0, -64, 48, 0},
            [21] = {0, -112, 48, 0},
        },
    .gfx =
        {
            [0] = GFX_MAP_TEXTURE_166,
            [1] = GFX_MAP_TEXTURE_166,
            [2] = GFX_MAP_TEXTURE_167,
            [3] = GFX_MAP_TEXTURE_167,
            [4] = GFX_MAP_TEXTURE_167,
            [5] = GFX_MAP_TEXTURE_167,
            [6] = 0,
            [7] = 0,
            [8] = 0,
            [9] = 0,
            [10] = 0,
            [11] = 0,
            [12] = 0,
            [13] = 0,
            [14] = 0,
            [15] = 0,
            [16] = 0,
            [17] = 0,
        },
    .faces =
        {
            [0] = {0, 6, 7, 4},
            [1] = {3, 2, 5, 1},
            [2] = {6, 0, 5, 2},
            [3] = {7, 6, 2, 3},
            [4] = {4, 7, 3, 1},
            [5] = {0, 4, 1, 5},
            [6] = {0, 0, 0, 0},
            [7] = {0, 0, 0, 0},
            [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},
            [10] = {0, 0, 0, 0},
            [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},
            [13] = {0, 0, 0, 0},
            [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},
            [16] = {0, 0, 0, 0},
            [17] = {0, 0, 0, 0},
        },
    .shades = {0},
    .faceCt = 6,
    .height = 0,
};

static MapTileModel sMapTileModel_801000b8 = {
    .vertices =
        {
            [0] = {-16, -32, 16, 0}, [1] = {-16, 0, -16, 0},   [2] = {16, 0, 16, 0},
            [3] = {16, 0, -16, 0},   [4] = {-16, -32, -16, 0}, [5] = {-16, 0, 16, 0},
            [6] = {16, -32, 16, 0},  [7] = {16, -32, -16, 0},  [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},      [10] = {0, 0, 0, 0},      [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},     [13] = {0, 0, 0, 0},      [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},     [16] = {0, 0, 0, 0},      [17] = {0, 0, 0, 0},
            [18] = {-8, -64, 32, 0}, [19] = {8, -64, 32, 0},   [20] = {0, -64, 48, 0},
            [21] = {0, -112, 48, 0},
        },
    .gfx =
        {
            [0] = GFX_MAP_TEXTURE_188,
            [1] = GFX_MAP_TEXTURE_188,
            [2] = GFX_MAP_TEXTURE_167,
            [3] = GFX_MAP_TEXTURE_167,
            [4] = GFX_MAP_TEXTURE_167,
            [5] = GFX_MAP_TEXTURE_184,
            [6] = 0,
            [7] = 0,
            [8] = 0,
            [9] = 0,
            [10] = 0,
            [11] = 0,
            [12] = 0,
            [13] = 0,
            [14] = 0,
            [15] = 0,
            [16] = 0,
            [17] = 0,
        },
    .faces =
        {
            [0] = {0, 6, 7, 4},
            [1] = {3, 2, 5, 1},
            [2] = {7, 6, 2, 3},
            [3] = {4, 7, 3, 1},
            [4] = {0, 4, 1, 5},
            [5] = {6, 0, 5, 2},
            [6] = {0, 0, 0, 0},
            [7] = {0, 0, 0, 0},
            [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},
            [10] = {0, 0, 0, 0},
            [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},
            [13] = {0, 0, 0, 0},
            [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},
            [16] = {0, 0, 0, 0},
            [17] = {0, 0, 0, 0},
        },
    .shades = {0},
    .faceCt = 6,
    .height = 0,
};

static MapTileModel sMapTileModel_801001e8 = {
    .vertices =
        {
            [0] = {16, -32, -16, 0},  [1] = {16, 0, 16, 0},    [2] = {-16, 0, -16, 0},
            [3] = {-16, 0, 16, 0},    [4] = {16, -32, 16, 0},  [5] = {16, 0, -16, 0},
            [6] = {-16, -32, -16, 0}, [7] = {-16, -32, 16, 0}, [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},       [10] = {0, 0, 0, 0},     [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},      [13] = {0, 0, 0, 0},     [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},      [16] = {0, 0, 0, 0},     [17] = {0, 0, 0, 0},
            [18] = {-8, -64, 32, 0},  [19] = {8, -64, 32, 0},  [20] = {0, -64, 48, 0},
            [21] = {0, -112, 48, 0},
        },
    .gfx =
        {
            [0] = GFX_MAP_TEXTURE_188,
            [1] = GFX_MAP_TEXTURE_188,
            [2] = GFX_MAP_TEXTURE_167,
            [3] = GFX_MAP_TEXTURE_167,
            [4] = GFX_MAP_TEXTURE_167,
            [5] = GFX_MAP_TEXTURE_184,
            [6] = 0,
            [7] = 0,
            [8] = 0,
            [9] = 0,
            [10] = 0,
            [11] = 0,
            [12] = 0,
            [13] = 0,
            [14] = 0,
            [15] = 0,
            [16] = 0,
            [17] = 0,
        },
    .faces =
        {
            [0] = {0, 6, 7, 4},
            [1] = {3, 2, 5, 1},
            [2] = {7, 6, 2, 3},
            [3] = {4, 7, 3, 1},
            [4] = {0, 4, 1, 5},
            [5] = {6, 0, 5, 2},
            [6] = {0, 0, 0, 0},
            [7] = {0, 0, 0, 0},
            [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},
            [10] = {0, 0, 0, 0},
            [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},
            [13] = {0, 0, 0, 0},
            [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},
            [16] = {0, 0, 0, 0},
            [17] = {0, 0, 0, 0},
        },
    .shades = {0},
    .faceCt = 6,
    .height = 0,
};

void Map13_RemoveBridgeSection(s32 x) {
   typedef struct DamagedBridgeTileInfo {
      u8 z;
      u8 faceIdx;
      u8 elevation;
   } DamagedBridgeTileInfo;

   static DamagedBridgeTileInfo tileInfo[8] = {{2, 0, 4}, {2, 1, 2}, {5, 0, 4}, {5, 1, 2},
                                               {3, 0, 3}, {3, 1, 2}, {4, 0, 3}, {4, 1, 2}};

   s32 i;
   DamagedBridgeTileInfo *p;
   MapTileModel *pTileModel;

   p = &tileInfo[0];

   for (i = 0; i < 8; i++) {
      pTileModel = &gMapRowPointers[p->z][x];
      AdjustFaceElevation(pTileModel, p->faceIdx, p->elevation * 16);
      pTileModel->gfx[0] = GFX_MAP_TEXTURE_52; // Water texture
      p++;
   }

   if (x > 4) {
      gMapRowPointers[2][x - 1].gfx[4] = GFX_MAP_TEXTURE_167;
      gMapRowPointers[3][x - 1].gfx[5] = GFX_MAP_TEXTURE_213;
      gMapRowPointers[4][x - 1].gfx[5] = GFX_MAP_TEXTURE_213;
      gMapRowPointers[5][x - 1].gfx[4] = GFX_MAP_TEXTURE_167;
   }

   for (i = 2; i <= 5; i++) {
      gTerrainPtr[i][x].s.elevation = -gMapRowPointers[i][x].vertices[0].vy >> 4;
      gTerrainPtr[i][x].s.terrain = TERRAIN_WATER;
   }
}

void Map13_ExplodeBridgeTile(s32 z, s32 x, s32 idx) {
   extern MapTileModel s_tileModels_80123e54[4]; //?

   static MapTileModel *baseTileModels[4] = {&sMapTileModel_800fff88, &sMapTileModel_801000b8,
                                             &sMapTileModel_801000b8, &sMapTileModel_800fff88};

   Object *explodingTile;

   idx &= 3;
   s_tileModels_80123e54[idx] = *baseTileModels[idx];

   explodingTile = Obj_GetUnused();
   explodingTile->functionIndex = OBJF_EXPLODING_TILE;
   // Unused? It looks like Objf653_ExplodingTile maintains its own working copy.
   explodingTile->d.objf653.tileModel_unused = &s_tileModels_80123e54[idx];
   explodingTile->d.objf653.tileModel = baseTileModels[idx];
   explodingTile->d.objf653.riverVelocity = -16;
   explodingTile->d.objf653.translation.vx = x * CV(1.0) + CV(0.5);
   explodingTile->d.objf653.translation.vy = CV(2.0);
   explodingTile->d.objf653.translation.vz = z * CV(1.0) + CV(0.5);
   explodingTile->d.objf653.translation.pad = 0;
}

#undef OBJF
#define OBJF 361
void Objf361_Map13_BridgeExplosion_Scene(Object *obj) {
   s32 i;
   s32 end;
   Object *explosion;
   s8 unused[8];

   switch (obj->state) {
   case 0:
      OBJ.currentX = 21;
      obj->x1.n = CV(21.5);
      obj->z1.n = CV(4.5);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->mem = 0;

      if (gState.mapState.s.field_0x0 != 0) {
         end = gState.mapState.s.field_0x0;
         for (i = 0; i < end; i++) {
            Map13_RemoveBridgeSection(OBJ.currentX--);
            obj->mem++;
            obj->x1.n -= CV(1.0);
         }
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 > obj->mem) {
         obj->state3 = 0;
         obj->state++;
      }
      break;

   case 2:
      obj->state++;
      break;

   case 3:
      obj->state++;

   // fallthrough
   case 4:
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 5:
      obj->state++;
      obj->z1.n = CV(0.5);

      for (i = 0; i < 4; i++) {
         if ((rand() & 0xfff) < 0x800) {
            Map13_ExplodeBridgeTile(i + 2, OBJ.currentX, i);
         }
      }

      for (i = 0; i < 4; i++) {
         if (gTerrainPtr[i + 2][OBJ.currentX].raw != 0) {
            gTileStateGridPtr[i + 2][OBJ.currentX].action = TA_22;
         }
      }

      for (i = 0; i < 4; i++) {
         explosion = Obj_GetUnused();
         explosion->functionIndex = OBJF_MAP13_EXPLOSION_PILLAR;
         explosion->x1.n = OBJ.currentX * CV(1.0) + CV(0.5);
         explosion->y1.n = CV(2.0);
         explosion->z1.n = (i + 2) * CV(1.0) + CV(0.5);
      }

      Map13_RemoveBridgeSection(OBJ.currentX);

   // fallthrough
   case 6:
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 7:
      if (--obj->state3 <= 0) {
         obj->state++;
      }
      break;

   case 8:
      OBJ.currentX--;
      obj->mem++;
      obj->x1.n -= CV(1.0);
      obj->state3 = 256;
      gState.field_0x96 = 0;
      obj->state = 1;
      break;
   }
}

#undef OBJF
#define OBJF 288
void Objf288_Map13_BridgeExplosion_Battle(Object *obj) {
   s32 i;
   s32 end;
   s32 numHit;
   Object *explosion;
   s8 unused[8];

   switch (obj->state) {
   case 0:
      OBJ.currentX = 21;
      obj->x1.n = CV(21.5);
      obj->z1.n = CV(4.5);
      obj->y1.n = GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi);
      obj->mem = 0;

      if (gState.mapState.s.field_0x0 != 0) {
         end = gState.mapState.s.field_0x0;

         for (i = 0; i < end; i++) {
            Map13_RemoveBridgeSection(OBJ.currentX--);
            obj->mem++;
            obj->x1.n -= CV(1.0);

            if (OBJ.currentX < 4) {
               break;
            }
         }
      }
      obj->state++;

   // fallthrough
   case 1:
      if (gState.mapState.s.field_0x0 > obj->mem) {
         if (OBJ.currentX < 4) {
            gState.field_0x96 = 0;
            break;
         }

         obj->state3 = 64;
         obj->state++;
         numHit = 0;
         OBJ.hitPlayerUnit = 0;

         for (i = 0; i < 4; i++) {
            if (gMapUnitsPtr[i + 2][OBJ.currentX].s.team == TEAM_PLAYER) {
               numHit++;
            }
         }

         if (numHit != 0) {
            OBJ.hitPlayerUnit = 1;
         }

         if (OBJ.hitPlayerUnit) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(18));
         } else {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(19));
         }
      }

      break;

   case 2:
      OBJ.dstRotY_unused = GetBestViewOfTarget(obj->z1.s.hi, obj->x1.s.hi, 1);
      gCameraRotation.vy &= 0xfff;
      AssignFromMainCamera(&OBJ.camera);
      gPlayerControlSuppressed = 1;
      obj->state3 = 48;
      obj->state++;

   // fallthrough
   case 3:
      gCameraRotation.vy += (OBJ.camera.rotX - gCameraRotation.vy) >> 2;
      gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
      gCameraZoom.vz += (768 - gCameraZoom.vz) >> 2;
      PanCamera(obj->x1.n, obj->y1.n, obj->z1.n, 2);

      if (OBJ.hitPlayerUnit && obj->state3 == 16) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(18));
      }
      if (!OBJ.hitPlayerUnit && obj->state3 == 16) {
         PerformAudioCommand(AUDIO_CMD_PLAY_XA(19));
      }

      if (--obj->state3 <= 0) {
         PositionCamera(obj->x1.n, obj->y1.n, obj->z1.n);
         gCameraRotation.vx = CV(1.5);
         gCameraRotation.vy = OBJ.camera.rotX;
         gCameraZoom.vz = 768;

         if (OBJ.hitPlayerUnit) {
            obj->state++;

            for (i = 0; i < 4; i++) {
               explosion = Obj_GetUnused();
               explosion->functionIndex = OBJF_MAP13_EXPLOSION_PILLAR;
               explosion->x1.n = OBJ.currentX * CV(1.0) + CV(0.5);
               explosion->y1.n = CV(2.0);
               explosion->z1.n = (i + 2) * CV(1.0) + CV(0.5);

               explosion = Obj_GetUnused();
               explosion->functionIndex = OBJF_MAP13_EXPLOSION_PILLAR;
               explosion->x1.n = OBJ.currentX * CV(1.0) + CV(0.5);
               explosion->y1.n = CV(2.0);
               explosion->z1.n = (i + 2) * CV(1.0) + CV(0.5);
            }
            obj->state3 = 32;
         } else {
            for (i = 0; i < 4; i++) {
               Map13_ExplodeBridgeTile(i + 2, OBJ.currentX, i);
               if (gMapUnitsPtr[i + 2][OBJ.currentX].s.unitIdx != UNIT_NULL) {
                  gTileStateGridPtr[i + 2][OBJ.currentX].action = TA_22;
               }
            }
            Map13_RemoveBridgeSection(OBJ.currentX);
            obj->state += 3;
            obj->state3 = 96;
         }
      }

      break;

   case 4:
      if (--obj->state3 <= 0) {
         obj->state2 = 0;
         obj->state++;
      }
      break;

   case 5:

      switch (obj->state2) {
      case 0:
         OBJ.iterator = 0;
         obj->state2++;

      // fallthrough
      case 1:
         i = OBJ.iterator;
         if (gMapUnitsPtr[i + 2][OBJ.currentX].raw != 0) {
            gState.msgFinished = 0;
            gTileStateGridPtr[i + 2][OBJ.currentX].action = TA_32;
            obj->state2 += 2;
         } else {
            obj->state2++;
         }
         break;

      case 2:
         if (++OBJ.iterator >= 4) {
            PerformAudioCommand(AUDIO_CMD_PREPARE_XA(19));
            obj->state2 += 3;
            obj->state3 = 0;
         } else {
            obj->state2 = 1;
         }
         break;

      case 3:
         if (gState.msgFinished) {
            obj->state2++;
            if (++OBJ.iterator >= 4) {
               PerformAudioCommand(AUDIO_CMD_PREPARE_XA(19));
               obj->state2 = 5;
               obj->state3 = 0;
            }
         }
         break;

      case 4:
         obj->state2 = 1;
         break;

      case 5:
         if (++obj->state3 >= 45) {
            obj->state2++;
         }
         if (obj->state3 == 40) {
            PerformAudioCommand(AUDIO_CMD_PLAY_XA(19));
         }
         break;

      case 6:
         for (i = 0; i < 4; i++) {
            if (gMapUnitsPtr[i + 2][OBJ.currentX].raw != 0) {
               gTileStateGridPtr[i + 2][OBJ.currentX].action = TA_22;
            }
            Map13_ExplodeBridgeTile(i + 2, OBJ.currentX, i);
         }
         Map13_RemoveBridgeSection(OBJ.currentX);
         obj->state++;
         obj->state3 = 96;
         break;
      } // switch (obj->state2) (via state:5)

      break;

   case 6:
      if (--obj->state3 <= 0) {
         obj->state3 = 48;
         obj->state++;
      }
      break;

   case 7:
      EaseOutCamera(&OBJ.camera, 3);

      if (--obj->state3 <= 0) {
         AssignToMainCamera(&OBJ.camera);
         obj->state++;
      }
      break;

   case 8:
      OBJ.currentX--;
      obj->mem++;
      obj->x1.n -= CV(1.0);
      obj->state3 = 256;
      gState.field_0x96 = 0;
      obj->state = 1;
      break;
   }
}

#undef OBJF
#define OBJF 085
void Objf085_Map13_ExplosionPillar(Object *obj) {
   static s16 explosionAnimData[26] = {0, GFX_EXPLOSION_1,  2, GFX_EXPLOSION_2,  2, GFX_EXPLOSION_3,
                                       2, GFX_EXPLOSION_4,  2, GFX_EXPLOSION_5,  2, GFX_EXPLOSION_6,
                                       2, GFX_EXPLOSION_7,  2, GFX_EXPLOSION_8,  2, GFX_EXPLOSION_9,
                                       2, GFX_EXPLOSION_10, 2, GFX_EXPLOSION_11, 2, GFX_NULL,
                                       0, GFX_NULL};

   s32 i;
   Object *explosion;

   switch (obj->state) {
   case 0:
      for (i = 0; i < 5; i++) {
         explosion = Obj_GetUnused();
         explosion->functionIndex = OBJF_EXPLOSION;
         explosion->x1.n = obj->x1.n;
         explosion->z1.n = obj->z1.n;
         explosion->y1.n = obj->y1.n;
         explosion->y2.n = i * CV(0.375);
         explosion->y3.n = i * CV(-0.03125);
         explosion->d.sprite.animData = explosionAnimData;
         explosion->d.sprite.boxIdx = 6;
         explosion->state = 1;
      }
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 297
void Objf297_SplashDroplets(Object *obj) {
   switch (obj->state) {
   case 0:
      obj->d.sprite.gfxIdx = GFX_DOTS;
      obj->d.sprite.boxIdx = 4;
      obj->state2 = 32;
      obj->state++;

   // fallthrough
   case 1:
      obj->y1.n += obj->y2.n;
      obj->y2.n += obj->y3.n;
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (--obj->state2 <= 0) {
         obj->functionIndex = OBJF_NULL;
      }
      if (obj->y1.n < GetTerrainElevation(obj->z1.s.hi, obj->x1.s.hi)) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 298
void Objf298_SplashWithDroplets(Object *obj) {
   static s16 splashAnimData[20] = {
       0, GFX_SPLASH_1, 2, GFX_SPLASH_2, 2, GFX_SPLASH_3, 2, GFX_SPLASH_4, 2, GFX_SPLASH_5,
       2, GFX_SPLASH_6, 2, GFX_SPLASH_7, 2, GFX_SPLASH_8, 2, GFX_NULL,     0, GFX_NULL};

   s32 i;
   Object *droplets;

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = splashAnimData;
      obj->d.sprite.boxIdx = 4;
      obj->y3.n = CV(-0.015625);

      for (i = 0; i < 8; i++) {
         droplets = Obj_GetUnused();
         droplets->functionIndex = OBJF_SPLASH_DROPLETS;
         droplets->x1.n = obj->x1.n;
         droplets->z1.n = obj->z1.n;
         droplets->y1.n = obj->y1.n;
         droplets->y2.n = (obj->y2.n / 16) * (i + 1);
         droplets->y3.n = CV(-0.03125);
      }

      obj->d.sprite.semiTrans = 2;
      obj->state++;

   // fallthrough
   case 1:
      obj->y2.n += obj->y3.n;
      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

void Map15_RaisePlank(s32 x, s32 z) {
   s32 i;
   MapTileModel *pTileModel;

   z = 11;

   for (i = 0; i < 4; i++) {
      pTileModel = &gMapRowPointers[z - i][x];
      pTileModel->gfx[0] = GFX_MAP_TEXTURE_14; // Plank texture
      AdjustFaceElevation(pTileModel, 0, -48);
      AdjustFaceElevation(pTileModel, 1, -32);
      gTerrainPtr[z - i][x].s.elevation = -gMapRowPointers[z - i][x].vertices[0].vy >> 4;
      gTerrainPtr[z - i][x].s.terrain = TERRAIN_PLAINS;
   }
}

void Map15_HideWaterTiles(void) {
   s16 *pSavedGfx;
   MapTileModel *pTileModel;
   s32 iz, ix, iGfx;

   pSavedGfx = (s16 *)gScratch1_801317c0;

   for (ix = 0; ix < 21; ix++) {
      for (iz = 0; iz < 21; iz++) {
         pTileModel = &gMapRowPointers[iz][ix];
         for (iGfx = 0; iGfx < ARRAY_COUNT(pTileModel->gfx); iGfx++) {
            *pSavedGfx++ = pTileModel->gfx[iGfx];
            if (pTileModel->gfx[iGfx] == GFX_MAP_TEXTURE_6) {
               pTileModel->gfx[iGfx] = GFX_NULL;
            }
         }
      }
   }
}

void Map15_RestoreWaterTiles(void) {
   s16 *pSavedGfx;
   MapTileModel *pTileModel;
   s32 iz, ix, iGfx;

   pSavedGfx = (s16 *)gScratch1_801317c0;

   for (ix = 0; ix < 21; ix++) {
      for (iz = 0; iz < 21; iz++) {
         pTileModel = &gMapRowPointers[iz][ix];
         for (iGfx = 0; iGfx < ARRAY_COUNT(pTileModel->gfx); iGfx++) {
            pTileModel->gfx[iGfx] = *pSavedGfx++;
         }
      }
   }
}

#undef OBJF
#define OBJF 089
void Objf089_Map15_Scn17_Cinematic(Object *obj) {
   // Spawned by EVDATA17.DAT
   Object *obj_s1;
   Object *obj_a0;
   Object *pObj;
   s32 i, j;
   s16 camPosX, camPosY, camRotY;
   s32 flag;

   switch (obj->state) {
   case 0:
      if (++obj->state3 >= 0) {
         obj->state++;
         gState.fieldRenderingDisabled = 1;

         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_MAP15_OCEAN;
         OBJ.ocean = obj_s1;
         obj->state3 = 0;
         gPlayerControlSuppressed = 1;

         obj_a0 = Obj_GetUnused();
         obj_a0->functionIndex = OBJF_NOOP;
         OBJ.focus = obj_a0;
      }
      break;

   case 1:
      obj->x1.n = CV(60.0);
      obj->z1.n = CV(30.0);
      obj->y1.n = CV(14.0);
      obj->x2.n = obj->x1.n + CV(16.0);
      obj->z2.n = CV(12.0);
      obj->y2.n = CV(2.0);

      AssignFromMainCamera(&OBJ.camera);
      OBJ.todo_x24 = 0xc00;
      obj->state++;
      break;

   case 2:
      // Player's ship
      for (i = gMapMinZ; i < 12; i++) {
         for (j = gMapMinX; j <= gMapMaxX; j++) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[i][j], GRID_COLOR_NONE);
         }
      }

      camPosY = gCameraPos.vy;
      camPosX = gCameraPos.vx;
      camRotY = gCameraRotation.vy;

      PushMatrix();
      RotMatrix(&gCameraRotation, &gCameraMatrix);
      TransMatrix(&gCameraMatrix, &gCameraZoom);
      SetRotMatrix(&gCameraMatrix);
      SetTransMatrix(&gCameraMatrix);
      gCameraPos.vx += OBJ.todo_x24;

      if (OBJ.todo_x24 >= 0x200) {
         OBJ.todo_x24 -= 8;
      } else if (OBJ.todo_x24 >= 0x40) {
         OBJ.todo_x24 -= 4;
      } else if (OBJ.todo_x24 >= 1) {
         OBJ.todo_x24 -= 1;
         OBJ.todo_x24 += -(OBJ.todo_x24) >> 4;
      } else {
         obj->state++;
      }

      RotTrans(&gCameraPos, (VECTOR *)gCameraMatrix.t, &flag);
      SetTransMatrix(&gCameraMatrix);

      // Temporary sprite used to render the stand-ins at their faux positions
      obj_s1 = Obj_GetUnused();
      obj_s1->functionIndex = OBJF_NOOP;

      pObj = &gObjectArray[0];
      for (i = 0; i < OBJ_DATA_CT; i++) {
         if (pObj->functionIndex == OBJF_MAP15_PIRATE_STAND_IN) {
            obj_a0 = pObj->d.entitySpawn.entitySpriteParam;
            obj_a0->d.sprite.hidden = 1;
            CopyObject(obj_a0, obj_s1);
            obj_s1->functionIndex = OBJF_NOOP;
            obj_s1->d.sprite.hidden = 0;
            RenderUnitSprite(gGraphicsPtr->ot, obj_s1, 0);
         }
         pObj++;
      }

      obj_s1->functionIndex = OBJF_NULL;

      // Pirate's ship
      for (i = 12; i <= gMapMaxZ; i++) {
         for (j = gMapMinX; j <= gMapMaxX; j++) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[i][j], GRID_COLOR_NONE);
         }
      }

      PopMatrix();

      gCameraPos.vx = camPosX;
      gCameraRotation.vy = camRotY;

      obj->x3.n = 0x1000 - OBJ.todo_x24;
      obj->x2.n = (OBJ.todo_x24 << 3) + obj->x3.n;
      obj->x1.n -= CV(0.0625);
      obj->z1.n -= CV(0.046875);
      obj->y1.n -= CV(0.015625);

      obj_a0 = OBJ.focus;
      obj_a0->x1.n = obj->x2.n;
      obj_a0->y1.n = obj->y2.n;
      obj_a0->z1.n = obj->z2.n;

      switch (obj->state2) {
      case 0:
         obj->state3 = 0;
         obj_a0->x1.n = obj->x2.n;
         obj_a0->y1.n = obj->y2.n;
         obj_a0->z1.n = obj->z2.n;
         gState.focus = obj_a0;
         obj->state2++;
         break;
      case 1:
         SnapCameraLookAt(obj);
         if (++obj->state3 >= 48) {
            obj->state2++;
         }
         break;
      case 2:
         SnapCameraLookAt(obj);
         break;
      }

      if (--obj->mem <= 0) {
         for (i = 0; i < 1; i++) {
            obj_s1 = Obj_GetUnused();
            obj_s1->functionIndex = OBJF_MAP15_HULL_SPLASH;
            obj_s1->x1.n = (i + 4) * CV(1.0) + (OBJ.todo_x24 << 3);
            obj_s1->z1.n = (i + 15) * CV(1.0);
            obj_s1->y1.n = CV(0.5);
            obj_s1->x2.n = CV(-0.1875);
            obj_s1->x3.n = CV(0.03125);
            obj_s1->z2.n = CV(0.25);
            obj_s1->z3.n = i - 3;
         }
         obj->mem = 4;
      }

      if (obj->state == 3) {
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_NOOP;

         pObj = &gObjectArray[0];
         for (i = 0; i < OBJ_DATA_CT; i++) {
            if (pObj->functionIndex == OBJF_MAP15_PIRATE_STAND_IN) {
               obj_a0 = pObj->d.entitySpawn.entitySpriteParam;
               obj_a0->d.sprite.hidden = 0;
               pObj->functionIndex = OBJF_NULL;
            }
            pObj++;
         }
         obj_s1->functionIndex = OBJF_NULL;
      }

      break;

   case 3:
      for (i = 0; i <= gMapMaxZ; i++) {
         for (j = gMapMinX; j <= gMapMaxX; j++) {
            RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[i][j], GRID_COLOR_NONE);
         }
      }
      obj->state2 = 0;
      obj->state++;
      break;

   case 4:
      if (gState.fieldRenderingDisabled) {
         for (i = 0; i <= gMapMaxZ; i++) {
            for (j = gMapMinX; j <= gMapMaxX; j++) {
               RenderMapTile(gGraphicsPtr->ot, &gMapRowPointers[i][j], GRID_COLOR_NONE);
            }
         }
      }

      switch (obj->state2) {
      case 0:
         obj->state3 = 0;
         obj->state2++;

      // fallthrough
      case 1:
         if (++obj->state3 >= 48) {
            obj->state2++;
         }
         break;

      case 2:
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_MAP15_PLANK;
         // Tile-based coords
         obj_s1->x1.n = 7;
         obj_s1->z1.n = 11;
         OBJ.plank = obj_s1;
         OBJ.dstRotY = GetBestViewOfTarget(11, 7, 1);
         obj->state3 = 0;
         obj->state2++;
         break;

      case 3:
         obj_a0 = OBJ.focus;
         obj_a0->x1.n = CV(7.0);
         obj_a0->y1.n = CV(2.0);
         obj_a0->z1.n = CV(11.0);

         gCameraRotation.vy += (OBJ.dstRotY - gCameraRotation.vy) >> 3;
         gCameraRotation.vx += (DEG(33.75) - gCameraRotation.vx) >> 2;
         gCameraZoom.vz += (384 - gCameraZoom.vz) >> 3;

         if (obj->state3 == 16) {
            gState.fieldRenderingDisabled = 0;
            Map15_RestoreWaterTiles();
         }

         if (obj->state3 == 17) {
            obj_s1 = OBJ.ocean;
            obj_s1->functionIndex = OBJF_NULL;
         }

         if (++obj->state3 >= 48) {
            obj->state2++;
         }

         break;

      case 4:
         obj_s1 = OBJ.plank;
         if (obj_s1->functionIndex != OBJF_MAP15_PLANK) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 5:
         obj_s1 = Obj_GetUnused();
         obj_s1->functionIndex = OBJF_MAP15_PLANK;
         // Tile-based coords
         obj_s1->x1.n = 14;
         obj_s1->z1.n = 11;
         OBJ.plank = obj_s1;
         OBJ.dstRotY += DEG(90);
         obj->state3 = 0;
         obj->state2++;
         break;

      case 6:
         obj_a0 = OBJ.focus;
         obj_a0->x1.n = CV(14.0);
         obj_a0->y1.n = CV(2.0);
         obj_a0->z1.n = CV(11.0);

         gCameraRotation.vy += (OBJ.dstRotY - gCameraRotation.vy) >> 3;
         gCameraZoom.vz += (384 - gCameraZoom.vz) >> 3;
         if (++obj->state3 >= 48) {
            obj->state2++;
         }
         break;

      case 7:
         obj_s1 = OBJ.plank;
         if (obj_s1->functionIndex != OBJF_MAP15_PLANK) {
            obj->state3 = 0;
            obj->state2++;
         }
         break;

      case 8:
         if (++obj->state3 >= 48) {
            obj->state2 = 0;
            obj->state++;
         }
         break;
      } // switch (obj->state2) (via state:4)

      break;

   case 5:
      gState.fieldRenderingDisabled = 0;
      gState.signal = 0;
      obj_a0 = OBJ.focus;
      gState.eventCameraPan.x = obj_a0->x1.n;
      gState.eventCameraPan.y = obj_a0->y1.n;
      gState.eventCameraPan.z = obj_a0->z1.n;
      obj_a0->functionIndex = OBJF_NULL;
      gState.focus = NULL;
      obj->functionIndex = OBJF_NULL;
      break;
   }

   gState.eventCameraRot = gCameraRotation.vy;
}

void Map15_GetChunkBrightness(s32 nearness, s16 sideIdx, CVECTOR *dstColor) {
   s16 r, g, b;
   s16 n;
   s8 unused[8];

   n = nearness - 150;
   if (n < 0) {
      n = 0;
   } else if (n > 256) {
      n = 256;
   }

   r = gSideLightColor[sideIdx].r;
   g = gSideLightColor[sideIdx].g;
   b = gSideLightColor[sideIdx].b;
   r *= n;
   g *= n;
   b *= n;
   dstColor->r = r >> 8;
   dstColor->g = g >> 8;
   dstColor->b = b >> 8;
}

void Map15_RenderOceanChunk(s32 x, s32 y, s32 z, s32 xsize, s32 zsize) {
   Object *chunk;
   CVECTOR color;
   s32 otz;
   s32 sxy, p, flag;
   POLY_FT4 *poly;

   chunk = Obj_GetUnused();
   chunk->functionIndex = OBJF_NOOP;

   chunk->x1.n = x;
   chunk->z1.n = -z;
   chunk->y1.n = y;

   chunk->d.dataStore.svectors[3].vx = x / 8;
   chunk->d.dataStore.svectors[3].vz = z / 8;
   chunk->d.dataStore.svectors[3].vy = -y / 8;
   chunk->d.dataStore.svectors[4].vx = x / 8;
   chunk->d.dataStore.svectors[4].vz = (z + zsize) / 8;
   chunk->d.dataStore.svectors[4].vy = -y / 8;
   chunk->d.dataStore.svectors[5].vx = (x + xsize) / 8;
   chunk->d.dataStore.svectors[5].vz = z / 8;
   chunk->d.dataStore.svectors[5].vy = -y / 8;
   chunk->d.dataStore.svectors[6].vx = (x + xsize) / 8;
   chunk->d.dataStore.svectors[6].vz = (z + zsize) / 8;
   chunk->d.dataStore.svectors[6].vy = -y / 8;

   // Presumably gfxIdx; but since it's unused anyway, this will do
   chunk->d.dataStore.svectors[0].vz = GFX_MAP_TEXTURE_6;

   chunk->vec.vx = chunk->x1.n >> 3;
   chunk->vec.vz = chunk->z1.n >> 3;
   chunk->vec.vy = chunk->y1.n >> 3;

   otz = RotTransPers(&chunk->vec, &sxy, &p, &flag);
   Map15_GetChunkBrightness(OT_SIZE + 6 - otz, 0, &color);

   poly = &gGraphicsPtr->quads[gQuadIndex++];
   SetPolyFT4(poly);
   RotTransPers4(&chunk->d.dataStore.svectors[3], &chunk->d.dataStore.svectors[4],
                 &chunk->d.dataStore.svectors[5], &chunk->d.dataStore.svectors[6], &poly->x0,
                 &poly->x1, &poly->x2, &poly->x3, &p, &flag);
   poly->r0 = color.r;
   poly->g0 = color.g;
   poly->b0 = color.b;
   poly->tpage = GetTPage(0, 0, 576, 256);
   poly->clut = gGfxClutIds[GFX_MAP_TEXTURE_6];
   setUVWH(poly, 0, 0, 255, 255);
   addPrim(&gGraphicsPtr->ot[2], poly);

   chunk->functionIndex = OBJF_NULL;
}

#undef OBJF
#define OBJF 299
void Objf299_Map15_Ocean(Object *obj) {
   extern DR_MODE s_drawModes_80123e24[2][2];

   RECT rect;
   u16 tpage;
   s32 ix, iz;
   s8 unused[8];

   switch (obj->state) {
   case 0:
      Map15_HideWaterTiles();
      obj->state++;

   // fallthrough
   case 1:
      obj->state2++;
      obj->state2 %= 2;

      rect.x = 0;
      rect.y = 0;
      rect.w = 0;
      rect.h = 0;
      tpage = GetTPage(0, 0, 576, 256);
      SetDrawMode(&s_drawModes_80123e24[0][obj->state2], 0, 0, tpage, &rect);
      AddPrim(&gGraphicsPtr->ot[2], &s_drawModes_80123e24[0][obj->state2]);

      for (iz = -1; iz < 4; iz++) {
         for (ix = -1; ix < 15; ix++) {
            Map15_RenderOceanChunk(ix * 0x800, 0x80, iz * 0x800, 0x800, 0x800);
         }
      }

      // Set up repeating texture
      rect.x = 0;
      rect.y = 0;
      rect.w = 32;
      rect.h = 32;
      tpage = GetTPage(0, 0, 576, 256);
      SetDrawMode(&s_drawModes_80123e24[1][obj->state2], 0, 0, tpage, &rect);
      AddPrim(&gGraphicsPtr->ot[2], &s_drawModes_80123e24[1][obj->state2]);

      // Copy the animated water drawn by OBJF_MAP_OBJECT_WATER_2
      rect.x = 332;
      rect.y = 0;
      rect.w = 32 >> 2;
      rect.h = 32;
      MoveImage(&rect, 576, 256);
      break;
   }
}

static MapTileModel sMapTileModel_8010039c = {
    .vertices =
        {
            [0] = {-16, -16, 16, 0}, [1] = {16, 0, 16, 0},     [2] = {16, 0, -16, 0},
            [3] = {-16, 0, -16, 0},  [4] = {-16, 0, 16, 0},    [5] = {16, -16, 16, 0},
            [6] = {16, -16, -16, 0}, [7] = {-16, -16, -16, 0}, [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},      [10] = {0, 0, 0, 0},      [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},     [13] = {0, 0, 0, 0},      [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},     [16] = {0, 0, 0, 0},      [17] = {0, 0, 0, 0},
            [18] = {0, 0, 0, 0},     [19] = {0, 0, 0, 0},      [20] = {0, 0, 0, 0},
            [21] = {0, 0, 0, 0},
        },
    .gfx =
        {
            [0] = GFX_MAP_TEXTURE_14,
            [1] = GFX_MAP_TEXTURE_153,
            [2] = 0,
            [3] = GFX_MAP_TEXTURE_153,
            [4] = 0,
            [5] = GFX_MAP_TEXTURE_14,
            [6] = 0,
            [7] = 0,
            [8] = 0,
            [9] = 0,
            [10] = 0,
            [11] = 0,
            [12] = 0,
            [13] = 0,
            [14] = 0,
            [15] = 0,
            [16] = 0,
            [17] = 0,
        },
    .faces =
        {
            [0] = {7, 0, 5, 6},
            [1] = {6, 5, 1, 2},
            [2] = {7, 6, 2, 3},
            [3] = {0, 7, 3, 4},
            [4] = {5, 0, 4, 1},
            [5] = {4, 3, 2, 1},
            [6] = {0, 0, 0, 0},
            [7] = {0, 0, 0, 0},
            [8] = {0, 0, 0, 0},
            [9] = {0, 0, 0, 0},
            [10] = {0, 0, 0, 0},
            [11] = {0, 0, 0, 0},
            [12] = {0, 0, 0, 0},
            [13] = {0, 0, 0, 0},
            [14] = {0, 0, 0, 0},
            [15] = {0, 0, 0, 0},
            [16] = {0, 0, 0, 0},
            [17] = {0, 0, 0, 0},
        },
    .shades = {0},
    .faceCt = 6,
    .height = 8,
};

#undef OBJF
#define OBJF 364
void Objf364_Map15_Plank(Object *obj) {
   static const s16 textureGfx[4] = {GFX_MAP_TEXTURE_14, GFX_MAP_TEXTURE_153, GFX_MAP_TEXTURE_153,
                                     GFX_NULL};
   s32 i, j, k;
   MapTileModel mapTile1;
   MapTileModel mapTile2;
   SVECTOR rotation;
   SVECTOR position;
   MapTileModel *pTileModel;

   switch (obj->state) {
   case 0:
      for (i = 0; i < 3; i++) {
         j = textureGfx[i];

         for (k = 0; k < 4; k++) {
            gGfxSubTextures[GFX_TILED_DYN_163 + i][k] = gGfxSubTextures[j][k];
         }

         gGfxClutIds[GFX_TILED_DYN_163 + i] = gGfxClutIds[j];
         gGfxTPageIds[GFX_TILED_DYN_163 + i] = gGfxTPageIds[j];
      }

      obj->z3.n = 16;
      obj->state++;

   // fallthrough
   case 1:
      if (--obj->z3.n < 0) {
         mapTile1 = sMapTileModel_8010039c;
         pTileModel = &mapTile1;

         for (i = 0; i < 8; i++) {
            pTileModel->vertices[i].vx += obj->x1.n * 32 + 16;
            pTileModel->vertices[i].vz += obj->z1.n * 32 + 16;
            pTileModel->vertices[i].vy -= 48;
         }

         for (i = 0; i < obj->state3; i++) {
            RenderMapTile(gGraphicsPtr->ot, pTileModel, GRID_COLOR_NONE);
            for (k = 0; k < 8; k++) {
               pTileModel->vertices[k].vz -= 32;
            }
         }

         mapTile2 = sMapTileModel_8010039c;

         j = 0;
         for (i = 0; i < 4; i++) {
            mapTile2.vertices[mapTile2.faces[2][i]].vz += 32 - (obj->state2 - j);
         }

         rotation.vx = 0;
         rotation.vz = rand() % 15 - 7;
         rotation.vy = 0;
         position.vx = obj->x1.n * CV(1.0) + CV(0.5);
         position.vy = CV(1.5);
         position.vz = (obj->z1.n - obj->state3) * CV(1.0) + CV(0.5);
         RotTransMapTile(&mapTile2, &rotation, &position, &mapTile2);

         mapTile2.gfx[5] = GFX_TILED_DYN_163;
         mapTile2.gfx[0] = GFX_TILED_DYN_163;
         mapTile2.gfx[4] = GFX_MAP_TEXTURE_153;
         mapTile2.gfx[2] = GFX_MAP_TEXTURE_153;
         mapTile2.gfx[1] = GFX_TILED_DYN_164;
         mapTile2.gfx[3] = GFX_TILED_DYN_165;

         gGfxSubTextures[GFX_TILED_DYN_163][0] =
             gGfxSubTextures[GFX_MAP_TEXTURE_14][0] - ((obj->state2 >> 1) - 16);
         gGfxSubTextures[GFX_TILED_DYN_163][2] = obj->state2 >> 1;
         gGfxSubTextures[GFX_TILED_DYN_164][0] =
             gGfxSubTextures[GFX_MAP_TEXTURE_153][0] - ((obj->state2 >> 1) - 16);
         gGfxSubTextures[GFX_TILED_DYN_164][2] = obj->state2 >> 1;
         gGfxSubTextures[GFX_TILED_DYN_165][2] = obj->state2 >> 1;

         RenderMapTile(gGraphicsPtr->ot, &mapTile2, GRID_COLOR_NONE);

         obj->state2 += 2;
         if (obj->state2 >= 33) {
            obj->state2 = 0;
            if (++obj->state3 >= 4) {
               Map15_RaisePlank(obj->x1.n, obj->z1.n);
               obj->state3 = 16;
               obj->state++;
            }
         }
      }

      break;

   case 2:
      if (--obj->state3 <= 0) {
         obj->state++;
      } else {
         i = rand() % obj->state3;
         i -= obj->state3 / 2;
         gCameraPos.vz += i;
      }
      break;

   case 3:
      obj->functionIndex = OBJF_NULL;
      break;
   }
}

#undef OBJF
#define OBJF 086
void Objf086_Map15_HullSplash(Object *obj) {
   static s16 splashAnimData[20] = {
       3, GFX_SPLASH_1, 2, GFX_SPLASH_2, 2, GFX_SPLASH_3, 2, GFX_SPLASH_4, 2, GFX_SPLASH_5,
       2, GFX_SPLASH_6, 2, GFX_SPLASH_7, 2, GFX_SPLASH_8, 2, GFX_NULL,     0, GFX_NULL};

   switch (obj->state) {
   case 0:
      obj->d.sprite.animData = splashAnimData;
      obj->state++;

   // fallthrough
   case 1:
      obj->x1.n += obj->x2.n;
      obj->x2.n += obj->x3.n;
      obj->z1.n += obj->z2.n;
      obj->z2.n += obj->z3.n;

      if (obj->z2.n <= 0) {
         obj->z3.n = 0;
      }

      UpdateObjAnimation(obj);
      AddObjPrim6(gGraphicsPtr->ot, obj, 0);

      if (obj->d.sprite.animFinished) {
         obj->functionIndex = OBJF_NULL;
      }
      break;
   }
}

#undef OBJF
#define OBJF 755
void Objf755_Map15_PirateStandIn(void) {
   // Spawned by EVDATA17.DAT as stand-ins for pirate sprites
}

void SnapCameraLookAt(Object *obj) {
   s32 dx, dy, dz;
   s32 dist;
   s16 zoom;

   PositionCamera(obj->x2.n, obj->y2.n, obj->z2.n);
   gCameraRotation.vy = ratan2(obj->z2.n - obj->z1.n, obj->x2.n - obj->x1.n) + DEG(270);

   dx = obj->x1.n - obj->x2.n;
   dz = obj->z1.n - obj->z2.n;
   dist = SquareRoot0(dx * dx + dz * dz);
   gCameraRotation.vx = -ratan2(obj->y2.n - obj->y1.n, dist);

   dy = obj->y1.n - obj->y2.n;
   zoom = SquareRoot0(dist * dist + dy * dy) >> 3;
   gCameraZoom.vz = zoom;
}

void EaseCameraLookAt(Object *obj) {
   s32 dx, dy, dz;
   s32 dist;
   s16 zoom;
   s16 rot;

   PanCamera(obj->x2.n, obj->y2.n, obj->z2.n, 2);
   rot = ratan2(obj->z2.n - obj->z1.n, obj->x2.n - obj->x1.n) + DEG(270);
   gCameraRotation.vy += (rot - gCameraRotation.vy) >> 2;

   dx = obj->x1.n - obj->x2.n;
   dz = obj->z1.n - obj->z2.n;
   dist = SquareRoot0(dx * dx + dz * dz);

   rot = -ratan2(obj->y2.n - obj->y1.n, dist);
   gCameraRotation.vx += (rot - gCameraRotation.vx) >> 2;

   dy = obj->y1.n - obj->y2.n;
   zoom = SquareRoot0(dist * dist + dy * dy) >> 3;
   gCameraZoom.vz += (zoom - gCameraZoom.vz) >> 2;
}
