#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "parser.h"
#include "cpu.h"
#include "ppu.h"
#include "controller.h"
#include "bus.h"

#define ADDRESS_SPACE 65536 //2^16

//globals (for cpu)
int cpu_cycle;

int main(int argc, char **argv) {
    //sdl setup
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Event event;
    SDL_Window* window = SDL_CreateWindow("Keyboard Input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        640, 480, 0);

    int quit = 0;

    //read rom from cmdline arg
    if(argc != 2) {
        printf("usage:\t./cnes <path/to/rom/>\n");
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
        
        //allocate memory for cpu/ppu
        unsigned char *instructions = parse_instructions(file);
        //unsigned char *mem = malloc(ADDRESS_SPACE);

        unsigned char *ram = malloc(0x0800);
        unsigned char *rom = malloc(0x8000);

        unsigned char *chrom = malloc(CHROM_SIZE);
        unsigned char *vram = malloc(VRAM_SIZE);
        unsigned char *palette = malloc(PALETTE_SIZE);
        unsigned char *oam = malloc(OAM_SIZE);
        
        //pass rom info to cpu and ppu
        bus_setup(ram, rom, instructions, num_blocks);
        cpu_setup();
        //ppu_setup(instructions, mem, chrom, vram, palette, oam); 
        //controller_setup(mem);


        int main_cycle = 0;
        int ret = 0;

        while(quit == 0 && ret != -1) {
            //operands[0] = number of operands
            //operands[1..num_operands] = bytes
            if(cpu_cycle < main_cycle) {
                ret = exec_instr();
            }
            main_cycle++;

            //handle input
            while (SDL_PollEvent(&event)) {
                if(handle_input(&event) == -1) {
                    quit = -1;
                    break;
                }
            }
            //SDL_Delay(16);
        }
     
        /*
         * clock "catches up" to cpu/ppu
         * cpu/ppu execute instruction, then wait until 
         * clock is caught back up to their cycle count before 
         */
        
        SDL_DestroyWindow(window);
        SDL_Quit();

        free(instructions);
        //free(mem);
        free(chrom);
        free(vram);
        free(palette);
        free(oam);

        free(ram);
        free(rom);

        fclose(file);
    }
    return 0;
}
