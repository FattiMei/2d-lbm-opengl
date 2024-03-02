CC       = gcc
CCFLAGS  = -Wall
OPTFLAGS = -O2 -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lEGL -lGL -ldl


sources  = $(wildcard src/*.c)
examples = $(wildcard examples/*.c)
objects  = $(patsubst src/%.c,build/%.o,$(sources))

imgui_sources = $(wildcard imgui/*.cpp)
imgui_objects = $(patsubst imgui/%.cpp,build/imgui/%.o,$(imgui_sources))

targets  += $(patsubst %.c,build/%,$(examples))
targets  += $(objects)
targets  += $(imgui_objects)


all: serial


serial: build/main.o build/window.o
	$(CC) $(CCFLAGS) $(OPTFLAGS) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(IMGUI_INCLUDE) $(CCFLAGS) $(OPTFLAGS) -o $@ $^


build/imgui/%.o: imgui/%.cpp
	$(CC) -c $(IMGUI_INCLUDE) $(CCFLAGS) $(OPTFLAGS) -o $@ $^


output.bin: serial
	./serial data/input.txt $@


run: serial
	./serial data/input.txt output.bin


test: serial output.bin
	python3.8 compare.py reference.bin output.bin


.PHONY folder:
folder:
	mkdir -p build
	mkdir -p build/imgui


.PHONY clean:
clean:
	rm -f $(targets) serial output.bin
