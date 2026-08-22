#include "common.h"
#include "object.h"
#include "state.h"
#include "cd_files.h"
#include "graphics.h"

typedef struct EvtEntityProperties {
   u8 **altAnims;
   u8 **baseAnims;
   u8 stripIdxA, stripIdxB;
   u16 padding;
} EvtEntityProperties;

extern s32 gEvtTextFiles[95];
extern s32 gEvtDataFiles[95];
extern s32 gEvtSeqSetFiles[95];
extern EvtEntityProperties *gEvtEntities[95];
extern s16 *gEvtEntityDataPointers[100];
extern u8 *gAnimSet_80101fc0[56];
extern u8 *gAnimSet_80103f8c[8];
extern u8 *gAnimSet_80103fd0[8];
extern u8 *gAnimSet_80104034[16];
extern u8 *gAnimSet_801042a8[92];
extern u8 *gAnimSet_800f2db4[52];
extern u8 *gAnimSet_8010488c[112];
extern u8 *gAnimSet_80104f18[72];
extern u8 *gAnimSet_801053cc[64];
extern u8 *gAnimSet_80105b10[124];
extern u8 *gAnimSet_80105d00[50];
extern u8 *gAnimSet_801063f4[82];

void Objf417_LoadEvent03(Object *);
void Objf418_LoadEvent06(Object *);
void Objf419_Noop(void);
void Objf252_LoadEvent05(Object *);
void Objf251_LoadEvent14(Object *);
void Objf250_350_LoadEvent16(Object *);
void Objf416_LoadEvent00(Object *);
void LoadEvent(s32);
void Objf254_LoadEvent23(Object *);
void Objf255_LoadEvent33(Object *);
void Objf257_LoadEvent37(Object *);
void Objf260_LoadEvent42(Object *);
void Objf259_LoadEvent44(Object *);
void Objf262_LoadEvent51(Object *);
void Objf263_LoadEvent54(Object *);
void Objf264_LoadEvent57(Object *);
void Noop_800b6da0(Object *);
void Noop_800b6da8(Object *);
void Objf267_LoadEvent62(Object *);
void Objf268_LoadEvent67(Object *);
void Objf460_LoadEvent01(Object *);
void Objf461_LoadEvent02(Object *);
void Objf462_LoadEvent04(Object *);
void Objf463_LoadEvent08(Object *);
void Objf464_LoadEvent09(Object *);
void Objf492_LoadEvent10(Object *);
void Objf465_LoadEvent11(Object *);
void Objf466_LoadEvent12(Object *);
void Objf467_LoadEvent13(Object *);
void Objf494_LoadEvent15(Object *);
void Objf495_LoadEvent17(Object *);
void Objf493_LoadEvent18(Object *);
void Objf468_LoadEvent19(Object *);
void Objf523_LoadEvent07(Object *);
void Objf524_LoadEvent20(Object *);
void Objf469_LoadEvent21(Object *);
void Objf470_LoadEvent22(Object *);
void Objf471_LoadEvent24(Object *);
void Objf520_LoadEvent25(Object *);
void Objf522_LoadEvent26(Object *);
void Objf496_LoadEvent27(Object *);
void Objf472_LoadEvent28(Object *);
void Objf473_LoadEvent29(Object *);
void Objf474_LoadEvent30(Object *);
void Objf475_LoadEvent31(Object *);
void Objf497_LoadEvent32(Object *);
void Objf476_LoadEvent34(Object *);
void Objf477_LoadEvent35(Object *);
void Objf478_LoadEvent36(Object *);
void Objf498_LoadEvent38(Object *);
void Objf479_LoadEvent39(Object *);
void Objf480_LoadEvent40(Object *);
void Objf481_LoadEvent41(Object *);
void Objf499_LoadEvent43(Object *);
void Objf500_LoadEvent46(Object *);
void Objf525_LoadEvent47(Object *);
void Objf501_LoadEvent48(Object *);
void Objf502_LoadEvent49(Object *);
void Objf482_LoadEvent50(Object *);
void Objf483_LoadEvent52(Object *);
void Objf503_LoadEvent53(Object *);
void Objf484_LoadEvent55(Object *);
void Objf485_LoadEvent56(Object *);
void Objf491_LoadEvent60(Object *);
void Objf486_LoadEvent61(Object *);
void Objf504_LoadEvent63(Object *);
void Objf487_LoadEvent64(Object *);
void Objf488_LoadEvent65(Object *);
void Objf489_LoadEvent66(Object *);
void Objf450_LoadEvent68(Object *);
void Objf505_LoadEvent69(Object *);
void Objf451_LoadEvent70(Object *);
void Objf490_LoadEvent71(Object *);
void Objf452_LoadEvent72(Object *);
void Objf453_LoadEvent73(Object *);
void Objf506_LoadEvent74(Object *);
void Objf454_LoadEvent75(Object *);
void Objf455_LoadEvent76(Object *);
void Objf507_LoadEvent77(Object *);
void Objf508_LoadEvent78(Object *);
void Objf456_LoadEvent79(Object *);
void Objf457_LoadEvent80(Object *);
void Objf458_LoadEvent81(Object *);
void Objf509_LoadEvent82(Object *);
void Objf459_LoadEvent83(Object *);
void Objf510_LoadEvent84(Object *);
void Objf511_LoadEvent85(Object *);
void Objf512_LoadEvent86(Object *);
void Objf513_LoadEvent87(Object *);
void Objf514_LoadEvent88(Object *);
void Objf515_LoadEvent89(Object *);
void Objf516_LoadEvent90(Object *);
void Objf517_LoadEvent91(Object *);
void Objf518_LoadEvent92(Object *);
void Objf519_LoadEvent93(Object *);
void Objf521_LoadEvent94(Object *);

