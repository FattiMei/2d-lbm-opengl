#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "lbm.h"
#include "window.h"
#include "experiment.h"


int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Invalid command line arguments\n");
		fprintf(stderr, "Usage: serial <input filename> <output filename>\n");

		return -1;
	}


	const char *input_filename = argv[1];
	const char *output_filename = argv[2];


	FILE *in  = fopen(input_filename,  "r");
	FILE *out = fopen(output_filename, "w");


	if (in == NULL) {
		fprintf(stderr, "Could not open input file %s\n", input_filename);
		return 1;
	}

	if (out == NULL) {
		fprintf(stderr, "Could not open output file %s\n", output_filename);
		return 1;
	}


	lbm_setup(in);


	int window_width = 800;
	int window_height = 600;

	if (window_init("lbm on opengl", window_width, window_height) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}

	experiment_init(window_width, window_height);
	window_set_callbacks();

	int it = 0;
	while (!window_should_close()) {
		experiment_render();

		for (int comp_step = 0; comp_step < 5; ++comp_step) {
			lbm_step(it);
			++it;
		}

		window_swap_buffers();
		window_poll_events();
	}

	window_close();
	fclose(out);

	return 0;
}
