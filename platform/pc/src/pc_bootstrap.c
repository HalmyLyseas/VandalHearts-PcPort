/*
 * PC-only startup glue: mounts the disc image and opens the game window
 * before the real game code's own main() (src/main.c) runs. Real hardware
 * never needs an equivalent step -- the disc is physically in the drive
 * and the TV is always displaying VRAM at boot, so there's no PSX API call
 * this could hook into; it has to run before main() via some mechanism
 * outside game source.
 *
 * Uses __attribute__((constructor)), the same mechanism already
 * established in build/generated_data.c for the extracted data-segment
 * initializers -- constructors run before main() with zero changes to any
 * real project file.
 */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <sys/mman.h>
#include <limits.h>
#include <signal.h>
#include <execinfo.h>

#include "pc_platform.h"

#define SCREEN_WIDTH 320  /* matches include/common.h's SCREEN_WIDTH/HEIGHT */
#define SCREEN_HEIGHT 240

/* PS1 games are statically linked with no dynamic allocator -- some already-
 * decompiled code (verified byte-exact, e.g. src/cd.c's gSoundSets table)
 * bakes literal fixed RAM addresses straight into C source as scratch
 * buffers (`(void *)0x80140878`), matching the original's real, fixed
 * 2MB memory map. On a 64-bit host those numeric values aren't valid
 * addresses at all. Reserving the real PS1 KUSEG RAM range (0x80000000,
 * 2MB) as real, writable memory in our own process makes every such
 * literal a valid buffer again, exactly like on real hardware -- found via
 * a real crash (fread() into gSoundSets[0].bufferPtr, PID coredump
 * backtrace), not a hypothetical. Confirmed only 8 such literals exist
 * (grep across src/*.c), none aliasing a real named symbol -- they're
 * anonymous scratch space, so this doesn't need to coexist with our own
 * generated globals at matching offsets, just be valid memory. Uses
 * MAP_FIXED_NOREPLACE (fails loudly instead of silently clobbering an
 * existing mapping) since this is a PIE binary and the exact address isn't
 * guaranteed free. */
#define PSX_RAM_BASE ((void *)0x80000000UL)
#define PSX_RAM_SIZE (2 * 1024 * 1024)

/* Real PS1 Scratchpad RAM (psx-spx iomap.md: "1F800000h 400h Scratchpad (1K Fast RAM)
 * (Data Cache mapped to fixed address)") -- a second, separate fixed-address region from
 * the main 2MB KUSEG RAM above, used by already-decompiled code as fast temp/dictionary
 * space (src/split_0a2ce0.c's UnpackMapFileData: `pCache = (u8 *)0x1f800000;`, indexed up
 * to `cacheOfs & 0x3ff`, i.e. the full real 1KB). Found via a real SIGSEGV once battle-map
 * loading was actually reached (UnpackMapFileData writing through this unmapped address),
 * not a hypothetical -- the original 8-literal audit for PSX_RAM_BASE only covered addresses
 * inside the main RAM range and missed this one since it's a different, smaller region
 * entirely. */
#define PSX_SCRATCHPAD_BASE ((void *)0x1f800000UL)
#define PSX_SCRATCHPAD_SIZE 0x400

/* Real PS1 hardware has NO memory protection: KUSEG (0x00000000) and KSEG0
 * (0x80000000) are documented mirrors of the exact same physical 2MB RAM
 * (psx-spx memorymap.md), just with different CPU caching behavior -- not
 * two different memories, and "address 0" isn't special or reserved the way
 * it is on a modern OS with virtual memory. This means a transient NULL
 * pointer dereference in already-decompiled game code (confirmed real via a
 * live BizHawk RAM trace against the actual retail game, not a
 * hypothetical: src/battle_0201b8.c's Objf013_BattleMgr reads
 * `unitSprite->x1.n` while `unitSprite` is genuinely 0x00000000 for a few
 * frames right after the demo battle's manager object is created, before a
 * later state assigns it a real value -- see
 * exchange/12-phase-c-bootstrap.md's "Current, unresolved puzzle" section
 * and exchange/13-bizhawk-ram-watch.md for the full derivation) is
 * completely harmless on real hardware: it just reads a few garbage-but-
 * valid bytes from the start of RAM for one frame. On our port, address 0
 * is a true unmapped NULL page, so the exact same code is a guaranteed
 * SIGSEGV.
 *
 * Reserving this range the same way as PSX_RAM_BASE/PSX_SCRATCHPAD_BASE
 * above closes that gap -- but mapping literal address 0 hits a real OS
 * security boundary most of those other two never touch: Linux's
 * `vm.mmap_min_addr` (this system: 65536) blocks unprivileged processes
 * from mapping anything below that address, specifically to make
 * NULL-pointer-dereference bugs in OTHER software harder to exploit. This
 * process needs CAP_SYS_RAWIO to bypass it -- see `make setcap` in this
 * directory's Makefile. Deliberately non-fatal if the capability isn't
 * present: the game runs fine without it right up until a real code path
 * actually needs this range, so warn and continue rather than refusing to
 * start. */
