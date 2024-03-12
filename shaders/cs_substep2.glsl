#version 430 core
layout (local_size_x = 10, local_size_y = 1, local_size_z = 1) in;


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


layout (location = 0) uniform ivec2 shape;


#define F(x) f[size * x + index]
#define NEW_F(x) new_f[size * x + index]


#define IS_OBSTACLE 1
#define TOP_WALL 2
#define BOTTOM_WALL 4
#define LEFT_WALL 8
#define RIGHT_WALL 16


#define width shape.x
#define height shape.y


void main() {
	int index = int(gl_GlobalInvocationID.x);
	int size = width * height;

	const int velocitiesX[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
	const int velocitiesY[9] = {0, 0, -1, 0, 1, -1, -1, 1, 1};

	if (index < size) {
		if ((obstacles[index] & IS_OBSTACLE) == 0) {
			// stream for index 0
			F(0) = NEW_F(0);

			// stream for other indices
			for (int i = 1; i < 9; i++) {
				const int row = index / width;
				const int col = index % width;

				// obtain new indices
				const int new_row = row + velocitiesY[i];
				const int new_col = col + velocitiesX[i];
				const int new_index = new_row * width + new_col;

				// stream if new index is not out of bounds or obstacle
				if (new_row >= 0 && new_row < height && new_col >= 0 && new_col < width && (obstacles[new_index] & IS_OBSTACLE) == 0) {
					f[size * i + new_index] = NEW_F(i);
				}
			}
		}
	}
}
