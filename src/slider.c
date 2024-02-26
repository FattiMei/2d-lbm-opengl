#include "slider.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <GLFW/glfw3.h>
#include "shader.h"


static GLint slider_program;
static GLint u_state;
static GLint u_sliderPosition;
static GLint u_sliderDimension;


static const GLfloat vertices[] = {
	-1.0f, -1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
	-1.0f,  1.0f, 0.0f,
	 1.0f,  1.0f, 0.0f,
	 1.0f, -1.0f, 0.0f,
};


GLchar slider_vertex_shader_src[] = R"(
	attribute vec4 vPosition;

	void main() {
		gl_Position = vPosition;
	}
)";


// @TODO: add better looks to the slider
GLchar slider_fragment_shader_src[] = R"(
	precision mediump float;

	uniform float state;
	uniform vec2 sliderPosition;
	uniform vec2 sliderDimension;

	#define BACKGROUND_COLOR vec4(0.1, 0.1, 0.1, 1.0)
	#define FOREGROUND_COLOR vec4(0.5, 0.5, 0.5, 1.0)

	void main() {
		float cutoff = sliderPosition.x + state * sliderDimension.x;

		if (gl_FragCoord.x < cutoff) {
			gl_FragColor = FOREGROUND_COLOR;
		}
		else {
			gl_FragColor = BACKGROUND_COLOR;
		}
	}
)";


void slider_init() {
	slider_program = program_load(slider_vertex_shader_src, slider_fragment_shader_src);

	// @TODO: is there a way to not send vertices positions in GLES2 when drawing the entire viewport?
	glBindAttribLocation(slider_program, 0, "vPosition");
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);
	glEnableVertexAttribArray(0);


	u_state           = glGetUniformLocation(slider_program, "state");
	u_sliderPosition  = glGetUniformLocation(slider_program, "sliderPosition");
	u_sliderDimension = glGetUniformLocation(slider_program, "sliderDimension");
}


void slider_update(struct Slider *S, float xpos) {
	float newstate = (xpos - S->x) / (double) S->w;

	if (newstate < 0.0f) newstate = 0.0f;
	else if (newstate > 1.0f) newstate = 1.0f;

	S->state = newstate;
}


void slider_render(struct Slider *S) {
	glViewport(S->x, S->y, S->w, S->h);

	glUseProgram(slider_program);

	glUniform1f(u_state, S->state);
	glUniform2f(u_sliderPosition, S->x, S->y);
	glUniform2f(u_sliderDimension, S->w, S->h);

	glDrawArrays(GL_TRIANGLES, 0, 6);
}
