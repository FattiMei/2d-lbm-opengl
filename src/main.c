#include <stdio.h>
#include <stdlib.h>
#include "window.h"
#include "lbm.h"
#include "experiment.h"


#define FRAME_COUNT 100


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


	experiment_init(window_width, window_height);
	window_set_callbacks();

	int frames = 0;
	double before = glfwGetTime();

	while (!window_should_close()) {
		if (!paused) {
			lbm_step();
		}

		experiment_render();

		window_swap_buffers();
		window_poll_events();


		++frames;
		if (frames == FRAME_COUNT) {
			double now = glfwGetTime();
			double delta = now - before;

			printf("Generated %d frames in %.3f seconds, FPS = %.1f\n", FRAME_COUNT, delta, ((double) FRAME_COUNT) / delta);
			before = now;


			frames = 0;
		}
	}

	window_close();
	return 0;
}
