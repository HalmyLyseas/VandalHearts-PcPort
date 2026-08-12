/* Movie subtitle cues (langpack F3): an opaque cover rect painted over the burned-in text
 * region of a playing FMV, with translated text drawn on top. Works identically on the native
 * MDEC path and the HD-video path because both become visible in MovieRenderFrame(frameNo)
 * (libcd.c), whose clock IS the STR header frame number (1-based).
 *
 * Cue files are per-movie, keyed by the movie's base LBA (same identity the HD video pack
 * uses), with frame ranges already in the runtime frameNo domain. Cues normally come from the
 * language pack's K_CUES section; VH_MOVIE_SUBS=<file-or-dir> is a dev override that wins. */
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

void PC_MovieSubsOpen(int baseLBA);              /* movie stream started (before frame 1 shows) */
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
