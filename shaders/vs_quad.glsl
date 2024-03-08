#version 430 core
layout (location = 0) in vec2 aPos;
layout (location = 1) in vec2 aTexCoord;

out vec2 texCoord;

void main() {
	vec2 uv;
	uv.x = (gl_VertexID & 1);
	uv.y = ((gl_VertexID >> 1) & 1);

	gl_Position = vec4(uv * 2.0 - 1.0, 0.0, 1.0);
	texCoord = uv;
}
