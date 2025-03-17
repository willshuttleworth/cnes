#include <stdio.h>
#include <SDL2/SDL.h>

#include "controller.h"

static Controller controller;
void controller_setup(unsigned char *state) {
    controller.state = state;
}

void set_strobe(unsigned char bit) {
    controller.index = 0;
    controller.strobe = bit;
}

unsigned char controller_read() {
    if(controller.strobe) {
        return controller.state[0];
    }
    else if(controller.index == 8) {
        return 1;
    }
    controller.index += 1;
    return controller.state[controller.index - 1];
}

// returns -1 on SDL_QUIT input, 0 otherwise
int handle_input(SDL_Event *event) {
    switch (event->type) {
        case SDL_QUIT:
            return -1;

        case SDL_KEYDOWN:
            switch (event->key.keysym.sym) {
                case SDLK_ESCAPE:
                    return -1;
                case SDLK_w:
                    controller.state[4] = 1;
                    break;
                case SDLK_a:
                    controller.state[6] = 1;
                    break;
                case SDLK_s:
                    controller.state[5] = 1;
                    break;
                case SDLK_d:
                    controller.state[7] = 1;
                    break;
                case SDLK_j:
                    controller.state[0] = 1;
                    break;
                case SDLK_k:
                    controller.state[1] = 1;
                    break;
                case SDLK_SPACE:
                    controller.state[2] = 1;
                    break;
                case SDLK_RETURN:
                    controller.state[3] = 1;
                    break;
            }
            break;
        case SDL_KEYUP:
            switch (event->key.keysym.sym) {
                case SDLK_w:
                    controller.state[4] = 0;
                    break;
                case SDLK_a:
                    controller.state[6] = 0;
                    break;
                case SDLK_s:
                    controller.state[5] = 0;
                    break;
                case SDLK_d:
                    controller.state[7] = 0;
                    break;
                case SDLK_j:
                    controller.state[0] = 0;
                    break;
                case SDLK_k:
                    controller.state[1] = 0;
                    break;
                case SDLK_SPACE:
                    controller.state[2] = 0;
                    break;
                case SDLK_RETURN:
                    controller.state[3] = 0;
                    break;
            }
            break;
    }
    return 0;
}
