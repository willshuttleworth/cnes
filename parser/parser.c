#include <stdio.h>
#include <stdlib.h>
#include <sys/stat.h>

#define ROM_SIZE 16384  //2^14

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
}Header;

int parse_blocks(FILE *file) {
    Header h;
    fread(&h, sizeof(Header), 1, file);
    //checking if trainer is present
    if((h.flags6 >> 2) | 0) {
        fseek(file, 512, SEEK_CUR);
    }
    
    return h.prg_rom_size;
}

unsigned char *parse_instructions(FILE *file) {
    //seek to end and find size of rom (dont include header yeah?) and allocate array big enough
    struct stat file_stats;
    file_stats.st_uid = 0;
    file_stats.st_gid = 0;
    file_stats.st_size = 0;
    file_stats.st_mode = 0;

    fstat(fileno(file), &file_stats); //better to use c library wrappers over syscalls? idk
    int num_bytes = file_stats.st_size;
    int len = num_bytes - sizeof(Header);
    unsigned char *instructions = malloc(len);
    
    //read all bytes of rom into an array
    int i = 0;
    while(!feof(file)) {
        fread(&instructions[i], sizeof(char), 1, file); 
        i++;
    } 
    return instructions;
}

//load instructions into correct memory addresses, setup interrupt vector
//size is number of 16kib blocks, so 1 or 2
void load(unsigned char *instructions, unsigned char *mem, int size) {
    //if size is 1, copy from 0x8000 to 0xBFFF and 0xC000 to 0x10000
    //if size is 2, copy from 0x8000 to 0x10000
    if(size == 1) {
        for(int i = 0; i < ROM_SIZE; i++) {
            mem[0x8000 + i] = instructions[i];
            mem[0xC000 + i] = instructions[i];
        }
        //initialize interupt vector
        mem[0xFFFC] = 0x00;
        mem[0xFFFD] = 0xC0;
        return;
    }
    else if(size == 2) {
        for(int i = 0; i < (ROM_SIZE * 2); i++) {
            mem[0x8000 + i] = instructions[i];
        } 
        //initialize interupt vector
        mem[0xFFFC] = 0x00;
        mem[0xFFFD] = 0x80;
        return;
    }
    else {
        puts("invalid number of prg rom blocks");
        exit(0);
    }
}
