/* platform/pc/src/pc_mdec.c -- PS1 MDEC / STR (BS v2 & v3) video decoder.
 * Reimplemented from psx-spx (cdromfileformats.md BS Compression + macroblockdecodermdec.md);
 * ffmpeg's mdec.c consulted only to cross-check the algorithm. Validated to pixel-parity with
 * ffmpeg on both BS versions the intro FMVs use (Konami logo = v3, TITLE_WS = v2), 100%% of
 * pixels within 8/255 (residual = float-vs-integer IDCT rounding, which psx-spx notes is
 * hardware-unspecified). Entry point: PC_MdecDecodeBS(bs, bsLen, w, h, outBGR555). */
/* Standalone PS1 MDEC / STR (BS v2 & v3) decoder -- reimplemented from psx-spx
 * (cdromfileformats.md BS Compression + macroblockdecodermdec.md). Offline test build;
 * the core (everything except main()) lifts into platform/pc/src/pc_mdec.c.
 *
 * Pipeline: STR demux (9 sectors/frame, 0x20-byte STR hdr + 0x7E0 payload each) ->
 * BS frame (8-byte hdr: MDECsize/4, ID 0x3800, q_scale, version) -> Huffman decode
 * (per-block DC + AC halfwords, block order Cr,Cb,Y1,Y2,Y3,Y4; macroblocks column-major)
 * -> rl_decode (dequant * default_quant * q_scale, zigzag) -> IDCT -> YUV->RGB -> BGR555.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <math.h>

typedef unsigned char  u8;
typedef unsigned short u16;
typedef unsigned int   u32;
typedef short          s16;

/* ---------------- bit reader (halfword LE, MSB-first within each 16-bit) ------------- */
typedef struct { const u8 *p; int nbytes; int hwIdx; int bit; u16 cur; } BitR;
static void br_init(BitR *b, const u8 *data, int nbytes) {
    b->p = data; b->nbytes = nbytes; b->hwIdx = 0; b->bit = -1; b->cur = 0;
}
static int br_bit(BitR *b) {
    if (b->bit < 0) {
        int o = b->hwIdx * 2;
        if (o + 1 >= b->nbytes) return -1;           /* out of data */
        b->cur = (u16)(b->p[o] | (b->p[o+1] << 8));   /* little-endian halfword */
        b->hwIdx++; b->bit = 15;
    }
    return (b->cur >> (b->bit--)) & 1;
}
static int br_bits(BitR *b, int n) {
    int v = 0;
    while (n--) { int bit = br_bit(b); if (bit < 0) return -1; v = (v << 1) | bit; }
    return v;
}

/* ---------------- tables ------------------------------------------------------------- */
/* FORWARD zigzag scan: transmission index -> natural raster position.
 * (psx-spx's `zigzag[]` is the INVERSE map raster->scan-index; this is its `zagzig`.) */
static const u8 ZIGZAG[64] = {
     0, 1, 8,16, 9, 2, 3,10, 17,24,32,25,18,11, 4, 5,
    12,19,26,33,40,48,41,34, 27,20,13, 6, 7,14,21,28,
    35,42,49,56,57,50,43,36, 29,22,15,23,30,37,44,51,
    58,59,52,45,38,31,39,46, 53,60,61,54,47,55,62,63
};
/* standard MDEC default quant table, natural (raster) order */
static const u8 QUANT[64] = {
     2,16,19,22,26,27,29,34, 16,16,22,24,27,29,34,37,
    19,22,26,27,29,34,34,38, 22,22,26,27,29,34,37,40,
    22,26,27,29,32,35,40,48, 26,27,29,32,35,40,48,58,
    26,27,29,34,38,46,56,69, 27,29,35,38,46,56,69,83
};

/* AC VLC -> positive MDEC value (run<<10 | level); caller applies sign 's'. */
static const u16 T_00100[8]  = {0x3401,0x0006,0x3001,0x2C01,0x0C02,0x0403,0x0005,0x2801};
static const u16 T_0001[4]   = {0x1C01,0x1801,0x0402,0x1401};
static const u16 T_00001[4]  = {0x0802,0x2401,0x0004,0x2001};
static const u16 T_0000001[8]= {0x4001,0x1402,0x0007,0x0803,0x0404,0x3C01,0x3801,0x1002};
static const u16 T_8z[16]    = {0x000B,0x2002,0x1003,0x000A,0x0804,0x1C02,0x5401,0x5001,
                                0x0009,0x4C01,0x4801,0x0405,0x0C03,0x0008,0x1802,0x4401};
