/* Standalone proof-of-concept: exercises the PC libetc.h backend directly, without pulling in the
 * rest of the game, to check the implementation's runtime behavior in isolation. Not part of the
 * game build. */
#include <stdio.h>
#include <SDL2/SDL.h>
#include "PsyQ/libetc.h"

int main(void) {
    if (SDL_Init(SDL_INIT_VIDEO) != 0) {
        fprintf(stderr, "SDL_Init failed: %s\n", SDL_GetError());
        return 1;
    }
    SDL_Window *win = SDL_CreateWindow("libetc PC backend test", SDL_WINDOWPOS_CENTERED,
                                        SDL_WINDOWPOS_CENTERED, 320, 240, 0);

    PadInit(0);

    Uint32 start = SDL_GetTicks();
    int frame = 0;
    for (frame = 0; frame < 120; frame++) {
        u_long pad = PadRead(0);
        VSync(0);
        if (frame % 30 == 0) {
            printf("frame %3d  t=%ums  pad=0x%08lx\n", frame, SDL_GetTicks() - start, pad);
        }
    }
    Uint32 elapsed = SDL_GetTicks() - start;
    printf("120 VSync-paced frames took %ums (expected ~2000ms at 60Hz)\n", elapsed);

    SDL_DestroyWindow(win);
    SDL_Quit();
    return 0;
}
