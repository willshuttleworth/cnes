#include <stdio.h>
#include <stdlib.h>

#include "cpu/cpu.h"
#include "ppu/ppu.h"

// .nes file header
typedef struct Header {
    char magic_num[4]; 
    char prg_rom_size; //16kb units
    char chr_rom_size; //8kb units
    char flags6;
    char flags7;
    char flags8;
    char flags9;
    char flags10;
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
        fread(&h, sizeof(Header), 1, file);
        printf("magic num: %s\n", h.magic_num);
        printf("%d %d\n", h.prg_rom_size, h.chr_rom_size);
        printf("%d\n", h.flags6);
        printf("%d\n", h.flags7);
        printf("%d\n", h.flags8);
        printf("%d\n", h.flags9);
        printf("%d\n", h.flags10);
        //checking if trainer is present
        if((h.flags6 >> 2) | 0) {
            fseek(file, 512, SEEK_CUR);
        }
        printf("%d\n", SEEK_CUR);
        //read actual code lol
        // just reading first byte
        char first;
        fread(&first, sizeof(char), 1, file);
        printf("first opcode: %x %d\n", first, first);
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
