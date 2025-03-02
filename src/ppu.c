#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>

typedef struct PPU {
    //sprites as bitmap images (pattern tables)
    unsigned char *chrom;
    //layout (nametables)
    unsigned char *vram;
    //colors
    unsigned char *palette;
    //sprite positioning (not part of ppu mmap, accessed independently)
    unsigned char *oam;
    int curr;

    //registers
    unsigned char ctrl;
    unsigned char mask;
    unsigned char status;
    unsigned char oamaddr;
    unsigned char oamdata;
    unsigned char scroll;
    unsigned char addr;
    unsigned char data;
    unsigned short v;
    unsigned short t;
    unsigned char w;

    unsigned char read_buffer;

    int ppu_cycle;
    int *nmi;

    // -1 to 260
    int scanline;
    // 1 to 340
    int dot;

    //sdl 
    SDL_Texture *texture;
    SDL_Renderer *renderer;
    unsigned char *pixels;
} PPU;

PPU ppu = {
    .chrom = 0,
    //layout (nametables)
    .vram = 0,
    //colors
    .palette = 0,
    //sprite positioning (not part of ppu mmap, accessed independently)
    .oam = 0,
    // sprite data for next scanline
    // pointer to next open spot in oam2
    .curr = 0,

    //registers
    .ctrl = 0,
    .mask = 0,
    .status = 0,
    .oamaddr = 0,
    .oamdata = 0,
    .scroll = 0,
    .addr = 0,
    .data = 0,
    .v = 0,
    .t = 0,
    .w = 0,

    .read_buffer = 0,
    .ppu_cycle = 0,
    .nmi = NULL,

    // rendering metadata
    .scanline = -1,
    .dot = 0,
    //sdl 
    .texture = NULL,
    .renderer = NULL,
    .pixels = NULL,
};

void ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam, int *nmi, SDL_Texture *texture, SDL_Renderer *renderer, unsigned char *pixels) {
    ppu.chrom = chrom;
    ppu.vram = vram;
    ppu.palette = palette;
    ppu.oam = oam;
    ppu.nmi = nmi;
    // vblank
    ppu.status = 0x80;
    *ppu.nmi = 0;
    //sdl 
    ppu.texture = texture;
    ppu.renderer = renderer;
    ppu.pixels = pixels;
}

void draw(unsigned char x, unsigned char y) {
    int pixel = (y * 256 + x) * 3;
    ppu.pixels[pixel] = 255;
    ppu.pixels[pixel + 1] = 255;
    ppu.pixels[pixel + 2] = 255;
}

Uint64 lastTime = 0;
double deltaTime = 0;
double fps = 0;

void startFrameTimer() {
    lastTime = SDL_GetPerformanceCounter();
}

void endFrameTimer() {
    Uint64 currentTime = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();

    deltaTime = (double)(currentTime - lastTime) / freq;  
    fps = 1.0 / deltaTime;

    fprintf(stderr, "frame time: %.3f ms, FPS: %.2f\n", deltaTime * 1000, fps);
}

