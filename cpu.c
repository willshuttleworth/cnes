#include <stdio.h>
#include <stdlib.h>
#include "parser.h"

#define STACK_TOP 0x1FF

typedef struct CPU {
    //accumulator
    char acc;
    //index register x
    unsigned char x;
    //index register y
    unsigned char y;
    //program counter
    unsigned short pc;
    //stack pointer
    unsigned char sp; 

    //processor status:
    
    //carry flag
    char cf;
    //zero flag
    char zf;
    //interrupt disable
    char id;
    //decimal mode
    char dm;
    //break command
    char brk;
    //overflow flag
    char of;
    //negative flag
    char neg;

}CPU;

//cpu globals
unsigned char *instructions;
int len;

//stack is 0x100 to 0x1FF, next stack address is mem[STACK_TOP - stack_ptr]
//stack grows down from STACK_TOP(0x1FF)
unsigned char *mem;
extern int cpu_cycle;
//where should clock cycle be stored?

CPU cpu = { 
            .acc = 0,
            .x = 0,
            .y = 0,
            .pc = 0,
            .sp = 0,
            .cf = 0,
            .zf = 0,
            .id = 0,
            .dm = 0,
            .brk = 0,
            .of = 0,
            .neg = 0,
};

void cpu_setup(unsigned char *instr, int length, unsigned char *memory) {
    instructions = instr;
    len = length;
    mem = memory;
}

void print_cpu(int cycle) {
    printf("cycle: %d\tacc: %x\tx: %x\ty: %x\tpc: %x\tsp: %x\tcf: %d\tzf: %d\tid: %d\tdm: %d\tbrk: %d\tof: %d\tneg: %d\n",
            cycle, cpu.acc, cpu.x, cpu.y, cpu.pc, cpu.sp, cpu.cf, cpu.zf, cpu.id, cpu.dm, cpu.brk, cpu.of, cpu.neg);
}

// parse out each opcode individually(except for repeats like jams, nops that take same num of cycles, etc)
int exec_instr() {
    unsigned char *operands = parse(instructions, cpu.pc, len);
    //CLC: set carry flag to zero
    //two cycles
    if(operands == NULL) {
        puts("parse error: parse returned null");
        free(operands);
        free(mem);
        
        //return some error to main? then main can cleanup everything and dont have to free instructions and mem in two different places?
        exit(0);
    }

    //brk
    else if(operands[1] == 0x00) {
        //technically a 2 byte instruction, so next pc is pc+2
        cpu.pc += 2;
        cpu.brk = 1;
        //push status reg, pc lo byte, pc hi byte onto stack (in that order)
        //set hi pc to val at 0xFFFF, lo pc to val at 0xFFFE (which is lo/hi of pc?)
        //what is at these memory addresses? when do they get loaded?
        cpu_cycle += 7;
        
        free(operands);
        return 0;
    }

    //CLC: clear carry bit
    else if(operands[1] == 0x18) {
        cpu.cf = 0;
        cpu_cycle += 2;
        cpu.pc += 1;

        free(operands);
        return 0;
    }
    free(operands);
    cpu.pc += 1;
    return 0;
}
