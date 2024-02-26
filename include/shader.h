#ifndef __SHADER_H__
#define __SHADER_H__


#include <GLES2/gl2.h>
#include <EGL/egl.h>


GLint shader_load          (GLenum type, const GLchar* string, const GLint *length);
GLint shader_load_from_file(GLenum type, const char *filename);

// GLint program_load(GLint vertex_shader, GLint fragment_shader);
GLint program_load(const GLchar *vertex_shader_src, const GLchar *fragment_shader_src);


#endif
