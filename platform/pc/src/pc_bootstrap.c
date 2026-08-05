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
#define _GNU_SOURCE           /* REG_EIP/REG_EAX greg indices in <ucontext.h> (see NULL-read fixup) */
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <limits.h>
#include <stdint.h>
#include <dirent.h>           /* opendir/readdir -- disc-image auto-detect (portable; MinGW provides it) */

#if defined(_WIN32)
/* Windows (MinGW-w64, Stage 2.4): Win32 replaces the POSIX facilities used below -- VirtualAlloc for
 * the fixed PSX RAM ranges, VirtualProtect (over the PE sections) for the .rodata remap. There is no
 * POSIX signal/backtrace/mmap path here: the 64-bit build absorbs transient PSX NULL reads with
 * source-level PC_PORT guards, not a fault handler, so Windows needs no signal machinery to run. */
#include <windows.h>          /* VirtualAlloc, VirtualProtect, GetModuleHandle, PE headers, GetModuleFileNameA */
#else
#include <unistd.h>
#include <sys/mman.h>
#include <signal.h>
#include <execinfo.h>
#include <ucontext.h>         /* ucontext_t, greg_t, REG_* -- for the NULL-read fixup handler */
#include <fcntl.h>            /* open() for the null-read log */
#endif

#if defined(__APPLE__)
#include <mach-o/dyld.h>      /* _NSGetExecutablePath */
#endif

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
 * (grep across all of src/), none aliasing a real named symbol -- they're
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
#if defined(_WIN32)
    /* Win32 has no mmap: VirtualAlloc the fixed low address directly. On Win64 these PSX ranges
     * (0x1f800000, 0x80000000) sit in the low 2GB of a 128TB user space and are normally free; if
     * one is taken, VirtualAlloc returns a different/NULL address and we warn + continue, matching
     * the POSIX branch's non-fatal behaviour. */
    void *p = VirtualAlloc(base, size, MEM_RESERVE | MEM_COMMIT, PAGE_READWRITE);
    if (p == NULL || p != base) {
        fprintf(stderr, "PC_Bootstrap: could not reserve %s at %p (%s) -- "
                        "hardcoded scratch-buffer addresses in already-decompiled code "
                        "will crash if used\n",
                label, base, p == NULL ? "VirtualAlloc failed" : "got a different address");
        if (p) VirtualFree(p, 0, MEM_RELEASE);
        return 0;
    }
    fprintf(stderr, "PC_Bootstrap: reserved %s at %p (%zu bytes)\n", label, p, size);
    return 1;
#else
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
#endif /* _WIN32 */
}

