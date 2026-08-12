#include "common.h"
#include "audio.h"

s16 gSfxSlotSustained = 0;
s16 gSfxSlotToggled = 0;
u8 gAudioEnabled = 0;
void *gSeqDataPtr = NULL;
s16 gSeqSetIdx = 0;

s16 gVolumeCurve[128] = {
    0x0,  0x2,  0x4,  0x6,  0x8,  0xa,  0xc,  0xe,  0x10, 0x12, 0x14, 0x16, 0x18, 0x1a, 0x1c, 0x1e,
    0x20, 0x22, 0x24, 0x26, 0x28, 0x2a, 0x2c, 0x2e, 0x30, 0x32, 0x34, 0x36, 0x38, 0x3a, 0x3c, 0x3e,
    0x40, 0x41, 0x42, 0x43, 0x44, 0x45, 0x46, 0x47, 0x48, 0x49, 0x4a, 0x4b, 0x4c, 0x4d, 0x4e, 0x4f,
    0x50, 0x51, 0x52, 0x53, 0x54, 0x55, 0x56, 0x57, 0x58, 0x59, 0x5a, 0x5b, 0x5c, 0x5d, 0x5e, 0x5f,
    0x60, 0x60, 0x61, 0x61, 0x62, 0x62, 0x63, 0x63, 0x64, 0x64, 0x65, 0x65, 0x66, 0x66, 0x67, 0x67,
    0x68, 0x68, 0x69, 0x69, 0x6a, 0x6a, 0x6b, 0x6b, 0x6c, 0x6c, 0x6d, 0x6d, 0x6e, 0x6e, 0x6f, 0x6f,
    0x70, 0x70, 0x71, 0x71, 0x72, 0x72, 0x73, 0x73, 0x74, 0x74, 0x75, 0x75, 0x76, 0x76, 0x77, 0x77,
    0x78, 0x78, 0x79, 0x79, 0x7a, 0x7a, 0x7b, 0x7b, 0x7c, 0x7c, 0x7d, 0x7d, 0x7e, 0x7e, 0x7f, 0x7f};

u32 gVabSoundBufferAddr[4] = {0x1010, 0x2ed0, 0x43890, 0x43890};

s32 gSeqDataOffsets[39][8] = {
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x2520, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x65b0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x91c0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x52d0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x34d0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x1530, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x7860, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x7860, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x65b0, 0x8e40, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x9660, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x3840, 0x6870, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x24b0, 0x3180, 0x4190, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x24b0, 0x3180, 0x4190, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x1220, 0x4af0, 0x6fc0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x24b0, 0x3180, 0x4190, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x1790, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x1790, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x2a20, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x2a20, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0xb70, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x550, 0x2a20, 0x52b0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
};

s16 gSeqTaggedIDs[38][8] = {
    {0x0, 0x216, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x217, 0x218, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x202, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x203, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x204, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x205, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x206, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x207, 0x21d, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x208, 0x21f, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x209, 0x224, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20a, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20b, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20c, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20d, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x20e, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x20f, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x210, 0x21d, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x211, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x212, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x213, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x214, 0x21d, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x215, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x202, 0x21d, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x202, 0x21e, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x207, 0x21e, 0x21f, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20b, 0x21f, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x201, 0x20d, 0x220, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x21a, 0x21b, 0x21c, 0x221, 0x0, 0x0, 0x0},
    {0x0, 0x21a, 0x21b, 0x21c, 0x222, 0x0, 0x0, 0x0},
    {0x0, 0x21b, 0x220, 0x221, 0x21f, 0x0, 0x0, 0x0},
    {0x0, 0x21a, 0x21b, 0x21c, 0x223, 0x0, 0x0, 0x0},
    {0x0, 0x223, 0x21f, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x223, 0x21d, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x221, 0x222, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x221, 0x21f, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x224, 0x222, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x225, 0x0, 0x0, 0x0, 0x0, 0x0, 0x0},
    {0x0, 0x221, 0x21e, 0x222, 0x0, 0x0, 0x0, 0x0},
};

SeqProperties gSeqProperties[38] = {
    {0x7f, 0x3c, 0x0},  {0x6c, 0x30, 0x21}, {0x74, 0x3c, 0x31}, {0x72, 0x3c, 0x31},
    {0x76, 0x3c, 0x31}, {0x78, 0x3c, 0x31}, {0x78, 0x3c, 0x31}, {0x70, 0x3c, 0x31},
    {0x64, 0x3c, 0x31}, {0x74, 0x3c, 0x31}, {0x78, 0x3c, 0x31}, {0x74, 0x3c, 0x31},
    {0x78, 0x3c, 0x31}, {0x78, 0x3c, 0x31}, {0x7c, 0x5c, 0x21}, {0x7c, 0x5c, 0x21},
    {0x70, 0x54, 0x21}, {0x7f, 0x5c, 0x21}, {0x7c, 0x5c, 0x21}, {0x7f, 0x5c, 0x21},
    {0x58, 0x4c, 0x21}, {0x68, 0x44, 0x21}, {0x68, 0x40, 0x31}, {0x78, 0x3c, 0x31},
    {0x78, 0x48, 0x31}, {0x70, 0x2c, 0x30}, {0x7f, 0x3c, 0x21}, {0x7f, 0x3c, 0x21},
    {0x7f, 0x3c, 0x21}, {0x7f, 0x34, 0x21}, {0x7a, 0x3c, 0x21}, {0x7f, 0x3c, 0x21},
    {0x7f, 0x40, 0x21}, {0x7f, 0x38, 0x21}, {0x7f, 0x40, 0x21}, {0x7f, 0x40, 0x21},
    {0x78, 0x48, 0x21}, {0x7f, 0x44, 0x21},
};

