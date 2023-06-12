main: main.c cpu.o ppu.o parser.o
	gcc -Wall -g -o main main.c cpu.o ppu.o parser.o

cpu.o: cpu.c cpu.h
	gcc -Wall -c -g cpu.c

ppu.o: ppu.c ppu.h
	gcc -Wall -c -g ppu.c

parser.o: parser.c
	gcc -Wall -g -c parser.c
