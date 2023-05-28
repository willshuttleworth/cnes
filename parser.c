#include <stdlib.h>

#define NUM_OPCODES 256

typedef struct Table {
    unsigned char[NUM_OPCODES] opcodes;
    unsigned char[NUM_OPCODES] modes;
}Table;

//change to struct of 2 arrays where each opcode is mapped to an addressing type
//ex: if(opcode in abs)
//      return these bytes
//ONLY PARSING BYTES CORRECTLY; no need to separate instructions based on address type, just on number of bytes parsed
unsigned char *parse(unsigned char *instructions, int pos, int len) {
    unsigned char opcode = instructions[pos];
    unsigned char lo = (unsigned char) (opcode << 4); 
    unsigned char hi = opcode >> 4;

    //impl (add jam and nop instructions)
    int high_nibble = 0; 
    if(lo == 0x0) {
        if((opcode == 0x0) || (opcode == 0x40) || (opcode == 0x60)) {
            high_nibble = 1;
        }
    }
    else if(lo == 0xA0) {
        if((hi >= 0x7 && hi <= 0xF) || (hi % 2 != 0)) {
            high_nibble = 1;
        }
    }
    if((lo == 0x80) || (lo == 0x0 && high_nibble) || (lo == 0xA0 && high_nibble)) {
        //return opcode (no operands)
        unsigned char *bytes = malloc(1 + sizeof(char));
        bytes[0] = 1;
        bytes[1] = opcode;
        return bytes;
    }
     
    //acc (lo nibble is a and hi is 0,2,4,6)
    if(lo == 0xA0 && (hi < 0x7) && (hi % 2 == 0)) {
        //return opcode (no operands) to cpu
        unsigned char *bytes = malloc(1 + sizeof(char));
        bytes[0] = 1;
        bytes[1] = opcode;
        return bytes;
    }

    //imm (lo is 9 or b and hi is even OR lo is 0 or 2 and hi is greater than 7 and even
    if(((lo == 0x90 || lo == 0xB0) && hi % 2 == 0) || ((lo == 0x0 || lo == 0x20) && (hi > 0x7 && hi % 2 == 0))) {
        //return opcode and bit after opcode(imm)
        unsigned char *bytes = malloc(1 + sizeof(char) * 2);
        bytes[0] = 2;
        bytes[1] = opcode;
        bytes[2] = instructions[pos + 1];
        return bytes;
    }

    //abs (0x20 OR lo is c,d,e,f and hi is even (except 0x6C)
    if(opcode == 0x20 || ((lo == 0xC0 || lo == 0xD0 || lo == 0xE0 || lo == 0xF0) && hi % 2 == 0 && opcode != 0x6C)) {
        //return opcode and 16bit address
        unsigned char *bytes = malloc(1 + sizeof(char) * 3);
        bytes[0] = 3;
        bytes[1] = opcode;
        bytes[2] = instructions[pos + 2];
        bytes[3] = instructions[pos + 1];
        return bytes;
    } 
    
    //abs, x (lo is c,d and hi is odd OR lo is e,f and hi is odd (except 0xB and 0xE)
    if((lo == 0xC0 || lo == 0xD0 || lo == 0xE0 || lo == 0xF0) && hi % 2 != 0 && (opcode != 0x9E && opcode != 0x9F && opcode != 0xBE && opcode != 0xBF)) {
        unsigned char *bytes = malloc(1 + sizeof(char) * 3);
        bytes[0] = 3;
        bytes[1] = opcode;
        bytes[2] = instructions[pos + 2];
        bytes[3] = instructions[pos + 1];
        return bytes;
    }

    //abs, y (lo is 9,b and hi is odd OR 0x9E, 0x9F, 0xBE, 0xBF)
    if(((lo == 0x90 || lo == 0xB0) && hi % 2 != 0 ) || (opcode == 0x9E || opcode == 0x9F || opcode == 0xBE || opcode == 0xBF)) {
        unsigned char *bytes = malloc(1 + sizeof(char) * 3);
        bytes[0] = 3;
        bytes[1] = opcode;
        bytes[2] = instructions[pos + 2];
        bytes[3] = instructions[pos + 1];
        return bytes;
    }

    //ind 
    if(opcode == 0x6C) {
        unsigned char *bytes = malloc(1 + sizeof(char) * 3);
        bytes[0] = 3;
        bytes[1] = opcode;
        bytes[2] = instructions[pos + 2];
        bytes[3] = instructions[pos + 1];
        return bytes;
    }

    //x, ind
    if() {
    }

    //ind, y
    if() {
    }


    //TODO: 
    //  rel
    //  zpg
    //  zpg, x
    //  zpg, y
    //  nops and jams for impl
    //  commit when done with parsing

    return NULL;
}
