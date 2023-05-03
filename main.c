#include <stdio.h>
#include "cpu.h"
#include "ppu.h"

void tick() {
    cpu_tick();
    ppu_tick();
}

int main() {
    for(int i = 0; i < 10; i++) {
        tick();
    }
    tick(); 
    return 0;
}
