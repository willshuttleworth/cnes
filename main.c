#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //fstat

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
    if(argc != 2) {
        printf("usage:\t./main <path/to/rom/>\n");
        exit(0);
    }
    else {
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

        //seek to end and find size of rom (dont include header yeah?) and allocate array big enough
        struct stat file_stats;
        file_stats.st_uid = 0;
        file_stats.st_gid = 0;
        file_stats.st_size = 0;
        file_stats.st_mode = 0;

        fstat(fileno(file), &file_stats); //better to use c library wrappers over syscalls? idk
        unsigned char *instructions = malloc(file_stats.st_size - sizeof(Header));
        
        //read all bytes of rom into an array
        //should this be read into an array? (yes should be stored in memory and read in once from file)
        int i = 0;
        while(!feof(file)) {
            fread(&instructions[i], sizeof(char), 1, file); 
            i++;
        } 

        printf("%d\n", (0x0A >> 4) % 2);
        //main execution loop. cpu will pass next pc as return val of execution instruction halt will be encoded as null/negative val
        int j = 0;
        while(j < file_stats.st_size) {
            unsigned char opcode = instructions[j];

            //impl
            unsigned char lshift = (unsigned char) (opcode << 4); 
            int high_nibble = 0; 
            if(lshift == 0x0) {
                if((opcode == 0x0) || (opcode == 0x40) || (opcode == 0x60)) {
                    high_nibble = 1;
                }
            }
            else if(lshift == 0xA0) {
                if((opcode >> 4 >= 0x7 && opcode >> 4 <= 0xF) || ((opcode >> 4) % 2 != 0)) {
                    high_nibble = 1;
                }
            }
            if((lshift == 0x80) || (lshift == 0x0 && high_nibble) || (lshift == 0xA0 && high_nibble)) {
                //pass opcode (no operands) to cpu
                printf("impl: %x\n", opcode);
            }
             
            //acc (lo nibble is a and hi is 0,2,4,6)
            if(lshift == 0xA0 && (opcode >> 4 < 0x7) && ((opcode >> 4) % 2 == 0)) {
                //pass opcode (no operands) to cpu
                printf("acc: %x\n", opcode);
            }
            j++;
        }
     
        /*
         * clock "catches up" to cpu/ppu
         * cpu/ppu execute instruction, then wait until clock is caught back up to their cycle count before 
        for(int i = 0; i < 10; i++) {
            tick(&cycle);
        }
        */
        
        free(instructions);
        fclose(file);
    }
    return 0;
}
