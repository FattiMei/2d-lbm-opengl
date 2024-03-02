#include <stdio.h>
#include <stdlib.h>
#define GLAD_GL_IMPLEMENTATION
#include "glad.h"
#include "window.h"
#include "lbm.h"
#include "experiment.h"


bool paused = false;


int main(int argc, char *argv[]) {
	int window_width = 800;
	int window_height = 600;


	if (argc != 3) {
		fprintf(stderr, "Invalid command line arguments\n");
		fprintf(stderr, "Usage: ./headless <input filename> <binary output filename>\n");
		exit(EXIT_FAILURE);
	}


	FILE *in  = fopen(argv[1], "r");
	FILE *out = fopen(argv[2], "w");


	if (in == NULL) {
		fprintf(stderr, "Could not open input file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}


	if (out == NULL) {
		fprintf(stderr, "Could not open output file %s\n", argv[2]);
		fclose(in);
		exit(EXIT_FAILURE);
	}


	lbm_init(in);


	if (window_init("lbm on opengl", window_width, window_height) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}


	gladLoadGL(glfwGetProcAddress);


	experiment_init(window_width, window_height);
	window_set_callbacks();


	while (!window_should_close()) {
		if (!paused) {
			lbm_step();
		}

		experiment_render();

		window_swap_buffers();
		window_poll_events();
	}

	window_close();
	return 0;
}
