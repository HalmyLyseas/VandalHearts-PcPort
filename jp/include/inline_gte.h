#ifndef INLINE_GTE_H
#define INLINE_GTE_H

#define gte_ldv0(r0)                                                                               \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("lwc2   $0, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $1, 4( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_ldv3(r0, r1, r2)                                                                       \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("move   $13, %0" : : "r"(r1) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("move   $14, %0" : : "r"(r2) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("lwc2   $0, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $1, 4( $12 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $2, 0( $13 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $3, 4( $13 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $4, 0( $14 )" : : : "$12", "$13", "$14", "$15", "memory");             \
   __asm__ volatile("lwc2   $5, 4( $14 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_ldrgb(r0)                                                                              \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("lwc2   $6, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_ldopv1(r0)                                                                             \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("lw     $13, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("lw     $14, 4( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("ctc2   $13, $0" : : : "$12", "$13", "$14", "$15", "memory");                  \
   __asm__ volatile("lw     $15, 8( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("ctc2   $14, $2" : : : "$12", "$13", "$14", "$15", "memory");                  \
   __asm__ volatile("ctc2   $15, $4" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_ldopv2(r0)                                                                             \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("lwc2   $11, 8( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("lwc2   $9,  0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("lwc2   $10, 4( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_rtps()                                                                                 \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x0180001" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_rtpt()                                                                                 \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x0280030" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_nclip()                                                                                \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x01400006" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_avsz4()                                                                                \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x0168002E" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_op0()                                                                                  \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x0170000C" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_nccs()                                                                                 \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("nop" : : : "$12", "$13", "$14", "$15", "memory");                             \
   __asm__ volatile("cop2 0x0108041B" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_stsxy(r0)                                                                              \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $14, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_stsxy3(r0, r1, r2)                                                                     \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("move   $13, %0" : : "r"(r1) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("move   $14, %0" : : "r"(r2) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $12, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("swc2   $13, 0( $13 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("swc2   $14, 0( $14 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_stotz(r0)                                                                              \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $7, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_stopz(r0)                                                                              \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $24, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_stlvnl(r0)                                                                             \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $25, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("swc2   $26, 4( $12 )" : : : "$12", "$13", "$14", "$15", "memory");            \
   __asm__ volatile("swc2   $27, 8( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#define gte_strgb(r0)                                                                              \
   __asm__ volatile("move   $12, %0" : : "r"(r0) : "$12", "$13", "$14", "$15", "memory");          \
   __asm__ volatile("swc2   $22, 0( $12 )" : : : "$12", "$13", "$14", "$15", "memory")

#endif
