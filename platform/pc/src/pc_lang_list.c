/* pc_lang_list.c -- language-pack enumeration, compiled in both region cores: pure file I/O so a
 * JP session can list installed packs and queue one for a pending US-disc restart. Owns the one
 * manifest reader the loader and the picklist share. See docs/language-packs.md, "Runtime layout". */
#include <dirent.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "pc_lang.h"
#include "pc_platform.h"   /* PC_GetDeployDir */

/* Minimal manifest reads (same posture as pc_hdpack.c's HdManifestRead: the manifest is a
 * CONTROLLED file our own builder writes -- this is a field extractor, not a JSON parser). */
static int MiniJsonStr(const char *buf, const char *key, char *out, size_t n) {
    char pat[64];
    const char *p, *e;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(buf, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    p = strchr(p, '"');
    if (!p) return 0;
    p++;
    e = strchr(p, '"');
    if (!e || (size_t)(e - p) >= n) return 0;
    memcpy(out, p, (size_t)(e - p));
    out[e - p] = '\0';
    return 1;
}

static int MiniJsonInt(const char *buf, const char *key, int *out) {
    char pat[64];
    const char *p;
    snprintf(pat, sizeof pat, "\"%s\"", key);
    p = strstr(buf, pat);
    if (!p) return 0;
    p = strchr(p + strlen(pat), ':');
    if (!p) return 0;
    *out = atoi(p + 1);
    return 1;
}

#define LANG_GAME_ID "vandal-hearts-usa"
#define LANG_FORMAT  2      /* format 2 adds 1-byte/16-char item names; format-1 packs still load */

/* The manifest is the pack's identity (the folder name is a human convention): a missing, foreign
 * or newer manifest is refused loudly and the game continues in English. Returns 1 if the pack may
 * load. `quiet` silences stderr for the picklist; nameOut (optional) gets the display name or "". */
int PC_LangManifestCheck(const char *dir, int *formatOut, char *nameOut, size_t nameN,
                         int quiet) {
    char path[640], buf[2048], game[64], name[96], version[32];
    FILE *f;
    size_t n;
    int format = 0;
    snprintf(path, sizeof path, "%s/manifest.json", dir);
    f = fopen(path, "r");
    if (!f) {
        if (!quiet)
            fprintf(stderr, "[lang] %s: no manifest.json -- not a language pack (or built by a "
                            "pre-manifest tool; rebuild it)\n", dir);
        return 0;
    }
    n = fread(buf, 1, sizeof buf - 1, f);
    buf[n] = '\0';
    fclose(f);
    if (!MiniJsonStr(buf, "game", game, sizeof game) || strcmp(game, LANG_GAME_ID) != 0) {
        if (!quiet)
            fprintf(stderr, "[lang] %s: pack is for game \"%s\", this build is \"%s\" -- refused\n",
                    dir, MiniJsonStr(buf, "game", game, sizeof game) ? game : "?", LANG_GAME_ID);
        return 0;
    }
    if (!MiniJsonInt(buf, "format", &format) || format > LANG_FORMAT) {
        if (!quiet)
            fprintf(stderr, "[lang] %s: pack format v%d, this build reads v%d -- refused (update "
                            "the port, or rebuild the pack)\n", dir, format, LANG_FORMAT);
        return 0;
    }
    if (formatOut) *formatOut = format;
    if (nameOut && nameN) {
        if (!MiniJsonStr(buf, "name", nameOut, nameN)) nameOut[0] = '\0';
    }
    if (!quiet) {
        if (!MiniJsonStr(buf, "name", name, sizeof name)) snprintf(name, sizeof name, "(unnamed)");
        if (!MiniJsonStr(buf, "version", version, sizeof version)) version[0] = '\0';
        fprintf(stderr, "[lang] pack \"%s\"%s%s\n", name, version[0] ? " v" : "", version);
    }
    return 1;
}

/* Enumerate installed packs for the overlay picklist: every <deploy>/langpacks/<folder> that
 * PC_LangManifestCheck accepts (the loader's gate, applied quietly), so the picklist never offers
 * a pack the loader would refuse at boot. Returns the count; folders/names are parallel arrays. */
int PC_LangListPacks(char folders[][64], char names[][64], int max) {
    char deploy[512], root[560], pdir[640];
    DIR *d;
    struct dirent *e;
    int n = 0;
    if (!PC_GetDeployDir(deploy, sizeof deploy)) return 0;
    snprintf(root, sizeof root, "%s/langpacks", deploy);
    d = opendir(root);
    if (!d) return 0;
    while ((e = readdir(d)) != NULL && n < max) {
        if (e->d_name[0] == '.') continue;
        /* a folder name too long for the picklist buffers can't be a valid pack -- skip it
         * (also proves to -Wformat-truncation that truncation is handled, not ignored) */
        if (strlen(e->d_name) >= 64) continue;
        if (snprintf(pdir, sizeof pdir, "%s/%s", root, e->d_name) >= (int)sizeof pdir) continue;
        if (!PC_LangManifestCheck(pdir, NULL, names[n], 64, 1)) continue;
        memcpy(folders[n], e->d_name, strlen(e->d_name) + 1);   /* length-checked above */
        if (!names[n][0])                                        /* unnamed: show the folder */
            memcpy(names[n], e->d_name, strlen(e->d_name) + 1);
        n++;
    }
    closedir(d);
    return n;
}
