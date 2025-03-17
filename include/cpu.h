void cpu_setup(unsigned char *oam, int *nmi);
int exec_instr();
void stack_test();

typedef struct CPU {
    unsigned char acc;
    unsigned char x;
    unsigned char y;
    unsigned short pc;
    short sp; 

    //processor status
    char cf;
    char zf;
    char id;
    char dm;
    char brk;
    char of;
    char neg;

    //copy of oam for dma
    unsigned char *oam;
    int *nmi;
    int cycle;
} CPU;

