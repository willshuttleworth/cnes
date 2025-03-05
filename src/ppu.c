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
    // pointer to next open spot in oam2
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
    .vram = 0,
    .palette = 0,
    .oam = 0,
    .curr = 0,

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

    .scanline = -1,
    .dot = 0,
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

// access palette and draw to pixel (x, y) 
void draw(int x, int y, int palette) {
    int pixel = (y * 256 + x) * 3;
    // only drawing white pixels, not background ones
    if(palette != 0) {
        ppu.pixels[pixel] = 255;
        ppu.pixels[pixel + 1] = 255;
        ppu.pixels[pixel + 2] = 255;
    }
}

// TODO: implement priority after background drawing done
//  - draw sprite if it has priority OR background is transparent
// TODO: implement 8x16 sprites
void draw_sprite(int index) {
    int x = ppu.oam[index * 4 + 3];
    int y = ppu.oam[index * 4];
    unsigned short pattern_index = ppu.oam[index * 4 + 1];
    unsigned short base = (ppu.ctrl & 0x08) << 3;
    pattern_index = base + pattern_index * 16;
    for(int i = 0; i < 8; i++) {
        for(int j = 0; j < 8; j++) {
            if(x + j > 255 || y + i > 240) {
                continue;
            }
            int hi = (ppu.chrom[pattern_index + 8 + i] << j) & 0x80;
            int lo = (ppu.chrom[pattern_index + i] << j) & 0x80;
            unsigned char palette = (hi << 1) + lo;
            fprintf(stderr, "index: %d base: %x pattern: %x palette: %d\n", index, base, pattern_index, palette);
            draw(x + j, y + i, palette);
        }
    }
}

void ppu_tick_to(int cycle) {
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
                            //draw(ppu.oam[i * 4 + 3], ppu.oam[i * 4]);
                            draw_sprite(i);
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
                    ppu.ppu_cycle = 0;
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
    // TODO: how to do dynamic delay to ensure 60fps?
    //SDL_Delay(16 - (deltaTime * 1000));
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
