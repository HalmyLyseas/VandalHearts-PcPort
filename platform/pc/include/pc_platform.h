/* PC-only platform glue -- NOT part of the PSX API surface. These configure the PC backend
 * implementations (e.g. which disc image backs the virtual CD-ROM); game code never calls them,
 * only this project's own platform startup/test code does. */
#ifndef PLATFORM_PC_PLATFORM_H
#define PLATFORM_PC_PLATFORM_H

#include <stddef.h>   /* size_t */

/* The directory the end user's files live in: next to the .AppImage when packaged, else the
 * executable's own directory (cwd-independent). Used for the disc auto-detect, vandalhearts.ini,
 * and the saves folder, so they all resolve to one predictable place. Returns 1 on success. */
int PC_GetDeployDir(char *out, size_t outSize);

/* VH_VERBOSE=1 (env or ini): per-event backend chatter ([lang]/[HD]/[HDvideo] progress, per-asset
 * replacements, per-read substitutions). Default OFF: the console shows only the one-time boot
 * summary and every warning/refusal. Resolved once (pc_bootstrap.c). */
int PC_Verbose(void);

/* The resolved saves directory the game reads/writes its memory-card file in (defined in libkernel.c,
 * resolved once via PC_GetDeployDir). The save-management backend (pc_saves.c) uses it to keep
 * archives beside the active card. */
const char *PC_SaveDir(void);

/* Mounts a raw CD image (2352-byte/sector BIN, as produced by `chdman
 * extractcd`) as the virtual disc libcd.c reads from. Returns nonzero on
 * success. Must be called before CdInit(). */
int PC_CdMount(const char *discImagePath);
int PC_CdDiscSignatureOk(void);   /* 1 if the mounted image has Vandal Hearts's boot signature (PS-X EXE @ LBA 23) */
long long PC_CdImageBytes(void);  /* mounted image size (-1 if none) -- the truncation gate (% 2352) */

/* Which byte-compatible Vandal Hearts release the mounted disc is. USA (SLUS-00447) and Asia
 * (SCPS-45183) share identical game code (only the memory-card id differs) and both run on this
 * port. UNKNOWN = a PS-X EXE disc not specifically recognized (still boots). See libcd.c. */
typedef enum {
    VH_DISC_UNKNOWN = 0,
    VH_DISC_USA,      /* SLUS-00447 */
    VH_DISC_ASIA,     /* SCPS-45183 -- byte-identical to USA except the memory-card id */
    VH_DISC_JAPAN     /* SLPM-86007 -- its own build (the jp/ tree); different disc layout */
} PC_DiscRelease;
PC_DiscRelease PC_CdDiscRelease(void);

/* Region identity of THIS build. The port Makefile/CMake define VH_REGION_JP for the JP core, and
 * every region-derived value hangs off these so no card id, pack id or boot LBA is hardcoded twice.
 * JP values come from the byte-exact SLPM_860.07 (card id string at VRAM 0x800f76a9). */
#ifdef VH_REGION_JP
#define VH_REGION_NAME        "Japan (SLPM-86007)"
#define VH_ACTIVE_CARD_NAME   "BISLPM-86007VH"
#define VH_HD_GAME_ID         "SLPM-86007"
#define VH_REGION_BOOT_LBA    15200
#define VH_REGION_DISC        VH_DISC_JAPAN
#else
#define VH_REGION_NAME        "USA (SLUS-00447) / Asia (SCPS-45183)"
#define VH_ACTIVE_CARD_NAME   "BASLUS-00447VH"
#define VH_HD_GAME_ID         "SLUS-00447"
#define VH_REGION_BOOT_LBA    23
#define VH_REGION_DISC        VH_DISC_USA   /* ASIA also accepted -- same master */
#endif
/* Fatal, user-actionable disc error: stderr + a message box, then exit (pc_bootstrap.c). Used at
 * mount validation and by libcd.c's per-read corruption guards, so a damaged image is reported
 * instead of hanging the game silently. */
void PC_FatalDiscError(const char *title, const char *body, const char *path);

/* Opens an SDL2 host window for the GPU backend to present into (Metal on macOS, OpenGL elsewhere).
 * Optional: if never called, PC_GpuPresent() just skips the windowing part, so libgpu.c's
 * VRAM/rasterizer/OT logic is fully testable headlessly. Returns nonzero on success. */
int PC_GpuInit(int width, int height, const char *title);
void PC_GpuGetWindowSize(int *w, int *h, int *scale);   /* actual scaled window size + VH_SCALE factor */
void PC_ShowErrorBox(const char *title, const char *body);   /* modal SDL error dialog; safe pre-SDL_Init, no-op headless */

