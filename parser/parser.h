int parse_blocks(FILE *file);
unsigned char *parse_instructions(FILE *file);
void load(unsigned char *instructions, unsigned char *mem, int num_blocks);
