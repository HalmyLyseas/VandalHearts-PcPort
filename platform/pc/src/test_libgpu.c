/* Standalone proof-of-concept: exercises the VRAM buffer, Ordering Table
 * walk, rasterizer (POLY_F4/POLY_FT4/SPRT/TILE), GetTPage/GetClut bit
 * packing, TIM parsing, and the SDL2/OpenGL present path. Not part of the
 * real game build. */
#include <stdio.h>
#include <string.h>
#include "PsyQ/libgpu.h"
#include "pc_platform.h"

static int failures = 0;
#define CHECK(cond, msg) do { \
    if (!(cond)) { printf("FAIL: %s\n", msg); failures++; } \
    else printf("OK:   %s\n", msg); \
} while (0)

static unsigned short ReadVramPixel(int x, int y) {
    RECT r; unsigned short px;
    setRECT(&r, x, y, 1, 1);
    StoreImage(&r, (u_long *)&px);
    return px;
}

int main(void) {
    DRAWENV drawEnv;
    DISPENV dispEnv;

    ResetGraph(0);
    SetDefDrawEnv(&drawEnv, 0, 0, 320, 240);
    SetDefDispEnv(&dispEnv, 0, 0, 320, 240);
    PutDrawEnv(&drawEnv);

    printf("=== GetTPage / GetClut bit packing (psx-spx exact) ===\n");
    /* tp=2 (15bit), abr=0, x=64 (page 1), y=0 -> bits0-3=1, bit4=0 */
    CHECK(GetTPage(2, 0, 64, 0) == ((1) | (0 << 4) | (0 << 5) | (2 << 7)), "GetTPage(15bit,abr0,x=64,y=0)");
    CHECK(GetTPage(0, 1, 0, 256) == ((0) | (1 << 4) | (1 << 5) | (0 << 7)), "GetTPage(4bit,abr1,x=0,y=256)");
    CHECK(GetClut(32, 10) == ((32 / 16) | (10 << 6)), "GetClut(32,10)");

    printf("\n=== ClearImage + StoreImage round trip ===\n");
    {
        RECT r; setRECT(&r, 10, 10, 4, 4);
        ClearImage(&r, 0xF8, 0x00, 0x00); /* pure red, 15-bit-representable */
        {
            unsigned short px = ReadVramPixel(11, 11);
            int rr, gg, bb;
            rr = (px & 0x1F) << 3; gg = ((px >> 5) & 0x1F) << 3; bb = ((px >> 10) & 0x1F) << 3;
            CHECK(rr > 240 && gg == 0 && bb == 0, "ClearImage fills VRAM with the requested color (15-bit quantized)");
        }
    }

    printf("\n=== POLY_F4 (flat quad) via OT/AddPrim/DrawOTag ===\n");
    {
        u_long ot[8];
        POLY_F4 quad;
        ClearOTag(ot, 8);
        SetPolyF4(&quad);
        setRGB0(&quad, 0x00, 0xF8, 0x00); /* pure green */
        setXYWH(&quad, 50, 50, 20, 20);
        AddPrim(&ot[0], &quad);
        DrawOTag(&ot[0]);
        {
            unsigned short px = ReadVramPixel(55, 55);
            int rr, gg, bb;
            rr = (px & 0x1F) << 3; gg = ((px >> 5) & 0x1F) << 3; bb = ((px >> 10) & 0x1F) << 3;
            CHECK(gg > 240 && rr == 0 && bb == 0, "POLY_F4 rasterizes flat green inside the quad");
        }
        CHECK(ReadVramPixel(200, 200) == 0, "POLY_F4 doesn't paint outside the quad");
    }

    printf("\n=== TILE (solid rect) ===\n");
    {
        u_long ot[8];
        TILE t;
        ClearOTag(ot, 8);
        SetTile(&t);
        setRGB0(&t, 0x00, 0x00, 0xF8);
        setXY0(&t, 100, 100);
        setWH(&t, 8, 8);
        AddPrim(&ot[0], &t);
        DrawOTag(&ot[0]);
        {
            unsigned short px = ReadVramPixel(103, 103);
            int bb = (px >> 10) & 0x1F;
            CHECK(bb > 24, "TILE fills a solid blue rect");
        }
    }

    printf("\n=== POLY_FT4 (textured quad) sampling a real 16bpp texture ===\n");
    {
        unsigned short texel[4] = {
            (unsigned short)((0x1F)), /* pure red BGR555 */
            (unsigned short)((0x1F)),
            (unsigned short)((0x1F)),
            (unsigned short)((0x1F)),
        };
        RECT texRect;
        u_short tpage;
        setRECT(&texRect, 640, 0, 2, 2); /* tpage x=640 -> page 10, 15bit needs page-aligned-ish; use GetTPage properly below */
        LoadImage(&texRect, (u_long *)texel);
        tpage = GetTPage(2 /*15bit*/, 0, 640, 0);
        {
            u_long ot[8];
            POLY_FT4 q;
            ClearOTag(ot, 8);
            SetPolyFT4(&q);
            setRGB0(&q, 128, 128, 128); /* neutral modulation */
            setXYWH(&q, 150, 50, 2, 2);
            q.tpage = tpage;
            q.clut = 0;
            q.u0 = 0; q.v0 = 0;
            q.u1 = 1; q.v1 = 0;
            q.u2 = 0; q.v2 = 1;
            q.u3 = 1; q.v3 = 1;
            AddPrim(&ot[0], &q);
            DrawOTag(&ot[0]);
            {
                unsigned short px = ReadVramPixel(150, 50);
                int rr = (px & 0x1F) << 3;
                CHECK(rr > 240, "POLY_FT4 samples and draws the uploaded 15bpp texture");
            }
        }
    }

    printf("\n=== SPRT (samples the DR_MODE-configured tpage, like real hw) ===\n");
    {
        unsigned short texel = 0x03E0; /* pure green BGR555 */
        RECT texRect;
        int pageX = 704; /* page-aligned (multiple of 64) so u/v start at 0 */
        setRECT(&texRect, pageX, 0, 1, 1);
        LoadImage(&texRect, (u_long *)&texel);
        {
            u_long ot[8];
            DR_MODE mode;
            SPRT s;
            ClearOTag(ot, 8);
            /* AddPrim inserts at the head of the bucket (LIFO), and DrawOTag
             * walks from the head -- add the sprite first so DR_MODE (added
             * last) ends up processed first, exactly like the real
             * SetDrawMode-before-sprite call sequence the OT encodes. */
            SetSprt(&s);
            setRGB0(&s, 128, 128, 128);
            setXY0(&s, 200, 50);
            setUV0(&s, 0, 0);
            s.clut = 0;
            setWH(&s, 1, 1);
            AddPrim(&ot[0], &s);
            SetDrawMode(&mode, 0, 0, GetTPage(2, 0, pageX, 0), NULL);
            AddPrim(&ot[0], &mode);
            DrawOTag(&ot[0]);
            {
                unsigned short px = ReadVramPixel(200, 50);
                int gg = ((px >> 5) & 0x1F);
                CHECK(gg > 24, "SPRT samples the texture at the DR_MODE-configured tpage");
            }
        }
    }

    printf("\n=== TIM parsing (synthetic in-memory TIM, no CLUT, 15bpp) ===\n");
    {
        /* header: magic/ver/reserved (1 word), mode (1 word, type=2, no clut) */
        /* pixel section: size, destCoord, whWord, then 2x2 pixels */
        u_long tim[2 + 3 + 4];
        unsigned short *pix = (unsigned short *)&tim[5];
        tim[0] = 0x00000010;
        tim[1] = 2; /* mode: type=2 (16bpp), bit3=0 (no clut) */
        tim[2] = (3 + 4) * 4; /* section size in bytes */
        tim[3] = (0 << 16) | 800; /* destCoord: y=0,x=800 */
        tim[4] = (2 << 16) | 2;   /* whWord: h=2,w=2 */
        pix[0] = 0x7C00; pix[1] = 0x7C00; /* pure blue x2 */
        pix[2] = 0x7C00; pix[3] = 0x7C00;

        OpenTIM(tim);
        {
            TIM_IMAGE t;
            ReadTIM(&t);
            CHECK(t.mode == 2, "ReadTIM parses the mode word");
            CHECK(t.paddr == (u_long *)pix, "ReadTIM.paddr points at the real pixel data in the file buffer, not a dummy placeholder");
            CHECK(t.prect->x == 800 && t.prect->y == 0 && t.prect->w == 2 && t.prect->h == 2,
                  "ReadTIM.prect reflects the TIM's own embedded destCoord/whWord");
            /* Real ReadTIM never uploads to VRAM itself -- the caller does,
             * exactly like core/screen_effects.c/world/dojo.c's real usage. */
            LoadImage(t.prect, t.paddr);
            {
                unsigned short px2 = ReadVramPixel(800, 0);
                CHECK(px2 == 0x7C00, "caller's own LoadImage(t.prect, t.paddr) uploads pixel data to the correct VRAM rect");
            }
        }
    }

    printf("\n=== SDL2/OpenGL present path (headless smoke test) ===\n");
    {
        /* No PC_GpuInit() called -- PC_GpuPresent (now called at the end of
         * DrawOTag, not PutDispEnv -- see the libgpu.c comment on both) must
         * no-op cleanly rather than crash, matching Pad/VSync's headless
         * testing precedent. Exercises the same PutDispEnv-then-DrawOTag
         * pairing every real call site uses. */
        u_long ot[1];
        ClearOTag(ot, 1);
        PutDispEnv(&dispEnv);
        DrawOTag(&ot[0]);
        CHECK(1, "PutDispEnv+DrawOTag (which calls PC_GpuPresent) doesn't crash with no window open");
    }

    printf("\n%s (%d failure%s)\n", failures == 0 ? "ALL PASS" : "FAILURES", failures, failures == 1 ? "" : "s");
    return failures == 0 ? 0 : 1;
}
