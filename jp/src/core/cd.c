#include "common.h"
#include "object.h"
#include "cd_files.h"
#include "state.h"
#include "graphics.h"
#include "audio.h"

#include "PsyQ/libcd.h"
#include "PsyQ/libpress.h"

static StHEADER *sMovieSectorHeader = NULL;

extern struct {
   s32 state;
   s32 loadingVabBody;
   s32 cdf;
   s32 stalledCounter;
   s32 sectorsToRead;
   s32 sector;
   s32 sectorsRead;
   CdlLOC location;
} gCdLoader;

extern struct {
   s32 state;
   s32 vabId;
   s32 headerCdf;
   s32 bodyCdf;
   s32 bodyTransferResult;
} gVabLoader;

extern struct {
   s32 state;
   s32 setIdx;
   s32 unused;
   s32 cdf;
} gSeqLoader;

extern struct {
   u32 *vlcBufferPtrs[2];
   s32 vlcBufferIdx;
   u32 *imgBufferPtr;
   RECT bufferRects[2];
   s32 bufferRectIdx;
   RECT slice;
   s16 frameFinished;
   s16 mode;
} gMovieDecoder;

typedef struct SoundSet {
   s32 vabId;
   s32 cdfVabHeader;
   s32 cdfVabBody;
   void *bufferPtr;
} SoundSet;

typedef struct SeqSet {
   s32 cdf;
   void *bufferPtr;
} SeqSet;

#ifdef PC_PORT
/* These were anonymous fixed PS1-RAM addresses. Some hosts cannot map the PS1 address range, so
 * give the overlapping sound work areas equivalent offsets in a real host allocation. Same gated
 * fix as the US tree (JP work areas 0x8014272c..0x8014672c+ fit the same 0xc0000 window). */
static u8 sPcSoundWorkRam[0xc0000];
#define SOUND_WORK_PTR(addr) ((void *)&sPcSoundWorkRam[(addr) - 0x80140000])
#else
#define SOUND_WORK_PTR(addr) ((void *)(addr))
#endif
SoundSet gSoundSets[13] = {{0, CDF_SD_JOU_VH, CDF_SD_JOU_VB, SOUND_WORK_PTR(0x8014272c)},
                           {1, CDF_SD_SEQ_VH, CDF_SD_SEQ_VB, SOUND_WORK_PTR(0x8014372c)},
                           {2, CDF_SD_BAT_VH, CDF_SD_BAT_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_TORI_VH, CDF_SD_TORI_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_TAKI_VH, CDF_SD_TAKI_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_KAZE_VH, CDF_SD_KAZE_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_BIBI_VH, CDF_SD_BIBI_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_ZIHI_VH, CDF_SD_ZIHI_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_RIN_VH, CDF_SD_RIN_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_AMA_VH, CDF_SD_AMA_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_BERA_VH, CDF_SD_BERA_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_DOR_VH, CDF_SD_DOR_VB, SOUND_WORK_PTR(0x8014672c)},
                           {2, CDF_SD_HI_VH, CDF_SD_HI_VB, SOUND_WORK_PTR(0x8014672c)}};

SeqSet gSeqSets[38] = {
    {CDF_SD_S00_BIN, gSeqData}, {CDF_SD_S01_BIN, gSeqData}, {CDF_SD_S02_BIN, gSeqData},
    {CDF_SD_S03_BIN, gSeqData}, {CDF_SD_S04_BIN, gSeqData}, {CDF_SD_S05_BIN, gSeqData},
    {CDF_SD_S06_BIN, gSeqData}, {CDF_SD_S07_BIN, gSeqData}, {CDF_SD_S08_BIN, gSeqData},
    {CDF_SD_S09_BIN, gSeqData}, {CDF_SD_S10_BIN, gSeqData}, {CDF_SD_S11_BIN, gSeqData},
    {CDF_SD_S12_BIN, gSeqData}, {CDF_SD_S13_BIN, gSeqData}, {CDF_SD_S14_BIN, gSeqData},
    {CDF_SD_S15_BIN, gSeqData}, {CDF_SD_S16_BIN, gSeqData}, {CDF_SD_S17_BIN, gSeqData},
    {CDF_SD_S18_BIN, gSeqData}, {CDF_SD_S19_BIN, gSeqData}, {CDF_SD_S20_BIN, gSeqData},
    {CDF_SD_S21_BIN, gSeqData}, {CDF_SD_S22_BIN, gSeqData}, {CDF_SD_S23_BIN, gSeqData},
    {CDF_SD_S24_BIN, gSeqData}, {CDF_SD_S25_BIN, gSeqData}, {CDF_SD_S26_BIN, gSeqData},
    {CDF_SD_S27_BIN, gSeqData}, {CDF_SD_S28_BIN, gSeqData}, {CDF_SD_S29_BIN, gSeqData},
    {CDF_SD_S30_BIN, gSeqData}, {CDF_SD_S31_BIN, gSeqData}, {CDF_SD_S32_BIN, gSeqData},
    {CDF_SD_S33_BIN, gSeqData}, {CDF_SD_S34_BIN, gSeqData}, {CDF_SD_S35_BIN, gSeqData},
    {CDF_SD_S36_BIN, gSeqData}, {CDF_SD_S37_BIN, gSeqData}};

