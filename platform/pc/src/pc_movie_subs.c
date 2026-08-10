/* Movie subtitle cue engine (langpack F3 pilot) -- see pc_movie_subs.h for the model.
 *
 * File format (emitted by exchange tooling from the extractor's JSON; line-based so the
 * parser stays trivial and greppable):
 *
 *   VHCUES 1
 *   lba 21618                      <- hex, matches hdpacks/videos/<lba>.mp4 naming
 *   cue 92 121 0 195 320 45        <- startFrame endFrame x y w h (runtime frameNo, native px)
 *   text Sostegaria...             <- 1..PC_SUBS_MAX_LINES per cue
 *   end
 *
 * Unknown lines are ignored (forward compatibility). A file whose lba does not match the
 * opening movie simply stays unloaded -- the movie plays with its burned-in text, which is
 * also the no-langpack behaviour by design. */
#include "pc_movie_subs.h"
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

static PC_MovieCue *s_cues = NULL;
static int s_cueCount = 0;
static int s_curFrame = 0;
static int s_loadedLBA = -1;

static void subsFree(void) {
    free(s_cues);
    s_cues = NULL; s_cueCount = 0; s_curFrame = 0; s_loadedLBA = -1;
}

static void subsLoad(const char *path, int baseLBA) {
    FILE *f = fopen(path, "r");
    char line[PC_SUBS_MAX_TEXT + 16];
    int cap = 0, fileLBA = -1, inCue = 0;
    PC_MovieCue cur;
    if (!f) { fprintf(stderr, "PC_MovieSubs: cannot open %s\n", path); return; }
    memset(&cur, 0, sizeof(cur));
    while (fgets(line, sizeof(line), f)) {
        size_t n = strlen(line);
        while (n && (line[n-1] == '\n' || line[n-1] == '\r')) line[--n] = '\0';
        if (strncmp(line, "lba ", 4) == 0) {
            fileLBA = (int)strtol(line + 4, NULL, 16);
            if (fileLBA != baseLBA) { fclose(f); return; }   /* not this movie's file: stay inert */
        } else if (strncmp(line, "cue ", 4) == 0) {
            memset(&cur, 0, sizeof(cur));
            inCue = sscanf(line + 4, "%d %d %d %d %d %d",
                           &cur.startFrame, &cur.endFrame,
                           &cur.x, &cur.y, &cur.w, &cur.h) == 6;
        } else if (inCue && strncmp(line, "text ", 5) == 0) {
            if (cur.lineCount < PC_SUBS_MAX_LINES) {
                strncpy(cur.lines[cur.lineCount], line + 5, PC_SUBS_MAX_TEXT - 1);
                cur.lineCount++;
            }
        } else if (inCue && strcmp(line, "end") == 0) {
            if (cur.lineCount > 0) {
                if (s_cueCount == cap) {
                    int next = cap ? cap * 2 : 48;
                    PC_MovieCue *grown = (PC_MovieCue *)realloc(s_cues, (size_t)next * sizeof(*s_cues));
                    if (!grown) break;
                    s_cues = grown; cap = next;
                }
                s_cues[s_cueCount++] = cur;
            }
            inCue = 0;
        }
    }
    fclose(f);
    if (fileLBA != baseLBA) { subsFree(); return; }
    s_loadedLBA = baseLBA;
    fprintf(stderr, "PC_MovieSubs: %d cues loaded for movie %x\n", s_cueCount, (unsigned)baseLBA);
}

void PC_MovieSubsOpen(int baseLBA) {
    const char *path;
    if (s_loadedLBA == baseLBA) { s_curFrame = 0; return; }   /* stream-start block re-runs */
    subsFree();
    path = getenv("VH_MOVIE_SUBS");
    if (path && *path) subsLoad(path, baseLBA);
}

void PC_MovieSubsClose(void) { subsFree(); }

void PC_MovieSubsFrame(int frameNo) { s_curFrame = frameNo; }

int PC_MovieSubsActive(const PC_MovieCue **out, int cap) {
    int i, n = 0;
    if (!s_cues || s_curFrame <= 0) return 0;
    for (i = 0; i < s_cueCount && n < cap; i++) {
        if (s_cues[i].startFrame <= s_curFrame && s_curFrame <= s_cues[i].endFrame)
            out[n++] = &s_cues[i];
    }
    return n;
}
