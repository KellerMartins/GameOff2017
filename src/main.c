#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <time.h>

#include <SDL2/SDL.h>
#include <SDL2/SDL_image.h>
#include <SDL_FontCache.h>

#include "renderer.h"
#include "utils.h"
#include "game_logic.h"
#include "game_state.h"

int InitProgram();
void FreeAllocations();

int main(int argc, char *argv[]) {
    // Initializes SDL, SDL_IMAGE, basic textures and general stuff
    if (InitProgram()) {
        #ifdef EMSCRIPTEN
            NextState();
        #else
            while (GetState() != STATE_EXIT) {
                NextState();
            }
        #endif
    }

    #ifndef EMSCRIPTEN
    FreeAllocations();
    #endif
    
    return 0;
}

int InitProgram() {
    // General initializations

    // Random from start time
    srand((unsigned)time(NULL));

    // Initializing SDL
    if (SDL_Init(SDL_INIT_VIDEO) < 0) {
        printf("SDL could not initialize! SLD_Error: %s\n", SDL_GetError());
        return 0;
    }
    // Initializing SDL_Image
    if ( !(IMG_Init(IMG_INIT_PNG) & IMG_INIT_PNG) ) {
        printf("SDL_image could not initialize! SDL_image Error: %s\n", IMG_GetError() );
        return 0;
    }

    InitRenderer();
    InitFPS();
    
    return 1;
}

void FreeAllocations() {
    FreeRenderer();

    if (SDL_WasInit(SDL_INIT_EVERYTHING) != 0)
        SDL_Quit();
}
