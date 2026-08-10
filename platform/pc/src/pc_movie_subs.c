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
#include <sys/stat.h>

static PC_MovieCue *s_cues = NULL;
static int s_cueCount = 0;
static int s_cuesOwned = 0;      /* dev-file cues are ours to free; pack sets are borrowed */
static int s_curFrame = 0;
static int s_loadedLBA = -1;

/* Language-pack cue sets (K_CUES): parsed once at pack load, live for the process. */
#define PACK_MAX_MOVIES 32
static struct { int lba; PC_MovieCue *cues; int count; } s_pack[PACK_MAX_MOVIES];
static int s_packN = 0;

static void subsFree(void) {
    if (s_cuesOwned) free(s_cues);
    s_cues = NULL; s_cueCount = 0; s_cuesOwned = 0; s_curFrame = 0; s_loadedLBA = -1;
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
                size_t sl = strlen(line + 5);          /* explicit bounded copy: truncation is */
                if (sl > PC_SUBS_MAX_TEXT - 1)         /* intended for over-long lines, and this */
                    sl = PC_SUBS_MAX_TEXT - 1;         /* form says so without a strncpy warning */
                memcpy(cur.lines[cur.lineCount], line + 5, sl);
                cur.lines[cur.lineCount][sl] = '\0';
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
    s_cuesOwned = 1;
    s_loadedLBA = baseLBA;
    fprintf(stderr, "PC_MovieSubs: %d cues loaded for movie %x\n", s_cueCount, (unsigned)baseLBA);
}

/* Parse the pack's K_CUES blob (see lang_build.py build_cues for the layout):
 * u32 movieCount; per movie u32 lba + u32 cueCount; per cue u32 start,end; u16 x,y,w,h;
 * u8 lineCount; per line u8 len + UTF-8 bytes. Every count and offset is hostile until checked;
 * a malformed record aborts the parse with a note rather than trusting what remains. */
static unsigned PackRdU32(const unsigned char *p) {
    return (unsigned)p[0] | ((unsigned)p[1] << 8) | ((unsigned)p[2] << 16) | ((unsigned)p[3] << 24);
}

void PC_MovieSubsLoadPack(const unsigned char *p, unsigned len) {
    unsigned off = 0, movies, m;
    if (len < 4) return;
    movies = PackRdU32(p); off = 4;
    if (movies > PACK_MAX_MOVIES) {
        fprintf(stderr, "[lang] cues: %u movies claimed -- truncated/hostile section, ignored\n", movies);
        return;
    }
    for (m = 0; m < movies; m++) {
        unsigned lba, count, i;
        PC_MovieCue *cues;
        if (off + 8 > len) goto malformed;
        lba = PackRdU32(p + off); count = PackRdU32(p + off + 4); off += 8;
        if (count == 0 || count > 512 || s_packN >= PACK_MAX_MOVIES) goto malformed;
        cues = (PC_MovieCue *)calloc(count, sizeof(PC_MovieCue));
        if (!cues) return;
        for (i = 0; i < count; i++) {
            PC_MovieCue *c = &cues[i];
            unsigned li, nl;
            if (off + 17 > len) { free(cues); goto malformed; }
            c->startFrame = (int)PackRdU32(p + off);
            c->endFrame   = (int)PackRdU32(p + off + 4);
            c->x = p[off + 8]  | (p[off + 9]  << 8);
            c->y = p[off + 10] | (p[off + 11] << 8);
            c->w = p[off + 12] | (p[off + 13] << 8);
            c->h = p[off + 14] | (p[off + 15] << 8);
            nl = p[off + 16]; off += 17;
            if (nl == 0 || nl > PC_SUBS_MAX_LINES ||
                c->startFrame < 1 || c->endFrame < c->startFrame ||
                c->x + c->w > 320 || c->y + c->h > 240) { free(cues); goto malformed; }
            for (li = 0; li < nl; li++) {
                unsigned sl;
                if (off + 1 > len) { free(cues); goto malformed; }
                sl = p[off]; off += 1;
                if (sl >= PC_SUBS_MAX_TEXT || off + sl > len) { free(cues); goto malformed; }
                memcpy(c->lines[li], p + off, sl);
                c->lines[li][sl] = '\0';
                off += sl;
            }
            c->lineCount = (int)nl;
        }
        s_pack[s_packN].lba = (int)lba;
        s_pack[s_packN].cues = cues;
        s_pack[s_packN].count = (int)count;
        s_packN++;
    }
    fprintf(stderr, "[lang] cues: subtitles for %d video(s) loaded\n", s_packN);
    return;
malformed:
    fprintf(stderr, "[lang] cues: malformed section at byte %u -- remaining movies ignored\n", off);
}

void PC_MovieSubsOpen(int baseLBA) {
    const char *path;
    struct stat st;
    int i;
    if (s_loadedLBA == baseLBA) { s_curFrame = 0; return; }   /* stream-start block re-runs */
    subsFree();
    path = getenv("VH_MOVIE_SUBS");
    if (!path || !*path) {
        /* No dev override: the active language pack's cue sets (K_CUES), if any. */
        for (i = 0; i < s_packN; i++) {
            if (s_pack[i].lba == baseLBA) {
                s_cues = s_pack[i].cues; s_cueCount = s_pack[i].count;
                s_cuesOwned = 0; s_loadedLBA = baseLBA;
                return;
            }
        }
        return;
    }
    if (stat(path, &st) == 0 && S_ISDIR(st.st_mode)) {
        /* Directory form: one <baseLBA hex>.txt per movie -- one setting covers every video,
         * and it mirrors the langpack layout (cues keyed like hdpacks/videos/<lba>.mp4). */
        char full[1024];
        struct stat fs;
        snprintf(full, sizeof(full), "%s/%x.txt", path, (unsigned)baseLBA);
        if (stat(full, &fs) != 0) return;        /* no cues for this movie (logo/title/...) -- silent */
        subsLoad(full, baseLBA);
    } else {
        subsLoad(path, baseLBA);
    }
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
