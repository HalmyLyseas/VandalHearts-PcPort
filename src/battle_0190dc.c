#include "common.h"
#include "object.h"
#include "battle.h"
#include "units.h"
#include "state.h"
#include "field.h"
#include "window.h"

#ifdef PC_FEAT
extern int gTacticalMode;   /* Stage 3 1.3 -- GAP 5 Monk/Ninja MP parity (pc_balance.c) */
#ifdef PC_FEAT
extern int TrialExpScalingLevel(int chapter);   /* GAP 9 -- per-chapter trial XP scaling / multiplier */
extern int TrialEnemyExpMulti(int chapter);
#endif
#endif

void EmbedIntAsSjis(s32, u8 *, u8);
void EmbedExp(s32, s8 *, u8);
u8 GetItemNameLength(u8);
void ShowExpDialog(s32, u8);
void SyncGainedHp(UnitStatus *, u8);
s32 CheckForSupporterBonus(u8, u8, u8, u8, u8);
void DisplaySupporterBonus(u8, u8, u8, u8, u8);
s16 CalculateAttackDamage(UnitStatus *, UnitStatus *);
s32 TryInflictingAilment(UnitStatus *, UnitStatus *, u8);
void CalculateSupportSpellExp(UnitStatus *);
s16 CalculateSpellPowerAndExp(UnitStatus *, UnitStatus *);
void DetermineMaxMpAndStatVariance(UnitStatus *);
void CalculateUnitStats(UnitStatus *);
s32 BigIntCompare(BigInt, BigInt);
void BigIntAdd(BigInt, BigInt);
void BigIntSubtract(BigInt, BigInt);
void BigIntDivide(BigInt, BigInt, u16);

