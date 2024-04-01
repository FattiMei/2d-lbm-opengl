#include <stdio.h>
#include <stdlib.h>
#include "glad.h"
#include "window.h"
#include "lbm.h"
#include "render.h"


#define DEFAULT_WINDOW_WIDTH  800
#define DEFAULT_WINDOW_HEIGHT 600


bool paused = false;


int main(int argc, char *argv[]) {
	FILE *in = NULL;
	int frames = 0;


	if (argc == 1) {
		fprintf(stderr, "Need an input file as command-line argument\n");
		exit(EXIT_FAILURE);
	}


	if ((in = fopen(argv[1], "r")) == NULL) {
		fprintf(stderr, "Could not open input file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}


	if (window_init("lbm on opengl", DEFAULT_WINDOW_WIDTH, DEFAULT_WINDOW_HEIGHT) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}


	// @TODO: maybe the loading of context could be moved inside the window module
	gladLoadGL(glfwGetProcAddress);


	lbm_init(in);
	render_init();
	window_set_callbacks();


	double last_time = glfwGetTime();

	while (!window_should_close()) {
		double current_time = glfwGetTime();
		++frames;

		if (current_time - last_time >= 1.0) {
			printf("%f ms/frame\n", (current_time - last_time) / ((double) frames));

			frames = 0;
			last_time += 1.0;
		}

		if (!paused) {
			lbm_step();
			lbm_write_on_texture();
		}

		render_present();

		window_swap_buffers();
		window_poll_events();
	}

	lbm_close();
	window_close();
	return 0;
}