__attribute__((constructor))
static void PC_ReservePsxRam(void) {
    ReservePsxMemory(PSX_RAM_BASE, PSX_RAM_SIZE, "PSX RAM range");
    ReservePsxMemory(PSX_SCRATCHPAD_BASE, PSX_SCRATCHPAD_SIZE, "PSX Scratchpad RAM");
    /* The KUSEG NULL-mirror (address 0) is deliberately NOT mapped: the portable NULL-read fixup
     * handler (see PC_SigCrash) absorbs transient NULL/low-address reads without privilege, and
     * letting the accesses fault is exactly what lets the handler log each site. */
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
/* Stage 2.4: the running executable's absolute path, per-OS. Returns 1 on success. Forward slashes
 * in the CONSTRUCTED path below work on all three (Win32 accepts '/'), but the RETURNED exe path may
 * use '\' on Windows, so the caller strips either separator. */
#ifndef PATH_MAX
#define PATH_MAX 4096
#endif
static int PC_GetExePath(char *out, size_t outSize) {
#if defined(_WIN32)
    DWORD n = GetModuleFileNameA(NULL, out, (DWORD)outSize);
    return (n > 0 && (size_t)n < outSize);
#elif defined(__APPLE__)
    uint32_t sz = (uint32_t)outSize;
    return _NSGetExecutablePath(out, &sz) == 0;   /* fails (sz set to needed) if buffer too small */
#else /* Linux + other /proc systems */
    ssize_t len = readlink("/proc/self/exe", out, outSize - 1);
    if (len <= 0) return 0;
    out[len] = '\0';
    return 1;
#endif
}

/* The executable's own directory (dirname of PC_GetExePath), stripping either separator. Returns 1
 * on success. Shared by the disc auto-detect and the .ini loader. */
static int PC_GetExeDir(char *out, size_t outSize) {
    char *sep, *bslash;
    if (!PC_GetExePath(out, outSize)) return 0;
    sep = strrchr(out, '/');
    bslash = strrchr(out, '\\');                    /* Windows separator */
    if (bslash && (!sep || bslash > sep)) sep = bslash;
    if (!sep) return 0;
    *sep = '\0';
    return 1;
}

/* The directory where the *end user's* files live (disc image, vandalhearts.ini). Normally this is
 * just the executable's own directory (PC_GetExeDir). But under an AppImage the executable runs from
 * a read-only squashfs mounted at /tmp/.mount_XXXX/usr/bin -- the user can't drop their disc there.
 * The AppImage runtime exports $APPIMAGE = the absolute path of the .AppImage file itself, so its
 * dirname is where the user actually keeps things. Prefer that when present; otherwise fall back to
 * the exe dir. Harmless on Windows/native Linux (env var simply unset). Returns 1 on success. */
int PC_GetDeployDir(char *out, size_t outSize) {
    const char *appimage = getenv("APPIMAGE");   /* set only when running as an AppImage */
    if (appimage && *appimage) {
        char *sep;
        snprintf(out, outSize, "%s", appimage);
        sep = strrchr(out, '/');
        if (sep) { *sep = '\0'; return 1; }
    }
    return PC_GetExeDir(out, outSize);
}

/* Portable "set env var" -- MinGW spells setenv() _putenv_s(). */
#if defined(_WIN32)
#define PC_Setenv(k, v) _putenv_s((k), (v))
#else
#define PC_Setenv(k, v) setenv((k), (v), 1)
#endif

/* Stage 2.4 QoL: load <exedir>/vandalhearts.ini so end users configure the game by editing a file
 * instead of setting environment variables. Format is plain `KEY=VALUE`, one per line; INI
 * [section] headers and ';' / '#' comment lines are ignored (sections are cosmetic -- keys are the
 * VH_* names directly). Precedence is env var > .ini > built-in default: a KEY already present in
 * the real environment is NOT overridden, so scripts/power users still win, and the file only fills
 * in what's unset. Runs at constructor priority 101 -- before PC_ReservePsxRam and
 * the window/audio init (VH_SCALE, ...) read anything. Absent file => silently all-defaults. */
__attribute__((constructor(101)))
static void PC_LoadIniConfig(void) {
    char dir[PATH_MAX], iniPath[PATH_MAX + 32], line[512];
    FILE *f;
    if (!PC_GetDeployDir(dir, sizeof(dir))) return;   /* AppImage: next to the .AppImage, not the mount */
    snprintf(iniPath, sizeof(iniPath), "%s/vandalhearts.ini", dir);
    f = fopen(iniPath, "r");
    if (!f) return;
    while (fgets(line, sizeof(line), f)) {
        char *p = line, *eq, *key, *val, *end;
        while (*p == ' ' || *p == '\t') p++;
        if (*p == ';' || *p == '#' || *p == '[' || *p == '\0' || *p == '\n' || *p == '\r') continue;
        eq = strchr(p, '=');
        if (!eq) continue;
        *eq = '\0';
        key = p;
        val = eq + 1;
        end = key + strlen(key);                    /* trim trailing ws on key */
        while (end > key && (end[-1] == ' ' || end[-1] == '\t')) *--end = '\0';
        while (*val == ' ' || *val == '\t') val++;   /* trim leading ws on value */
        /* Strip an inline comment: the first ';' or '#' that is whitespace-preceded (or at the very
         * start) begins a comment (`VH_X=1 ; note` -> "1"). Requiring leading whitespace keeps a ';'/'#'
         * that is genuinely part of a value (e.g. a path) intact. */
        { char *c;
          for (c = val; *c; c++)
              if ((*c == ';' || *c == '#') && (c == val || c[-1] == ' ' || c[-1] == '\t')) { *c = '\0'; break; } }
        end = val + strlen(val);                     /* trim trailing ws/newline on value */
        while (end > val && (end[-1]=='\n'||end[-1]=='\r'||end[-1]==' '||end[-1]=='\t')) *--end = '\0';
        if (strncmp(key, "VH_", 3) != 0) continue;   /* only our own keys */
        if (*val == '\0' || getenv(key) != NULL) continue;  /* env var wins; skip empty */
        PC_Setenv(key, val);
        fprintf(stderr, "PC_Config: %s=%s (from vandalhearts.ini)\n", key, val);
    }
    fclose(f);
}

/* Does `line` set (or comment out) `key`? Matches "KEY=", ";KEY=", "# KEY =", etc. -- an optional
 * leading comment marker, then the key, optional ws, '='. On a match, *inlineCmt (if non-NULL) is set
 * to the ';'/'#' inline-comment tail on that line (past the '='), or NULL if there is none, so the
 * rewrite can preserve the human-readable note (e.g. "; horizontal (rotate direction)"). */
static int PC_IniLineIsKey(const char *line, const char *key, const char **inlineCmt) {
    const char *p = line;
    size_t klen = strlen(key);
    while (*p == ' ' || *p == '\t') p++;
    if (*p == ';' || *p == '#') { p++; while (*p == ' ' || *p == '\t') p++; }
    if (strncmp(p, key, klen) != 0) return 0;
    p += klen;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '=') return 0;
    if (inlineCmt) {
        const char *c = p + 1, *cm = NULL;                 /* scan the value region for ';' or '#' */
        for (; *c && *c != '\n' && *c != '\r'; c++)
            if (*c == ';' || *c == '#') { cm = c; break; }
        *inlineCmt = cm;
    }
    return 1;
}

/* If `line` is a "[name]" section header, copy `name` (trimmed) into nameOut and return 1, else 0. */
static int PC_IniHeaderName(const char *line, char *nameOut, size_t cap) {
    const char *p = line, *e;
    size_t n;
    while (*p == ' ' || *p == '\t') p++;
    if (*p != '[') return 0;
    p++;
    e = p;
    while (*e && *e != ']' && *e != '\n' && *e != '\r') e++;
    if (*e != ']') return 0;
    n = (size_t)(e - p);
    if (n >= cap) n = cap - 1;
    memcpy(nameOut, p, n);
    nameOut[n] = '\0';
    return 1;
}

