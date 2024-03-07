#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;


layout (std430, binding = 0) buffer Obstacles {
	int obstacles[];
};


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


layout (std430, binding = 6) buffer NewF {
	float new_f[];
};


layout (std430, binding = 7) buffer Bound {
	int boundary[];
};


layout (location = 0) uniform float u_in_now;
layout (location = 1) uniform float om_p;
layout (location = 2) uniform float sum_param;
layout (location = 3) uniform float sub_param;


void main() {
	uint index = gl_GlobalInvocationID.x;
}
