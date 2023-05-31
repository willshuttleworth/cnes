#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //fstat

#include "parser.h"
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
        int num_bytes = file_stats.st_size;
        unsigned char *instructions = malloc(num_bytes - sizeof(Header));
        
        //read all bytes of rom into an array
        //should this be read into an array? (yes should be stored in memory and read in once from file)
        int i = 0;
        while(!feof(file)) {
            fread(&instructions[i], sizeof(char), 1, file); 
            i++;
        } 

        //main execution loop. cpu will pass next pc as return val of execution instruction halt will be encoded as null/negative val
        int j = 0;
        while(j < num_bytes) {
            //operands[0] = number of operands
            //operands[1..num_operands] = bytes
            unsigned char *operands = parse(instructions, j, num_bytes);
            if(operands != NULL) {
                for(i = 1; i <= operands[0]; i++) {
                    //printf("%x ", operands[i]);
                }
                //printf("\n");
                j += operands[0]; 
                exec_instr(operands);
                free(operands);
            }
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
