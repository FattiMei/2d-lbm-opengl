#ifdef USE_GLES2
	#include <GLES2/gl2.h>
	#define gladLoadGL(arg) {}
#else
	#include "gl.h"
#endif