void Objf417_LoadEvent03(Object *obj) {
   LoadEvent(3);
   obj->functionIndex = OBJF_NULL;
}

void Objf418_LoadEvent06(Object *obj) {
   LoadEvent(6);
   obj->functionIndex = OBJF_NULL;
}

void Objf419_Noop(void) {}

void Objf252_LoadEvent05(Object *obj) {
   LoadEvent(5);
   obj->functionIndex = OBJF_NULL;
}

void Objf251_LoadEvent14(Object *obj) {
   LoadEvent(14);
   obj->functionIndex = OBJF_NULL;
}

void Objf250_350_LoadEvent16(Object *obj) {
   LoadEvent(16);
   obj->functionIndex = OBJF_NULL;
}

void Objf416_LoadEvent00(Object *obj) {
   LoadEvent(0);
   obj->functionIndex = OBJF_NULL;
}

void LoadEvent(s32 num) {
   s32 i, entityCt;
   EvtEntityProperties *p;

   gPlayerControlSuppressed = 1;
   gState.inEvent = 1;
   gState.eventResumeLocation = 0;

   if (gEvtTextFiles[num]) {
      LoadText(gEvtTextFiles[num], gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
   }

   p = gEvtEntities[num];
   LoadCdFile(gEvtDataFiles[num], 0);
   entityCt = LoadEventData();
   for (i = 0; i < entityCt; i++, p++) {
      SetupEventEntity(p->altAnims, p->baseAnims, gEvtEntityDataPointers[i], p->stripIdxA,
                       p->stripIdxB);
   }

   if (gEvtSeqSetFiles[num]) {
      LoadSeqSet(gEvtSeqSetFiles[num]);
      FinishLoadingSeq();
   }
}

void Objf254_LoadEvent23(Object *obj) {
   LoadEvent(23);
   obj->functionIndex = OBJF_NULL;
}

void Objf255_LoadEvent33(Object *obj) {
   LoadEvent(33);
   obj->functionIndex = OBJF_NULL;
}

void Objf257_LoadEvent37(Object *obj) {
   LoadEvent(37);
   obj->functionIndex = OBJF_NULL;
}

void Objf260_LoadEvent42(Object *obj) {
   LoadEvent(42);
   obj->functionIndex = OBJF_NULL;
}

void Objf259_LoadEvent44(Object *obj) {
   LoadEvent(44);
   obj->functionIndex = OBJF_NULL;
}

void Objf262_LoadEvent51(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      LoadText(CDF_SIBAIB_DAT, gText, gTextPointers);
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      gState.currentTextPointers = gTextPointers;
      LoadCdFile(CDF_EVDATA51_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[0], 4, 5);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[1], 6, 6);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[2], 0, 0);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[4], 2, 2);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[5], 1, 1);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[6], 7, 8);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[7], 9, 6);
      LoadSeqSet(34);
      FinishLoadingSeq();
      obj->state++;
   }
}

void Objf263_LoadEvent54(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      LoadText(CDF_SIBAIC_DAT, gText, gTextPointers);
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      gState.currentTextPointers = gTextPointers;
      LoadCdFile(CDF_EVDATA54_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[0], 0, 1);
      SetupEventEntity(gAnimSet_80103f8c, gAnimSet_80101fc0, gEvtEntityDataPointers[1], 2, 3);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[2], 4, 5);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[3], 6, 6);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[4], 6, 6);
      LoadSeqSet(28);
      FinishLoadingSeq();
      obj->state++;
   }
}

void Objf264_LoadEvent57(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      LoadText(CDF_SIBAID_DAT, gText, gTextPointers);
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      gState.currentTextPointers = gTextPointers;
      LoadCdFile(CDF_EVDATA57_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[0], 0, 1);
      SetupEventEntity(gAnimSet_80103fd0, gAnimSet_80101fc0, gEvtEntityDataPointers[1], 2, 3);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[2], 4, 5);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[3], 6, 7);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[4], 19, 19);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[5], 18, 18);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[6], 17, 17);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[7], 16, 16);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[8], 15, 15);
      LoadSeqSet(17);
      FinishLoadingSeq();
      obj->state++;
   }
}

