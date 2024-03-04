CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lGL


sources  = $(wildcard src/*.c)
objects  = $(patsubst src/%.c,build/%.o,$(sources))
targets  = $(objects)


all: serial


serial: $(targets)
	$(CC) $(OPTFLAGS) $(CONFIG) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OPTFLAGS) $(CONFIG) -o $@ $^


run: serial
	./$^ data/input.txt output.bin


.PHONY folder:
folder:
	mkdir -p build


.PHONY clean:
clean:
	rm -f $(targets) serial output.bin
