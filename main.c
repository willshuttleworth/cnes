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

//addressing modes
//char impl[] = {0x00, 0x08, 0x
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
        //checking if trainer is present
        if((h.flags6 >> 2) | 0) {
            fseek(file, 512, SEEK_CUR);
        }
        //unsigned int cycle = 0;

        //parse instruction (have addressing modes grouped so it reads enough data and sends to cpu/ppu
        while(!feof(file)) {
            /*
            char opcode = 0;
            fread(&opcode, sizeof(char), 1, file);
            printf("%x\n", opcode);
            */
            unsigned char opcode = 0;
            fread(&opcode, sizeof(char), 1, file);

            //impl
            unsigned char lshift = (unsigned char) (opcode << 4); 
            int high_nibble = 0; 
            if(lshift == 0x0) {
                if((opcode == 0x0) || (opcode == 0x40) || (opcode == 0x60)) {
                    high_nibble = 1;
                }
            }
            else if(lshift == 0xA0) {
                if(opcode >> 4 >= 0x8 && opcode >> 4 <= 0xE) {
                    high_nibble = 1;
                }
            }
            if((lshift == 0x80) || (lshift == 0x0 && high_nibble) || (lshift == 0xA0 && high_nibble)) {
                //pass opcode (no operands) to cpu
                printf("impl: %x\n", opcode);
            }
    
            //acc

        } 
     
        /*
         * clock "catches up" to cpu/ppu
         * cpu/ppu execute instruction, then wait until clock is caught back up to their cycle count before 
        for(int i = 0; i < 10; i++) {
            tick(&cycle);
        }
        */
        

        fclose(file);
    }
    else {
        printf("usage:\t./main <path/to/rom/>\n");
        exit(0);
    }
    return 0;
}
