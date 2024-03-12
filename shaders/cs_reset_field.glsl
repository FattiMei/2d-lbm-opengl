#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;


layout (std430, binding = 1) buffer U_out {
	float u_out[];
};


layout (std430, binding = 2) buffer U_x {
	float ux[];
};


layout (std430, binding = 3) buffer U_y {
	float uy[];
};


layout (std430, binding = 4) buffer Rho {
	float rho[];
};


layout (std430, binding = 5) buffer F {
	float f[];
};


layout (location = 0) uniform ivec2 shape;


void main() {
	uint index = gl_GlobalInvocationID.x;
	uint size = shape.x * shape.y;

	const float weights[9] = {
		4.0 /  9.0,
		1.0 /  9.0,
		1.0 /  9.0,
		1.0 /  9.0,
		1.0 /  9.0,
		1.0 / 36.0,
		1.0 / 36.0,
		1.0 / 36.0,
		1.0 / 36.0
	};


	if (index < size) {
		u_out[index] = 0.0f;
		rho[index] = 1;
		ux[index]  = 0;
		uy[index]  = 0;

		for (int i = 0; i < 9; ++i) {
			f[index + size * i] = weights[i];
		}
	}
}
