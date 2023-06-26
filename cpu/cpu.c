#include <stdio.h>
#include <stdlib.h>
#include "../parser/parser.h"

#define STACK_TOP 0x1FF
#define PAGE_SIZE 256

typedef struct CPU {
    //accumulator
    unsigned char acc;
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
            .pc = 0,
            .sp = 0xFF,
            .cf = 0,
            .zf = 0,
            .id = 1,
            .dm = 0,
            .brk = 0,
            .of = 0,
            .neg = 0,
};

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

//set cpu flags based on status value
void decode_status(unsigned char status) {
    //cf
    if((status & 0x1) == 0x1) {
        cpu.cf = 1;
    }
    else {
        cpu.cf = 0;
    }

    //zf
    if((status & 0x2) == 0x2) {
        cpu.zf = 1;
    }
    else {
        cpu.zf = 0;
    }

    //id
    if((status & 0x4) == 0x4) {
        cpu.id = 1;
    }
    else {
        cpu.id = 0;
    }
    
    //dm
    if((status & 0x8) == 0x8) {
        cpu.dm = 1;
    }
    else {
        cpu.dm = 0;
    }

    //brk is ignored

    //of
    if((status & 0x40) == 0x40) {
        cpu.of = 1;
    }
    else {
        cpu.of = 0;
    }

    //neg
    if((status & 0x80) == 0x80) {
        cpu.neg = 1;
    }
    else {
        cpu.neg = 0;
    }
}

//print cpu state
void print_cpu(unsigned char *args, int len) {
    printf("%X  ", cpu.pc);
    for(int i = 0; i < len; i++) {
        printf("%02X ", args[i]);
    }
    if(len == 3) {
        printf(" ");
    }
    else if(len == 2){
        printf("    ");
    }
    else {
        printf("       ");
    }
    printf("A:%02X X:%02X Y:%02X P:%02X SP:%X CYC:%d\n", (unsigned char) cpu.acc, cpu.x, cpu.y, encode_status(), cpu.sp, cpu_cycle);
}

//are addresses on same page?
int same_page(short address1, short address2) {
    int page1 = address1 / PAGE_SIZE;
    int page2 = address2 / PAGE_SIZE;
    
    if(page1 == page2) {
        return 1;
    } 
    else {
        return 0;
    }
}

//after acc is changed, update zf and neg
void set_flags_a() {
    //set zf
    if(cpu.acc == 0) {
        cpu.zf = 1;
    }
    else {
        cpu.zf = 0;
    }
    //set neg
    cpu.neg = (unsigned char) cpu.acc >> 7;
}

void set_flags_x() {
    //set zf
    if(cpu.x == 0) {
        cpu.zf = 1;
    }
    else {
        cpu.zf = 0;
    }
    //set neg
    cpu.neg = (unsigned char) cpu.x >> 7;
}

