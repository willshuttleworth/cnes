#include <stdio.h>
#include <stdlib.h>
#include "ppu.h"
#include "../parser/parser.h"

typedef struct PPU {
    //loaded from rom
    unsigned char *cpu_mem;
    //sprites as bitmap images
    unsigned char *chrom;
    //layout
    unsigned char *vram;
    //colors
    unsigned char *palette_table;
    //sprite positioning
    unsigned char *oam_data;
}PPU;

PPU ppu = { .cpu_mem = NULL, 
            .chrom = NULL,
            .vram = NULL,
            .palette_table = NULL,
            .oam_data = NULL, 
};

//memory maps
//0x2000 to 0x2007 are ppu registers accesible in cpu memory map

void ppu_setup(unsigned char *instr, unsigned char *cpu, unsigned char *chrom, unsigned char *ram, unsigned char *palette, unsigned char *oam) {
    ppu.cpu_mem = cpu;
    ppu.chrom = chrom;
    ppu.vram = ram;
    ppu.palette_table = palette;
    ppu.oam_data = oam;
    load_ppu(instr, ppu.chrom);
}

//write functions that read/write to ppu mem by getting data from cpu mem map
//  also that encode/decode other registers