SfxProperties gSfxProperties[269] = {
    {0, 0, 0, 0, SFX_ROLE_NONE, 0},         {2, 0, 26, 85, SFX_ROLE_NONE, 4},
    {2, 7, 86, 67, SFX_ROLE_NONE, 12},      {2, 0, 24, 90, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 83, SFX_ROLE_RELEASE, 0},    {2, 9, 97, 92, SFX_ROLE_NONE, 2},
    {2, 6, 79, 96, SFX_ROLE_NONE, 14},      {2, 6, 78, 95, SFX_ROLE_NONE, 12},
    {2, 2, 40, 77, SFX_ROLE_NONE, 0},       {2, 2, 42, 77, SFX_ROLE_NONE, 4},
    {2, 2, 41, 89, SFX_ROLE_NONE, 2},       {2, 7, 87, 80, SFX_ROLE_SUSTAIN, 14},
    {2, 7, 87, 56, SFX_ROLE_RELEASE, 14},   {2, 2, 40, 87, SFX_ROLE_NONE, 0},
    {2, 2, 42, 80, SFX_ROLE_NONE, 4},       {2, 4, 56, 78, SFX_ROLE_NONE, 0},
    {2, 0, 28, 82, SFX_ROLE_SUSTAIN, 8},    {2, 0, 28, 60, SFX_ROLE_RELEASE, 8},
    {2, 0, 26, 83, SFX_ROLE_NONE, 4},       {2, 8, 91, 97, SFX_ROLE_NONE, 6},
    {2, 4, 58, 66, SFX_ROLE_SUSTAIN, 4},    {2, 4, 58, 52, SFX_ROLE_RELEASE, 4},
    {2, 8, 92, 89, SFX_ROLE_NONE, 8},       {2, 4, 59, 95, SFX_ROLE_NONE, 6},
    {2, 0, 28, 85, SFX_ROLE_SUSTAIN, 8},    {2, 0, 28, 60, SFX_ROLE_RELEASE, 8},
    {2, 4, 60, 60, SFX_ROLE_SUSTAIN, 8},    {2, 4, 60, 48, SFX_ROLE_RELEASE, 8},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 2, 42, 85, SFX_ROLE_NONE, 4},       {2, 0, 26, 85, SFX_ROLE_NONE, 4},
    {2, 0, 26, 83, SFX_ROLE_NONE, 4},       {2, 1, 34, 89, SFX_ROLE_NONE, 4},
    {2, 0, 27, 69, SFX_ROLE_SUSTAIN, 6},    {2, 0, 27, 52, SFX_ROLE_RELEASE, 6},
    {2, 1, 37, 81, SFX_ROLE_NONE, 10},      {2, 2, 41, 90, SFX_ROLE_NONE, 2},
    {2, 9, 100, 115, SFX_ROLE_NONE, 8},     {2, 0, 26, 90, SFX_ROLE_NONE, 4},
    {2, 0, 26, 86, SFX_ROLE_NONE, 4},       {2, 8, 90, 95, SFX_ROLE_NONE, 4},
    {2, 0, 26, 85, SFX_ROLE_NONE, 4},       {2, 8, 90, 94, SFX_ROLE_NONE, 4},
    {2, 0, 29, 76, SFX_ROLE_SUSTAIN, 10},   {2, 0, 29, 56, SFX_ROLE_RELEASE, 10},
    {2, 2, 42, 90, SFX_ROLE_NONE, 4},       {2, 8, 95, 103, SFX_ROLE_NONE, 14},
    {2, 8, 90, 98, SFX_ROLE_NONE, 4},       {2, 8, 89, 82, SFX_ROLE_NONE, 2},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 89, SFX_ROLE_NONE, 6},
    {2, 9, 96, 86, SFX_ROLE_SUSTAIN, 0},    {2, 9, 96, 57, SFX_ROLE_RELEASE, 0},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 8, 95, 107, SFX_ROLE_NONE, 14},     {2, 3, 55, 64, SFX_ROLE_SUSTAIN, 14},
    {2, 3, 55, 52, SFX_ROLE_RELEASE, 14},   {2, 8, 88, 93, SFX_ROLE_NONE, 0},
    {2, 2, 40, 90, SFX_ROLE_NONE, 0},       {2, 8, 95, 97, SFX_ROLE_NONE, 14},
    {2, 0, 29, 76, SFX_ROLE_SUSTAIN, 10},   {2, 0, 29, 52, SFX_ROLE_RELEASE, 10},
    {2, 2, 42, 92, SFX_ROLE_NONE, 4},       {2, 0, 24, 87, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},    {2, 2, 42, 86, SFX_ROLE_NONE, 4},
    {2, 4, 61, 101, SFX_ROLE_NONE, 10},     {2, 1, 36, 89, SFX_ROLE_NONE, 8},
    {2, 1, 35, 85, SFX_ROLE_NONE, 6},       {2, 8, 92, 93, SFX_ROLE_NONE, 8},
    {2, 8, 90, 97, SFX_ROLE_NONE, 4},       {2, 0, 27, 67, SFX_ROLE_SUSTAIN, 6},
    {2, 0, 27, 52, SFX_ROLE_RELEASE, 6},    {2, 2, 40, 76, SFX_ROLE_NONE, 0},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 8, 92, 91, SFX_ROLE_NONE, 8},       {2, 8, 89, 85, SFX_ROLE_NONE, 2},
    {2, 8, 89, 86, SFX_ROLE_NONE, 2},       {2, 3, 50, 85, SFX_ROLE_SUSTAIN, 4},
    {2, 3, 50, 52, SFX_ROLE_RELEASE, 4},    {2, 2, 42, 90, SFX_ROLE_NONE, 4},
    {2, 8, 90, 96, SFX_ROLE_NONE, 4},       {2, 8, 91, 103, SFX_ROLE_NONE, 6},
    {2, 8, 90, 90, SFX_ROLE_NONE, 4},       {2, 7, 87, 80, SFX_ROLE_SUSTAIN, 14},
    {2, 7, 87, 52, SFX_ROLE_RELEASE, 14},   {2, 8, 92, 99, SFX_ROLE_NONE, 8},
    {2, 8, 90, 90, SFX_ROLE_NONE, 4},       {2, 1, 36, 89, SFX_ROLE_NONE, 8},
    {2, 1, 35, 85, SFX_ROLE_NONE, 6},       {2, 1, 36, 89, SFX_ROLE_NONE, 8},
    {2, 1, 35, 85, SFX_ROLE_NONE, 6},       {2, 0, 25, 99, SFX_ROLE_SUSTAIN, 2},
    {2, 0, 25, 59, SFX_ROLE_RELEASE, 2},    {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 8, 92, 93, SFX_ROLE_NONE, 8},       {2, 1, 34, 96, SFX_ROLE_NONE, 4},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 8, 92, 92, SFX_ROLE_NONE, 8},       {2, 8, 90, 99, SFX_ROLE_NONE, 4},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 2, 42, 84, SFX_ROLE_NONE, 4},       {2, 4, 56, 82, SFX_ROLE_NONE, 0},
    {2, 4, 62, 102, SFX_ROLE_NONE, 12},     {2, 2, 40, 82, SFX_ROLE_NONE, 0},
    {2, 2, 41, 94, SFX_ROLE_NONE, 2},       {2, 1, 34, 87, SFX_ROLE_NONE, 4},
    {2, 2, 42, 83, SFX_ROLE_NONE, 4},       {2, 2, 41, 86, SFX_ROLE_NONE, 2},
    {2, 2, 42, 88, SFX_ROLE_NONE, 4},       {2, 8, 95, 107, SFX_ROLE_NONE, 14},
    {2, 9, 99, 81, SFX_ROLE_SUSTAIN, 6},    {2, 9, 99, 60, SFX_ROLE_RELEASE, 6},
    {2, 2, 44, 86, SFX_ROLE_NONE, 8},       {2, 2, 43, 76, SFX_ROLE_NONE, 6},
    {2, 0, 28, 92, SFX_ROLE_SUSTAIN, 8},    {2, 0, 28, 56, SFX_ROLE_RELEASE, 8},
    {2, 8, 93, 106, SFX_ROLE_NONE, 10},     {2, 8, 94, 100, SFX_ROLE_NONE, 12},
    {2, 2, 42, 88, SFX_ROLE_NONE, 4},       {2, 8, 93, 110, SFX_ROLE_NONE, 10},
    {2, 2, 42, 91, SFX_ROLE_NONE, 4},       {2, 8, 95, 108, SFX_ROLE_NONE, 14},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 9, 98, 88, SFX_ROLE_SUSTAIN, 4},    {2, 9, 98, 85, SFX_ROLE_RELEASE, 4},
    {2, 4, 58, 78, SFX_ROLE_SUSTAIN, 4},    {2, 4, 58, 48, SFX_ROLE_RELEASE, 4},
    {2, 3, 52, 90, SFX_ROLE_NONE, 8},       {2, 3, 54, 86, SFX_ROLE_NONE, 12},
    {2, 3, 48, 88, SFX_ROLE_SUSTAIN, 0},    {2, 3, 48, 60, SFX_ROLE_RELEASE, 0},
    {2, 2, 47, 88, SFX_ROLE_NONE, 12},      {2, 9, 98, 86, SFX_ROLE_SUSTAIN, 4},
    {2, 9, 98, 56, SFX_ROLE_RELEASE, 4},    {2, 0, 26, 87, SFX_ROLE_NONE, 4},
    {2, 8, 93, 113, SFX_ROLE_NONE, 10},     {2, 0, 26, 89, SFX_ROLE_NONE, 4},
    {2, 8, 92, 102, SFX_ROLE_NONE, 8},      {2, 9, 103, 107, SFX_ROLE_NONE, 14},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 2, 42, 88, SFX_ROLE_NONE, 4},       {2, 2, 43, 78, SFX_ROLE_NONE, 6},
    {2, 1, 34, 92, SFX_ROLE_NONE, 4},       {2, 2, 43, 79, SFX_ROLE_NONE, 6},
    {2, 2, 45, 101, SFX_ROLE_NONE, 10},     {2, 3, 53, 93, SFX_ROLE_NONE, 10},
    {2, 0, 24, 91, SFX_ROLE_SUSTAIN, 0},    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},
    {2, 9, 101, 113, SFX_ROLE_SUSTAIN, 10}, {2, 9, 101, 76, SFX_ROLE_RELEASE, 10},
    {2, 8, 88, 92, SFX_ROLE_NONE, 0},       {2, 1, 34, 91, SFX_ROLE_NONE, 4},
    {2, 3, 51, 96, SFX_ROLE_NONE, 6},       {2, 10, 23, 89, SFX_ROLE_SUSTAIN, 0},
    {2, 10, 23, 64, SFX_ROLE_RELEASE, 0},   {2, 3, 53, 93, SFX_ROLE_NONE, 10},
    {2, 0, 26, 94, SFX_ROLE_NONE, 4},       {2, 6, 77, 99, SFX_ROLE_NONE, 10},
    {2, 3, 49, 82, SFX_ROLE_NONE, 2},       {2, 2, 43, 77, SFX_ROLE_NONE, 6},
    {2, 2, 41, 89, SFX_ROLE_NONE, 2},       {2, 2, 46, 91, SFX_ROLE_SUSTAIN, 14},
    {2, 2, 46, 64, SFX_ROLE_RELEASE, 14},   {2, 5, 71, 110, SFX_ROLE_NONE, 12},
    {2, 1, 36, 89, SFX_ROLE_NONE, 8},       {2, 1, 35, 85, SFX_ROLE_NONE, 6},
    {2, 1, 38, 109, SFX_ROLE_NONE, 12},     {2, 2, 42, 92, SFX_ROLE_NONE, 4},
    {2, 9, 102, 96, SFX_ROLE_NONE, 12},     {2, 1, 36, 89, SFX_ROLE_NONE, 8},
    {2, 1, 35, 85, SFX_ROLE_NONE, 6},       {2, 1, 36, 89, SFX_ROLE_NONE, 8},
    {2, 1, 35, 85, SFX_ROLE_NONE, 6},       {2, 2, 42, 93, SFX_ROLE_NONE, 4},
    {2, 3, 53, 94, SFX_ROLE_NONE, 10},      {2, 0, 26, 91, SFX_ROLE_NONE, 4},
    {2, 3, 51, 85, SFX_ROLE_NONE, 6},       {2, 3, 54, 84, SFX_ROLE_NONE, 12},
    {2, 8, 92, 99, SFX_ROLE_NONE, 8},       {2, 3, 54, 84, SFX_ROLE_NONE, 12},
    {2, 8, 92, 100, SFX_ROLE_NONE, 8},      {2, 1, 34, 92, SFX_ROLE_NONE, 4},
    {2, 3, 53, 89, SFX_ROLE_NONE, 10},      {2, 5, 70, 111, SFX_ROLE_NONE, 10},
    {2, 5, 67, 113, SFX_ROLE_NONE, 4},      {2, 6, 72, 61, SFX_ROLE_SUSTAIN, 0},
    {2, 6, 72, 47, SFX_ROLE_RELEASE, 0},    {2, 8, 92, 93, SFX_ROLE_NONE, 8},
    {2, 5, 69, 69, SFX_ROLE_SUSTAIN, 8},    {2, 5, 69, 52, SFX_ROLE_RELEASE, 8},
    {2, 5, 66, 101, SFX_ROLE_NONE, 2},      {2, 0, 26, 92, SFX_ROLE_NONE, 4},
    {2, 2, 40, 82, SFX_ROLE_NONE, 0},       {2, 2, 40, 82, SFX_ROLE_NONE, 0},
    {2, 5, 66, 106, SFX_ROLE_NONE, 2},      {2, 5, 71, 111, SFX_ROLE_NONE, 12},
    {2, 3, 48, 94, SFX_ROLE_SUSTAIN, 0},    {2, 3, 48, 56, SFX_ROLE_RELEASE, 0},
    {2, 2, 47, 82, SFX_ROLE_NONE, 12},      {2, 2, 43, 72, SFX_ROLE_NONE, 6},
    {2, 6, 75, 102, SFX_ROLE_NONE, 6},      {2, 6, 76, 96, SFX_ROLE_NONE, 8},
    {2, 6, 73, 93, SFX_ROLE_NONE, 2},       {2, 6, 74, 100, SFX_ROLE_NONE, 4},
    {2, 7, 80, 98, SFX_ROLE_NONE, 0},       {2, 8, 90, 103, SFX_ROLE_NONE, 4},
    {2, 8, 88, 89, SFX_ROLE_NONE, 0},       {2, 1, 33, 112, SFX_ROLE_NONE, 2},
    {2, 7, 84, 106, SFX_ROLE_NONE, 8},      {2, 7, 85, 111, SFX_ROLE_NONE, 10},
    {2, 1, 32, 101, SFX_ROLE_NONE, 0},      {2, 0, 31, 93, SFX_ROLE_NONE, 14},
    {2, 0, 30, 98, SFX_ROLE_NONE, 12},      {2, 5, 64, 118, SFX_ROLE_NONE, 0},
    {2, 4, 63, 55, SFX_ROLE_SUSTAIN, 14},   {2, 4, 63, 56, SFX_ROLE_RELEASE, 14},
    {2, 5, 68, 118, SFX_ROLE_NONE, 6},      {2, 7, 81, 108, SFX_ROLE_NONE, 2},
    {2, 7, 82, 119, SFX_ROLE_NONE, 4},      {2, 7, 83, 118, SFX_ROLE_NONE, 6},
    {0, 0, 24, 123, SFX_ROLE_NONE, 6},      {0, 0, 25, 92, SFX_ROLE_NONE, 0},
    {0, 0, 26, 102, SFX_ROLE_NONE, 2},      {0, 0, 27, 97, SFX_ROLE_NONE, 4},
    {2, 2, 43, 83, SFX_ROLE_NONE, 6},       {2, 0, 24, 56, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 36, 70, SFX_ROLE_SUSTAIN, 2},
    {2, 0, 36, 70, SFX_ROLE_RELEASE, 2},    {2, 0, 24, 56, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 24, 52, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 52, SFX_ROLE_RELEASE, 0},    {2, 0, 24, 56, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 24, 121, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 36, 68, SFX_ROLE_SUSTAIN, 2},
    {2, 0, 36, 68, SFX_ROLE_RELEASE, 2},    {2, 0, 48, 56, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 48, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 43, 64, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 43, 56, SFX_ROLE_RELEASE, 0},    {2, 0, 36, 80, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 36, 72, SFX_ROLE_RELEASE, 0},    {2, 10, 22, 68, SFX_ROLE_SUSTAIN, 2},
    {2, 10, 22, 68, SFX_ROLE_RELEASE, 2},   {2, 10, 21, 104, SFX_ROLE_SUSTAIN, 4},
    {2, 10, 21, 68, SFX_ROLE_RELEASE, 4},   {2, 0, 24, 72, SFX_ROLE_NONE, 0},
    {2, 0, 25, 72, SFX_ROLE_NONE, 2},       {2, 0, 24, 104, SFX_ROLE_SUSTAIN, 0},
    {2, 0, 24, 104, SFX_ROLE_RELEASE, 0},
};

