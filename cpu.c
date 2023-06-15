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
    //needs to be 8bits and normal for testing purposes. hi byte is always 0x01 so stack address is 0x01(sp) ex: 0x01FF
    //stack pointer is decreased on pushes and increased on pops
    //sp is pointer so it can be null, and null is empty stack
    unsigned char *sp; 

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
extern unsigned char *sp_init_val;

//stack is 0x100 to 0x1FF, next stack address is mem[STACK_TOP - stack_ptr]
//stack grows down from STACK_TOP(0x1FF)
unsigned char *mem;
extern int cpu_cycle;

CPU cpu = { 
            .acc = 0,
            .x = 0,
            .y = 0,
            .pc = 0,
            .sp = NULL,
            .cf = 1,
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
            cycle, cpu.acc, cpu.x, cpu.y, cpu.pc, *cpu.sp, cpu.cf, cpu.zf, cpu.id, cpu.dm, cpu.brk, cpu.of, cpu.neg);
}

//push/pop stuff from stack
//stack pointer points to last occupied stack pos
//push: move everything from STACK_TOP to stack_pointer down one address, put new value at mem[STACK_TOP]
//  overflow: if stack pointer is already 255, then another push would overflow (is overflow an error that is ignored on 6502?)
//pop: save mem[STACK_TOP], move everything from STACK_TOP-1 to stack_pointer up one address, return saved value
//  underflow: stack pointer is negative (should only ever be -1) and a pop was called

void push(unsigned char byte) {
    //push on empty stack
    if(cpu.sp == NULL) {
        mem[STACK_TOP] = byte;
        cpu.sp = sp_init_val;
        return;
    }
    //overflow
    if(*cpu.sp == 0) {
        puts("stack overflow");
        return;
    }

    for(unsigned short i = *cpu.sp; i <= 0xFF; i++) {
        mem[0x100 + i] = mem[0x100 + i + 1];
    }
    mem[STACK_TOP] = byte;
    *cpu.sp -= 1;
}

unsigned char pop() {
    if(cpu.sp == NULL) {
        puts("stack underflow"); 
        exit(0); //is there a sentinel value that could be returned instead of crashing?
    }

    //save mem[STACK_TOP], move everything else up an address
    unsigned char popped = mem[STACK_TOP];
    for(short i = 0xFF; i >= *cpu.sp; i--) {
        mem[0x100 + i + 1] = mem[0x100 + i];
    }
    if(*cpu.sp == 0xFF) {
        cpu.sp = NULL;
        return popped;
    }
    *cpu.sp += 1;
    return popped;
}

void print_stack() {
    if(cpu.sp == NULL) {
        puts("stack is empty");
        return;
    }
    printf("stack: ");
    for(short i = 0xFF; i >= *cpu.sp; i--) {
        printf("%x ", mem[0x100 + i]);
    }
    printf("\n");
}

//stack tests:
//  - add one to empty stack
//  - pop one
//  - fill stack with increasing numbers
//  - overflow
//  - pop all 
//  - underflow
void stack_test() {
    print_stack(); //stack should start off empty
    push(0);
    print_stack(); //should contain one zero
    for(int i = 1; i < 256; i++) {
        push((unsigned char) i);
    }
    print_stack();
    push(5); //should print overflow warning
    //print_stack(); //should be full
    for(int i = 0; i < 256; i++) {
        pop();
        print_stack();
    }
    pop(); //should underflow
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
