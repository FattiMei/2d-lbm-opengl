#include <stdio.h>
#include <stdlib.h>
#include "window.h"


void experiment_init(int width, int height) {
	glViewport(0, 0, width, height);
}


void experiment_render() {
	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);
}


void experiment_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


int main(int argc, char *argv[]) {
	int width = 800;
	int height = 600;

	if (window_init("lbm on opengl", width, height) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}

	experiment_init(width, height);
	window_set_callbacks();

	while (!window_should_close()) {
		experiment_render();

		window_swap_buffers();
		window_poll_events();
	}

	window_close();
	return 0;
}