void Noop_800b6da0(Object *obj) { obj->functionIndex = OBJF_NULL; }

void Noop_800b6da8(Object *obj) { obj->functionIndex = OBJF_NULL; }

void Objf267_LoadEvent62(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      LoadText(CDF_SIBAIG_DAT, gText, gTextPointers);
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      gState.currentTextPointers = gTextPointers;
      LoadCdFile(CDF_EVDATA62_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80104034, gAnimSet_80101fc0, gEvtEntityDataPointers[0], 13, 13);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[1], 2, 3);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[2], 18, 0);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[3], 16, 17);
      obj->state++;
      LoadSeqSet(27);
      FinishLoadingSeq();
   }
}

void Objf268_LoadEvent67(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      LoadText(CDF_SIBAIH_DAT, gText, gTextPointers);
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      gState.currentTextPointers = gTextPointers;
      LoadCdFile(CDF_EVDATA67_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[0], 12, 12);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[1], 6, 6);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[2], 7, 7);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[3], 0, 13);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[4], 9, 9);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[5], 1, 1);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[6], 8, 8);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[7], 3, 3);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[8], 4, 4);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[9], 10, 10);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[10], 5, 5);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[11], 2, 2);
      SetupEventEntity(gAnimSet_80101fc0, gAnimSet_80101fc0, gEvtEntityDataPointers[12], 0, 0);
      LoadSeqSet(27);
      FinishLoadingSeq();
      obj->state++;
   }
}

void Objf460_LoadEvent01(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT01_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(7);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA01_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 18);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 17);
      SetupEventEntity(gUnitAnimSets[82], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[101], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 5, 5);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 4, 4);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 4, 4);
      SetupEventEntity(gUnitAnimSets[101], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 5, 5);
      SetupEventEntity(gUnitAnimSets[101], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 5, 5);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 4, 4);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 13, 13);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 12, 12);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 11, 11);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[13], 0, 0);
      obj->state++;
   }
}

void Objf461_LoadEvent02(Object *obj) {
   LoadEvent(2);
   obj->functionIndex = OBJF_NULL;
}

void Objf462_LoadEvent04(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT04_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(23);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA04_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 18, 18);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 17, 17);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 15, 16);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 14, 14);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 12, 12);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 12, 12);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[74], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 3, 3);
      SetupEventEntity(gUnitAnimSets[74], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 3, 3);
      SetupEventEntity(gUnitAnimSets[112], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 4, 4);
      SetupEventEntity(gUnitAnimSets[112], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 4, 4);
      SetupEventEntity(gUnitAnimSets[112], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 4, 4);
      SetupEventEntity(gUnitAnimSets[112], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 4, 4);
      obj->state++;
   }
}

void Objf463_LoadEvent08(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT08_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(3);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA08_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 17, 17);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 15, 16);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 14, 14);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 3, 3);
      SetupEventEntity(gUnitAnimSets[113], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 3, 3);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[13], 0, 0);
      obj->state++;
   }
}

void Objf464_LoadEvent09(Object *obj) {
   LoadEvent(9);
   obj->functionIndex = OBJF_NULL;
}

void Objf492_LoadEvent10(Object *obj) {
   LoadEvent(10);
   obj->functionIndex = OBJF_NULL;
}

void Objf465_LoadEvent11(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT11_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(22);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA11_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 18, 18);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 17, 17);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 14, 14);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 16, 16);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 6, 6);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 6, 6);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 6, 6);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 6, 6);
      SetupEventEntity(gUnitAnimSets[100], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 6, 6);
      SetupEventEntity(gUnitAnimSets[101], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 7, 7);
      SetupEventEntity(gUnitAnimSets[101], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 7, 7);
      SetupEventEntity(gUnitAnimSets[114], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 8, 8);
      SetupEventEntity(gUnitAnimSets[114], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 8, 8);
      SetupEventEntity(gUnitAnimSets[114], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 8, 8);
      SetupEventEntity(gUnitAnimSets[114], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 8, 8);
      SetupEventEntity(gUnitAnimSets[114], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 0, 0);
      obj->state++;
   }
}

void Objf466_LoadEvent12(Object *obj) {
   LoadEvent(12);
   obj->functionIndex = OBJF_NULL;
}

void Objf467_LoadEvent13(Object *obj) {
   LoadEvent(13);
   obj->functionIndex = OBJF_NULL;
}

void Objf494_LoadEvent15(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT15_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(9);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA15_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 18, 18);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 17, 17);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 16, 16);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 15, 15);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 14, 14);
      SetupEventEntity(gUnitAnimSets[115], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 7, 7);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 7, 7);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 7, 7);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 7, 7);
      SetupEventEntity(gUnitAnimSets[116], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 7, 7);
      obj->state++;
   }
}

