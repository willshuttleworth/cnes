main: main.c ${PWD}/cpu/cpu.o ${PWD}/ppu/ppu.o parser.o
	gcc -Wall -g -o main main.c ${PWD}/cpu/cpu.o ${PWD}/ppu/ppu.o parser.o

/cpu/cpu.o: ${PWD}/cpu/cpu.c ${PWD}/cpu/cpu.h
	gcc -Wall -c -g ${PWD}/cpu/cpu.c

./ppu/ppu.o: ${PWD}/ppu/ppu.c ${PWD}/ppu/ppu.h
	gcc -Wall -c -g ${PWD}/ppu/ppu.c

parser.o: parser.c
	gcc -Wall -g -c parser.c
