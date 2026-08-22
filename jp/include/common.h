#ifndef COMMON_H
#define COMMON_H

#include "include_asm.h"
#include "types.h"
#include "sys/types.h"
#include "PsyQ/libetc.h"

#define NULL ((void *)0)
#define ASCII_DIGIT 0x30

// Trying these out; may revert later; is something like DEG(88.59375) any better than 0x3f0?
// Clearer in some areas, but if I want consistency, I'll end up w/ stuff like CV(0.00390625) vs 1
#define FIXED(x) ((s32)((x)*4096))
#define DEG(x) ((s32)((x) / 360.0 * 4096))
#define CV(x) ((s32)((x)*256))

// #define ANGLE_0_DEGREES 0
// #define ANGLE_45_DEGREES 0x200
// #define ANGLE_90_DEGREES 0x400
// #define ANGLE_180_DEGREES 0x800
// #define ANGLE_270_DEGREES 0xc00
// #define ANGLE_360_DEGREES 0x1000

#define ANGLE_SOUTH 0
#define ANGLE_WEST 0x400
#define ANGLE_NORTH 0x800
#define ANGLE_EAST 0xc00

#define PAD_UP PADLup
#define PAD_DOWN PADLdown
#define PAD_LEFT PADLleft
#define PAD_RIGHT PADLright
#define PAD_TRIANGLE PADRup
#define PAD_X PADRdown
#define PAD_SQUARE PADRleft
#define PAD_CIRCLE PADRright
#define PAD_L1 PADL1
#define PAD_L2 PADL2
#define PAD_R1 PADR1
#define PAD_R2 PADR2
#define PAD_START PADstart
#define PAD_SELECT PADselect
#define PAD_DPAD (PAD_UP | PAD_DOWN | PAD_LEFT | PAD_RIGHT)

#define SCREEN_WIDTH 320
#define SCREEN_HEIGHT 240
#define SCREEN_HALF_WIDTH (SCREEN_WIDTH / 2)
#define SCREEN_HALF_HEIGHT (SCREEN_HEIGHT / 2)

#define UNIT_CT 40
#define PARTY_CT 13
#define DEPOT_CT 150
#define BATTLE_CT 44
#define UNIT_DB_CT 144

// #define ABS(x) (((x)>=0)?(x):(-(x)))
// #define ABS(arg) abs(arg)
#define SET_ABS(x)                                                                                 \
   do {                                                                                            \
      s32 tmp = x;                                                                                 \
      if (tmp < 0)                                                                                 \
         x = -tmp;                                                                                 \
   } while (0)

#define CLAMP(x, min, max) ((x) < (min) ? (min) : (x) > (max) ? (max) : (x))
#define MAX(a, b) (((a) > (b)) ? (a) : (b))
#define MIN(a, b) (((a) < (b)) ? (a) : (b))
#define LO(arg) ((s8 *)&arg)[0]
#define HI(arg) ((s8 *)&arg)[1]
#define LO_H(arg) ((s16 *)&arg)[0]
#define HI_H(arg) ((s16 *)&arg)[1]
// #define TO_TILE(arg) (*((s8 *)&arg + 1))
#define ARRAY_COUNT(arr) (s32)(sizeof(arr) / sizeof(arr[0]))

#define OBJ_TERRAIN(obj) gTerrainPtr[(obj)->z1.s.hi][(obj)->x1.s.hi]
#define OBJ_TILE_STATE(obj) gTileStateGridPtr[(obj)->z1.s.hi][(obj)->x1.s.hi]
#define OBJ_TILE_MODEL(obj) gMapRowPointers[(obj)->z1.s.hi][(obj)->x1.s.hi]
#define OBJ_MAP_UNIT(obj) gMapUnitsPtr[(obj)->z1.s.hi][(obj)->x1.s.hi]

#define OBJ_TARGET_TERRAIN(obj) gTerrainPtr[(obj)->z3.s.hi][(obj)->x3.s.hi]
#define OBJ_TARGET_TILE_STATE(obj) gTileStateGridPtr[(obj)->z3.s.hi][(obj)->x3.s.hi]
#define OBJ_TARGET_TILE_MODEL(obj) gMapRowPointers[(obj)->z3.s.hi][(obj)->x3.s.hi]
#define OBJ_TARGET_MAP_UNIT(obj) gMapUnitsPtr[(obj)->z3.s.hi][(obj)->x3.s.hi]

typedef u16 BigInt[8];

struct Object;

// Note: On-screen compass vs internal coords: -N...S+  ;  -E...W+ (flipped)
typedef enum Direction { DIR_SOUTH = 0, DIR_WEST = 1, DIR_NORTH = 2, DIR_EAST = 3 } Direction;
typedef enum CameraDirection {
   CAM_DIR_SOUTH = 0,
   CAM_DIR_EAST = 1,
   CAM_DIR_NORTH = 2,
   CAM_DIR_WEST = 3
} CameraDirection;

typedef struct SVectorXZY {
   s16 x;
   s16 z;
   s16 y;
} SVectorXZY;

typedef struct SVectorXYZ {
   s16 x;
   s16 y;
   s16 z;
} SVectorXYZ;

typedef struct SVectorXY {
   s16 x;
   s16 y;
} SVectorXY;

typedef struct SVectorXZ {
   s16 x;
   s16 z;
} SVectorXZ;

typedef struct BVectorZXY {
   u8 z;
   u8 x;
   u8 y;
} BVectorZXY;

typedef struct BVectorXZ {
   u8 x;
   u8 z;
} BVectorXZ;

typedef struct BVectorZX {
   u8 z;
   u8 x;
} BVectorZX;

typedef union CoordinateValue {
   s16 n;
   struct {
      s8 lo;
      s8 hi;
   } s;
} CoordinateValue;

extern u8 gScratch1_801317c0[];
extern u8 gScratch3_80180210[];

extern s32 gSavedPadState;
extern s32 gSavedPad2State;
extern s32 gSavedPadStateNewPresses;
extern s32 gSavedPad2StateNewPresses;
extern s32 gPadState;
extern s32 gPad2State;
extern s32 gPadStateNewPresses;
extern s32 gPad2StateNewPresses;
extern s32 gClearSavedPadState;

extern u8 gPlayerControlSuppressed;
extern u8 gMapCursorSuppressed;
extern s32 gSignal1;
extern s32 gSignal2;
extern s32 gSignal3; // ?: Used for signaling completion/state of various battle fx/mechanics
extern s32 gSignal4;
extern u8 gSignal5;

extern s8 gText[];
extern s8 *gTextPointers[101];
extern u8 *gStringTable[100];
extern s16 gEvtEntityData[4096];
extern s16 *gEvtEntityDataPointers[100];

s32 BigIntCompare(BigInt, BigInt);
void BigIntAdd(BigInt, BigInt);
void BigIntSubtract(BigInt, BigInt);
void BigIntDivide(BigInt, BigInt, u16);

s32 abs(s32);
s32 rand(void);

#endif