void Objf495_LoadEvent17(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT17_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(5);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA17_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 17, 17);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 16, 16);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 15, 15);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 14, 14);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 13, 13);
      SetupEventEntity(gAnimSet_801042a8, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 12, 12);
      SetupEventEntity(gUnitAnimSets[105], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 8, 8);
      SetupEventEntity(gUnitAnimSets[103], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 8, 8);
      SetupEventEntity(gUnitAnimSets[102], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 9, 9);
      SetupEventEntity(gUnitAnimSets[102], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 9, 9);
      SetupEventEntity(gUnitAnimSets[102], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 9, 9);
      SetupEventEntity(gUnitAnimSets[102], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 9, 9);
      obj->state++;
   }
}

void Objf493_LoadEvent18(Object *obj) {
   LoadEvent(18);
   obj->functionIndex = OBJF_NULL;
}

void Objf468_LoadEvent19(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT19_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(36);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA19_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 17, 18);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 16, 16);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 15, 15);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 14, 14);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 13, 13);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 12, 12);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 7, 7);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 7, 7);
      SetupEventEntity(gUnitAnimSets[117], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 8, 8);
      SetupEventEntity(gUnitAnimSets[117], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 8, 8);
      SetupEventEntity(gUnitAnimSets[117], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 8, 8);
      SetupEventEntity(gUnitAnimSets[118], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 9, 9);
      SetupEventEntity(gUnitAnimSets[118], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 9, 9);
      SetupEventEntity(gUnitAnimSets[118], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 9, 9);
      SetupEventEntity(gUnitAnimSets[119], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 10, 10);
      SetupEventEntity(gUnitAnimSets[119], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 10, 10);
      SetupEventEntity(gUnitAnimSets[119], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 10, 10);
      obj->state++;
   }
}

void Objf523_LoadEvent07(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT07_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(36);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA07_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 8);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 9, 10);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 11, 12);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 7, 8);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 9, 10);
      obj->state++;
   }
}

void Objf524_LoadEvent20(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT20_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(32);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA20_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[107], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 13, 13);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 13, 13);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 13, 13);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[16], 0, 0);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[17], 7, 7);
      obj->state++;
   }
}

void Objf469_LoadEvent21(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT21_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(22);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA21_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 18, 19);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 16, 17);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 14, 15);
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 6, 6);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 10, 10);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 11, 11);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 11, 11);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 11, 11);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 11, 11);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 11, 11);
      obj->state++;
   }
}

void Objf470_LoadEvent22(Object *obj) {
   LoadEvent(22);
   obj->functionIndex = OBJF_NULL;
}

void Objf471_LoadEvent24(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT24_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(36);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA24_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 18);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 16);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 10, 10);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 11, 11);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 12, 12);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 12, 12);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 12, 12);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 13, 13);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 13, 13);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 13, 13);
      obj->state++;
   }
}

void Objf520_LoadEvent25(Object *obj) {
   LoadEvent(25);
   obj->functionIndex = OBJF_NULL;
}

void Objf522_LoadEvent26(Object *obj) {
   LoadEvent(26);
   obj->functionIndex = OBJF_NULL;
}

void Objf496_LoadEvent27(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT27_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(3);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA27_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 10, 10);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 10, 10);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 11, 11);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 12, 12);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 12, 12);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 13, 13);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 13, 13);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 13, 13);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 14, 14);
      SetupEventEntity(gUnitAnimSets[77], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 14, 14);
      obj->state++;
   }
}

void Objf472_LoadEvent28(Object *obj) {
   LoadEvent(28);
   obj->functionIndex = OBJF_NULL;
}

void Objf473_LoadEvent29(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT29_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(25);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA29_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 3);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 18, 19);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gAnimSet_8010488c, gUnitAnimSets[107], gEvtEntityDataPointers[10], 10, 17);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 11, 11);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 11, 11);
      SetupEventEntity(gUnitAnimSets[65], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 11, 11);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 12, 12);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 12, 12);
      SetupEventEntity(gUnitAnimSets[121], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 12, 12);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 13, 13);
      SetupEventEntity(gUnitAnimSets[122], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 13, 13);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 14, 14);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 14, 14);
      SetupEventEntity(gUnitAnimSets[123], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 14, 14);
      obj->state++;
   }
}

void Objf474_LoadEvent30(Object *obj) {
   LoadEvent(30);
   obj->functionIndex = OBJF_NULL;
}

void Objf475_LoadEvent31(Object *obj) {
   LoadEvent(31);
   obj->functionIndex = OBJF_NULL;
}

void Objf497_LoadEvent32(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT32_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(10);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA32_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 9, 9);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 3, 3);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 3, 3);
      obj->state++;
   }
}