/* Case-insensitive string equality (ASCII), for matching section names regardless of case. */
static int PC_CiEq(const char *a, const char *b) {
    for (; *a && *b; a++, b++)
        if ((*a | 0x20) != (*b | 0x20)) return 0;
    return *a == *b;
}

/* Write "KEY=VALUE" to `out`, re-appending a trimmed inline comment if the original line had one. */
static void PC_IniWriteKeyLine(FILE *out, const char *key, const char *value, const char *inlineCmt) {
    fprintf(out, "%s=%s", key, value);
    if (inlineCmt) {
        char cbuf[256]; size_t i = 0;
        while (inlineCmt[i] && inlineCmt[i] != '\n' && inlineCmt[i] != '\r' && i < sizeof(cbuf) - 1) {
            cbuf[i] = inlineCmt[i]; i++;
        }
        while (i > 0 && (cbuf[i-1] == ' ' || cbuf[i-1] == '\t')) i--;   /* trim trailing ws */
        cbuf[i] = '\0';
        fprintf(out, "    %s", cbuf);
    }
    fputc('\n', out);
}

/* See pc_platform.h. Surgical single-key ini write; keeps the file otherwise byte-for-byte.
 *
 * Two passes over the (tiny) file so the three cases stay unambiguous and never fight each other:
 *   1. key already present (active or commented, in any section) -> replace that line in place,
 *      preserving its inline comment. Highest priority, so a stray value is never duplicated.
 *   2. key absent but its [section] exists -> insert the line at the END of that section (before the
 *      next header, or at EOF), so it joins the existing section rather than spawning a new one.
 *   3. neither -> append a fresh [section] header + the line (also the from-scratch/no-file case). */
int PC_SaveIniConfig(const char *section, const char *key, const char *value) {
    char dir[PATH_MAX], iniPath[PATH_MAX + 32], tmpPath[PATH_MAX + 40], line[512], hdr[128];
    FILE *in, *out;
    int keyExists = 0, sectionExists = 0, keyWritten = 0, inTarget = 0;
    if (!section) section = "";
    if (!key || !value) return 0;
    if (!PC_GetDeployDir(dir, sizeof(dir))) return 0;
    snprintf(iniPath, sizeof(iniPath), "%s/vandalhearts.ini", dir);
    snprintf(tmpPath, sizeof(tmpPath), "%s/vandalhearts.ini.tmp", dir);

    /* Pass 1: does the key exist anywhere, and does the target section exist? */
    in = fopen(iniPath, "r");
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            if (PC_IniHeaderName(line, hdr, sizeof(hdr))) {
                if (section[0] && PC_CiEq(hdr, section)) sectionExists = 1;
            } else if (PC_IniLineIsKey(line, key, NULL)) {
                keyExists = 1;
            }
        }
        fclose(in);
    }

    /* Pass 2: rewrite. */
    out = fopen(tmpPath, "w");
    if (!out) return 0;
    in = fopen(iniPath, "r");
    if (in) {
        while (fgets(line, sizeof(line), in)) {
            const char *inlineCmt = NULL;
            if (keyExists) {                                  /* case 1: replace in place */
                if (!keyWritten && PC_IniLineIsKey(line, key, &inlineCmt)) {
                    PC_IniWriteKeyLine(out, key, value, inlineCmt);
                    keyWritten = 1;
                    continue;
                }
            } else if (sectionExists && PC_IniHeaderName(line, hdr, sizeof(hdr))) {
                /* case 2: leaving a section -- if it was the target and we haven't inserted, do it now */
                if (inTarget && !keyWritten) { PC_IniWriteKeyLine(out, key, value, NULL); keyWritten = 1; }
                inTarget = (section[0] && PC_CiEq(hdr, section));
            }
            fputs(line, out);
        }
        fclose(in);
    }
    if (!keyWritten && sectionExists && inTarget)             /* case 2: target was the last section */
        { PC_IniWriteKeyLine(out, key, value, NULL); keyWritten = 1; }
    if (!keyWritten) {                                        /* case 3: no key, no section (or no file) */
        if (section[0]) fprintf(out, "\n[%s]\n", section);
        PC_IniWriteKeyLine(out, key, value, NULL);
    }
    fclose(out);

#if defined(_WIN32)
    remove(iniPath);                 /* MinGW rename() won't clobber an existing target */
#endif
    if (rename(tmpPath, iniPath) != 0) { remove(tmpPath); return 0; }
    return 1;
}

/* Case-insensitive ".bin" suffix test (no strcasecmp dependency -- MinGW spells it _stricmp). */
static int HasBinExt(const char *name) {
    size_t n = strlen(name);
    const char *e;
    if (n < 4) return 0;
    e = name + n - 4;
    return e[0] == '.' && (e[1]|0x20) == 'b' && (e[2]|0x20) == 'i' && (e[3]|0x20) == 'n';
}

/* Return (into out) the first "*.bin" found directly inside `dir`, or 0 if none / dir unreadable.
 * Forward slash in the joined path is fine on all three OSes (Win32 accepts '/'). */
static int FirstBinInDir(const char *dir, char *out, size_t outSize) {
    DIR *d = opendir(dir);
    struct dirent *ent;
    if (!d) return 0;
    while ((ent = readdir(d)) != NULL) {
        if (HasBinExt(ent->d_name)) {
            snprintf(out, outSize, "%s/%s", dir, ent->d_name);
            closedir(d);
            return 1;
        }
    }
    closedir(d);
    return 0;
}