static const u16 T_9z[16]    = {0x2802,0x2402,0x1403,0x0C04,0x0805,0x0407,0x0406,0x000F,
                                0x000E,0x000D,0x000C,0x6801,0x6401,0x6001,0x5C01,0x5801};
static const u16 T_10z[16]   = {0x001F,0x001E,0x001D,0x001C,0x001B,0x001A,0x0019,0x0018,
                                0x0017,0x0016,0x0015,0x0014,0x0013,0x0012,0x0011,0x0010};
static const u16 T_11z[16]   = {0x0028,0x0027,0x0026,0x0025,0x0024,0x0023,0x0022,0x0021,
                                0x0020,0x040E,0x040D,0x040C,0x040B,0x040A,0x0409,0x0408};
static const u16 T_12z[16]   = {0x0412,0x0411,0x0410,0x040F,0x1803,0x4002,0x3C02,0x3802,
                                0x3402,0x3002,0x2C02,0x7C01,0x7801,0x7401,0x7001,0x6C01};

#define AC_EOB   0x7fffffff
#define AC_ERR   0x7ffffffe

/* returns (run<<16)|(level&0xffff signed) packed, or AC_EOB / AC_ERR */
static int ac_next(BitR *b) {
    int s, x, v;
    #define SIGNED10(n) ( ((n) & 0x200) ? ((n) | ~0x3ff) : (n) )
    #define EMIT(val) do{ int _v=(val); int run=(_v>>10)&0x3f; int lev=_v&0x3ff; \
                          if(s) { lev=-lev; } return (run<<16)|(lev & 0xffff); }while(0)
    int b1 = br_bit(b); if (b1<0) return AC_ERR;
    if (b1==1) { int b2=br_bit(b); if(b2<0)return AC_ERR;
        if (b2==0) return AC_EOB;                       /* 10 */
        s=br_bit(b); EMIT(0x0001); }                    /* 11s */
    /* first bit 0 */
    { int b2=br_bit(b); if(b2<0)return AC_ERR;
      if (b2==1) { int b3=br_bit(b); if(b3<0)return AC_ERR;
          if (b3==1){ s=br_bit(b); EMIT(0x0401); }      /* 011s */
          x=br_bit(b); s=br_bit(b); EMIT(x?0x0801:0x0002); } /* 010xs */
      { int b3=br_bit(b); if(b3<0)return AC_ERR;
        if (b3==1){ int b4=br_bit(b); if(b4<0)return AC_ERR;
            if (b4==1){ x=br_bit(b); s=br_bit(b); EMIT(x?0x0C01:0x1001); } /* 0011xs */
            { int b5=br_bit(b); if(b5<0)return AC_ERR;
              if (b5==1){ s=br_bit(b); EMIT(0x0003); }   /* 00101s */
              x=br_bits(b,3); s=br_bit(b); EMIT(T_00100[x]); } } /* 00100xxxs */
        { int b4=br_bit(b); if(b4<0)return AC_ERR;
          if (b4==1){ x=br_bits(b,2); s=br_bit(b); EMIT(T_0001[x]); } /* 0001xxs */
          { int b5=br_bit(b); if(b5<0)return AC_ERR;
            if (b5==1){ x=br_bits(b,2); s=br_bit(b); EMIT(T_00001[x]); } /* 00001xxs */
            { int b6=br_bit(b); if(b6<0)return AC_ERR;
              if (b6==1){ v=br_bits(b,16); if(v<0)return AC_ERR;        /* 000001 escape */
                          { int run=(v>>10)&0x3f; int lev=SIGNED10(v&0x3ff);
                            return (1<<30)|(run<<16)|(lev & 0xffff); } }   /* bit30 = escape */
              { int b7=br_bit(b); if(b7<0)return AC_ERR;
                if (b7==1){ x=br_bits(b,3); s=br_bit(b); EMIT(T_0000001[x]); } /* 0000001xxxs */
                { int b8=br_bit(b); if(b8<0)return AC_ERR;
                  if (b8==1){ x=br_bits(b,4); s=br_bit(b); EMIT(T_8z[x]); }
                  { int b9=br_bit(b); if(b9<0)return AC_ERR;
                    if (b9==1){ x=br_bits(b,4); s=br_bit(b); EMIT(T_9z[x]); }
                    { int b10=br_bit(b); if(b10<0)return AC_ERR;
                      if (b10==1){ x=br_bits(b,4); s=br_bit(b); EMIT(T_10z[x]); }
                      { int b11=br_bit(b); if(b11<0)return AC_ERR;
                        if (b11==1){ x=br_bits(b,4); s=br_bit(b); EMIT(T_11z[x]); }
                        { int b12=br_bit(b); if(b12<0)return AC_ERR;
                          if (b12==1){ x=br_bits(b,4); s=br_bit(b); EMIT(T_12z[x]); }
                          return AC_ERR; } } } } } } } } } } }
    #undef EMIT
    #undef SIGNED10
}