CdFileInfo gCdFiles[712] = {[0] = {0x0000, 0x00, NULL},
                            [1] = {0x0000, 0x00, NULL},
                            [2] = {0x0000, 0x00, NULL},
                            [3] = {0x0000, 0x00, NULL},
                            [4] = {0x0000, 0x00, NULL},
                            [5] = {0x0000, 0x00, NULL},
                            [6] = {0x0000, 0x00, NULL},
                            [7] = {0x0000, 0x00, NULL},
                            [CDF_SIBAI1_1_DAT] = {0x194a, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI1_2_DAT] = {0x194b, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI1_3_DAT] = {0x194c, 0x02, gScratch1_801317c0},
                            [11] = {0x0000, 0x00, gScratch1_801317c0},
                            [CDF_SIBAI3_DAT] = {0x1950, 0x02, gScratch1_801317c0},
                            [CDF_SIBAI4_DAT] = {0x1952, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI5_DAT] = {0x1953, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI6_DAT] = {0x1954, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI7_DAT] = {0x1955, 0x01, gScratch1_801317c0},
                            [CDF_SIBAI8_DAT] = {0x1956, 0x03, gScratch1_801317c0},
                            [CDF_SIBAI9_DAT] = {0x1959, 0x01, gScratch1_801317c0},
                            [CDF_SIBAIA_DAT] = {0x195a, 0x02, gScratch1_801317c0},
                            [CDF_SIBAIB_DAT] = {0x195c, 0x01, gScratch1_801317c0},
                            [CDF_SIBAIC_DAT] = {0x195d, 0x01, gScratch1_801317c0},
                            [CDF_SIBAID_DAT] = {0x195e, 0x02, gScratch1_801317c0},
                            [CDF_SIBAIE_DAT] = {0x1960, 0x01, gScratch1_801317c0},
                            [CDF_SIBAIF_DAT] = {0x1961, 0x01, gScratch1_801317c0},
                            [CDF_SIBAIG_DAT] = {0x1962, 0x01, gScratch1_801317c0},
                            [CDF_SIBAIH_DAT] = {0x1963, 0x01, gScratch1_801317c0},
                            [CDF_SD_JOU_VH] = {0x148a, 0x02, gScratch3_80180210},
                            [CDF_SD_JOU_VB] = {0x1486, 0x04, gScratch3_80180210},
                            [CDF_SD_SEQ_VH] = {0x176e, 0x06, gScratch3_80180210},
                            [CDF_SD_SEQ_VB] = {0x16ec, 0x82, gScratch3_80180210},
                            [CDF_SD_BAT_VH] = {0x13f4, 0x05, gScratch3_80180210},
                            [CDF_SD_BAT_VB] = {0x138b, 0x69, gScratch3_80180210},
                            [CDF_SD_TORI_VH] = {0x181d, 0x02, gScratch3_80180210},
                            [CDF_SD_TORI_VB] = {0x17b2, 0x6b, gScratch3_80180210},
                            [CDF_SD_TAKI_VH] = {0x17b0, 0x02, gScratch3_80180210},
                            [CDF_SD_TAKI_VB] = {0x1774, 0x3c, gScratch3_80180210},
                            [CDF_SD_KAZE_VH] = {0x14db, 0x02, gScratch3_80180210},
                            [CDF_SD_KAZE_VB] = {0x148c, 0x4f, gScratch3_80180210},
                            [CDF_SD_BIBI_VH] = {0x1454, 0x02, gScratch3_80180210},
                            [CDF_SD_BIBI_VB] = {0x1409, 0x4b, gScratch3_80180210},
                            [CDF_SD_ZIHI_VH] = {0x1888, 0x02, gScratch3_80180210},
                            [CDF_SD_ZIHI_VB] = {0x181f, 0x69, gScratch3_80180210},
                            [CDF_SD_RIN_VH] = {0x150f, 0x02, gScratch3_80180210},
                            [CDF_SD_RIN_VB] = {0x14dd, 0x32, gScratch3_80180210},
                            [CDF_SD_S00_BIN] = {0x1511, 0x05, gScratch3_80180210},
                            [CDF_SD_S01_BIN] = {0x1516, 0x06, gScratch3_80180210},
                            [CDF_SD_S02_BIN] = {0x151c, 0x10, gScratch3_80180210},
                            [CDF_SD_S03_BIN] = {0x152c, 0x0a, gScratch3_80180210},
                            [CDF_SD_S04_BIN] = {0x1536, 0x0d, gScratch3_80180210},
                            [CDF_SD_S05_BIN] = {0x1543, 0x0f, gScratch3_80180210},
                            [CDF_SD_S06_BIN] = {0x1552, 0x0f, gScratch3_80180210},
                            [CDF_SD_S07_BIN] = {0x1561, 0x14, gScratch3_80180210},
                            [CDF_SD_S08_BIN] = {0x1575, 0x17, gScratch3_80180210},
                            [CDF_SD_S09_BIN] = {0x158c, 0x0c, gScratch3_80180210},
                            [CDF_SD_S10_BIN] = {0x1598, 0x11, gScratch3_80180210},
                            [CDF_SD_S11_BIN] = {0x15a9, 0x13, gScratch3_80180210},
                            [CDF_SD_S12_BIN] = {0x15bc, 0x11, gScratch3_80180210},
                            [CDF_SD_S13_BIN] = {0x15cd, 0x0e, gScratch3_80180210},
                            [CDF_SD_S14_BIN] = {0x15db, 0x03, gScratch3_80180210},
                            [CDF_SD_S15_BIN] = {0x15de, 0x06, gScratch3_80180210},
                            [CDF_SD_S16_BIN] = {0x15e4, 0x0e, gScratch3_80180210},
                            [CDF_SD_S17_BIN] = {0x15f2, 0x09, gScratch3_80180210},
                            [CDF_SD_S18_BIN] = {0x15fb, 0x05, gScratch3_80180210},
                            [CDF_SD_S19_BIN] = {0x1600, 0x04, gScratch3_80180210},
                            [CDF_SD_S20_BIN] = {0x1604, 0x0a, gScratch3_80180210},
                            [CDF_SD_S21_BIN] = {0x160e, 0x03, gScratch3_80180210},
                            [CDF_SD_S22_BIN] = {0x1611, 0x16, gScratch3_80180210},
                            [CDF_SD_S23_BIN] = {0x1627, 0x15, gScratch3_80180210},
                            [CDF_SD_S24_BIN] = {0x163c, 0x16, gScratch3_80180210},
                            [CDF_SD_S25_BIN] = {0x1652, 0x17, gScratch3_80180210},
                            [CDF_SD_S26_BIN] = {0x1669, 0x15, gScratch3_80180210},
                            [CDF_SD_S27_BIN] = {0x167e, 0x0d, gScratch3_80180210},
                            [CDF_SD_S28_BIN] = {0x168b, 0x0c, gScratch3_80180210},
                            [CDF_SD_S29_BIN] = {0x1697, 0x12, gScratch3_80180210},
                            [CDF_SD_S30_BIN] = {0x16a9, 0x0b, gScratch3_80180210},
                            [CDF_SD_S31_BIN] = {0x16b4, 0x07, gScratch3_80180210},
                            [CDF_SD_S32_BIN] = {0x16bb, 0x0a, gScratch3_80180210},
                            [CDF_SD_S33_BIN] = {0x16c5, 0x09, gScratch3_80180210},
                            [CDF_SD_S34_BIN] = {0x16ce, 0x0a, gScratch3_80180210},
                            [CDF_SD_S35_BIN] = {0x16d8, 0x05, gScratch3_80180210},
                            [CDF_SD_S36_BIN] = {0x16dd, 0x01, gScratch3_80180210},
                            [CDF_SD_S37_BIN] = {0x16de, 0x0e, gScratch3_80180210},
                            [CDF_SD_AMA_VH] = {0x1389, 0x02, gScratch3_80180210},
                            [CDF_SD_AMA_VB] = {0x1343, 0x46, gScratch3_80180210},
                            [CDF_SD_BERA_VH] = {0x1407, 0x02, gScratch3_80180210},
                            [CDF_SD_BERA_VB] = {0x13f9, 0x0e, gScratch3_80180210},
                            [CDF_SD_DOR_VH] = {0x145e, 0x02, gScratch3_80180210},
                            [CDF_SD_DOR_VB] = {0x1456, 0x08, gScratch3_80180210},
                            [CDF_SD_HI_VH] = {0x1484, 0x02, gScratch3_80180210},
                            [CDF_SD_HI_VB] = {0x1460, 0x24, gScratch3_80180210},
                            [91] = {0x0000, 0x00, NULL},
                            [92] = {0x0000, 0x00, NULL},
                            [93] = {0x0000, 0x00, NULL},
                            [94] = {0x0000, 0x00, NULL},
                            [95] = {0x0000, 0x00, NULL},
                            [96] = {0x0000, 0x00, NULL},
                            [97] = {0x0000, 0x00, NULL},
                            [98] = {0x0000, 0x00, NULL},
                            [99] = {0x0000, 0x00, NULL},
                            [CDF_UNIT_00_DAT] = {0x1bb8, 0x4a, gScratch3_80180210},
                            [CDF_UNIT_01_DAT] = {0x1c02, 0x4d, gScratch3_80180210},
                            [CDF_UNIT_02_DAT] = {0x1c4f, 0x4c, gScratch3_80180210},
                            [CDF_UNIT_03_DAT] = {0x1c9b, 0x4a, gScratch3_80180210},
                            [CDF_UNIT_04_DAT] = {0x1ce5, 0x45, gScratch3_80180210},
                            [CDF_UNIT_05_DAT] = {0x1d2a, 0x48, gScratch3_80180210},
                            [CDF_UNIT_06_DAT] = {0x1d72, 0x4c, gScratch3_80180210},
                            [CDF_UNIT_07_DAT] = {0x1dbe, 0x50, gScratch3_80180210},
                            [CDF_UNIT_08_DAT] = {0x1e0e, 0x47, gScratch3_80180210},
                            [CDF_UNIT_09_DAT] = {0x1e55, 0x4b, gScratch3_80180210},
                            [CDF_UNIT_0A_DAT] = {0x1ea0, 0x4a, gScratch3_80180210},
                            [CDF_UNIT_0B_DAT] = {0x1eea, 0x4e, gScratch3_80180210},
                            [CDF_UNIT_0C_DAT] = {0x1f38, 0x43, gScratch3_80180210},
                            [CDF_UNIT_0D_DAT] = {0x1f7b, 0x4a, gScratch3_80180210},
                            [CDF_UNIT_0E_DAT] = {0x1fc5, 0x3c, gScratch3_80180210},
                            [CDF_UNIT_0F_DAT] = {0x2001, 0x42, gScratch3_80180210},
                            [CDF_UNIT_10_DAT] = {0x2043, 0x4c, gScratch3_80180210},
                            [CDF_UNIT_11_DAT] = {0x208f, 0x4c, gScratch3_80180210},
                            [CDF_UNIT_12_DAT] = {0x20db, 0x45, gScratch3_80180210},
                            [CDF_UNIT_13_DAT] = {0x2120, 0x42, gScratch3_80180210},
                            [CDF_UNIT_14_DAT] = {0x2162, 0x4a, gScratch3_80180210},
                            [CDF_UNIT_15_DAT] = {0x21ac, 0x3b, gScratch3_80180210},
                            [CDF_UNIT_16_DAT] = {0x21e7, 0x34, gScratch3_80180210},
                            [CDF_UNIT_17_DAT] = {0x221b, 0x42, gScratch3_80180210},
                            [CDF_UNIT_18_DAT] = {0x225d, 0x37, gScratch3_80180210},
                            [CDF_UNIT_19_DAT] = {0x2294, 0x38, gScratch3_80180210},
                            [CDF_UNIT_1A_DAT] = {0x22cc, 0x37, gScratch3_80180210},
                            [CDF_UNIT_1B_DAT] = {0x2303, 0x37, gScratch3_80180210},
                            [CDF_UNIT_1C_DAT] = {0x233a, 0x36, gScratch3_80180210},
                            [CDF_UNIT_1D_DAT] = {0x2370, 0x34, gScratch3_80180210},
                            [CDF_UNIT_1E_DAT] = {0x23a4, 0x31, gScratch3_80180210},
                            [CDF_UNIT_1F_DAT] = {0x23d5, 0x32, gScratch3_80180210},
                            [CDF_UNIT_20_DAT] = {0x2407, 0x31, gScratch3_80180210},
                            [CDF_UNIT_21_DAT] = {0x2438, 0x31, gScratch3_80180210},
                            [CDF_UNIT_22_DAT] = {0x2469, 0x30, gScratch3_80180210},
                            [CDF_UNIT_23_DAT] = {0x2499, 0x31, gScratch3_80180210},
                            [CDF_UNIT_24_DAT] = {0x24ca, 0x31, gScratch3_80180210},
                            [CDF_UNIT_25_DAT] = {0x24fb, 0x31, gScratch3_80180210},
                            [CDF_UNIT_26_DAT] = {0x252c, 0x31, gScratch3_80180210},
                            [CDF_UNIT_27_DAT] = {0x255d, 0x30, gScratch3_80180210},
                            [CDF_UNIT_28_DAT] = {0x258d, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_29_DAT] = {0x25ba, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_2A_DAT] = {0x25e7, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_2B_DAT] = {0x2614, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_2C_DAT] = {0x2641, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_2D_DAT] = {0x266e, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_2E_DAT] = {0x269b, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_2F_DAT] = {0x26c7, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_30_DAT] = {0x26f3, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_31_DAT] = {0x271f, 0x2e, gScratch3_80180210},
                            [CDF_M_MAP01_PRS] = {0x1166, 0x0b, gScratch1_801317c0},
                            [CDF_M_MAP02_PRS] = {0x1171, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP03_PRS] = {0x117d, 0x0d, gScratch1_801317c0},
                            [CDF_M_MAP04_PRS] = {0x118a, 0x0b, gScratch1_801317c0},
                            [CDF_M_MAP05_PRS] = {0x1195, 0x0b, gScratch1_801317c0},
                            [CDF_M_MAP06_PRS] = {0x11a0, 0x0e, gScratch1_801317c0},
                            [CDF_M_MAP07_PRS] = {0x11ae, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP08_PRS] = {0x11ba, 0x0e, gScratch1_801317c0},
                            [CDF_M_MAP09_PRS] = {0x11c8, 0x0b, gScratch1_801317c0},
                            [CDF_M_MAP10_PRS] = {0x11d3, 0x10, gScratch1_801317c0},
                            [CDF_M_MAP11_PRS] = {0x11e3, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP12_PRS] = {0x11ef, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP13_PRS] = {0x11fb, 0x0a, gScratch1_801317c0},
                            [CDF_M_MAP14_PRS] = {0x1205, 0x09, gScratch1_801317c0},
                            [CDF_M_MAP15_PRS] = {0x120e, 0x0e, gScratch1_801317c0},
                            [CDF_M_MAP16_PRS] = {0x121c, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP17_PRS] = {0x1228, 0x0f, gScratch1_801317c0},
                            [CDF_M_MAP18_PRS] = {0x1237, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP19_PRS] = {0x1243, 0x0d, gScratch1_801317c0},
                            [CDF_M_MAP20_PRS] = {0x1250, 0x0f, gScratch1_801317c0},
                            [CDF_M_MAP21_PRS] = {0x125f, 0x0d, gScratch1_801317c0},
                            [CDF_M_MAP22_PRS] = {0x126c, 0x10, gScratch1_801317c0},
                            [CDF_M_MAP23_PRS] = {0x127c, 0x12, gScratch1_801317c0},
                            [CDF_M_MAP24_PRS] = {0x128e, 0x0e, gScratch1_801317c0},
                            [CDF_M_MAP25_PRS] = {0x129c, 0x0f, gScratch1_801317c0},
                            [CDF_M_MAP26_PRS] = {0x12ab, 0x0d, gScratch1_801317c0},
                            [CDF_M_MAP27_PRS] = {0x12b8, 0x0c, gScratch1_801317c0},
                            [CDF_M_MAP28_PRS] = {0x12c4, 0x11, gScratch1_801317c0},
                            [CDF_M_MAP29_PRS] = {0x12d5, 0x0f, gScratch1_801317c0},
                            [CDF_M_MAP30_PRS] = {0x12e4, 0x14, gScratch1_801317c0},
                            [CDF_M_MAP31_PRS] = {0x12f8, 0x10, gScratch1_801317c0},
                            [CDF_M_MAP32_PRS] = {0x1308, 0x0b, gScratch1_801317c0},
                            [CDF_M_MAP33_PRS] = {0x1313, 0x0e, gScratch1_801317c0},
                            [CDF_M_MAP34_PRS] = {0x1321, 0x0d, gScratch1_801317c0},
                            [184] = {0x0000, 0x00, gScratch1_801317c0},
                            [185] = {0x0000, 0x00, gScratch1_801317c0},
                            [186] = {0x0000, 0x00, gScratch1_801317c0},
                            [187] = {0x0000, 0x00, gScratch1_801317c0},
                            [188] = {0x0000, 0x00, gScratch1_801317c0},
                            [189] = {0x0000, 0x00, gScratch1_801317c0},
                            [190] = {0x0000, 0x00, gScratch1_801317c0},
                            [191] = {0x0000, 0x00, gScratch1_801317c0},
                            [CDF_M_IVE17_PRS] = {0x1101, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE18_PRS] = {0x1108, 0x08, gScratch1_801317c0},
                            [CDF_M_IVE19_PRS] = {0x1110, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE20_PRS] = {0x1117, 0x09, gScratch1_801317c0},
                            [CDF_M_IVE21_PRS] = {0x1120, 0x08, gScratch1_801317c0},
                            [CDF_IVENT_PRS] = {0x0baa, 0x07, gScratch1_801317c0},
                            [CDF_M_KEY01_PRS] = {0x1128, 0x0b, gScratch1_801317c0},
                            [CDF_M_KEY02_PRS] = {0x1133, 0x0d, gScratch1_801317c0},
                            [CDF_LAND_DT_DAT] = {0x0bbd, 0x3d, gScratch3_80180210},
                            [CDF_F_WD_DAT] = {0x0b5a, 0x50, gScratch3_80180210},
                            [CDF_F_FACE01_DAT] = {0x0813, 0x6c, gScratch3_80180210},
                            [CDF_F_FACE02_DAT] = {0x087f, 0x6c, gScratch3_80180210},
                            [CDF_F_CLAS01_DAT] = {0x05e9, 0x6c, gScratch3_80180210},
                            [CDF_F_CLAS02_DAT] = {0x0655, 0x6c, gScratch3_80180210},
                            [CDF_F_CLAS03_DAT] = {0x06c1, 0x6c, gScratch3_80180210},
                            [CDF_F_COM_01_DAT] = {0x07ad, 0x10, gScratch3_80180210},
                            [CDF_F_COM_02_DAT] = {0x07bd, 0x10, gScratch3_80180210},
                            [CDF_F_COM_03_DAT] = {0x07cd, 0x10, gScratch3_80180210},
                            [CDF_F_COM_04_DAT] = {0x07dd, 0x10, gScratch3_80180210},
                            [211] = {0x0000, 0x00, gScratch3_80180210},
                            [CDF_F_IVE03_DAT] = {0x091b, 0x10, gScratch3_80180210},
                            [CDF_F_IVE04_DAT] = {0x092b, 0x10, gScratch3_80180210},
                            [CDF_F_IVE05_DAT] = {0x093b, 0x10, gScratch3_80180210},
                            [CDF_F_IVE06_DAT] = {0x094b, 0x10, gScratch3_80180210},
                            [CDF_F_TEX01_DAT] = {0x09ee, 0x10, gScratch3_80180210},
                            [CDF_F_TEX02_DAT] = {0x09fe, 0x10, gScratch3_80180210},
                            [CDF_F_TEX03_DAT] = {0x0a0e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX04_DAT] = {0x0a1e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX05_DAT] = {0x0a2e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX06_DAT] = {0x0a3e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX07_DAT] = {0x0a4e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX08_DAT] = {0x0a5e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX09_DAT] = {0x0a6e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX10_DAT] = {0x0a7e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX11_DAT] = {0x0a8e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX12_DAT] = {0x0a9e, 0x10, gScratch3_80180210},
                            [CDF_F_TEX13_DAT] = {0x0aae, 0x10, gScratch3_80180210},
                            [CDF_F_TEX14_DAT] = {0x0abe, 0x10, gScratch3_80180210},
                            [CDF_F_TEX15_DAT] = {0x0ace, 0x10, gScratch3_80180210},
                            [CDF_F_TEX16_DAT] = {0x0ade, 0x10, gScratch3_80180210},
                            [CDF_F_TEX17_DAT] = {0x0aee, 0x10, gScratch3_80180210},
                            [CDF_F_TEX18_DAT] = {0x0afe, 0x10, gScratch3_80180210},
                            [CDF_F_IVE01_DAT] = {0x08fb, 0x10, gScratch3_80180210},
                            [CDF_F_IVE02_DAT] = {0x090b, 0x10, gScratch3_80180210},
                            [CDF_UNIT_32_DAT] = {0x274d, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_33_DAT] = {0x2778, 0x3c, gScratch3_80180210},
                            [CDF_UNIT_34_DAT] = {0x27b4, 0x2e, gScratch3_80180210},
                            [CDF_UNIT_35_DAT] = {0x27e2, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_36_DAT] = {0x280d, 0x42, gScratch3_80180210},
                            [CDF_UNIT_37_DAT] = {0x284f, 0x30, gScratch3_80180210},
                            [CDF_UNIT_38_DAT] = {0x287f, 0x31, gScratch3_80180210},
                            [CDF_UNIT_39_DAT] = {0x28b0, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_3A_DAT] = {0x28df, 0x30, gScratch3_80180210},
                            [CDF_UNIT_3B_DAT] = {0x290f, 0x34, gScratch3_80180210},
                            [CDF_UNIT_3C_DAT] = {0x2943, 0x3d, gScratch3_80180210},
                            [CDF_UNIT_3D_DAT] = {0x2980, 0x38, gScratch3_80180210},
                            [CDF_UNIT_3E_DAT] = {0x29b8, 0x34, gScratch3_80180210},
                            [CDF_UNIT_3F_DAT] = {0x29ec, 0x3e, gScratch3_80180210},
                            [CDF_UNIT_40_DAT] = {0x2a2a, 0x36, gScratch3_80180210},
                            [CDF_UNIT_41_DAT] = {0x2a60, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_42_DAT] = {0x2a8f, 0x2e, gScratch3_80180210},
                            [CDF_UNIT_43_DAT] = {0x2abd, 0x32, gScratch3_80180210},
                            [CDF_UNIT_44_DAT] = {0x2aef, 0x3e, gScratch3_80180210},
                            [CDF_UNIT_45_DAT] = {0x2b2d, 0x40, gScratch3_80180210},
                            [CDF_UNIT_46_DAT] = {0x2b6d, 0x45, gScratch3_80180210},
                            [CDF_UNIT_47_DAT] = {0x2bb2, 0x38, gScratch3_80180210},
                            [CDF_UNIT_48_DAT] = {0x2bea, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_49_DAT] = {0x2c19, 0x37, gScratch3_80180210},
                            [CDF_UNIT_4A_DAT] = {0x2c50, 0x32, gScratch3_80180210},
                            [CDF_UNIT_4B_DAT] = {0x2c82, 0x31, gScratch3_80180210},
                            [CDF_UNIT_4C_DAT] = {0x2cb3, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_4D_DAT] = {0x2ce2, 0x40, gScratch3_80180210},
                            [CDF_UNIT_4E_DAT] = {0x2d22, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_4F_DAT] = {0x2d51, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_50_DAT] = {0x2d7c, 0x3b, gScratch3_80180210},
                            [CDF_SHOP_T_DAT] = {0x1948, 0x02, gScratch1_801317c0},
                            [CDF_SAKABA_T_DAT] = {0x133d, 0x06, gScratch1_801317c0},
                            [CDF_F_TENS_TIM] = {0x09cc, 0x22, gScratch3_80180210},
                            [CDF_F_TEN2_TIM] = {0x09a7, 0x25, gScratch3_80180210},
                            [CDF_M_KEY03_PRS] = {0x1140, 0x0a, gScratch1_801317c0},
                            [CDF_M_KEY04_PRS] = {0x114a, 0x0c, gScratch1_801317c0},
                            [CDF_M_KEY05_PRS] = {0x1156, 0x0c, gScratch1_801317c0},
                            [CDF_M_KEY06_PRS] = {0x1162, 0x04, gScratch1_801317c0},
                            [CDF_F_ITEM_DAT] = {0x08eb, 0x10, gScratch3_80180210},
                            [CDF_F_COM41X_DAT] = {0x072d, 0x10, gScratch3_80180210},
                            [CDF_F_COM42X_DAT] = {0x073d, 0x10, gScratch3_80180210},
                            [CDF_F_COM43X_DAT] = {0x074d, 0x10, gScratch3_80180210},
                            [CDF_F_COM44X_DAT] = {0x075d, 0x10, gScratch3_80180210},
                            [CDF_F_COM45X_DAT] = {0x076d, 0x10, gScratch3_80180210},
                            [CDF_F_COM46X_DAT] = {0x077d, 0x10, gScratch3_80180210},
                            [CDF_F_COM47X_DAT] = {0x078d, 0x10, gScratch3_80180210},
                            [CDF_F_COM48X_DAT] = {0x079d, 0x10, gScratch3_80180210},
                            [CDF_M_IVE01_PRS] = {0x1088, 0x0b, gScratch1_801317c0},
                            [CDF_M_IVE02_PRS] = {0x1093, 0x0a, gScratch1_801317c0},
                            [CDF_M_IVE03_PRS] = {0x109d, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE04_PRS] = {0x10a4, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE05_PRS] = {0x10ab, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE06_PRS] = {0x10b2, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE07_PRS] = {0x10b9, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE08_PRS] = {0x10c0, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE09_PRS] = {0x10c7, 0x08, gScratch1_801317c0},
                            [CDF_M_IVE10_PRS] = {0x10cf, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE11_PRS] = {0x10d6, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE12_PRS] = {0x10dd, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE13_PRS] = {0x10e4, 0x08, gScratch1_801317c0},
                            [CDF_M_IVE14_PRS] = {0x10ec, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE15_PRS] = {0x10f3, 0x07, gScratch1_801317c0},
                            [CDF_M_IVE16_PRS] = {0x10fa, 0x07, gScratch1_801317c0},
                            [CDF_D_BAR_TIM] = {0x01e5, 0x26, gScratch3_80180210},
                            [CDF_D_MURA_TIM] = {0x0315, 0x26, gScratch3_80180210},
                            [CDF_D_TNSK_TIM] = {0x041f, 0x26, gScratch3_80180210},
                            [CDF_D_TOWN_TIM] = {0x0445, 0x26, gScratch3_80180210},
                            [CDF_US_DIARY_TIM] = {0x07ed, 0x26, gScratch3_80180210},
                            [CDF_F_TITLE1_TIM] = {0x0b0e, 0x26, gScratch3_80180210},
                            [CDF_D_SHOP_TIM] = {0x0387, 0x26, gScratch3_80180210},
                            [307] = {0x095b, 0x26, gScratch3_80180210},
                            [CDF_US_LOAD_TIM] = {0x0981, 0x26, gScratch3_80180210},
                            [309] = {0x0000, 0x00, NULL},
                            [CDF_MAP_1_1_TIM] = {0x0bfa, 0x26, gScratch3_80180210},
                            [CDF_MAP_1_2_TIM] = {0x0c20, 0x26, gScratch3_80180210},
                            [CDF_MAP_1_3_TIM] = {0x0c46, 0x26, gScratch3_80180210},
                            [CDF_MAP_1_4_TIM] = {0x0c6c, 0x26, gScratch3_80180210},
                            [CDF_MAP_1_5_TIM] = {0x0c92, 0x26, gScratch3_80180210},
                            [CDF_MAP_1_6_TIM] = {0x0cb8, 0x26, gScratch3_80180210},
                            [CDF_MAP_2_1_TIM] = {0x0cde, 0x26, gScratch3_80180210},
                            [CDF_MAP_2_2_TIM] = {0x0d04, 0x26, gScratch3_80180210},
                            [CDF_MAP_2_3_TIM] = {0x0d2a, 0x26, gScratch3_80180210},
                            [CDF_MAP_3_1_TIM] = {0x0d50, 0x26, gScratch3_80180210},
                            [CDF_MAP_4_1_TIM] = {0x0d76, 0x26, gScratch3_80180210},
                            [CDF_MAP_4_2_TIM] = {0x0d9c, 0x26, gScratch3_80180210},
                            [CDF_MAP_4_3_TIM] = {0x0dc2, 0x26, gScratch3_80180210},
                            [CDF_MAP_5_1_TIM] = {0x0de8, 0x26, gScratch3_80180210},
                            [CDF_MAP_5_2_TIM] = {0x0e0e, 0x26, gScratch3_80180210},
                            [CDF_MAP_5_3_TIM] = {0x0e34, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_1_TIM] = {0x0e5a, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_2_TIM] = {0x0e80, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_3_TIM] = {0x0ea6, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_4_TIM] = {0x0ecc, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_5_TIM] = {0x0ef2, 0x26, gScratch3_80180210},
                            [CDF_MAP_6_7_TIM] = {0x0f18, 0x26, gScratch3_80180210},
                            [CDF_MAP_7_1_TIM] = {0x0f3e, 0x26, gScratch3_80180210},
                            [CDF_MAP_7_2_TIM] = {0x0f64, 0x26, gScratch3_80180210},
                            [CDF_MAP_7_3_TIM] = {0x0f8a, 0x26, gScratch3_80180210},
                            [CDF_USEND_1_TIM] = {0x188a, 0x26, gScratch3_80180210},
                            [CDF_USEND_2_TIM] = {0x18b0, 0x26, gScratch3_80180210},
                            [CDF_USEND_3_TIM] = {0x18d6, 0x26, gScratch3_80180210},
                            [CDF_USEND_4_TIM] = {0x18fc, 0x26, gScratch3_80180210},
                            [CDF_USEND_5_TIM] = {0x1922, 0x26, gScratch3_80180210},
                            [CDF_D_SYMRIA_TIM] = {0x03d3, 0x26, gScratch3_80180210},
                            [CDF_D_PORTS_TIM] = {0x0361, 0x26, gScratch3_80180210},
                            [CDF_D_USU_TIM] = {0x046b, 0x26, gScratch3_80180210},
                            [CDF_D_JIGEN_TIM] = {0x02a3, 0x26, gScratch3_80180210},
                            [CDF_D_TESTA_TIM] = {0x03f9, 0x26, gScratch3_80180210},
                            [CDF_D_KNRTH_TIM] = {0x02c9, 0x26, gScratch3_80180210},
                            [CDF_D_KRCH_TIM] = {0x02ef, 0x26, gScratch3_80180210},
                            [CDF_D_SORUBO_TIM] = {0x03ad, 0x26, gScratch3_80180210},
                            [CDF_D_HEKITI_TIM] = {0x027d, 0x26, gScratch3_80180210},
                            [CDF_D_GRSGO_TIM] = {0x0257, 0x26, gScratch3_80180210},
                            [CDF_D_OMAKE_TIM] = {0x033b, 0x26, gScratch3_80180210},
                            [CDF_EVDEMO0_DAT] = {0x056e, 0x01, gScratch1_801317c0},
                            [CDF_EVDEMO3_DAT] = {0x056f, 0x01, gScratch1_801317c0},
                            [CDF_EVDEMO6_DAT] = {0x0570, 0x03, gScratch1_801317c0},
                            [CDF_EVDEMO7_DAT] = {0x0573, 0x02, gScratch1_801317c0},
                            [CDF_US_TITLE_TIM] = {0x0b34, 0x26, gScratch3_80180210},
                            [CDF_SP_SYM_TIM] = {0x1a6e, 0x26, gScratch3_80180210},
                            [CDF_SP_PORTS_TIM] = {0x1a22, 0x26, gScratch3_80180210},
                            [CDF_SP_USU_TIM] = {0x1aba, 0x26, gScratch3_80180210},
                            [CDF_SP_JIGEN_TIM] = {0x19b0, 0x26, gScratch3_80180210},
                            [CDF_SP_TESTA_TIM] = {0x1a94, 0x26, gScratch3_80180210},
                            [CDF_SP_KNRTH_TIM] = {0x19d6, 0x26, gScratch3_80180210},
                            [CDF_SP_KRCH_TIM] = {0x19fc, 0x26, gScratch3_80180210},
                            [CDF_SP_SORBO_TIM] = {0x1a48, 0x26, gScratch3_80180210},
                            [CDF_SP_HNKYO_TIM] = {0x198a, 0x26, gScratch3_80180210},
                            [CDF_SP_GRSGO_TIM] = {0x1964, 0x26, gScratch3_80180210},
                            [CDF_BR_SYM_TIM] = {0x0135, 0x26, gScratch3_80180210},
                            [CDF_BR_PORTS_TIM] = {0x00e9, 0x26, gScratch3_80180210},
                            [CDF_BR_USU_TIM] = {0x0181, 0x26, gScratch3_80180210},
                            [CDF_BR_JIGEN_TIM] = {0x0077, 0x26, gScratch3_80180210},
                            [CDF_BR_TESTA_TIM] = {0x015b, 0x26, gScratch3_80180210},
                            [CDF_BR_KNRTH_TIM] = {0x009d, 0x26, gScratch3_80180210},
                            [CDF_BR_KRCH_TIM] = {0x00c3, 0x26, gScratch3_80180210},
                            [CDF_BR_SORBO_TIM] = {0x010f, 0x26, gScratch3_80180210},
                            [CDF_BR_HNKYO_TIM] = {0x0051, 0x26, gScratch3_80180210},
                            [CDF_BR_GRSGO_TIM] = {0x002b, 0x26, gScratch3_80180210},
                            [CDF_EVDATA00_DAT] = {0x0505, 0x01, gEvtEntityData},
                            [CDF_EVDATA03_DAT] = {0x0508, 0x01, gEvtEntityData},
                            [CDF_EVDATA06_DAT] = {0x050e, 0x02, gEvtEntityData},
                            [CDF_EVDATA07_DAT] = {0x0510, 0x01, gEvtEntityData},
                            [CDF_EVDATA05_DAT] = {0x050b, 0x03, gEvtEntityData},
                            [CDF_EVDATA14_DAT] = {0x0517, 0x01, gEvtEntityData},
                            [CDF_EVDATA16_DAT] = {0x0519, 0x01, gEvtEntityData},
                            [CDF_EVDATA20_DAT] = {0x051d, 0x02, gEvtEntityData},
                            [CDF_EVDATA23_DAT] = {0x0521, 0x01, gEvtEntityData},
                            [CDF_EVDATA26_DAT] = {0x0524, 0x01, gEvtEntityData},
                            [CDF_EVDATA33_DAT] = {0x052c, 0x01, gEvtEntityData},
                            [CDF_EVDATA37_DAT] = {0x0531, 0x01, gEvtEntityData},
                            [CDF_EVDATA42_DAT] = {0x0536, 0x01, gEvtEntityData},
                            [CDF_EVDATA44_DAT] = {0x0538, 0x02, gEvtEntityData},
                            [CDF_EVDATA45_DAT] = {0x053a, 0x01, gEvtEntityData},
                            [CDF_EVDATA47_DAT] = {0x053c, 0x02, gEvtEntityData},
                            [CDF_EVDATA51_DAT] = {0x0541, 0x01, gEvtEntityData},
                            [CDF_EVDATA54_DAT] = {0x0544, 0x01, gEvtEntityData},
                            [CDF_EVDATA57_DAT] = {0x0548, 0x02, gEvtEntityData},
                            [CDF_EVDATA62_DAT] = {0x054c, 0x01, gEvtEntityData},
                            [CDF_EVDATA67_DAT] = {0x0551, 0x01, gEvtEntityData},
                            [CDF_US_S_END_TIM] = {0x1ae0, 0x26, gScratch3_80180210},
                            [CDF_US_S_ED2_TIM] = {0x1b06, 0x26, gScratch3_80180210},
                            [CDF_D_EXCHG_TIM] = {0x0231, 0x26, gScratch3_80180210},
                            [CDF_EVENT01_DAT] = {0x0575, 0x01, gScratch1_801317c0},
                            [CDF_EVENT02_DAT] = {0x0576, 0x01, gScratch1_801317c0},
                            [CDF_EVENT04_DAT] = {0x0577, 0x01, gScratch1_801317c0},
                            [CDF_EVENT07_DAT] = {0x0578, 0x01, gScratch1_801317c0},
                            [CDF_EVENT08_DAT] = {0x0579, 0x01, gScratch1_801317c0},
                            [CDF_EVENT09_DAT] = {0x057a, 0x02, gScratch1_801317c0},
                            [CDF_EVENT10_DAT] = {0x057c, 0x01, gScratch1_801317c0},
                            [CDF_EVENT11_DAT] = {0x057d, 0x01, gScratch1_801317c0},
                            [CDF_EVENT12_DAT] = {0x057e, 0x01, gScratch1_801317c0},
                            [CDF_EVENT13_DAT] = {0x057f, 0x02, gScratch1_801317c0},
                            [CDF_EVENT15_DAT] = {0x0581, 0x01, gScratch1_801317c0},
                            [CDF_EVENT17_DAT] = {0x0582, 0x01, gScratch1_801317c0},
                            [CDF_EVENT18_DAT] = {0x0583, 0x01, gScratch1_801317c0},
                            [CDF_EVENT19_DAT] = {0x0584, 0x01, gScratch1_801317c0},
                            [CDF_EVENT21_DAT] = {0x0587, 0x01, gScratch1_801317c0},
                            [CDF_EVENT22_DAT] = {0x0588, 0x01, gScratch1_801317c0},
                            [CDF_EVENT24_DAT] = {0x0589, 0x01, gScratch1_801317c0},
                            [CDF_EVENT25_DAT] = {0x058a, 0x01, gScratch1_801317c0},
                            [CDF_EVENT26_DAT] = {0x058b, 0x01, gScratch1_801317c0},
                            [CDF_EVENT27_DAT] = {0x058c, 0x01, gScratch1_801317c0},
                            [CDF_EVENT28_DAT] = {0x058d, 0x01, gScratch1_801317c0},
                            [CDF_EVENT29_DAT] = {0x058e, 0x01, gScratch1_801317c0},
                            [CDF_EVENT30_DAT] = {0x058f, 0x02, gScratch1_801317c0},
                            [CDF_EVENT31_DAT] = {0x0591, 0x01, gScratch1_801317c0},
                            [CDF_EVENT32_DAT] = {0x0592, 0x01, gScratch1_801317c0},
                            [CDF_EVENT34_DAT] = {0x0593, 0x02, gScratch1_801317c0},
                            [CDF_EVENT35_DAT] = {0x0595, 0x01, gScratch1_801317c0},
                            [CDF_EVENT36_DAT] = {0x0596, 0x01, gScratch1_801317c0},
                            [CDF_EVENT38_DAT] = {0x0597, 0x01, gScratch1_801317c0},
                            [CDF_EVENT39_DAT] = {0x0598, 0x01, gScratch1_801317c0},
                            [CDF_EVENT40_DAT] = {0x0599, 0x01, gScratch1_801317c0},
                            [CDF_EVENT41_DAT] = {0x059a, 0x01, gScratch1_801317c0},
                            [CDF_EVENT43_DAT] = {0x059b, 0x01, gScratch1_801317c0},
                            [CDF_EVENT46_DAT] = {0x059c, 0x01, gScratch1_801317c0},
                            [CDF_EVENT48_DAT] = {0x059f, 0x01, gScratch1_801317c0},
                            [CDF_EVENT49_DAT] = {0x05a0, 0x01, gScratch1_801317c0},
                            [CDF_EVENT50_DAT] = {0x05a1, 0x01, gScratch1_801317c0},
                            [CDF_EVENT52_DAT] = {0x05a2, 0x01, gScratch1_801317c0},
                            [CDF_EVENT53_DAT] = {0x05a3, 0x01, gScratch1_801317c0},
                            [CDF_EVENT55_DAT] = {0x05a4, 0x01, gScratch1_801317c0},
                            [CDF_EVENT56_DAT] = {0x05a5, 0x01, gScratch1_801317c0},
                            [CDF_EVENT60_DAT] = {0x05a6, 0x01, gScratch1_801317c0},
                            [CDF_EVENT61_DAT] = {0x05a7, 0x01, gScratch1_801317c0},
                            [CDF_EVENT63_DAT] = {0x05a8, 0x01, gScratch1_801317c0},
                            [CDF_EVENT64_DAT] = {0x05a9, 0x01, gScratch1_801317c0},
                            [CDF_EVENT65_DAT] = {0x05aa, 0x01, gScratch1_801317c0},
                            [CDF_EVENT66_DAT] = {0x05ab, 0x01, gScratch1_801317c0},
                            [CDF_EVENT68_DAT] = {0x05ac, 0x01, gScratch1_801317c0},
                            [CDF_EVENT69_DAT] = {0x05ad, 0x01, gScratch1_801317c0},
                            [CDF_EVENT70_DAT] = {0x05ae, 0x01, gScratch1_801317c0},
                            [CDF_EVENT71_DAT] = {0x05af, 0x01, gScratch1_801317c0},
                            [CDF_EVENT72_DAT] = {0x05b0, 0x01, gScratch1_801317c0},
                            [CDF_EVENT73_DAT] = {0x05b1, 0x01, gScratch1_801317c0},
                            [CDF_EVENT74_DAT] = {0x05b2, 0x01, gScratch1_801317c0},
                            [CDF_EVENT75_DAT] = {0x05b3, 0x01, gScratch1_801317c0},
                            [CDF_EVENT76_DAT] = {0x05b4, 0x02, gScratch1_801317c0},
                            [CDF_EVENT77_DAT] = {0x05b6, 0x01, gScratch1_801317c0},
                            [CDF_EVENT78_DAT] = {0x05b7, 0x01, gScratch1_801317c0},
                            [CDF_EVENT79_DAT] = {0x05b8, 0x01, gScratch1_801317c0},
                            [CDF_EVENT80_DAT] = {0x05b9, 0x01, gScratch1_801317c0},
                            [CDF_EVENT81_DAT] = {0x05ba, 0x01, gScratch1_801317c0},
                            [CDF_EVENT82_DAT] = {0x05bb, 0x01, gScratch1_801317c0},
                            [CDF_EVENT83_DAT] = {0x05bc, 0x02, gScratch1_801317c0},
                            [CDF_EVENT84_DAT] = {0x05be, 0x01, gScratch1_801317c0},
                            [CDF_EVENT85_DAT] = {0x05bf, 0x01, gScratch1_801317c0},
                            [CDF_EVENT86_DAT] = {0x05c0, 0x01, gScratch1_801317c0},
                            [CDF_EVENT87_DAT] = {0x05c1, 0x01, gScratch1_801317c0},
                            [CDF_EVENT88_DAT] = {0x05c2, 0x01, gScratch1_801317c0},
                            [CDF_EVENT89_DAT] = {0x05c3, 0x01, gScratch1_801317c0},
                            [CDF_EVENT90_DAT] = {0x05c4, 0x01, gScratch1_801317c0},
                            [CDF_EVENT91_DAT] = {0x05c5, 0x01, gScratch1_801317c0},
                            [CDF_EVENT92_DAT] = {0x05c6, 0x01, gScratch1_801317c0},
                            [CDF_EVENT93_DAT] = {0x05c7, 0x01, gScratch1_801317c0},
                            [CDF_EVENT94_DAT] = {0x05c8, 0x01, gScratch1_801317c0},
                            [CDF_EVENT47_DAT] = {0x059d, 0x02, gScratch1_801317c0},
                            [CDF_EVENT20_DAT] = {0x0585, 0x02, gScratch1_801317c0},
                            [476] = {0x0000, 0x00, NULL},
                            [477] = {0x0000, 0x00, NULL},
                            [478] = {0x0000, 0x00, NULL},
                            [479] = {0x0000, 0x00, NULL},
                            [480] = {0x0000, 0x00, NULL},
                            [481] = {0x0000, 0x00, NULL},
                            [482] = {0x0000, 0x00, NULL},
                            [483] = {0x0000, 0x00, NULL},
                            [484] = {0x0000, 0x00, NULL},
                            [485] = {0x0000, 0x00, NULL},
                            [486] = {0x0000, 0x00, NULL},
                            [487] = {0x0000, 0x00, NULL},
                            [488] = {0x0000, 0x00, NULL},
                            [489] = {0x0000, 0x00, NULL},
                            [490] = {0x0000, 0x00, NULL},
                            [491] = {0x0000, 0x00, NULL},
                            [492] = {0x0000, 0x00, NULL},
                            [493] = {0x0000, 0x00, NULL},
                            [494] = {0x0000, 0x00, NULL},
                            [495] = {0x0000, 0x00, NULL},
                            [496] = {0x0000, 0x00, NULL},
                            [497] = {0x0000, 0x00, NULL},
                            [498] = {0x0000, 0x00, NULL},
                            [499] = {0x0000, 0x00, NULL},
                            [CDF_EVDATA01_DAT] = {0x0506, 0x01, gEvtEntityData},
                            [CDF_EVDATA02_DAT] = {0x0507, 0x01, gEvtEntityData},
                            [CDF_EVDATA04_DAT] = {0x0509, 0x02, gEvtEntityData},
                            [CDF_EVDATA07_DAT_2] = {0x0510, 0x01, gEvtEntityData},
                            [CDF_EVDATA08_DAT] = {0x0511, 0x01, gEvtEntityData},
                            [CDF_EVDATA09_DAT] = {0x0512, 0x01, gEvtEntityData},
                            [CDF_EVDATA10_DAT] = {0x0513, 0x01, gEvtEntityData},
                            [CDF_EVDATA11_DAT] = {0x0514, 0x01, gEvtEntityData},
                            [CDF_EVDATA12_DAT] = {0x0515, 0x01, gEvtEntityData},
                            [CDF_EVDATA13_DAT] = {0x0516, 0x01, gEvtEntityData},
                            [CDF_EVDATA15_DAT] = {0x0518, 0x01, gEvtEntityData},
                            [CDF_EVDATA17_DAT] = {0x051a, 0x01, gEvtEntityData},
                            [CDF_EVDATA18_DAT] = {0x051b, 0x01, gEvtEntityData},
                            [CDF_EVDATA19_DAT] = {0x051c, 0x01, gEvtEntityData},
                            [CDF_EVDATA21_DAT] = {0x051f, 0x01, gEvtEntityData},
                            [CDF_EVDATA22_DAT] = {0x0520, 0x01, gEvtEntityData},
                            [CDF_EVDATA24_DAT] = {0x0522, 0x01, gEvtEntityData},
                            [CDF_EVDATA25_DAT] = {0x0523, 0x01, gEvtEntityData},
                            [CDF_EVDATA26_DAT_2] = {0x0524, 0x01, gEvtEntityData},
                            [CDF_EVDATA27_DAT] = {0x0525, 0x01, gEvtEntityData},
                            [CDF_EVDATA28_DAT] = {0x0526, 0x01, gEvtEntityData},
                            [CDF_EVDATA29_DAT] = {0x0527, 0x01, gEvtEntityData},
                            [CDF_EVDATA30_DAT] = {0x0528, 0x02, gEvtEntityData},
                            [CDF_EVDATA31_DAT] = {0x052a, 0x01, gEvtEntityData},
                            [CDF_EVDATA32_DAT] = {0x052b, 0x01, gEvtEntityData},
                            [CDF_EVDATA34_DAT] = {0x052d, 0x02, gEvtEntityData},
                            [CDF_EVDATA35_DAT] = {0x052f, 0x01, gEvtEntityData},
                            [CDF_EVDATA36_DAT] = {0x0530, 0x01, gEvtEntityData},
                            [CDF_EVDATA38_DAT] = {0x0532, 0x01, gEvtEntityData},
                            [CDF_EVDATA39_DAT] = {0x0533, 0x01, gEvtEntityData},
                            [CDF_EVDATA40_DAT] = {0x0534, 0x01, gEvtEntityData},
                            [CDF_EVDATA41_DAT] = {0x0535, 0x01, gEvtEntityData},
                            [CDF_EVDATA43_DAT] = {0x0537, 0x01, gEvtEntityData},
                            [CDF_EVDATA46_DAT] = {0x053b, 0x01, gEvtEntityData},
                            [CDF_EVDATA48_DAT] = {0x053e, 0x01, gEvtEntityData},
                            [CDF_EVDATA49_DAT] = {0x053f, 0x01, gEvtEntityData},
                            [CDF_EVDATA50_DAT] = {0x0540, 0x01, gEvtEntityData},
                            [CDF_EVDATA52_DAT] = {0x0542, 0x01, gEvtEntityData},
                            [CDF_EVDATA53_DAT] = {0x0543, 0x01, gEvtEntityData},
                            [CDF_EVDATA55_DAT] = {0x0545, 0x02, gEvtEntityData},
                            [CDF_EVDATA56_DAT] = {0x0547, 0x01, gEvtEntityData},
                            [CDF_EVDATA60_DAT] = {0x054a, 0x01, gEvtEntityData},
                            [CDF_EVDATA61_DAT] = {0x054b, 0x01, gEvtEntityData},
                            [CDF_EVDATA63_DAT] = {0x054d, 0x01, gEvtEntityData},
                            [CDF_EVDATA64_DAT] = {0x054e, 0x01, gEvtEntityData},
                            [CDF_EVDATA65_DAT] = {0x054f, 0x01, gEvtEntityData},
                            [CDF_EVDATA66_DAT] = {0x0550, 0x01, gEvtEntityData},
                            [CDF_EVDATA68_DAT] = {0x0552, 0x01, gEvtEntityData},
                            [CDF_EVDATA69_DAT] = {0x0553, 0x01, gEvtEntityData},
                            [CDF_EVDATA70_DAT] = {0x0554, 0x01, gEvtEntityData},
                            [CDF_EVDATA71_DAT] = {0x0555, 0x01, gEvtEntityData},
                            [CDF_EVDATA72_DAT] = {0x0556, 0x01, gEvtEntityData},
                            [CDF_EVDATA73_DAT] = {0x0557, 0x01, gEvtEntityData},
                            [CDF_EVDATA74_DAT] = {0x0558, 0x01, gEvtEntityData},
                            [CDF_EVDATA75_DAT] = {0x0559, 0x01, gEvtEntityData},
                            [CDF_EVDATA76_DAT] = {0x055a, 0x01, gEvtEntityData},
                            [CDF_EVDATA77_DAT] = {0x055b, 0x01, gEvtEntityData},
                            [CDF_EVDATA78_DAT] = {0x055c, 0x01, gEvtEntityData},
                            [CDF_EVDATA79_DAT] = {0x055d, 0x01, gEvtEntityData},
                            [CDF_EVDATA80_DAT] = {0x055e, 0x01, gEvtEntityData},
                            [CDF_EVDATA81_DAT] = {0x055f, 0x01, gEvtEntityData},
                            [CDF_EVDATA82_DAT] = {0x0560, 0x01, gEvtEntityData},
                            [CDF_EVDATA83_DAT] = {0x0561, 0x02, gEvtEntityData},
                            [CDF_EVDATA84_DAT] = {0x0563, 0x01, gEvtEntityData},
                            [CDF_EVDATA85_DAT] = {0x0564, 0x01, gEvtEntityData},
                            [CDF_EVDATA86_DAT] = {0x0565, 0x01, gEvtEntityData},
                            [CDF_EVDATA87_DAT] = {0x0566, 0x01, gEvtEntityData},
                            [CDF_EVDATA88_DAT] = {0x0567, 0x01, gEvtEntityData},
                            [CDF_EVDATA89_DAT] = {0x0568, 0x01, gEvtEntityData},
                            [CDF_EVDATA90_DAT] = {0x0569, 0x01, gEvtEntityData},
                            [CDF_EVDATA91_DAT] = {0x056a, 0x01, gEvtEntityData},
                            [CDF_EVDATA92_DAT] = {0x056b, 0x01, gEvtEntityData},
                            [CDF_EVDATA93_DAT] = {0x056c, 0x01, gEvtEntityData},
                            [CDF_EVDATA94_DAT] = {0x056d, 0x01, gEvtEntityData},
                            [574] = {0x0000, 0x00, NULL},
                            [575] = {0x0000, 0x00, NULL},
                            [576] = {0x0000, 0x00, NULL},
                            [577] = {0x0000, 0x00, NULL},
                            [578] = {0x0000, 0x00, NULL},
                            [579] = {0x0000, 0x00, NULL},
                            [580] = {0x0000, 0x00, NULL},
                            [581] = {0x0000, 0x00, NULL},
                            [582] = {0x0000, 0x00, NULL},
                            [583] = {0x0000, 0x00, NULL},
                            [584] = {0x0000, 0x00, NULL},
                            [585] = {0x0000, 0x00, NULL},
                            [586] = {0x0000, 0x00, NULL},
                            [587] = {0x0000, 0x00, NULL},
                            [588] = {0x0000, 0x00, NULL},
                            [589] = {0x0000, 0x00, NULL},
                            [590] = {0x0000, 0x00, NULL},
                            [591] = {0x0000, 0x00, NULL},
                            [592] = {0x0000, 0x00, NULL},
                            [593] = {0x0000, 0x00, NULL},
                            [594] = {0x0000, 0x00, NULL},
                            [595] = {0x0000, 0x00, NULL},
                            [596] = {0x0000, 0x00, NULL},
                            [597] = {0x0000, 0x00, NULL},
                            [598] = {0x0000, 0x00, NULL},
                            [599] = {0x0000, 0x00, NULL},
                            [CDF_B_TXT00_DAT] = {0x01a7, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT01_DAT] = {0x01a8, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT02_DAT] = {0x01a9, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT03_DAT] = {0x01aa, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT04_DAT] = {0x01ab, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT05_DAT] = {0x01ac, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT06_DAT] = {0x01ad, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT07_DAT] = {0x01ae, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT08_DAT] = {0x01af, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT09_DAT] = {0x01b0, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT10_DAT] = {0x01b1, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT11_DAT] = {0x01b2, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT12_DAT] = {0x01b3, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT13_DAT] = {0x01b4, 0x02, gScratch1_801317c0},
                            [CDF_B_TXT14_DAT] = {0x01b6, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT15_DAT] = {0x01b7, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT16_DAT] = {0x01b8, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT17_DAT] = {0x01b9, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT18_DAT] = {0x01ba, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT19_DAT] = {0x01bb, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT20_DAT] = {0x01bc, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT21_DAT] = {0x01bd, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT22_DAT] = {0x01be, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT23_DAT] = {0x01bf, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT24_DAT] = {0x01c0, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT25_DAT] = {0x01c1, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT26_DAT] = {0x01c2, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT27_DAT] = {0x01c3, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT28_DAT] = {0x01c4, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT29_DAT] = {0x01c5, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT30_DAT] = {0x01c6, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT31_DAT] = {0x01c7, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT32_DAT] = {0x01c8, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT33_DAT] = {0x01c9, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT34_DAT] = {0x01ca, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT35_DAT] = {0x01cb, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT36_DAT] = {0x01cc, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT37_DAT] = {0x01cd, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT38_DAT] = {0x01ce, 0x02, gScratch1_801317c0},
                            [CDF_B_TXT39_DAT] = {0x01d0, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT40_DAT] = {0x01d1, 0x02, gScratch1_801317c0},
                            [CDF_B_TXT41_DAT] = {0x01d3, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT42_DAT] = {0x01d4, 0x01, gScratch1_801317c0},
                            [CDF_B_TXT43_DAT] = {0x01d5, 0x01, gScratch1_801317c0},
                            [CDF_TOWN_T_DAT] = {0x1b2e, 0x03, gScratch1_801317c0},
                            [CDF_TENS_T_DAT] = {0x1b2c, 0x02, gScratch1_801317c0},
                            [CDF_COL_DAT_DAT] = {0x01d6, 0x04, gScratch1_801317c0},
                            [CDF_UNIT_51_DAT] = {0x2db7, 0x34, gScratch3_80180210},
                            [CDF_UNIT_52_DAT] = {0x2deb, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_53_DAT] = {0x2e1a, 0x29, gScratch3_80180210},
                            [CDF_UNIT_54_DAT] = {0x2e43, 0x3e, gScratch3_80180210},
                            [CDF_UNIT_55_DAT] = {0x2e81, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_56_DAT] = {0x2eae, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_57_DAT] = {0x2eda, 0x34, gScratch3_80180210},
                            [CDF_UNIT_58_DAT] = {0x2f0e, 0x45, gScratch3_80180210},
                            [CDF_UNIT_59_DAT] = {0x2f53, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_5A_DAT] = {0x2f7f, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_5B_DAT] = {0x2faa, 0x4f, gScratch3_80180210},
                            [CDF_UNIT_5C_DAT] = {0x2ff9, 0x33, gScratch3_80180210},
                            [CDF_UNIT_5D_DAT] = {0x302c, 0x34, gScratch3_80180210},
                            [CDF_UNIT_5E_DAT] = {0x3060, 0x29, gScratch3_80180210},
                            [CDF_UNIT_5F_DAT] = {0x3089, 0x32, gScratch3_80180210},
                            [CDF_UNIT_60_DAT] = {0x30bb, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_61_DAT] = {0x30e6, 0x3c, gScratch3_80180210},
                            [CDF_UNIT_62_DAT] = {0x3122, 0x31, gScratch3_80180210},
                            [CDF_UNIT_63_DAT] = {0x3153, 0x41, gScratch3_80180210},
                            [CDF_UNIT_64_DAT] = {0x3194, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_65_DAT] = {0x31c3, 0x45, gScratch3_80180210},
                            [CDF_UNIT_66_DAT] = {0x3208, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_67_DAT] = {0x3234, 0x2c, gScratch3_80180210},
                            [CDF_UNIT_68_DAT] = {0x3260, 0x36, gScratch3_80180210},
                            [CDF_UNIT_69_DAT] = {0x3296, 0x4b, gScratch3_80180210},
                            [CDF_UNIT_6A_DAT] = {0x32e1, 0x2b, gScratch3_80180210},
                            [CDF_UNIT_6B_DAT] = {0x330c, 0x40, gScratch3_80180210},
                            [CDF_UNIT_6C_DAT] = {0x334c, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_6D_DAT] = {0x337b, 0x4b, gScratch3_80180210},
                            [CDF_UNIT_6E_DAT] = {0x33c6, 0x2a, gScratch3_80180210},
                            [CDF_UNIT_6F_DAT] = {0x33f0, 0x30, gScratch3_80180210},
                            [CDF_UNIT_70_DAT] = {0x3420, 0x44, gScratch3_80180210},
                            [CDF_UNIT_71_DAT] = {0x3464, 0x2a, gScratch3_80180210},
                            [CDF_UNIT_72_DAT] = {0x348e, 0x3d, gScratch3_80180210},
                            [CDF_UNIT_73_DAT] = {0x34cb, 0x2a, gScratch3_80180210},
                            [CDF_UNIT_74_DAT] = {0x34f5, 0x38, gScratch3_80180210},
                            [CDF_UNIT_75_DAT] = {0x352d, 0x2e, gScratch3_80180210},
                            [CDF_UNIT_76_DAT] = {0x355b, 0x47, gScratch3_80180210},
                            [CDF_UNIT_77_DAT] = {0x35a2, 0x36, gScratch3_80180210},
                            [CDF_UNIT_78_DAT] = {0x35d8, 0x33, gScratch3_80180210},
                            [CDF_UNIT_79_DAT] = {0x360b, 0x30, gScratch3_80180210},
                            [CDF_UNIT_7A_DAT] = {0x363b, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_7B_DAT] = {0x366a, 0x29, gScratch3_80180210},
                            [CDF_UNIT_7C_DAT] = {0x3693, 0x47, gScratch3_80180210},
                            [CDF_UNIT_7D_DAT] = {0x36da, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_7E_DAT] = {0x3709, 0x49, gScratch3_80180210},
                            [CDF_UNIT_7F_DAT] = {0x3752, 0x2f, gScratch3_80180210},
                            [CDF_UNIT_80_DAT] = {0x3781, 0x2d, gScratch3_80180210},
                            [CDF_UNIT_81_DAT] = {0x37ae, 0x2a, gScratch3_80180210},
                            [CDF_UNIT_82_DAT] = {0x37d8, 0x29, gScratch3_80180210},
                            [CDF_UNIT_83_DAT] = {0x3801, 0x44, gScratch3_80180210},
                            [CDF_UNIT_84_DAT] = {0x3845, 0x4e, gScratch3_80180210},
                            [CDF_UNIT_85_DAT] = {0x3893, 0x34, gScratch3_80180210},
                            [CDF_UNIT_86_DAT] = {0x38c7, 0x3a, gScratch3_80180210},
                            [CDF_UNIT_87_DAT] = {0x3901, 0x40, gScratch3_80180210},
                            [CDF_UNIT_88_DAT] = {0x3941, 0x4b, gScratch3_80180210},
                            [CDF_UNIT_89_DAT] = {0x398c, 0x35, gScratch3_80180210},
                            [CDF_UNIT_8A_DAT] = {0x39c1, 0x45, gScratch3_80180210},
                            [CDF_UNIT_8B_DAT] = {0x3a06, 0x4b, gScratch3_80180210},
                            [CDF_UNIT_8C_DAT] = {0x3a51, 0x3f, gScratch3_80180210},
                            [CDF_UNIT_8D_DAT] = {0x3a90, 0x4d, gScratch3_80180210},
                            [CDF_UNIT_8E_DAT] = {0x3add, 0x50, gScratch3_80180210},
                            [CDF_UNIT_8F_DAT] = {0x3b2d, 0x33, gScratch3_80180210},
                            [710] = {0x0000, 0x00, gScratch3_80180210},
                            [CDF_M_SHOW_PRS] = {0x132e, 0x0f, gScratch1_801317c0}};