/* Locate the disc image with NO configuration needed for the common cases (Stage 2.4 QoL). Anchored
 * to the executable's own directory (via PC_GetExePath, cwd-independent), tried in order:
 *   1. a `game/` folder next to the .exe holding a *.bin  -- the recommended portable-binary layout;
 *   2. a *.bin sitting directly beside the .exe;
 *   3. the dev repo layout (game/ four levels up from platform/pc/build*).
 * VH_DISC_IMAGE (checked by the caller) still overrides all of this. Whatever this returns, a failed
 * mount prints the path + a hint, so a wrong guess is self-explanatory rather than silent. */
static const char *DefaultDiscPath(void) {
    static char path[PATH_MAX + 64];
    char deployDir[PATH_MAX], exeDir[PATH_MAX], cand[PATH_MAX + 16];
    /* Where the user keeps their disc: next to the .AppImage when packaged, else the exe dir. */
    if (PC_GetDeployDir(deployDir, sizeof(deployDir))) {
        /* 1. `game/` subfolder there */
        snprintf(cand, sizeof(cand), "%s/game", deployDir);
        if (FirstBinInDir(cand, path, sizeof(path))) return path;
        /* 2. a .bin sitting directly there */
        if (FirstBinInDir(deployDir, path, sizeof(path))) return path;
    }
    if (!PC_GetExeDir(exeDir, sizeof(exeDir)))
        return "../external/game/Vandal Hearts (USA).bin"; /* exe path unavailable: cwd-relative fallback */
    /* 3. dev build layout: the disc lives in vh/external/game (three levels up from
     * platform/pc/build*, then external/game). VH_DISC_IMAGE overrides this. */
    snprintf(path, sizeof(path), "%s/../../../external/game/Vandal Hearts (USA).bin", exeDir);
    return path;
}

/* Diagnostics (PC-only): dump the current call stack + game state to stderr. Used two ways:
 *   - SIGUSR1 (`kill -USR1 <pid>`): sample a running/hung process without gdb/ptrace (the setcap
 *     binary makes those awkward), then keep running -- for "freeze" (state-stall) diagnosis.
 *   - SIGSEGV/SIGBUS: dump on a real crash before dying, so the window doesn't just vanish -- the
 *     backtrace (+ symbol names, linked -rdynamic) + gState point at where it crashed.
 * addr2line -e the binary on any bare addresses. (POSIX-only: backtrace()/signals. Windows relies on
 * source-level PC_PORT guards + the debugger, so this diagnostics path is compiled out there.) */
#if !defined(_WIN32)
static void PC_DumpDiag(const char *tag) {
    void *frames[64];
    int n = backtrace(frames, 64);
    ssize_t w = write(2, tag, strlen(tag)); (void)w;
    backtrace_symbols_fd(frames, n, 2);
    { extern void PC_DumpGameState(int fd); PC_DumpGameState(2); }   /* which state/scene/map */
}
static void PC_SigUsr1(int sig) { (void)sig; PC_DumpDiag("\n*** SIGUSR1: call stack (frozen?) ***\n"); }
#endif /* !_WIN32 */

/* ---- NULL-read fixup (Stage 2.2): make a transient PSX-style NULL/low-address access survive on a
 * native host, portably, instead of needing the CAP_SYS_RAWIO low-page mapping. On PSX, address 0
 * (KUSEG) is real 2MB RAM, so game code that transiently dereferences a not-yet-assigned pointer
 * just reads garbage for a frame; on a host, address 0 faults. Rather than map address 0 (privileged,
 * and impossible on Windows/macOS), we catch the fault, emulate the access as reading 0 (identical to
 * what the old MAP_ANONYMOUS zero page returned) or discarding the store, step over the instruction,
 * log the site once, and continue. Un-guarded sites therefore no longer crash -- they surface in
 * vh_null_reads.log so they can be given an explicit source-level guard later. Currently x86-32
 * (-m32) only. (The old privileged low-page-mapping fallback was retired -- this handler is the sole
 * path; nothing runs privileged.) */

#if defined(__i386__) && !defined(_WIN32)
/* Decode the memory-access instruction at `ip` (the faulting one). Returns its length and, for a
 * load, the destination greg index + width to zero; for a store, isWrite=1 (nothing to zero, just
 * skip). Returns 0 for forms we don't handle yet (caller then crashes with diagnostics so this can be
 * extended). Covers what gcc -O0 emits for struct-field loads/stores: mov / movzx / movsx, with the
 * operand-size prefix and full ModRM/SIB/disp addressing. */
