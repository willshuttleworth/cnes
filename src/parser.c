#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h> //fstat
#include "ppu.h"

#define PRG_ROM_SIZE 16384  //2^14
#define CHR_ROM_SIZE 8192  //2^13

#define IMPL_LEN 43
#define IMM_LEN 24
#define ACC_LEN 4
#define _ABS_LEN 32
#define ABS_X_LEN 30
#define ABS_Y_LEN 18
#define IND_LEN 1
#define X_IND_LEN 16
#define IND_Y_LEN 16
#define REL_LEN 8
#define ZPG_LEN 32
#define ZPG_X_LEN 28
#define ZPG_Y_LEN 4

unsigned char impl[IMPL_LEN] = {0x0, 0x40, 0x60, 0x08, 0x18, 0x28, 0x38, 0x48, 0x58, 0x68, 0x78, 0x88, 0x98, 0xA8, 0xB8, 0xC8, 0xD8, 0xE8, 0xF8, 0x02, 0x12, 0x22, 0x32, 0x42, 0x52, 0x62, 0x72, 0x92, 0xB2, 0xD2, 0xF2, 0x1A, 0x3A, 0x5A, 0x7A, 0x8A, 0x9A, 0xAA, 0xBA, 0xCA, 0xDA, 0xEA, 0xFA};
unsigned char imm[IMM_LEN] = {0x80, 0xA0, 0xC0, 0xE0, 0x82, 0xA2, 0xC2, 0xE2, 0x09, 0x29, 0x49, 0x69, 0x89, 0xA9, 0xC9, 0xE9, 0x0B, 0x2B, 0x4B, 0x6B, 0x8B, 0xAB, 0xCB, 0xEB};
unsigned char acc[ACC_LEN] = {0x0A, 0x2A, 0x4A, 0x6A};
unsigned char _abs[_ABS_LEN] = {0x20, 0x0C, 0x0D, 0x0E, 0x0F, 0x2C, 0x2D, 0x2E, 0x2F, 0x4C, 0x4D, 0x4E, 0x4F, 0x6D, 0x6E, 0x6F, 0x8C, 0x8D, 0x8E, 0x8F, 0xAC, 0xAD, 0xAE, 0xAF, 0xCC, 0xCD, 0xCE, 0xCF, 0xEC, 0xED, 0xEE, 0xEF};
unsigned char abs_x[ABS_X_LEN] = {0x1C, 0x1D, 0x1E, 0x1F, 0x3C, 0x3D, 0x3E, 0x3F, 0x5C, 0x5D, 0x5E, 0x5F, 0x7C, 0x7D, 0x7E, 0x7F, 0x9C, 0x9D, 0x9E, 0x9F, 0xBC, 0xBD, 0xDC, 0xDD, 0xDE, 0xDF, 0xFC, 0xFD, 0xFE, 0xFF};
unsigned char abs_y[ABS_Y_LEN] = {0x19, 0x39, 0x59, 0x79, 0x99, 0xB9, 0xD9, 0xF9, 0x1B, 0x3B, 0x5B, 0x7B, 0x9B, 0xBB, 0xDB, 0xFB, 0xBE, 0xBF};
unsigned char ind[IND_LEN] = {0x6C};
unsigned char x_ind[X_IND_LEN] = {0x01, 0x21, 0x41, 0x61, 0x81, 0xA1, 0xC1, 0xE1, 0x03, 0x23, 0x43, 0x63, 0x83, 0xA3, 0xC3, 0xE3};
unsigned char ind_y[IND_Y_LEN] = {0x11, 0x31, 0x51, 0x71, 0x91, 0xB1, 0xD1, 0xF1, 0x13, 0x33, 0x53, 0x73, 0x93, 0xB3, 0xD3, 0xF3};
unsigned char rel[REL_LEN] = {0x10, 0x30, 0x50, 0x70, 0x90, 0xB0, 0xD0, 0xF0};
unsigned char zpg[ZPG_LEN] = {0x04, 0x05, 0x06, 0x07, 0x24, 0x25, 0x26, 0x27, 0x44, 0x45, 0x46, 0x47, 0x64, 0x65, 0x66, 0x67, 0x84, 0x85, 0x86, 0x87, 0xA4, 0xA5, 0xA6, 0xA7, 0xC4, 0xC5, 0xC6, 0xC7, 0xE4, 0xE5, 0xE6, 0xE7};
unsigned char zpg_x[ZPG_X_LEN] = {0x14, 0x15, 0x16, 0x17, 0x34, 0x35, 0x36, 0x37, 0x54, 0x55, 0x56, 0x57, 0x74, 0x75, 0x76, 0x77, 0x94, 0x95, 0xB4, 0xB5, 0xD4, 0xD5, 0xD6, 0xD7, 0xF4, 0xF5, 0xF6, 0xF7};
unsigned char zpg_y[ZPG_Y_LEN] = {0x96, 0x97, 0xB6, 0xB7};


