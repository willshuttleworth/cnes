#include <stdio.h>
#include <stdlib.h>
#include "../parser/parser.h"

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
    //sp points to next available stack location, so it starts at 0xFF, and negative 1 means full stack
    short sp; 

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

//stack is 0x100 to 0x1FF, next stack address is mem[STACK_TOP - stack_ptr]
//stack grows down from STACK_TOP(0x1FF)
unsigned char *mem;
extern int cpu_cycle;

CPU cpu = { 
            .acc = 0,
            .x = 0,
            .y = 0,
            .pc = 0xC000,
            .sp = 0xFF,
            .cf = 0,
            .zf = 0,
            .id = 0,
            .dm = 0,
            .brk = 0,
            .of = 0,
            .neg = 0,
};

void cpu_setup(unsigned char *instr, unsigned char *memory, int size) {
    mem = memory;
    //load instructions into correct spot in memory
    load(instr, mem, size);
}

//encode all status registers as single byte
unsigned char encode_status() {
    unsigned char status = 0;
    if(cpu.neg == 1) {
        status |= 1;
    }
    status <<= 1;
   
    if(cpu.of == 1) {
        status |= 1;
    }
    status <<= 1; 
    //bit 5 is always 1 when pushed onto stack
    status |= 1;
    status <<= 1; 
    
    if(cpu.brk == 1) {
        status |= 1;
    }
    status <<= 1; 

    if(cpu.dm == 1) {
        status |= 1;
    }
    status <<= 1; 
    
    if(cpu.id == 1) {
        status |= 1;
    }
    status <<= 1; 
    
    if(cpu.zf == 1) {
        status |= 1;
    }
    status <<= 1; 
    
    if(cpu.cf == 1) {
        status |= 1;
    }

    return status;
}

//print cpu state
void print_cpu(unsigned short old_pc, unsigned char *args, int len) {
    printf("%X  ", old_pc);
    for(int i = 0; i < len; i++) {
        printf("%X ", args[i]);
    }
    if(len == 1) {
        printf("\t\t");
    }
    else {
        printf("\t");
    }
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%X CYC:%d\n", cpu.acc, cpu.x, cpu.y, encode_status(), cpu.sp, cpu_cycle);
}

//push/pop stuff from stack
//stack pointer points to last occupied stack pos
//push: move everything from STACK_TOP to stack_pointer down one address, put new value at mem[STACK_TOP]
//  overflow: if stack pointer is already 255, then another push would overflow (is overflow an error that is ignored on 6502?)
//pop: save mem[STACK_TOP], move everything from STACK_TOP-1 to stack_pointer up one address, return saved value
//  underflow: stack pointer is negative (should only ever be -1) and a pop was called
void push(unsigned char byte) {
    //overflow
    if(cpu.sp < 0) {
        puts("stack overflow");
        return;
    }

    for(unsigned short i = cpu.sp; i <= 0xFF; i++) {
        mem[0x100 + i - 1] = mem[0x100 + i];
    }
    mem[STACK_TOP] = byte;
    cpu.sp -= 1;
}

unsigned char pop() {
    if(cpu.sp == 0xFF) {
        puts("stack underflow"); 
        exit(0); //is there a sentinel value that could be returned instead of crashing?
    }

    //save mem[STACK_TOP], move everything else up an address
    unsigned char popped = mem[STACK_TOP];
    for(short i = 0xFF; i > cpu.sp; i--) {
        mem[0x100 + i + 1] = mem[0x100 + i];
    }
    cpu.sp += 1;
    return popped;
}

void print_stack() {
    if(cpu.sp == 0xFF) {
        puts("stack is empty");
        return;
    }
    printf("stack: ");
    for(short i = 0xFF; i > cpu.sp; i--) {
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
    unsigned char opcode = mem[cpu.pc];

    //brk
    if(opcode == 0x00) {
        //technically a 2 byte instruction, so next pc is pc+2
        unsigned char args[1] = {mem[cpu.pc]};
        unsigned short old_pc = cpu.pc;

        cpu.pc += 2;
        cpu.brk = 1;
        //push status reg, pc lo byte, pc hi byte onto stack (in that order)
        //have to encode sr gahhhhhhh
        //will write function that encodes all individual statuses to one byte
        //push(cpu.sr);
        push((unsigned char)cpu.pc); 
        push((unsigned char)(cpu.pc >> 8)); 
        //set hi pc to val at 0xFFFF, lo pc to val at 0xFFFE (which is lo/hi of pc?)
        //what is at these memory addresses? when do they get loaded?
        cpu.pc = mem[0xFFFF];
        cpu.pc <<= 8;
        cpu.pc |= mem[0xFFFE];
        cpu_cycle += 7;
        cpu.brk = 0; 

        print_cpu(old_pc, args, 1);
        return 0;
    }

    //CLC: set carry flag to zero
    else if(opcode == 0x18) {
        unsigned char args[1] = {mem[cpu.pc]};
        unsigned short old_pc = cpu.pc;
        cpu.cf = 0;
        cpu_cycle += 2;
        cpu.pc += 1;

        print_cpu(old_pc, args, 1);
        return 0;
    }
    
    //JMP abs: jump to absolute address
    else if(opcode == 0x4C) {
        unsigned char args[3] = {mem[cpu.pc], mem[cpu.pc+1], mem[cpu.pc+2]};
        unsigned short old_pc = cpu.pc;
        //set next pc to address
        cpu.pc = mem[old_pc + 2];
        cpu.pc <<= 8;
        cpu.pc |= mem[old_pc + 1];
        cpu_cycle += 3;
        print_cpu(old_pc, args, 3);
        return 0;
    }
    cpu.pc += 1; 
    return 0;
}