static int VhDecodeMemAccess(const unsigned char *ip, int *outIsWrite, int *outGreg, int *outBytes, int *outHigh) {
    int i = 0, opsize = 4, has0F = 0;
    for (;;) {                                   /* prefixes */
        unsigned char b = ip[i];
        if (b == 0x66) { opsize = 2; i++; continue; }
        if (b == 0x67) return 0;                 /* 16-bit addressing: bail (not emitted at -O0) */
        if (b == 0xF0 || b == 0xF2 || b == 0xF3) { i++; continue; }
        if (b == 0x2E || b == 0x36 || b == 0x3E || b == 0x26 || b == 0x64 || b == 0x65) { i++; continue; }
        break;
    }
    unsigned char op = ip[i++], op2 = 0;
    if (op == 0x0F) { has0F = 1; op2 = ip[i++]; }

    int isWrite, bytes, immBytes = 0;
    if (!has0F) {
        switch (op) {
            case 0x8B: isWrite = 0; bytes = opsize; break;                  /* mov r,   r/m   */
            case 0x8A: isWrite = 0; bytes = 1;      break;                  /* mov r8,  r/m8  */
            case 0x89: isWrite = 1; bytes = opsize; break;                  /* mov r/m, r     */
            case 0x88: isWrite = 1; bytes = 1;      break;                  /* mov r/m8,r8    */
            case 0xC7: isWrite = 1; bytes = opsize; immBytes = opsize; break;/* mov r/m, imm  */
            case 0xC6: isWrite = 1; bytes = 1;      immBytes = 1;      break;/* mov r/m8,imm8 */
            default: return 0;
        }
    } else {
        switch (op2) {
            case 0xB6: case 0xB7: case 0xBE: case 0xBF: isWrite = 0; bytes = 4; break; /* movzx/movsx r32 */
            default: return 0;
        }
    }

    unsigned char modrm = ip[i++];
    int mod = modrm >> 6, reg = (modrm >> 3) & 7, rm = modrm & 7;
    if (mod == 3) return 0;                        /* register operand -> not the faulting memory op */
    int sibBase = -1;
    if (rm == 4) { unsigned char sib = ip[i++]; sibBase = sib & 7; }     /* SIB */
    if (mod == 1) i += 1;                          /* disp8  */
    else if (mod == 2) i += 4;                     /* disp32 */
    else if (rm == 5 && sibBase < 0) i += 4;       /* mod0, rm5    -> disp32 */
    else if (sibBase == 5) i += 4;                 /* mod0, sib b5 -> disp32 */
    i += immBytes;

    *outIsWrite = isWrite;
    if (!isWrite) {
        if (bytes == 1) {                          /* 8-bit reg: 0-3 low byte, 4-7 high byte, of A/C/D/B */
            if (reg < 4) { *outGreg = REG_EAX - reg;       *outHigh = 0; }
            else         { *outGreg = REG_EAX - (reg - 4); *outHigh = 1; }
            *outBytes = 1;
        } else {
            *outGreg = REG_EAX - reg;              /* modrm.reg 0..7 = EAX..EDI = greg REG_EAX-reg */
            *outBytes = bytes;
            *outHigh = 0;
        }
    }
    return i;
}
#endif /* __i386__ && !_WIN32 */

#if !defined(_WIN32)
#if defined(__i386__)   /* only the i386 NULL-decode fixup below calls this */
/* Log a fixed-up NULL-region access once per unique instruction pointer (async-signal-safe-ish:
 * open/write/backtrace_symbols_fd, no malloc-heavy fprintf). */
