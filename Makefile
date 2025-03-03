TARGET = cnes 
CC = gcc 
CFLAGS = -Wall
OUTDIR = .
DIR_OBJ = ./obj

# SDL2 configuration
CFLAGS += $(shell pkg-config --cflags sdl2)
LDFLAGS += $(shell pkg-config --libs sdl2)

# Properly capture all .c files in src directory
SRCS = $(wildcard src/*.c)
# Create object file names by replacing src/ with obj/ and .c with .o
OBJS = $(SRCS:src/%.c=$(DIR_OBJ)/%.o)

# Include directory
INCS = include
INC_DIRS = -I$(INCS)

.PHONY: all clean echoes

all: $(TARGET)

$(TARGET): $(OBJS)
	$(CC) -o $(OUTDIR)/$@ $(OBJS) $(CFLAGS) $(LDFLAGS)

$(DIR_OBJ)/%.o: src/%.c
	@mkdir -p $(DIR_OBJ)
	$(CC) $(CFLAGS) $(INC_DIRS) -c $< -o $@

clean:
	rm -rf $(DIR_OBJ) $(TARGET)

echoes:
	@echo "INC files: $(INCS)"  
	@echo "SRC files: $(SRCS)"
	@echo "OBJ files: $(OBJS)"
