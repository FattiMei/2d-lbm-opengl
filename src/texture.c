#include "texture.h"
#include "glad.h"
#include <stdlib.h>


unsigned int texture_create(int width, int height) {
	unsigned int id;

	glGenTextures(1, &id);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, id);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);

	#ifndef USE_GLES2
		// set specifically to interact with compute shaders
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA32F, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
		glBindImageTexture(0, id, 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
	#else
		// @TODO: is this instruction needed?
		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGBA, GL_FLOAT, NULL);
	#endif

	return id;
}


void texture_destroy(unsigned int texture_id) {
	glDeleteTextures(1, &texture_id);
}