static void PC_LogNullRead(void *ip, uintptr_t fault, int isWrite) {
    static void *seen[512];
    static int seenCount = 0;
    static int logFd = -2;
    int k;
    for (k = 0; k < seenCount; k++) if (seen[k] == ip) return;   /* already reported this site */
    if (seenCount < 512) seen[seenCount++] = ip;
    if (logFd == -2) logFd = open("vh_null_reads.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
    char buf[192];
    int n = snprintf(buf, sizeof(buf),
                     "\n--- NULL-region %s  fault=0x%08lx  eip=%p  (distinct site #%d) ---\n",
                     isWrite ? "WRITE" : "read", (unsigned long)fault, ip, seenCount);
    if (n > 0) {
        if (logFd >= 0) { ssize_t w = write(logFd, buf, (size_t)n); (void)w; }
        ssize_t w2 = write(2, buf, (size_t)n); (void)w2;
        void *frames[24];
        int fn = backtrace(frames, 24);
        if (logFd >= 0) backtrace_symbols_fd(frames, fn, logFd);
        backtrace_symbols_fd(frames, fn, 2);
    }
}
#endif /* __i386__ -- PC_LogNullRead */

/* Stage 2.3: available on x86-64 as well as x86-32. `REG_ERR` and the page-fault error-code
 * layout are identical on both -- bit 1 set means the access was a write. This must NOT be
 * gated to __i386__ along with the NULL instruction decoder: the .rodata fixup below depends
 * on it and is still required under -m64 (the game mutates string literals in place). Windows is
 * excluded: it has no ucontext_t/mprotect fault path -- the startup PE-section remap (below)
 * makes .rodata writable there without any on-fault handler. */
#if defined(__i386__) || defined(__x86_64__)
#define PC_HAVE_WRITE_FAULT_INFO 1
static int PC_IsWriteFault(void *ucv) {   /* x86 page-fault error code bit 1 == write */
    return (((ucontext_t *)ucv)->uc_mcontext.gregs[REG_ERR] & 0x2) != 0;
}

/* Lazily make the page containing `addr` writable, to satisfy a write to the executable's read-only
 * .rodata -- the game mutates string literals in place (e.g. ShowExpDialog writing the EXP digits
 * into "You got     "), harmless on PSX where all RAM is writable, a SIGSEGV on a host. This replaces
 * the old startup PC_MakeRodataWritable() that parsed /proc/self/maps and remapped ALL rodata RW:
 * this is portable (no /proc), on-demand (only pages actually written), and logs each site. Returns
 * 1 if the page is now writable (retry the store) or 0 if mprotect refused it (a genuine wild write
 * -> crash). Dedups by page, which also guards against an infinite retry loop. */
static int PC_MakePageWritable(uintptr_t addr) {
    static uintptr_t seen[128];
    static int seenCount = 0;
    long ps = sysconf(_SC_PAGESIZE);
    if (ps <= 0) ps = 4096;
    uintptr_t page = addr & ~((uintptr_t)ps - 1);
    int i;
    for (i = 0; i < seenCount; i++)
        if (seen[i] == page) return 0;         /* remapped once already yet still faulting -> give up */
    if (mprotect((void *)page, (size_t)ps, PROT_READ | PROT_WRITE) != 0) return 0;  /* not our page */
    if (seenCount < 128) seen[seenCount++] = page;
    {   /* log the site once */
        static int logFd = -2;
        if (logFd == -2) logFd = open("vh_null_reads.log", O_WRONLY | O_CREAT | O_APPEND, 0644);
        char buf[160];
        int n = snprintf(buf, sizeof(buf),
                         "\n--- read-only-DATA write  fault=0x%08lx  (page 0x%08lx -> writable) ---\n",
                         (unsigned long)addr, (unsigned long)page);
        if (n > 0) {
            if (logFd >= 0) { ssize_t w = write(logFd, buf, (size_t)n); (void)w; }
            ssize_t w2 = write(2, buf, (size_t)n); (void)w2;
            void *frames[24];
            int fn = backtrace(frames, 24);
            if (logFd >= 0) backtrace_symbols_fd(frames, fn, logFd);
            backtrace_symbols_fd(frames, fn, 2);
        }
    }
    return 1;
}
#endif /* __i386__ || __x86_64__ */

static void PC_SigCrash(int sig, siginfo_t *si, void *ucv) {
    uintptr_t fault = (uintptr_t)(si ? si->si_addr : 0);
#if defined(__i386__)
    /* PSX NULL-region (< 2MB main-RAM size) access: emulate reading 0 / discarding the store and
     * carry on, instead of dying. */
    if (ucv && fault < PSX_NULL_MIRROR_SIZE) {
        ucontext_t *uc = (ucontext_t *)ucv;
        greg_t *g = uc->uc_mcontext.gregs;
        unsigned char *ip = (unsigned char *)(uintptr_t)g[REG_EIP];
        int isWrite = 0, greg = 0, bytes = 0, high = 0;
        int len = VhDecodeMemAccess(ip, &isWrite, &greg, &bytes, &high);
        if (len > 0) {
            if (!isWrite) {
                if (bytes == 1)      { if (high) g[greg] &= ~0xff00L; else g[greg] &= ~0xffL; }
                else if (bytes == 2) g[greg] &= ~0xffffL;
                else                 g[greg] = 0;
            }
            g[REG_EIP] = (greg_t)((uintptr_t)ip + len);
            PC_LogNullRead(ip, fault, isWrite);
            return;                                /* resume the game */
        }
        PC_DumpDiag("\n*** NULL-region fault but UNDECODABLE instruction -- extend VhDecodeMemAccess ***\n");
    }
#endif /* __i386__ -- NULL instruction-decode fixup ends here */

    /* Write to the executable's read-only .rodata (in-place string-literal mutation): make the page
     * writable and retry the store -- no instruction decode needed, and no /proc (see above).
     *
     * Stage 2.3: this is OUTSIDE the __i386__ gate on purpose. It used to sit inside it, which
     * meant the -m64 build would have silently lost the .rodata fixup and hard-crashed on the
     * first string-literal mutation (ShowExpDialog writing EXP digits into a literal -- the one
     * fault still present in vh_null_reads.log after the Step-A guard pass). Unlike the NULL
     * decoder, nothing here is 32-bit-specific: si_addr + the x86 write bit + mprotect. */
#if defined(PC_HAVE_WRITE_FAULT_INFO)
    if (ucv && fault >= PSX_NULL_MIRROR_SIZE && PC_IsWriteFault(ucv) && PC_MakePageWritable(fault)) {
        return;                                    /* page now writable -> retry the faulting store */
    }
#endif
    PC_DumpDiag("\n*** CRASH: fatal signal, call stack ***\n");
    signal(sig, SIG_DFL); raise(sig);   /* restore default + re-raise so it dies for real (core, etc.) */
}
#endif /* !_WIN32 -- POSIX diagnostics + NULL/rodata on-fault handlers */

/* ---- startup .rodata RW remap (Stage 2.4, cross-platform) -------------------------------------
 *
 * The game mutates read-only string literals in place (e.g. ShowExpDialog writes the EXP digits
 * into "You got     "), harmless on PSX where all RAM is writable. Two mechanisms have existed:
 * an old startup remap that parsed /proc/self/maps (Linux-only), and the on-demand
 * PC_MakePageWritable() fault path above. The fault path is not portable: it needs a POSIX SIGSEGV
 * handler (not Windows) and the x86 page-fault write bit (not ARM/Apple Silicon).
 *
 * So the running path is made signal/arch-free again, portably this time: at startup, make the
 * executable's read-only DATA segments writable. No /proc, no signals, no instruction decode. The
 * on-fault path stays as an x86 safety net for anything a platform's remap misses. Per-platform:
 *   - Linux: dl_iterate_phdr -> mprotect each PF_R-only PT_LOAD of the main program.
 *   - Windows (MinGW): PE section walk of the main module -> VirtualProtect each read-only,
 *     non-executable initialized-data section (.rdata) to PAGE_READWRITE.
 *   - macOS (Apple Silicon): dyld segment walk + mprotect  -- TODO, its 2.4 phase.
 * Best-effort: failures are non-fatal (the on-fault path or a later crash will surface a real
 * problem); success just means string-literal writes never fault. */
#if defined(__linux__)
#include <link.h>   /* dl_iterate_phdr, ElfW, PT_LOAD, PF_* */
static int PC_RodataPhdrCb(struct dl_phdr_info *info, size_t size, void *unused) {
    (void)size; (void)unused;
    /* Main program only (listed first); shared-lib rodata needn't be touched. */
    static int done = 0;
    if (done) return 1;
    done = 1;
    long ps = sysconf(_SC_PAGESIZE);
    if (ps <= 0) ps = 4096;
    for (int i = 0; i < info->dlpi_phnum; i++) {
        const ElfW(Phdr) *p = &info->dlpi_phdr[i];
        if (p->p_type != PT_LOAD) continue;
        if (p->p_flags & (PF_W | PF_X)) continue;   /* already writable, or code (leave RX) */
        uintptr_t start = (uintptr_t)info->dlpi_addr + p->p_vaddr;
        uintptr_t end   = start + p->p_memsz;
        uintptr_t pstart = start & ~((uintptr_t)ps - 1);
        uintptr_t pend   = (end + (uintptr_t)ps - 1) & ~((uintptr_t)ps - 1);
        mprotect((void *)pstart, (size_t)(pend - pstart), PROT_READ | PROT_WRITE);
    }
    return 1;
}
static void PC_MakeRodataWritable(void) { dl_iterate_phdr(PC_RodataPhdrCb, NULL); }
#elif defined(_WIN32)
/* Windows (MinGW): walk the main module's PE section table and make every read-only,
 * non-executable initialized-data section (.rdata, where string literals live) writable. This is
 * the whole .rodata-write story on Windows -- there is no on-fault fallback here. */
static void PC_MakeRodataWritable(void) {
    HMODULE mod = GetModuleHandleA(NULL);
    if (!mod) return;
    BYTE *base = (BYTE *)mod;
    IMAGE_DOS_HEADER *dos = (IMAGE_DOS_HEADER *)base;
    if (dos->e_magic != IMAGE_DOS_SIGNATURE) return;
    IMAGE_NT_HEADERS *nt = (IMAGE_NT_HEADERS *)(base + dos->e_lfanew);
    if (nt->Signature != IMAGE_NT_SIGNATURE) return;
    IMAGE_SECTION_HEADER *sec = IMAGE_FIRST_SECTION(nt);
    unsigned i;
    for (i = 0; i < nt->FileHeader.NumberOfSections; i++) {
        DWORD ch = sec[i].Characteristics;
        if ((ch & IMAGE_SCN_MEM_READ) && !(ch & IMAGE_SCN_MEM_WRITE) && !(ch & IMAGE_SCN_MEM_EXECUTE)) {
            void *addr = base + sec[i].VirtualAddress;
            SIZE_T sz = sec[i].Misc.VirtualSize ? sec[i].Misc.VirtualSize : sec[i].SizeOfRawData;
            DWORD old;
            VirtualProtect(addr, sz, PAGE_READWRITE, &old);
        }
    }
}
#else
static void PC_MakeRodataWritable(void) {
    /* macOS: implemented in its Stage-2.4 phase (dyld segment walk + mprotect). Until then that
     * build relies on the on-fault path, so this is where its startup remap goes. */
}
#endif

/* Fatal startup error the user can actually act on (wrong/missing disc). Prints to stderr (visible
 * in a console launch) AND, on Windows, pops a message box so double-click users -- who have no
 * console -- still see it. `path` is the disc path we tried, appended so it's clear what was wrong.
 * Then exits: booting on into a blank window / garbage would only confuse. */
void PC_FatalDiscError(const char *title, const char *body, const char *path) {   /* non-static: libcd.c's corruption guards use it (pc_platform.h) */
    fprintf(stderr, "\n*** %s ***\n%s\nDisc path tried: %s\n", title, body, path);
#if defined(_WIN32)
    {
        char full[PATH_MAX + 512];
        snprintf(full, sizeof(full), "%s\n\nDisc path tried:\n%s", body, path);
        MessageBoxA(NULL, full, title, MB_OK | MB_ICONERROR);
    }
#endif
    exit(1);
}

__attribute__((constructor))
static void PC_Bootstrap(void) {
    /* GPU-trace replay mode (regression harness): feed a recorded trace straight through the
     * rasterizer and print the deterministic VRAM signature -- no game, no disc, no window.
     * See tools/regress/raster_check.sh. Must run before any other bootstrap work. */
    { const char *rp = getenv("VH_GPU_REPLAY");
      if (rp && *rp) { extern int PC_GpuReplayTrace(const char *path); exit(PC_GpuReplayTrace(rp)); } }

    PC_MakeRodataWritable();            /* make string-literal writes work without faulting (portable) */
#if !defined(_WIN32)
    signal(SIGUSR1, PC_SigUsr1);        /* kill -USR1 <pid> -> stack dump (freeze diagnosis) */
    /* SIGSEGV/SIGBUS via sigaction+SA_SIGINFO so the handler gets the faulting address (si_addr) and
     * CPU context (for the NULL-read fixup). SA_NODEFER lets a genuine re-fault inside the handler
     * still terminate rather than deadlock. */
    {
        struct sigaction sa;
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = PC_SigCrash;
        sa.sa_flags = SA_SIGINFO | SA_NODEFER;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS,  &sa, NULL);
    }
#endif /* !_WIN32 -- no POSIX signal handlers on Windows (see notes above) */
    const char *discPath = getenv("VH_DISC_IMAGE");
    if (!discPath) discPath = DefaultDiscPath();

    if (!PC_CdMount(discPath)) {
        PC_FatalDiscError("Vandal Hearts - disc image not found",
            "Could not open a game disc image.\n\n"
            "Put your Vandal Hearts (USA) .bin file in a \"game\" folder next to the "
            "executable (or right beside it), or set the disc path via VH_DISC_IMAGE in "
            "vandalhearts.ini (or the environment).", discPath);
    }
    /* Corruption guard 1/3 (post-1.6.1, from a real user report): a raw .bin is a whole number of
     * 2352-byte sectors, and an interrupted copy/download almost never is. Catching it here turns a
     * silent boot hang (the loader retrying garbage forever) into an error the user can act on. */
    {
        long long sz = PC_CdImageBytes();
        if (sz > 0 && (sz % 2352) != 0) {
            PC_FatalDiscError("Vandal Hearts - disc image is incomplete",
                "This disc image looks TRUNCATED: its size is not a whole number of raw CD "
                "sectors (2352 bytes), which usually means the file was only partially "
                "copied or downloaded.\n\n"
                "Re-copy or re-dump your Vandal Hearts (USA) disc and try again.", discPath);
        }
    }
    if (!PC_CdDiscSignatureOk()) {
        PC_FatalDiscError("Vandal Hearts - wrong disc image",
            "This does not look like a Vandal Hearts (USA) disc image: the SLUS_004.47 boot "
            "signature is missing.\n\n"
            "Use a raw .bin dump of Vandal Hearts (USA) (not a different game, region, or a "
            ".cue/.iso file).", discPath);
    }
    fprintf(stderr, "PC_Bootstrap: mounted disc image '%s'\n", discPath);

    if (!PC_GpuInit(SCREEN_WIDTH, SCREEN_HEIGHT, "Vandal Hearts")) {
        fprintf(stderr, "PC_Bootstrap: failed to open a window (no display, or SDL2 issue)\n");
    } else {
        int ww = SCREEN_WIDTH, wh = SCREEN_HEIGHT, sc = 1;
        int isc = g_vhInternalScale > 0 ? g_vhInternalScale : 1;
        PC_GpuGetWindowSize(&ww, &wh, &sc);
        /* Report all three distinct resolutions accurately: the fullscreen/windowed presentation, the
         * native (logical) framebuffer, and -- when G2 supersampling is on -- the internal render size. */
        if (g_vhFullscreen)
            fprintf(stderr, "PC_Bootstrap: opened a fullscreen window (native %dx%d",
                    SCREEN_WIDTH, SCREEN_HEIGHT);
        else
            fprintf(stderr, "PC_Bootstrap: opened a %dx%d window (VH_SCALE=%d, native %dx%d",
                    ww, wh, sc, SCREEN_WIDTH, SCREEN_HEIGHT);
        if (isc > 1)
            fprintf(stderr, ", internal render %dx%d [VH_INTERNAL_SCALE=%d]",
                    SCREEN_WIDTH * isc, SCREEN_HEIGHT * isc, isc);
        fprintf(stderr, ")\n");
    }
}