/* ---------------- DC decoders -------------------------------------------------------- */
/* v2: direct signed 10-bit DC; +0x1FF (0111111111) => end of frame */
static int dc_v2(BitR *b, int *eof) {
    int n = br_bits(b, 10); if (n<0){*eof=1;return 0;}
    if (n == 0x1FF) { *eof = 1; return 0; }
    return (n & 0x200) ? (n | ~0x3ff) : n;    /* signed10 */
}
/* v3: Huffman offset (relative to prev DC), separate CrCb / Y trees; 10 ones => EOF.
 * isY selects the Y tree. Returns the DC OFFSET; caller accumulates + wraps. */
static int dc_v3(BitR *b, int isY, int *eof) {
    /* count leading ones */
    int ones = 0, bit;
    while ((bit = br_bit(b)) == 1) { ones++; if (ones >= 10) { *eof = 1; return 0; } }
    if (bit < 0) { *eof = 1; return 0; }
    /* now `ones` ones followed by a 0 have been consumed */
    int numN;
    if (!isY) {
        /* CrCb: 00->+0 ; 01s->n0 ; (L ones)0 s (L n-bits) -> numN=L  (L>=1) */
        if (ones == 0) {
            int b2 = br_bit(b); if (b2 < 0){*eof=1;return 0;}
            if (b2 == 0) return 0;                 /* 00 -> +0 */
            /* 01s -> numN=0 */
            numN = 0;
        } else numN = ones;                        /* 1..7 leading ones */
    } else {
        /* Y: 00s->n0 ; 01sn->n1 ; 100->+0 ; 101snn->n2 ; 110snnn->n3 ; ... */
        if (ones == 0) {
            int b2 = br_bit(b); if (b2 < 0){*eof=1;return 0;}
            numN = (b2 == 0) ? 0 : 1;              /* 00s (n0) or 01.. (n1) */
        } else if (ones == 1) {
            int b2 = br_bit(b); if (b2 < 0){*eof=1;return 0;}
            if (b2 == 0) return 0;                 /* 100 -> +0 */
            numN = 2;                              /* 101snn */
        } else numN = ones + 1;                    /* 110->n3, 1110->n4, ... */
    }
    int s = br_bit(b); if (s < 0){*eof=1;return 0;}
    int n = (numN > 0) ? br_bits(b, numN) : 0; if (n < 0){*eof=1;return 0;}
    if (!s) n ^= ((1 << numN) - 1);                /* negative: bit-invert magnitude bits */
    int mag = (1 << numN) + n;                     /* numN=0 -> mag=1 */
    return (s ? mag : -mag) * 4;
}

/* ---------------- IDCT (separable float) --------------------------------------------- */
static float s_ct[8][8];
static void idct_init(void) {
    for (int u = 0; u < 8; u++) for (int x = 0; x < 8; x++)
        s_ct[u][x] = cosf((2*x+1)*u*3.14159265358979f/16.0f) * (u==0 ? 0.353553390593f : 0.5f);
}
static void idct8x8(float *blk) {   /* in-place, natural order */
    float tmp[64];
    for (int y = 0; y < 8; y++)                 /* rows */
        for (int x = 0; x < 8; x++) {
            float s = 0; for (int u = 0; u < 8; u++) s += s_ct[u][x] * blk[y*8+u];
            tmp[y*8+x] = s;
        }
    for (int x = 0; x < 8; x++)                 /* cols */
        for (int y = 0; y < 8; y++) {
            float s = 0; for (int v = 0; v < 8; v++) s += s_ct[v][y] * tmp[v*8+x];
            blk[y*8+x] = s;
        }
}

/* ---------------- one 8x8 block: DC halfword + AC run/level -> pixels ----------------- */
/* Decode block coefficients from the huffman stream into a dequantized natural-order
 * float block, then IDCT. dcCoef already extracted (signed10), qScale from header. */
