/* Internal glue between libspu.c and libsnd.c -- not part of any PSX API,
 * not installed under include/PsyQ/. */
#ifndef PLATFORM_PC_LIBSND_INTERNAL_H
#define PLATFORM_PC_LIBSND_INTERNAL_H

void LibSnd_StopAllVoices(void);

/* Test-only: decodes raw SPU-ADPCM (VAG) bytes to signed 16-bit PCM and
 * returns the sample count. Not used by any real backend code path --
 * exposed purely so test_libsnd.c can inspect decoder output directly. */
int LibSnd_DecodeVagForTest(const unsigned char *data, int size, short **outPcm);

#endif
