main: main.c cpu/cpu.o ppu/ppu.o
	gcc -Wall -g -o main main.c cpu/cpu.o ppu/ppu.o

cpu/cpu.o: cpu/cpu.c
	gcc -Wall -c -g cpu/cpu.c

ppu/ppu.o: ppu/ppu.c
	gcc -Wall -c -g ppu/ppu.c
