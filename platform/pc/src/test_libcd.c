/* Standalone check: reads SIBAI1_1.DAT (gCdFiles[CDF_SIBAI1_1_DAT] = {0x27e8, ...}) through the
 * same CdControl(CdlSetloc)+CdRead flow core/cd.c uses and compares the bytes against an
 * independently-extracted reference. Not part of the game build. */
#include <stdio.h>
#include <string.h>
#include "PsyQ/libcd.h"
#include "pc_platform.h"

int main(int argc, char **argv) {
    if (argc < 3) {
        fprintf(stderr, "usage: %s <disc.bin> <reference_SIBAI1_1.DAT>\n", argv[0]);
        return 1;
    }

    if (!PC_CdMount(argv[1])) {
        fprintf(stderr, "PC_CdMount failed\n");
        return 1;
    }
    if (!CdInit()) {
        fprintf(stderr, "CdInit failed\n");
        return 1;
    }

    /* Exactly the flow ContinueLoadingCdFile() in src/core/cd.c uses. */
    CdlLOC loc;
    CdIntToPos(0x27e8, &loc); /* gCdFiles[CDF_SIBAI1_1_DAT].startingSector */
    printf("MSF for LBA 0x27e8: %02x:%02x:%02x (BCD)\n", loc.minute, loc.second, loc.sector);

    if (CdControl(CdlSetloc, (u_char *)&loc, NULL) == 0) {
        fprintf(stderr, "CdControl(CdlSetloc) failed\n");
        return 1;
    }
    if (CdSync(1, NULL) != CdlComplete) {
        fprintf(stderr, "CdSync did not report complete\n");
        return 1;
    }

    unsigned char buf[2048];
    if (CdRead(1, (u_long *)buf, 0) == 0) {
        fprintf(stderr, "CdRead failed\n");
        return 1;
    }
    if (CdReadSync(1, NULL) != 0) {
        fprintf(stderr, "CdReadSync did not report complete\n");
        return 1;
    }

    FILE *ref = fopen(argv[2], "rb");
    if (!ref) {
        fprintf(stderr, "could not open reference file\n");
        return 1;
    }
    unsigned char refbuf[660];
    size_t n = fread(refbuf, 1, sizeof(refbuf), ref);
    fclose(ref);

    if (n != 660) {
        fprintf(stderr, "reference file wrong size: %zu\n", n);
        return 1;
    }
    if (memcmp(buf, refbuf, 660) != 0) {
        fprintf(stderr, "MISMATCH: CdRead's sector 1 (first 660 bytes) does not match the reference file\n");
        return 1;
    }

    printf("MATCH: %d bytes read via CdControl(CdlSetloc)+CdRead are byte-identical "
           "to the independently-extracted SIBAI1_1.DAT\n", 660);
    return 0;
}
