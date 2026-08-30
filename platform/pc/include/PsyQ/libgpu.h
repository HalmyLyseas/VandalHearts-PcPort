/* Clean-room PC replacement for the PsyQ libgpu.h interface: struct layouts and signatures only, for the
 * surface the game references. `tag` holds a 32-bit TOKEN into libgpu.c's per-frame OT registry, never a
 * host pointer. See docs/pc-port/subsystems/gpu.md, "Primitive packet layout in the clean-room header". */
#ifndef PLATFORM_PC_PSYQ_LIBGPU_H
#define PLATFORM_PC_PSYQ_LIBGPU_H

#include "sys/types.h"
#include "types.h" /* u32, for the 32-bit P_TAG.tag token */

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

/* P_TAG.tag replaces the hardware's 24-bit addr / 8-bit len word with a 32-bit token. Every primitive
 * struct below starts with the same tag/r0/g0/b0/code layout so a `(P_TAG *)p` cast works uniformly,
 * as in the real header. */
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

/* Hardware DR_MODE is `{tag; code[2]}` (raw GP0(E1h)/GP0(E2h) words). It gets the common
 * tag/r0/g0/b0/code header instead so DrawOTag's generic getcode() dispatch works; only SetDrawMode
 * builds it and only DrawOTag reads it (r0/g0/b0 carry dtd + the texture window, see libgpu.c). */
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

/* Internal primitive-type discriminators stored in the `code` byte by SetPolyF4/SetPolyFT4/SetSprt/
 * SetTile/SetDrawMode. These are real functions here, so the scheme need not match Sony's GP0 command
 * bytes; nothing outside libgpu.c reads `code` directly (but see PC_GPU_PRIM_TYPE below). */
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

/* `tag` holds a TOKEN, not an address: only AddPrim/ClearOTag/DrawOTag may mint or resolve one. A raw
 * pointer stored through setaddr() resolves to NULL in DrawOTag, which TERMINATES the walk and drops
 * every later primitive. Kept for raw tag inspection only; nextPrim/isendprim are token-domain too. */
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

/* `code`'s low nibble is the PC_GPU_PRIM_* tag; bit 0x80 is the semi-transparency flag. AddObjPrim_Gui
 * and friends (core/object.c) instead write the real GP0 byte, `poly->code = GPU_CODE_POLY_FT4` (0x2c,
 * optionally | 0x02 semi-trans), so the decoders below accept both schemes. See gpu.md, "Gotchas". */
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