s32 s_frameNum_80123264;
s32 s_totalFrames_80123268;
s32 s_movieFinished_8012326c;

void CDInit(void) {
   CdInit();
   gCdLoader.state = 99;
   gVabLoader.state = 99;
   gSeqLoader.state = 99;
}

void LoadCdFile(s32 cdfIdx, s32 showLoadingScreen) {
   extern TILE s_blackScreen_80123918;
   Object *nowLoading;
   Object *logo;
   Object *faerie;
   s32 faerieFrameTimer;
   s32 faerieFrameToggle;

   gCdLoader.cdf = cdfIdx;
   gCdLoader.loadingVabBody = 0;
   gCdLoader.state = 0;
   gCdLoader.stalledCounter = 0;
   gCdLoader.sectorsToRead = gCdFiles[cdfIdx].sectorCt;
   gCdLoader.sector = gCdFiles[cdfIdx].startingSector;

   if (gState.suppressLoadingScreen || !showLoadingScreen) {
      while (GetCdFileLoadStatus() != 0) {
         ContinueLoadingCdFile();
#ifdef PC_PORT
         /* JP retail spins here WITHOUT VSync -- fine on hardware, where the sequencer runs off
          * a timer interrupt, but the port drives its SEQ tick + SPU render pump from VSync():
          * the tight spin starved audio for the whole simulated read window, audibly cutting
          * town music on every sub-area load (shop/tavern/dojo). The US build's own version of
          * this loop ADDS exactly this VSync(0) (src/core/cd.c) -- Konami's localization-era
          * change -- so the gated line reproduces shipped US behaviour, not an invention.
          * Diagnosed 2026-08-21 via VH_SEQ_LOG tick-gap timestamps + VH_CD_LOG read clusters. */
         VSync(0);
#endif
      }
   } else {
      SetTile(&s_blackScreen_80123918);
      s_blackScreen_80123918.r0 = s_blackScreen_80123918.g0 = s_blackScreen_80123918.b0 = 0;
      s_blackScreen_80123918.x0 = s_blackScreen_80123918.y0 = 0;
      s_blackScreen_80123918.w = SCREEN_WIDTH;
      s_blackScreen_80123918.h = SCREEN_HEIGHT;

      nowLoading = Obj_GetUnused();
      nowLoading->functionIndex = OBJF_NOOP;
      nowLoading->d.sprite.gfxIdx = GFX_NOW_LOADING;
      nowLoading->x1.n = 120;
      nowLoading->y1.n = 95;
      nowLoading->x3.n = nowLoading->x1.n + 80;
      nowLoading->y3.n = nowLoading->y1.n + 50;

      logo = Obj_GetUnused();
      logo->functionIndex = OBJF_NOOP;
      logo->d.sprite.gfxIdx = GFX_VANDAL_HEARTS;
      logo->x1.n = 156;
      logo->y1.n = 184;
      logo->x3.n = logo->x1.n + 128;
      logo->y3.n = logo->y1.n + 32;

      faerie = Obj_GetUnused();
      faerie->functionIndex = OBJF_NOOP;
      faerie->x1.n = 24;
      faerie->y1.n = 192;
      faerie->x3.n = faerie->x1.n + 24;
      faerie->y3.n = faerie->y1.n + 24;
      faerieFrameTimer = 0;
      faerieFrameToggle = 0;
      faerie->d.sprite.gfxIdx = GFX_FAERIE_1;

      do {
         UpdateAudio();
         rand();
         if (gState.vsyncMode == 2) {
            gState.frameCounter += 2;
         } else {
            gState.frameCounter += 1;
         }
         gState.unitMarkerSpin += 0x20;
         gOscillation = (gOscillation + 0x100) & 0xfff;
         gGridColorOscillation = rcos(gOscillation) * 100 >> 12;
         gGridColorOscillation += 155U;
         gQuadIndex = 0;
         UpdateInput();
         gGraphicsPtr =
             (gGraphicsPtr == &gGraphicBuffers[0]) ? &gGraphicBuffers[1] : &gGraphicBuffers[0];
         ClearOTag(gGraphicsPtr->ot, OT_SIZE);
         if (++faerieFrameTimer > 4) {
            faerieFrameToggle = !faerieFrameToggle;
            faerie->d.sprite.gfxIdx = GFX_FAERIE_1 + faerieFrameToggle;
            faerieFrameTimer = 0;
         }
         AddObjPrim_Gui(gGraphicsPtr->ot, nowLoading);
         AddObjPrim_Gui(gGraphicsPtr->ot, logo);
         AddObjPrim_Gui(gGraphicsPtr->ot, faerie);
         AddPrim(&gGraphicsPtr->ot[OT_SIZE - 1], &s_blackScreen_80123918);
         DrawSync(0);
         VSync(0);
         PutDrawEnv(&gGraphicsPtr->drawEnv);
         PutDispEnv(&gGraphicsPtr->dispEnv);
         DrawOTag(gGraphicsPtr->ot);
         ContinueLoadingCdFile();
      } while (GetCdFileLoadStatus() != 0);

      faerie->functionIndex = OBJF_NULL;
      logo->functionIndex = OBJF_NULL;
      nowLoading->functionIndex = OBJF_NULL;
   }
}

