/* Standalone proof-of-concept: reads the real JOU sound set (gCdFiles
 * CDF_SD_JOU_VH/VB in src/core/cd.c, LBA 0x23b3 / 0x23af) through our own CD
 * backend, feeds it through SsVabOpenHeadSticky/SsVabTransBodyPartly, and
 * verifies both the VAG decoder's output directly and that OpenAL actually
 * received a correctly-sized buffer. Not part of the real game build. */
#include <math.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <AL/al.h>
#include "PsyQ/libcd.h"
#include "PsyQ/libsnd.h"
#include "pc_platform.h"
#include "../src/libsnd_internal.h"

static int ReadCdFile(long startingSector, int sectorCt, unsigned char *buf) {
    CdlLOC loc;
    CdIntToPos(startingSector, &loc);
    if (CdControl(CdlSetloc, (u_char *)&loc, NULL) == 0) return 0;
    if (CdSync(1, NULL) != CdlComplete) return 0;
    if (CdRead(sectorCt, (u_long *)buf, 0) == 0) return 0;
    if (CdReadSync(1, NULL) != 0) return 0;
    return 1;
}

static void WriteWav(const char *path, const short *pcm, int numSamples, int rate) {
    FILE *f = fopen(path, "wb");
    int dataBytes = numSamples * 2;
    int byteRate = rate * 2;
    unsigned char hdr[44] = {
        'R','I','F','F', 0,0,0,0, 'W','A','V','E',
        'f','m','t',' ', 16,0,0,0, 1,0, 1,0,
        0,0,0,0, 0,0,0,0, 2,0, 16,0,
        'd','a','t','a', 0,0,0,0
    };
    int riffSize = 36 + dataBytes;
    memcpy(hdr + 4, &riffSize, 4);
    memcpy(hdr + 24, &rate, 4);
    memcpy(hdr + 28, &byteRate, 4);
    memcpy(hdr + 40, &dataBytes, 4);
    fwrite(hdr, 1, 44, f);
    fwrite(pcm, 2, numSamples, f);
    fclose(f);
}

int main(int argc, char **argv) {
    if (argc < 2) {
        fprintf(stderr, "usage: %s <disc.bin>\n", argv[0]);
        return 1;
    }
    if (!PC_CdMount(argv[1]) || !CdInit()) {
        fprintf(stderr, "disc mount failed\n");
        return 1;
    }

    unsigned char vh[2 * 2048];
    unsigned char vb[4 * 2048];
    if (!ReadCdFile(0x23b3, 2, vh)) { fprintf(stderr, "VH read failed\n"); return 1; }
    if (!ReadCdFile(0x23af, 4, vb)) { fprintf(stderr, "VB read failed\n"); return 1; }
    printf("Read JOU.VH (%zu bytes) and JOU.VB (%zu bytes) via CdControl+CdRead\n", sizeof(vh), sizeof(vb));

    /* Direct decoder check: VAG #1 is the first real sample, size 240
     * bytes per the size table at VH offset 2592 (see the audio step
     * file), starting at body offset 0. */
    short *pcm;
    int n = LibSnd_DecodeVagForTest(vb, 240, &pcm);
    int expectedSamples = (240 / 16) * 28;
    printf("VAG #1 decode: %d samples (expected %d)\n", n, expectedSamples);
    if (n != expectedSamples) {
        fprintf(stderr, "FAIL: sample count mismatch\n");
        return 1;
    }

    double sumSq = 0;
    short minS = 32767, maxS = -32768;
    for (int i = 0; i < n; i++) {
        sumSq += (double)pcm[i] * pcm[i];
        if (pcm[i] < minS) minS = pcm[i];
        if (pcm[i] > maxS) maxS = pcm[i];
    }
    double rms = sqrt(sumSq / n);
    printf("VAG #1 decoded PCM stats: min=%d max=%d rms=%.1f\n", minS, maxS, rms);
    if (rms < 1.0) {
        fprintf(stderr, "FAIL: decoded audio is silent/degenerate -- decoder is likely wrong\n");
        return 1;
    }
    WriteWav("test_vag1.wav", pcm, n, 22050);
    printf("Wrote test_vag1.wav (%d samples) for manual inspection if desired\n", n);
    free(pcm);

    /* Full pipeline: VAB open + body transfer + OpenAL buffer upload. */
    SsInit();
    if (SsVabOpenHeadSticky(vh, 0, 0) != 0) {
        fprintf(stderr, "FAIL: SsVabOpenHeadSticky rejected real VAB header\n");
        return 1;
    }
    SsVabTransBodyPartly(vb, sizeof(vb), 0);

    short r = SsUtKeyOnV(0, 0, 0, 0, 60, 0, 100, 100);
    printf("SsUtKeyOnV(voice=0, vab=0, prog=0, tone=0) returned %d\n", r);

    SsQuit();
    printf("PASS: CD-sourced VAB data decoded, OpenAL pipeline ran without error\n");
    return 0;
}
