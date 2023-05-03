main: main.c cpu.o ppu.o
	gcc -Wall -g -o main main.c cpu.o ppu.o

cpu.o: cpu.c
	gcc -Wall -c cpu.c

ppu.o: ppu.c
	gcc -Wall -c ppu.c