XaProperties gXaProperties[188] = {
    {0x0, 0x0, 0x0, 0x0, 0x0, 0x0, XA_LOOP},
    {0x0, 0x1963, 0x1, 0x0, 0x1c, 0x0, XA_LOOP},
    {0x3f78, 0x1920, 0x2, 0x0, 0x30, 0x0, XA_LOOP},
    {0x7e48, 0x1553, 0x3, 0x0, 0x40, 0x0, XA_LOOP},
    {0xb398, 0x2d0, 0x4, 0x0, 0x70, 0x0, XA_NO_LOOP},
    {0xbaa0, 0x129, 0x5, 0x0, 0x48, 0x0, XA_NO_LOOP},
    {0xbd88, 0x149, 0x6, 0x0, 0x48, 0x0, XA_NO_LOOP},
    {0xc0c0, 0x12c, 0x7, 0x0, 0x48, 0x0, XA_NO_LOOP},
    {0xc3b0, 0x5c, 0x8, 0x0, 0x18, 0x0, XA_NO_LOOP},
    {0xc498, 0x66, 0x9, 0x0, 0x28, 0x0, XA_NO_LOOP},
    {0xc598, 0x60, 0xa, 0x0, 0x1c, 0x0, XA_NO_LOOP},
    {0xc688, 0x80, 0xb, 0x0, 0x1c, 0x0, XA_NO_LOOP},
    {0xc7c8, 0x80, 0xc, 0x0, 0x1c, 0x0, XA_NO_LOOP},
    {0xc908, 0x60, 0xd, 0x0, 0x30, 0x0, XA_NO_LOOP},
    {0xc9f8, 0xbc, 0xe, 0x0, 0x60, 0x0, XA_NO_LOOP},
    {0xcbd0, 0x113, 0xf, 0x0, 0x48, 0x0, XA_NO_LOOP},
    {0xce80, 0x30, 0x10, 0x0, 0x68, 0x0, XA_NO_LOOP},
    {0xcef8, 0x3766, 0x11, 0x0, 0xc, 0x0, XA_LOOP},
    {0x15978, 0x59, 0x12, 0x0, 0x66, 0x0, XA_NO_LOOP},
    {0x15a58, 0x80, 0x13, 0x0, 0x5e, 0x0, XA_NO_LOOP},
    {0x15b98, 0x39, 0x14, 0x0, 0x70, 0x0, XA_NO_LOOP},
    {0x0, 0x6e6, 0x1, 0x1, 0x4d, 0x0, XA_NO_LOOP},
    {0x1140, 0x3a2c, 0x2, 0x1, 0x30, 0x0, XA_LOOP},
    {0xa2b0, 0x40, 0x3, 0x1, 0x70, 0x0, XA_NO_LOOP},
    {0xa350, 0x34b3, 0x4, 0x1, 0xe, 0x0, XA_LOOP},
    {0x12710, 0x599, 0x5, 0x1, 0x40, 0x0, XA_NO_LOOP},
    {0x13510, 0x349, 0x6, 0x1, 0x7f, 0x0, XA_NO_LOOP},
    {0x13d48, 0x2a0, 0x7, 0x1, 0x70, 0x0, XA_NO_LOOP},
    {0x143d8, 0x49, 0x8, 0x1, 0x4c, 0x0, XA_NO_LOOP},
    {0x14490, 0x11c, 0x9, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x14758, 0x11c, 0xa, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x14a20, 0x11c, 0xb, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x14ce8, 0xa9c, 0xc, 0x1, 0x74, 0x0, XA_NO_LOOP},
    {0x16770, 0x53, 0xd, 0x1, 0x50, 0x0, XA_NO_LOOP},
    {0x16840, 0x106, 0xe, 0x1, 0x70, 0x0, XA_NO_LOOP},
    {0x16ad0, 0x1a9, 0xf, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x16ef8, 0xbc, 0x10, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x170d0, 0x1a0, 0x11, 0x1, 0x70, 0x0, XA_NO_LOOP},
    {0x174e0, 0x6c, 0x12, 0x1, 0x58, 0x0, XA_NO_LOOP},
    {0x175f0, 0xf0, 0x13, 0x1, 0x78, 0x0, XA_NO_LOOP},
    {0x17848, 0x99, 0x14, 0x1, 0x78, 0x0, XA_NO_LOOP},
    {0x179c8, 0x4b0, 0x15, 0x1, 0x78, 0x0, XA_NO_LOOP},
    {0x18580, 0xe3, 0x16, 0x1, 0x78, 0x0, XA_NO_LOOP},
    {0x187b8, 0x29, 0x17, 0x1, 0x58, 0x0, XA_NO_LOOP},
    {0x18820, 0x69, 0x18, 0x1, 0x68, 0x0, XA_NO_LOOP},
    {0x0, 0x3306, 0x1, 0x2, 0x10, 0x0, XA_LOOP},
    {0x7f90, 0x3426, 0x2, 0x2, 0x20, 0x0, XA_LOOP},
    {0x101f0, 0x3876, 0x3, 0x2, 0x74, 0x0, XA_NO_LOOP},
    {0x0, 0x39c3, 0x1, 0x3, 0x18, 0x0, XA_LOOP},
    {0x9068, 0x159, 0x2, 0x3, 0x7f, 0x0, XA_NO_LOOP},
    {0x93c8, 0x3633, 0x3, 0x3, 0x30, 0x0, XA_LOOP},
    {0x11b48, 0x130, 0x4, 0x3, 0x24, 0x0, XA_NO_LOOP},
    {0x11e40, 0x146, 0x5, 0x3, 0x3c, 0x0, XA_NO_LOOP},
    {0x12170, 0x16c, 0x6, 0x3, 0x38, 0x0, XA_NO_LOOP},
    {0x12500, 0x5c, 0x7, 0x3, 0x3e, 0x0, XA_NO_LOOP},
    {0x125e8, 0x203, 0x8, 0x3, 0x48, 0x0, XA_NO_LOOP},
    {0x12af0, 0x206, 0x9, 0x3, 0x63, 0x0, XA_NO_LOOP},
    {0x13000, 0x216, 0xa, 0x3, 0x76, 0x0, XA_NO_LOOP},
    {0x13538, 0xdc, 0xb, 0x3, 0x48, 0x0, XA_NO_LOOP},
    {0x13760, 0x26, 0xc, 0x3, 0x2e, 0x0, XA_NO_LOOP},
    {0x137c0, 0x140, 0xd, 0x3, 0x73, 0x0, XA_NO_LOOP},
    {0x13ae0, 0x100, 0xe, 0x3, 0x50, 0x0, XA_NO_LOOP},
    {0x13d60, 0x139, 0xf, 0x3, 0x54, 0x0, XA_NO_LOOP},
    {0x14070, 0x26, 0x10, 0x3, 0x38, 0x0, XA_NO_LOOP},
    {0x140d0, 0x2ac, 0x11, 0x3, 0x6a, 0x0, XA_NO_LOOP},
    {0x14780, 0x32c, 0x12, 0x3, 0x5d, 0x0, XA_NO_LOOP},
    {0x14f70, 0x60, 0x13, 0x3, 0x44, 0x0, XA_NO_LOOP},
    {0x15060, 0x239, 0x14, 0x3, 0x48, 0x0, XA_NO_LOOP},
    {0x155f0, 0x26, 0x15, 0x3, 0x28, 0x0, XA_NO_LOOP},
    {0x15650, 0x116, 0x16, 0x3, 0x34, 0x0, XA_NO_LOOP},
    {0x15908, 0x26, 0x17, 0x3, 0x24, 0x0, XA_NO_LOOP},
    {0x15968, 0x200, 0x18, 0x3, 0x28, 0x0, XA_NO_LOOP},
    {0x15e68, 0x1d0, 0x19, 0x3, 0x5a, 0x0, XA_NO_LOOP},
    {0x162f0, 0x29, 0x1a, 0x3, 0x58, 0x0, XA_NO_LOOP},
    {0x16358, 0x59, 0x1b, 0x3, 0x60, 0x0, XA_NO_LOOP},
    {0x16438, 0x10c, 0x1c, 0x3, 0x40, 0x0, XA_NO_LOOP},
    {0x166d8, 0x263, 0x1d, 0x3, 0x60, 0x0, XA_NO_LOOP},
    {0x16cd0, 0xb9, 0x1e, 0x3, 0x60, 0x0, XA_NO_LOOP},
    {0x16ea0, 0x229, 0x1f, 0x3, 0x60, 0x0, XA_NO_LOOP},
    {0x17408, 0x739, 0x20, 0x3, 0x60, 0x0, XA_NO_LOOP},
    {0x18618, 0xc0, 0x21, 0x3, 0x30, 0x0, XA_NO_LOOP},
    {0x0, 0xd3, 0x1, 0x4, 0x7f, 0x0, XA_NO_LOOP},
    {0x210, 0x3673, 0x2, 0x4, 0x18, 0x0, XA_LOOP},
    {0x8a30, 0x116, 0x3, 0x4, 0x70, 0x0, XA_NO_LOOP},
    {0x8ce8, 0x440, 0x4, 0x4, 0x66, 0x0, XA_NO_LOOP},
    {0x9788, 0x383, 0x5, 0x4, 0x64, 0x0, XA_NO_LOOP},
    {0xa050, 0x430, 0x6, 0x4, 0x68, 0x0, XA_NO_LOOP},
    {0xaac8, 0x3c, 0x7, 0x4, 0x60, 0x0, XA_NO_LOOP},
    {0xab60, 0x83, 0x8, 0x4, 0x18, 0x0, XA_NO_LOOP},
    {0xaca8, 0x3719, 0x9, 0x4, 0x30, 0x0, XA_LOOP},
    {0x13668, 0x3163, 0xa, 0x4, 0x40, 0x0, XA_LOOP},
    {0x0, 0x26, 0x1, 0x5, 0x5c, 0x0, XA_NO_LOOP},
    {0x60, 0x39, 0x2, 0x5, 0x40, 0x0, XA_NO_LOOP},
    {0xf0, 0x23, 0x3, 0x5, 0x50, 0x0, XA_NO_LOOP},
    {0x148, 0x20, 0x4, 0x5, 0x48, 0x0, XA_NO_LOOP},
    {0x198, 0x146, 0x5, 0x5, 0x60, 0x0, XA_NO_LOOP},
    {0x4c8, 0x3746, 0x6, 0x5, 0x38, 0x0, XA_LOOP},
    {0x8ef8, 0x36f3, 0x7, 0x5, 0x4c, 0x0, XA_LOOP},
    {0x11858, 0xbec, 0x8, 0x5, 0x28, 0x0, XA_NO_LOOP},
    {0x13628, 0x5dc, 0x9, 0x5, 0x28, 0x0, XA_NO_LOOP},
    {0x144d0, 0x24c, 0xa, 0x5, 0x7f, 0x0, XA_NO_LOOP},
    {0x14a90, 0xec, 0xb, 0x5, 0x30, 0x0, XA_NO_LOOP},
    {0x14ce0, 0x7a0, 0xc, 0x5, 0x68, 0x0, XA_NO_LOOP},
    {0x15ff0, 0x230, 0xd, 0x5, 0x5c, 0x0, XA_NO_LOOP},
    {0x16568, 0x449, 0xe, 0x5, 0x68, 0x0, XA_NO_LOOP},
    {0x17020, 0x823, 0xf, 0x5, 0x68, 0x0, XA_NO_LOOP},
    {0x18478, 0x9c, 0x10, 0x5, 0x74, 0x0, XA_NO_LOOP},
    {0x18600, 0x30, 0x11, 0x5, 0x3c, 0x0, XA_NO_LOOP},
    {0x18678, 0x33, 0x12, 0x5, 0x58, 0x0, XA_NO_LOOP},
    {0x186f8, 0x36, 0x13, 0x5, 0x50, 0x0, XA_NO_LOOP},
    {0x18780, 0x2c, 0x14, 0x5, 0x3c, 0x0, XA_NO_LOOP},
    {0x187f0, 0x99, 0x15, 0x5, 0x7f, 0x0, XA_NO_LOOP},
    {0x18970, 0x16c, 0x16, 0x5, 0x70, 0x0, XA_NO_LOOP},
    {0x18d00, 0x136, 0x17, 0x5, 0x70, 0x0, XA_NO_LOOP},
    {0x19008, 0x206, 0x18, 0x5, 0x70, 0x0, XA_NO_LOOP},
    {0x19518, 0x259, 0x19, 0x5, 0x50, 0x0, XA_NO_LOOP},
    {0x19af8, 0x90, 0x1a, 0x5, 0x68, 0x0, XA_NO_LOOP},
    {0x0, 0x38b6, 0x1, 0x6, 0x24, 0x0, XA_LOOP},
    {0x8dc8, 0x3890, 0x2, 0x6, 0x54, 0x0, XA_LOOP},
    {0x11b30, 0x86, 0x3, 0x6, 0x4c, 0x0, XA_NO_LOOP},
    {0x11c80, 0xb26, 0x4, 0x6, 0x30, 0x0, XA_NO_LOOP},
    {0x13860, 0x499, 0x5, 0x6, 0x78, 0x0, XA_NO_LOOP},
    {0x143e0, 0x40, 0x6, 0x6, 0xa, 0x0, XA_NO_LOOP},
    {0x14480, 0x33, 0x7, 0x6, 0x54, 0x0, XA_NO_LOOP},
    {0x14500, 0x2c, 0x8, 0x6, 0x10, 0x0, XA_NO_LOOP},
    {0x14570, 0x3c, 0x9, 0x6, 0x10, 0x0, XA_NO_LOOP},
    {0x14608, 0x1c, 0xa, 0x6, 0x5c, 0x0, XA_NO_LOOP},
    {0x14650, 0x3a0, 0xb, 0x6, 0x7f, 0x0, XA_NO_LOOP},
    {0x14f60, 0x43, 0xc, 0x6, 0x70, 0x0, XA_NO_LOOP},
    {0x15008, 0x480, 0xd, 0x6, 0x48, 0x0, XA_NO_LOOP},
    {0x15b48, 0x70, 0xe, 0x6, 0x30, 0x0, XA_NO_LOOP},
    {0x15c60, 0x156, 0xf, 0x6, 0x58, 0x0, XA_NO_LOOP},
    {0x15fb8, 0xc9, 0x10, 0x6, 0x5c, 0x0, XA_NO_LOOP},
    {0x161b0, 0xf3, 0x11, 0x6, 0x70, 0x0, XA_NO_LOOP},
    {0x16410, 0x610, 0x12, 0x6, 0x78, 0x0, XA_NO_LOOP},
    {0x17338, 0x480, 0x13, 0x6, 0x28, 0x0, XA_NO_LOOP},
    {0x17e78, 0x60, 0x14, 0x6, 0x68, 0x0, XA_NO_LOOP},
    {0x17f68, 0x80, 0x15, 0x6, 0x6c, 0x0, XA_NO_LOOP},
    {0x180a8, 0x2b3, 0x16, 0x6, 0x74, 0x0, XA_NO_LOOP},
    {0x0, 0x3293, 0x1, 0x7, 0x30, 0x0, XA_LOOP},
    {0x7e70, 0x33e0, 0x2, 0x7, 0x30, 0x0, XA_LOOP},
    {0x10020, 0x90, 0xe, 0x7, 0x28, 0x0, XA_NO_LOOP},
    {0x10188, 0x90, 0xf, 0x7, 0x28, 0x0, XA_NO_LOOP},
    {0x102f0, 0x11c, 0x10, 0x7, 0x64, 0x0, XA_NO_LOOP},
    {0x105b8, 0x1d6, 0x11, 0x7, 0x44, 0x0, XA_NO_LOOP},
    {0x10a50, 0x166, 0x12, 0x7, 0x4c, 0x0, XA_NO_LOOP},
    {0x10dd0, 0x26, 0x13, 0x7, 0x3c, 0x0, XA_NO_LOOP},
    {0x10e30, 0x1e9, 0x14, 0x7, 0x6b, 0x0, XA_NO_LOOP},
    {0x112f8, 0xb9, 0x15, 0x7, 0x6b, 0x0, XA_NO_LOOP},
    {0x114c8, 0x17c, 0x16, 0x7, 0x2c, 0x0, XA_NO_LOOP},
    {0x11880, 0x90, 0x17, 0x7, 0x38, 0x0, XA_NO_LOOP},
    {0x119e8, 0x1e9, 0x18, 0x7, 0x40, 0x0, XA_NO_LOOP},
    {0x11eb0, 0x90, 0x19, 0x7, 0x50, 0x0, XA_NO_LOOP},
    {0x12018, 0x123, 0x1a, 0x7, 0x60, 0x0, XA_NO_LOOP},
    {0x122f0, 0xa6, 0x1b, 0x7, 0x44, 0x0, XA_NO_LOOP},
    {0x12490, 0x26, 0x1c, 0x7, 0x38, 0x0, XA_NO_LOOP},
    {0x124f0, 0x83, 0x1d, 0x7, 0x64, 0x0, XA_NO_LOOP},
    {0x12638, 0x26, 0x1e, 0x7, 0x32, 0x0, XA_NO_LOOP},
    {0x12698, 0x113, 0x1f, 0x7, 0x5c, 0x0, XA_NO_LOOP},
    {0x12948, 0x6c, 0x20, 0x7, 0x40, 0x0, XA_NO_LOOP},
    {0x12a58, 0x260, 0x21, 0x7, 0x54, 0x0, XA_NO_LOOP},
    {0x13048, 0x39, 0x22, 0x7, 0x41, 0x0, XA_NO_LOOP},
    {0x130d8, 0xa9, 0x23, 0x7, 0x64, 0x0, XA_NO_LOOP},
    {0x13280, 0x89, 0x24, 0x7, 0x77, 0x0, XA_NO_LOOP},
    {0x133d8, 0x149, 0x25, 0x7, 0x60, 0x0, XA_NO_LOOP},
    {0x13710, 0x60, 0x26, 0x7, 0x53, 0x0, XA_NO_LOOP},
    {0x13800, 0x190, 0x27, 0x7, 0x5c, 0x0, XA_NO_LOOP},
    {0x13be8, 0x1ec, 0x28, 0x7, 0x44, 0x0, XA_NO_LOOP},
    {0x140b8, 0x46, 0x29, 0x7, 0x38, 0x0, XA_NO_LOOP},
    {0x14168, 0x180, 0x2a, 0x7, 0x53, 0x0, XA_NO_LOOP},
    {0x14528, 0x1bc, 0x2b, 0x7, 0x60, 0x0, XA_NO_LOOP},
    {0x14980, 0x180, 0x2c, 0x7, 0x4d, 0x0, XA_NO_LOOP},
    {0x14d40, 0x160, 0x2d, 0x7, 0x4e, 0x0, XA_NO_LOOP},
    {0x150b0, 0x60, 0x2e, 0x7, 0x51, 0x0, XA_NO_LOOP},
    {0x151a0, 0xe6, 0x2f, 0x7, 0x60, 0x0, XA_NO_LOOP},
    {0x153e0, 0x3c, 0x30, 0x7, 0x54, 0x0, XA_NO_LOOP},
    {0x15478, 0x14c, 0x31, 0x7, 0x54, 0x0, XA_NO_LOOP},
    {0x157b8, 0x73, 0x32, 0x7, 0x54, 0x0, XA_NO_LOOP},
    {0x158d8, 0x1dc, 0x33, 0x7, 0x4a, 0x0, XA_NO_LOOP},
    {0x15d80, 0x60, 0x34, 0x7, 0x38, 0x0, XA_NO_LOOP},
    {0x15e70, 0x253, 0x35, 0x7, 0x4c, 0x0, XA_NO_LOOP},
    {0x16440, 0x169, 0x36, 0x7, 0x3d, 0x0, XA_NO_LOOP},
    {0x167c8, 0x1d9, 0x37, 0x7, 0x7f, 0x0, XA_NO_LOOP},
    {0x16c68, 0x5d3, 0x38, 0x7, 0x4e, 0x0, XA_NO_LOOP},
    {0x17af8, 0xf0, 0x39, 0x7, 0x3c, 0x0, XA_NO_LOOP},
    {0x17d50, 0x83, 0x3a, 0x7, 0x5c, 0x0, XA_NO_LOOP},
    {0x17e98, 0x609, 0x3b, 0x7, 0x20, 0x0, XA_NO_LOOP},
    {0x13498, 0x63c, 0x9, 0x5, 0x28, 0x0, XA_NO_LOOP},
};

