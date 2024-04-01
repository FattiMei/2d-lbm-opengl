#include "shader.h"
#include "glad.h"
#include <stdio.h>
#include <stdlib.h>


#define MAX_LOG_LENGTH 100000
static char log[MAX_LOG_LENGTH];


char *load_cstring_from_file(const char *filename) {
	char *result = NULL;
	FILE *fp = fopen(filename, "r");

	if (fp == NULL) {
		fprintf(stderr, "Error when opening file %s\n", filename);
		return NULL;
	}

	fseek(fp, 0L, SEEK_END);
	size_t numbytes = ftell(fp);
	fseek(fp, 0L, SEEK_SET);

	result = (char *) malloc(numbytes + 1);
	if (result == NULL) {
		fprintf(stderr, "[ERROR]: failed allocation of %u bytes\n", numbytes);
	}
	else {
		if (fread(result, sizeof(char), numbytes, fp) != numbytes) {
			fprintf(stderr, "[ERROR]: failed read of %u bytes\n", numbytes);
		}

		result[numbytes] = '\0';
	}

	fclose(fp);
	return result;
}


int shader_load(GLenum type, const GLchar* string, const int *length) {
	GLuint shader = glCreateShader(type);
	int compiled;

	if (shader == 0) {
		fprintf(stderr, "Could not create shader\n");
		return -1;
	}


	glShaderSource(shader, 1, &string, length);

	glCompileShader(shader);
	glGetShaderiv(shader, GL_COMPILE_STATUS, &compiled);

	if (!compiled) {
		fprintf(stderr, "Compile error:\n");

		int info_len = 0;
		glGetShaderiv(shader, GL_INFO_LOG_LENGTH, &info_len);

		if (info_len < MAX_LOG_LENGTH) {
			glGetShaderInfoLog(shader, info_len, NULL, log);

			fprintf(stderr, "%s\n", log);
			glDeleteShader(shader);
		}
		else {
			fprintf(stderr, "Log was too big (%d bytes)\n", info_len);
		}
	}

	return shader;
}


int shader_load_from_file(GLenum type, const char *filename) {
	char *shader_src = load_cstring_from_file(filename);
	int shader = shader_load(type, shader_src, NULL);

	free(shader_src);

	return shader;
}


int program_load(const GLchar *vertex_shader_src, const GLchar *fragment_shader_src) {
	int vertex_shader   = shader_load(GL_VERTEX_SHADER  , vertex_shader_src  , NULL);
	int fragment_shader = shader_load(GL_FRAGMENT_SHADER, fragment_shader_src, NULL);
	int program = glCreateProgram();
	int linked;

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

		int info_len = 0;
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


int program_load_from_file(const char *vs_filename, const char *fs_filename) {
	char *vertex_shader_src   = load_cstring_from_file(vs_filename);
	char *fragment_shader_src = load_cstring_from_file(fs_filename);
	int program = program_load(vertex_shader_src, fragment_shader_src);

	free(vertex_shader_src);
	free(fragment_shader_src);

	return program;
}


#ifndef USE_GLES2
int compute_program_load(const GLchar *compute_shader_src) {
	int compute_shader = shader_load(GL_COMPUTE_SHADER, compute_shader_src, NULL);
	int program = glCreateProgram();
	int linked;

	if (program == 0) {
		fprintf(stderr, "Error in creating program\n");
		return -1;
	}

	glAttachShader(program, compute_shader);
	glLinkProgram(program);

	glGetProgramiv(program, GL_LINK_STATUS, &linked);

	if (!linked) {
		fprintf(stderr, "Linker error:\n");

		int info_len = 0;
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


int compute_program_load_from_file(const char *filename) {
	char *program_src = load_cstring_from_file(filename);
	int program = compute_program_load(program_src);
	free(program_src);

	return program;
}
#endif
