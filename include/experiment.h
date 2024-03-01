#ifndef __EXPERIMENT_H__
#define __EXPERIMENT_H__


#include <GLES2/gl2.h>
#include <EGL/egl.h>


extern GLint program;


void experiment_init(int width, int height);
void experiment_resize(int width, int height);
void experiment_render();


#endif
