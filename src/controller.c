#include <stdio.h>
#include <SDL2/SDL.h>

typedef struct Controller {
    unsigned char *mem;
    // array of size 8, one element per button
    unsigned char *state;
} Controller;

Controller controller = {
    .mem = NULL,
    .state = NULL,
};

// joypad state
// A -> B -> Select -> Start -> Up -> Down -> Left -> Right
// all buttons can be 1 (pressed) or 0 unpressed
// index: next bit to be read (button state)
// strobe bit: lowest order bit in 0x4016
//  - 1: reset index to 0, return A's state
//  - 0: return state of index button, increment index
//  read operation: read strobe bit and index, update index and return button val

void controller_setup(unsigned char *cpu_ram, unsigned char *state) {
    controller.mem = cpu_ram;
    controller.state = state;
}

//instead of printing, update memory accordingly
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
                    printf("up pressed\n");
                    break;
                case SDLK_a:
                    printf("left pressed\n");
                    break;
                case SDLK_s:
                    printf("down pressed\n");
                    break;
                case SDLK_d:
                    printf("right pressed\n");
                    break;
                case SDLK_j:
                    printf("A pressed\n");
                    break;
                case SDLK_k:
                    printf("B pressed\n");
                    break;
                case SDLK_SPACE:
                    printf("select pressed\n");
                    break;
                case SDLK_RETURN:
                    printf("start pressed\n");
                    break;
                default:
                    printf("other input event, ignoring\n");
                    break;
            }
            break;
        case SDL_KEYUP:
            printf("no key pressed\n");
    }
    return 0;

}
