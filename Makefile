CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lGL -ldl


sources  = $(wildcard src/*.c)
examples = $(wildcard examples/*.c)
objects  = $(patsubst src/%.c,build/%.o,$(sources))


targets  += $(patsubst %.c,build/%,$(examples))
targets  += $(objects)
targets  += $(imgui_objects)


all: serial headless


serial: build/main.o build/window.o build/experiment.o build/lbm.o build/shader.o build/texture.o build/glad.o
	$(CC) $(OPTFLAGS) $(CONFIG) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OPTFLAGS) $(CONFIG) -o $@ $^


run: serial
	./serial data/input.txt output.bin


.PHONY folder:
folder:
	mkdir -p build


.PHONY clean:
clean:
	rm -f $(targets) serial output.bin
