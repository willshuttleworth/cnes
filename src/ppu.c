#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>

static const unsigned char colors[64][3] = {
    {0x80, 0x80, 0x80}, {0x00, 0x3D, 0xA6}, {0x00, 0x12, 0xB0}, {0x44, 0x00, 0x96},
    {0xA1, 0x00, 0x5E}, {0xC7, 0x00, 0x28}, {0xBA, 0x06, 0x00}, {0x8C, 0x17, 0x00},
    {0x5C, 0x2F, 0x00}, {0x10, 0x45, 0x00}, {0x05, 0x4A, 0x00}, {0x00, 0x47, 0x2E},
    {0x00, 0x41, 0x66}, {0x00, 0x00, 0x00}, {0x05, 0x05, 0x05}, {0x05, 0x05, 0x05},

    {0xC7, 0xC7, 0xC7}, {0x00, 0x77, 0xFF}, {0x21, 0x55, 0xFF}, {0x82, 0x37, 0xFA},
    {0xEB, 0x2F, 0xB5}, {0xFF, 0x29, 0x50}, {0xFF, 0x22, 0x00}, {0xD6, 0x32, 0x00},
    {0xC4, 0x62, 0x00}, {0x35, 0x80, 0x00}, {0x05, 0x8F, 0x00}, {0x00, 0x8A, 0x55},
    {0x00, 0x99, 0xCC}, {0x21, 0x21, 0x21}, {0x09, 0x09, 0x09}, {0x09, 0x09, 0x09},

    {0xFF, 0xFF, 0xFF}, {0x0F, 0xD7, 0xFF}, {0x69, 0xA2, 0xFF}, {0xD4, 0x80, 0xFF},
    {0xFF, 0x45, 0xF3}, {0xFF, 0x61, 0x8B}, {0xFF, 0x88, 0x33}, {0xFF, 0x9C, 0x12},
    {0xFA, 0xBC, 0x20}, {0x9F, 0xE3, 0x0E}, {0x2B, 0xF0, 0x35}, {0x0C, 0xF0, 0xA4},
    {0x05, 0xFB, 0xFF}, {0x5E, 0x5E, 0x5E}, {0x0D, 0x0D, 0x0D}, {0x0D, 0x0D, 0x0D},

    {0xFF, 0xFF, 0xFF}, {0xA6, 0xFC, 0xFF}, {0xB3, 0xEC, 0xFF}, {0xDA, 0xAB, 0xEB},
    {0xFF, 0xA8, 0xF9}, {0xFF, 0xAB, 0xB3}, {0xFF, 0xD2, 0xB0}, {0xFF, 0xEF, 0xA6},
    {0xFF, 0xF7, 0x9C}, {0xD7, 0xE8, 0x95}, {0xA6, 0xED, 0xAF}, {0xA2, 0xF2, 0xDA},
    {0x99, 0xFF, 0xFC}, {0xDD, 0xDD, 0xDD}, {0x11, 0x11, 0x11}, {0x11, 0x11, 0x11}
};

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

#ifdef SHOWFPS
    #define START_TIMER() startFrameTimer()
    #define END_TIMER() endFrameTimer()
#else
    #define START_TIMER()
    #define END_TIMER()
#endif

// access palette and draw to pixel (x, y) 
void draw(int x, int y, int palette, int color, int sprite) {
    int pixel = (y * 256 + x) * 3;
    unsigned char palette_addr = 0;
    if(sprite) {
        palette_addr = 0x10 | ((palette << 2) | color);
    }
    else {
        palette_addr = palette * 4 + color;
    }
    unsigned char sys_color = ppu.palette[palette_addr];
    // only drawing white pixels, not background ones
    if(color != 0) {
        ppu.pixels[pixel] = colors[sys_color][0];
        ppu.pixels[pixel + 1] = colors[sys_color][1];
        ppu.pixels[pixel + 2] = colors[sys_color][2];
    }
}