void set_flags_y() {
    //set zf
    if(cpu.y == 0) {
        cpu.zf = 1;
    }
    else {
        cpu.zf = 0;
    }
    //set neg
    cpu.neg = (unsigned char) cpu.y >> 7;
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

//just calls push twice but does shifts and ordering
void push_word(short word) {
    //push hi then lo 
    push((unsigned char) (word >> 8));
    push((unsigned char) word);
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

short pop_word() {
    unsigned char lo = pop();
    unsigned char hi = pop();
    short word = hi;
    word <<= 8;
    word |= lo;
    return word;
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
    //pop(); //should underflow
    push_word(0x1234);
    print_stack();
    printf("%x\n", pop_word());
    print_stack();
}

void cpu_setup(unsigned char *instr, unsigned char *memory, int size) {
    mem = memory;
    //load instructions into correct spot in memory
    load(instr, mem, size);

    push(0);
    push(0);
    //take 7 cycles to set pc to value in mem[0xFFFC/D]
    cpu.pc = mem[0xFFFD];     
    cpu.pc <<= 8;
    cpu.pc |= mem[0xFFFC];
    cpu_cycle = 7;
}

// parse out each opcode individually(except for repeats like jams, nops that take same num of cycles, etc)
int exec_instr() {
    unsigned char opcode = mem[cpu.pc];
    /*
     *
     * -------------------------- IMPLs -------------------------- 
     *
     */

    //brk
    if(opcode == 0x00) {
        //technically a 2 byte instruction, so next pc is pc+2
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);

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

        return 0;
    }

    //CLC impl: set carry flag to zero
    else if(opcode == 0x18) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.cf = 0;
        cpu_cycle += 2;
        cpu.pc += 1;

        return 0;
    }

    //SEC impl: set carry flag
    else if(opcode == 0x38) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.cf = 1;

        cpu_cycle += 2;
        cpu.pc += 1;
        return 0;
    }
    
    //NOP
    else if(opcode == 0xEA) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu_cycle += 2;
        cpu.pc += 1;
        return 0;
    }

    //RTS: return from subroutine
    else if(opcode == 0x60) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.pc = pop_word();
        cpu_cycle += 6;
        return 0;
    }

    //SEI: set id
    else if(opcode == 0x78) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.id = 1; 
        cpu_cycle += 2;
        cpu.pc += 1;
        return 0;
    }

    //SED: set df
    else if(opcode == 0xF8) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.dm = 1; 
        cpu_cycle += 2;
        cpu.pc += 1;
        return 0;
    }

    //CLD: clear df
    else if(opcode == 0xD8) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.dm = 0; 
        cpu_cycle += 2;
        cpu.pc += 1;
        return 0;
    }

    //PHP: push status to stack
    else if(opcode == 0x08) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        //bits 4 and 5 are always pushed as 1s
        push(encode_status() | 0x10);
        cpu_cycle += 3;
        cpu.pc += 1;
        return 0;
    }

    //PLA: sets acc to stack value
    else if(opcode == 0x68) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.acc = pop();
        set_flags_a();
        cpu.pc += 1;
        cpu_cycle += 4;
        return 0;
    }

    //PHA: push acc onto stack
    else if(opcode == 0x48) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        push(cpu.acc);
        cpu.pc += 1;
        cpu_cycle += 3;
        return 0;
    }

    //PLP: set processor status from stack
    else if(opcode == 0x28) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        decode_status(pop());
        cpu.pc += 1;
        cpu_cycle += 4;
        return 0;
    }

    //CLV: clear overflow flag
    else if(opcode == 0xB8) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.of = 0;
        cpu.pc += 1;
        cpu_cycle += 2;
        return 0;
    }

    //INY: increment y
    else if(opcode == 0xC8) {
        unsigned char args[1] = {mem[cpu.pc]};
        print_cpu(args, 1);
        cpu.y += 1;
        set_flags_y();
        cpu.pc += 1;
        cpu_cycle += 2;
        return 0;
    }
    
    /*
     *
     * -------------------------- IMM -------------------------- 
     *
     */

    //LDX imm: load register x with imm
    else if(opcode == 0xA2) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load x with imm value
        cpu.x = mem[cpu.pc+1];
        set_flags_x();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //LDY imm: load register y with imm
    else if(opcode == 0xA0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load y with imm value
        cpu.y = mem[cpu.pc+1];
        set_flags_y();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //LDA imm: load register acc with imm
    else if(opcode == 0xA9) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load a with imm value
        cpu.acc = mem[cpu.pc+1];
        set_flags_a();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //AND imm: acc = acc & imm
    else if(opcode == 0x29) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load a with imm value
        cpu.acc &= args[1];
        set_flags_a();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //ORA imm: acc = acc | imm
    else if(opcode == 0x09) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load a with imm value
        cpu.acc |= args[1];
        set_flags_a();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //EOR imm: acc = acc ^ imm
    else if(opcode == 0x49) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //load a with imm value
        cpu.acc ^= args[1];
        set_flags_a();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //ADC imm: add with carry
    else if(opcode == 0x69) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        short result = cpu.acc + args[1] + cpu.cf;
        //setting carry
        if(result > 0xFF) {
            cpu.cf = 1;
        }
        else {
            cpu.cf = 0;
        }
        //setting overflow
        if((cpu.acc >> 7) == (args[1] >> 7)) {
            if((result >> 7) != (cpu.acc >> 7)) {
                cpu.of = 1;
            }
            else {
                cpu.of = 0;
            }
        }
        else {
            cpu.of = 0;
        }
        cpu.acc = (unsigned char) result;
        set_flags_a();
        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //SBC imm: subtract with carry
    else if(opcode == 0xE9) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        short result = cpu.acc + (unsigned char) ~args[1] + (unsigned char) cpu.cf;
        
        //setting carry
        if(result > 0xFF) {
            cpu.cf = 1;
        }
        else {
            cpu.cf = 0;
        }
        //setting overflow
        if((cpu.acc >> 7) == (~args[1] >> 7)) {
            if((result >> 7) != (cpu.acc >> 7)) {
                cpu.of = 1;
            }
            else {
                cpu.of = 0;
            }
        }
        else {
            cpu.of = 0;
        }
        cpu.acc = (unsigned char) result;
        set_flags_a();

        cpu.pc += 2;
        cpu_cycle += 2;
        return 0;
    } 

    //CMP imm: compare acc and imm and set flags
    else if(opcode == 0xC9) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //set flags
        if(cpu.acc >= args[1]) {
            cpu.cf = 1;
        }
        else {
            cpu.cf = 0;
        }
        if(cpu.acc == args[1]) {
            cpu.zf = 1;
        } 
        else {
            cpu.zf = 0;
        }
        cpu.neg = (((unsigned char) (cpu.acc - args[1])) >> 7);

        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //CPX imm: compare x and imm and set flags
    else if(opcode == 0xE0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //set flags
        if(cpu.x >= args[1]) {
            cpu.cf = 1;
        }
        else {
            cpu.cf = 0;
        }
        if(cpu.x == args[1]) {
            cpu.zf = 1;
        } 
        else {
            cpu.zf = 0;
        }
        cpu.neg = (((unsigned char) (cpu.x - args[1])) >> 7);

        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    //CPY imm: compare y and imm and set flags
    else if(opcode == 0xC0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        //set flags
        if(cpu.y >= args[1]) {
            cpu.cf = 1;
        }
        else {
            cpu.cf = 0;
        }
        if(cpu.y == args[1]) {
            cpu.zf = 1;
        } 
        else {
            cpu.zf = 0;
        }
        cpu.neg = (((unsigned char) (cpu.y - args[1])) >> 7);

        cpu.pc += 2;
        cpu_cycle +=2; 
        return 0;
    }

    /*
     *
     * -------------------------- ABS -------------------------- 
     *
     */

    //JMP abs: jump to absolute address
    else if(opcode == 0x4C) {
        unsigned char args[3] = {mem[cpu.pc], mem[cpu.pc+1], mem[cpu.pc+2]};
        print_cpu(args, 3);
        unsigned short old_pc = cpu.pc;
        //set next pc to address
        cpu.pc = mem[old_pc + 2];
        cpu.pc <<= 8;
        cpu.pc |= mem[old_pc + 1];
        cpu_cycle += 3;
        return 0;
    }

    //JSR abs: jump to abs address, save current pc
    else if(opcode == 0x20) {
        unsigned char args[3] = {mem[cpu.pc], mem[cpu.pc+1], mem[cpu.pc+2]};
        print_cpu(args, 3);
        //push current pc to stack
        push_word(cpu.pc + 2); 
        //set pc to abs 
        cpu.pc = args[2] << 8;
        cpu.pc |= args[1];
        cpu_cycle += 6;
        return 0;
    }

    /*
     *
     * -------------------------- REL -------------------------- 
     *
     */

    //BCS rel: branch if carry bit is set
    else if(opcode == 0xB0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.cf == 1) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BCC rel: branch if carry bit is cleared
    else if(opcode == 0x90) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.cf == 0) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BEQ rel: branch if zero flag is set
    else if(opcode == 0xF0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.zf == 1) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }
    
    //BNE rel: branch if zero flag is clear
    else if(opcode == 0xD0) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.zf == 0) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BVS rel: branch if overflow set
    else if(opcode == 0x70) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.of == 1) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BVC rel: branch if overflow clear
    else if(opcode == 0x50) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.of == 0) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BPL rel: branch if negative flag is cleared
    else if(opcode == 0x10) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.neg == 0) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    //BMI rel: branch if negative flag is set
    else if(opcode == 0x30) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);

        if(cpu.neg == 1) {
            cpu.pc += mem[cpu.pc+1];
            //3 or 4 cycles depending on page boundary
            if(same_page(cpu.pc, cpu.pc + mem[cpu.pc+1])) {
                cpu_cycle += 3;
            }
            else {
                cpu_cycle += 4;
            }
        }
        //not branching
        else {
            cpu_cycle += 2;
        }
        cpu.pc += 2;
        return 0;
    }

    /*
     *
     * -------------------------- ZPG -------------------------- 
     *
     */

    //STX zpg: store value in register x at zeropage address
    else if(opcode == 0x86) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        unsigned char addr = mem[cpu.pc+1];
        //store x 
        mem[addr] = cpu.x;
        cpu.pc += 2;
        cpu_cycle += 3;
        return 0;
    }

    //STA zpg: store value in register acc at zeropage address
    else if(opcode == 0x85) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        unsigned char addr = mem[cpu.pc+1];
        //store acc 
        mem[addr] = cpu.acc;

        cpu.pc += 2;
        cpu_cycle += 3;
        return 0;
    }

    //BIT zpg: read mem value at zpg address and set status flags and also logical and register acc and mem value to set zf
    else if(opcode == 0x24) {
        unsigned char args[2] = {mem[cpu.pc], mem[cpu.pc+1]};
        print_cpu(args, 2);
        unsigned char addr = mem[cpu.pc+1];
        //set zf if result of AND is zero 
        if((cpu.acc & mem[addr]) == 0) {
            cpu.zf = 1;
        }
        else {
            cpu.zf = 0;
        }
        //set status flags based on bits at mem location
        cpu.neg = mem[addr] >> 7;
        cpu.of = (mem[addr] >> 6) & 1;

        cpu.pc += 2;
        cpu_cycle += 3;
        return 0;
    }
    cpu.pc += 1; 
    return 0;
}