/* ---- Stage 2.3 UI-visibility probe (PC_DEBUG_UI_LOG) ------------------------------------
 * The -m64 build renders terrain, sprites and the damage-number correctly but drops the Vandal
 * Hearts logo, the compass and every textbox. Windows never call AddPrim themselves -- they
 * compose into VRAM and spawn child sprite objects that AddObjPrim2 draws. This logs both ends
 * so one run distinguishes "the window object never runs" from "it runs but its child sprites
 * are hidden / off-screen / at a bad OT index". Enabled with VH_UI_LOG=1; the hooks that call it
 * are PC_DEBUG_UI_LOG-gated and compile out of the matching build entirely. */
void PC_DebugUiLog(const char *tag, int a, int b, int c, int d, int e, int f, int g, int h) {
    static FILE *fp = NULL;
    static int enabled = -1, lines = 0;
    if (enabled < 0) { const char *e2 = getenv("VH_UI_LOG"); enabled = (e2 && e2[0] == '1'); }
    if (!enabled || lines > 4000) return;
    if (!fp) { fp = fopen("vh_ui_log.txt", "w");
               if (!fp) { enabled = 0; return; }
               fprintf(fp, "tag,a,b,c,d,e,f,g,h\n"); }
    fprintf(fp, "%s,%d,%d,%d,%d,%d,%d,%d,%d\n", tag, a, b, c, d, e, f, g, h);
    if ((++lines % 64) == 0) fflush(fp);
}
