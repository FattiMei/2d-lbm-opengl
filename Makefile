CC       = gcc
CCFLAGS  = -Wall -Wextra -Wpedantic
OMPFLAGS = -fopenmp
INCLUDE  = -I ./include
LIBS     = -lm -lglfw -lGL


serial_src   = src/glad.c src/window.c src/texture.c src/shader.c src/render.c src/lbm.c src/main.c
serial_obj   = $(patsubst src/%.c,build/%.o,$(serial_src))


parallel_src = src/glad.c src/window.c src/texture.c src/shader.c src/render.c src/gpu.c src/main.c
parallel_obj = $(patsubst src/%.c,build/%.o,$(parallel_src))


all: serial parallel


serial: $(serial_obj)
	$(CC) $(OMPFLAGS) -o $@ $^ $(LIBS)


parallel: $(parallel_obj)
	$(CC) $(OMPFLAGS) -o $@ $^ $(LIBS)


build/%.o: src/%.c
	$(CC) -c $(INCLUDE) $(CCFLAGS) $(OMPFLAGS) $(CONFIG) -o $@ $^


release: $(serial_src)
	$(CC) $(INCLUDE) $(CCFLAGS) $(OMPFLAGS) -O2 -o $@ $^ $(LIBS)


run: parallel
	./$^ data/input.txt output.bin


.PHONY: clean
clean:
	rm -f $(serial_obj) $(parallel_obj) serial parallel release