static int decode_block(BitR *b, int dcCoef, int qScale, float *out) {
    float blk[64]; for (int i = 0; i < 64; i++) blk[i] = 0;
    blk[0] = (float)(dcCoef * QUANT[0]);        /* DC: *qt[0] (=2), no qscale */
    int k = 0;
    for (;;) {
        int code = ac_next(b);
        if (code == AC_EOB) break;
        if (code == AC_ERR) return -1;
        int esc = (code >> 30) & 1;
        int run = (code >> 16) & 0x3f;
        int lev = (short)(code & 0xffff);       /* signed */
        k += run + 1;
        if (k > 63) break;
        int p = ZIGZAG[k];                       /* natural position */
        /* MDEC dequant (matches ffmpeg mdec.c): magnitude * qscale * quant >> 3 (floor),
         * escape-coded levels additionally oddified with (v-1)|1; then re-sign. */
        int mag = lev < 0 ? -lev : lev;
        int v   = (int)(((unsigned)mag * qScale * QUANT[p]) >> 3);
        if (esc) v = (v - 1) | 1;
        if (lev < 0) v = -v;
        blk[p] = (float)v;
    }
    idct8x8(blk);
    memcpy(out, blk, sizeof(blk));
    return 0;
}

/* ---------------- full frame decode -> BGR555 (width*height) ------------------------- */
static int s_idctReady = 0;
int PC_MdecDecodeBS(const unsigned char *bs, int bsLen, int width, int height, unsigned short *outBGR555) {
    if (!s_idctReady) { idct_init(); s_idctReady = 1; }
    if (bsLen < 8) return -1;
    u16 id      = (u16)(bs[2] | (bs[3]<<8));
    int qScale  = (u16)(bs[4] | (bs[5]<<8)) & 0x3f;
    int version = (u16)(bs[6] | (bs[7]<<8));
    if (id != 0x3800) return -2;
    BitR br; br_init(&br, bs + 8, bsLen - 8);

    int mbW = (width + 15) / 16, mbH = (height + 15) / 16;
    int oldY = 0, oldCr = 0, oldCb = 0;
    float Cr[64], Cb[64], Y[4][64];

    /* macroblocks: column-major (down each 16-px column, then next column) */
    for (int mx = 0; mx < mbW; mx++) {
      for (int my = 0; my < mbH; my++) {
        int eof = 0, dc;
        /* block order: Cr, Cb, Y1, Y2, Y3, Y4 */
        for (int blk = 0; blk < 6; blk++) {
            if (version == 3) {
                int isY = (blk >= 2);
                int off = dc_v3(&br, isY, &eof);
                if (eof) return 0;                 /* clean end of frame */
#ifdef V3_NOMASK
                if (blk == 0)      { oldCr += off; dc = oldCr; }
                else if (blk == 1) { oldCb += off; dc = oldCb; }
                else               { oldY  += off; dc = oldY;  }
#else
                if (blk == 0)      { oldCr = (oldCr + off) & 0x3ff; dc = oldCr; }
                else if (blk == 1) { oldCb = (oldCb + off) & 0x3ff; dc = oldCb; }
                else               { oldY  = (oldY  + off) & 0x3ff; dc = oldY;  }
                dc = (dc & 0x200) ? (dc | ~0x3ff) : dc;   /* signed10 */
#endif
            } else { /* v2 */
                dc = dc_v2(&br, &eof);
                if (eof) return 0;
            }
            float *dst = (blk==0)?Cr : (blk==1)?Cb : Y[blk-2];
            if (decode_block(&br, dc, qScale, dst) < 0) return -3;
        }
        /* assemble 16x16 macroblock -> BGR555 at (mx*16, my*16) */
        for (int sub = 0; sub < 4; sub++) {
            int ox = (sub & 1) * 8, oy = (sub >> 1) * 8;
            for (int yy = 0; yy < 8; yy++) for (int xx = 0; xx < 8; xx++) {
                int px = mx*16 + ox + xx, py = my*16 + oy + yy;
                if (px >= width || py >= height) continue;
                float yv = Y[sub][yy*8+xx];
                int cx = (ox+xx)/2, cy = (oy+yy)/2;
                float r = Cr[cy*8+cx], b = Cb[cy*8+cx];
                float R = yv + 1.402f*r;
                float G = yv - 0.3437f*b - 0.7143f*r;
                float B = yv + 1.772f*b;
                int Ri = (int)lrintf(R)+128, Gi=(int)lrintf(G)+128, Bi=(int)lrintf(B)+128;
                if(Ri<0){Ri=0;} if(Ri>255){Ri=255;} if(Gi<0){Gi=0;} if(Gi>255){Gi=255;}
                if(Bi<0){Bi=0;} if(Bi>255){Bi=255;}
                u16 bgr = ((Bi>>3)<<10) | ((Gi>>3)<<5) | (Ri>>3);
                outBGR555[py*width + px] = bgr;
            }
        }
      }
    }
    return 0;
}
