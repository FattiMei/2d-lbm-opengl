#ifndef __SHADER_H__
#define __SHADER_H__


#include "glad.h"


GLint shader_load          (GLenum type, const GLchar* string, const GLint *length);
GLint shader_load_from_file(GLenum type, const char *filename);

GLint program_load(const GLchar *vertex_shader_src, const GLchar *fragment_shader_src);
GLint compute_program_load(const GLchar *compute_shader_src);
GLint compute_program_load_from_file(const char *filename);


#endif
