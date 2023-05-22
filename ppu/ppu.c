#include <stdio.h>
#include "../main.c"

typedef struct PPU {
    //cycle that ppu is on. increase until it matches overall clock
    unsigned int cycle;
    
    //whatever the hell else ppu needs
}PPU;

PPU ppu = { .cycle = 0 };

void ppu_tick_to(unsigned int cycle) {
    printf("stepping up to cycle number %d from cycle number %d\n", cycle, ppu.cycle);
    while(ppu.cycle < cycle) {
        //do something that takes one cycle? or more cycles but still within max count?
        ppu.cycle++;
    }
}
