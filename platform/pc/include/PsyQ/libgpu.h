/*
 * PC-backend replacement for the PSX SDK's libgpu.h GPU (rendering) interface.
 *
 * Clean-room reimplementation: struct layouts and function signatures are
 * functional facts already legitimately extracted via
 * exchange/02-phase-c-interface-contract.md -- no text from Sony's original
 * header. Scope covers only what the game's source actually calls (30 files,
 * 41 symbols; only 4 primitive types confirmed used: POLY_F4, POLY_FT4,
 * SPRT, TILE).
 *
 * `tag`'s representation: real hardware packs a primitive's "next" pointer
 * into a 24-bit bitfield (`P_TAG.addr:24`) sharing a 32-bit word with
 * `len:8`, because real PS1 addresses fit in 21 bits. `Graphics.ot` is
 * declared `u32 ot[OT_SIZE]` in include/graphics.h -- a REAL project struct
 * we can't resize, fixed at 4 bytes/slot, matching the original
 * 32-bit-pointer PS1 memory model. Two representations were tried and
 * rejected before landing here (see exchange/12-phase-c-bootstrap.md):
 *   1. Widen `tag` to a full 64-bit `unsigned int` holding a real host pointer --
 *      overran the real 4-byte-wide `Graphics.ot` array by 2x (found via
 *      AddressSanitizer, not standalone testing against our own
 *      self-declared OT arrays).
 *   2. A small-integer handle registry (PC_GPU_RegisterHandle/
 *      PC_GPU_ResolveHandle) mapping handle<->pointer, reset each
 *      ClearOTag call -- fit the 4-byte slot, but broke PS1-style
 *      double buffering: the game builds next frame's OT while drawing the
 *      previous frame's already-built one, and the registry's single
 *      global counter is shared across both, so clearing the *next* OT
 *      silently invalidated every handle already registered in the
 *      *previous* one before DrawOTag ever walked it -- every OT came up
 *      empty (confirmed via gdb: PC_GPU_RegisterHandle hit 300+ times over
 *      12s, FillQuad/FillRect hit zero times), a permanent black screen.
 * This build is `-m32` (see exchange/12-phase-c-bootstrap.md's "32-bit
 * decision") -- host pointers are already 4 bytes here, the same width as
 * `tag`, so the simplest and most literal option is to store the pointer
 * directly, exactly like the original 32-bit game/hardware assumed. This
 * stops working the moment the project migrates to 64-bit (pointers won't
 * fit in `tag` again) -- whichever of the two rejected approaches gets
 * revived then must also solve the double-buffering lifetime problem this
 * one exposed, not just the 4-byte-width one.
 */
#ifndef PLATFORM_PC_PSYQ_LIBGPU_H
#define PLATFORM_PC_PSYQ_LIBGPU_H

#include "sys/types.h"
#include "types.h" /* u32, for P_TAG.tag -- see the file-header comment on tag's width */

typedef struct {
    short x, y;
    short w, h;
} RECT;

typedef struct {
    u32 tag;
    unsigned int code[15];
} DR_ENV;

typedef struct {
    RECT   clip;
    short  ofs[2];
    RECT   tw;
    u_short tpage;
    u_char dtd;
    u_char dfe;
    u_char isbg;
    u_char r0, g0, b0;
    DR_ENV dr_env;
} DRAWENV;

typedef struct {
    RECT   disp;
    RECT   screen;
    u_char isinter;
    u_char isrgb24;
    u_char pad0, pad1;
} DISPENV;

/* P_TAG's addr is widened from the real 24-bit bitfield -- see file header
 * comment. Every primitive struct below starts with the same tag/r0/g0/b0/
 * code layout so a `(P_TAG *)p` cast works uniformly, matching the real
 * header's design intent even though the field width differs. */
typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
} P_TAG;

typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
    short  x0, y0;
    short  x1, y1;
    short  x2, y2;
    short  x3, y3;
} POLY_F4; /* Flat Quadrangle */

typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
    short  x0, y0;
    u_char u0, v0; u_short clut;
    short  x1, y1;
    u_char u1, v1; u_short tpage;
    short  x2, y2;
    u_char u2, v2; u_short pad1;
    short  x3, y3;
    u_char u3, v3; u_short pad2;
} POLY_FT4; /* Flat Textured Quadrangle */

typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
    short  x0, y0;
    u_char u0, v0; u_short clut;
    short  w, h;
} SPRT; /* free-size Sprite */

typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
    short  x0, y0;
    short  w, h;
} TILE; /* free-size Tile (solid-color rect, no texture) */

/* Real hardware's DR_MODE is `{unsigned int tag; unsigned int code[2];}` -- code[0]/
 * code[1] are the raw GP0(E1h)/GP0(E2h) command words, never inspected by
 * game code directly (only ever built by SetDrawMode() and consumed by our
 * own DrawOTag()). We give it the same tag/r0/g0/b0/code header every other
 * primitive struct has instead, so DrawOTag's generic P_TAG-cast type
 * dispatch (getcode()) works uniformly -- the real code[2] payload doesn't
 * leave room for that header without colliding, and nothing outside
 * libgpu.c reads these fields to notice the difference. */
typedef struct {
    u32 tag;
    u_char r0, g0, b0, code;
    unsigned int tpage;
} DR_MODE; /* Drawing Mode */

typedef struct {
    unsigned int  mode;
    RECT   *crect;
    unsigned int *caddr;
    RECT   *prect;
    unsigned int *paddr;
} TIM_IMAGE;

/* Our own internal primitive-type discriminators, stored in the `code` byte
 * by SetPolyF4/SetPolyFT4/SetSprt/SetTile/SetDrawMode. Since these are all
 * real functions (not macros) we implement ourselves, we're free to pick our
 * own scheme rather than replicate Sony's raw GP0 command byte values --
 * nothing outside libgpu.c ever reads `code` directly. */
#define PC_GPU_PRIM_POLY_F4  1
#define PC_GPU_PRIM_POLY_FT4 2
#define PC_GPU_PRIM_SPRT     3
#define PC_GPU_PRIM_TILE     4
#define PC_GPU_PRIM_DR_MODE  5

#define setRECT(r, _x, _y, _w, _h) \
    ((r)->x = (_x), (r)->y = (_y), (r)->w = (_w), (r)->h = (_h))

#define setRGB0(p, _r0, _g0, _b0) \
    ((p)->r0 = (_r0), (p)->g0 = (_g0), (p)->b0 = (_b0))

#define setXY0(p, _x0, _y0) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0))

#define setXYWH(p, _x0, _y0, _w, _h) \
    ((p)->x0 = (_x0), (p)->y0 = (_y0), \
     (p)->x1 = (_x0) + (_w), (p)->y1 = (_y0), \
     (p)->x2 = (_x0), (p)->y2 = (_y0) + (_h), \
     (p)->x3 = (_x0) + (_w), (p)->y3 = (_y0) + (_h))

#define setUV0(p, _u0, _v0) \
    ((p)->u0 = (_u0), (p)->v0 = (_v0))

#define setUVWH(p, _u0, _v0, _w, _h) \
    ((p)->u0 = (_u0), (p)->v0 = (_v0), \
     (p)->u1 = (_u0) + (_w), (p)->v1 = (_v0), \
     (p)->u2 = (_u0), (p)->v2 = (_v0) + (_h), \
     (p)->u3 = (_u0) + (_w), (p)->v3 = (_v0) + (_h))

#define setWH(p, _w, _h) ((p)->w = (_w), (p)->h = (_h))

#define setTPage(p, tp, abr, x, y) ((p)->tpage = GetTPage(tp, abr, x, y))
#define setClut(p, x, y)           ((p)->clut = GetClut(x, y))

/* ⚠️ Stage 2.3: `tag` holds a TOKEN (an index into libgpu.c's per-frame OT registry), NOT an
 * address -- see the token-bridge comment in platform/pc/src/libgpu.c. Only AddPrim/ClearOTag/
 * DrawOTag may mint or resolve tokens, so these two macros must NEVER be used to build an OT
 * link: storing a raw pointer through setaddr() produces a value DrawOTag resolves to NULL,
 * which TERMINATES the whole walk and silently drops every primitive after it in the chain.
 *
 * That is exactly what happened when the token bridge first landed: `addPrim` below was left as
 * a raw setaddr/getaddr pair while `AddPrim` was converted, so engine.c's compass
 * (ot[OT_SIZE-5]) and screen_effects.c's overlays poisoned the tail of the walk -- killing the
 * compass, the logo and every textbox, while 3D geometry (earlier buckets, walked first) looked
 * perfect. Kept only for raw tag inspection; `nextPrim`/`isendprim` are likewise token-domain. */