int parse_opcode(unsigned char opcode) {
    //impl
    for(int i = 0; i < IMPL_LEN; i++) {
        if(impl[i] == opcode) {
            return 1;
        }
    }

    //imm
    for(int i = 0; i < IMM_LEN; i++) {
        if(imm[i] == opcode) {
            return 2;
        }
    }

    //acc
    for(int i = 0; i < ACC_LEN; i++) {
        if(acc[i] == opcode) {
            return 3;
        }
    }

    //abs
    for(int i = 0; i < _ABS_LEN; i++) {
        if(_abs[i] == opcode) {
            return 4;
        }
    }

    //abs_x
    for(int i = 0; i < ABS_X_LEN; i++) {
        if(abs_x[i] == opcode) {
            return 5;
        }
    }

    //abs_y
    for(int i = 0; i < ABS_Y_LEN; i++) {
        if(abs_y[i] == opcode) {
            return 6;
        }
    }

    //ind
    for(int i = 0; i < IND_LEN; i++) {
        if(ind[i] == opcode) {
            return 7;
        }
    }

    //x_ind
    for(int i = 0; i < X_IND_LEN; i++) {
        if(x_ind[i] == opcode) {
            return 8;
        }
    }

    //ind_y
    for(int i = 0; i < IND_Y_LEN; i++) {
        if(ind_y[i] == opcode) {
            return 9;
        }
    }

    //rel
    for(int i = 0; i < REL_LEN; i++) {
        if(rel[i] == opcode) {
            return 10;
        }
    }

    //zpg
    for(int i = 0; i < ZPG_LEN; i++) {
        if(zpg[i] == opcode) {
            return 11;
        }
    }

    //zpg_x
    for(int i = 0; i < ZPG_X_LEN; i++) {
        if(zpg_x[i] == opcode) {
            return 12;
        }
    }

    //zpg_y
    for(int i = 0; i < ZPG_Y_LEN; i++) {
        if(zpg_y[i] == opcode) {
            return 13;
        }
    }

    return 0;
}

// .nes file header
typedef struct Header {
    char magic_num[4]; 
    char prg_rom_size; //16kb units
    char chr_rom_size; //8kb units
    char flags6;
    char flags7;
    char flags8;
    char flags9;
    char flags10;
    char padding[5];
} Header;

char *parse_blocks(FILE *file) {
    Header h;
    fread(&h, sizeof(Header), 1, file);
    //checking if trainer is present
    if((h.flags6 >> 2) | 0) {
        fseek(file, 512, SEEK_CUR);
    }
    char *ret = malloc(2);
    ret[0] = h.prg_rom_size;
    ret[1] = h.chr_rom_size;
    return ret;
}

void parse_instructions(FILE *file, unsigned char *rom, unsigned char *chrom, int prg_blocks, int chr_blocks) {
    //seek to end and find size of rom (dont include header yeah?) and allocate array big enough
    struct stat file_stats;
    file_stats.st_uid = 0;
    file_stats.st_gid = 0;
    file_stats.st_size = 0;
    file_stats.st_mode = 0;

    fstat(fileno(file), &file_stats);

    for(int i = 0; i < PRG_ROM_SIZE * prg_blocks; i++) {
        fread(&rom[i], sizeof(char), 1, file); 
    }
    for(int i = 0; i < CHR_ROM_SIZE * chr_blocks; i++) {
        fread(&chrom[i], sizeof(char), 1, file); 
    }

    if(prg_blocks == 1) {
        for(int i = 0; i < PRG_ROM_SIZE; i++) {
            rom[i + 0x4000] = rom[i];
        }    
        rom[0x7FFC] = 0x00;
        rom[0x7FFD] = 0xC0;
    }
    else if(prg_blocks == 2) {
        rom[0x7FFC] = 0x00;
        rom[0x7FFD] = 0x80;
    }
    else {
        perror("ERROR: invalid number of prg rom blocks");
    }
}
