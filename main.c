#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "ppu/ppu.h"

// .nes file header
typedef struct Header {
    char magic_num[4]; 
    char prg_rom_size; //16kb units
    char chr_rom_size; //8kb units
    char flags[5];
    char padding[5];
}Header;

void tick(unsigned int *cycle) {
    cycle++;
    cpu_tick_to(*cycle);
    ppu_tick_to(*cycle);
}

int main(int argc, char **argv) {
    //read rom from cmdline arg
    if(argc == 2) {
        FILE *file = fopen(argv[1], "r"); 
        if(file == NULL) {
            printf("invalid file\n");
            exit(0);
        }
        //read each instruction into an array
        Header h;
        fread(&h, sizeof(h), 1, file);
        //read actual code lol
        fclose(file);
    }
    else {
        printf("usage:\t./main <path/to/rom/>\n");
        exit(0);
    }
    unsigned int cycle = 0;

    /*
    for(int i = 0; i < 10; i++) {
        tick(&cycle);
    }
    */
    
    return 0;
}
