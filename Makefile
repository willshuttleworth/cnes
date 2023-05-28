main: main.c cpu.o ppu.o parser.o
	gcc -Wall -g -o main main.c cpu.o ppu.o parser.o

parser.o: parser.c
	gcc -Wall -g -c parser.c

cpu/cpu.o: cpu/cpu.c
	gcc -Wall -c -g cpu/cpu.c

ppu/ppu.o: ppu/ppu.c
	gcc -Wall -c -g ppu/ppu.c
