#include "render.h"
#include "shader.h"
#include "texture.h"
#include "lbm.h"
#include "glad.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>


unsigned int program;


static const float vertices[] = {
	 1.0f,  1.0f, 1.0f, 1.0f,
	 1.0f, -1.0f, 1.0f, 0.0f,
	-1.0f, -1.0f, 0.0f, 0.0f,
	-1.0f,  1.0f, 0.0f, 1.0f
};


static unsigned int indices[] = {
	0, 1, 3,
	1, 2, 3
};


unsigned int VAO, VBO, EBO;


const char* vertex_shader_src = R"(
	#version 430 core
	layout (location = 0) in vec2 aPos;
	layout (location = 1) in vec2 aTexCoord;

	out vec2 texCoord;

	void main() {
		gl_Position = vec4(aPos, 0.0, 1.0);
		texCoord = aTexCoord;
	}
)";


const char* fragment_shader_src = R"(
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
	program = program_load(vertex_shader_src, fragment_shader_src);


	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);

	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (void*)(2 * sizeof(float)));
	glEnableVertexAttribArray(1);

	glBindBuffer(GL_ARRAY_BUFFER, 0); 
	glBindVertexArray(0); 
}


void render_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


void render_render() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