static inline u32 EncodeDigits(u32 val) { return (val / 10) * 16 + (val % 10); }

void SetSeqDataPtr(void *arg0) { gSeqDataPtr = arg0; }

void SetCurrentSeqSet(s32 arg0) { gSeqSetIdx = arg0; }

void SetSeqAccessNum(s16 arg0) { gSeqAccessNum = arg0; }

void SetXaCdSectorOffset(s32 arg0) { gXaCdSectorOffset = arg0; }

u32 GetCurrentSeqId(void) {
   u16 res = 0;
   if (gSeqCurrentID != 0) {
      res = gSeqCurrentID | 0x200;
   } else {
      res = 0;
   }
   return res;
}

void SetReverbDepth(s16 depth) { SsUtSetReverbDepth(depth, depth); }

u8 ExecuteCdControl(u8 arg0, u8 *arg1, u8 *arg2) {
   extern s32 s_cdSyncStatus;
   u8 buf[8];

   s_cdSyncStatus = CdSync(1, buf);

   if (((s_cdSyncStatus & 0x10) != 0) || ((s_cdSyncStatus & 4) != 0)) {
      CdControl(1, 0, 0);
      return 2;
   }
   if (s_cdSyncStatus == CdlComplete) {
      if (CdControl(arg0, arg1, arg2) != 0) {
         D_80125AAC = 0;
         return 0;
      }
      return 1;
   }
   return 1;
}

