/* Regression fixture for src/pc_bootstrap.c's PC_MakePageWritable/PC_AddrInMainImage
 * write-fault retry. Compiled and run by fault_retry.sh; not part of the real game build. */

/* `#include`s src/pc_bootstrap.c directly since its crash-handling pieces are `static` (a
 * separate-TU link, like audio_bounds_test.c uses, can't reach them without pc_bootstrap.c
 * itself exporting them). */

/* -DVH_UNIFIED compiles out its automatic constructor hooks (disc mount, GPU init, .ini load);
 * only the rodata remap + SIGSEGV/SIGBUS handler are installed by hand below, the same few
 * lines PC_BootstrapRegion runs before disc mount. */

/* PC_BootstrapRegion() itself still compiles in (VH_UNIFIED only gates its constructor
 * wrapper), so the CD/GPU externs it references still need real symbols -- stubbed below,
 * never actually called by this fixture. */
#define VH_UNIFIED
#define VH_REGION_US
#include "pc_bootstrap.c"

#include <stdint.h>
#include <sys/prctl.h>

/* ---- stubs for externs PC_BootstrapRegion() references but this fixture never calls ---- */
int PC_CdMount(const char *path) { (void)path; return 0; }
long long PC_CdImageBytes(void) { return -1; }
int PC_CdDiscSignatureOk(void) { return 0; }
PC_DiscRelease PC_CdDiscRelease(void) { return VH_DISC_UNKNOWN; }
int PC_GpuInit(int w, int h, const char *title) { (void)w; (void)h; (void)title; return 0; }
void PC_GpuGetWindowSize(int *w, int *h, int *scale) { *w = *h = *scale = 0; }
void PC_ShowErrorBox(const char *title, const char *body) { (void)title; (void)body; }
void PC_DumpGameState(int fd) { (void)fd; }
int PC_GpuReplayTrace(const char *path) { (void)path; return 0; }
int g_vhFullscreen = 0;
int g_vhInternalScale = 1;

/* A buffer in THIS binary's own main image, on its own dedicated page: this test binary's whole
 * .bss is a few hundred bytes, so an ordinary static would share a page with pc_bootstrap.c's
 * own handler statics (s_crashHandlerActive, PC_MakePageWritable's dedup table, ...). */

/* ForceReadOnly below would then make THOSE unwritable too, and the handler could never even
 * record that it ran. */
static char s_literal[4096] __attribute__((aligned(4096))) =
    "fault_retry: a mutable-looking copy of a main-image literal";

/* Install just the rodata remap + crash handler PC_BootstrapRegion sets up before disc mount
 * (copied here, not factored out of pc_bootstrap.c -- see the file header comment). */
static void InstallHandler(void) {
    PC_MakeRodataWritable();
#if !defined(_WIN32)
    signal(SIGUSR1, PC_SigUsr1);
    {
        struct sigaction sa;
        static char s_sigStack[64 * 1024];
        stack_t st;
        memset(&st, 0, sizeof(st));
        st.ss_sp = s_sigStack;
        st.ss_size = sizeof(s_sigStack);
        sigaltstack(&st, NULL);
        memset(&sa, 0, sizeof(sa));
        sa.sa_sigaction = PC_SigCrash;
        sa.sa_flags = SA_SIGINFO | SA_NODEFER | SA_ONSTACK;
        sigemptyset(&sa.sa_mask);
        sigaction(SIGSEGV, &sa, NULL);
        sigaction(SIGBUS,  &sa, NULL);
    }
#endif
}

/* Force `addr`'s page back to read-only, undoing InstallHandler's own startup remap for just
 * that one page, so the write below genuinely faults into PC_SigCrash instead of silently
 * succeeding because the page was already writable. */
static void ForceReadOnly(const void *addr) {
    long ps = sysconf(_SC_PAGESIZE);
    uintptr_t page = (uintptr_t)addr & ~((uintptr_t)ps - 1);
    if (mprotect((void *)page, (size_t)ps, PROT_READ) != 0) {
        perror("fault_retry: mprotect(PROT_READ)");
        _exit(2);
    }
}

int main(int argc, char **argv) {
    if (argc != 2) {
        fprintf(stderr, "usage: %s main-image|outside\n", argv[0]);
        return 2;
    }
    InstallHandler();

    if (strcmp(argv[1], "main-image") == 0) {
        ForceReadOnly(s_literal);
        s_literal[0] = 'F';                 /* main-image page: PC_AddrInMainImage must accept */
        printf("fault_retry: main-image write survived, byte now '%c'\n", s_literal[0]);
        return 0;
    }
    if (strcmp(argv[1], "outside") == 0) {
        /* A fresh anonymous PROT_READ mapping: real memory, but outside every PT_LOAD segment
         * of the main executable, so it must NOT be retried. */

        /* (Not a libc symbol: writing into libc's own .text also strips its PROT_EXEC, which
         * crashes the process on its own the next time execution returns there -- a false
         * "PASS" regardless of this fix, which this mapping avoids.) */
        long ps = sysconf(_SC_PAGESIZE);
        unsigned char *p = mmap(NULL, (size_t)ps, PROT_READ, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);
        if (p == MAP_FAILED) { perror("fault_retry: mmap"); return 2; }
        /* This crash is the expected outcome: mark the process non-dumpable so the kernel
         * skips the core dump (and the host's crash notification) for it. */
        prctl(PR_SET_DUMPABLE, 0);
        *p = 0xCC;
        printf("fault_retry: BUG -- outside-main-image write survived, should have crashed\n");
        return 0;
    }
    fprintf(stderr, "usage: %s main-image|outside\n", argv[0]);
    return 2;
}
