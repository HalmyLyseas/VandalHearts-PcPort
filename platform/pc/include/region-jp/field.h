/* JP-region variant of include/field.h for the port's JP include stage: the US header (with its
 * PC_PORT gate blocks) plus the one JP difference, gTerrainBonus is u16 (s16 in US). Re-merge by
 * hand when include/field.h changes. See docs/pc-port/bootstrap.md, "`region-jp/` headers". */
#ifndef FIELD_H
#define FIELD_H

#include "common.h"
#include "graphics.h"

typedef enum TerrainType {
   TERRAIN_PLAINS = 0,
   TERRAIN_PRAIRIE = 1,
   TERRAIN_THICKET = 2,
   TERRAIN_BARREN = 3,
   TERRAIN_WATER = 4,
   TERRAIN_VILE_BOG = 5,
   TERRAIN_LAVA = 6,
   TERRAIN_BOUNDARY = 7,
   TERRAIN_OBSTACLE = 8,
   TERRAIN_NO_ENTRY = 9,
   TERRAIN_10 = 10
} TerrainType;

typedef union TerrainTile {
   s8 bytes[2];
   s16 raw;
   struct {
      s8 elevation;
      s8 terrain;
   } s;
} TerrainTile;

/* Crates, Boulders, ... */
typedef struct MapObject2 {
   u8 z;
   u8 x;
} MapObject2;

/* Chests, ... */
typedef struct MapObject3 {
   u8 z;
   u8 x;
   u8 item;
} MapObject3;

typedef union MapUnit {
   s8 bytes[2];
   s16 raw;
   struct {
      s8 unitIdx;
      s8 team;
   } s;
} MapUnit;

typedef struct MapTileModel {
   SVECTOR vertices[22];
   s16 gfx[18];
   u8 faces[18][4];
   u8 shades[18]; // TBD
   u8 faceCt;
   u8 height;
} MapTileModel;

typedef enum PathStep {
   PATH_STEP_UNSET = 0x0,
   PATH_STEP_SOUTH = 0x1,
   PATH_STEP_WEST = 0x2,
   PATH_STEP_NORTH = 0x3,
   PATH_STEP_EAST = 0x4,
   PATH_STEP_INVALID = 0xff
} PathStep;

typedef enum GridColor {
   GRID_COLOR_NONE = 0,
   GRID_COLOR_BLUE = 1,
   GRID_COLOR_RED = 2,
   GRID_COLOR_YELLOW = 3
} GridColor;

#ifdef PC_FEAT
/* Purple enemy-threat overlay. A macro, not an enum member, so the matching build's GridColor is
 * byte-for-byte unchanged (no trailing-comma / value-set difference). */
#define GRID_COLOR_PURPLE 4
/* Orange for a reachable-AND-threatened tile, distinct from the native yellow attack/AoE target
 * grid. Macro (not enum) for the same byte-exact reason as PURPLE. */
#define GRID_COLOR_ORANGE 5
#endif

/* A 4-byte ImpededStep {z, x, step, rem} record is the likely shape of each gImpededSteps entry
 * (declared below as raw u8 bytes). */

typedef struct HiddenItem {
   u8 z;
   u8 x;
   u8 item;
} HiddenItem;

// ?: a PathGrid can contain PathStep (direction), num steps from start, ...
typedef u8 PathGridRow[65];

extern PathGridRow gPathGrid0[30];
extern PathGridRow gPathGrid1[30];
extern PathGridRow gPathGrid2[30];
extern PathGridRow gPathGrid3[30];
extern PathGridRow gPathGrid4[30];
extern PathGridRow gPathGrid5[30];
extern PathGridRow gPathGrid6[30];
extern PathGridRow gPathGrid10[30];
extern PathGridRow gCrateGrid[30];

extern PathGridRow *gRedAttackGridPtr;
extern PathGridRow *gYellowTargetGridPtr;
extern PathGridRow *gBlueMovementGridPtr;
extern PathGridRow *gPathGrid0_Ptr;
extern PathGridRow *gPathGrid1_Ptr;
extern PathGridRow *gPathGrid2_Ptr;
extern PathGridRow *gPathGrid3_Ptr;
extern PathGridRow *gPathGrid4_Ptr;
extern PathGridRow *gPathGrid5_Ptr;
extern PathGridRow *gPathGrid6_Ptr;
extern PathGridRow *gCrateGrid_Ptr;

// extern ImpededStep gImpededSteps[5][50];
// extern ImpededStep *gImpededStepsQueue[5];
extern u8 gImpededSteps[5][200];
extern u8 *gImpededStepsQueue[5];
extern u8 gTravelTerrainCost[14][11]; // [stepping-type][terrain-type]
/* Indexed `[stepType][diff]` in battle/path_grids.c. Hardware dimensions are [14][20]; the PC build
 * widens the OUTER dimension only, so a boundary-tile overread returns the hardware-contiguous bytes.
 * The stride (20) is never changed. See docs/pc-port/bootstrap.md, "`region-jp/` headers". */
#ifdef PERMUTER
extern u8 gTravelAscentCost[20][20];  // [stepping-type][elevation-diff]  (outer widened; stride=20)
extern u8 gTravelDescentCost[20][20];
#else
extern u8 gTravelAscentCost[14][20];  // [stepping-type][elevation-diff]
extern u8 gTravelDescentCost[14][20];
#endif
extern u8 gTravelRange[14];
extern u8 gPathBackToUnit[300];

extern u16 gTerrainBonus[];
extern s16 gTerrainPreference[];
extern TerrainTile gTerrain[30][65];
extern TerrainTile (*gTerrainPtr)[65];
extern MapUnit gMapUnits[30][65];
extern MapUnit (*gMapUnitsPtr)[65];
extern u8 *gMapDataPtr;
extern MapTileModel *gMapRowPointers[28];
extern s16 gMapCursorX, gMapCursorZ;
extern s16 gMapMinX, gMapMinZ, gMapMaxX, gMapMaxZ;
extern s16 gMapMarginX, gMapMarginZ;
extern s16 gMapSizeX, gMapSizeZ;
extern s16 gMapDataSizeX, gMapDataSizeZ;
extern VECTOR gMapScale;
extern s32 gMapViewOriginX, gMapViewOriginZ;
extern u8 gOverheadMapState;
extern BVectorZXY gMapCursorStartingPos[BATTLE_CT];
extern HiddenItem gMapHiddenItems[BATTLE_CT][2];
extern u8 gShowBlueMovementGrid;

#ifdef PC_FEAT
/* Enemy threat overlay. gThreatGrid holds the union of every living enemy's move+attack reach;
 * gShowThreatGrid toggles its display; ComputeThreatGrid() rebuilds it. */
extern PathGridRow gThreatGrid[30];
extern PathGridRow *gThreatGridPtr;   /* = &gThreatGrid[1], the [1]-origin alias the others use */
extern u8 gShowThreatGrid;
void ComputeThreatGrid(void);
#endif

s16 GetTerrainElevation(s8, s8);
void DepressButton(s32 x, s32 z);

#endif
