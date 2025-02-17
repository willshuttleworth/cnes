#include <stdio.h>
#include <stdlib.h>
#include "ppu.h"

typedef struct PPU {
    //sprites as bitmap images (pattern tables)
    unsigned char *chrom;
    //layout (nametables)
    unsigned char *vram;
    //colors
    unsigned char *palette;
    //sprite positioning (not part of ppu mmap, accessed independently)
    unsigned char *oam;

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

    // oam dma ?
} PPU;

PPU ppu = {
    .chrom = 0,
    //layout (nametables)
    .vram = 0,
    //colors
    .palette = 0,
    //sprite positioning (not part of ppu mmap, accessed independently)
    .oam = 0,

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
};

void ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam) {
    ppu.chrom = chrom;
    ppu.vram = vram;
    ppu.palette = palette;
    ppu.oam = oam;
}

unsigned char ppu_read(unsigned short addr) {
    addr %= 0x4000; 

    // chrom
    if(addr < 0x2000) {

    }
    // vram
    else if(addr < 0x3000) {

    }
    // palette
    else {

    }
    return 0;
}

void ppu_write(unsigned short addr, unsigned char data) {
    addr %= 0x4000; 

    // chrom
    if(addr < 0x2000) {

    }
    // vram
    else if(addr < 0x3000) {

    }
    // palette
    else {

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
    printf("addr: %x w: %d t: %x\n", ppu.addr, ppu.w, ppu.t);
}
