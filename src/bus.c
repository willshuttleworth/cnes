#include <stdio.h>
#include "ppu.h"
#include "controller.h"

const int PRG_ROM_SIZE = 16384; // 2^14
const int CHR_ROM_SIZE = 8192; // 2^13

typedef struct Mem {
    unsigned char *cpu_ram;  
    unsigned char *rom;
} Mem;

Mem mem = {
    .cpu_ram = NULL,
    .rom = NULL,
};

void bus_setup(unsigned char *cpu_mem, unsigned char *cpu_rom) {
    mem.cpu_ram = cpu_mem;    
    mem.rom = cpu_rom;
}

unsigned char bus_read(unsigned short addr) {
    // cpu ram
    if(addr < 0x2000) {
        addr &= 0x07FF;
        return mem.cpu_ram[addr];
    }
    // ppu register
    else if(addr < 0x4000) {
        // addr is 0-7, corresponding to ppu reg
        addr %= 8;
        switch(addr) {
            // ctrl
            case 0:
                perror("attempting to read from write only address (PPUCTRL)");
                break;
            // mask
            case 1:
                perror("attempting to read from write only address (PPUMASK)");
                break;
            // status
            case 2:
                return status_read();
            // oam addr
            case 3:
                perror("attempting to read from write only address (OAMADDR)");
                break;
            // oam data
            case 4:
                return oamdata_read();
            // scroll
            case 5:
                perror("attempting to read from write only address (PPUSCROLL)");
                break;
            // addr
            case 6:
                perror("attempting to read from write only address (PPUADDR)");
                break;
            // data
            case 7:
                return data_read();
        }
        
    }
    // joypad
    else if(addr == 0x4016) {
        return controller_read();
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
        // addr is 0-7, corresponding to ppu reg
        addr %= 8;
        switch(addr) {
            // ctrl
            case 0:
                ctrl_write(data);
                break;
            // mask
            case 1:
                mask_write(data);
                break;
            // status
            case 2:
                perror("attempting to write to read only address (PPUSTATUS)");
                break;
            // oam addr
            case 3:
                oamaddr_write(data);
                break;
            // oam data
            case 4:
                oamdata_write(data);
                break;
            // scroll
            case 5:
                //perror("ERROR: PPUSCROLL writing not implemented");
                break;
            // addr
            case 6:
                addr_write(data);
                break;
            // data
            case 7:
                data_write(data);
                break;
        }
    }
    // joypad
    else if(addr == 0x4016) {
        set_strobe(data);
    }
    else {
        printf("ERROR: tried to write %x to address %x\n", data, addr); 
    }
}
