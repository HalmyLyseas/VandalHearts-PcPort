/*
 * PC-backend replacement for the PSX SDK's gtemac.h GTE composite macros.
 *
 * Clean-room reimplementation, scoped to only what core/graphics.c actually calls
 * (per exchange/02-phase-c-interface-contract.md): gte_NormalColorCol and
 * gte_OuterProduct0. Like the real header, these are built by composing the
 * raw primitive macros -- here, the ones declared in
 * platform/pc/include/inline_gte.h, which must be included first (core/graphics.c
 * already does this, matching the real project's own include order).
 */
#ifndef PLATFORM_PC_PSYQ_GTEMAC_H
#define PLATFORM_PC_PSYQ_GTEMAC_H

#define gte_NormalColorCol(r1, r2, r3) \
    { gte_ldv0(r1); gte_ldrgb(r2); gte_nccs(); gte_strgb(r3); }

#define gte_OuterProduct0(r1, r2, r3) \
    { gte_ldopv1(r1); gte_ldopv2(r2); gte_op0(); gte_stlvnl(r3); }

#endif