void ResetVolume(void) {
   gCurrentVolume = 0x7f;
   gMaxVolume = 0x7f;
   SsSetMVol(0x7f, 0x7f);
}

void ResetAudioState(void) {
   extern s32 s_audioJobIdx;
   for (s_audioJobIdx = 0; s_audioJobIdx < 63; s_audioJobIdx++) {
      gAudioJobQueue[s_audioJobIdx] = AUDIO_JOB_NULL;
   }
   gSfxSlotSustained = 0;
   gSfxSlotToggled = 0;
   D_801801B0 = 0;
   gSeqAccessNum = 1;
   gSeqDataPtr = NULL;
   gSeqSetIdx = 0;
   gXaCurrentID = 0;
   gXaIdToPrepare = 0;
   gXaLoopEnabled = 0;
   gXaPauseInProgress = 0;
   D_801733B8 = 0;
   D_8012C0D0 = 0;
   gAudioJobPlayXaState = 0;
   gAudioJobPlayPreparedXaState = 0;
   gAudioJobPrepareXaState = 0;
   gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_FADE;
   D_80125AAC = 0;
   gXaVolumeState = XA_VOLUME_STATE_NONE;
   gVolumeFadeOutMode = 3;
   gVolumeFadeInMode = VOLUME_FADE_IN_MODE_NONE;
   gVolumeFadePauseCounter = 0;
   gXaFadeInState = XA_FADE_IN_STATE_NONE;
   gXaAdjustedVolume = gXaMaxVolume;
   gSfxMasterVolume = 0x7f;
   gSeqCurrentVolume = 0x70;
   gSeqMaxVolume = 0x70;
   gVolumeFadeOutSpeed = 0;
   gVolumeFadeInSpeed = 0;
}

void SetSerialVolumeFromCurveIdx(s16 arg0, s16 arg1, s16 arg2) {
   SsSetSerialVol(arg0, gVolumeCurve[arg1], gVolumeCurve[arg2]);
}

void InitAudio(void) {
   gAudioEnabled = 1;
   SsInit();
   SsSetTickMode(SS_TICK60);
   SetAudioMode(AUDIO_MODE_STEREO);
   SsSetReservedVoice(0x14);
   SsStart();
   SsSetTableSize(gSeqTable, 0x10, 1);
   SsUtSetReverbType(SPU_REV_MODE_STUDIO_C);
   SpuClearReverbWorkArea(SPU_REV_MODE_STUDIO_C);
   SsUtReverbOn();
   ResetVolume();
   gXaCurrentVolume = 0x6e;
   SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
   SetSerialVolumeFromCurveIdx(0, gXaCurrentVolume, gXaCurrentVolume);
   gXaCdControlParam.file = (CdlModeSF | CdlModeRT | CdlModeSpeed);
   ExecuteCdControl(CdlSetmode, (void *)&gXaCdControlParam, NULL);
   ResetAudioState();
   SetReverbDepth(0x3c);
   SpuSetTransferMode(0);
   SpuSetIRQCallback(0);
   CdReadyCallback(0);
   SpuMallocWithStartAddr(0x1010, 0x10000);
}

u8 AudioIsPlaying(void) {
   if (gXaCurrentID != 0) {
      return 1;
   }
   if (gSeqCurrentID != 0) {
      return 3;
   }
   if (D_8012C0D0 != 0) {
      return 2;
   }
   return 0;
}

void WaitForAudio(void) {
   while (AudioIsPlaying()) {
      VSync(0);
      UpdateAudio();
   }
}

void CloseAudio(void) {
   SsVabClose(0);
   SsVabClose(1);
   SsEnd();
   SsQuit();
   gAudioEnabled = 0;
}

void SetAudioMode(u8 param_1) {
   CdlATV atv;

   switch (param_1) {
   case AUDIO_MODE_MONO:
      if (gAudioMode != AUDIO_MODE_MONO) {
         SsSetMono();
         atv.val2 = 79;
         atv.val0 = 79;
         atv.val3 = 79;
         atv.val1 = 79;
         CdMix(&atv);
         gSfxMasterVolume = 108;
         gAudioMode = 0;
      }
      break;
   case AUDIO_MODE_STEREO:
      if (gAudioMode != AUDIO_MODE_STEREO) {
         SsSetStereo();
         atv.val2 = 127;
         atv.val0 = 127;
         atv.val3 = 0;
         atv.val1 = 0;
         CdMix(&atv);
         gSfxMasterVolume = 127;
         gAudioMode = 1;
      }
      break;
   }
}

void PlayVab(s16 vab_pro, s16 pitch, u16 vol) {
   if (pitch != 0) {
      SsVoKeyOff(vab_pro, pitch);
      SsVoKeyOn(vab_pro, pitch, vol, vol);
   }
}

void PlaySoundEffect(s16 param_1) {
   extern s16 s_sfxCurrentId;
   s_sfxCurrentId = param_1 - 0x500;
   AudioJobQueue_Insert(AUDIO_JOB_RESET_VOLUME);

   gSfxPrevVolume = (gSfxMasterVolume * gSfxProperties[s_sfxCurrentId].volume) >> 7;

   if (gSfxProperties[s_sfxCurrentId].role == SFX_ROLE_SUSTAIN) {
      // Once sustained, toggle state is locked, and subsequent SFX will be played in the
      // toggled/alternate slot until it is released;
      gSfxSlotSustained = 1;
      SsUtKeyOnV(gSfxSlotToggled * 2 + 20, gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);

      SsUtKeyOnV(gSfxSlotToggled * 2 + 21, gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone + 1,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);
      return;
   } else if (gSfxProperties[s_sfxCurrentId].role == SFX_ROLE_RELEASE) {
      gSfxSlotSustained = 0;
      SsUtKeyOffV(gSfxSlotToggled * 2 + 20);
      SsUtKeyOffV(gSfxSlotToggled * 2 + 21);
      return;
   }

   // SFX_ROLE_NONE:
   if (gSfxSlotSustained) {
      SsUtKeyOnV((gSfxSlotToggled ^ 1) * 2 + 20, gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);

      SsUtKeyOnV((gSfxSlotToggled ^ 1) * 2 + 21, gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone + 1,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);
   } else {
      SsUtKeyOnV((gSfxSlotToggled * 2 + 20), gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);

      SsUtKeyOnV((gSfxSlotToggled * 2 + 21), gSfxProperties[s_sfxCurrentId].vabid,
                 gSfxProperties[s_sfxCurrentId].prog, gSfxProperties[s_sfxCurrentId].tone + 1,
                 gSfxProperties[s_sfxCurrentId].note, 0, gSfxPrevVolume, gSfxPrevVolume);
      gSfxSlotToggled ^= 1;
   }
}

void QueuePrepareXa(s8 arg0) {
   gXaIdToPrepare = arg0;
   AudioJobQueue_Insert(AUDIO_JOB_PREPARE_XA);
}

