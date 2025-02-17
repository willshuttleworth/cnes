char *parse_blocks(FILE *file);
void parse_instructions(FILE *file, unsigned char *rom, unsigned char *chrom, int prg_blocks, int chr_blocks);
int parse_opcode(unsigned char opcode);
