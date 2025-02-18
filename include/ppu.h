void ppu_tick_to(unsigned int cycle);
void ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam);
unsigned char ppu_read(unsigned short addr);
void ppu_write(unsigned short addr, unsigned char data);
void addr_write(unsigned char addr);
unsigned char data_read();
void data_write(unsigned char data);

#define CHROM_SIZE 8192
#define VRAM_SIZE 2048
#define PALETTE_SIZE 32
#define OAM_SIZE 256