void PrepareToBeginLoadingVabBody(s32 cdfIdx, s32 maxSectors) {
   gCdLoader.cdf = cdfIdx;
   gCdLoader.loadingVabBody = 1;
   gCdLoader.state = 0;
   gCdLoader.stalledCounter = 0;
   gCdLoader.sector = gCdFiles[cdfIdx].startingSector;
   gCdLoader.sectorsRead = 0;
   gCdLoader.sectorsToRead = MIN(gCdFiles[cdfIdx].sectorCt, maxSectors);
}

void PrepareToResumeLoadingVabBody(void) {
   s32 remaining;

   gCdLoader.state = 0;
   gCdLoader.sector += gCdLoader.sectorsToRead;
   remaining = gCdFiles[gCdLoader.cdf].sectorCt - gCdLoader.sectorsRead;
   gCdLoader.sectorsToRead = MIN(remaining, gCdLoader.sectorsToRead);
}


void ContinueLoadingCdFile(void) {
   s32 result;
   s32 originalState = gCdLoader.state;

   switch (gCdLoader.state) {
   case 0:
      CdIntToPos(gCdLoader.sector, &gCdLoader.location);
      gCdLoader.state++;

   // fallthrough
   case 1:
      if (CdControl(CdlSetloc, &gCdLoader.location, NULL) == 0) {
         break;
      }

      gCdLoader.state++;

   // fallthrough
   case 2:
      // The read carries CdlModeSpeed itself, so there is no separate CdlSetmode step.
      CdRead(gCdLoader.sectorsToRead, gCdFiles[gCdLoader.cdf].bufferPtr, CdlModeSpeed);
      gCdLoader.state++;

   // fallthrough
   case 3:
      result = CdReadSync(1, NULL);
      if (result > 0) {
         // Still some sectors remaining
         break;
      }
      if (result < 0) {
         // -1: Error occurred
         CdControl(CdlReset, NULL, NULL);
         gCdLoader.state = 1;
         break;
      }

      // 0: Completed
      gCdLoader.state++;

   // fallthrough
   case 4:
      if (gCdLoader.loadingVabBody) {
         gCdLoader.sectorsRead += gCdLoader.sectorsToRead;

         if (gCdLoader.sectorsRead >= gCdFiles[gCdLoader.cdf].sectorCt) {
            gCdLoader.state = 99;
         } else {
            gCdLoader.state = 98;
         }
      } else {
         gCdLoader.state = 99;
      }
      break;

   case 98:
      break;

   case 99:
      break;
   }

   if (gCdLoader.state < 3 && gCdLoader.state == originalState &&
       (++gCdLoader.stalledCounter > 60)) {
      CdControlB(CdlNop, NULL, NULL);
      CdControlB(CdlReset, NULL, NULL);
      gCdLoader.state = 1;
      gCdLoader.stalledCounter = 0;
   }
   ContinueLoadingVab();
   ContinueLoadingSeq();
}

