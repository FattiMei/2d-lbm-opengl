#include "render.h"
#include "shader.h"
#include "texture.h"
#include "lbm.h"
#include "glad.h"


static unsigned int render_program;
static unsigned int VAO;


// to be called after initializing lbm
void render_init() {
	render_program = program_load_from_file("shaders/vs_quad.glsl", "shaders/fs_texture_apply.glsl");

	glGenVertexArrays(1, &VAO);
	glBindVertexArray(VAO);
}


void render_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


void render_present() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);

	glUseProgram(render_program);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
}
