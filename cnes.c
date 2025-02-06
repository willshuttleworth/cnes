#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "parser/parser.h"
#include "cpu/cpu.h"
#include "ppu/ppu.h"

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
        
        //allocate memory for cpu/ppu
        unsigned char *instructions = parse_instructions(file);
        unsigned char *mem = malloc(ADDRESS_SPACE);
        unsigned char *chrom = malloc(CHROM_SIZE);
        unsigned char *vram = malloc(VRAM_SIZE);
        unsigned char *palette = malloc(PALETTE_SIZE);
        unsigned char *oam = malloc(OAM_SIZE);
        
        //pass rom info to cpu and ppu
        cpu_setup(instructions, mem, num_blocks);
        ppu_setup(instructions, mem, chrom, vram, palette, oam); 
        //main execution loop. cpu will pass next pc as return val of execution instruction halt will be encoded as null/negative val
        //loop while cpu not halted/interrupted
        //cpu has access to instruction byte array and gets next instruction independent of main
        int main_cycle = 0;
        int ret = 0;

        //run 7 cycle startup instruction first (add a specific cpu function for that)
        while(quit == 0 && ret != -1) {
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
            //handle input
            while (SDL_PollEvent(&event)) {
                switch (event.type) {
                    case SDL_QUIT:
                        quit = 1;
                        break;

                    case SDL_KEYDOWN:
                        switch (event.key.keysym.sym) {
                            case SDLK_ESCAPE:
                                quit = 1;
                                break;
                            case SDLK_LEFT:
                                printf("Left arrow pressed\n");
                                break;
                            case SDLK_RIGHT:
                                printf("Right arrow pressed\n");
                                break;
                            case SDLK_z:
                                printf("Z pressed\n");
                                break;
                            case SDLK_x:
                                printf("X pressed\n");
                                break;
                        }
                        break;
                    case SDL_KEYUP:
                        printf("no key pressed\n");
                }
            }
            SDL_Delay(16);
        }
     
        /*
         * clock "catches up" to cpu/ppu
         * cpu/ppu execute instruction, then wait until clock is caught back up to their cycle count before 
        for(int i = 0; i < 10; i++) {
            tick(&cycle);
        }
        */
        
        SDL_DestroyWindow(window);
        SDL_Quit();

        free(instructions);
        free(mem);
        free(chrom);
        free(vram);
        free(palette);
        free(oam);

        fclose(file);
    }
    return 0;
}