s32 GetCdFileLoadStatus(void) {
   if (gCdLoader.state == 99) {
      return 0;
   }
   if (gCdLoader.state == 98) {
      return 2;
   }
   return 1;
}

s32 LoadSoundSet(u32 setIdx) {
   static s16 prevSetIdx = -1;

   if (setIdx > 12) {
      return 0;
   }

   if (gSoundSets[setIdx].vabId == 2 && prevSetIdx == setIdx) {
      return 0;
   }

   prevSetIdx = setIdx;
   gVabLoader.state = 0;
   gVabLoader.vabId = gSoundSets[setIdx].vabId;
   gVabLoader.headerCdf = gSoundSets[setIdx].cdfVabHeader;
   gVabLoader.bodyCdf = gSoundSets[setIdx].cdfVabBody;
   SsVabClose(gVabLoader.vabId);
   gCdFiles[gVabLoader.headerCdf].bufferPtr = gSoundSets[setIdx].bufferPtr;
   LoadCdFile(gVabLoader.headerCdf, 1);

   return 1;
}

void ContinueLoadingVab(void) {
   switch (gVabLoader.state) {
   case 0:
      if (GetCdFileLoadStatus() != 0) {
         return;
      }
      gVabLoader.state++;

   // fallthrough
   case 1:
      SsVabOpenHeadSticky(gCdFiles[gVabLoader.headerCdf].bufferPtr, gVabLoader.vabId,
                          gVabSoundBufferAddr[gVabLoader.vabId]);
      gVabLoader.state++;

   // fallthrough
   case 2:
      PrepareToBeginLoadingVabBody(gVabLoader.bodyCdf, 90);
      gVabLoader.state++;

   // fallthrough
   case 3:
      if (GetCdFileLoadStatus() == 1) {
         return;
      }

      gVabLoader.bodyTransferResult =
          SsVabTransBodyPartly(gCdFiles[gVabLoader.bodyCdf].bufferPtr, 90 * 2048, gVabLoader.vabId);
      gVabLoader.state++;

   // fallthrough
   case 4:
      if (SsVabTransCompleted(0) != 1) {
         return;
      }
      gVabLoader.state++;

   // fallthrough
   case 5:
      if (gVabLoader.bodyTransferResult == -2) {
         // -2: Incomplete
         PrepareToResumeLoadingVabBody();
         gVabLoader.state = 3;
      } else if (gVabLoader.bodyTransferResult == -1) {
         //-1: Transfer failed
         gVabLoader.state = 3;
      } else {
         gVabLoader.state = 99;
      }
      break;

   case 99:
      break;
   }
}