void Objf476_LoadEvent34(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT34_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(33);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA34_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 10);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 12);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 13, 14);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 15, 16);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 17, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 17, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 17, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 17, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 17, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 17, 18);
      SetupEventEntity(gAnimSet_8010488c, gAnimSet_800f2db4, gEvtEntityDataPointers[19], 0, 0);
      obj->state++;
   }
}

void Objf477_LoadEvent35(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT35_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(9);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA35_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 18, 19);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 16, 17);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 14, 15);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 13, 13);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 1, 1);
      obj->state++;
   }
}

void Objf478_LoadEvent36(Object *obj) {
   LoadEvent(36);
   obj->functionIndex = OBJF_NULL;
}

void Objf498_LoadEvent38(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT38_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(12);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA38_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 17, 17);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 15, 15);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 13, 13);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 1, 1);
      SetupEventEntity(gUnitAnimSets[73], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 1, 1);
      SetupEventEntity(gUnitAnimSets[93], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 2, 2);
      SetupEventEntity(gUnitAnimSets[93], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 2, 2);
      SetupEventEntity(gUnitAnimSets[93], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 2, 2);
      SetupEventEntity(gUnitAnimSets[93], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 2, 2);
      obj->state++;
   }
}

void Objf479_LoadEvent39(Object *obj) {
   LoadEvent(39);
   obj->functionIndex = OBJF_NULL;
}
void Objf480_LoadEvent40(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT40_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(22);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA40_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 2, 2);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 19, 19);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 14, 14);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 4, 4);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 18, 18);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 17, 17);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 16, 16);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 15, 15);
      SetupEventEntity(gUnitAnimSets[104], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 1, 1);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 3, 3);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 3, 3);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 3, 3);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 3, 3);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 3, 3);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 5, 5);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 5, 5);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 5, 5);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 8, 8);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 8, 8);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 8, 8);
      SetupEventEntity(gUnitAnimSets[79], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 11, 11);
      SetupEventEntity(gUnitAnimSets[78], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 12, 12);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[22], 0, 0);
      obj->state++;
   }
}

void Objf481_LoadEvent41(Object *obj) {
   LoadEvent(41);
   obj->functionIndex = OBJF_NULL;
}

void Objf499_LoadEvent43(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT43_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(8);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA43_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 2, 2);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 4, 4);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 7, 7);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 10, 10);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 1, 1);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 1, 1);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 1, 1);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 1, 1);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 3, 3);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 3, 3);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 5, 5);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 5, 5);
      SetupEventEntity(gUnitAnimSets[79], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 8, 8);
      SetupEventEntity(gUnitAnimSets[78], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 11, 11);
      obj->state++;
   }
}

void Objf500_LoadEvent46(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT46_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(10);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA46_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 18, 18);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 17, 17);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 16, 16);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 15, 15);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 14, 14);
      SetupEventEntity(gAnimSet_80104f18, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 13, 13);
      SetupEventEntity(gUnitAnimSets[90], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 1, 1);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 3, 3);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 3, 3);
      SetupEventEntity(gUnitAnimSets[70], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 5, 5);
      SetupEventEntity(gUnitAnimSets[70], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 5, 5);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 8, 8);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 8, 8);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 8, 8);
      SetupEventEntity(gUnitAnimSets[120], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 8, 8);
      obj->state++;
   }
}

void Objf525_LoadEvent47(Object *obj) {
   LoadEvent(47);
   obj->functionIndex = OBJF_NULL;
}

void Objf501_LoadEvent48(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT48_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(13);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA48_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 1, 1);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 3, 3);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 8, 8);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 11, 11);
      obj->state++;
   }
}

void Objf502_LoadEvent49(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT49_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(8);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA49_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 1, 1);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 3, 3);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 8, 8);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 11, 11);
      SetupEventEntity(gUnitAnimSets[106], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 5, 5);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 12, 12);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 13, 13);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 13, 13);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 13, 13);
      SetupEventEntity(gUnitAnimSets[94], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 14, 14);
      SetupEventEntity(gUnitAnimSets[94], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 14, 14);
      SetupEventEntity(gUnitAnimSets[79], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 15, 15);
      SetupEventEntity(gUnitAnimSets[79], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 15, 15);
      SetupEventEntity(gUnitAnimSets[78], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 16, 16);
      SetupEventEntity(gUnitAnimSets[78], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 16, 16);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 17, 17);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 17, 17);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 17, 17);
      obj->state++;
   }
}

void Objf482_LoadEvent50(Object *obj) {
   LoadEvent(50);
   obj->functionIndex = OBJF_NULL;
}

void Objf483_LoadEvent52(Object *obj) {
   LoadEvent(52);
   obj->functionIndex = OBJF_NULL;
}

