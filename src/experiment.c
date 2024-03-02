#include "experiment.h"
#include "shader.h"
#include "lbm.h"
#include "glad.h"
#include <GLES2/gl2.h>
#include <EGL/egl.h>
#include <stdlib.h>
#include <math.h>
#include <stdbool.h>


GLuint texture;
unsigned int program;
unsigned char *texture_buffer;


unsigned int shaderProgram;
unsigned int VAO;


//const float vertices[] = {
//	-1.0f, -1.0f, 0.0f,
//	-1.0f,  1.0f, 0.0f,
//	 1.0f, -1.0f, 0.0f,
//	-1.0f,  1.0f, 0.0f,
//	 1.0f,  1.0f, 0.0f,
//	 1.0f, -1.0f, 0.0f
//};


const float texture_vertices[] = {
	0.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 0.0f,
	0.0f, 1.0f,
	1.0f, 1.0f,
	1.0f, 0.0f,
};


const char* vertex_shader_src = R"(
	#version 330 core
	layout (location = 0) in vec3 aPos;

	void main() {
		gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
	}
)";


const char* fragment_shader_src = R"(
	#version 330 core
	out vec4 FragColor;

	void main() {
		FragColor = vec4(1.0f, 0.5f, 0.2f, 1.0f);
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
void experiment_init(int width, int height) {
	unsigned int vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vertex_shader_src, NULL);
	glCompileShader(vertexShader);
	// check for shader compile errors
	int success;
	glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
	// fragment shader
	unsigned int fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fragment_shader_src, NULL);
	glCompileShader(fragmentShader);
	// check for shader compile errors
	glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
	// link shaders
	shaderProgram = glCreateProgram();
	glAttachShader(shaderProgram, vertexShader);
	glAttachShader(shaderProgram, fragmentShader);
	glLinkProgram(shaderProgram);
	// check for linking errors
	glGetProgramiv(shaderProgram, GL_LINK_STATUS, &success);
	glDeleteShader(vertexShader);
	glDeleteShader(fragmentShader);

	// set up vertex data (and buffer(s)) and configure vertex attributes
	// ------------------------------------------------------------------
	float vertices[] = {
		0.5f,  0.5f, 0.0f,  // top right
		0.5f, -0.5f, 0.0f,  // bottom right
		-0.5f, -0.5f, 0.0f,  // bottom left
		-0.5f,  0.5f, 0.0f   // top left 
	};
	unsigned int indices[] = {  // note that we start from 0!
		0, 1, 3,  // first Triangle
		1, 2, 3   // second Triangle
	};
	unsigned int VBO, EBO;
	glGenVertexArrays(1, &VAO);
	glGenBuffers(1, &VBO);
	glGenBuffers(1, &EBO);
	// bind the Vertex Array Object first, then bind and set vertex buffer(s), and then configure vertex attributes(s).
	glBindVertexArray(VAO);

	glBindBuffer(GL_ARRAY_BUFFER, VBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);
	glEnableVertexAttribArray(0);

	// note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
	glBindBuffer(GL_ARRAY_BUFFER, 0); 

	// remember: do NOT unbind the EBO while a VAO is active as the bound element buffer object IS stored in the VAO; keep the EBO bound.
	//glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);

	// You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
	// VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
	glBindVertexArray(0); 


	// uncomment this call to draw in wireframe polygons.
	//glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	//program = program_load(vertex_shader_src, fragment_shader_src);

	//glBindAttribLocation(program, 0, "position");
	//glEnableVertexAttribArray(0);
	//glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 0, vertices);

	//glBindAttribLocation(program, 1, "atexCoord");
	//glEnableVertexAttribArray(1);
	//glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 0, texture_vertices);

	//glGenTextures(1, &texture);
	//glBindTexture(GL_TEXTURE_2D, texture);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);	
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	//// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
	//// glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	//glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);


	//// RGB rendering so for each cell we need 3 colors
	texture_buffer = (unsigned char *) malloc(width * height * sizeof(unsigned char) * 3);
}


void experiment_resize(int width, int height) {
	glViewport(0, 0, width, height);
}


void experiment_render() {
	experiment_populate_texture();

	glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// draw our first triangle
	glUseProgram(shaderProgram);
	glBindVertexArray(VAO); // seeing as we only have a single VAO there's no need to bind it every time, but we'll do so to keep things a bit more organized
				//glDrawArrays(GL_TRIANGLES, 0, 6);
	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	// glBindVertexArray(0); // no need to unbind it every time 
}
