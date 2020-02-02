#include "input.h"

#include <string.h>
#include <SDL2/SDL.h>

// Array with the keyboard state
Key keyboard_current[NUMKEYS] = {};
Key keyboard_last[NUMKEYS] = {};
SDL_Event event;

int GetKey(Key k) {
  return keyboard_current[k];
}

int GetKeyDown(Key k) {
  return (keyboard_current[k] && !keyboard_last[k]);
}

int GetKeyUp(Key k) {
  return (!keyboard_current[k] && keyboard_last[k]);
}

void InputUpdate() {
    // Updates keyboard array and manages events
    memcpy(keyboard_last, keyboard_current, NUMKEYS*sizeof(Key));

    SDL_Event event;
    while (SDL_PollEvent(&event)) {
        switch (event.type) {
            case SDL_KEYDOWN:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: keyboard_current[KEY_RIGHT] = 1; break;
                    case SDLK_LEFT: keyboard_current[KEY_LEFT] = 1; break;
                    case SDLK_UP: keyboard_current[KEY_UP] = 1; break;
                    case SDLK_DOWN: keyboard_current[KEY_DOWN] = 1; break;

                    case SDLK_w: keyboard_current[KEY_UP] = 1; break;
                    case SDLK_a: keyboard_current[KEY_LEFT] = 1; break;
                    case SDLK_s: keyboard_current[KEY_DOWN] = 1; break;
                    case SDLK_d: keyboard_current[KEY_RIGHT] = 1; break;

                    case SDLK_b: keyboard_current[KEY_TOGGLE_BLOOM] = 1; break;

                    case SDLK_RETURN: keyboard_current[KEY_ENTER] = 1; break;
                    case SDLK_ESCAPE: keyboard_current[KEY_BACK] = 1; break;
                }
            break;
            case SDL_KEYUP:
                switch (event.key.keysym.sym) {
                    case SDLK_RIGHT: keyboard_current[KEY_RIGHT] = 0; break;
                    case SDLK_LEFT: keyboard_current[KEY_LEFT] = 0; break;
                    case SDLK_UP: keyboard_current[KEY_UP] = 0; break;
                    case SDLK_DOWN: keyboard_current[KEY_DOWN] = 0; break;

                    case SDLK_w: keyboard_current[KEY_UP] = 0; break;
                    case SDLK_a: keyboard_current[KEY_LEFT] = 0; break;
                    case SDLK_s: keyboard_current[KEY_DOWN] = 0; break;
                    case SDLK_d: keyboard_current[KEY_RIGHT] = 0; break;

                    case SDLK_b: keyboard_current[KEY_TOGGLE_BLOOM] = 0; break;

                    case SDLK_RETURN: keyboard_current[KEY_ENTER] = 0; break;
                    case SDLK_ESCAPE: keyboard_current[KEY_BACK] = 0; break;
                }
            break;
        }
    }
}