void Objf503_LoadEvent53(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT53_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(4);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA53_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 11);
      SetupEventEntity(gUnitAnimSets[89], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 5, 5);
      SetupEventEntity(gUnitAnimSets[91], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[108], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 13, 13);
      SetupEventEntity(gUnitAnimSets[61], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 15, 15);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 17, 17);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 17, 17);
      SetupEventEntity(gUnitAnimSets[68], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 18, 18);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 15, 15);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 15, 15);
      SetupEventEntity(gUnitAnimSets[68], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 18, 18);
      obj->state++;
   }
}

void Objf484_LoadEvent55(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT55_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(29);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA55_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 6, 6);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 9, 9);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 19, 19);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 7, 7);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 11);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 18, 18);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 5, 5);
      SetupEventEntity(gUnitAnimSets[75], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 5, 5);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[29], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[30], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[31], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[32], 12, 12);
      SetupEventEntity(gUnitAnimSets[76], gAnimSet_800f2db4, gEvtEntityDataPointers[33], 12, 12);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[34], 0, 0);
      obj->state++;
   }
}

void Objf485_LoadEvent56(Object *obj) {
   LoadEvent(56);
   obj->functionIndex = OBJF_NULL;
}

void Objf491_LoadEvent60(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT60_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(12);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA60_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 7, 7);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 11);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 5, 5);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 12, 12);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 13, 13);
      obj->state++;
   }
}

void Objf486_LoadEvent61(Object *obj) {
   LoadEvent(61);
   obj->functionIndex = OBJF_NULL;
}

void Objf504_LoadEvent63(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT63_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(6);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA63_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 7, 7);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 11);
      SetupEventEntity(gUnitAnimSets[108], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 5, 5);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 12, 12);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 13, 13);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 13, 13);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 13, 13);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 13, 13);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 14, 14);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 14, 14);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 14, 14);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 14, 14);
      obj->state++;
   }
}

void Objf487_LoadEvent64(Object *obj) {
   LoadEvent(64);
   obj->functionIndex = OBJF_NULL;
}

void Objf488_LoadEvent65(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT65_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(26);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA65_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 19, 19);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 6, 6);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 10, 10);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 7, 7);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 11, 11);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 18, 18);
      SetupEventEntity(gAnimSet_801053cc, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 17, 17);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 5, 5);
      SetupEventEntity(gUnitAnimSets[95], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 5, 5);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 12, 12);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 12, 12);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 12, 12);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[29], 14, 14);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[30], 15, 15);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[31], 15, 15);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[32], 15, 15);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[33], 15, 15);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[34], 16, 16);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[35], 16, 16);
      obj->state++;
   }
}

void Objf489_LoadEvent66(Object *obj) {
   LoadEvent(66);
   obj->functionIndex = OBJF_NULL;
}

void Objf450_LoadEvent68(Object *obj) {
   LoadEvent(68);
   obj->functionIndex = OBJF_NULL;
}

void Objf505_LoadEvent69(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT69_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(2);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA69_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 14, 14);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 15, 15);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 15, 15);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 16, 16);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 16, 16);
      obj->state++;
   }
}

void Objf451_LoadEvent70(Object *obj) {
   LoadEvent(70);
   obj->functionIndex = OBJF_NULL;
}

void Objf490_LoadEvent71(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT71_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(24);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA71_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 7, 7);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 6, 6);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 13);
      SetupEventEntity(gUnitAnimSets[61], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 14, 14);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[14], 15, 15);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[15], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[16], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[17], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[18], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[19], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[20], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[21], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[22], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[23], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[24], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[25], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[26], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[27], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[28], 19, 19);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[29], 19, 19);
      obj->state++;
   }
}

void Objf452_LoadEvent72(Object *obj) {
   LoadEvent(72);
   obj->functionIndex = OBJF_NULL;
}

void Objf453_LoadEvent73(Object *obj) {
   LoadEvent(73);
   obj->functionIndex = OBJF_NULL;
}

void Objf506_LoadEvent74(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT74_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(10);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA74_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 18, 18);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[13], 19, 19);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 12, 12);
      SetupEventEntity(gUnitAnimSets[99], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 12, 12);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 13, 13);
      SetupEventEntity(gUnitAnimSets[63], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 13, 13);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 14, 14);
      SetupEventEntity(gUnitAnimSets[64], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 14, 14);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 15, 15);
      SetupEventEntity(gUnitAnimSets[80], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 15, 15);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 16, 16);
      SetupEventEntity(gUnitAnimSets[124], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[29], 18, 18);
      obj->state++;
   }
}

void Objf454_LoadEvent75(Object *obj) {
   LoadEvent(75);
   obj->functionIndex = OBJF_NULL;
}

void Objf455_LoadEvent76(Object *obj) {
   LoadEvent(76);
   obj->functionIndex = OBJF_NULL;
}

void Objf507_LoadEvent77(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT77_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(13);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA77_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 18, 19);
      obj->state++;
   }
}

