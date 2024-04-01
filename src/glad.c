#ifdef USE_GLES2
	#define GLAD_GLES2_IMPLEMENTATION
	#include "gles2.h"
	#define gladLoadGL(arg) gladLoadGLES2(arg)
#else
	#define GLAD_GL_IMPLEMENTATION
	#include "gl.h"
#endif
