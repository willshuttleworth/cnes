#include <stdio.h>
#include <stdlib.h>
#include "ppu.h"

PPU *ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam) {
    PPU *ppu = malloc(sizeof(PPU));
    ppu->chrom = chrom;
    ppu->vram = vram;
    ppu->palette = palette;
    ppu->oam = oam;
    return ppu;
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
