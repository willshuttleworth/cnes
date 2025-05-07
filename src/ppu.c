#include <stdio.h>
#include <time.h>
#include <SDL2/SDL.h>

#include "ppu.h"

#ifdef SHOW_FPS
    #define END_TIMER() end_frame_timer()
#else
    #define END_TIMER()
#endif

static PPU ppu;

void ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam, int *nmi, SDL_Texture *texture, SDL_Renderer *renderer, unsigned char *pixels) {
    ppu.chrom = chrom;
    ppu.vram = vram;
    ppu.palette = palette;
    ppu.oam = oam;
    ppu.nmi = nmi;
    // vblank
    ppu.scanline = -1;
    ppu.status = 0x80;
    *ppu.nmi = 0;
    //sdl 
    ppu.texture = texture;
    ppu.renderer = renderer;
    ppu.pixels = pixels;
}

static Uint64 last_time = 0;

void start_frame_timer() {
    last_time = SDL_GetPerformanceCounter();
}

double elapsed_time() {
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    double delta_time = (double)(current_time - last_time) / freq;  
    return delta_time * 1000;
}

void end_frame_timer() {
    Uint64 current_time = SDL_GetPerformanceCounter();
    Uint64 freq = SDL_GetPerformanceFrequency();
    double delta_time = (double)(current_time - last_time) / freq;  
    double fps = 1.0 / delta_time;
    fprintf(stderr, "frame time: %.3f ms, FPS: %.2f\n", delta_time * 1000, fps);
}

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
        ppu.pixels[pixel] = SYS_COLORS[sys_color][0];
        ppu.pixels[pixel + 1] = SYS_COLORS[sys_color][1];
        ppu.pixels[pixel + 2] = SYS_COLORS[sys_color][2];
    }
}

// TODO: implement priority after background drawing done
//  - draw sprite if it has priority OR background is transparent
// TODO: implement 8x16 sprites
// TODO: get rid of i/rev_i nonsense
void draw_sprite(int index) {
    int long_sprite_enabled = 0;
    if((ppu.ctrl >> 5) & 1) long_sprite_enabled = 1;
    int x = ppu.oam[index * 4 + 3];
    int y = ppu.oam[index * 4];
    int flip_x = ((ppu.oam[index * 4 + 2] << 1) & 0x80) >> 7;
    int flip_y = (ppu.oam[index * 4 + 2] & 0x80) >> 7;
    unsigned short pattern_index = ppu.oam[index * 4 + 1];
    unsigned short base = (ppu.ctrl & 0x08) << 9;

    // change base palette table if 8x16 sprites enabled
    if((ppu.ctrl >> 5) & 1) {
        if(ppu.oam[index * 4 + 1] & 1) base = 0x1000;
        else base = 0;
    }

    pattern_index = base + pattern_index * 16;
    unsigned char palette = ppu.oam[index * 4 + 2] & 0x03;

    int i_max = 8;
    if(long_sprite_enabled) i_max = 16;

    int i, j, rev_i, rev_j; 
    if(flip_x) { j = 7; rev_j = 0; }
    else       { rev_j = 7; j = 0; }
    if(flip_y) { i = i_max - 1; rev_i = 0; }
    else       { rev_i = i_max - 1; i = 0; }


    while(i < i_max && i > -1) {
        while(j < 8 && j > -1) {
            if(x + j > 255 || y + i > 240) {
                if(flip_x) { j--; rev_j++; }
                else       { j++; rev_j--; }
                continue;
            }
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
    if(ppu.scanline == -1 && ppu.dot == 0) {
       start_frame_timer(); 
    }
    while(ppu.scanline <= 260) {
        // read oam, find sprites on this scanline
        ppu.curr = 0;
        while(ppu.dot <= 340) {
            if(ppu.ppu_cycle >= (cycle * 3)) {
                return;
            }
            if(ppu.dot == 0 && ppu.scanline < 240) {
                if(ppu.scanline == -1) {
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
    double elapsed = elapsed_time();
    if(elapsed <= 17) {
        SDL_Delay(17 - elapsed);
    }
    END_TIMER();
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
    /* TODO: is this special case ever needed?
    if(ppu.status >> 7 && data >> 7 && (ppu.ctrl >> 7) == 0) {
        printf("vblank special case: scanline %d dot %d\n", ppu.scanline, ppu.dot);
        *ppu.nmi = 1;
    }
    */
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
