#include <stdio.h>
#include <stdlib.h>
#include "glad.h"
#include "window.h"
#include "lbm.h"
#include "render.h"


bool paused = false;


int main(int argc, char *argv[]) {
	int window_width = 800;
	int window_height = 600;


	if (argc == 1) {
		fprintf(stderr, "Need an input file as command-line argument\n");
		exit(EXIT_FAILURE);
	}


	FILE *in  = fopen(argv[1], "r");


	if (in == NULL) {
		fprintf(stderr, "Could not open input file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}


	if (window_init("lbm on opengl", window_width, window_height) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}


	gladLoadGL(glfwGetProcAddress);


	lbm_init(in);
	render_init(window_width, window_height);
	window_set_callbacks();


	while (!window_should_close()) {
		if (!paused) {
			lbm_step();
		}

		lbm_write_on_texture();
		render_render();

		window_swap_buffers();
		window_poll_events();
	}

	window_close();
	return 0;
}
