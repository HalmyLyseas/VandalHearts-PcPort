/* pc_gpu_trace.c -- GPU trace record / replay (regression harness, post-1.6 "3a").
 * Extracted verbatim from libgpu.c (the code accreted there because it hooks the walker; the
 * subsystem itself is a harness, not GPU emulation). Seams with libgpu.c: pc_gpu_internal.h.
 *
 * Record (VH_GPU_RECORD=<file> [VH_GPU_RECORD_FRAMES=N, default 400]): serialize, in call order,
 * everything that mutates the rasterizer's state -- VRAM uploads/blits/clears, PutDrawEnv, and
 * every primitive the DrawOTag walker dispatches (raw struct bytes; state prims like DR_MODE are
 * primitives too, so mode/texture-window changes replay in order). 'Z' marks each DrawOTag end.
 *
 * Replay (VH_GPU_REPLAY=<file>, handled in pc_bootstrap before the game starts): feed the ops back
 * through the very same entry points -- recorded prims are copied to an arena, chained with real
 * OT tokens, and handed to DrawOTag itself -- so the full production raster pipeline runs with no
 * game, no disc, no window, fully deterministically. After every frame the 1MB VRAM is FNV-hashed;
 * the combined hash is the regression signature (tools/regress/raster_check.sh records a boot
 * trace once, stores the signature, and re-verifies it on demand). Traces contain game-derived
 * texture data -> they live under build/ (gitignored), never in the repo. */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>

#include "PsyQ/libgpu.h"
#include "pc_platform.h"
#include "pc_gpu_internal.h"

static FILE *s_trcF; static int s_trcState = -1;   /* -1 unchecked, 0 off, 1 recording, 2 done */
static unsigned s_trcFrames, s_trcMaxFrames;
static int s_trcReplaying;

static u32 TrcPrimSize(int type) {
    switch (type) {
    case PC_GPU_PRIM_POLY_F4:  return (u32)sizeof(POLY_F4);
    case PC_GPU_PRIM_POLY_FT4: return (u32)sizeof(POLY_FT4);
    case PC_GPU_PRIM_SPRT:     return (u32)sizeof(SPRT);
    case PC_GPU_PRIM_TILE:     return (u32)sizeof(TILE);
    case PC_GPU_PRIM_DR_MODE:  return (u32)sizeof(DR_MODE);
    default: return 0;                     /* unknown = the walker skips it too */
    }
}
void TrcWrite(char op, const void *a, u32 na, const void *b, u32 nb) {
    if (s_trcState != 1) return;
    fputc(op, s_trcF);
    fwrite(&na, 4, 1, s_trcF); if (na) fwrite(a, 1, na, s_trcF);
    fwrite(&nb, 4, 1, s_trcF); if (nb) fwrite(b, 1, nb, s_trcF);
}
/* One walked primitive, raw struct bytes in walk order (state check + size lookup included so the
 * walker's call site stays a single line). */
void TrcPrim(int type, const void *prim) {
    if (s_trcState != 1) return;
    { u32 psz = TrcPrimSize(type);
      if (psz) { u32 t32 = (u32)type; TrcWrite('P', &t32, 4, prim, psz); } }
}
/* VH_GPU_RECORD_BATTLE=1: hold recording until the game is in an active battle (libetc's VSync arms
 * this from the battle-state dispatch each tick), so a 600-frame budget is pure battle content
 * instead of being eaten by the boot/menu lead-in. */
static int s_trcArmed;
void PC_GpuTraceArmBattle(int on) { s_trcArmed = on; }

void TrcInit(void) {
    const char *p, *n;
    if (s_trcState >= 0) return;
    if (s_trcReplaying) { s_trcState = 0; return; }
    p = getenv("VH_GPU_RECORD"); if (!p || !*p) { s_trcState = 0; return; }
    if (getenv("VH_GPU_RECORD_BATTLE") && !s_trcArmed) return;   /* stay unchecked; retry next frame */
    s_trcF = fopen(p, "wb");
    if (!s_trcF) { fprintf(stderr, "[trace] cannot open '%s' for recording\n", p); return; }
    fwrite("VHT1", 1, 4, s_trcF);
    n = getenv("VH_GPU_RECORD_FRAMES");
    s_trcMaxFrames = (n && atoi(n) > 0) ? (unsigned)atoi(n) : 400;
    s_trcState = 1;
    fprintf(stderr, "[trace] recording GPU trace -> %s (%u DrawOTag frames)\n", p, s_trcMaxFrames);
}
void TrcFrameEnd(void) {
    if (s_trcState != 1) return;
    TrcWrite('Z', NULL, 0, NULL, 0);
    if (++s_trcFrames >= s_trcMaxFrames) {
        fclose(s_trcF); s_trcF = NULL; s_trcState = 2;
        fprintf(stderr, "[trace] recording complete (%u frames)\n", s_trcFrames);
    }
}
int TrcReplaying(void) { return s_trcReplaying; }

static unsigned long long TrcVramHash(void) {
    size_t n, i; unsigned long long h = 1469598103934665603ull;
    const unsigned char *b = (const unsigned char *)PC_GpuVramBytes(&n);
    for (i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ull; }
    return h;
}

static unsigned long long TrcHiresHash(void) {   /* verification only: 16x the data at scale 4 */
    size_t n, i; unsigned long long h = 1469598103934665603ull;
    const unsigned char *b = (const unsigned char *)PC_GpuHiresBytes(&n);
    if (!b) return 0;
    for (i = 0; i < n; i++) { h ^= b[i]; h *= 1099511628211ull; }
    return h;
}

