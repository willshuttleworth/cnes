#include <SDL2/SDL.h>

void ppu_tick_to(int cycle);
void ppu_setup(unsigned char *chrom, unsigned char *vram, unsigned char *palette, unsigned char *oam2, int *nmi, SDL_Texture *texture, SDL_Renderer *renderer, unsigned char *pixels);
unsigned char ppu_read(unsigned short addr);
void ppu_write(unsigned short addr, unsigned char data);
void addr_write(unsigned char addr);
unsigned char data_read();
void data_write(unsigned char data);
void ctrl_write(unsigned char data);
void mask_write(unsigned char data);
unsigned char status_read();
void oamaddr_write(unsigned char data);
unsigned char oamdata_read();
void oamdata_write(unsigned char data);
