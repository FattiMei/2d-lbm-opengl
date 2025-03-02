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
		fprintf(stderr, "Usage: %s <input_file>\n", argv[0]);
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
	fprintf(stderr, "%s\n%s\n", glGetString(GL_VENDOR), glGetString(GL_RENDERER));


	lbm_init(in);
	render_init();
	window_set_callbacks();


	double last_time = glfwGetTime();

	while (!window_should_close()) {
		const double current_time = glfwGetTime();
		const double delta_t = current_time - last_time;

		if (current_time - last_time >= 1.0) {
			printf(
				"%f ms/frame (%.1f FPS)\n",
				1000.0 * delta_t / ((double) frames),
				((double) frames) / delta_t
			);

			frames = 0;
			last_time = current_time;
		}

		if (!paused) {
			lbm_step();
			lbm_write_on_texture();
		}

		render_present();

		window_swap_buffers();
		window_poll_events();
		++frames;
	}

	lbm_close();
	window_close();
	return 0;
}