// TODO: implement priority after background drawing done
//  - draw sprite if it has priority OR background is transparent
// TODO: implement 8x16 sprites
// TODO: get rid of i/rev_i nonsense
void draw_sprite(int index) {
    int x = ppu.oam[index * 4 + 3];
    int y = ppu.oam[index * 4];
    // TODO: get rid of double shifts, just shift left at first prob
    int flip_x = ((ppu.oam[index * 4 + 2] << 1) & 0x80) >> 7;
    int flip_y = (ppu.oam[index * 4 + 2] & 0x80) >> 7;
    unsigned short pattern_index = ppu.oam[index * 4 + 1];
    unsigned short base = (ppu.ctrl & 0x08) << 9;
    pattern_index = base + pattern_index * 16;

    unsigned char palette = ppu.oam[index * 4 + 2] & 0x03;

    int i, j, rev_i, rev_j; 
    if(flip_x) { j = 7; rev_j = 0; }
    else       { rev_j = 7; j = 0; }
    if(flip_y) { i = 7; rev_i = 0; }
    else       { rev_i = 7; i = 0; }

    while(i < 8 && i > -1) {
        while(j < 8 && j > -1) {
            if(x + j > 255 || y + i > 240) {
                if(flip_x) { j--; rev_j++; }
                else       { j++; rev_j--; }
                continue;
            }
            // TODO: get rid of double shifts, just shift left at first prob
            int hi = ((ppu.chrom[pattern_index + 8 + i] << j) & 0x80) >> 7;
            int lo = ((ppu.chrom[pattern_index + i] << j) & 0x80) >> 7;
            unsigned char color = (hi << 1) + lo;
            int draw_x = x + j;
            int draw_y = y + i;
            if(flip_x) draw_x = x + rev_j;
            if(flip_y) draw_y = y + rev_i;

            draw(draw_x, draw_y, palette, color, 1);

            if(flip_x) { j--; rev_j++; }
            else       { j++; rev_j--; }
        }
        if(flip_x) { j = 7; rev_j = 0; }
        else       { j = 0; rev_j = 7; }
        if(flip_y) { i--; rev_i++; }
        else       { i++; rev_i--; }
    }
}

void ppu_tick_to(int cycle) {
    while(ppu.scanline <= 260) {
        // read oam, find sprites on this scanline
        ppu.curr = 0;
        while(ppu.dot <= 340) {
            if(ppu.ppu_cycle >= (cycle * 3)) {
                return;
            }
            if(ppu.dot == 0 && ppu.scanline < 240) {
                if(ppu.scanline == -1) {
                    START_TIMER();
                    ppu.ctrl &= 0xDF;
                    
                    // TODO: this is hardcoded to only use first nametable
                    // each one of these bytes indexes pattern table
                    // within this loop, loop over all 64 pixels in the 8x8 section, using draw() to set color of each in pixels array
                    for(int i = 0; i < 0x03C0; i++) {
                        unsigned short pattern_index = ppu.vram[i];
                        unsigned short base = (ppu.ctrl & 0x10) << 8;
                        pattern_index = base + pattern_index * 16;
                        int palette = 0;
                        int block_row = (i / 128);
                        int block_col = i % 32 / 4;
                        int block = block_row * 8 + block_col;
                        /// TODO: hardcoded to first nametable
                        unsigned char block_data = ppu.vram[0x03C0 + block];
                        int row = i / 32;
                        int col = i % 32;
                        // tl
                        if((row % 4 == 0 || row % 4 == 1) && (col % 4 == 0 || col % 4 == 1)) {
                            palette = block_data & 0x3;
                        }
                        // tr
                        else if((row % 4 == 0 || row % 4 == 1) && (col % 4 == 2 || col % 4 == 3)) {
                            palette = (block_data >> 2) & 0x3;
                        }
                        // bl
                        else if((row % 4 == 2 || row % 4 == 3) && (col % 4 == 0 || col % 4 == 1)) {
                            palette = (block_data >> 4) & 0x3;
                        }
                        // br
                        else {
                            palette = (block_data >> 6) & 0x3;
                        }

                        for(int j = 0; j < 8; j++) {
                            for(int k = 0; k < 8; k++) {
                                int hi = ((ppu.chrom[pattern_index + 8 + j] << k) & 0x80) >> 7;
                                int lo = ((ppu.chrom[pattern_index + j] << k) & 0x80) >> 7;
                                unsigned char color = (hi << 1) + lo;
                                int x = (i % 32) * 8;
                                int y = (i / 32) * 8;
                                draw(x + k, y + j, palette, color, 0);
                            }
                        }
                    }
                }
                for(int i = 0; i < 64; i++) {
                    if(ppu.oam[i * 4] - 1 == ppu.scanline && (ppu.oam[i * 4 + 2] & 0x20) == 0) {
                        // check if sprite rendering enabled
                        if(ppu.mask & 0x10 && ppu.curr < 8) {
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
    END_TIMER();
    // TODO: how to do dynamic delay to ensure 60fps?
    //SDL_Delay(15);
}

unsigned char ppu_read(unsigned short addr) {
    // chrom
    if(addr < 0x2000) {
        return ppu.chrom[addr];
    }
    // vram
    else if(addr < 0x3000) {
        addr %= 0x2000;
        return ppu.vram[addr % 0x0800];
    }
    // palette
    else if(addr >= 0x3F00) {
        addr = addr % 0x3F00;
        addr = addr % 0x20;
        return ppu.palette[addr];
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
        addr %= 0x2000;
        ppu.vram[addr % 0x0800] = data;
    }
    // palette
    else if(addr >= 0x3F00) {
        addr = addr % 0x3F00;
        addr = addr % 0x20;
        ppu.palette[addr] = data;
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
    char inc = (ppu.ctrl >> 2) & 1;
    if(inc == 0) {
        ppu.t = (ppu.t + 1) % 0x3FFF;      
    }
    else {
        ppu.t = (ppu.t + 32) % 0x3FFF;      
    }
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
