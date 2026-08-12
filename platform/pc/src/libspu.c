/*
 * PC backend for PsyQ/libspu.h. All of this project's call sites (in
 * src/core/audio.c) use these for one-time setup/teardown around SPU RAM and
 * IRQ management -- concepts that don't map onto an OpenAL-based backend
 * (OpenAL owns its own buffers; there's no manual SPU RAM heap to
 * allocate, and no hardware IRQ to hook). SpuSetKey(SPU_OFF, SPU_ALLCH)
 * is the one call with real, useful behavior on PC: it means "silence
 * everything," which maps directly onto stopping all active voices.
 */
#include "PsyQ/libspu.h"
#include "libsnd_internal.h"

int SpuSetTransferMode(int mode) {
    (void)mode;
    return 0;
}

int SpuSetIRQ(int on_off) {
    (void)on_off;
    return 0;
}

SpuIRQCallbackProc SpuSetIRQCallback(SpuIRQCallbackProc callback) {
    (void)callback;
    return 0; /* no real SPU IRQ on this backend to hook a callback to */
}

void SpuSetKey(int on_off, unsigned int voiceBit) {
    if (on_off == SPU_OFF && voiceBit == SPU_ALLCH) {
        LibSnd_StopAllVoices();
    }
    /* Partial voice-bitmask control isn't exercised by any current call
     * site (only the "stop everything" pattern is) -- extend if needed. */
}

int SpuMallocWithStartAddr(unsigned int addr, int size) {
    (void)addr;
    (void)size;
    return 0; /* no real SPU RAM heap to manage on this backend */
}

int SpuClearReverbWorkArea(int mode) {
    (void)mode;
    return 0;
}
