#include <stdio.h>
#include <stdlib.h>
#include <SDL2/SDL.h>

#include "cnes.h"
#include "parser.h"
#include "cpu.h"
#include "controller.h"
#include "bus.h"

int main(int argc, char **argv) {
    SDL_Init(SDL_INIT_VIDEO);
    SDL_Window* window = SDL_CreateWindow("cnes",
        SDL_WINDOWPOS_CENTERED, SDL_WINDOWPOS_CENTERED,
        256 * 3, 240 * 3,
        SDL_WINDOW_SHOWN);
    SDL_Renderer *renderer = SDL_CreateRenderer(window, -1, SDL_RENDERER_ACCELERATED);
    SDL_Texture *texture = SDL_CreateTexture(renderer, SDL_PIXELFORMAT_RGB24,
    SDL_TEXTUREACCESS_STREAMING, 256, 240);
    
    if(argc != 2) {
        printf("usage:\t./cnes <path/to/rom/>\n");
    }
    else {
        FILE *file = fopen(argv[1], "r"); 
        if(file == NULL) {
            printf("invalid file\n");
            exit(0);
        }
        //find number of prg(16kb)/chr(8kb) rom blocks 
        char *num_blocks = parse_blocks(file);

        unsigned char ram[CPU_RAM_SIZE];
        memset(ram, 0, CPU_RAM_SIZE);
        unsigned char rom[ROM_SIZE];
        memset(rom, 0, ROM_SIZE);
        unsigned char chrom[CHR_ROM_SIZE * num_blocks[1]];
        memset(chrom, 0, CHR_ROM_SIZE * num_blocks[1]);
        unsigned char vram[VRAM_SIZE];
        memset(vram, 0, VRAM_SIZE);
        unsigned char palette[PALETTE_SIZE];
        memset(palette, 0, PALETTE_SIZE);
        unsigned char oam[OAM_SIZE];
        memset(oam, 0, OAM_SIZE);
        unsigned char pixels[256 * 240 * 3];
        memset(pixels, 0, 256 * 240 * 3);
        unsigned char controller_state[8];
        memset(controller_state, 0, 8);
        
        int nmi = 0;
        parse_instructions(file, rom, chrom, num_blocks[0], num_blocks[1]);
        ppu_setup(chrom, vram, palette, oam, &nmi, texture, renderer, pixels);
        bus_setup(ram, rom);
        cpu_setup(oam, &nmi);
        controller_setup(controller_state);
        
        int quit = 0;
        while(quit != -1) {
            int cycle = exec_instr();
            ppu_tick_to(cycle);

            SDL_Event event;
            if(nmi) {
                while (SDL_PollEvent(&event)) {
                    quit = handle_input(&event);
                }
            }
        }
        fclose(file);
    }
    SDL_DestroyTexture(texture);
    SDL_DestroyRenderer(renderer);
    SDL_DestroyWindow(window);
    SDL_Quit();
    return 0;
}
