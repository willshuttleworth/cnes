#include <stdio.h>
#include "cpu/cpu.h"
#include "ppu/ppu.h"

void tick(unsigned int *cycle) {
    cycle++;
    cpu_tick_to(*cycle);
    ppu_tick_to(*cycle);
}

int main(int argc, char **argv) {
    //read rom from cmdline arg
    printf("%d\n", argc);
    unsigned int cycle = 0;
    for(int i = 0; i < 10; i++) {
        tick(&cycle);
    }
    return 0;
}
