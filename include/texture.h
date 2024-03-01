#ifndef __TEXTURE_H__
#define __TEXTURE_H__


#include <GLES2/gl2.h>
#include <EGL/egl.h>


class Texture {
	public:
		Texture(int width_, int height_);
		GLuint id;
		unsigned char *buffer = NULL;
		int width;
		int height;
};


#endif
