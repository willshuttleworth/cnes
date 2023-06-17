#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //fstat

#include "parser/parser.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

#define ADDRESS_SPACE 65536 //2^16


//globals (for cpu)
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

        //find number of 16kib blocks
        int num_blocks = parse_blocks(file);
        //read each instruction into an array
        unsigned char *instructions = parse_instructions(file);
        unsigned char *mem = malloc(ADDRESS_SPACE);
        //pass rom info to cpu
        cpu_setup(instructions, mem, num_blocks);

        //main execution loop. cpu will pass next pc as return val of execution instruction halt will be encoded as null/negative val
        //loop while cpu not halted/interrupted
        //cpu has access to instruction byte array and gets next instruction independent of main
        int main_cycle = 0;
        cpu_cycle = 0;
        int ret = 0;

        //run 7 cycle startup instruction first (add a specific cpu function for that)
        while(ret != -1) {
            //operands[0] = number of operands
            //operands[1..num_operands] = bytes
            /*
            for(i = 1; i <= operands[0]; i++) {
                printf("%x ", operands[i]);
            }
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
