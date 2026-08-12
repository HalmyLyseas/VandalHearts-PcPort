/*
 * Stage-3 (1.2b) save management -- file-level card archiving.
 *
 * Vandal Hearts uses ONE memory-card file, "BASLUS-00447VH" (core/card.c), holding all 3 in-game slots.
 * To beat the 3-slot limit *without* diverging the save format (each archive stays a byte-identical,
 * real-hardware-valid card), this backend keeps whole-card snapshots in a hidden "<saves>/.archive/"
 * folder -- dot-prefixed so the game's own firstfile("bu00:*") enumeration skips it.
 *
 * Archives are auto-named "BASLUS-00447VH.<YYYYMMDD-HHMMSS>" (PC clock; no gamepad text entry). This
 * layer is pure file I/O; the options overlay (pc_overlay.c) drives it and owns the UI + confirms.
 */
#ifndef PLATFORM_PC_SAVES_H
#define PLATFORM_PC_SAVES_H

typedef struct {
    char file[64];    /* archive filename within .archive/ (e.g. BASLUS-00447VH.20260725-153045) */
    char label[24];   /* human display, parsed from the name: "2026-07-25 15:30"                 */
    long mtime;       /* modification time (seconds); tie-breaker / fallback ordering             */
    int  active;      /* 1 if byte-identical to the current active card (i.e. this IS the loaded state) */
} PC_SaveArchive;

/* The readable content of a card: its three regular save slots (the in-battle "continue" record has
 * no listing caption, so it's naturally excluded). `slot[i]` is the game's own caption, uppercased for
 * the overlay font ("CHAP. 1 SCT. 1  L5  0:06"), or "" when that slot is empty. */
typedef struct {
    int  occupied[3];
    char slot[3][40];
} PC_SaveCard;

/* Parse the listing block of archive `file` into `out` (3 regular slots). Returns 1 on success. */
int  PC_SaveReadCard(const char *file, PC_SaveCard *out);

/* Fill `out` (up to `cap`) with the archives, newest first. Returns the count (0 if none / no folder). */
int  PC_SaveArchiveList(PC_SaveArchive *out, int cap);

/* Allocate and return the complete archive list. The caller owns `*out` and releases it with
 * PC_SaveArchiveListFree. Returns -1 on allocation/I/O failure, otherwise the count (possibly 0). */
int  PC_SaveArchiveListAlloc(PC_SaveArchive **out);
void PC_SaveArchiveListFree(PC_SaveArchive *out);

/* 1 if an active card file exists (so "Back up" has something to copy). */
int  PC_SaveHasActive(void);

/* Copy the active card -> a new timestamped archive. Returns 1 on success, 0 on failure
 * (no active card, or I/O error). Creates .archive/ on demand. */
int  PC_SaveBackupCurrent(void);

/* Overwrite the active card with archive `file` (a name from PC_SaveArchiveList). Returns 1 on
 * success. The caller is responsible for any "back up current first" step + confirmation. */
int  PC_SaveRestore(const char *file);

/* Delete archive `file`. Returns 1 on success. */
int  PC_SaveDeleteArchive(const char *file);

#endif