void ppu_tick_to(unsigned long long cycle) {
    while(ppu.scanline <= 260) {
        // read oam, find sprites on this scanline
        ppu.curr = 0;
        while(ppu.dot <= 340) {
            if(ppu.ppu_cycle > (cycle * 3)) {
                return;
            }
            if(ppu.dot == 0 && ppu.scanline < 240) {
                if(ppu.scanline == -1) {
                    startFrameTimer();
                    ppu.ctrl &= 0xDF;
                }
                for(int i = 0; i < 64; i++) {
                    if(ppu.oam[i * 4] - 1 == ppu.scanline && (ppu.oam[i * 4 + 2] & 0x20) == 0) {
                        // check if sprite rendering enabled
                        if(ppu.mask & 0x10 && ppu.curr < 8) {
                            draw(ppu.oam[i * 4 + 3], ppu.oam[i * 4]);
                            ppu.curr++;
                        }
                        else if(ppu.curr == 8) {
                            ppu.ctrl |= 0x20;
                        }
                    }
                }
            }
            if(ppu.scanline == 241 && ppu.dot == 1) {
                if(ppu.ctrl >> 7) {
                    *ppu.nmi = 1;
                }
                ppu.status |= 0x80;
                SDL_UpdateTexture(ppu.texture, NULL, ppu.pixels, 256 * 3);
                SDL_RenderCopy(ppu.renderer, ppu.texture, NULL, NULL);
                SDL_RenderPresent(ppu.renderer);
                memset(ppu.pixels, 0, 256 * 240 * 3);
            }
            if(ppu.scanline == -1 && ppu.dot == 0) {
                ppu.status &= 0x1F;
            }
            ppu.ppu_cycle++;
            ppu.dot++;
        } 
        ppu.dot = 0;
        ppu.scanline++;
    }
    ppu.scanline = -1;
    endFrameTimer();
    //SDL_Delay(30);
}

        unsigned char ppu_read(unsigned short addr) {
            addr %= 0x4000; 

            // chrom
            if(addr < 0x2000) {
                return ppu.chrom[addr];
            }
            // vram
            else if(addr < 0x3000) {
                return ppu.vram[addr % 0x3000];
            }
            // palette
            else {
                return ppu.palette[addr % 0x3F00];
            }
            return 0;
        }

        void ppu_write(unsigned short addr, unsigned char data) {
            // chrom
            if(addr < 0x2000) {
                // no op
                return;
            }
            // vram
            else if(addr < 0x3000) {
                ppu.vram[addr % 0x2000] = data;
            }
            // palette
            else {
                ppu.palette[addr % 0x3F00] = data;
            }
        }

        void addr_write(unsigned char addr) {
            // hi byte
            if(ppu.w == 0) {
                ppu.addr = addr;
                ppu.t = addr;
                ppu.t <<= 8;
                ppu.w = 1;
            }
            // lo byte
            else {
                ppu.addr = addr;
                ppu.t |= addr;
                ppu.w = 0;
            }
            //mirroring 
            if(ppu.t > 0x3FFF) {
                ppu.t &= 0x3FFF;
            }
        }

        unsigned char data_read() {
            unsigned char new = ppu_read(ppu.t);     
            unsigned char ret = 0;

            // data is from palette, return it immediately
            if(ppu.t >= 0x3F00 && ppu.t <= 0x3FFF) {
                // how should read buffer be updated/cleared after read to palette?
                ppu.read_buffer = new;
                ret = new;
            }
            else {
                //swap new and buffer, return former buffer value
                unsigned char swap = ppu.read_buffer;
                ppu.read_buffer = new;
                ret = swap;
            }

            // increment read address 
            char inc = (ppu.ctrl >> 2) & 1;
            if(inc == 0) {
                ppu.t = (ppu.t + 1) % 0x3FFF;      
            }
            else {
                ppu.t = (ppu.t + 32) % 0x3FFF;      
            }
            return ret;
        }

        void data_write(unsigned char data) {
            ppu_write(ppu.t, data);
        }

        void ctrl_write(unsigned char data) {
            if(ppu.status >> 7 && data >> 7 && (ppu.ctrl >> 7) == 0) {
                printf("vblank special case: scanline %d dot %d\n", ppu.scanline, ppu.dot);
                *ppu.nmi = 1;
            }
            ppu.ctrl = data;
        }

        void mask_write(unsigned char data) {
            ppu.mask = data;    
        }

        unsigned char status_read() {
            ppu.w = 0;
            unsigned char ret = ppu.status;
            ppu.status &= 0x1F;
            return ret;
        }

        void oamaddr_write(unsigned char data) {
            ppu.oamaddr = data;
        }

        unsigned char oamdata_read() {
            return ppu.oam[ppu.oamaddr];
        }

        void oamdata_write(unsigned char data) {
            ppu.oam[ppu.oamaddr] = data;
        }
