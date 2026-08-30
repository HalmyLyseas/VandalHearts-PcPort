/* Clean-room PC replacement for the PSX SDK libgte.h: layouts and signatures as recovered by the
 * byte-exact decompile, behaviour from psx-spx. Every integer is `int`, never `long` -- LP64 `long`
 * grows MATRIX to 48 bytes and smashes 4-byte `int` locals. See docs/pc-port/subsystems/gte.md. */
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

/* No GTE function here takes a CVECTOR, but graphics.h/object.h use the type for struct fields. */
typedef struct {
    unsigned char r, g, b, cd;
} CVECTOR;

/* Likewise DVECTOR: no function here takes it, but game code uses it as a field/local type. */
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