void Objf508_LoadEvent78(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT78_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(8);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA78_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 13);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[13], 14, 14);
      SetupEventEntity(gUnitAnimSets[91], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 15, 15);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[15], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[16], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[17], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[18], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[19], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[20], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[21], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[22], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[23], 16, 16);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[24], 17, 17);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[25], 17, 17);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[26], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[27], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[28], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[29], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[30], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[31], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[32], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[33], 18, 18);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[34], 18, 18);
      obj->state++;
   }
}

void Objf456_LoadEvent79(Object *obj) {
   LoadEvent(79);
   obj->functionIndex = OBJF_NULL;
}

void Objf457_LoadEvent80(Object *obj) {
   LoadEvent(80);
   obj->functionIndex = OBJF_NULL;
}

void Objf458_LoadEvent81(Object *obj) {
   LoadEvent(81);
   obj->functionIndex = OBJF_NULL;
}

void Objf509_LoadEvent82(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT82_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(3);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA82_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_80105d00, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 19, 19);
      SetupEventEntity(gUnitAnimSets[126], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 12, 12);
      SetupEventEntity(gUnitAnimSets[126], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 12, 12);
      SetupEventEntity(gUnitAnimSets[126], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 12, 12);
      SetupEventEntity(gUnitAnimSets[126], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 12, 12);
      SetupEventEntity(gUnitAnimSets[127], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 13, 13);
      SetupEventEntity(gUnitAnimSets[127], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 13, 13);
      SetupEventEntity(gUnitAnimSets[127], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 13, 13);
      SetupEventEntity(gUnitAnimSets[127], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 13, 13);
      SetupEventEntity(gAnimSet_80105b10, gAnimSet_800f2db4, gEvtEntityDataPointers[21], 0, 0);
      obj->state++;
   }
}

void Objf459_LoadEvent83(Object *obj) {
   LoadEvent(83);
   obj->functionIndex = OBJF_NULL;
}

void Objf510_LoadEvent84(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT84_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(35);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA84_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[89], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 14, 14);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 14, 14);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 15, 15);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 15, 15);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 15, 15);
      SetupEventEntity(gUnitAnimSets[71], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 15, 15);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 16, 16);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 16, 16);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 16, 16);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 16, 16);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 16, 16);
      SetupEventEntity(gUnitAnimSets[69], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 16, 16);
      SetupEventEntity(gUnitAnimSets[67], gAnimSet_800f2db4, gEvtEntityDataPointers[29], 18, 18);
      SetupEventEntity(gUnitAnimSets[67], gAnimSet_800f2db4, gEvtEntityDataPointers[30], 18, 18);
      SetupEventEntity(gUnitAnimSets[67], gAnimSet_800f2db4, gEvtEntityDataPointers[31], 18, 18);
      SetupEventEntity(gUnitAnimSets[72], gAnimSet_800f2db4, gEvtEntityDataPointers[32], 19, 19);
      SetupEventEntity(gUnitAnimSets[72], gAnimSet_800f2db4, gEvtEntityDataPointers[33], 19, 19);
      SetupEventEntity(gUnitAnimSets[72], gAnimSet_800f2db4, gEvtEntityDataPointers[34], 19, 19);
      obj->state++;
   }
}

void Objf511_LoadEvent85(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT85_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(9);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA85_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 19, 19);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 0, 0);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 12, 12);
      SetupEventEntity(gUnitAnimSets[110], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 12, 12);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 13, 13);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 13, 13);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 13, 13);
      SetupEventEntity(gUnitAnimSets[111], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 13, 13);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 14, 14);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 14, 14);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 14, 14);
      SetupEventEntity(gUnitAnimSets[62], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 14, 14);
      obj->state++;
   }
}

void Objf512_LoadEvent86(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT86_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(32);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA86_DAT, 0);
      LoadEventData();
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 1);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[1], 2, 2);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[2], 3, 3);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[3], 4, 5);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[4], 6, 6);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[5], 7, 7);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[6], 8, 8);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[7], 9, 9);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[8], 10, 10);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[9], 11, 11);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[10], 12, 12);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[11], 13, 13);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[12], 14, 15);
      SetupEventEntity(gAnimSet_801063f4, gAnimSet_800f2db4, gEvtEntityDataPointers[13], 16, 16);
      obj->state++;
   }
}

void Objf513_LoadEvent87(Object *obj) {
   LoadEvent(87);
   obj->functionIndex = OBJF_NULL;
}

void Objf514_LoadEvent88(Object *obj) {
   LoadEvent(88);
   obj->functionIndex = OBJF_NULL;
}

void Objf515_LoadEvent89(Object *obj) {
   LoadEvent(89);
   obj->functionIndex = OBJF_NULL;
}

