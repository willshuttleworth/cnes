#include <SDL2/SDL.h>

void controller_setup(unsigned char *state);
int handle_input(SDL_Event *event);
void set_strobe(unsigned char bit);
unsigned char controller_read();

typedef struct Controller {
    // array of size 8, one element per button
    unsigned char *state;
    // next state index to read
    unsigned char index;
    unsigned char strobe;
} Controller;

