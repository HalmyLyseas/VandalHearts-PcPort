/*
 * PC-only platform glue -- NOT part of the PSX API surface. Functions here
 * configure the PC backend implementations (e.g. which disc image file
 * backs the virtual CD-ROM). Real game code never calls these; only this
 * project's own platform startup/test code does.
 */
#ifndef PLATFORM_PC_PLATFORM_H
#define PLATFORM_PC_PLATFORM_H

/* Mounts a raw CD image (2352-byte/sector BIN, as produced by `chdman
 * extractcd`) as the virtual disc libcd.c reads from. Returns nonzero on
 * success. Must be called before CdInit(). */
int PC_CdMount(const char *discImagePath);
int PC_CdDiscSignatureOk(void);   /* 1 if the mounted image has Vandal Hearts (USA)'s boot signature */

/* Opens an SDL2+OpenGL window for the GPU backend to present into. Optional:
 * if never called, PC_GpuPresent() below just no-ops the windowing part,
 * so libgpu.c's VRAM/rasterizer/OT logic is fully testable headlessly.
 * Returns nonzero on success. */
int PC_GpuInit(int width, int height, const char *title);
void PC_GpuGetWindowSize(int *w, int *h, int *scale);   /* actual scaled window size + VH_SCALE factor */

/* Called by PutDispEnv() every frame: blits a VRAM sub-rect (BGR555, vramW
 * halfwords/line) to the window opened by PC_GpuInit(). No-ops if
 * PC_GpuInit() was never called. */
void PC_GpuPresent(unsigned short *vram, int vramW, int vramH,
                    int x, int y, int w, int h);

/* Fullscreen movie (MDEC/FMV) overlay. libcd decodes each .STR frame to a 320x240 BGR555 buffer
 * and registers it here; PC_GpuPresent shows it fullscreen while set. Pass NULL to disable. */
void PC_GpuSetMovieOverlay(const unsigned short *bgr555, int w, int h);

/* Stage-3 (1.1C) in-game options overlay: paints the pc_overlay.c menu over the presented frame.
 * Called by PC_GpuPresent when PC_OverlayIsOpen(). w,h are the native scratch dimensions. */
void PC_GpuDrawOverlay(int w, int h);

/* Decode one BS (v2/v3) MDEC bitstream frame to BGR555. Reimplemented from psx-spx (pc_mdec.c). */
int PC_MdecDecodeBS(const unsigned char *bs, int bsLen, int w, int h, unsigned short *outBGR555);

/* Debug camera OSD (feedback-11 follow-up): libetc.c formats the live camera
 * pose into this buffer each VSync; PC_GpuPresent() renders it top-left of the
 * window (mirroring BizHawk's RAM Watch) when the VH_CAM_OSD env var is set, so
 * captured frames carry their own pose for matched-pose comparison against real
 * hardware. Empty string = nothing drawn. */
extern char g_camOsdText[96];

/* Refreshes g_camOsdText from the live camera globals. Implemented in libetc.c
 * (which has the game struct layouts); called by libgpu.c right before the
 * present so the label matches the frame being shown (no 1-frame lag). */
void PC_UpdateCamOsd(void);

/* Debug sprite-pipeline log (feedback-14 follow-up): called from RenderUnitSprite (object.c,
 * gated by PC_DEBUG_SPRITE_LOG / SPRITE_LOG=1) to record each unit sprite's fate at the
 * matched-pose repro -- tile position, render window, cull result, projected screen coords, and
 * OT index -- so we can tell whether missing units are culled by winOrigin, projected off-screen,
 * or depth-gated. Writes vh_sprite_fate.csv when VH_SPRITE_LOG env var is set; no-op otherwise. */
void PC_DebugTerrainTile(int otz, int r0, int g0, int b0);
void PC_GteProjEntry(int back, int *sx, int *sy, int *ir1, int *ir2, int *ir3, int *sz3, int *nout);
void PC_DebugSpriteLog(int tileX, int tileZ, int winX, int winZ, int mapSX, int mapSZ,
                       int culled, int gfxIdx, int sx, int sy, int otz, int otIdx);

/* Reads the GTE projection state used by the last TransformOne (defined in libgte.c). */
void PC_GteDebugState(int *ofx, int *ofy, int *h, int *rt00, int *rt02, int *rt22,
                      int *trx, int *trz);
int PC_GteLastOtz(void); /* last AVSZ4 terrain OTZ */
int PC_GteZsf4(void);    /* current zsf4 */

/* Stage-3 in-game options overlay: persist a single `VH_*` setting back to vandalhearts.ini
 * (next to the executable, or the .AppImage under AppImage). Surgical -- rewrites only the one
 * key's line in place, preserving every other line, comment and section, and keeping any inline
 * comment on the key's own line. If the key is absent it is appended under `[section]` (the section
 * is created if missing); if the file itself is absent a minimal one is created. `section` is used
 * only for that append path (a header for readability -- our loader ignores headers). Returns 1 on
 * success, 0 on any I/O failure (the in-memory setting still applies for the session either way). */
int PC_SaveIniConfig(const char *section, const char *key, const char *value);

#endif