void FinishLoadingVab(void) {
   while (gVabLoader.state != 99) {
      ContinueLoadingCdFile();
   }
}

void LoadSeqSet(u32 setIdx) {
   static s16 prevSetIdx = -1;

   if (setIdx < 38 && prevSetIdx != setIdx) {
      PerformAudioCommand(AUDIO_CMD_STOP_SEQ);
      SetSeqDataPtr(&gSeqData);
      prevSetIdx = setIdx;
      gSeqLoader.state = 0;
      gSeqLoader.setIdx = setIdx;
      gSeqLoader.cdf = gSeqSets[setIdx].cdf;
      gCdFiles[gSeqLoader.cdf].bufferPtr = gSeqSets[setIdx].bufferPtr;
      LoadCdFile(gSeqLoader.cdf, 0);
   }
}

void ContinueLoadingSeq(void) {
   switch (gSeqLoader.state) {
   case 0:
      if (GetCdFileLoadStatus() != 0) {
         return;
      }
      gSeqLoader.state++;
   // fallthrough
   case 1:
      SetCurrentSeqSet(gSeqLoader.setIdx);
      gSeqLoader.state = 99;
   // fallthrough
   case 99:
      break;
   }
}

void FinishLoadingSeq(void) {
   while (gSeqLoader.state != 99) {
      ContinueLoadingCdFile();
   }
}

