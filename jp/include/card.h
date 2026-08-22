#ifndef CARD_H
#define CARD_H

#include "common.h"
#include "units.h"
#include "battle.h"
#include "field.h"

#define BYTES_PER_BLOCK 8192
#define TOTAL_BLOCKS 15
#define REQUIRED_BLOCKS 2

typedef enum CardEventType {
   CARD_EVENT_TYPE_IOE = 0,
   CARD_EVENT_TYPE_ERROR = 1,
   CARD_EVENT_TYPE_TIMEOUT = 2,
   CARD_EVENT_TYPE_NEW = 3
} CardEventType;

typedef enum SaveLocation {
   SAVE_LOC_BATTLE = 0,
   SAVE_LOC_UNK = 1 /* possibly an unimplemented chapter-end save? */,
   SAVE_LOC_WORLD_MAP = 2,
   SAVE_LOC_TOWN = 3
} SaveLocation;

typedef struct CardFileData_Header {
   u8 magicNumber[2];
   u8 type;
   u8 blockCount;
   u8 sjisName[64];
   u8 padding[28];
   u8 clut[32];
   u8 icon1[128];
   u8 icon2[128];
   u8 icon3[128];
} CardFileData_Header;

typedef struct CardFileData_RegularSave {
   u32 checksum;
   PartyMember party[13];
   s16 depot[DEPOT_CT];
   // u8 field3_0x2ea;
   // u8 field4_0x2eb;
   u32 frameCounter;
   s32 gold;
   u8 textSpeed;
   u8 cameraMode;
   u8 mono;
   u8 worldMapState;
   u8 townState;
   u8 worldMapDestination;
   u8 saveLocation;
   u8 chapter;
   u8 section;
   u8 debug;
   // u8 field17_0x2fe;
   // u8 field18_0x2ff;
} CardFileData_RegularSave;

typedef struct CardFileData_InBattleSave {
   u32 checksum;
   PartyMember party[13];
   // u8 field2_0x1be;
   // u8 field3_0x1bf;
   UnitStatus units[40];
   MapObject3 chests[20];
   MapObject2 crates[10];
   MapObject2 boulders[10];
   u8 worldMapState;
   u8 townState;
   u8 worldMapDestination;
   // u8 field11_0x14e7;
   u32 frameCounter;
   u8 chapter;
   u8 section;
   u8 mapState[20];
   s16 turn;
   u8 battle;
   // u8 field18_0x1505;
   s16 depot[DEPOT_CT];
   // u8 field20_0x1632;
   // u8 field21_0x1633;
   s32 gold;
   u8 zoom;
   u8 lastSelectedUnit;
   u8 mono;
   u8 cameraMode;
   u8 textSpeed;
   // u8 field28_0x163d;
   SlainUnit slainUnits[20];
   u8 partyMemberSlain[12];
   u8 mapCursorX;
   u8 mapCursorZ;
   s16 cameraRotationX;
   s16 cameraRotationY;
   u8 debug;
} CardFileData_InBattleSave;

typedef struct DeferredInBattleSaveData {
   MapObject3 chests[20];
   MapObject2 crates[10];
   MapObject2 boulders[10];
   u8 worldMapState;
   u8 townState;
   u8 worldMapDestination;
   // u8 padding_0x67;
   u32 frameCounter;
   u8 chapter;
   u8 section;
   u8 mapState[20];
   s16 turn;
   u8 battle;
   // u8 padding_0x85;
   s16 depot[DEPOT_CT];
   // u8 padding_0x1b2[2];
   s32 gold;
   u8 zoom;
   u8 lastSelectedUnit;
   u8 mono;
   u8 cameraMode;
   u8 textSpeed;
   // u8 padding_0x1bd;
   SlainUnit slainUnits[20];
   u8 partyMemberSlain[12];
   u8 mapCursorX;
   u8 mapCursorZ;
   s16 cameraRotationX;
   s16 cameraRotationY;
   u8 debug;
} DeferredInBattleSaveData;

typedef struct CardFileData_Listing {
   u32 checksum;
   u8 slotOccupied[4];
   s8 captions[3][40];
} CardFileData_Listing;

extern CardFileData_RegularSave gRegularSave;
extern CardFileData_InBattleSave *gInBattleSaveDataPtr;
extern DeferredInBattleSaveData gDeferredInBattleSaveData;
extern CardFileData_RegularSave *gRegularSaveDataPtr;
extern CardFileData_Listing *gCardFileListingPtr;

void State_FileSaveScreen(void);
void State_FileLoadScreen(void);
void Card_PopulateRegularSave(void);
void Card_LoadRegularSaveFromBuf(void);
void Card_LoadRegularSave_Internal(void);
void CommitPartyMemberStatus(UnitStatus *);
void Card_LoadInBattleSaveFromBuf(void);
void Card_PopulateInBattleSave(void);
s32 Card_ReadInBattleSaveIntoBuf(void);
s32 Card_WriteInBattleSaveFromBuf(void);
s32 Card_ReadRegularSaveIntoBuf(u8);
s32 Card_WriteRegularSaveFromBuf(u8);
s32 Card_WriteFileListingFromBuf(void);
s32 Card_ReadFileListingIntoBuf(void);
// s32 Card_WriteRegularSave(u8);
// s32 Card_LoadRegularSave(u8);
s32 ReloadBattle(void);
s32 Card_LoadInBattleSave(void);
s32 Card_WriteInBattleSave(void);
s32 Card_WriteFileListing(void);
s32 Card_ReadFileListing(void);
s32 Card_EmbedIntIntoCaption(s32, u8 *);
s32 Card_EmbedIntIntoSjisCaption(s32, u8 *);
// void Card_UpdateCaption(u8);
s32 Card_WriteRegularSaveFrom(s32, CardFileData_RegularSave *);
s32 Card_ReadRegularSaveInto(s32, CardFileData_RegularSave *);
s32 Card_WriteFileListingFrom(CardFileData_Listing *);
s32 Card_ReadFileListingInto(CardFileData_Listing *);
s32 Card_WriteInBattleSaveFrom(CardFileData_InBattleSave *);
s32 Card_ReadInBattleSaveInto(CardFileData_InBattleSave *);
void Card_Init(void);
s32 Card_CheckState(void);
s32 Card_Format(void);
s32 Card_CountFreeBlocks(void);
s32 Card_CreateFile(u8 *, CardFileData_Header *);
s32 Card_FileExists(u8 *);
s32 Card_WriteFile(u8 *, void *, s32, s32);
s32 Card_ReadFile(u8 *, void *, s32, s32);
s32 Card_ReadHeadOfArbitraryFile(u8 *, void *);
u32 CalculateChecksum(u32, u8 *);
s32 Card_WaitForSwCardEvent(void);
void Card_ClearSwCardEvents(void);
s32 Card_WaitForHwCardEvent(void);
void Card_ClearHwCardEvents(void);

#endif
