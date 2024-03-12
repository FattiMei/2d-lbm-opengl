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
layout (location = 1) uniform float u_in_now;
layout (location = 2) uniform float om_p;
layout (location = 3) uniform float sum_param;
layout (location = 4) uniform float sub_param;


#define F(x) f[size * x + index]
#define NEW_F(x) new_f[size * x + index]


#define IS_OBSTACLE 1
#define TOP_WALL 2
#define BOTTOM_WALL 4
#define LEFT_WALL 8
#define RIGHT_WALL 16


void main() {
	uint index = gl_GlobalInvocationID.x;
	uint size = shape.x * shape.y;

	const int velocitiesX[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
	const int velocitiesY[9] = {0, 0, -1, 0, 1, -1, -1, 1, 1};
	const int opposite[9]    = {0, 3, 4, 1, 2, 7, 8, 5, 6};
	const float weights[9] = {
		  4.0 / 9.0
		, 1.0 / 9.0
		, 1.0 / 9.0
		, 1.0 / 9.0
		, 1.0 / 9.0
		, 1.0 / 36.0
		, 1.0 / 36.0
		, 1.0 / 36.0
		, 1.0 / 36.0
	};


	if (index < size) {
		const int type = obstacles[index];

		if(!((type & IS_OBSTACLE) != 0)) {
			// if i'm a any boundary set u to 0
			if ((type & (~IS_OBSTACLE)) != 0) {
				ux[index] = 0;
				uy[index] = 0;
			}

			// set parabolic profile inlet
			if ((type & LEFT_WALL) != 0) {
				const float height = float(shape.y);
				const float halfDim = (height - 1) / 2.0;
				const float float_index = float(index);
				const float float_width = float(shape.x);
				// const float temp = (float)((index / width) / halfDim) - 1.0;
				const float temp = ((float_index / float_width) / halfDim) - 1.0;
				const float mul = 1.0 - temp * temp;

				ux[index] = u_in_now * mul;
			}

			// zou he

			// top wall
			if (((type & TOP_WALL) != 0) && !((type & (LEFT_WALL | RIGHT_WALL)) != 0)) {
				rho[index] = (F(0) + F(1) + F(3) + 2.0 * (F(2) + F(5) + F(6))) / (1.0 + uy[index]);
				F(4) = F(2) - 2.0 / 3.0 * rho[index] * uy[index];
				F(7) = F(5) + 0.5 * (F(1) - F(3)) - 0.5 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
				F(8) = F(6) - 0.5 * (F(1) - F(3)) + 0.5 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
			}
			// right wall
			else if (((type & RIGHT_WALL) != 0) && !((type & (TOP_WALL | BOTTOM_WALL)) != 0)) {
				rho[index] = 1;
				ux[index] = F(0) + F(2) + F(4) + 2.0 * (F(1) + F(5) + F(8)) - 1.0;
				F(3) = F(1) - 2.0 / 3.0 * ux[index];
				F(6) = F(8) - 0.5 * (F(2) - F(4)) - 1.0 / 6.0 * ux[index];
				F(7) = F(5) + 0.5 * (F(2) - F(4)) - 1.0 / 6.0 * ux[index];
			}
			// bottom wall
			else if (((type & BOTTOM_WALL) != 0) && !((type & (LEFT_WALL | RIGHT_WALL)) != 0)) {
				rho[index] = (F(0) + F(1) + F(3) + 2.0 * (F(4) + F(7) + F(8))) / (1.0 - uy[index]);
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(5) = F(7) - 0.5 * (F(1) - F(3)) + 0.5 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
				F(6) = F(8) + 0.5 * (F(1) - F(3)) - 0.5 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
			}
			// left wall
			else if (((type & LEFT_WALL) != 0) && !((type & (TOP_WALL | BOTTOM_WALL)) != 0)) {
				rho[index] = (F(0) + F(2) + F(4) + 2.0 * (F(3) + F(7) + F(6))) / (1.0 - ux[index]);
				F(1) = F(3) + 2.0 / 3.0 * rho[index] * ux[index];
				F(5) = F(7) - 0.5 * (F(2) - F(4)) + 1.0 / 6.0 * rho[index] * ux[index] + 0.5 * rho[index] * uy[index];
				F(8) = F(6) + 0.5 * (F(2) - F(4)) + 1.0 / 6.0 * rho[index] * ux[index] - 0.5 * rho[index] * uy[index];
			}
			// top right corner
			else if (((type & TOP_WALL) != 0) && ((type & RIGHT_WALL) != 0)) {
				rho[index] = rho[index - 1];
				F(3) = F(1) - 2.0 / 3.0 * rho[index] * ux[index];
				F(4) = F(2) - 2.0 / 3.0 * rho[index] * uy[index];
				F(7) = F(5) - 1.0 / 6.0 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
				F(8) = 0;
				F(6) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(5) - F(7);
			}
			// bottom right corner
			else if (((type & BOTTOM_WALL) != 0) && ((type & RIGHT_WALL) != 0)) {
				rho[index] = rho[index - 1];
				F(3) = F(1) - 2.0 / 3.0 * rho[index] * ux[index];
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(6) = F(8) + 1.0 / 6.0 * rho[index] * uy[index] - 1.0 / 6.0 * rho[index] * ux[index];
				F(7) = 0;
				F(5) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(6) - F(8);
			}
			// bottom left corner
			else if (((type & BOTTOM_WALL) != 0) && ((type & LEFT_WALL) != 0)) {
				rho[index] = rho[index + 1];
				F(1) = F(3) + 2.0 / 3.0 * rho[index] * ux[index];
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(5) = F(7) + 1.0 / 6.0 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
				F(6) = 0;
				F(8) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(5) - F(7);
			}
			// top left corner
			else if (((type & TOP_WALL) != 0) && ((type & LEFT_WALL) != 0)) {
				rho[index] = rho[index + 1];
				F(1) = F(3) + 2.0 / 3.0 * rho[index] * ux[index];
				F(4) = F(2) - 2.0 / 3.0 * rho[index] * uy[index];
				F(8) = F(6) + 1.0 / 6.0 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
				F(7) = 0;
				F(5) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(6) - F(8);
			}

			// update macro

			rho[index] = 0;
			ux[index] = 0;
			uy[index] = 0;
			for (int i = 0; i < 9; i++) {
				rho[index] += F(i);
				ux[index] += F(i) * velocitiesX[i];
				uy[index] += F(i) * velocitiesY[i];
			}
			ux[index] /= rho[index];
			uy[index] /= rho[index];
			u_out[index] = sqrt(ux[index] * ux[index] + uy[index] * uy[index]);

			// equilibrium
			float feq[9];
			const float temp1 = 1.5 * (ux[index] * ux[index] + uy[index] * uy[index]);
			for (int i = 0; i < 9; i++) {
				const float temp2 = 3.0 * (velocitiesX[i] * ux[index] + velocitiesY[i] * uy[index]);
				feq[i] = weights[i] * rho[index] * (1.0 + temp2 + 0.5 * temp2 * temp2 - temp1);
			}

			// collision for index 0
			NEW_F(0) = (1.0 - om_p) * F(0) + om_p * feq[0];

			// collision for other indices
			for (int i = 1; i < 9; i++) {
				NEW_F(i) = (1.0 - sum_param) * F(i) - sub_param * F(opposite[i]) + sum_param * feq[i] + sub_param * feq[opposite[i]];
			}

			// regular bounce back

			if (boundary[index] == 1) {
				F(3) = NEW_F(1);
			}
			else if (boundary[index] == -1) {
				F(1) = NEW_F(3);
			}
			if (boundary[size + index] == 1) {
				F(2) = NEW_F(4);
			}
			else if (boundary[size + index] == -1) {
				F(4) = NEW_F(2);
			}
			if (boundary[size * 2 + index] == 1) {
				F(6) = NEW_F(8);
			}
			else if (boundary[size * 2 + index] == -1) {
				F(8) = NEW_F(6);
			}
			if (boundary[size * 3 + index] == 1) {
				F(5) = NEW_F(7);
			}
			else if (boundary[size * 3 + index] == -1) {
				F(7) = NEW_F(5);
			}
		}
	}
}