int PC_GpuReplayTrace(const char *path) {
    FILE *f = fopen(path, "rb");
    char magic[4]; unsigned frame = 0; unsigned long long combined = 1469598103934665603ull;
    double drawMs = 0.0; int hashHires;
    unsigned char *arena = NULL; size_t arenaCap = 0, arenaUsed = 0;
    size_t *poff = NULL; u32 *psz = NULL; size_t pcap = 0, pcount = 0;
    unsigned char *payload = NULL; size_t payloadCap = 0;
    if (!f) { fprintf(stderr, "[replay] cannot open '%s'\n", path); return 2; }
    if (fread(magic, 1, 4, f) != 4 || memcmp(magic, "VHT1", 4) != 0) {
        fprintf(stderr, "[replay] '%s' is not a VHT1 GPU trace\n", path); fclose(f); return 2;
    }
    s_trcReplaying = 1; s_trcState = 0;
    /* Bench knobs: VH_INTERNAL_SCALE engages the hi-res pass (the game normally sets it in GpuInit,
     * which replay mode skips); VH_RASTER_THREADS is read by the pool as usual. Timing accumulates
     * DrawOTag wall time ONLY -- hashing (which can dwarf rasterization at scale 4) stays outside.
     * VH_GPU_REPLAY_HASH_HIRES=1 folds the hi-res buffer into the signature (slow; for verifying an
     * optimization bit-identical, not for timing). */
    { const char *e = getenv("VH_INTERNAL_SCALE"); if (e && atoi(e) > 1) PC_GpuSetInternalScale(atoi(e)); }
    hashHires = getenv("VH_GPU_REPLAY_HASH_HIRES") != NULL;
    for (;;) {
        int op = fgetc(f); u32 na, nb;
        unsigned char hdrA[64];
        if (op == EOF) break;
        if (fread(&na, 4, 1, f) != 1) break;
        if (na > sizeof(hdrA)) { fprintf(stderr, "[replay] corrupt header size %u\n", na); break; }
        if (na && fread(hdrA, 1, na, f) != na) break;
        if (fread(&nb, 4, 1, f) != 1) break;
        if (nb) {
            if (nb > payloadCap) { payloadCap = nb * 2 + 4096; payload = (unsigned char *)realloc(payload, payloadCap); }
            if (!payload || fread(payload, 1, nb, f) != nb) break;
        }
        switch (op) {
        case 'L': LoadImage((RECT *)hdrA, (unsigned int *)payload); break;
        case 'M': { int *xy = (int *)payload; MoveImage((RECT *)hdrA, xy[0], xy[1]); } break;
        case 'C': ClearImage((RECT *)hdrA, payload[0], payload[1], payload[2]); break;
        case 'E': { DRAWENV env; if (nb == sizeof(env)) { memcpy(&env, payload, sizeof(env)); PutDrawEnv(&env); } } break;
        case 'P': {
            if (arenaUsed + nb > arenaCap) { arenaCap = (arenaUsed + nb) * 2 + 65536; arena = (unsigned char *)realloc(arena, arenaCap); }
            if (pcount == pcap) { pcap = pcap * 2 + 256; poff = (size_t *)realloc(poff, pcap * sizeof(*poff)); psz = (u32 *)realloc(psz, pcap * sizeof(*psz)); }
            if (!arena || !poff || !psz) { fprintf(stderr, "[replay] out of memory\n"); fclose(f); return 2; }
            memcpy(arena + arenaUsed, payload, nb);
            poff[pcount] = arenaUsed; psz[pcount] = nb; pcount++;
            arenaUsed += (nb + 7u) & ~7u;                  /* keep prims 8-aligned */
            break;
        }
        case 'Z': {
            u32 tok = 0, headSlot; size_t i; unsigned long long fh;
            PC_OtResetTokens();
            for (i = pcount; i-- > 0; ) {                  /* chain in record order: i -> i+1 */
                P_TAG *pr = (P_TAG *)(arena + poff[i]);
                pr->tag = tok;
                tok = PC_OtMint(pr, 0);
            }
            headSlot = tok;
#ifndef _WIN32
            { struct timespec t0, t1;
              clock_gettime(CLOCK_MONOTONIC, &t0);
              DrawOTag(&headSlot);
              clock_gettime(CLOCK_MONOTONIC, &t1);
              drawMs += (t1.tv_sec - t0.tv_sec) * 1e3 + (t1.tv_nsec - t0.tv_nsec) / 1e6; }
#else
            DrawOTag(&headSlot);
#endif
            fh = TrcVramHash();
            if (hashHires) { unsigned long long hh = TrcHiresHash(); combined ^= hh; combined *= 1099511628211ull; }
            combined ^= fh; combined *= 1099511628211ull;
            frame++;
            if (getenv("VH_GPU_REPLAY_VERBOSE"))
                fprintf(stderr, "[replay] frame %u prims=%u vram=%016llx\n", frame, (unsigned)pcount, fh);
            pcount = 0; arenaUsed = 0;
            break;
        }
        default:
            fprintf(stderr, "[replay] unknown op 0x%02x at frame %u -- trace corrupt?\n", op, frame);
            fclose(f); return 2;
        }
    }
    fclose(f); free(arena); free(poff); free(psz); free(payload);
    printf("REPLAY frames=%u combined=%016llx\n", frame, combined);
    if (drawMs > 0.0 && frame)
        fprintf(stderr, "[replay] scale=%d DrawOTag total=%.0fms  mean=%.2fms/frame  (%.1f fps-equivalent)\n",
                PC_GpuGetInternalScale(), drawMs, drawMs / frame, 1000.0 * frame / drawMs);
    return frame ? 0 : 2;
}
