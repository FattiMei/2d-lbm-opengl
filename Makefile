CC       = gcc
CCFLAGS  = -Wall -Wextra -Wpedantic
OMPFLAGS = -fopenmp
OPTFLAGS = -O2
INCLUDE  = -I ./include
LIBS     = -lm -lglfw


serial_src   = src/glad.c src/window.c src/texture.c src/shader.c src/render.c src/lbm.c src/main.c
parallel_src = src/glad.c src/window.c src/texture.c src/shader.c src/render.c src/gpu.c src/main.c


target = parallel serial


all: $(target)


serial: $(serial_src)
	$(CC) $(CCFLAGS) $(OMPFLAGS) $(OPTFLAGS) -DUSE_GLES2 $(INCLUDE) -o $@ $^ $(LIBS) -lEGL -lGL


parallel: $(parallel_src)
	$(CC) $(CCFLAGS) $(INCLUDE) $(OPTFLAGS) -o $@ $^ $(LIBS)


run: parallel
	./$^ data/input.txt output.bin


.PHONY: clean
clean:
	rm -f $(serial_obj) $(parallel_obj) serial parallel
