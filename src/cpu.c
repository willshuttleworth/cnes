#include <stdio.h>
#include <stdlib.h>
#include "parser.h"
#include "bus.h"

#define STACK_TOP 0x1FF
#define PAGE_SIZE 100

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

    //copy of oam for dma
    unsigned char *oam;

    int *nmi;
}CPU;

//stack is 0x100 to 0x1FF, next stack address is mem[STACK_TOP - stack_ptr]
//stack grows down from STACK_TOP(0x1FF)
int cpu_cycle;

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
            .oam = NULL,
            .nmi = NULL,
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
    printf("%04X  ", cpu.pc);
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
    if(address1 >> 8 == address2 >> 8) {
        return 1;
    } 
    else {
        return 0;
    }
}

void branch(char offset) {
    //3 or 4 cycles depending on page boundary
    if(same_page(cpu.pc + 2, cpu.pc + 2 + offset)) {
        cpu_cycle += 3;
    }
    else {
        cpu_cycle += 4;
    }
    cpu.pc = (short) cpu.pc;
    cpu.pc += (char) offset;
    cpu.pc = (short) cpu.pc;
    return;
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
    cpu.neg = cpu.acc >> 7;
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
    cpu.neg = cpu.x >> 7;
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
    cpu.neg = cpu.y >> 7;
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
    bus_write(cpu.sp + 0x100, byte);
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
    
    cpu.sp += 1;
    return bus_read(cpu.sp + 0x100);
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
    for(short i = 0xFF; i > 0; i--) {
        printf("%x ", bus_read(0x100 + i));
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
        printf("popped %x\n", pop());
        print_stack();
    }
    //pop(); //should underflow
    push_word(0x1234);
    print_stack();
    printf("%x\n", pop_word());
    print_stack();
}

void cpu_setup(unsigned char *oam, int *nmi) {
    cpu.oam = oam;
    cpu.nmi = nmi;
    //cpu_cycle = 0;
    push(0);
    push(0);
    //take 7 cycles to set pc to value in mem[0xFFFC/D]
    cpu.pc = bus_read(0xFFFD);
    cpu.pc <<= 8;
    cpu.pc |= bus_read(0xFFFC);
    cpu_cycle = 7;
}

//BRK impl: generates interrupt
void brk() {
    //technically a 2 byte instruction, so next pc is pc+2
    cpu.pc += 2;
    cpu.brk = 1;

    //push status reg, pc lo byte, pc hi byte onto stack (in that order)
    /*
    push((unsigned char)cpu.pc); 
    push((unsigned char)(cpu.pc >> 8)); 
    */
    push_word(cpu.pc + 2);
    push(encode_status() | 0x10);
    cpu.pc = bus_read(0xFFFF);
    cpu.pc <<= 8;
    cpu.pc |= bus_read(0xFFFE);
    cpu_cycle += 7;
    cpu.brk = 0; 
    return;
}

//RTI: return from interrupt
void rti() {
    decode_status(pop());

    cpu.pc = pop_word();
    cpu_cycle += 6;
}