#define setaddr(p, _addr) (((P_TAG *)(p))->tag = (u32)(size_t)(_addr))
#define getaddr(p)        ((unsigned int)(size_t)(((P_TAG *)(p))->tag))
#define setlen(p, _len)   ((void)(p), (void)(_len)) /* no length field to set -- kept for source compatibility */
#define getlen(p)         0
#define setcode(p, _code) (((P_TAG *)(p))->code = (u_char)(_code))
#define getcode(p)        (((P_TAG *)(p))->code)

#define nextPrim(p)  ((void *)getaddr(p))
#define isendprim(p) (((P_TAG *)(p))->tag == 0) /* raw zero-check -- no resolve needed */

/* MUST go through the real function so the primitive gets a registry token (see above). */
#define addPrim(ot, p)       AddPrim((ot), (p))
#define addPrims(ot, p0, p1) (AddPrim((ot), (p1)), AddPrim((ot), (p0)))

#define setPolyF4(p)  SetPolyF4(p)

/* `code`'s low nibble is our primitive-type tag (PC_GPU_PRIM_*); bit 0x80 is
 * free for the real semi-transparency flag since our tags only use 1-5.
 *
 * BUT: not all game code builds a primitive via our SetPolyF4/SetPolyFT4/
 * etc setters (which are the only place that writes our PC_GPU_PRIM_* tag).
 * AddObjPrim_Gui and friends (object.c, used across 10 files -- the
 * primary way most sprites in this game actually get drawn) set `.code`
 * directly to the real hardware GP0 command byte instead:
 * `poly->code = GPU_CODE_POLY_FT4` (0x2c, optionally |GPU_CODE_SEMI_TRANS
 * =0x02) -- confirmed via grep to be the ONLY raw `.code =` assignment
 * anywhere in src (any .c), always for POLY_FT4. Primitives built this way
 * never carried our discriminator at all, so DrawOTag's dispatch silently
 * skipped them -- found from a real screenshot (a splash-screen sprite
 * never drew, leaving stale VRAM content visible instead), not a
 * hypothetical. PC_GPU_PRIM_TYPE/PC_GPU_IS_SEMI now recognize the real
 * 0x2c-based code (masking out its own semi-trans bit) as POLY_FT4 in
 * addition to our own 1-5 scheme -- see exchange/12-phase-c-bootstrap.md. */
#define PC_GPU_REAL_CODE_POLY_FT4 0x2c
#define setSemiTrans(p, abe) \
    ((abe) ? setcode(p, getcode(p) | 0x80) : setcode(p, getcode(p) & ~0x80))
#define PC_GPU_PRIM_TYPE(p) \
    (((getcode(p) & ~0x02) == PC_GPU_REAL_CODE_POLY_FT4) ? PC_GPU_PRIM_POLY_FT4 : (getcode(p) & 0x7F))
#define PC_GPU_IS_SEMI(p) \
    (((getcode(p) & ~0x02) == PC_GPU_REAL_CODE_POLY_FT4) ? (getcode(p) & 0x02) : (getcode(p) & 0x80))

#define WAIT_TIME 0x800000

DISPENV *GetDispEnv(DISPENV *env);
DISPENV *PutDispEnv(DISPENV *env);
DISPENV *SetDefDispEnv(DISPENV *env, int x, int y, int w, int h);
DRAWENV *PutDrawEnv(DRAWENV *env);
DRAWENV *SetDefDrawEnv(DRAWENV *env, int x, int y, int w, int h);

TIM_IMAGE *ReadTIM(TIM_IMAGE *timimg);
int OpenTIM(unsigned int *addr);

int ResetGraph(int mode);
int SetGraphDebug(int level);
void SetDispMask(int mask);
int DrawSync(int mode);

int LoadImage(RECT *rect, unsigned int *p);
int StoreImage(RECT *rect, unsigned int *p);
int MoveImage(RECT *rect, int x, int y);
int ClearImage(RECT *rect, u_char r, u_char g, u_char b);

u_short GetTPage(int tp, int abr, int x, int y);
u_short GetClut(int x, int y);
void SetDrawMode(DR_MODE *p, int dfe, int dtd, int tpage, RECT *tw);

unsigned int *ClearOTag(unsigned int *ot, int n);
void AddPrim(void *ot, void *p);
void DrawOTag(unsigned int *p);

void SetSemiTrans(void *p, int abe);
void SetPolyF4(POLY_F4 *p);
void SetPolyFT4(POLY_FT4 *p);
void SetSprt(SPRT *p);
void SetTile(TILE *p);

int FntPrint(const char *fmt, ...);
unsigned int *FntFlush(int id);

#endif
