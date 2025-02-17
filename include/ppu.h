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

} PPU;

void ppu_tick_to(unsigned int cycle);
PPU *ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam);
unsigned char ppu_read(unsigned short addr);
void ppu_write(unsigned short addr, unsigned char data);

#define CHROM_SIZE 8192
#define VRAM_SIZE 2048
#define PALETTE_SIZE 32
#define OAM_SIZE 256

