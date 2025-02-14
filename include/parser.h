int parse_blocks(FILE *file);
unsigned char *parse_instructions(FILE *file);
void load_ppu(unsigned char *instructions, unsigned char *chrom);
int parse_opcode(unsigned char opcode);
