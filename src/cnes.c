#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "parser.h"
#include "cpu.h"
#include "controller.h"
#include "bus.h"

#define ADDRESS_SPACE 65536 //2^16
#define PRG_ROM_SIZE 16384

//globals (for cpu)
int cpu_cycle;

int main(int argc, char **argv) {
    //sdl setup
    /*
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Event event;
    SDL_Window* window = SDL_CreateWindow("Keyboard Input",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256, 240, 0);

    int quit = 0;
    */

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

        //find number of prg(16kb)/chr(8kb) rom blocks 
        char *num_blocks = parse_blocks(file);
        
        //allocate memory for cpu/ppu
        unsigned char *ram = malloc(0x0800);
        unsigned char *rom = malloc(0x8000);
        unsigned char *chrom = malloc(CHROM_SIZE);
        unsigned char *vram = malloc(VRAM_SIZE);
        unsigned char *palette = malloc(PALETTE_SIZE);
        unsigned char *oam = malloc(OAM_SIZE);
        
        parse_instructions(file, rom, chrom, num_blocks[0], num_blocks[1]);
        //pass rom info to cpu and ppu
        ppu_setup(chrom, vram, palette, oam);
        bus_setup(ram, rom);
        cpu_setup();
        //controller_setup(mem);

        bus_write(0x200E, 0x40);
        bus_write(0x200E, 0x00);

        int main_cycle = 0;
        int ret = 0;
        
        int test = 1;

        //while(quit == 0 && ret != -1) {
        while(ret != -1 && test == 0) {
            //operands[0] = number of operands
            //operands[1..n] = bytes
            if(cpu_cycle < main_cycle) {
                ret = exec_instr();
            }
            main_cycle++;

            //handle input
            /*
            while (SDL_PollEvent(&event)) {
                if(handle_input(&event) == -1) {
                    quit = -1;
                    break;
                }
            }
            SDL_Delay(16);
            */
        }
        
        // TODO: clean up the cleanup
        /*
        SDL_DestroyWindow(window);
        SDL_Quit();
        */

        free(num_blocks);
        free(ram);
        free(rom);
        free(chrom);
        free(vram);
        free(palette);
        free(oam);

        fclose(file);
    }
    return 0;
}
