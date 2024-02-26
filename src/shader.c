#include "shader.h"
#include <stdio.h>
#include <stdlib.h>


#define MAX_LOG_LENGTH 1024
static char log[MAX_LOG_LENGTH];


GLint shader_load(
	  GLenum type
	, const GLchar* string
	, const GLint *length
) {
	GLuint shader = glCreateShader(type);
	GLint compiled;

	if (shader == 0) {
		fprintf(stderr, "Could not create shader\n");
		return -1;
	}


	glShaderSource(shader, 1, &string, length);

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		fprintf(stderr, "Compile error:\n");

		GLint info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);

		if (info_len < MAX_LOG_LENGTH) {
			glGetShaderInfoLog(shader, info_len, NULL, log);

			fprintf(stderr, "%s\n", log);
			glDeleteShader(shader);
		}
	}

	return shader;
}


GLint shader_load_from_file(GLenum type, const char *filename) {
	GLuint shader;
	FILE *fp = fopen(filename, "r");

	fprintf(stderr, "[DEBUG]: opening file %s\n", filename);

	if (fp == NULL) {
		fprintf(stderr, "Error when opening file %s\n", filename);
		return -1;
	}

	{
		GLint numbytes = 0;
		char *buffer = NULL;

		// hack for finding the dimension of a file
		fseek(fp, 0L, SEEK_END);
		numbytes = ftell(fp);
		fseek(fp, 0L, SEEK_SET);

		fprintf(stderr, "[DEBUG]: file is %d bytes long\n", numbytes);

		buffer = (char *) malloc(numbytes + 1);
		if (buffer == NULL) {
			fprintf(stderr, "[ERROR]: failed allocation of %d bytes\n", numbytes);
			return -1;
		}

		fread(buffer, sizeof(char), numbytes, fp);
		buffer[numbytes] = '\0';

		shader = shader_load(type, buffer, NULL);

		free(buffer);
		fclose(fp);
	}

	return shader;
}


GLint program_load(const GLchar *vertex_shader_src, const GLchar *fragment_shader_src) {
	GLint vertex_shader   = shader_load(GL_VERTEX_SHADER  , vertex_shader_src  , NULL);
	GLint fragment_shader = shader_load(GL_FRAGMENT_SHADER, fragment_shader_src, NULL);
	GLint program = glCreateProgram();
	GLint linked;

	if (program == 0) {
		fprintf(stderr, "Error in creating program\n");
		return -1;
	}

	glAttachShader(program, vertex_shader);
	glAttachShader(program, fragment_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if (!linked) {
		fprintf(stderr, "Linker error:\n");

		GLint info_len = 0;
		glGetProgramiv(program, GL_INFO_LOG_LENGTH, &info_len);

		if (info_len < MAX_LOG_LENGTH) {
			glGetProgramInfoLog(program, info_len, NULL, log);

			fprintf(stderr, "%s\n", log);
			glDeleteProgram(program);
		}

		return -1;
	}

	// @TODO: delete the shaders

	return program;
}
