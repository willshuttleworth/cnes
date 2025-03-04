#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "parser.h"
#include "cpu.h"
#include "controller.h"
#include "bus.h"

#define ADDRESS_SPACE 65536 //2^16
#define PRG_ROM_SIZE 16384
#define CHR_ROM_SIZE 8192
 
int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("cnes",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256 * 3, 240 * 3,
        SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
    SDL_TEXTUREACCESS_STREAMING, 256, 240);
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

        //find number of prg(16kb)/chr(8kb) rom blocks 
        char *num_blocks = parse_blocks(file);
        
        //allocate memory for cpu/ppu
        unsigned char *ram = malloc(0x0800);
        unsigned char *rom = malloc(0x8000);
        unsigned char *chrom = malloc(CHR_ROM_SIZE);
        unsigned char *vram = malloc(VRAM_SIZE);
        unsigned char *palette = malloc(PALETTE_SIZE);
        unsigned char *oam = malloc(OAM_SIZE);
        unsigned char *pixels = malloc(256 * 240 * 3);
        unsigned char controller_state[8];
        memset(controller_state, 0, 8);
        
        int nmi = 0;
        parse_instructions(file, rom, chrom, num_blocks[0], num_blocks[1]);
        //pass rom info to cpu and ppu
        ppu_setup(chrom, vram, palette, oam, &nmi, texture, renderer, pixels);
        bus_setup(ram, rom);
        cpu_setup(oam, &nmi);
        controller_setup(controller_state);
        
        while(quit != -1) {
            //operands[0] = number of operands
            //operands[1..n] = bytes
            int cycle = exec_instr();
            ppu_tick_to(cycle);

            SDL_Event event;
            if(nmi) {
                while (SDL_PollEvent(&event)) {
                    if(handle_input(&event) == -1) {
                        quit = -1;
                        break;
                    }
                }
            }
        }
        
        // TODO: clean up the cleanup (arena allocation!)
        SDL_DestroyTexture(texture);
        SDL_DestroyRenderer(renderer);
        SDL_DestroyWindow(window);
        SDL_Quit();

        free(num_blocks);
        free(ram);
        free(rom);
        free(chrom);
        free(vram);
        free(palette);
        free(oam);
        free(pixels);

        fclose(file);
    }
    puts("exiting");
    return 0;
}
