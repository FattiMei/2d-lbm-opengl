#ifdef USE_GLES2
	#include <GLES2/gl2.h>
	#define gladLoadGL(arg) {}
#else
	#define GLAD_GL_IMPLEMENTATION
	#include "gl.h"
#endif
