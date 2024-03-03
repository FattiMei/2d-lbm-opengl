#include "experiment.h"
#include "shader.h"
#include "texture.h"
#include "lbm.h"
#include "glad.h"
#include <stdlib.h>
#include <math.h>


GLuint texture;
unsigned int program;
unsigned int compute_shader_program;
unsigned char *texture_buffer;


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


const char* compute_shader_src = R"(
	#version 430 core
	layout (local_size_x = 10, local_size_y = 10, local_size_z = 1) in;

	layout (location = 0) writeonly uniform image2D imgOutput;

	void main() {
		uvec2 index = gl_GlobalInvocationID.xy;

		vec4 color = vec4(0.5, 0.2, 0.7, 1.0);
		imageStore(imgOutput, ivec2(index), color);
	}
)";


static float colormap_red(float x) {
	return 4.04377880184332E+00 * x - 5.17956989247312E+02;
}


static float colormap_green(float x) {
	if (x < (5.14022177419355E+02 + 1.13519230769231E+01) / (4.20313644688645E+00 + 4.04233870967742E+00)) {
		return 4.20313644688645E+00 * x - 1.13519230769231E+01;
	} else {
		return -4.04233870967742E+00 * x + 5.14022177419355E+02;
	}
}


static float colormap_blue(float x) {
	if (x < 1.34071303331385E+01 / (4.25125657510228E+00 - 1.0)) { // 4.12367649967
		return x;
	} else if (x < (255.0 + 1.34071303331385E+01) / 4.25125657510228E+00) { // 63.1359518278
		return 4.25125657510228E+00 * x - 1.34071303331385E+01;
	} else if (x < (1.04455240613432E+03 - 255.0) / 4.11010047593866E+00) { // 192.100512082
		return 255.0;
	} else {
		return -4.11010047593866E+00 * x + 1.04455240613432E+03;
	}
}


void experiment_populate_texture() {
	#pragma omp parallel for
	for (int i = 0; i < width * height; ++i) {
		unsigned char *base = texture_buffer + 3 * i;

		if (obstacles[i]) {
			base[0] = 255;
			base[1] = 255;
			base[2] = 255;
		}
		else {
			// assuming u_out is in [0, 0.3]
			const float u = 255.0f * u_out[i] / 0.3;

			base[0] = (unsigned char) floor(colormap_red(u));
			base[1] = (unsigned char) floor(colormap_green(u));
			base[2] = (unsigned char) floor(colormap_blue(u));
		}
	}


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, texture_buffer);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, texture);
}


// to be called after initializing lbm
void experiment_init() {
	program = program_load(vertex_shader_src, fragment_shader_src);
	compute_shader_program = compute_program_load(compute_shader_src);


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


	texture_buffer = (unsigned char *) malloc(width * height * sizeof(unsigned char) * 3);

	texture = texture_create(width, height);
}


void experiment_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


void experiment_render() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, texture);


	// experiment_populate_texture();

	glUseProgram(compute_shader_program);
	glDispatchCompute(width / 10, height / 10, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
