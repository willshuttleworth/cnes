#include <stdio.h>

const int PRG_ROM_SIZE = 16384; // 2^14
const int CHR_ROM_SIZE = 8192; // 2^13

//TODO: implement PPU access to bus
typedef struct Mem {
    unsigned char *cpu_ram;  
    unsigned char *rom;
    // ppu registers (pointer to ppu struct)
} Mem;

Mem mem = {
    .cpu_ram = NULL,
    .rom = NULL,
};

void bus_setup(unsigned char *cpu_mem, unsigned char *cpu_rom, unsigned char *instructions, int size) {
    mem.cpu_ram = cpu_mem;    
    mem.rom = cpu_rom;
    if(size == 1) {
        for(int i = 0; i < PRG_ROM_SIZE; i++) {
            mem.rom[i] = instructions[i];
            mem.rom[i + 0x4000] = instructions[i];
        }    
        //init interrupt vector
        mem.rom[(PRG_ROM_SIZE * 2) - 4] = 0x00;
        mem.rom[(PRG_ROM_SIZE * 2) - 3] = 0xC0;
    }
    else if(size == 2) {
        for(int i = 0; i < PRG_ROM_SIZE * 2; i++) {
            mem.rom[i] = instructions[i];
        }    
        //init interrupt vector
        mem.rom[(PRG_ROM_SIZE * 2) - 4] = 0x00;
        mem.rom[(PRG_ROM_SIZE * 2) - 3] = 0x80;
    }
    else {
        puts("invalid number of blocks");
    }
}

unsigned char bus_read(unsigned short addr) {
    // cpu ram
    if(addr < 0x2000) {
        addr &= 0x07FF;
        return mem.cpu_ram[addr];
    }
    // ppu register
    else if(addr < 0x4000) {
        printf("ERROR: tried to access address %x\n", addr); 
    }
    // joypad
    else if(addr == 0x4016) {
        printf("ERROR: tried to access address %x\n", addr); 
    }
    // cart rom
    else if(addr >= 0x8000) {
        addr %= 0x8000;
        return mem.rom[addr];
    }
    else {
        printf("ERROR: tried to access address %x\n", addr); 
    }
    return 0;
}

void bus_write(unsigned short addr, unsigned char data) {
    // cpu ram
    if(addr < 0x2000) {
        addr &= 0x07FF;
        mem.cpu_ram[addr] = data;
    }
    // ppu register
    else if(addr < 0x4000) {
        printf("ERROR: tried to write %x to address %x\n", data, addr); 
    }
    // joypad
    else if(addr == 0x4016) {
        printf("ERROR: tried to write %x to address %x\n", data, addr); 
    }
    else {
        // printf("ERROR: tried to write %x to address %x\n", data, addr); 
    }
}
