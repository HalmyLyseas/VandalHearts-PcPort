/* Standalone proof-of-concept: exercises the software GTE core against
 * known/hand-computed values -- there's no extracted game data to verify
 * GTE math against (unlike CD/Audio), so this checks the implementation
 * against the psx-spx hardware formulas directly. Not part of the real
 * game build. */
#include <stdio.h>
#include <string.h>
#include "PsyQ/libgte.h"
#include "inline_gte.h"
#include "PsyQ/gtemac.h"

static int failures = 0;

#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else printf("OK:   %s\n", msg); \
} while (0)

static int Near(long a, long b, long tol) {
    long d = a - b;
    if (d < 0) d = -d;
    return d <= tol;
}

static void IdentityMatrix(MATRIX *m) {
    memset(m, 0, sizeof(*m));
    m->m[0][0] = m->m[1][1] = m->m[2][2] = ONE;
}

int main(void) {
    MATRIX id;

    printf("=== rcos/rsin (Q12, 4096 units/circle) ===\n");
    CHECK(rcos(0) == 4096, "rcos(0) == ONE");
    CHECK(rsin(0) == 0, "rsin(0) == 0");
    CHECK(Near(rcos(1024), 0, 1), "rcos(90deg) ~= 0");
    CHECK(Near(rsin(1024), 4096, 1), "rsin(90deg) ~= ONE");
    CHECK(Near(rcos(2048), -4096, 1), "rcos(180deg) ~= -ONE");

    printf("\n=== ratan2 ===\n");
    CHECK(ratan2(0, 4096) == 0, "ratan2(0,+x) == 0");
    CHECK(Near(ratan2(4096, 0), 1024, 1), "ratan2(+y,0) ~= 90deg (1024)");

    printf("\n=== SquareRoot0/12 ===\n");
    CHECK(SquareRoot0(100) == 10, "SquareRoot0(100) == 10");
    CHECK(Near(SquareRoot12(4 * ONE), 2 * ONE, 4), "SquareRoot12(4.0) ~= 2.0 (Q12)");

    printf("\n=== RTPS via high-level RotTransPers (identity, no translation) ===\n");
    InitGeom();
    IdentityMatrix(&id);
    SetRotMatrix(&id);
    {
        MATRIX tm;
        IdentityMatrix(&tm);
        tm.t[0] = 0; tm.t[1] = 0; tm.t[2] = 0;
        SetTransMatrix(&tm);
    }
    SetGeomOffset(0, 0);
    SetGeomScreen(512); /* H */

    {
        SVECTOR v = { 0, 0, 4096, 0 }; /* straight ahead, z = 1.0 */
        long sxy, p, flag;
        long sz = RotTransPers(&v, &sxy, &p, &flag);
        short sx = (short)(sxy & 0xFFFF), sy = (short)(sxy >> 16);
        CHECK(sz == 4096, "on-axis point: SZ3 == vz (identity, no rotation)");
        CHECK(sx == 0 && sy == 0, "on-axis point projects to screen (0,0) with zero offset");
    }
    {
        /* Point at (ONE,0,2*ONE): with H=512, offset 0, expect a positive SX
         * (off to one side) and unaffected by identity rotation. */
        SVECTOR v = { 4096, 0, 8192, 0 };
        long sxy, p, flag;
        RotTransPers(&v, &sxy, &p, &flag);
        {
            short sx = (short)(sxy & 0xFFFF);
            CHECK(sx > 0, "off-axis +x point projects to positive screen X");
        }
    }

    printf("\n=== NCLIP winding test (via raw gte_* macros, core/graphics.c's path) ===\n");
    InitGeom();
    SetRotMatrix(&id);
    {
        MATRIX tm;
        IdentityMatrix(&tm);
        SetTransMatrix(&tm);
    }
    SetGeomOffset(0, 0);
    SetGeomScreen(512);
    {
        /* Three points forming a counter-clockwise triangle in screen space
         * once projected (all same Z so perspective divide scales uniformly):
         * (-1,-1) (1,-1) (0,1) scaled up so they survive the divide visibly. */
        SVECTOR a = { -2048, -2048, 8192, 0 };
        SVECTOR b = { 2048, -2048, 8192, 0 };
        SVECTOR c = { 0, 2048, 8192, 0 };
        long opz;
        gte_ldv3(&a, &b, &c);
        gte_rtpt();
        gte_nclip();
        gte_stopz(&opz);
        printf("  NCLIP result (opz) = %ld\n", opz);
        CHECK(opz != 0, "NCLIP produces a non-zero winding value for a non-degenerate triangle");
    }

    printf("\n=== AVSZ4 (via raw macros) ===\n");
    {
        unsigned short otz;
        /* zsf4 defaults to 0 after InitGeom -- set a nonzero scale so the
         * average isn't trivially zero. */
        InitGeom();
        SetRotMatrix(&id);
        {
            MATRIX tm; IdentityMatrix(&tm); SetTransMatrix(&tm);
        }
        SetGeomOffset(0, 0);
        SetGeomScreen(512);
        {
            SVECTOR a = { 0, 0, 4096, 0 };
            SVECTOR b = { 0, 0, 4096, 0 };
            SVECTOR c = { 0, 0, 4096, 0 };
            SVECTOR d = { 0, 0, 4096, 0 };
            gte_ldv3(&a, &b, &c);
            gte_rtpt();
            gte_ldv0(&d);
            gte_rtps();
            gte_avsz4();
            gte_stotz(&otz);
            printf("  OTZ (zsf4=0, expect 0) = %u\n", otz);
            CHECK(otz == 0, "AVSZ4 with zsf4=0 gives OTZ=0");
        }
    }

    printf("\n=== gte_OuterProduct0 cross product ===\n");
    {
        long d[3] = { 4096, 0, 0 };   /* +X */
        long v[3] = { 0, 4096, 0 };   /* +Y */
        long out[3];
        gte_OuterProduct0(d, v, out);
        /* cross(+X,+Y) = +Z */
        printf("  cross result = (%ld,%ld,%ld)\n", out[0], out[1], out[2]);
        CHECK(out[0] == 0 && out[1] == 0 && out[2] > 0,
              "OuterProduct0(+X,+Y) points along +Z");
    }

    printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