CdlLOC s_movieLocation_8012325c;

s32 PlayMovie(s32 sector, s32 frameCt, s32 is24bit) {
   extern void Movie_StrCallback(void);

   CdIntToPos(sector, &s_movieLocation_8012325c);
   s_frameNum_80123264 = 0;
   s_totalFrames_80123268 = frameCt;
   gMovieDecoder.mode = is24bit ? 3 : 2;

   Movie_InitDecoder();
   // Note: Requires tweaking fixupTextRelocs to also scan for HI16/LO16 (for function ptr)
   Movie_Init(&s_movieLocation_8012325c, 0xffffffff, Movie_StrCallback, 0);
   Movie_DecodeNextFrame();
   return 0;
}

void Movie_Reset(s32 frameCt) {
   //?
   s_frameNum_80123264 = 0;
   s_totalFrames_80123268 = frameCt;
   s_movieFinished_8012326c = 0;
}

s32 Movie_PlayNextFrame(void) {
   DecDCTin(gMovieDecoder.vlcBufferPtrs[gMovieDecoder.vlcBufferIdx], gMovieDecoder.mode);
   DecDCTout(gMovieDecoder.imgBufferPtr, gMovieDecoder.slice.w * gMovieDecoder.slice.h / 2);

   if (Movie_DecodeNextFrame() == 0) {
      Movie_SyncFrame();
      return s_movieFinished_8012326c;
   }

   return 1;
}

