#ifndef __SHADER_H__
#define __SHADER_H__


#include "glad.h"


int shader_load          (GLenum type, const GLchar* string, const int *length);
int shader_load_from_file(GLenum type, const char *filename);

int program_load(const GLchar *vertex_shader_src, const GLchar *fragment_shader_src);
int program_load_from_file(const char *fs_filename, const char *vs_filename);

int compute_program_load(const GLchar *compute_shader_src);
int compute_program_load_from_file(const char *filename);


#endif
