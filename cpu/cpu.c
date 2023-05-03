#include <stdio.h>

typedef struct CPU {
    //cycle that cpu is on. increase until it matches overall clock
    unsigned int cycle;
    
    //register file
    char a;
    char x;
    char y;
    char p;
    short pc;
    char sp; 
}CPU;

CPU cpu = { .cycle = 0,
            .a = 0,
            .x = 0,
            .y = 0,
            .p = 0,
            .pc = 0,
            .sp = 0 };


void cpu_tick_to(unsigned int cycle) {
    printf("stepping up to cycle number %d from cycle number %d\n", cycle, cpu.cycle);
    while(cpu.cycle < cycle) {
        //execute instruction, and count how many cycles that took. will probably overshoot
        //  main clock. then, wait until main clock catches up to do anything else
        cpu.cycle++;
    }
}