void Movie_FinishNB(void) {
   DecDCToutCallback(NULL);
   StUnSetRing();
   sMovieSectorHeader = NULL;
}

void Movie_Finish(void) {
   u8 unused[8];

   DecDCToutCallback(NULL);
   StUnSetRing();
   while (CdControlB(CdlPause, NULL, NULL) == 0) {
      // Nothing
   }
   sMovieSectorHeader = NULL;
}

#ifdef PC_PORT
/* Overlay RETURN TO TITLE: tear down an in-flight movie exactly like the START skip (mute
 * serial A, Movie_Finish -- whose CdlPause the PC backend uses to close the frame overlay /
 * HD video / subtitles). No-op when no stream is armed. Same gated twin as the US tree
 * (src/core/cd.c), where the full rationale lives. */
void Movie_AbortForReturnToTitle(void) {
   extern void SsSetSerialVol(char, short, short);   /* PsyQ libsnd; cd.c doesn't include it */
   if (sMovieSectorHeader == NULL) return;
   SsSetSerialVol(0 /* SS_SERIAL_A */, 0, 0);
   Movie_Finish();
}
#endif

u32 Movie_GetFrameNum(void) {
   if (sMovieSectorHeader == NULL) {
      return 0;
   } else {
      return sMovieSectorHeader->frameCount;
   }
}

void Movie_InitDecoder(void) {
   static void *vlcBufAddr1 = &gScratch3_80180210;
   // static void *vlcBufAddr2 = (void *)0x801a24f0; //&gScratch3_80180210[140000]; //FIXME (reloc)
   // static void *imgBufAddr = (void *)0x801c47d0;  //&gScratch3_80180210[280000]; //FIXME (reloc)
   static void *vlcBufAddr2 = &gScratch3_80180210[140000];
   static void *imgBufAddr = &gScratch3_80180210[280000];

   gMovieDecoder.vlcBufferIdx = 0;
   gMovieDecoder.bufferRectIdx = (gGraphicsPtr != &gGraphicBuffers[0]);
   gMovieDecoder.frameFinished = 0;
   gMovieDecoder.vlcBufferPtrs[0] = vlcBufAddr1;
   gMovieDecoder.vlcBufferPtrs[1] = vlcBufAddr2;
   gMovieDecoder.imgBufferPtr = imgBufAddr;
   gMovieDecoder.bufferRects[0].x = 0;
   gMovieDecoder.bufferRects[0].y = 16;
   gMovieDecoder.bufferRects[0].w = SCREEN_WIDTH * gMovieDecoder.mode / 2;
   gMovieDecoder.bufferRects[0].h = SCREEN_HEIGHT;
   gMovieDecoder.bufferRects[1].x = 0;
   gMovieDecoder.bufferRects[1].y = 272;
   gMovieDecoder.bufferRects[1].w = SCREEN_WIDTH * gMovieDecoder.mode / 2;
   gMovieDecoder.bufferRects[1].h = SCREEN_HEIGHT;
   gMovieDecoder.slice.x = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].x;
   gMovieDecoder.slice.y = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].y;
   gMovieDecoder.slice.w = 16 * gMovieDecoder.mode / 2;
   gMovieDecoder.slice.h = SCREEN_HEIGHT;
}

void Movie_Init(CdlLOC *location, u32 endFrame, void (*strCallback)(), void (*endCallback)()) {
#ifdef PC_PORT
   /* 0x801c9384 = &gScratch3_80180210[0x472c0] (same +0x472c0 offset as the US tree) --
    * the raw PSX address dangles on PC; use the relocated live buffer. */
   static void *ringBufAddr = &gScratch3_80180210[0x472c0];
#else
   static void *ringBufAddr = (void *)0x801c9384;
#endif

   DecDCTReset(0);
   s_movieFinished_8012326c = 0;
   DecDCToutCallback(strCallback);
   StSetRing(ringBufAddr, 0x20);
   StSetStream(gMovieDecoder.mode != 2, 1, endFrame, NULL, endCallback);
   Movie_Start(location);
}

void Movie_StrCallback(void) {
   extern u32 StCdIntrFlag;
   if (gMovieDecoder.mode == 3 && StCdIntrFlag != 0) {
      StCdInterrupt();
      StCdIntrFlag = 0;
   }
   LoadImage(&gMovieDecoder.slice, gMovieDecoder.imgBufferPtr);
   gMovieDecoder.slice.x += gMovieDecoder.slice.w;
   if (gMovieDecoder.slice.x < (gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].x +
                                gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].w)) {
      DecDCTout(gMovieDecoder.imgBufferPtr, gMovieDecoder.slice.w * gMovieDecoder.slice.h / 2);
   } else {
      gMovieDecoder.frameFinished = 1;
      gMovieDecoder.bufferRectIdx = !gMovieDecoder.bufferRectIdx;
      gMovieDecoder.slice.x = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].x;
      gMovieDecoder.slice.y = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].y;
   }
}

s32 Movie_DecodeNextFrame(void) {
   u32 *nextFrame;
   s32 tries = 1;

   while ((nextFrame = Movie_GetNextFrame()) == 0) {
      if (--tries == 0) {
         return -1;
      }
   }

   gMovieDecoder.vlcBufferIdx = !gMovieDecoder.vlcBufferIdx;
   DecDCTvlc(nextFrame, gMovieDecoder.vlcBufferPtrs[gMovieDecoder.vlcBufferIdx]);
   StFreeRing(nextFrame);
   return 0;
}

u32 *Movie_GetNextFrame(void) {
   u32 *addr;
   s32 tries = 0x100000;

   if (CdSync(1, NULL) & 0x10) {
      //? What flag is 0x10? Does CdSync report shell open?
      return NULL;
   }

   while (StGetNext(&addr, &sMovieSectorHeader) != 0) {
      if (--tries == 0) {
         return NULL;
      }
   }

   //? Can't make sense of this.
   if (addr[0] != sMovieSectorHeader->dummy1 || addr[1] != sMovieSectorHeader->dummy2) {
      StFreeRing(addr);
      return NULL;
   }

   if (sMovieSectorHeader->frameCount >= s_totalFrames_80123268) {
      s_movieFinished_8012326c = 1;
   }

   if (sMovieSectorHeader->frameCount < s_frameNum_80123264) {
      s_movieFinished_8012326c = 1;
   } else {
      s_frameNum_80123264 = sMovieSectorHeader->frameCount;
   }

   return addr;
}

void Movie_SyncFrame(void) {
   s32 tries = WAIT_TIME;

   while (!gMovieDecoder.frameFinished) {
      if (--tries == 0) {
         gMovieDecoder.frameFinished = 1;
         gMovieDecoder.bufferRectIdx = !gMovieDecoder.bufferRectIdx;
         gMovieDecoder.slice.x = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].x;
         gMovieDecoder.slice.y = gMovieDecoder.bufferRects[gMovieDecoder.bufferRectIdx].y;
      }
   }

   gMovieDecoder.frameFinished = 0;
}


void Movie_Start(CdlLOC *location) {
   s32 prevState;
   s32 result;
   s32 tries = 0;
   s32 state = 0;

   while (1) {
      prevState = state;

      switch (state) {
      case 0:
         if (CdControl(CdlNop, NULL, NULL) == 0) {
            break;
         }
         state++;

      // fallthrough
      case 1:
         result = CdSync(1, NULL);
         if (result & 0x10) { //?
            return;
         }
         if (result != CdlComplete) {
            break;
         }
         state++;

      // fallthrough
      case 2:
         if (CdControl(CdlSeekL, location, NULL) == 0) {
            break;
         }
         state++;

      // fallthrough
      case 3:
         // CdlModeStream | CdlModeSpeed | CdlModeRT computes 0x1e0, but the original binary
         // uses 0x1c0 here — matched to the literal value; true flag breakdown TBD
         if (CdRead2(0x1c0) != 0) {
            // 1: Success
            return;
         }
      } // switch (state)

      if (state == prevState) {
         if (++tries > 16) {
            return;
         }
      }
   }
}
