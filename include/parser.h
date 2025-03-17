char *parse_blocks(FILE *file);
void parse_instructions(FILE *file, unsigned char *rom, unsigned char *chrom, int prg_blocks, int chr_blocks);
int parse_opcode(unsigned char opcode);

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
