#include <stdio.h>
#include "ppu.h"
#include "controller.h"
#include "cnes.h"

#ifdef DEBUG
    #define DEBUG_PRINT(s) printf(s)
#else
    #define DEBUG_PRINT(s) 
#endif

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
                DEBUG_PRINT("attempting to read from write only address (PPUCTRL)\n");
                break;
            // mask
            case 1:
                DEBUG_PRINT("attempting to read from write only address (PPUMASK)\n");
                break;
            // status
            case 2:
                return status_read();
            // oam addr
            case 3:
                DEBUG_PRINT("attempting to read from write only address (OAMADDR)\n");
                break;
            // oam data
            case 4:
                return oamdata_read();
            // scroll
            case 5:
                DEBUG_PRINT("attempting to read from write only address (PPUSCROLL)\n");
                break;
            // addr
            case 6:
                DEBUG_PRINT("attempting to read from write only address (PPUADDR)\n");
                break;
            // data
            case 7:
                return data_read();
        }
        
    }
    // joypad 1
    else if(addr == 0x4016) {
        return controller_read();
    }
    // joypad 2
    else if(addr == 0x4017) {
        return 0;
    }
    // cart rom
    else if(addr >= 0x8000) {
        addr %= 0x8000;
        return mem.rom[addr];
    }
    else {
        return 0;
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
                DEBUG_PRINT("attempting to write to read only address (PPUSTATUS)\n");
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
    // joypad 1
    else if(addr == 0x4016) {
        set_strobe(data);
    }
    // joypad 2
    else if(addr == 0x4017) {
        return;
    }
    else {
        return;
    }
}
