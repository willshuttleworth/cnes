#include "ppu.h"

void bus_setup(unsigned char *cpu_mem, unsigned char *cpu_rom);
unsigned char bus_read(unsigned short addr);
void bus_write(unsigned short addr, char data);

