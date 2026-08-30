/* Movie subtitle cues: an opaque cover rect painted over the burned-in text of a playing FMV, with
 * translated text on top. Identical on the native MDEC and HD-video paths: both become visible in
 * MovieRenderFrame(frameNo) (libcd.c), whose clock IS the 1-based STR header frame number. */
#ifndef PC_MOVIE_SUBS_H
#define PC_MOVIE_SUBS_H

#define PC_SUBS_MAX_LINES 4
#define PC_SUBS_MAX_TEXT  160

typedef struct PC_MovieCue {
    int startFrame, endFrame;                    /* inclusive, runtime frameNo (STR numbering) */
    int x, y, w, h;                              /* cover rect, native 320x240 coordinates */
    int lineCount;
    char lines[PC_SUBS_MAX_LINES][PC_SUBS_MAX_TEXT];
} PC_MovieCue;

void PC_MovieSubsOpen(int baseLBA);              /* stream started; cue sets are keyed by base LBA */
void PC_MovieSubsClose(void);                    /* movie ended / skipped / aborted */
void PC_MovieSubsFrame(int frameNo);             /* this frame is becoming current */
int  PC_MovieSubsLoaded(void);                   /* cues exist for the open movie (fixed at Open) */

/* Language pack K_CUES section (pc_lang.c hands the raw blob over at pack load). Parsed into
 * per-movie cue sets that live for the process; PC_MovieSubsOpen consults them when the
 * VH_MOVIE_SUBS dev override is not set. Bounds-checked -- a pack is a third-party download. */
void PC_MovieSubsLoadPack(const unsigned char *p, unsigned len);

/* Cues active on the current frame (band + card can overlap on the same frame).
 * Fills up to cap pointers, returns the count. Pointers are valid until the next
 * PC_MovieSubsOpen/Close. */
int PC_MovieSubsActive(const PC_MovieCue **out, int cap);

#endif
