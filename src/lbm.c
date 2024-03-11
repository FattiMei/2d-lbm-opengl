#include "lbm.h"
#include "texture.h"
#include "glad.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>


int width;
int height;
float *u_out;
unsigned int lbm_texture_id;


static int it = 0;
static bool first_write = true;
static unsigned char *lbm_texture_buffer;


static float reynolds, u_in;
static float nu,
	     tau,
	     sigma,
	     double_square_sigma,
	     lambda_trt,
	     tau_minus,
	     omega_plus,
	     omega_minus,
	     sub_param,
	     sum_param;


static int *boundary;
static float *ux,
	     *uy,
	     *f,
	     *new_f,
	     *rho;


// this variable was a bool, now it has become an unsigned int (same memory footprint) and we will use the possible values to store information about obstacles and walls in a bitfield fashion
// at the moment 0 means no obstacle and 1 means obstacle
#define IS_OBSTACLE 1
#define TOP_WALL 2
#define BOTTOM_WALL 4
#define LEFT_WALL 8
#define RIGHT_WALL 16

static unsigned int *obstacles;


static void lbm_reset_field(float f[], float rho[], float u_out[], float ux[], float uy[], const int width, const int height, const unsigned int obstacles[]) {
	const int size = width * height;
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


	for (int index = 0; index < size; ++index) {
		u_out[index] = 0.0f;

		if (obstacles[index] & IS_OBSTACLE) {
			ux[index] = NAN;
			uy[index] = NAN;
		}
		else {
			for (int i = 0; i < 9; ++i) {
				f[index + size * i] = weights[i];
			}

			rho[index] = 1;
			ux[index]  = 0;
			uy[index]  = 0;
		}
	}
}


// @TODO: boundary can encode the obstacle information, while being memory efficient, not sure if that's the case, look at shape
static void lbm_calc_boundary(int boundary[], const unsigned int obstacles[], const int width, const int height) {
	const int dirs[4][2] = {{1, 0}, {0, 1}, {1, 1}, {-1, 1}};
	const int size = width * height;


	for (int row = 0; row < height; ++row) {
		for (int col = 0; col < width; ++col) {
			const int index = col + row * width;

			for (int d = 0; d < 4; d++) {
				// get the offsets for the current direction
				const int dx = dirs[d][0];
				const int dy = dirs[d][1];

				// check the adjacent cells in the current direction
				if (col - dx >= 0 && row - dy >= 0 && (obstacles[col - dx + (row - dy) * width] & IS_OBSTACLE)) {
					boundary[size * d + index] = -1;
				}
				else if (col + dx < width && row + dy < height && (obstacles[col + dx + (row + dy) * width] & IS_OBSTACLE)) {
					boundary[size * d + index] = 1;
				}
				else {
					boundary[size * d + index] = 0;
				}
			}
		}
	}
}


