#include "ppu.h"

void bus_setup(unsigned char *cpu_mem, unsigned char *cpu_rom);
unsigned char bus_read(unsigned short addr);
void bus_write(unsigned short addr, unsigned char data);

typedef struct Mem {
    unsigned char *cpu_ram;
    unsigned char *rom;
} Mem;

