#version 430 core
layout (local_size_x = 1, local_size_y = 1, local_size_z = 1) in;


layout (std430, binding = 0) buffer Obstacles {
	int obstacles[];
};


layout (std430, binding = 1) buffer U_out {
	float u_out[];
};


layout (location = 0) writeonly uniform image2D imgOutput;
layout (location = 1) uniform ivec2 shape;


float colormap_red(float x) {
	return 4.04377880184332E+00 * x - 5.17956989247312E+02;
}


float colormap_green(float x) {
	if (x < (5.14022177419355E+02 + 1.13519230769231E+01) / (4.20313644688645E+00 + 4.04233870967742E+00)) {
		return 4.20313644688645E+00 * x - 1.13519230769231E+01;
	} else {
		return -4.04233870967742E+00 * x + 5.14022177419355E+02;
	}
}


float colormap_blue(float x) {
	if (x < 1.34071303331385E+01 / (4.25125657510228E+00 - 1.0)) { // 4.12367649967
		return x;
	} else if (x < (255.0 + 1.34071303331385E+01) / 4.25125657510228E+00) { // 63.1359518278
		return 4.25125657510228E+00 * x - 1.34071303331385E+01;
	} else if (x < (1.04455240613432E+03 - 255.0) / 4.11010047593866E+00) { // 192.100512082
		return 255.0;
	} else {
		return -4.11010047593866E+00 * x + 1.04455240613432E+03;
	}
}


void main() {
	const int size = shape.x * shape.y;
	const int index = int(gl_GlobalInvocationID.x);

	if (index < size) {
		float x = 255.0 * (u_out[index] / 0.3);
		vec4 color;

		if ((obstacles[index] & 1) == 1) {
			color = vec4(1.0, 1.0, 1.0, 1.0);
		}
		else {
			color = vec4(colormap_red(x) / 255.0, colormap_green(x) / 255.0, colormap_blue(x) / 255.0, 1.0);
		}

		imageStore(imgOutput, ivec2(index % shape.x, index / shape.x), color);
	}
}
