#include <stdio.h>
#include <stdlib.h>

typedef struct CPU {
    //cpu can mark that it is ready to receive next instruction from main
    int ready;

    //cycle that cpu is on. increase until it matches overall clock
    unsigned int cycle;
    
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

CPU cpu = { .ready = 1, 
            .cycle = 0,
            .acc = 0,
            .x = 0,
            .y = 0,
            .pc = 0,
            .sp = 0,
            .cf = 1,
            .zf = 0,
            .id = 0,
            .dm = 0,
            .brk = 0,
            .of = 0,
            .neg = 0,
};

void print_cpu() {
    printf("cycle: %d\tacc: %x\tx: %x\ty: %x\tpc: %x\tsp: %x\tcf: %d\tzf: %d\tid: %d\tdm: %d\tbrk: %d\tof: %d\tneg: %d\n",
            cpu.cycle, cpu.acc, cpu.x, cpu.y, cpu.pc, cpu.sp, cpu.cf, cpu.zf, cpu.id, cpu.dm, cpu.brk, cpu.of, cpu.neg);
}

// parse out each opcode individually(except for repeats like jams, nops that take same num of cycles, etc)
void exec_instr(unsigned char *operands) {
    //CLC: set carry flag to zero
    //two cycles
    if(operands[1] == 0x18) {
        cpu.cf = 0;
    }
}

void cpu_tick_to(unsigned int cycle) {
    printf("stepping up to cycle number %d from cycle number %d\n", cycle, cpu.cycle);
    while(cpu.cycle < cycle) {
        //execute instruction, and count how many cycles that took. will probably overshoot
        //  main clock. then, wait until main clock catches up to do anything else
        //exec_instr();
        cpu.cycle++;
    }
}
