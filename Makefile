CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lEGL -lGL -ldl


sources  = $(wildcard src/*.c)
examples = $(wildcard examples/*.c)
objects  = $(patsubst src/%.c,build/%.o,$(sources))


targets  += $(patsubst %.c,build/%,$(examples))
targets  += $(objects)
targets  += $(imgui_objects)


all: serial headless


serial: build/main.o build/window.o
	$(CC) $(CCFLAGS) $(OPTFLAGS) -o $@ $^ $(LIBS)


headless: build/headless.o
	$(CC) $(CCFLAGS) $(OPTFLAGS) -o $@ $^ -lm


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OPTFLAGS) -o $@ $^


output.bin: headless
	./headless data/input.txt $@


run: serial
	./serial data/input.txt output.bin


test: headless output.bin
	python3.8 compare.py reference.bin output.bin


.PHONY folder:
folder:
	mkdir -p build


.PHONY clean:
clean:
	rm -f $(targets) serial output.bin