void Objf516_LoadEvent90(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT90_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(29);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA90_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[98], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 19, 19);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 19, 19);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 19, 19);
      SetupEventEntity(gUnitAnimSets[81], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 19, 19);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 13, 13);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 13, 13);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 13, 13);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 13, 13);
      SetupEventEntity(gUnitAnimSets[129], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 14, 14);
      SetupEventEntity(gUnitAnimSets[129], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 14, 14);
      SetupEventEntity(gUnitAnimSets[128], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 15, 15);
      SetupEventEntity(gUnitAnimSets[128], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 15, 15);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 16, 16);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 16, 16);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[27], 16, 16);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[28], 16, 16);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[29], 17, 17);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[30], 17, 17);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[31], 17, 17);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[32], 17, 17);
      obj->state++;
   }
}

void Objf517_LoadEvent91(Object *obj) {
   LoadEvent(91);
   obj->functionIndex = OBJF_NULL;
}

void Objf518_LoadEvent92(Object *obj) {
   if (obj->state == 0) {
      gPlayerControlSuppressed = 1;
      gState.inEvent = 1;
      gState.eventResumeLocation = 0;
      LoadText(CDF_EVENT92_DAT, gText, gTextPointers);
      gState.currentTextPointers = gTextPointers;
      LoadSeqSet(27);
      FinishLoadingSeq();
      LoadCdFile(CDF_EVDATA92_DAT, 0);
      LoadEventData();
      SetupEventEntity(gUnitAnimSets[1], gAnimSet_800f2db4, gEvtEntityDataPointers[0], 0, 0);
      SetupEventEntity(gUnitAnimSets[2], gAnimSet_800f2db4, gEvtEntityDataPointers[1], 1, 1);
      SetupEventEntity(gUnitAnimSets[3], gAnimSet_800f2db4, gEvtEntityDataPointers[2], 2, 2);
      SetupEventEntity(gUnitAnimSets[4], gAnimSet_800f2db4, gEvtEntityDataPointers[3], 3, 3);
      SetupEventEntity(gUnitAnimSets[5], gAnimSet_800f2db4, gEvtEntityDataPointers[4], 4, 4);
      SetupEventEntity(gUnitAnimSets[6], gAnimSet_800f2db4, gEvtEntityDataPointers[5], 5, 5);
      SetupEventEntity(gUnitAnimSets[7], gAnimSet_800f2db4, gEvtEntityDataPointers[6], 6, 6);
      SetupEventEntity(gUnitAnimSets[8], gAnimSet_800f2db4, gEvtEntityDataPointers[7], 7, 7);
      SetupEventEntity(gUnitAnimSets[9], gAnimSet_800f2db4, gEvtEntityDataPointers[8], 8, 8);
      SetupEventEntity(gUnitAnimSets[10], gAnimSet_800f2db4, gEvtEntityDataPointers[9], 9, 9);
      SetupEventEntity(gUnitAnimSets[11], gAnimSet_800f2db4, gEvtEntityDataPointers[10], 10, 10);
      SetupEventEntity(gUnitAnimSets[12], gAnimSet_800f2db4, gEvtEntityDataPointers[11], 11, 11);
      SetupEventEntity(gUnitAnimSets[97], gAnimSet_800f2db4, gEvtEntityDataPointers[12], 12, 12);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[13], 14, 14);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[14], 14, 14);
      SetupEventEntity(gUnitAnimSets[92], gAnimSet_800f2db4, gEvtEntityDataPointers[15], 14, 14);
      SetupEventEntity(gUnitAnimSets[129], gAnimSet_800f2db4, gEvtEntityDataPointers[16], 15, 15);
      SetupEventEntity(gUnitAnimSets[129], gAnimSet_800f2db4, gEvtEntityDataPointers[17], 15, 15);
      SetupEventEntity(gUnitAnimSets[129], gAnimSet_800f2db4, gEvtEntityDataPointers[18], 15, 15);
      SetupEventEntity(gUnitAnimSets[128], gAnimSet_800f2db4, gEvtEntityDataPointers[19], 16, 16);
      SetupEventEntity(gUnitAnimSets[128], gAnimSet_800f2db4, gEvtEntityDataPointers[20], 16, 16);
      SetupEventEntity(gUnitAnimSets[128], gAnimSet_800f2db4, gEvtEntityDataPointers[21], 16, 16);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[22], 17, 17);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[23], 17, 17);
      SetupEventEntity(gUnitAnimSets[130], gAnimSet_800f2db4, gEvtEntityDataPointers[24], 17, 17);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[25], 18, 18);
      SetupEventEntity(gUnitAnimSets[96], gAnimSet_800f2db4, gEvtEntityDataPointers[26], 18, 18);
      obj->state++;
   }
}

void Objf519_LoadEvent93(Object *obj) {
   LoadEvent(93);
   obj->functionIndex = OBJF_NULL;
}

void Objf521_LoadEvent94(Object *obj) {
   gState.textSpeed = 1;
   LoadEvent(94);
   obj->functionIndex = OBJF_NULL;
}