void AudioJob_PrepareXa(void) {
   extern u8 s_cdSyncResultBuffer_80124f0c[8];
   extern u8 s_xaIdToPrepare;
   extern u32 s_minutes_80124f1c;
   u32 seconds;
   u32 sector;
   u32 minutes;
   u32 res;

   switch (gAudioJobPrepareXaState) {
   case 0:
      gXaCurrentVolume -= 0x20;
      if (gXaCurrentVolume < 0) {
         gXaCurrentVolume = 0;
      }
      SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaCurrentVolume, gXaCurrentVolume);
      if (gXaCurrentVolume == 0) {
         gAudioJobPrepareXaState = 1;
      }
      break;
   case 1:
      gXaCurrentID = 0;
      gXaCdControlParam.file = (CdlModeSF | CdlModeRT | CdlModeSpeed);
      s_xaIdToPrepare = gXaIdToPrepare;

      gAudioJobPrepareXaState = 2;
      /* fallthrough */
   case 2:
      res = ExecuteCdControl(CdlSetmode, (void *)&gXaCdControlParam, NULL);
      if (res == 0) {
         gAudioJobPrepareXaState = 3;
      }

   CheckResult:
      if (res == 2) {
         gAudioJobPrepareXaState = 8;
      }
      break;
   case 3:
      gXaCdControlParam.file = gXaProperties[s_xaIdToPrepare].fileId;
      gXaCdControlParam.chan = gXaProperties[s_xaIdToPrepare].channelId & 0xf;

      gAudioJobPrepareXaState = 4;
      /* fallthrough */
   case 4:
      res = ExecuteCdControl(CdlSetfilter, (void *)&gXaCdControlParam, NULL);

      if (res == 0) {
         gAudioJobPrepareXaState = 5;
      }
      goto CheckResult;
   case 5:
      s_minutes_80124f1c = gXaProperties[s_xaIdToPrepare].channelId +
                           gXaProperties[s_xaIdToPrepare].startingSector + STARTING_SECTOR;
      s_minutes_80124f1c += gXaCdSectorOffset;

      seconds = s_minutes_80124f1c / SECTORS_PER_SECOND;
      sector = s_minutes_80124f1c % SECTORS_PER_SECOND;
      gXaCdlLOC.sector = EncodeDigits(sector);

      s_minutes_80124f1c = seconds;
      minutes = seconds / SECONDS_PER_MINUTE;
      seconds = seconds % SECONDS_PER_MINUTE;
      gXaCdlLOC.second = EncodeDigits(seconds);

      s_minutes_80124f1c = minutes;
      gXaCdlLOC.minute = EncodeDigits(minutes);

      gAudioJobPrepareXaState = 6;
      /* fallthrough */
   case 6:
      res = ExecuteCdControl(CdlSeekL, (void *)&gXaCdlLOC, NULL);
      if (res == 0) {
         gAudioJobPrepareXaState = 7;
      }
      goto CheckResult;
   case 7: {
      CdSync(1, s_cdSyncResultBuffer_80124f0c);
      res = s_cdSyncResultBuffer_80124f0c[0];
      if ((res & CdlStatSeek) == 0) {
         gAudioJobPrepareXaState = 8;
      }
      break;
   }
   case 8:
      gAudioJobPrepareXaState = 0;
      AudioJobQueue_Pop();
   }
}

void QueuePlayXa(u8 id) {
   gXaIdToPlay = id;

   gXaVolumeState = XA_VOLUME_STATE_NONE;
   gXaAdjustedVolume = gXaMaxVolume;
   gXaMaxVolume = gXaProperties[gXaIdToPlay].volume;

   if (gXaIdToPrepare == gXaIdToPlay) {
      AudioJobQueue_Insert(AUDIO_JOB_RESET_VOLUME);
      AudioJobQueue_Insert(AUDIO_JOB_PLAY_PREPARED_XA);
   } else {
      if (gXaCurrentID != 0) {
         AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
      }
      AudioJobQueue_Insert(AUDIO_JOB_RESET_VOLUME);
      AudioJobQueue_Insert(AUDIO_JOB_PLAY_XA);
   }
   gXaIdToPrepare = 0;
}

void AudioJob_PlayPreparedXa(void) {
   extern u8 s_xaId_80124f24;
   extern s32 s_delay_80124f2c;
   u8 res;

   switch (gAudioJobPlayPreparedXaState) {
   case 0:
      s_delay_80124f2c = 10;
      s_xaId_80124f24 = gXaIdToPlay;
      if (gXaFadeInState == XA_FADE_IN_STATE_PENDING) {
         gXaCurrentVolume = 0;
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, 0, 0);
         gXaFadeInState = XA_FADE_IN_STATE_FADING;
      } else {
         gXaCurrentVolume = gXaMaxVolume;
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaMaxVolume, gXaMaxVolume);
      }

      gXaDuration = gXaProperties[s_xaId_80124f24].duration + 0x10;
      if (gXaProperties[s_xaId_80124f24].loop == XA_NO_LOOP) {
         gXaLoopEnabled = 0;
      } else {
         gXaLoopEnabled = 1;
      }
      gAudioJobPlayPreparedXaState = 1;
      /* fallthrough */
   case 1:
      res = ExecuteCdControl(CdlReadN, NULL, NULL);
      if (res == 0) {
         gAudioJobPlayPreparedXaState = 2;
      }
      if (res == 2) {
         gAudioJobPlayPreparedXaState = 3;
      }
      break;
   case 2:
      gXaCurrentID = s_xaId_80124f24;
      SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
      gXaStartTime = VSync(-1);
      gAudioJobPlayPreparedXaState = 3;
      break;
   case 3:
      if (s_delay_80124f2c != 0) {
         s_delay_80124f2c--;
         return;
      }
      gAudioJobPlayPreparedXaState = 0;
      AudioJobQueue_Pop();
      break;
   }
}

void AudioJob_PlayXa(void) {
   extern u8 s_cdSyncResultBuffer_80124f34[8];
   extern u32 s_minutes_80124f44;
   extern u8 s_xaIdToPlay_80124f3c;
   extern s32 s_delay_80124f4c;
   u32 seconds;
   u32 sector;
   u32 minutes;
   u8 res;

   switch (gAudioJobPlayXaState) {
   case 0:
      s_delay_80124f4c = 10;
      gXaCdControlParam.file = (CdlModeSF | CdlModeRT | CdlModeSpeed);
      gAudioJobPlayXaState = 1;
      s_xaIdToPlay_80124f3c = gXaIdToPlay;
      /* fallthrough */
   case 1:
      res = ExecuteCdControl(CdlSetmode, (void *)&gXaCdControlParam, NULL);
      if (res == 0) {
         gAudioJobPlayXaState = 2;
      }
      if (res == 2) {
         gAudioJobPlayXaState = 9;
      }
      break;
   case 2:
      gXaCdControlParam.file = gXaProperties[s_xaIdToPlay_80124f3c].fileId;
      gXaCdControlParam.chan = gXaProperties[s_xaIdToPlay_80124f3c].channelId & 0xf;
      gAudioJobPlayXaState = 3;
      /* fallthrough */
   case 3:
      res = ExecuteCdControl(CdlSetfilter, (void *)&gXaCdControlParam, NULL);
      if (res == 0) {
         gAudioJobPlayXaState = 4;
      }
      if (res == 2) {
         gAudioJobPlayXaState = 9;
      }
      break;
   case 4:
      s_minutes_80124f44 = gXaProperties[s_xaIdToPlay_80124f3c].channelId +
                           gXaProperties[s_xaIdToPlay_80124f3c].startingSector + STARTING_SECTOR;

      gXaDuration = gXaProperties[s_xaIdToPlay_80124f3c].duration + 0x10;

      if (gXaProperties[s_xaIdToPlay_80124f3c].loop == XA_NO_LOOP) {
         gXaLoopEnabled = 0;
      } else {
         gXaLoopEnabled = 1;
      }
      s_minutes_80124f44 += gXaCdSectorOffset;

      seconds = s_minutes_80124f44 / SECTORS_PER_SECOND;
      sector = s_minutes_80124f44 % SECTORS_PER_SECOND;
      gXaCdlLOC.sector = EncodeDigits(sector);

      s_minutes_80124f44 = seconds;
      minutes = seconds / SECONDS_PER_MINUTE;
      seconds = seconds % SECONDS_PER_MINUTE;
      gXaCdlLOC.second = EncodeDigits(seconds);
      s_minutes_80124f44 = minutes;
      gXaCdlLOC.minute = EncodeDigits(minutes);

      gAudioJobPlayXaState = 5;
      /* fallthrough */
   case 5:
      res = ExecuteCdControl(CdlSeekL, (void *)&gXaCdlLOC, NULL);
      if (res == 0) {
         gAudioJobPlayXaState = 6;
      }
      if (res == 2) {
         gAudioJobPlayXaState = 9;
      }
      break;
   case 6:
      CdSync(1, s_cdSyncResultBuffer_80124f34);
      res = s_cdSyncResultBuffer_80124f34[0];
      if ((res & CdlStatSeek) != 0) {
         return;
      }
      if ((res & CdlStatShellOpen) == 0) {
         gAudioJobPlayXaState = 7;
         return;
      }
      gAudioJobPlayXaState = 9;
      break;
   case 7:
      res = ExecuteCdControl(CdlReadN, NULL, NULL);
      if (res == 0) {
         gAudioJobPlayXaState = 8;
      }
      if (res == 2) {
         gAudioJobPlayXaState = 9;
      }
      break;
   case 8:
      if (gXaFadeInState == XA_FADE_IN_STATE_PENDING) {
         gXaCurrentVolume = 0;
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, 0, 0);
         gXaFadeInState = XA_FADE_IN_STATE_FADING;
      } else {
         gXaCurrentVolume = gXaMaxVolume;
         SetSerialVolumeFromCurveIdx(0, gXaCurrentVolume, gXaCurrentVolume);
      }
      gXaCurrentID = s_xaIdToPlay_80124f3c;
      SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SON);
      gXaStartTime = VSync(-1);
      gAudioJobPlayXaState = 9;
      break;
   case 9:
      if (s_delay_80124f4c != 0) {
         s_delay_80124f4c--;
         return;
      }
      gAudioJobPlayXaState = 0;
      AudioJobQueue_Pop();
   default:
      break;
   }
}

void QueuePauseXa(void) {
   gXaVolumeState = XA_VOLUME_STATE_NONE;
   gXaAdjustedVolume = gXaMaxVolume;
   AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
}

void AudioJob_PauseXa(void) {
   s32 res;

   if (gXaCurrentID != 0) {
      gXaPauseInProgress = 1;
      switch (gAudioJobPauseXaState) {
      case AUDIO_JOB_PAUSE_XA_STATE_FADE:
         gXaCurrentVolume -= 0x20;
         if (gXaCurrentVolume < 0) {
            gXaCurrentVolume = 0;
         }
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaCurrentVolume, gXaCurrentVolume);
         if (gXaCurrentVolume == 0) {
            gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_MUTE;
         }
         break;
      case AUDIO_JOB_PAUSE_XA_STATE_MUTE:
         SsSetSerialAttr(SS_SERIAL_A, SS_MIX, SS_SOFF);
         gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_PAUSE;
         /* fallthrough */
      case AUDIO_JOB_PAUSE_XA_STATE_PAUSE:
         res = ExecuteCdControl(CdlPause, NULL, NULL);
         if (res == 0) {
            gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_DONE;
         }
         if (res == 2) {
            gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_DONE;
         }
         break;
      case AUDIO_JOB_PAUSE_XA_STATE_DONE:
         gXaPauseInProgress = 0;
         gXaCurrentID = 0;
         gXaFadeInState = XA_FADE_IN_STATE_NONE;
         gAudioJobPauseXaState = AUDIO_JOB_PAUSE_XA_STATE_FADE;
         AudioJobQueue_Pop();
      }
   } else {
      AudioJobQueue_Pop();
   }
}

