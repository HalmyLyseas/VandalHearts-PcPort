/* Clean-room PC replacement for the PSX SDK gtemac.h, scoped to what core/graphics.c calls.
 * Composed from the primitive macros in platform/pc/include/inline_gte.h, which must be included
 * first (core/graphics.c does, matching the real include order). */
#ifndef PLATFORM_PC_PSYQ_GTEMAC_H
#define PLATFORM_PC_PSYQ_GTEMAC_H

#define gte_NormalColorCol(r1, r2, r3) \
    { gte_ldv0(r1); gte_ldrgb(r2); gte_nccs(); gte_strgb(r3); }

#define gte_OuterProduct0(r1, r2, r3) \
    { gte_ldopv1(r1); gte_ldopv2(r2); gte_op0(); gte_stlvnl(r3); }

#endif
