CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lGL


sources  = $(wildcard src/*.c)
objects  = $(patsubst src/%.c,build/%.o,$(sources))
targets  = $(objects)


all: serial parallel


serial: build/render.o build/glad.o build/lbm.o build/main.o build/shader.o build/texture.o build/window.o
	$(CC) $(OPTFLAGS) $(CONFIG) -o $@ $^ $(LIBS)


parallel: build/render.o build/glad.o build/gpu.o build/main.o build/shader.o build/texture.o build/window.o
	$(CC) $(OPTFLAGS) $(CONFIG) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OPTFLAGS) $(CONFIG) -o $@ $^


run: parallel
	./$^ data/input.txt output.bin


.PHONY clean:
clean:
	rm -f $(targets) serial parallel
