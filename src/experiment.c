#include "experiment.h"
#include "shader.h"
#include "texture.h"
#include "lbm.h"
#include "glad.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>


unsigned int program;
unsigned int compute_shader_program;
unsigned int u_buffer;


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
	layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;

	layout (std430, binding = 0) buffer U {
		float u_out[];
	};
	layout (location = 0) writeonly uniform image2D imgOutput;
	layout (location = 1) uniform ivec2 shape;

	void main() {
		uvec2 index = gl_GlobalInvocationID.xy;

		// hack
		vec4 color = vec4(u_out[index.y * 200 + index.x] / 0.3, 0.2, 0.7, 1.0);
		imageStore(imgOutput, ivec2(index), color);
	}
)";


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


	glGenBuffers(1, &u_buffer);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, u_buffer);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), NULL, GL_STATIC_DRAW);
}


void experiment_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


void experiment_render() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);



	// float *ptr = (float *) glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, width * height * sizeof(float), GL_MAP_WRITE_BIT);

	// memcpy(ptr, u_out, width * height * sizeof(float));
	// glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

	// glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, u_buffer);


	// glUseProgram(compute_shader_program);
	// glDispatchCompute(width, height, 1);
	// glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	glUseProgram(program);
	glBindVertexArray(VAO);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
}