#define PSX_NULL_MIRROR_BASE ((void *)0x00000000UL)
#define PSX_NULL_MIRROR_SIZE PSX_RAM_SIZE

static int ReservePsxMemory(void *base, size_t size, const char *label) {
    void *p = mmap(base, size, PROT_READ | PROT_WRITE,
                    MAP_PRIVATE | MAP_ANONYMOUS | MAP_FIXED_NOREPLACE, -1, 0);
    if (p == MAP_FAILED || p != base) {
        fprintf(stderr, "PC_Bootstrap: could not reserve %s at %p (%s) -- "
                        "hardcoded scratch-buffer addresses in already-decompiled code "
                        "will crash if used\n",
                label, base, p == MAP_FAILED ? "mmap failed" : "got a different address");
        return 0;
    }
    fprintf(stderr, "PC_Bootstrap: reserved %s at %p (%zu bytes)\n", label, p, size);
#ifdef __SANITIZE_ADDRESS__
    /* ASAN's shadow memory doesn't know about a raw mmap() done outside
     * its own allocator hooks -- without this, every write into this
     * (perfectly valid) region reports as an "unknown-crash" false
     * positive. Debug-build-only concern, never affects the real
     * binary. */
    void __asan_unpoison_memory_region(void *, size_t);
    __asan_unpoison_memory_region(p, size);
#endif
    return 1;
}

__attribute__((constructor))
static void PC_ReservePsxRam(void) {
    ReservePsxMemory(PSX_RAM_BASE, PSX_RAM_SIZE, "PSX RAM range");
    ReservePsxMemory(PSX_SCRATCHPAD_BASE, PSX_SCRATCHPAD_SIZE, "PSX Scratchpad RAM");
    if (!ReservePsxMemory(PSX_NULL_MIRROR_BASE, PSX_NULL_MIRROR_SIZE, "PSX RAM's KUSEG NULL-mirror range")) {
        fprintf(stderr, "  (this needs CAP_SYS_RAWIO on this binary -- run `make setcap` in "
                        "platform/pc/ once, then relaunch. Without it, the game runs fine until "
                        "a real code path transiently dereferences a NULL pointer -- e.g. the "
                        "title screen's demo battle -- which will then crash instead of the "
                        "harmless no-op it is on real hardware.)\n");
    }
}

/* The default disc path used to be a plain relative literal
 * ("../../../game/..."), which only resolved correctly when launched from
 * one specific working directory (platform/pc/). Running the built binary
 * the natural way -- `cd build && ./vandalhearts_pc`, one directory deeper
 * -- silently mounted the wrong path and left the game running with no
 * disc data at all (every CdRead fails, nothing ever renders): a real,
 * reported failure, not a hypothetical. /proc/self/exe gives the
 * executable's own real location regardless of the caller's cwd, so the
 * default is anchored to that instead: build/vandalhearts_pc's own
 * directory, four levels up to the repo layout's game/ sibling. */
static const char *DefaultDiscPath(void) {
    static char path[PATH_MAX];
    char exePath[PATH_MAX];
    ssize_t len = readlink("/proc/self/exe", exePath, sizeof(exePath) - 1);
    char *lastSlash;
    if (len <= 0) return "../../../game/Vandal Hearts (USA).bin"; /* /proc unavailable: fall back to the old behavior */
    exePath[len] = '\0';
    lastSlash = strrchr(exePath, '/');
    if (!lastSlash) return "../../../game/Vandal Hearts (USA).bin";
    *lastSlash = '\0'; /* exePath is now the executable's directory, e.g. .../platform/pc/build */
    snprintf(path, sizeof(path), "%s/../../../../game/Vandal Hearts (USA).bin", exePath);
    return path;
}