void SetXaReverbEnabled(s8 enabled) { SsSetSerialAttr(SS_SERIAL_A, SS_REV, enabled == 1); }

void AudioJobQueue_Insert(u8 job) {
   extern s32 s_iJob_80124f54, s_iJob2_80124f5c;

   /* Remove any existing jobs of this type by shifting elements to the left: */
   for (s_iJob_80124f54 = 1; s_iJob_80124f54 < 62; s_iJob_80124f54++) {
      if (gAudioJobQueue[s_iJob_80124f54] == job) {
         for (s_iJob2_80124f5c = s_iJob_80124f54; s_iJob2_80124f5c < 62; s_iJob2_80124f5c++) {
            gAudioJobQueue[s_iJob2_80124f5c] = gAudioJobQueue[s_iJob2_80124f5c + 1];
         }
         gAudioJobQueue[63] = AUDIO_JOB_NULL;
      }
   }
   /* Insert job into first available slot: */
   for (s_iJob_80124f54 = 0; s_iJob_80124f54 < 63; s_iJob_80124f54++) {
      if (gAudioJobQueue[s_iJob_80124f54] == AUDIO_JOB_NULL) {
         gAudioJobQueue[s_iJob_80124f54] = job;
         return;
      }
   }
}

void AudioJobQueue_Pop(void) {
   extern s32 s_iJob_80124f64;

   if (gAudioJobQueue[0] != AUDIO_JOB_NULL) {
      for (s_iJob_80124f64 = 0; s_iJob_80124f64 < 62; s_iJob_80124f64++) {
         gAudioJobQueue[s_iJob_80124f64] = gAudioJobQueue[s_iJob_80124f64 + 1];
      }
      gAudioJobQueue[63] = AUDIO_JOB_NULL;
   }
}

void AudioJob_Unused_0x7(void) { return; }

void DisableSpuIRQ(void) {
   SpuSetIRQ(SPU_OFF);
   SpuSetIRQCallback((SpuIRQCallbackProc)NULL);
   gClearedSpuIRQ = 1;
}

void AudioJob_Unused_0x8(void) { return; }

void QueuePlaySeq(s8 id) {
   gSeqIDToPlay = id;
   if (gSeqCurrentID != 0) {
      AudioJobQueue_Insert(AUDIO_JOB_STOP_SEQ);
   }
   AudioJobQueue_Insert(AUDIO_JOB_RESET_VOLUME);
   AudioJobQueue_Insert(AUDIO_JOB_PLAY_SEQ);
}

void AudioJob_PlaySeq(void) {
   extern s16 s_seqId;
   s32 *pSeqDataOffset;
   s16 *p;
   s32 idx, j;

   s_seqId = gSeqIDToPlay;
   p = &gSeqTaggedIDs[gSeqSetIdx][0];

   /* Search for seq id in current set: */
   for (idx = 0; idx < 8; idx++) {
      /* before comparing, re-or the upper 0x02 byte onto gSeqIDToPlay that
       * was dropped in PerformAudioCommand / QueuePlaySeq */
      if ((s_seqId | 0x200) == *p) {
         break;
      }
      p++;
   }

   /* Increment a matching amount to locate the associated data offset: */
   pSeqDataOffset = &gSeqDataOffsets[gSeqSetIdx][0];
   for (j = 0; j < idx; j++)
      pSeqDataOffset++;

   gSeqAccessNum = SsSeqOpen(gSeqDataPtr + *pSeqDataOffset, 1);
   gSeqReverb = gSeqProperties[s_seqId].reverb;
   SetReverbDepth(gSeqReverb);
   gSeqCurrentVolume = gSeqProperties[s_seqId].volume;
   gSeqMaxVolume = gSeqProperties[s_seqId].volume;
   SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);
   if ((gSeqProperties[s_seqId].loop & 0xf) == 0) {
      SsSeqPlay(gSeqAccessNum, SSPLAY_PLAY, 1);
   } else {
      SsSeqPlay(gSeqAccessNum, SSPLAY_PLAY, SSPLAY_INFINITY);
   }

   gSeqCurrentID = gSeqIDToPlay;
   AudioJobQueue_Pop();
}

void QueueStopSeq(void) { AudioJobQueue_Insert(AUDIO_JOB_STOP_SEQ); }

void AudioJob_StopSeq(void) {
   if (gSeqCurrentID != 0) {
      SsSeqStop(gSeqAccessNum);
      SsSeqClose(gSeqAccessNum);
      gSeqCurrentID = 0;
   }
   AudioJobQueue_Pop();
}

void SetupVolumeFadeOut(s16 speed, s16 mode) {
   extern s8 s_unused_80125a94;
   gVolumeFadeCounter = 0;
   gVolumeFadeOutSpeed = speed;
   gVolumeFadeOutMode = mode;
   s_unused_80125a94 = 0;
}

void SetupVolumeFadeIn(s16 speed, u8 mode) {
   switch (mode) {
   case VOLUME_FADE_IN_MODE_XA:
      if (gXaCurrentID == 0) {
         gVolumeFadeCounter = 0;
         gVolumeFadeInSpeed = speed;
         gVolumeFadeInMode = mode;
         gXaFadeInState = XA_FADE_IN_STATE_PENDING;
         gXaCurrentVolume = 0;
         gXaMaxVolume = 0;
         return;
      }
      return;
   case VOLUME_FADE_IN_MODE_SEQ:
      if (gSeqCurrentID != 0) {
         gVolumeFadeCounter = 0;
         gVolumeFadeInSpeed = speed;
         gVolumeFadeInMode = mode;
         return;
      }
      break;
   case VOLUME_FADE_IN_MODE_MAIN:
      if ((gXaCurrentID == 0) && (gSeqCurrentID == 0)) {
         gVolumeFadeCounter = 0;
         gVolumeFadeInSpeed = speed;
         gVolumeFadeInMode = mode;
         gCurrentVolume = 0;
         SsSetMVol(0, 0);
         gMaxVolume = 0x7f;
      }
      break;
   }
}

void StopSound(void) {
   SpuSetKey(SPU_OFF, SPU_ALLCH);
   gSfxSlotSustained = 0;
}

void PerformParamlessAudioCommand(s16 cmd) {
   // TODO cmd constants
   switch (cmd) {
   case 0x1:
      SetAudioMode(AUDIO_MODE_MONO);
      break;
   case 0x2:
      SetAudioMode(AUDIO_MODE_STEREO);
      break;
   case 0x3:
      QueueStopSeq();
      break;
   case 0x4:
      StopSound();
      break;
   case 0x5:
      QueuePauseXa();
      break;
   case 0x6:
   case 0x7:
      QueuePauseXa();
      QueueStopSeq();
      WaitForAudio();
      StopSound();
      gVolumeFadeOutSpeed = 0;
      break;
   case 0x8:
      WaitForAudio();
      break;
   case 0x50:
      StopSound();
      SsSeqSetVol(gSeqAccessNum, 64, 64);
      break;
   case 0x51:
      SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);
      break;
   case 0x52:
      SetupVolumeFadeOut(8, 8);
      break;
   case 0x53:
      SetupVolumeFadeIn(8, 8);
      break;
   case 0x54:
      gXaVolumeState = XA_VOLUME_STATE_DECREASING;
      gXaAdjustedVolume = gXaMaxVolume;
      gXaReducedVolumeTarget = gXaMaxVolume / 3;
      break;
   case 0x55:
      gXaVolumeState = XA_VOLUME_STATE_INCREASING;
      break;
   case 0x20:
      SetupVolumeFadeOut(32, 4);
      break;
   case 0x21:
      SetupVolumeFadeOut(8, 4);
      break;
   case 0x22:
      SetupVolumeFadeOut(4, 4);
      break;
   case 0x23:
      SetupVolumeFadeOut(2, 4);
      break;
   case 0x24:
      SetupVolumeFadeOut(1, 4);
      break;
   case 0x28:
      SetupVolumeFadeOut(128, 3);
      break;
   case 0x29:
      SetupVolumeFadeOut(8, 3);
      break;
   case 0x2a:
      SetupVolumeFadeOut(4, 3);
      break;
   case 0x2b:
      SetupVolumeFadeOut(2, 3);
      break;
   case 0x2c:
      SetupVolumeFadeOut(1, 3);
      break;
   case 0x30:
      SetupVolumeFadeOut(128, 1);
      break;
   case 0x31:
      SetupVolumeFadeOut(8, 1);
      break;
   case 0x32:
      SetupVolumeFadeOut(4, 1);
      break;
   case 0x33:
      SetupVolumeFadeOut(2, 1);
      break;
   case 0x34:
      SetupVolumeFadeOut(1, 1);
      break;
   case 0x38:
      SetupVolumeFadeOut(128, 2);
      break;
   case 0x39:
      SetupVolumeFadeOut(8, 2);
      break;
   case 0x3a:
      SetupVolumeFadeOut(4, 2);
      break;
   case 0x3b:
      SetupVolumeFadeOut(2, 2);
      break;
   case 0x3c:
      SetupVolumeFadeOut(1, 2);
      break;
   case 0x40:
      SetupVolumeFadeIn(32, VOLUME_FADE_IN_MODE_XA);
      break;
   case 0x41:
      SetupVolumeFadeIn(8, VOLUME_FADE_IN_MODE_XA);
      break;
   case 0x42:
      SetupVolumeFadeIn(4, VOLUME_FADE_IN_MODE_XA);
      break;
   case 0x43:
      SetupVolumeFadeIn(2, VOLUME_FADE_IN_MODE_XA);
      break;
   case 0x44:
      SetupVolumeFadeIn(1, VOLUME_FADE_IN_MODE_XA);
      break;
   case 0x48:
      SetupVolumeFadeIn(4, VOLUME_FADE_IN_MODE_MAIN);
      break;
   case 0xb0:
      SetXaReverbEnabled(1);
      break;
   case 0xb1:
      SetXaReverbEnabled(0);
      break;
   case 0xc0:
      SetReverbDepth(8);
      break;
   case 0xc1:
      SetReverbDepth(0x10);
      break;
   case 0xc2:
      SetReverbDepth(0x18);
      break;
   case 0xc3:
      SetReverbDepth(0x20);
      break;
   case 0xc4:
      SetReverbDepth(0x28);
      break;
   case 0xc5:
      SetReverbDepth(0x30);
      break;
   case 0xc6:
      SetReverbDepth(0x38);
      break;
   case 0xc7:
      SetReverbDepth(0x40);
      break;
   case 0xc8:
      SetReverbDepth(0x48);
      break;
   case 0xc9:
      SetReverbDepth(0x50);
      break;
   case 0xca:
      SetReverbDepth(0x58);
      break;
   case 0xcb:
      SetReverbDepth(0x60);
      break;
   case 0xcc:
      SetReverbDepth(0x68);
      break;
   case 0xcd:
      SetReverbDepth(0x70);
      break;
   case 0xce:
      SetReverbDepth(0x78);
      break;
   case 0xcf:
      SetReverbDepth(0x7f);
      break;
   default:
      return;
   }
}