//CLC impl: set carry flag to zero
void clc() {
    cpu.cf = 0;
    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//SEC impl: set carry flag
void sec() {
    cpu.cf = 1;

    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//NOP
void nop() {
    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//RTS: return from subroutine
void rts() {
    cpu.pc = pop_word() + 1;
    cpu_cycle += 6;
    if(cpu.pc == 0x01) {
        exit(0);
    }
    return;
}

//SEI: set id
void sei() {
    cpu.id = 1; 
    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//SED: set df
void sed() {
    cpu.dm = 1; 
    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//CLD: clear df
void cld() {
    cpu.dm = 0; 
    cpu_cycle += 2;
    cpu.pc += 1;
    return;
}

//PHP: push status to stack
void php() {
    //bits 4 and 5 are always pushed as 1s
    push(encode_status() | 0x10);
    cpu_cycle += 3;
    cpu.pc += 1;
    return;
}

//PLA: sets acc to stack value
void pla() {
    cpu.acc = pop();
    set_flags_a();
    cpu.pc += 1;
    cpu_cycle += 4;
    return;
}

//PHA: push acc onto stack
void pha() {
    push(cpu.acc);
    cpu.pc += 1;
    cpu_cycle += 3;
    return;
}

//PLP: set processor status from stack
void plp() {
    decode_status(pop());
    cpu.pc += 1;
    cpu_cycle += 4;
    return;
}

//CLV: clear overflow flag
void clv() {
    cpu.of = 0;
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//INX: increment x
void inx() {
    cpu.x += 1;
    set_flags_x();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//INY: increment y
void iny() {
    cpu.y += 1;
    set_flags_y();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TAX: transfer acc to x
void tax() {
    cpu.x = cpu.acc;
    set_flags_x();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TAY: transfer acc to y
void tay() {
    cpu.y = cpu.acc;
    set_flags_y();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TYA: transfer y to acc
void tya() {
    cpu.acc = cpu.y;
    set_flags_a();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TXA: transfer x to acc
void txa() {
    cpu.acc = cpu.x;
    set_flags_a();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TXS: transfer x to sp
void txs() {
    cpu.sp = cpu.x;
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//TSX: transfer sp to x
void tsx() {
    cpu.x = cpu.sp;
    set_flags_x();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//DEX: decrement x
void dex() {
    cpu.x -= 1;
    set_flags_x();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//DEY: decrement y
void dey() {
    cpu.y -= 1;
    set_flags_y();
    cpu.pc += 1;
    cpu_cycle += 2;
    return;
}

//LDX: load register x with data
void ldx(unsigned char data) {
    //load x with imm value
    cpu.x = data;
    set_flags_x();
    return;
}

//LDY: load register y with data
void ldy(unsigned char data) {
    //load y with imm value
    cpu.y = data;
    set_flags_y();
    return;
}

//LDA: load register acc with data
void lda(unsigned char data) {
    //load a with imm value
    cpu.acc = data;
    set_flags_a();
    return;
}

//AND: acc = acc & data
void and(unsigned char data) {
    //load a with data value
    cpu.acc &= data;
    set_flags_a();
    return;
}

//ORA: acc = acc | data
void ora(unsigned char data) {
    //load a with data value
    cpu.acc |= data;
    set_flags_a();
    return;
}

//EOR: acc = acc ^ data
void eor(unsigned char data) {
    //load a with data value
    cpu.acc ^= data;
    set_flags_a();
    return;
}

//ADC: add with carry
void adc(unsigned char data) {
    short result = cpu.acc + data + cpu.cf;
    //setting carry
    if(result > 0xFF) {
        cpu.cf = 1;
    }
    else {
        cpu.cf = 0;
    }
    //setting overflow
    if((cpu.acc >> 7) == (data >> 7)) {
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
    return;
}

//SBC: subtract with carry
void sbc(unsigned char data) {
    short result = cpu.acc + (unsigned char) ~data + (unsigned char) cpu.cf;
    
    cpu.cf = result > 0xFF;
    result = (unsigned char) result;
    //setting overflow
    if((cpu.acc >> 7) == ((unsigned char) (~data)) >> 7) {
        if((result >> 7) != (cpu.acc >> 7)) {
            cpu.of = cpu.acc  >> 7;
        }
        else {
            cpu.of = (~cpu.acc) >> 7;
        }
    }
    else {
        cpu.of = 0;
    }
    cpu.acc = (unsigned char) result;
    set_flags_a();
    return;
} 

//CMP: compare acc and data and set flags
void cmp(unsigned char data) {
    //set flags
    if(cpu.acc >= data) {
        cpu.cf = 1;
    }
    else {
        cpu.cf = 0;
    }
    if(cpu.acc == data) {
        cpu.zf = 1;
    } 
    else {
        cpu.zf = 0;
    }
    cpu.neg = (((unsigned char) (cpu.acc - data)) >> 7);
    return;
}

//CPX: compare x and data and set flags
void cpx(unsigned char data) {
    //set flags
    if(cpu.x >= data) {
        cpu.cf = 1;
    }
    else {
        cpu.cf = 0;
    }
    if(cpu.x == data) {
        cpu.zf = 1;
    } 
    else {
        cpu.zf = 0;
    }
    cpu.neg = (((unsigned char) (cpu.x - data)) >> 7);
    return;
}

//CPY: compare y and data and set flags
void cpy(unsigned char data) {
    //set flags
    if(cpu.y >= data) {
        cpu.cf = 1;
    }
    else {
        cpu.cf = 0;
    }
    if(cpu.y == data) {
        cpu.zf = 1;
    } 
    else {
        cpu.zf = 0;
    }
    cpu.neg = (((unsigned char) (cpu.y - data)) >> 7);
    return;
}
//JMP: jump to address
void jmp(unsigned char lo, unsigned char hi) {
    //set next pc to address
    cpu.pc = hi;
    cpu.pc <<= 8;
    cpu.pc |= lo;
    return;
}

//JSR: jump to address, save current pc
void jsr(unsigned char lo, unsigned char hi) {
    //push current pc to stack
    push_word(cpu.pc + 2); 
    //set pc to new addr
    cpu.pc = hi << 8;
    cpu.pc |= lo;
    return;
}
//BCS rel: branch if carry bit is set
void bcs(unsigned char offset) {
    if(cpu.cf == 1) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BCC rel: branch if carry bit is cleared
void bcc(unsigned char offset) {
    if(cpu.cf == 0) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BEQ rel: branch if zero flag is set
void beq(unsigned char offset) {
    if(cpu.zf == 1) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BNE rel: branch if zero flag is clear
void bne(unsigned char offset) {
    if(cpu.zf == 0) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BVS rel: branch if overflow set
void bvs(unsigned char offset) {
    if(cpu.of == 1) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BVC rel: branch if overflow clear
void bvc(unsigned char offset) {
    if(cpu.of == 0) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BPL rel: branch if negative flag is cleared
void bpl(unsigned char offset) {
    if(cpu.neg == 0) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//BMI rel: branch if negative flag is set
void bmi(unsigned char offset) {
    if(cpu.neg == 1) {
        branch(offset);
    }
    //not branching
    else {
        cpu_cycle += 2;
    }
    cpu.pc += 2;
    return;
}

//STX: store value in register x at addr
void stx(short addr) {
    //store x 
    bus_write(addr, cpu.x);
    return;
}

//STY: store value in register y at addr
void sty(short addr) {
    //store y
    bus_write(addr, cpu.y);
    return;
}

// add cycles for dma, including existing ones accounted for by the addressing mode
void dma_clock_cycles(int existing) {
    if(cpu_cycle % 2 == 0) {
       cpu_cycle += (513 - existing); 
    }
    else {
       cpu_cycle += (514 - existing); 
    }
}

//STA: store value in register acc at addr
void sta(short addr, int existing) {
    // check for OAMDMA
    if(addr == 0x4014) {
        for(int i = 0; i < 256; i++) {
            cpu.oam[i] = bus_read((cpu.acc << 8) + i);
        }
        dma_clock_cycles(existing);
    }
    else {
        bus_write(addr, cpu.acc);
    }
    return;
}

//BIT zpg: read mem value at address and set status flags and also logical and register acc and mem value to set zf
void bit(short addr) {
    //set zf if result of AND is zero 
    if((cpu.acc & bus_read(addr)) == 0) {
        cpu.zf = 1;
    }
    else {
        cpu.zf = 0;
    }
    //set status flags based on bits at mem location
    cpu.neg = bus_read(addr) >> 7;
    cpu.of = (bus_read(addr) >> 6) & 1;
    return;
}

//LSR: logical shift right
unsigned char lsr(unsigned char data) {
    cpu.cf = data & 1;
    unsigned char result = data >> 1;
    cpu.zf = result == 0;
    cpu.neg = result >> 7;
    return result;
}

//ASL: arithemtic shift left
unsigned char asl(unsigned char data) {
    cpu.cf = (data & 0x80) >> 7;
    unsigned char result = (unsigned char) data << 1;
    cpu.zf = result == 0;
    cpu.neg = result >> 7;
    return result;
}

//ROR: rotate right
unsigned char ror(unsigned char data) {
    int new_cf = data & 1;
    unsigned char rotated = data >> 1;
    if(cpu.cf == 1) {
        rotated |= 128;
    }
    cpu.zf = rotated == 0;
    cpu.neg = rotated >> 7;
    cpu.cf = new_cf;
    return rotated;
}

//ROL: rotate left
unsigned char rol(unsigned char data) {
    int new_cf = (data & 128) >> 7;
    unsigned char rotated = data << 1;
    if(cpu.cf == 1) {
        rotated |= 1;
    }
    cpu.zf = rotated == 0;
    cpu.neg = rotated >> 7;
    cpu.cf = new_cf;
    return rotated;
}

//INC: increment memory
void inc(short addr) {
    bus_write(addr, bus_read(addr) + 1);
    cpu.zf = bus_read(addr) == 0;
    cpu.neg = bus_read(addr) >> 7;
}

//DEC: decrement memory
void dec(short addr) {
    bus_write(addr, bus_read(addr) - 1);
    cpu.zf = bus_read(addr) == 0;
    cpu.neg = bus_read(addr) >> 7;
}

//LAX: load acc and x
void lax(unsigned char data) {
    lda(data);
    ldx(data);
}

//SAX: store acc AND x
void sax(short addr) {
    bus_write(addr, cpu.acc & cpu.x);
}

//DCP: dec mem and cmp
void dcp(short addr) {
    bus_write(addr, bus_read(addr) - 1);
    cmp(bus_read(addr));
}

//ISC: inc mem and sbc
void isc(short addr) {
    bus_write(addr, bus_read(addr) + 1);
    sbc(bus_read(addr));
}

//SLO: asl mem and ora
void slo(short addr) {
    bus_write(addr, asl(bus_read(addr)));
    ora(bus_read(addr));
}

//RLA: rol mem and AND
void rla(short addr) {
    bus_write(addr, rol(bus_read(addr)));
    and(bus_read(addr));
}

//SRE: lsr mem and eor
void sre(short addr) {
    bus_write(addr, lsr(bus_read(addr)));
    eor(bus_read(addr));
}

//RRA: ror mem and adc
void rra(short addr) {
    bus_write(addr, ror(bus_read(addr)));
    adc(bus_read(addr));
}

int exec_instr() {
    if(*cpu.nmi == 1) {
        puts("nmi");
        push_word(cpu.pc);
        push(encode_status() | 0x10);

        cpu.pc = bus_read(0xFFFB) << 8;
        cpu.pc |= bus_read(0xFFFA);
        *cpu.nmi = 0;
        cpu_cycle += 7;
    }
    unsigned char opcode = bus_read(cpu.pc);
    unsigned char args[3] = {opcode, bus_read(cpu.pc + 1), bus_read(cpu.pc + 2)};

    switch(parse_opcode(opcode)) {
        short addr;
        //default
        case 0:
            cpu.pc++;
            break;
        //impl
        case 1:
            print_cpu(args, 1);
            if(opcode == 0x00) {
                brk();
            }
            else if(opcode == 0x18) {
                clc();
            }
            else if(opcode == 0x38) {
                sec();
            }
            else if(opcode == 0xEA) {
                nop();
            }
            else if(opcode == 0xE8) {
                inx();
            }
            else if(opcode == 0xC8) {
                iny();
            }
            else if(opcode == 0xCA) {
                dex();
            }
            else if(opcode == 0x88) {
                dey();
            }
            else if(opcode == 0xB8) {
                clv();
            }
            else if(opcode == 0x28) {
                plp();
            }
            else if(opcode == 0x48) {
                pha();
            }
            else if(opcode == 0x68) {
                pla();
            }
            else if(opcode == 0x08) {
                php();
            }
            else if(opcode == 0xD8) {
                cld();
            }
            else if(opcode == 0xF8) {
                sed();
            }
            else if(opcode == 0x78) {
                sei();
            }
            else if(opcode == 0x60) {
                rts();
            }
            else if(opcode == 0xAA) {
                tax();
            }
            else if(opcode == 0xA8) {
                tay();
            }
            else if(opcode == 0x98) {
                tya();
            }
            else if(opcode == 0x8A) {
                txa();
            }
            else if(opcode == 0x9A) {
                txs();
            }
            else if(opcode == 0xBA) {
                tsx();
            }
            else if(opcode == 0x40) {
                rti();
            }
            else if(opcode == 0x1A) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            else if(opcode == 0x3A) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            else if(opcode == 0x5A) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            else if(opcode == 0x7A) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            else if(opcode == 0xDA) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            else if(opcode == 0xFA) {
                //nop
                cpu.pc++;
                cpu_cycle += 2;
            }
            break;
        //imm
        case 2:
            print_cpu(args, 2);
            if(opcode == 0xA2) {
                ldx(args[1]);
            }
            else if(opcode == 0xA0) {
                ldy(args[1]);
            }
            else if(opcode == 0xA9) {
                lda(args[1]);
            }
            else if(opcode == 0x29) {
                and(args[1]);
            }
            else if(opcode == 0x09) {
                ora(args[1]);
            }
            else if(opcode == 0x49) {
                eor(args[1]);
            }
            else if(opcode == 0x69) {
                adc(args[1]);
            }
            else if(opcode == 0xE9) {
                adc(~args[1]);
            }
            else if(opcode == 0xC9) {
                cmp(args[1]);
            }
            else if(opcode == 0xE0) {
                cpx(args[1]);
            }
            else if(opcode == 0xC0) {
                cpy(args[1]);
            }
            else if(opcode == 0xEB) {
                sbc(args[1]);
            }
            cpu.pc += 2;
            cpu_cycle += 2;
            break;
        //acc
        case 3:
            print_cpu(args, 1);
            if(opcode == 0x4A) {
                cpu.acc = lsr(cpu.acc);
                set_flags_a();
            }
            else if(opcode == 0x0A) {
                cpu.acc = asl(cpu.acc);
                set_flags_a();
            }
            else if(opcode == 0x6A) {
                cpu.acc = ror(cpu.acc);
                set_flags_a();
            }
            else if(opcode == 0x2A) {
                cpu.acc = rol(cpu.acc);
                set_flags_a();
            }
            cpu.pc++;
            cpu_cycle += 2;
            break;
        //abs
        case 4:
            addr = args[2];
            addr <<= 8;
            addr |= args[1];
            print_cpu(args, 3);
            if(opcode == 0x4C) {
                jmp(bus_read(cpu.pc + 1), bus_read(cpu.pc + 2));
                cpu_cycle -= 1;
                cpu.pc -= 3;
            }
            else if(opcode == 0x20) {
                jsr(bus_read(cpu.pc + 1), bus_read(cpu.pc + 2));
                cpu_cycle += 2;
                cpu.pc -= 3;
            }
            else if(opcode == 0x8E) {
                stx(addr);
            }
            else if(opcode == 0x8C) {
                sty(addr);
            }
            else if(opcode == 0x8F) {
                sax(addr);
            }
            else if(opcode == 0xEC) {
                cpx(bus_read(addr));
            }
            else if(opcode == 0xCC) {
                cpy(bus_read(addr));
            }
            else if(opcode == 0xAE) {
                ldx(bus_read(addr));
            }
            else if(opcode == 0xAC) {
                ldy(bus_read(addr));
            }
            else if(opcode == 0xAD) {
                lda(bus_read(addr));
            }
            else if(opcode == 0xAF) {
                lax(bus_read(addr));
            }
            else if(opcode == 0x8D) {
                sta(addr, 4);
            }
            else if(opcode == 0x2C) {
                bit(addr);
            }
            else if(opcode == 0x0D) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x2D) {
                and(bus_read(addr));
            }
            else if(opcode == 0x4D) {
                eor(bus_read(addr));
            }
            else if(opcode == 0xEE) {
                inc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xCE) {
                dec(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xCF) {
                dcp(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xEF) {
                isc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x6D) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xED) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xCD) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0x4E) {
                bus_write(addr, lsr(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x0E) {
                bus_write(addr, asl(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x6E) {
                bus_write(addr, ror(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x2E) {
                bus_write(addr, rol(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x0F) {
                slo(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x2F) {
                rla(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x4F) {
                sre(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x6F) {
                rra(addr);
                cpu_cycle += 2;
            }
            cpu_cycle += 4;
            cpu.pc += 3;
            break;
        //abs_x
        case 5:
            print_cpu(args, 3); 
            addr = args[2] << 8;
            addr |= args[1];
            if(!same_page(addr, addr + cpu.x)) {
                cpu_cycle += 1;
            }
            addr += cpu.x;
            if(opcode == 0xBD) {
                lda(bus_read(addr));
            }
            else if(opcode == 0x9D) {
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                    sta(addr, 5);
                }
                else {
                    sta(addr, 4);
                }
            }
            else if(opcode == 0xBC) {
                ldy(bus_read(addr));
            }
            else if(opcode == 0x1D) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x3D) {
                and(bus_read(addr));
            }
            else if(opcode == 0x5D) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x7D) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xFD) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xDD) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0x5E) {
                bus_write(addr, lsr(bus_read(addr)));
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x1E) {
                bus_write(addr, asl(bus_read(addr)));
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x7E) {
                bus_write(addr, ror(bus_read(addr)));
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x3E) {
                bus_write(addr, rol(bus_read(addr)));
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0xFE) {
                inc(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0xDE) {
                dec(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0xDF) {
                dcp(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0xFF) {
                isc(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x1F) {
                slo(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x3F) {
                rla(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x5F) {
                sre(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            else if(opcode == 0x7F) {
                rra(addr);
                cpu_cycle += 2;
                if(same_page(addr, addr - cpu.x)) {
                    cpu_cycle += 1;
                }
            }
            cpu.pc += 3;
            cpu_cycle += 4;
            break;
        //abs_y
        case 6:
            print_cpu(args, 3); 
            addr = args[2] << 8;
            addr |= args[1];
            if(!same_page(addr, addr + cpu.y)) {
                cpu_cycle += 1;
            }
            addr += cpu.y;
            if(opcode == 0xB9) {
                lda(bus_read(addr));
            }
            else if(opcode == 0xBE) {
                ldx(bus_read(addr));
            }
            else if(opcode == 0xBF) {
                lax(bus_read(addr));
            }
            else if(opcode == 0x99) {
                cpu_cycle += 1;
                sta(addr, 5);
            }
            else if(opcode == 0x19) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x39) {
                and(bus_read(addr));
            }
            else if(opcode == 0x59) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x79) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xF9) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xD9) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0xDB) {
                dcp(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xFB) {
                isc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x1B) {
                slo(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x3B) {
                rla(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x5B) {
                sre(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x7B) {
                rra(addr);
                cpu_cycle += 2;
            }
            cpu.pc += 3;
            cpu_cycle += 4;
            break;
        //ind
        case 7:
            print_cpu(args, 3);
            addr = (args[2] << 8) | args[1];
            short addr_hi = (args[2] << 8) | args[1];
            short addr_lo = (args[2] << 8) | (unsigned char)(args[1] + 1);
            //printf("addr: %x\thi: %x\tlo: %x\n", addr, addr_hi, addr_lo);
            if(opcode == 0x6C) {
                jmp(bus_read(addr_hi), bus_read(addr_lo));
            }
            cpu_cycle += 5;
            break;
        //x_ind
        case 8:
            print_cpu(args, 2);
            addr = bus_read((cpu.x + args[1] + 1) % 256) << 8;
            addr |= bus_read((cpu.x + args[1]) % 256);
            if(opcode == 0xA1) {
                lda(bus_read(addr));
            }
            else if(opcode == 0x81) {
                sta(addr, 6);
            }
            else if(opcode == 0x83) {
                sax(addr);
            }
            else if(opcode == 0x01) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x21) {
                and(bus_read(addr));
            }
            else if(opcode == 0x41) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x61) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xE1) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xC1) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0xA3) {
                lax(bus_read(addr));
            }
            else if(opcode == 0xC3) {
                dcp(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xE3) {
                isc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x03) {
                slo(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x23) {
                rla(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x43) {
                sre(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x63) {
                rra(addr);
                cpu_cycle += 2;
            }
            cpu.pc += 2;
            cpu_cycle += 6;
            break;
        //ind_y
        case 9:
            print_cpu(args, 2);
            int int_addr = bus_read((args[1] + 1) % 256) << 8;
            int_addr |= bus_read(args[1]);
            if(!same_page(int_addr, int_addr + cpu.y)) {
                cpu_cycle += 1;
            }
            int_addr += cpu.y;
            addr = (short) int_addr;
            if(opcode == 0xB1) {
                lda(bus_read(addr));
            }
            if(opcode == 0x91) {
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                    sta(addr, 6);
                }
                else {
                    sta(addr, 5);
                }
            }
            else if(opcode == 0x11) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x31) {
                and(bus_read(addr));
            }
            else if(opcode == 0x51) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x71) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xF1) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xD1) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0xB3) {
                lax(bus_read(addr));
            }
            else if(opcode == 0xD3) {
                dcp(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            else if(opcode == 0xF3) {
                isc(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            else if(opcode == 0x13) {
                slo(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            else if(opcode == 0x33) {
                rla(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            else if(opcode == 0x53) {
                sre(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            else if(opcode == 0x73) {
                rra(addr);
                if(same_page(int_addr, int_addr - cpu.y)) {
                    cpu_cycle += 1;
                }
                cpu_cycle += 2;
            }
            cpu_cycle += 5;
            cpu.pc += 2;
            break;
        //rel
        case 10:
            print_cpu(args, 2);
            if(opcode == 0xB0) {
                bcs(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0x90) {
                bcc(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0xF0) {
                beq(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0xD0) {
                bne(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0x70) {
                bvs(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0x50) {
                bvc(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0x10) {
                bpl(bus_read(cpu.pc + 1));
            }
            else if(opcode == 0x30) {
                bmi(bus_read(cpu.pc + 1));
            }
            break;
        //zpg
        case 11:
            print_cpu(args, 2);
            addr = (short) args[1];
            if(opcode == 0x86) {
                stx(addr);
            }
            else if(opcode == 0x84) {
                sty(addr);
            }
            else if(opcode == 0x85) {
                sta(addr, 3);
            }
            else if(opcode == 0x87) {
                sax(addr);
            }
            else if(opcode == 0x24) {
                bit(addr);
            }
            else if(opcode == 0xA5) {
                lda(bus_read(addr));
            }
            else if(opcode == 0xA6) {
                ldx(bus_read(addr));
            }
            else if(opcode == 0xA4) {
                ldy(bus_read(addr));
            }
            else if(opcode == 0xA7) {
                lax(bus_read(addr));
            }
            else if(opcode == 0x05) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x45) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x25) {
                and(bus_read(addr));
            }
            else if(opcode == 0x65) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xE5) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xC5) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0xC7) {
                dcp(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xE7) {
                isc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xE4) {
                cpx(bus_read(addr));
            }
            else if(opcode == 0xC4) {
                cpy(bus_read(addr));
            }
            else if(opcode == 0x46) {
                bus_write(addr, lsr(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0xE6) {
                inc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xC6) {
                dec(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x06) {
                bus_write(addr, asl(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x66) {
                bus_write(addr, ror(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x26) {
                bus_write(addr, rol(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x07) {
                slo(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x27) {
                rla(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x47) {
                sre(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x67) {
                rra(addr);
                cpu_cycle += 2;
            }
            cpu.pc += 2;
            cpu_cycle += 3;
            break;
        //zpg_x
        case 12:
            print_cpu(args, 2);
            addr = (unsigned char) (cpu.x + args[1]);
            if(opcode == 0xB4) {
                ldy(bus_read(addr));
            }
            else if(opcode == 0xB5) {
                lda(bus_read(addr));
            }
            else if(opcode == 0x94) {
                sty(addr);
            }
            else if(opcode == 0x95) {
                sta(addr, 4);
            }
            else if(opcode == 0x15) {
                ora(bus_read(addr));
            }
            else if(opcode == 0x35) {
                and(bus_read(addr));
            }
            else if(opcode == 0x55) {
                eor(bus_read(addr));
            }
            else if(opcode == 0x75) {
                adc(bus_read(addr));
            }
            else if(opcode == 0xF5) {
                sbc(bus_read(addr));
            }
            else if(opcode == 0xD5) {
                cmp(bus_read(addr));
            }
            else if(opcode == 0xD7) {
                dcp(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xF7) {
                isc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x56) {
                bus_write(addr, lsr(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x16) {
                bus_write(addr, asl(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x76) {
                bus_write(addr, ror(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0x36) {
                bus_write(addr, rol(bus_read(addr)));
                cpu_cycle += 2;
            }
            else if(opcode == 0xF6) {
                inc(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0xD6) {
                dec(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x17) {
                slo(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x37) {
                rla(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x57) {
                sre(addr);
                cpu_cycle += 2;
            }
            else if(opcode == 0x77) {
                rra(addr);
                cpu_cycle += 2;
            }
            cpu.pc += 2;
            cpu_cycle += 4;
            break;
        //zpg_y
        case 13:
            print_cpu(args, 2);
            addr = (unsigned char) (cpu.y + args[1]);
            if(opcode == 0xB6) {
                ldx(bus_read(addr));
            }
            else if(opcode == 0xB7) {
                lax(bus_read(addr));
            }
            else if(opcode == 0x96) {
                stx(addr);
            }
            else if(opcode == 0x97) {
                sax(addr);
            }
            cpu.pc += 2;
            cpu_cycle += 4;
            break;
    }
    return cpu_cycle;
}