/* Diagnostics (PC-only): dump the current call stack + game state to stderr. Used two ways:
 *   - SIGUSR1 (`kill -USR1 <pid>`): sample a running/hung process without gdb/ptrace (the setcap
 *     binary makes those awkward), then keep running -- for "freeze" (state-stall) diagnosis.
 *   - SIGSEGV/SIGBUS: dump on a real crash before dying, so the window doesn't just vanish -- the
 *     backtrace (+ symbol names, linked -rdynamic) + gState point at where it crashed.
 * addr2line -e the binary on any bare addresses. */
static void PC_DumpDiag(const char *tag) {
    void *frames[64];
    int n = backtrace(frames, 64);
    ssize_t w = write(2, tag, strlen(tag)); (void)w;
    backtrace_symbols_fd(frames, n, 2);
    { extern void PC_DumpGameState(int fd); PC_DumpGameState(2); }   /* which state/scene/map */
}
static void PC_SigUsr1(int sig) { (void)sig; PC_DumpDiag("\n*** SIGUSR1: call stack (frozen?) ***\n"); }
static void PC_SigCrash(int sig) {
    PC_DumpDiag("\n*** CRASH: fatal signal, call stack ***\n");
    signal(sig, SIG_DFL); raise(sig);   /* restore default + re-raise so it dies for real (core, etc.) */
}

/* Make the executable's read-only data segment writable. The decompiled game freely mutates string
 * literals in place (e.g. ShowExpDialog writes the EXP digits into "You got     "; more sites like
 * it exist) -- harmless on the PSX where all RAM is writable, but a SIGSEGV on Linux where literals
 * live in read-only .rodata. Remapping .rodata RW at startup makes PC memory behave like PSX RAM and
 * fixes the whole class at once (backend-only; no decompiled-source edits). Only the main
 * executable's own r--p mappings are touched, not shared libraries'. */
static void PC_MakeRodataWritable(void) {
    char exe[PATH_MAX];
    ssize_t n = readlink("/proc/self/exe", exe, sizeof(exe) - 1);
    if (n <= 0) return;
    exe[n] = '\0';
    FILE *f = fopen("/proc/self/maps", "r");
    if (!f) return;
    char line[1024];
    int fixed = 0;
    while (fgets(line, sizeof(line), f)) {
        unsigned long lo, hi;
        char perms[8];
        if (sscanf(line, "%lx-%lx %7s", &lo, &hi, perms) != 3) continue;
        if (perms[0] == 'r' && perms[1] == '-' && perms[2] == '-' && strstr(line, exe)) {
            if (mprotect((void *)lo, hi - lo, PROT_READ | PROT_WRITE) == 0) fixed++;
        }
    }
    fclose(f);
    fprintf(stderr, "PC_Bootstrap: remapped %d read-only data region(s) writable "
                    "(PSX-style writable literals)\n", fixed);
}

__attribute__((constructor))
static void PC_Bootstrap(void) {
    PC_MakeRodataWritable();            /* PSX-style writable string literals (see above) */
    signal(SIGUSR1, PC_SigUsr1);        /* kill -USR1 <pid> -> stack dump (freeze diagnosis) */
    signal(SIGSEGV, PC_SigCrash);       /* dump stack+state on a real crash before dying */
    signal(SIGBUS,  PC_SigCrash);
    const char *discPath = getenv("VH_DISC_IMAGE");
    if (!discPath) discPath = DefaultDiscPath();

    if (!PC_CdMount(discPath)) {
        fprintf(stderr, "PC_Bootstrap: failed to mount disc image '%s'\n", discPath);
        fprintf(stderr, "  (set VH_DISC_IMAGE to the real .bin path if this default is wrong)\n");
    } else {
        fprintf(stderr, "PC_Bootstrap: mounted disc image '%s'\n", discPath);
    }

    if (!PC_GpuInit(SCREEN_WIDTH, SCREEN_HEIGHT, "Vandal Hearts")) {
        fprintf(stderr, "PC_Bootstrap: failed to open a window (no display, or SDL2 issue)\n");
    } else {
        fprintf(stderr, "PC_Bootstrap: opened a %dx%d window\n", SCREEN_WIDTH, SCREEN_HEIGHT);
    }
}
