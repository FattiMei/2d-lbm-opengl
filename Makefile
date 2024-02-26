CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lEGL -lGL

# @TODO: find the default installed version of python
PYTHON   = python3.8


sources  = $(wildcard src/*.c)
examples = $(wildcard examples/*.c)

objects  = $(patsubst src/%.c,build/%.o,$(sources))

targets  += $(patsubst %.c,build/%,$(examples))
targets  += $(objects)


all: serial


serial: $(targets)
	$(CC) $(CCFLAGS) $(OPTFLAGS) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OPTFLAGS) -o $@ $^


output.bin: serial
	./serial data/input.txt $@


run: serial
	./serial data/input.txt output.bin


.PHONY folder:
folder:
	mkdir -p build


.PHONY clean:
clean:
	rm -f $(targets) serial output.bin
