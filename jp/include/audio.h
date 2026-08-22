#ifndef AUDIO_H
#define AUDIO_H

#include "common.h"
#include "PsyQ/libcd.h"
#include "PsyQ/libsnd.h"
#include "PsyQ/libspu.h"

#define STARTING_SECTOR 150
#define SECTORS_PER_SECOND 75
#define SECONDS_PER_MINUTE 60

#define XA_LOOP 0
#define XA_NO_LOOP 2

#define AUDIO_CMD_PLAY_SEQ(id) (0x200 + (id))
#define AUDIO_CMD_PLAY_XA(id) (0x300 + (id))
#define AUDIO_CMD_PLAY_SFX(id) (0x500 + (id))
#define AUDIO_CMD_PREPARE_XA(id) (0x1300 + (id))

// TODO: Add more of these
typedef enum AudioCommand {
   AUDIO_CMD_MONO = 1,
   AUDIO_CMD_STEREO = 2,
   AUDIO_CMD_STOP_SEQ = 3,
   AUDIO_CMD_STOP_ALL = 6,
   // TODO: Descriptive names
   AUDIO_CMD_FADE_OUT_32_4 = 32,
   AUDIO_CMD_FADE_OUT_8_4 = 33,
   AUDIO_CMD_FADE_OUT_1_4 = 36,
   AUDIO_CMD_FADE_OUT_128_1 = 48,
   AUDIO_CMD_FADE_OUT_8_1 = 49,
   AUDIO_CMD_FADE_OUT_4_1 = 50,
   AUDIO_CMD_FADE_OUT_2_1 = 51,
   AUDIO_CMD_FADE_OUT_128_2 = 56,
   AUDIO_CMD_FADE_OUT_8_8 = 82,
   AUDIO_CMD_FADE_IN_8_8 = 83,
} AudioCommand;

typedef enum AudioMode { AUDIO_MODE_MONO = 0, AUDIO_MODE_STEREO = 1 } AudioMode;

typedef enum AudioJob {
   AUDIO_JOB_NULL = 0,
   AUDIO_JOB_PLAY_SEQ = 1,
   AUDIO_JOB_PLAY_XA = 2,
   AUDIO_JOB_PREPARE_XA = 3,
   AUDIO_JOB_STOP_SEQ = 4,
   AUDIO_JOB_PAUSE_XA = 5,
   AUDIO_JOB_UNUSED_0x7 = 7,
   AUDIO_JOB_UNUSED_0x8 = 8,
   AUDIO_JOB_PLAY_PREPARED_XA = 10,
   AUDIO_JOB_RESET_VOLUME = 11
} AudioJob;

typedef enum XaFadeInState {
   XA_FADE_IN_STATE_NONE = 0,
   XA_FADE_IN_STATE_PENDING = 1,
   XA_FADE_IN_STATE_FADING = 2
} XaFadeInState;

typedef enum XaVolumeState {
   XA_VOLUME_STATE_NONE = 0,
   XA_VOLUME_STATE_DECREASING = 1,
   XA_VOLUME_STATE_DECREASED = 2,
   XA_VOLUME_STATE_INCREASING = 3
} XaVolumeState;

typedef enum VolumeFadeInMode {
   VOLUME_FADE_IN_MODE_XA = 1,
   VOLUME_FADE_IN_MODE_NONE = 3,
   VOLUME_FADE_IN_MODE_MAIN = 4,
   VOLUME_FADE_IN_MODE_SEQ = 8
} VolumeFadeInMode;

typedef enum AudioJobPauseXaState {
   AUDIO_JOB_PAUSE_XA_STATE_FADE = 0,
   AUDIO_JOB_PAUSE_XA_STATE_MUTE = 1,
   AUDIO_JOB_PAUSE_XA_STATE_PAUSE = 2,
   AUDIO_JOB_PAUSE_XA_STATE_DONE = 3
} AudioJobPauseXaState;

typedef enum SfxRole { SFX_ROLE_NONE = 0, SFX_ROLE_SUSTAIN = 1, SFX_ROLE_RELEASE = 2 } SfxRole;

typedef struct SfxProperties {
   u8 vabid;
   u8 prog;
   u8 note;
   s8 volume;
   u8 role;
   u8 tone;
} SfxProperties;

typedef struct XaProperties {
   s32 startingSector;
   s32 duration;
   u8 fileId;
   u8 channelId;
   u8 volume;
   u8 padding;
   s32 loop;
} XaProperties;

typedef struct SeqProperties {
   u8 volume;
   u8 reverb;
   u8 loop;
} SeqProperties;