/* Video settings driven by the in-game options overlay. The window is resizable and the present
 * path re-letterboxes each frame, so these just resize / toggle it. g_vhScale (1..8) and
 * g_vhFullscreen (0/1) are the live values the overlay reads for display. */
extern int g_vhScale;
extern int g_vhFullscreen;
void PC_GpuSetScale(int scale);
void PC_GpuSetFullscreen(int on);

/* Internal-resolution supersampling. g_vhInternalScale is the live overlay setting (1 = off .. 4);
 * PC_GpuSetInternalScale changes it live (reallocation-free). Backend-only, default off. */
extern int g_vhInternalScale;
void PC_GpuSetInternalScale(int scale);

/* Online CPU count via SDL_GetCPUCount, so backends (the threaded rasterizer) can size their thread
 * pool without pulling in SDL or a per-OS sysconf/GetSystemInfo branch. Returns >= 1. */
int PC_CpuCount(void);

/* Called by PutDispEnv() every frame: blits a VRAM sub-rect (BGR555, vramW
 * halfwords/line) to the window opened by PC_GpuInit(). No-ops if
 * PC_GpuInit() was never called. */
void PC_GpuPresent(unsigned short *vram, int vramW, int vramH,
                    int x, int y, int w, int h);

/* Fullscreen movie (MDEC/FMV) overlay. libcd decodes each .STR frame to a 320x240 BGR555 buffer
 * and registers it here; PC_GpuPresent shows it fullscreen while set. Pass NULL to disable. */
void PC_GpuSetMovieOverlay(const unsigned short *bgr555, int w, int h);
/* Pump + drain the SDL event queue (quit/Escape handling, compositor ping). Called from the
 * present path and every VSync so long wait loops stay "responsive" to the window manager. */
void PC_GpuPumpEvents(void);

/* In-game options overlay: paints the pc_overlay.c menu over the presented frame. Called by
 * PC_GpuPresent when PC_OverlayIsOpen(). w,h are the native scratch dimensions. */
void PC_GpuDrawOverlay(int w, int h);

/* Decode one BS (v2/v3) MDEC bitstream frame to BGR555. Reimplemented from psx-spx (pc_mdec.c). */
int PC_MdecDecodeBS(const unsigned char *bs, int bsLen, int w, int h, unsigned short *outBGR555);

/* Debug camera OSD: libetc.c formats the live camera pose into this buffer each VSync and
 * PC_GpuPresent() renders it top-left (mirroring BizHawk's RAM Watch) when VH_CAM_OSD is set, so
 * captured frames carry their own pose for matched-pose comparison. Empty string = nothing drawn. */
extern char g_camOsdText[96];

/* Refreshes g_camOsdText from the live camera globals. Implemented in libetc.c
 * (which has the game struct layouts); called by libgpu.c right before the
 * present so the label matches the frame being shown (no 1-frame lag). */
void PC_UpdateCamOsd(void);

/* Debug sprite-pipeline log: called from RenderUnitSprite (core/object.c, PC_DEBUG_SPRITE_LOG) to
 * record each unit sprite's fate -- tile position, render window, cull result, projected screen
 * coords, OT index. Writes vh_sprite_fate.csv when VH_SPRITE_LOG is set; no-op otherwise. */
void PC_DebugTerrainTile(int otz, int r0, int g0, int b0);
void PC_GteProjEntry(int back, int *sx, int *sy, int *ir1, int *ir2, int *ir3, int *sz3, int *nout);
void PC_DebugSpriteLog(int tileX, int tileZ, int winX, int winZ, int mapSX, int mapSZ,
                       int culled, int gfxIdx, int sx, int sy, int otz, int otIdx);

/* Reads the GTE projection state used by the last TransformOne (defined in libgte.c). */
void PC_GteDebugState(int *ofx, int *ofy, int *h, int *rt00, int *rt02, int *rt22,
                      int *trx, int *trz);
int PC_GteLastOtz(void); /* last AVSZ4 terrain OTZ */
int PC_GteZsf4(void);    /* current zsf4 */

/* In-game options overlay: persist one `VH_*` setting to vandalhearts.ini (in the deploy dir),
 * rewriting only that key's line; appended under `[section]` if absent. Returns 1 on success, 0 on
 * I/O failure. See docs/pc-port/bootstrap.md, "Writing a setting back to `vandalhearts.ini`". */
int PC_SaveIniConfig(const char *section, const char *key, const char *value);

#endif