u8 gAdvantage[49][49] = {{20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 25, 25, 25, 37,
                          35, 35, 35, 35, 45, 40, 35, 47, 45, 45, 45, 45, 55, 50, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 37, 35, 25, 25, 25, 47,
                          45, 35, 35, 35, 55, 50, 40, 57, 55, 45, 45, 45, 65, 60, 50},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 42, 40, 35, 30, 30, 52,
                          50, 45, 40, 40, 60, 35, 45, 62, 60, 55, 50, 50, 70, 45, 55},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 35, 25, 25, 37,
                          35, 45, 35, 35, 55, 45, 35, 47, 45, 55, 45, 45, 65, 55, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 42, 40, 30, 30, 30, 52,
                          50, 40, 40, 40, 60, 55, 45, 62, 60, 50, 50, 50, 70, 65, 55},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 25, 25, 25, 37,
                          35, 35, 35, 35, 45, 40, 35, 47, 45, 45, 45, 45, 55, 50, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 15, 15, 15, 37,
                          35, 25, 25, 25, 45, 40, 30, 47, 45, 35, 35, 35, 55, 50, 40},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 32, 30, 30, 20, 20, 42,
                          40, 40, 30, 30, 50, 40, 35, 52, 50, 50, 40, 40, 60, 50, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 32, 30, 20, 20, 20, 42,
                          40, 30, 30, 30, 50, 45, 35, 52, 50, 40, 40, 40, 60, 55, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 37, 35, 30, 25, 25, 47,
                          45, 40, 35, 35, 55, 30, 40, 57, 55, 50, 45, 45, 65, 40, 50},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 42, 40, 40, 30, 30, 52,
                          50, 50, 40, 40, 60, 50, 45, 62, 60, 60, 50, 50, 70, 60, 55},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 20, 20, 20, 20, 32,
                          30, 30, 30, 30, 40, 35, 30, 42, 40, 40, 40, 40, 50, 45, 40},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 20, 30, 20, 20, 32,
                          30, 40, 30, 30, 50, 40, 30, 42, 40, 50, 40, 40, 60, 50, 40},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 37, 35, 25, 25, 25, 47,
                          45, 35, 35, 35, 55, 50, 40, 57, 55, 45, 45, 45, 65, 60, 50},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 20, 10, 10, 10, 32,
                          30, 20, 20, 20, 40, 35, 25, 42, 40, 30, 30, 30, 50, 45, 35},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 20, 15, 15, 37,
                          35, 30, 25, 25, 45, 20, 30, 47, 45, 40, 35, 35, 55, 30, 40},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 32, 30, 30, 20, 20, 42,
                          40, 40, 30, 30, 50, 40, 35, 52, 50, 50, 40, 40, 60, 50, 45},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 30, 28, 28, 18, 20, 40,
                          38, 38, 28, 30, 48, 38, 33, 50, 48, 48, 38, 40, 58, 48, 43},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 10, 10, 10, 10, 22,
                          20, 20, 20, 20, 30, 25, 20, 32, 30, 30, 30, 30, 40, 35, 30},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 10, 20, 10, 10, 22,
                          20, 30, 20, 20, 40, 30, 20, 32, 30, 40, 30, 30, 50, 40, 30},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 27, 25, 15, 15, 15, 37,
                          35, 25, 25, 25, 45, 40, 30, 47, 45, 35, 35, 35, 55, 50, 40},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 12, 10, 1,  1,  1,  22,
                          20, 10, 10, 10, 30, 25, 15, 32, 30, 20, 20, 20, 40, 35, 25},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 17, 15, 10, 5,  5,  27,
                          25, 20, 15, 15, 35, 10, 20, 37, 35, 30, 25, 25, 45, 20, 30},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 22, 20, 20, 10, 10, 32,
                          30, 30, 20, 20, 40, 30, 25, 42, 40, 40, 30, 30, 50, 40, 35},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 18, 18, 8,  10, 30,
                          28, 28, 18, 20, 38, 28, 23, 40, 38, 38, 28, 30, 48, 38, 33},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 2,  1,  1,  1,  1,  12,
                          10, 10, 10, 10, 20, 15, 10, 22, 20, 20, 20, 20, 30, 25, 20},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 2,  1,  10, 1,  1,  12,
                          10, 20, 10, 10, 30, 20, 10, 22, 20, 30, 20, 20, 40, 30, 20},
                         {20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 17, 15, 5,  5,  5,  27,
                          25, 15, 15, 15, 35, 30, 20, 37, 35, 25, 25, 25, 45, 40, 30},
                         {57, 22, 12, 27, 22, 1,  22, 22, 27, 17, 17, 37, 32, 22, 37, 27, 27,
                          27, 47, 42, 32, 47, 37, 37, 37, 57, 52, 42, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {60, 25, 15, 30, 25, 1,  25, 25, 30, 20, 20, 40, 35, 25, 40, 30, 30,
                          30, 50, 45, 35, 50, 40, 40, 40, 60, 55, 45, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {65, 30, 25, 15, 30, 1,  30, 30, 35, 30, 25, 45, 20, 30, 45, 40, 35,
                          35, 55, 30, 40, 55, 50, 45, 45, 65, 40, 50, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {70, 35, 35, 35, 35, 1,  35, 35, 40, 40, 30, 50, 40, 35, 50, 50, 40,
                          40, 60, 50, 45, 60, 60, 50, 50, 70, 60, 55, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {68, 33, 33, 33, 33, 1,  33, 33, 38, 38, 28, 48, 38, 33, 48, 48, 38,
                          40, 58, 48, 43, 58, 58, 48, 50, 68, 58, 53, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {47, 12, 2,  17, 12, 1,  12, 12, 17, 7,  7,  27, 22, 12, 27, 17, 17,
                          17, 37, 32, 22, 37, 27, 27, 27, 47, 42, 32, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {50, 15, 5,  20, 15, 1,  15, 15, 20, 10, 10, 30, 25, 15, 30, 20, 20,
                          20, 40, 35, 25, 40, 30, 30, 30, 50, 45, 35, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {55, 20, 15, 5,  20, 1,  20, 20, 25, 20, 15, 35, 10, 20, 35, 30, 25,
                          25, 45, 20, 30, 45, 40, 35, 35, 55, 30, 40, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {60, 25, 25, 25, 25, 1,  25, 25, 30, 30, 20, 40, 30, 25, 40, 40, 30,
                          30, 50, 40, 35, 50, 50, 40, 40, 60, 50, 45, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {58, 23, 23, 23, 23, 1,  23, 23, 28, 28, 18, 38, 28, 23, 38, 38, 28,
                          30, 48, 38, 33, 48, 48, 38, 40, 58, 48, 43, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {40, 5,  5,  10, 10, 1,  15, 15, 10, 10, 10, 20, 15, 10, 20, 20, 20,
                          20, 30, 25, 20, 30, 30, 30, 30, 40, 35, 30, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {50, 5,  15, 15, 10, 1,  15, 15, 10, 20, 10, 30, 20, 10, 20, 30, 20,
                          20, 40, 30, 20, 30, 40, 30, 30, 50, 40, 30, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {55, 20, 10, 25, 20, 1,  20, 20, 25, 15, 15, 35, 30, 20, 35, 25, 25,
                          25, 45, 40, 30, 45, 35, 35, 35, 55, 50, 40, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {37, 2,  1,  7,  2,  1,  2,  2,  7,  1,  1,  17, 12, 2,  17, 7,  7,
                          7,  27, 22, 12, 27, 17, 17, 17, 37, 32, 22, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {40, 5,  1,  10, 5,  1,  5,  5,  10, 1,  1,  20, 15, 5,  20, 10, 10,
                          10, 30, 25, 15, 30, 20, 20, 20, 40, 35, 25, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {45, 10, 5,  1,  10, 1,  10, 10, 15, 10, 5,  25, 1,  10, 25, 20, 15,
                          15, 35, 10, 20, 35, 30, 25, 25, 45, 20, 30, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {50, 15, 15, 15, 15, 1,  15, 15, 20, 20, 10, 30, 20, 15, 30, 30, 20,
                          20, 40, 30, 25, 40, 40, 30, 30, 50, 40, 35, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {48, 13, 13, 13, 13, 1,  13, 13, 18, 18, 8,  28, 18, 13, 28, 28, 18,
                          20, 38, 28, 23, 38, 38, 28, 30, 48, 38, 33, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {30, 1,  1,  1,  1,  1,  5,  5,  1,  1,  1,  10, 5,  1,  10, 10, 10,
                          10, 20, 15, 10, 20, 20, 20, 20, 30, 25, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {40, 1,  5,  5,  1,  1,  5,  5,  1,  10, 1,  20, 10, 1,  10, 20, 10,
                          10, 30, 20, 10, 20, 30, 20, 20, 40, 30, 20, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20},
                         {45, 10, 1,  15, 10, 1,  10, 10, 15, 5,  5,  25, 20, 10, 25, 15, 15,
                          15, 35, 30, 20, 35, 25, 25, 25, 45, 40, 30, 20, 20, 20, 20, 20, 20,
                          20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20, 20}};

u16 gClassBlockChance[10] = {[CLASS_KNIGHT] = 0xccc,     [CLASS_ARMOR] = 0x666,
                             [CLASS_MONK] = 0xfff,       [CLASS_ARCHER] = 0xccc,
                             [CLASS_AIRMAN] = 0xccc,     [CLASS_MAGE] = 0xfff,
                             [CLASS_PRIEST] = 0xfff,     [CLASS_7] = 0x0,
                             [CLASS_VANDALIER] = 0x7fff, 0x0};

u8 gLevelDisparityBonus[160] = {
    0,   3,   6,   9,   12,  15,  18,  21,  24,  27,  30,  33,  36,  39,  42,  45,  48,  51,
    54,  57,  60,  63,  66,  69,  72,  75,  78,  81,  84,  87,  90,  93,  96,  99,  102, 105,
    108, 111, 114, 117, 120, 123, 126, 129, 132, 135, 138, 141, 144, 147, 150, 153, 156, 159,
    162, 165, 168, 171, 174, 177, 180, 183, 186, 189, 192, 195, 198, 201, 204, 207, 210, 213,
    216, 219, 222, 225, 228, 231, 234, 237, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240,
    240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240, 240};

s16 gTerrainBonus[14] = {[TERRAIN_PLAINS] = 0,
                         [TERRAIN_PRAIRIE] = 500,
                         [TERRAIN_THICKET] = 1500,
                         [TERRAIN_BARREN] = 0,
                         [TERRAIN_WATER] = 2000,
                         [TERRAIN_VILE_BOG] = 0,
                         [TERRAIN_LAVA] = 0,
                         [TERRAIN_BOUNDARY] = 3000,
                         [TERRAIN_OBSTACLE] = 0,
                         [TERRAIN_NO_ENTRY] = 0,
                         [TERRAIN_10] = 0,
                         0,
                         0,
                         0};

s16 gTerrainPreference[14] = {[TERRAIN_PLAINS] = 0,
                              [TERRAIN_PRAIRIE] = 500,
                              [TERRAIN_THICKET] = 1500,
                              [TERRAIN_BARREN] = 0,
                              [TERRAIN_WATER] = 2000,
                              [TERRAIN_VILE_BOG] = -10000,
                              [TERRAIN_LAVA] = 0,
                              [TERRAIN_BOUNDARY] = 10000,
                              [TERRAIN_OBSTACLE] = 0,
                              [TERRAIN_NO_ENTRY] = 0,
                              [TERRAIN_10] = 0,
                              0,
                              0,
                              0};

BigInt gExperienceLevels[113] = {
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x100},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x200},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x400},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x800},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1000},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2000}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4000},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x20, 0x0},   {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0},   {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x100, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x200, 0x0},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x400, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x800, 0x0},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x1000, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x2000, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x4000, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x20, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x0, 0x0, 0x100, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x200, 0x0, 0x0},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x400, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x800, 0x0, 0x0},  {0x0, 0x0, 0x0, 0x0, 0x0, 0x1000, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x2000, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x0, 0x4000, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x8000, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x20, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x20, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x80, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x0, 0x100, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x200, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x0, 0x0, 0x400, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x800, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x0, 0x0, 0x1000, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x2000, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x0, 0x4000, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x0, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x0, 0x100, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x200, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x0, 0x400, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x800, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x0, 0x1000, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x2000, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x0, 0x4000, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x0, 0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x0, 0x100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x200, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x400, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x800, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x0, 0x1000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x2000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x0, 0x4000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x8000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x1, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x4, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x8, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},    {0x10, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x20, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x40, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x80, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},   {0x100, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x200, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x400, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x800, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},  {0x1000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x2000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}, {0x4000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x8000, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0}};

s16 gClassBaseAttack[10] = {[CLASS_KNIGHT] = 1150,    [CLASS_ARMOR] = 1300,
                            [CLASS_MONK] = 900,       [CLASS_ARCHER] = 1000,
                            [CLASS_AIRMAN] = 1400,    [CLASS_MAGE] = 650,
                            [CLASS_PRIEST] = 750,     [CLASS_7] = 0,
                            [CLASS_VANDALIER] = 1800, 0};

s16 gClassBaseDefense[10] = {[CLASS_KNIGHT] = 1000,    [CLASS_ARMOR] = 1400,
                             [CLASS_MONK] = 850,       [CLASS_ARCHER] = 800,
                             [CLASS_AIRMAN] = 1100,    [CLASS_MAGE] = 550,
                             [CLASS_PRIEST] = 600,     [CLASS_7] = 500,
                             [CLASS_VANDALIER] = 1750, 0};

s16 gClassBaseMaxHp[10] = {[CLASS_KNIGHT] = 700,    [CLASS_ARMOR] = 800,
                           [CLASS_MONK] = 650,      [CLASS_ARCHER] = 630,
                           [CLASS_AIRMAN] = 750,    [CLASS_MAGE] = 550,
                           [CLASS_PRIEST] = 500,    [CLASS_7] = 700,
                           [CLASS_VANDALIER] = 900, 0};

s16 gClassBaseAgility[10] = {[CLASS_KNIGHT] = 1000,    [CLASS_ARMOR] = 750,
                             [CLASS_MONK] = 1150,      [CLASS_ARCHER] = 950,
                             [CLASS_AIRMAN] = 1050,    [CLASS_MAGE] = 1200,
                             [CLASS_PRIEST] = 1225,    [CLASS_7] = 0,
                             [CLASS_VANDALIER] = 2500, 0};

void EmbedIntAsSjis(s32 num, u8 *dst, u8 numDigits) {
   const s32 powersOfTen[] = {10, 100, 1000, 10000, 100000};
   s32 i, power, units;
   u8 idx;
   s32 started = 0;

   if (numDigits > 1) {
      idx = numDigits - 2;
      for (i = idx; i >= 0; i--, idx--) {
         // SJIS 8140: Space
         dst[0] = 0x81;
         dst[1] = 0x40;

         if ((num > powersOfTen[idx] - 1) || started) {
            power = powersOfTen[idx];
            units = num / power;
            num = num % power;
            // SJIS 824F: Zero
            *dst++ = 0x82;
            *dst++ = 0x4f + units;
            started = 1;
         } else {
            // Leading spaces
            *dst++ = 0x81;
            *dst++ = 0x40;
         }
      }
   }
   // Final digit
   *dst++ = 0x82;
   *dst++ = 0x4f + num;
}

void EmbedExp(s32 num, s8 *dst, u8 numDigits) {
   // This will share rdata with EmbedIntAsSjis
   const s32 powersOfTen[] = {10, 100, 1000, 10000, 100000};
   s32 i, power, units;
   u8 idx;
   s32 started = 0;

   if (numDigits > 1) {
      idx = numDigits - 2;
      for (i = idx; i >= 0; idx--, i--) {
         *dst = '0';

         if ((num > powersOfTen[idx] - 1) || started) {
            power = powersOfTen[idx];
            units = num / power;
            num = num % power;
            *dst++ = '0' + units;
            started = 1;
         } else {
            dst++;
         }
      }
   }
   *dst++ = '0' + num;
   *dst++ = ' ';
   *dst++ = ' ';
   *dst++ = ' ';
   *dst++ = ' ';
}

u8 GetItemNameLength(u8 item) {
   u8 *p;
   s32 count;

   count = 0;
   p = gItemNames[item];
   while (1) {
      if (*p++ == '\0') {
         return count;
      }
      count++;
   }
}

void ShowExpDialog(s32 exp, u8 windowId) {
   u8 numDigits;
   s8 *pLine1 = "You got            ";
   s8 *pLine2 = "  experience points.";

   numDigits = 1;
   if (exp >= 10) {
      numDigits = 2;
   }
   if (exp >= 100) {
      numDigits = 3;
   }
   if (exp >= 1000) {
      numDigits = 4;
   }
   EmbedExp(exp, &pLine1[8], numDigits); // Writes to rdata?
   DrawWindow(windowId, 0, 0, 192, 45, 60, 82, WBS_CROSSED, 0);
   DrawText(12, 9, 20, 2, 0, pLine1);
   DrawText(12, 21, 20, 2, 0, pLine2);
   DisplayBasicWindow(windowId);
}

s32 s_hp_801231c4;
s32 s_maxHp_801231c8;

void SyncGainedHp(UnitStatus *unit, u8 after) {
   s32 targetHp;

   if (!after) {
      // Save values for next call (after:1)
      s_hp_801231c4 = unit->hp;
      s_maxHp_801231c8 = unit->maxHp;
   } else {
      if (unit->hpFrac != 0) {
         targetHp = s_hp_801231c4 + unit->maxHp - s_maxHp_801231c8;
         while (unit->hp < targetHp) {
            unit->hpFrac++;
            unit->hp = (gClassBaseMaxHp[unit->class] * 2 +
                        gClassBaseMaxHp[unit->class] * (unit->level - 2) +
                        (gClassBaseMaxHp[unit->class] * unit->hpVar100 / 100)) *
                       unit->hpFrac / 10000 / 100;
         }
      }
   }
}

s32 CheckForSupporterBonus(u8 z, u8 x, u8 attackerZ, u8 attackerX, u8 team) {
   if ((z != attackerZ) || (x != attackerX)) {
      if (gMapUnitsPtr[z][x].s.unitIdx != 0 && gMapUnitsPtr[z][x].s.team == team) {
         return 2;
      }
   }
   return 0;
}

void DisplaySupporterBonus(u8 z, u8 x, u8 attackerZ, u8 attackerX, u8 team) {
   Object *marker;

   if (CheckForSupporterBonus(z, x, attackerZ, attackerX, team)) {
      marker = Obj_GetUnused();
      marker->functionIndex = OBJF_ATTACK_INFO_MARKER;
      marker->d.objf052.type = ATK_MARKER_SUPPORT;
      marker->x1.s.hi = x;
      marker->z1.s.hi = z;
   }
}

s16 CalculateAttackDamage(UnitStatus *attacker, UnitStatus *defender) {
   // gExperienceLevels[n-2] etc results in negative addends, requiring the same fixup as large
   // addends to match
   Object *attackerSprite, *defenderSprite;
   u8 ax, az, dx, dz;
   s16 resist;
   s32 power;
   s8 levelDiff;
   s32 damage;
   s32 blockChance;
   BigInt expBuffer1;
   BigInt expBuffer2;
   BigInt expBuffer3;
   s32 rnd;
   s16 dirBonus = 0;

   attackerSprite = attacker->sprite;
   defenderSprite = defender->sprite;
   resist = gAdvantage[attacker->advantage][defender->advantage];

   dx = defenderSprite->x1.s.hi;
   dz = defenderSprite->z1.s.hi;
   az = attackerSprite->z1.s.hi;
   ax = attackerSprite->x1.s.hi;

   resist -= CheckForSupporterBonus(dz - 1, dx, az, ax, attacker->team);
   resist -= CheckForSupporterBonus(dz, dx - 1, az, ax, attacker->team);
   resist -= CheckForSupporterBonus(dz + 1, dx, az, ax, attacker->team);
   resist -= CheckForSupporterBonus(dz, dx + 1, az, ax, attacker->team);

   if (attacker->team == TEAM_PLAYER) {
      resist -= gItemEquipmentPower[attacker->weapon];
   }
   if (defender->team == TEAM_PLAYER) {
      resist += gItemEquipmentPower[defender->helmet];
      resist += gItemEquipmentPower[defender->armor];
   }

   // High-ground bonus
   if (attacker->class == CLASS_ARCHER) {
      resist -= (OBJ_TERRAIN(attackerSprite).s.elevation - OBJ_TERRAIN(defenderSprite).s.elevation);
   } else {
      resist -=
          (OBJ_TERRAIN(attackerSprite).s.elevation - OBJ_TERRAIN(defenderSprite).s.elevation) * 3;
   }

   // Direction bonus
   switch (defender->direction >> 10) {
   case DIR_SOUTH:
      if (attackerSprite->x1.s.hi != defenderSprite->x1.s.hi) {
         dirBonus += 1;
      } else if (attackerSprite->z1.s.hi < defenderSprite->z1.s.hi) {
         dirBonus += 2;
      }
      break;
   case DIR_WEST:
      if (attackerSprite->z1.s.hi != defenderSprite->z1.s.hi) {
         dirBonus += 1;
      } else if (attackerSprite->x1.s.hi < defenderSprite->x1.s.hi) {
         dirBonus += 2;
      }
      break;
   case DIR_NORTH:
      if (attackerSprite->x1.s.hi != defenderSprite->x1.s.hi) {
         dirBonus += 1;
      } else if (attackerSprite->z1.s.hi > defenderSprite->z1.s.hi) {
         dirBonus += 2;
      }
      break;
   default:
      if (attackerSprite->z1.s.hi != defenderSprite->z1.s.hi) {
         dirBonus += 1;
      } else if (attackerSprite->x1.s.hi > defenderSprite->x1.s.hi) {
         dirBonus += 2;
      }
      break;
   }

   resist -= dirBonus * 2;

   // Level disparity bonus
   levelDiff = (attacker->level + attacker->atkBoosted) - (defender->level + defender->defBoosted);

   if (levelDiff > 0) {
      resist -= gLevelDisparityBonus[levelDiff];
      if (resist < 1) {
         resist = 1;
      }
   } else {
      resist += gLevelDisparityBonus[-levelDiff];
      if (resist < 1) {
         resist = 1;
      }
   }

   power = attacker->atkVar10000 + (100000 / resist);
   damage = power - (defender->defVar10000 + gTerrainBonus[OBJ_TERRAIN(defenderSprite).s.terrain]);

   if (damage <= 0) {
      damage = 100;
   }

   if (defender->hpFrac - damage < 100) {
      damage = defender->hpFrac;
   }

   if (!(gState.mapNum == 21 && defender->team == TEAM_ENEMY) && !defender->paralyzed) {
      rnd = rand() & 0x7fff;
      if (defender->level > attacker->level) {
         blockChance = (defender->level - attacker->level) * 32;
      } else {
         blockChance = 0;
      }
      blockChance += gClassBlockChance[defender->class];
      blockChance -= (blockChance / 2) * dirBonus;
      if (blockChance > rnd) {
         damage = 0;
      }
      if (defender->unitId == UNIT_ID_JUGGERNAUT && dirBonus != 2) {
         damage = 0;
      }
   }

   if (defender->aglBoosted) {
      defender->aglBoosted = 0;
      damage = 0;
   }

   if (damage != 0) {
      if (attacker->unitType == UNIT_TYPE_HELL_BAT || attacker->unitType == UNIT_TYPE_GUARDDOG ||
          attacker->unitType == UNIT_TYPE_ACID_GHOST) {
         TryInflictingAilment(attacker, defender, AILMENT_POISON);
      }
      if (attacker->unitType == UNIT_TYPE_ANT_ARM || attacker->unitType == UNIT_TYPE_BLOOD_BAT) {
         TryInflictingAilment(attacker, defender, AILMENT_PARAYLYZE);
      }
   }

   // Calculate experience
   if (attacker->team == TEAM_PLAYER) {
      s32 i, j;

      if (defender->team == TEAM_PLAYER) {
         for (i = 0; i < 8; i++) {
            gState.experience[i] = 0;
         }
      }

      if (attacker->team == TEAM_PLAYER && defender->team != TEAM_PLAYER) {
         for (i = 0; i < 8; i++) {
            gState.experience[i] = 0;
            expBuffer2[i] = 0;
            expBuffer3[i] = 0;
         }
         if (damage != 0) {
#ifdef PC_FEAT
            /* GAP 9 fallback (2026-07-29): the FINAL Trial's enemies are set up through a path where
             * gState.chapter/mapNum are still in flux, so the spawn-time expMulti override AND the
             * battle_eval expScalingLevel override both read stale values -> trial enemies keep
             * expMulti 0 (no attack XP) and expScalingLevel lands on a wrong chapter. At XP-computation
             * time chap/map are stable + correct, so force the trial values here -- catches every trial
             * enemy regardless of spawn path. Mutating the enemy's expMulti + the global scaling level
             * is a persistent, harmless correction (both only feed XP). Normal mode untouched. */
            if (gTacticalMode && gState.mapNum <= 5) {
               gState.expScalingLevel = TrialExpScalingLevel(gState.chapter);
               if (defender->expMulti == 0) {
                  defender->expMulti = TrialEnemyExpMulti(gState.chapter);
               }
            }
#endif
            // expBuffer1 = SLm1
            for (i = 0; i < 8; i++) {
               expBuffer1[i] = gExperienceLevels[gState.expScalingLevel - 1][i];
            }
            // expBuffer1 -= SLm2
            BigIntSubtract(expBuffer1, gExperienceLevels[gState.expScalingLevel - 2]);
            // expBuffer2 = expBuffer1 / 20
            BigIntDivide(expBuffer1, expBuffer2, 20);
            // expBuffer3 = expBuffer2 * expMulti:
            for (i = 0; i < defender->expMulti; i++) {
               BigIntAdd(expBuffer3, expBuffer2);
            }
            // expBuffer1 = expBuffer3 / 15
            BigIntDivide(expBuffer3, expBuffer1, 15);
            // ?: expBuffer1 = (((SLm1 - SLm2) / 20) * multi) / 15

            i = 1;
            for (j = damage; j > 1000; j -= 1000) {
               i++;
            }
            if (i < 3) {
               i = 3;
            }
            if (i > 10) {
               i = 10;
            }

            if (defender->hpFrac == damage) {
               // Extra EXP for a kill
               i += 5;
            }

            for (j = 0; j < i; j++) {
               // gState.experience += expBuffer1 * i
               BigIntAdd(gState.experience, expBuffer1);
            }
         }
      }
   }

   return damage;
}

s32 TryInflictingAilment(UnitStatus *source, UnitStatus *target, u8 ailment) {
   // Unused non-void return type is to resolve a swapped instruction
   s32 chance;
   s32 i;
   BigInt expBuffer1;
   BigInt expBuffer2;
   BigInt expBuffer3;

   if (source->team == TEAM_PLAYER) {
      chance = (source->level - target->level) * 0x7fff / 100 + 0x5994;
   } else {
      chance = (source->level - target->level) * 0x7fff / 100 + 0x3fff;
   }

   if (target->ailmentSusceptibility == 1) {
      chance = 0;
   }
   if (target->ailmentSusceptibility == 2) {
      chance /= 2;
   }

   if (chance > (rand() & 0x7fff)) {
      // Calculate experience
      // Bug? I think the OR should've been bracketed differently
      if (((target->team != TEAM_PLAYER) && (ailment == AILMENT_POISON && !target->poisoned)) ||
          (ailment != AILMENT_POISON && !target->paralyzed)) {

         for (i = 0; i < 8; i++) {
            expBuffer1[i] = gExperienceLevels[gState.expScalingLevel - 1][i];
            expBuffer3[i] = 0;
         }
         BigIntSubtract(expBuffer1, gExperienceLevels[gState.expScalingLevel - 2]);
         BigIntDivide(expBuffer1, expBuffer2, 20);
         for (i = 0; i < target->expMulti; i++) {
            BigIntAdd(expBuffer3, expBuffer2);
         }
         BigIntDivide(expBuffer3, expBuffer1, 15);
         for (i = 0; i < 5; i++) {
            BigIntAdd(gState.experience, expBuffer1);
         }
      }
      // Apply ailment
      if (ailment == AILMENT_POISON) {
         target->poisoned++;
      } else {
         target->paralyzed = 1;
      }
   }
}

void CalculateSupportSpellExp(UnitStatus *target) {
   u8 level;
   s32 i;
   BigInt expBuffer1;
   BigInt expBuffer2;
   BigInt expBuffer3;

   if (target->team == TEAM_PLAYER) {
      level = target->level;
      if (level < 2) {
         level = 2;
      }

      for (i = 0; i < 8; i++) {
         expBuffer1[i] = gExperienceLevels[level - 1][i];
         expBuffer3[i] = 0;
      }
      BigIntSubtract(expBuffer1, gExperienceLevels[level - 2]);
      BigIntDivide(expBuffer1, expBuffer2, 20);
      for (i = 0; i < 20; i++) {
         BigIntAdd(expBuffer3, expBuffer2);
      }
      BigIntDivide(expBuffer3, expBuffer1, 15);
      for (i = 0; i < 5; i++) {
         BigIntAdd(gState.experience, expBuffer1);
      }
   }
}

s16 CalculateSpellPowerAndExp(UnitStatus *caster, UnitStatus *target) {
   s32 damage = 0;
   s32 power;
   s8 level;
   s16 resist = 10;
   s16 half;
   s32 i, j;
   BigInt expBuffer1;
   BigInt expBuffer2;
   BigInt expBuffer3;

   if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_DAMAGE) {
      level = gSpells[gCurrentSpell].power - target->level;
      if (level > 0) {
         resist -= level * 2;
         if (resist < 1) {
            resist = 1;
         }
      } else {
         resist -= level * 2;
      }

      half = resist >> 1;

      switch (target->magicSusceptibility) {
      case 1:
         resist += half;
         break;
      case 2:
         resist += half >> 1;
         break;
      case 4:
         resist -= half >> 1;
         break;
      case 5:
         resist -= half;
         break;
      }

      if (resist < 1) {
         resist = 1;
      }

      power = caster->atkVar10000 + (100000 / resist);
      damage = power - target->defVar10000;

      if (damage <= 0) {
         damage = 100;
      }

      if (target->hpFrac - damage < 100) {
         damage = target->hpFrac;
      }

   } else if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_RESTORE_HP) {
      //@144c
      level = gSpells[gCurrentSpell].power - target->level;
      if (level >= 0) {
         damage = 10000 - target->hpFrac;
      } else {
         damage = level * 500 + 10000;
         if (damage < 500) {
            damage = 500;
         }
         if (damage + target->hpFrac > 10000) {
            damage = 10000 - target->hpFrac;
         }
      }
   }

   // Calculate experience
   if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_DAMAGE) {
      if (caster->team == TEAM_PLAYER && target->team != TEAM_PLAYER) {
         for (i = 0; i < 8; i++) {
            expBuffer2[i] = 0;
            expBuffer3[i] = 0;
         }
         if (damage != 0) {
            for (i = 0; i < 8; i++) {
               expBuffer1[i] = gExperienceLevels[gState.expScalingLevel - 1][i];
            }
            BigIntSubtract(expBuffer1, gExperienceLevels[gState.expScalingLevel - 2]);
            BigIntDivide(expBuffer1, expBuffer2, 20);
            for (i = 0; i < target->expMulti; i++) {
               BigIntAdd(expBuffer3, expBuffer2);
            }
            BigIntDivide(expBuffer3, expBuffer1, 15);

            i = 1;
            for (j = damage; j > 1000; j -= 1000) {
               i++;
            }
            if (i > 10) {
               i = 10;
            }
            if (target->hpFrac == damage) {
               // Extra EXP for a kill
               i += 5;
            }
            for (j = 0; j < i; j++) {
               BigIntAdd(gState.experience, expBuffer1);
            }
         }
      }
   } else if (gSpellsEx[gCurrentSpell][SPELL_EX_EFFECT] == SPELL_EFFECT_RESTORE_HP) {
      if (caster->team == TEAM_PLAYER && target->team == TEAM_PLAYER) {
         //@16e8
         level = target->level;
         if (level < 2) {
            level = 2;
         }

         for (i = 0; i < 8; i++) {
            expBuffer2[i] = 0;
            expBuffer3[i] = 0;
         }

         if (damage != 0) {
            for (i = 0; i < 8; i++) {
               expBuffer1[i] = gExperienceLevels[level - 1][i];
            }
            BigIntSubtract(expBuffer1, gExperienceLevels[level - 2]);
            BigIntDivide(expBuffer1, expBuffer2, 20);
            for (i = 0; i < 20; i++) {
               BigIntAdd(expBuffer3, expBuffer2);
            }
            BigIntDivide(expBuffer3, expBuffer1, 15);

            i = 1;
            for (j = damage; j > 1000; j -= 1000) {
               i++;
            }
            if (i > 10) {
               i = 10;
            }
            if (target->hpFrac == damage) {
               i += 5;
            }
            for (j = 0; j < i; j++) {
               BigIntAdd(gState.experience, expBuffer1);
            }
         }
      }
   }

   return damage;
}

void DetermineMaxMpAndStatVariance(UnitStatus *unit) {
   s32 unitClass;

   if (unit->team == TEAM_PLAYER) {
      unitClass = unit->class;
      if (unit->name == UNIT_ASH) {
         unitClass = CLASS_7;
      }
      unit->maxMp = gClassMpMultiplier[unitClass] * unit->level;
      if (unit->class == CLASS_MONK
#ifdef PC_FEAT
          /* GAP 5 (Tactical): retail gives the Monk +advLevelFirst to compensate for its half MP rate
           * (mult 1). Tactical fixes the rate at the source (mult 2 = caster parity), so keeping this
           * compensation would double-dip and push Monk/Ninja ABOVE the pure casters. Drop it in
           * Tactical so maxMp == 2*level, identical to Mage/Priest/Sorcerer. (Ninja is also CLASS_MONK.) */
          && !gTacticalMode
#endif
         ) {
         unit->maxMp += gPartyMembers[unit->name].advLevelFirst;
      }
      if (unit->maxMp > 99) {
         unit->maxMp = 99;
      }
   }
   // 80..119
   unit->hpVar100 = (rand() % 40) + 80;
   unit->atkVar100 = (rand() % 40) + 80;
   unit->defVar100 = (rand() % 40) + 80;
   unit->agiVar100 = (rand() % 40) + 80;
}

void CalculateUnitStats(UnitStatus *unit) {
   // s32 level;
   s32 i;
   u32 a0;
   u32 a2;
   BigInt expBuffer1;
   BigInt expBuffer2;

   // 9700..10285 (assuming var100 of 80..119)
   unit->atkVar10000 = (unit->atkVar100 * 15) + 8500;
   unit->defVar10000 = (unit->defVar100 * 15) + 8500;
   unit->agiVar10000 = (unit->agiVar100 * 15) + 8500;
   // += -200..190
   unit->defVar10000 += (unit->hpVar100 * 10) - 1000;

   i = 0;
   while (BigIntCompare(unit->experience, gExperienceLevels[i]) < 2) {
      i++;
   }
   unit->level = i;

   if (unit->level > 1) {
      unit->maxHp =
          (gClassBaseMaxHp[unit->class] * 2 + gClassBaseMaxHp[unit->class] * (unit->level - 2) +
           (gClassBaseMaxHp[unit->class] * unit->hpVar100) / 100) /
          100;

      unit->hp =
          ((gClassBaseMaxHp[unit->class] * 2 + gClassBaseMaxHp[unit->class] * (unit->level - 2) +
            (gClassBaseMaxHp[unit->class] * unit->hpVar100) / 100) *
           unit->hpFrac / 10000) /
          100;

      if (unit->hp == 0 && unit->hpFrac != 0) {
         unit->hp = 1;
      }

      unit->attack =
          (gClassBaseAttack[unit->class] + (gClassBaseAttack[unit->class] / 3) * (unit->level - 2) +
           ((gClassBaseAttack[unit->class] / 3) * unit->atkVar100) / 100) /
          100;

      unit->defense = (gClassBaseDefense[unit->class] +
                       (gClassBaseDefense[unit->class] / 3) * (unit->level - 2) +
                       ((gClassBaseDefense[unit->class] / 3) * unit->defVar100) / 100) /
                      100;

      unit->agility = (gClassBaseAgility[unit->class] +
                       (gClassBaseAgility[unit->class] / 3) * (unit->level - 2) +
                       ((gClassBaseAgility[unit->class] / 3) * unit->agiVar100) / 100) /
                      100;

   } else {
      unit->maxHp = gClassBaseMaxHp[unit->class] / 50;
      unit->hp = (gClassBaseMaxHp[unit->class] * ((unit->hpFrac / 100) << 1)) / 10000;
      if (unit->hp == 0 && unit->hpFrac != 0) {
         unit->hp = 1;
      }
      unit->attack = gClassBaseAttack[unit->class] / 100;
      unit->defense = gClassBaseDefense[unit->class] / 100;
      unit->agility = gClassBaseAgility[unit->class] / 100;
   }

   for (i = 0; i < 8; i++) {
      expBuffer1[i] = unit->experience[i];
   }

   BigIntSubtract(expBuffer1, gExperienceLevels[unit->level - 1]);

   for (i = 0; i < 8; i++) {
      expBuffer2[i] = gExperienceLevels[unit->level][i];
   }

   BigIntSubtract(expBuffer2, gExperienceLevels[unit->level - 1]);

   i = 0;
   while (1) {
      if (expBuffer2[i] == 0) {
         i++;
      } else {
         break;
      }
   }

   if (i == 7) {
      i = 6;
   }

   if (expBuffer1[i] < 100) {
      a2 = expBuffer2[i + 1] + (expBuffer2[i] << 16);
      a0 = expBuffer1[i + 1] + (expBuffer1[i] << 16);
   } else {
      a2 = expBuffer2[i];
      a0 = expBuffer1[i];
   }

   a0 *= 100;
   unit->exp = (unit->level - 1) * 100;
   if (a2 != 0) {
      unit->exp += a0 / a2;
   }
}

s32 BigIntCompare(BigInt lhs, BigInt rhs) {
   s32 i;

   i = 0;
   while (1) {
      if (*lhs > *rhs) {
         return 0;
      }
      if (*lhs++ < *rhs++) {
         return 2;
      }
      i++;
      if (i == 8) {
         return 1;
      }
   }
}

void BigIntAdd(BigInt lhs, BigInt rhs) {
   // lhs += rhs
   s32 i;
   s32 value;
   s32 *pLhs, *pRhs;
   s32 lhsBuf[8];
   s32 rhsBuf[8];

   pLhs = &lhsBuf[7];
   pRhs = &rhsBuf[7];

   for (i = 0; i < 8; i++) {
      lhsBuf[i] = (s32)lhs[i];
      rhsBuf[i] = (s32)rhs[i];
   }

   for (i = 0; i < 8; i++) {
      value = *pLhs + *pRhs;

      if (value > 0xffff) {
         value -= 0x10000;
         pLhs[-1] += 1;
      }
      *pLhs-- = value;
      pRhs--;
   }

   for (i = 0; i < 8; i++) {
      lhs[i] = (u16)lhsBuf[i];
   }
}

void BigIntSubtract(BigInt lhs, BigInt rhs) {
   // lhs -= rhs
   s32 i;
   s32 *pLhs, *pRhs;
   s32 lhsBuf[8];
   s32 rhsBuf[8];

   pLhs = &lhsBuf[7];
   pRhs = &rhsBuf[7];

   for (i = 0; i < 8; i++) {
      lhsBuf[i] = (s32)lhs[i];
      rhsBuf[i] = (s32)rhs[i];
   }

   for (i = 0; i < 8; i++) {
      if (*pLhs >= *pRhs) {
         *pLhs-- = *pLhs - *pRhs;
      } else {
         *pLhs-- = 0x10000 - (*pRhs - *pLhs);
         pRhs[-1] += 1;
      }
      pRhs--;
   }

   for (i = 0; i < 8; i++) {
      lhs[i] = (u16)lhsBuf[i];
   }
}

void BigIntDivide(BigInt dividend, BigInt quotient, u16 divisor) {
   s32 i;
   s32 *pQuotient, *pDividend;
   s32 dividendBuf[8];
   s32 quotientBuf[8];

   pDividend = &dividendBuf[0];
   pQuotient = &quotientBuf[0];

   for (i = 0; i < 8; i++) {
      dividendBuf[i] = (s32)dividend[i];
      quotientBuf[i] = 0;
   }

   for (i = 0; i < 7; i++) {
      if (*pDividend >= divisor) {
         *pQuotient++ = *pDividend / divisor;
         pDividend[1] += (*pDividend % divisor) * 0x10000;
      } else {
         *pQuotient++ = 0;
         pDividend[1] += *pDividend * 0x10000;
      }
      pDividend++;
   }

   if (*pDividend >= divisor) {
      *pQuotient = *pDividend / divisor;
   } else {
      *pQuotient = 0;
   }

   for (i = 0; i < 8; i++) {
      quotient[i] = (u16)quotientBuf[i];
   }
}
