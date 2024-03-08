#include "render.h"
#include "shader.h"
#include "texture.h"
#include "lbm.h"
#include "glad.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>


static unsigned int render_program;
static unsigned int VAO;


static const char* vertex_shader_src = R"(
	#version 430 core
	layout (location = 0) in vec2 aPos;
	layout (location = 1) in vec2 aTexCoord;

	out vec2 texCoord;

	void main() {
		vec2 uv;
		uv.x = (gl_VertexID & 1);
		uv.y = ((gl_VertexID >> 1) & 1);

		gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
		texCoord = uv;
	}
)";


static const char* fragment_shader_src = R"(
	#version 430 core
	in vec2 texCoord;
	out vec4 FragColor;

	uniform sampler2D tex;

	void main() {
		FragColor = texture(tex, texCoord);
	}
)";


// to be called after initializing lbm
void render_init() {
	render_program = program_load(vertex_shader_src, fragment_shader_src);

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
