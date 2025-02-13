void ppu_tick_to(unsigned int cycle);
void ppu_setup(unsigned char *instr, unsigned char *cpu, unsigned char *chrom, unsigned char *ram, unsigned char *palette, unsigned char *oam);

#define CHROM_SIZE 8192
#define VRAM_SIZE 2048
#define PALETTE_SIZE 32
#define OAM_SIZE 256

