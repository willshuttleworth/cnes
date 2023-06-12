#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //fstat

#include "parser.h"
#include "cpu.h"
#include "ppu.h"

#define ADDRESS_SPACE 65536 

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

//globals (for cpu)
unsigned char *instructions;
int len;
int cpu_cycle;

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
        len = num_bytes - sizeof(Header);
        instructions = malloc(len);
        
        //read all bytes of rom into an array
        int i = 0;
        while(!feof(file)) {
            fread(&instructions[i], sizeof(char), 1, file); 
            i++;
        } 

        //main execution loop. cpu will pass next pc as return val of execution instruction halt will be encoded as null/negative val
        //loop while cpu not halted/interrupted
        //cpu has access to instruction byte array and gets next instruction independent of main
        int main_cycle = 0;
        cpu_cycle = 0;
        int ret = 0;

        //allocate cpu's address space in heap (16bit address space so 2^16 bytes)
        unsigned char *mem = malloc(ADDRESS_SPACE);
        if(mem != NULL) {
            //pass instructions to cpu
            cpu_setup(instructions, len, mem);
        }

        while(ret != -1) {
            //operands[0] = number of operands
            //operands[1..num_operands] = bytes
            /*
            for(i = 1; i <= operands[0]; i++) {
                printf("%x ", operands[i]);
            }
            printf("\n");
            */
            if(cpu_cycle < main_cycle) {
                ret = exec_instr();
            }
            main_cycle++;
        }
     
        /*
         * clock "catches up" to cpu/ppu
         * cpu/ppu execute instruction, then wait until clock is caught back up to their cycle count before 
        for(int i = 0; i < 10; i++) {
            tick(&cycle);
        }
        */
        
        free(instructions);
        free(mem);
        fclose(file);
    }
    return 0;
}