void PerformAudioCommand(s16 cmd) {
#ifdef PC_DEBUG_AUDIO_LOG
   { extern void PC_DebugAudioCmdLog(int cmd, int enabled);
     PC_DebugAudioCmdLog((int)(u16)cmd, gAudioEnabled); }
#endif
   if (gAudioEnabled) {
      switch ((u8)(cmd >> 8)) {
      case 0:
         PerformParamlessAudioCommand(cmd);
         return;
      case 2:
         QueuePlaySeq(cmd);
         return;
      case 3:
         QueuePlayXa(cmd);
         return;
      case 5:
      case 6:
      case 7:
         PlaySoundEffect(cmd);
         return;
      case 0x13:
         QueuePrepareXa(cmd);
         break;
      }
   }
}

void AudioJobQueue_ProcessNext(void) {
   switch (gAudioJobQueue[0]) {
   case AUDIO_JOB_NULL:
      break;
   case AUDIO_JOB_PLAY_SEQ:
      AudioJob_PlaySeq();
      break;
   case AUDIO_JOB_PLAY_XA:
      AudioJob_PlayXa();
      break;
   case AUDIO_JOB_PLAY_PREPARED_XA:
      AudioJob_PlayPreparedXa();
      break;
   case AUDIO_JOB_PREPARE_XA:
      AudioJob_PrepareXa();
      break;
   case AUDIO_JOB_STOP_SEQ:
      AudioJob_StopSeq();
      break;
   case AUDIO_JOB_PAUSE_XA:
      AudioJob_PauseXa();
      break;
   case AUDIO_JOB_UNUSED_0x8:
      AudioJob_Unused_0x8();
      break;
   case AUDIO_JOB_UNUSED_0x7:
      AudioJob_Unused_0x7();
      break;
   case AUDIO_JOB_RESET_VOLUME:
      ResetVolume();
      AudioJobQueue_Pop();
      break;
   default:
      return;
   }
}

void HandleVolumeFadeIn(void) {
   if (gVolumeFadeInSpeed == 0) {
      return;
   }
   if (++gVolumeFadePauseCounter != 4) {
      return;
   }
   gVolumeFadePauseCounter = 0;

   switch (gVolumeFadeInMode) {
   case VOLUME_FADE_IN_MODE_XA:
      if (gXaFadeInState == XA_FADE_IN_STATE_PENDING) {
         return;
      }
      if (gXaFadeInState == XA_FADE_IN_STATE_FADING) {
         gVolumeFadeCounter++;
         if (gXaCurrentVolume <= gXaMaxVolume) {
            gXaCurrentVolume = (gXaMaxVolume * gVolumeFadeInSpeed * gVolumeFadeCounter) / 512;
            SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaCurrentVolume, gXaCurrentVolume);
         } else {
            gXaCurrentVolume = gXaMaxVolume;
            SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaCurrentVolume, gXaCurrentVolume);
            gXaFadeInState = XA_FADE_IN_STATE_NONE;
            gVolumeFadeInSpeed = 0;
         }
      }
      break;
   case VOLUME_FADE_IN_MODE_SEQ:
      gVolumeFadeCounter++;
      if (gSeqCurrentVolume <= gSeqMaxVolume) {
         gSeqCurrentVolume = (gSeqMaxVolume * gVolumeFadeInSpeed * gVolumeFadeCounter) / 512;
         SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);
      } else {
         gSeqCurrentVolume = gSeqMaxVolume;
         SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);
         gVolumeFadeInSpeed = 0;
      }
      break;
   case VOLUME_FADE_IN_MODE_MAIN:
      gVolumeFadeCounter++;
      if (gCurrentVolume <= gMaxVolume) {
         gCurrentVolume = (gMaxVolume * gVolumeFadeInSpeed * gVolumeFadeCounter) / 512;
         SsSetMVol(gCurrentVolume, gCurrentVolume);
      } else {
         gCurrentVolume = gMaxVolume;
         SsSetMVol(gCurrentVolume, gCurrentVolume);
         gVolumeFadeInSpeed = 0;
      }
      break;
   }
}

void HandleVolumeFadeOut(void) {
   if (gVolumeFadeOutSpeed == 0) {
      return;
   }
   gVolumeFadeCounter++;

   switch (gVolumeFadeOutMode) {
   case 1:
      if (gXaCurrentVolume < 1) {
         gXaCurrentVolume = 0;
      } else {
         gXaCurrentVolume =
             gXaMaxVolume - (gXaMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gXaCurrentVolume < 1) {
            gXaCurrentVolume = 0;
         }
      }

      SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaCurrentVolume, gXaCurrentVolume);

      if (gXaCurrentVolume == 0) {
         gVolumeFadeOutSpeed = 0;
         AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
      }
      break;
   case 2:
      if (gSeqCurrentVolume > 0) {
         gSeqCurrentVolume =
             gSeqMaxVolume - (gSeqMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gSeqCurrentVolume < 1) {
            gSeqCurrentVolume = 0;
         }
      } else {
         gSeqCurrentVolume = 0;
      }

      SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);

      if (gSeqCurrentVolume == 0) {
         gVolumeFadeOutSpeed = 0;
         AudioJobQueue_Insert(AUDIO_JOB_STOP_SEQ);
      }
      break;
   case 3:
      if (gXaCurrentVolume < 1) {
         gXaCurrentVolume = 0;
      } else {
         gXaCurrentVolume =
             gXaMaxVolume - (gXaMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gXaCurrentVolume < 1) {
            gXaCurrentVolume = 0;
         }
      }

      SetSerialVolumeFromCurveIdx(0, gXaCurrentVolume, gXaCurrentVolume);

      if (gSeqCurrentVolume < 1) {
         gSeqCurrentVolume = 0;
      } else {
         gSeqCurrentVolume =
             gSeqMaxVolume - (gSeqMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gSeqCurrentVolume < 1) {
            gSeqCurrentVolume = 0;
         }
      }

      SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);

      if (gXaCurrentVolume == 0 && gSeqCurrentVolume == 0) {
         gVolumeFadeOutSpeed = 0;
         AudioJobQueue_Insert(AUDIO_JOB_STOP_SEQ);
         AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
      }
      break;
   case 4:
      if (gCurrentVolume < 1) {
         gCurrentVolume = 0;
      } else {
         gCurrentVolume =
             gMaxVolume - (gMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gCurrentVolume < 1) {
            gCurrentVolume = 0;
         }
      }

      SsSetMVol(gCurrentVolume, gCurrentVolume);

      if (gCurrentVolume == 0) {
         StopSound();
         AudioJobQueue_Insert(AUDIO_JOB_STOP_SEQ);
         AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
         gVolumeFadeOutSpeed = 0;
      }
      break;
   case 8:
      if (gSeqCurrentVolume < 1) {
         gSeqCurrentVolume = 0;
      } else {
         gSeqCurrentVolume =
             gSeqMaxVolume - (gSeqMaxVolume * gVolumeFadeOutSpeed * gVolumeFadeCounter) / 512;
         if (gSeqCurrentVolume < 1) {
            gSeqCurrentVolume = 0;
         }
      }

      SsSeqSetVol(gSeqAccessNum, gSeqCurrentVolume, gSeqCurrentVolume);

      if (gSeqCurrentVolume == 0) {
         gVolumeFadeOutSpeed = 0;
         StopSound();
      }
      break;
   }
}

void HandleXaCompletion(void) {
   extern u32 s_xaElapsedTime;
   s32 queueContainsPlayXa, currentTime, i;

   if (gXaCurrentID == 0 || gXaDuration == 0) {
      return;
   }

   currentTime = VSync(-1);
   s_xaElapsedTime = currentTime - gXaStartTime;
   if (s_xaElapsedTime > gXaDuration) {
      if (!gXaLoopEnabled) {
         // Only queue a pause if no plays are currently queued
         i = 0;
         queueContainsPlayXa = 0;
         for (; i < 62; i++) {
            if (gAudioJobQueue[i] == AUDIO_JOB_PLAY_XA) {
               queueContainsPlayXa = 1;
            }
         }
         if (!queueContainsPlayXa) {
            AudioJobQueue_Insert(AUDIO_JOB_PAUSE_XA);
         }
      } else {
         // Looping enabled, so re-queue same xa
         if (!gXaPauseInProgress) {
            gXaIdToPlay = gXaCurrentID;
            AudioJobQueue_Insert(AUDIO_JOB_PLAY_XA);
            gXaStartTime = VSync(-1);
         }
      }
   }
}

void AdjustXaVolume(void) {
   if (gXaVolumeState == 0)
      return;

   switch (gXaVolumeState) {
   case XA_VOLUME_STATE_DECREASING:
      if (gXaAdjustedVolume > gXaReducedVolumeTarget) {
         gXaAdjustedVolume -= 2;
         if (gXaAdjustedVolume <= gXaReducedVolumeTarget) {
            gXaAdjustedVolume = gXaReducedVolumeTarget;
            gXaVolumeState = XA_VOLUME_STATE_DECREASED;
         }
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaAdjustedVolume, gXaAdjustedVolume);
      }
      break;
   case XA_VOLUME_STATE_INCREASING:
      if (gXaAdjustedVolume < gXaMaxVolume) {
         gXaAdjustedVolume += 2;
         if (gXaAdjustedVolume >= gXaMaxVolume) {
            gXaAdjustedVolume = gXaMaxVolume;
            gXaVolumeState = XA_VOLUME_STATE_NONE;
         }
         SetSerialVolumeFromCurveIdx(SS_SERIAL_A, gXaAdjustedVolume, gXaAdjustedVolume);
      }
      break;
   case XA_VOLUME_STATE_NONE:
   case XA_VOLUME_STATE_DECREASED:
   default:
      return;
   }
}

void UpdateAudio(void) {
   if (gAudioEnabled) {
      AudioJobQueue_ProcessNext();
      HandleVolumeFadeIn();
      HandleVolumeFadeOut();
      AdjustXaVolume();
      HandleXaCompletion();
   }
}

void Noop_800c72c0(void) { return; }
