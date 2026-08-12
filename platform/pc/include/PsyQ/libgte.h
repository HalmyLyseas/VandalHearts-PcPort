/*
 * ⚠️ WIDTH RULE (Stage 2.3, 2026-07-21): PSX `int` is ALWAYS 32-bit. On an LP64 host `int`
 * is 64-bit, which silently changed MATRIX from 32 to 48 bytes (t[3] moving from offset 20 to
 * 24) and made every `int *` out-parameter write 8 bytes into the 4-byte `int` locals that
 * src/ passes -- a stack smash that killed the -m64 build before the first MDEC frame.
 * Every `int` here is therefore `int` / `unsigned int`. Do NOT reintroduce `int` in this or any other
 * PC-owned PsyQ header.
 *
 * PC-backend replacement for the PSX SDK's libgte.h GTE (Geometry Transformation
 * Engine) interface.
 *
 * Clean-room reimplementation: struct layouts and function signatures are
 * functional facts (already known precisely from the Phase B byte-exact
 * decompile of src/core/text.c, which reconstructed MATRIX/VECTOR/SVECTOR from real
 * data) -- no text from Sony's original header. Behavior is implemented against
 * the public Nocash PSX Specifications hardware reference (psx-spx), not
 * against any Sony documentation.
 *
 * Scope: the ~25 high-level functions used by 37 of 38 GTE-touching files (per
 * exchange/02-phase-c-interface-contract.md). These only need to be
 * geometrically correct, not cycle/bit-exact -- backed by the same software
 * GTE core as the raw gte_* macros (see platform/pc/include/inline_gte.h),
 * just entered through a friendlier API.
 */
#ifndef PLATFORM_PC_PSYQ_LIBGTE_H
#define PLATFORM_PC_PSYQ_LIBGTE_H

#define ONE 4096

typedef struct {
    short m[3][3];
    int  t[3];
} MATRIX;

typedef struct {
    int vx, vy, vz, pad;
} VECTOR;

typedef struct {
    short vx, vy, vz, pad;
} SVECTOR;

/* Not part of the confirmed-used function contract (no CVECTOR-typed GTE
 * function is actually called), but project headers (graphics.h/object.h)
 * reference the type directly for struct fields -- surfaced only once real
 * src .c compilation was attempted, not by the header-symbol contract scan. */
typedef struct {
    unsigned char r, g, b, cd;
} CVECTOR;

/* Same story as CVECTOR above: not part of the confirmed-used function
 * contract, but referenced directly as a field/local type by game code. */
typedef struct {
    short vx, vy;
} DVECTOR;

void InitGeom(void);

void SetGeomOffset(int ofx, int ofy);
void SetGeomScreen(int h);

void SetRotMatrix(MATRIX *m);
void SetTransMatrix(MATRIX *m);
void SetLightMatrix(MATRIX *m);
void SetColorMatrix(MATRIX *m);
void SetBackColor(int r, int g, int b);

void PushMatrix(void);
void PopMatrix(void);

MATRIX *RotMatrix(SVECTOR *r, MATRIX *m);
MATRIX *TransMatrix(MATRIX *m, VECTOR *v);
MATRIX *ScaleMatrix(MATRIX *m, VECTOR *v);

void RotTrans(SVECTOR *v0, VECTOR *v1, int *flag);
int RotTransPers(SVECTOR *v0, int *sxy, int *p, int *flag);
int RotTransPers4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                    int *sxy0, int *sxy1, int *sxy2, int *sxy3,
                    int *p, int *flag);
int RotAverage4(SVECTOR *v0, SVECTOR *v1, SVECTOR *v2, SVECTOR *v3,
                  int *sxy0, int *sxy1, int *sxy2, int *sxy3,
                  int *p, int *flag);

int VectorNormalS(VECTOR *v0, SVECTOR *v1);

int SquareRoot0(int a);
int SquareRoot12(int a);
int  csqrt(int a);

int  rcos(int a);
int  rsin(int a);
int ratan2(int y, int x);

#endif