extern void *gSeqDataPtr;
extern s16 gSeqSetIdx;
extern s16 gSeqAccessNum;
extern s32 gXaCdSectorOffset;
extern u8 gSeqCurrentID;
extern s32 D_80125AAC;
extern s16 gCurrentVolume;
extern s16 gMaxVolume;
extern u8 gAudioJobQueue[64];

extern u8 D_8012DF88;
extern u8 gXaCurrentID;
extern u8 gSeqCurrentID;
extern u8 gAudioEnabled;

extern u8 gAudioMode;
extern u16 gSfxMasterVolume;
extern u16 gSfxPrevVolume;
extern s16 gSfxSlotSustained;
extern s16 gSfxSlotToggled;

extern s8 gSeqTable[SS_SEQ_TABSIZ * 16];
extern u8 gAudioJobPrepareXaState;
extern u8 gAudioJobPlayXaState;
extern s8 D_801733B8;
extern s32 D_801801B0;
extern u8 gAudioJobPlayPreparedXaState;
extern u8 gAudioJobPauseXaState;
extern CdlLOC gXaCdlLOC;
extern CdlFILTER gXaCdControlParam;
extern s16 gXaCurrentVolume;
extern s16 gXaFadeInState;
extern u8 gXaIdToPrepare;
extern u8 gXaIdToPlay;
extern u8 gXaLoopEnabled;
extern s16 gXaMaxVolume;
extern u8 gXaPauseInProgress;
extern s16 gSeqCurrentVolume;
extern s16 gSeqMaxVolume;
extern s16 gVolumeCurve[128];
extern u8 gVolumeFadeInMode;
extern u8 gVolumeFadeOutMode;
extern s16 gVolumeFadePauseCounter;
extern s32 gXaDuration;
extern s32 gXaStartTime;
extern s8 gClearedSpuIRQ;
extern u8 gSeqIDToPlay;
extern u8 gSeqReverb;
extern s16 gVolumeFadeCounter;
extern u32 gVabSoundBufferAddr[4];

/* TBD volatile? */
extern volatile s16 gXaReducedVolumeTarget;
extern volatile s16 gXaVolumeState;
extern volatile s16 gXaAdjustedVolume;
extern volatile s16 gVolumeFadeInSpeed;
extern volatile s16 gVolumeFadeOutSpeed;

extern XaProperties gXaProperties[188];
extern SfxProperties gSfxProperties[269];
extern SeqProperties gSeqProperties[38];

extern s16 gSeqTaggedIDs[38][8];
extern s32 gSeqDataOffsets[39][8];
extern u8 gSeqData[];

void SetSeqDataPtr(void *);
void SetCurrentSeqSet(s32);
void SetSeqAccessNum(s16);
void SetXaCdSectorOffset(s32);
u32 GetCurrentSeqId(void);
void SetReverbDepth(s16);
u8 ExecuteCdControl(u8, u8 *, u8 *);
void ResetVolume(void);
void ResetAudioState(void);
void SetSerialVolumeFromCurveIdx(s16, s16, s16);
void InitAudio(void);
u8 AudioIsPlaying(void);
void WaitForAudio(void);
void CloseAudio(void);
void SetAudioMode(u8);
void PlayVab(s16, s16, u16);
void PlaySoundEffect(s16);
void QueuePrepareXa(s8);
void AudioJob_PrepareXa(void);
void QueuePlayXa(u8);
void AudioJob_PlayPreparedXa(void);
void AudioJob_PlayXa(void);
void QueuePauseXa(void);
void AudioJob_PauseXa(void);
void SetXaReverbEnabled(s8);
void AudioJobQueue_Insert(u8);
void AudioJobQueue_Pop(void);
void AudioJob_Unused_0x7(void);
void DisableSpuIRQ(void);
void AudioJob_Unused_0x8(void);
void QueuePlaySeq(s8);
void AudioJob_PlaySeq(void);
void QueueStopSeq(void);
void AudioJob_StopSeq(void);
void SetupVolumeFadeOut(s16, s16);
void SetupVolumeFadeIn(s16, u8);
void StopSound(void);
void PerformParamlessAudioCommand(s16);
// void PerformAudioCommand(s16); // FIXME
void AudioJobQueue_ProcessNext(void);
void HandleVolumeFadeIn(void);
void HandleVolumeFadeOut(void);
void HandleXaCompletion(void);
void AdjustXaVolume(void);
void UpdateAudio(void);
void Noop_800c72c0(void);

#endif
