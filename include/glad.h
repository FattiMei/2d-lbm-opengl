#ifdef USE_GLES2
	#include "gles2.h"
	#define gladLoadGL(arg) gladLoadGLES2(arg)
#else
	#include "gl.h"
#endif