static void lbm_substep1(
		const int width,
		const int height,
		const float u_in_now,
		const float om_p,
		const float sum_param,
		const float sub_param,
		float f[],
		float new_f[],
		float rho[],
		float ux[],
		float uy[],
		float u_out[],
		const int boundary[],
		const unsigned int obstacles[]) {

#define F(x) f[size * x + index]
#define NEW_F(x) new_f[size * x + index]


	const int size = width * height;
	const int velocitiesX[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
	const int velocitiesY[9] = {0, 0, -1, 0, 1, -1, -1, 1, 1};
	const int opposite[9]    = {0, 3, 4, 1, 2, 7, 8, 5, 6};
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


#pragma omp parallel for
	for (int index = 0; index < size; ++index) {
		const unsigned int type = obstacles[index];

		if(!(type & IS_OBSTACLE)) {
			// if i'm a any boundary set u to 0
			if (type & (~IS_OBSTACLE)) {
				ux[index] = 0;
				uy[index] = 0;
			}

			// set parabolic profile inlet
			if (type & LEFT_WALL) {
				const float halfDim = (float)(height - 1) / 2.0;
				const float temp = (float)((index / width) / halfDim) - 1.0;
				const float mul = 1.0 - temp * temp;

				ux[index] = u_in_now * mul;
			}

			// zou he

			// top wall
			if ((type & TOP_WALL) && !(type & (LEFT_WALL | RIGHT_WALL))) {
				rho[index] = (F(0) + F(1) + F(3) + 2.0 * (F(2) + F(5) + F(6))) / (1.0 + uy[index]);
				F(4) = F(2) - 2.0 / 3.0 * rho[index] * uy[index];
				F(7) = F(5) + 0.5 * (F(1) - F(3)) - 0.5 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
				F(8) = F(6) - 0.5 * (F(1) - F(3)) + 0.5 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
			}
			// right wall
			else if ((type & RIGHT_WALL) && !(type & (TOP_WALL | BOTTOM_WALL))) {
				rho[index] = 1;
				ux[index] = F(0) + F(2) + F(4) + 2.0 * (F(1) + F(5) + F(8)) - 1.0;
				F(3) = F(1) - 2.0 / 3.0 * ux[index];
				F(6) = F(8) - 0.5 * (F(2) - F(4)) - 1.0 / 6.0 * ux[index];
				F(7) = F(5) + 0.5 * (F(2) - F(4)) - 1.0 / 6.0 * ux[index];
			}
			// bottom wall
			else if ((type & BOTTOM_WALL) && !(type & (LEFT_WALL | RIGHT_WALL))) {
				rho[index] = (F(0) + F(1) + F(3) + 2.0 * (F(4) + F(7) + F(8))) / (1.0 - uy[index]);
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(5) = F(7) - 0.5 * (F(1) - F(3)) + 0.5 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
				F(6) = F(8) + 0.5 * (F(1) - F(3)) - 0.5 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
			}
			// left wall
			else if ((type & LEFT_WALL) && !(type & (TOP_WALL | BOTTOM_WALL))) {
				rho[index] = (F(0) + F(2) + F(4) + 2.0 * (F(3) + F(7) + F(6))) / (1.0 - ux[index]);
				F(1) = F(3) + 2.0 / 3.0 * rho[index] * ux[index];
				F(5) = F(7) - 0.5 * (F(2) - F(4)) + 1.0 / 6.0 * rho[index] * ux[index] + 0.5 * rho[index] * uy[index];
				F(8) = F(6) + 0.5 * (F(2) - F(4)) + 1.0 / 6.0 * rho[index] * ux[index] - 0.5 * rho[index] * uy[index];
			}
			// top right corner
			else if ((type & TOP_WALL) && (type & RIGHT_WALL)) {
				rho[index] = rho[index - 1];
				F(3) = F(1) - 2.0 / 3.0 * rho[index] * ux[index];
				F(4) = F(2) - 2.0 / 3.0 * rho[index] * uy[index];
				F(7) = F(5) - 1.0 / 6.0 * rho[index] * ux[index] - 1.0 / 6.0 * rho[index] * uy[index];
				F(8) = 0;
				F(6) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(5) - F(7);
			}
			// bottom right corner
			else if ((type & BOTTOM_WALL) && (type & RIGHT_WALL)) {
				rho[index] = rho[index - 1];
				F(3) = F(1) - 2.0 / 3.0 * rho[index] * ux[index];
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(6) = F(8) + 1.0 / 6.0 * rho[index] * uy[index] - 1.0 / 6.0 * rho[index] * ux[index];
				F(7) = 0;
				F(5) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(6) - F(8);
			}
			// bottom left corner
			else if ((type & BOTTOM_WALL) && (type & LEFT_WALL)) {
				rho[index] = rho[index + 1];
				F(1) = F(3) + 2.0 / 3.0 * rho[index] * ux[index];
				F(2) = F(4) + 2.0 / 3.0 * rho[index] * uy[index];
				F(5) = F(7) + 1.0 / 6.0 * rho[index] * ux[index] + 1.0 / 6.0 * rho[index] * uy[index];
				F(6) = 0;
				F(8) = 0;
				F(0) = rho[index] - F(1) - F(2) - F(3) - F(4) - F(5) - F(7);
			}
			// top left corner
			else if ((type & TOP_WALL) && (type & LEFT_WALL)) {
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
			u_out[index] = sqrtf(ux[index] * ux[index] + uy[index] * uy[index]);

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

#undef F
#undef NEW_F
}


static void lbm_substep2(
		const int width,
		const int height,
		float f[],
		const float new_f[],
		const unsigned int obstacles[]) {


#define F(x) f[size * x + index]
#define NEW_F(x) new_f[size * x + index]


	const int size = width * height;
	const int velocitiesX[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
	const int velocitiesY[9] = {0, 0, -1, 0, 1, -1, -1, 1, 1};


#pragma omp parallel for
	for (int index = 0; index < size; ++index) {
		if (!(obstacles[index] & IS_OBSTACLE)) {
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
				if (new_row >= 0 && new_row < height && new_col >= 0 && new_col < width && !(obstacles[new_index] & IS_OBSTACLE)) {
					f[size * i + new_index] = NEW_F(i);
				}
			}
		}
	}

#undef F
#undef NEW_F
}


void lbm_allocate_resources() {
	obstacles = malloc(    width * height * sizeof(*obstacles));
	ux        = malloc(    width * height * sizeof(*ux));
	uy        = malloc(    width * height * sizeof(*uy));
	u_out     = malloc(    width * height * sizeof(*u_out));
	rho       = malloc(    width * height * sizeof(*rho));
	f         = malloc(9 * width * height * sizeof(*f));
	new_f     = malloc(9 * width * height * sizeof(*new_f));
	boundary  = malloc(4 * width * height * sizeof(*boundary));
}


void lbm_release_resources() {
	// free(obstacles);
	// free(ux);
	// free(uy);
	// free(u_out);
	// free(rho);
	// free(f);
	// free(new_f);
	// free(boundary);
	// free(lbm_texture_buffer);
	// texture_destroy(lbm_texture_id);
}


// to be called only after opening an opengl context + glad setup
void lbm_init(FILE *in) {
	int max_it;
	int read = fscanf(in, "%d %d\n%f %d %f\n", &width, &height, &reynolds, &max_it, &u_in);
	(void) read;
	(void) max_it;


	nu = u_in * (float) (height) / reynolds * 2.0 / 3.0;
	tau = 3.0 * nu + 0.5;
	sigma = ceil(10.0 * height);
	double_square_sigma = 2.0 * sigma * sigma;
	lambda_trt = 1.0 / 4.0;
	tau_minus = lambda_trt / (tau - 0.5) + 0.5;
	omega_plus = 1.0 / tau;
	omega_minus = 1.0 / tau_minus;
	sub_param = 0.5 * (omega_plus - omega_minus);
	sum_param = 0.5 * (omega_plus + omega_minus);


	lbm_allocate_resources();


	// this procedure could be astracted away
	int x, y;
	memset(obstacles, 0, width * height * sizeof(unsigned int));
	while (fscanf(in, "%d %d\n", &x, &y) == 2) {
		obstacles[x + y * width] |= IS_OBSTACLE;
	}
	fclose(in);


	// populate obstacles wall information
	const int size = width * height;

	for (int i = 0; i < width; ++i) {
		// @TODO: find which way is up
		obstacles[i]            |= TOP_WALL;
		obstacles[size - 1 - i] |= BOTTOM_WALL;
	}


	for (int j = 0; j < height; ++j) {
		// @TODO: find which way is right
		obstacles[j * width] |= LEFT_WALL;
		obstacles[j * width + width - 1] |= RIGHT_WALL;
	}


	lbm_calc_boundary(boundary, obstacles, width, height);
	lbm_reset_field(f, rho, u_out, ux, uy, width, height, obstacles);


	lbm_texture_id = texture_create(width, height);
	lbm_texture_buffer = (unsigned char *) malloc(3 * width * height * sizeof(unsigned char));
}


void lbm_step() {
	const float u_in_now = u_in * (1.0 - exp(-(it * it) / double_square_sigma));

	lbm_substep1(width, height, u_in_now, omega_plus, sum_param, sub_param, f, new_f, rho, ux, uy, u_out, boundary, obstacles);
	lbm_substep2(width, height, f, new_f, obstacles);

	++it;
}


void lbm_reload() {
	lbm_reset_field(f, rho, u_out, ux, uy, width, height, obstacles);
	it = 0;
}


static float colormap_red(float x) {
	return 4.04377880184332E+00 * x - 5.17956989247312E+02;
}


static float colormap_green(float x) {
	if (x < (5.14022177419355E+02 + 1.13519230769231E+01) / (4.20313644688645E+00 + 4.04233870967742E+00)) {
		return 4.20313644688645E+00 * x - 1.13519230769231E+01;
	} else {
		return -4.04233870967742E+00 * x + 5.14022177419355E+02;
	}
}


static float colormap_blue(float x) {
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


void lbm_write_on_texture() {
#pragma omp parallel for
	for (int i = 0; i < width * height; ++i) {
		unsigned char *base = lbm_texture_buffer + 3 * i;

		if (obstacles[i]) {
			base[0] = 255;
			base[1] = 255;
			base[2] = 255;
		}
		else {
			// assuming u_out is in [0, 0.3]
			const float u = 255.0f * u_out[i] / 0.3;

			base[0] = (unsigned char) floor(colormap_red(u));
			base[1] = (unsigned char) floor(colormap_green(u));
			base[2] = (unsigned char) floor(colormap_blue(u));
		}
	}


	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, lbm_texture_buffer);
	glGenerateMipmap(GL_TEXTURE_2D);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);
}


void lbm_write_on_file(FILE *out) {
	if (first_write) {
		fprintf(out, "%d %d\n", width, height);
		first_write = false;
	}

	fprintf(out, "%d\n", it);
	fwrite(u_out, sizeof(float), width * height, out);
}


void lbm_close() {
	lbm_release_resources();
}
