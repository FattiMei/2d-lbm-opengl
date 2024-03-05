#include "lbm.h"
#include "texture.h"
#include "shader.h"
#include "glad.h"
#include <stdlib.h>
#include <math.h>
#include <string.h>


int width;
int height;
float *u_out;
unsigned int lbm_texture_id;


static int it = 0;
static bool first_write = true;
static unsigned char *lbm_texture_buffer;


static unsigned int cs_render_program;


static unsigned int ssbo_obstacles;
static unsigned int ssbo_u_out;
static unsigned int ssbo_ux;
static unsigned int ssbo_uy;
static unsigned int ssbo_rho;
static unsigned int ssbo_f;
static unsigned int ssbo_new_f;
static unsigned int ssbo_boundary;



static const char* cs_render_src = R"(
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
		uvec2 index = gl_GlobalInvocationID.xy;

		if (index.x < shape.x && index.y < shape.y) {
			float x = 255.0 * (u_out[index.y * shape.x + index.x] / 0.3);
			vec4 color;

			if ((obstacles[index.y * shape.x + index.x] % 2) == 1) {
				color = vec4(1.0, 1.0, 1.0, 1.0);
			}
			else {
				color = vec4(colormap_red(x) / 255.0, colormap_green(x) / 255.0, colormap_blue(x) / 255.0, 1.0);
			}


			imageStore(imgOutput, ivec2(index), color);
		}
	}
)";


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


// this variable was a bool, now it has become an unsigned char (same memory footprint) and we will use the possible values to store information about obstacles and walls in a bitfield fashion
// at the moment 0 means no obstacle and 1 means obstacle
// since we can't have byte data type in glsl we must declare obstacles as int
#define IS_OBSTACLE 1
#define TOP_WALL 2
#define BOTTOM_WALL 4
#define LEFT_WALL 8
#define RIGHT_WALL 16

static int *obstacles;


static void lbm_reset_field(
	  float f[]
	, float rho[]
	, float ux[]
	, float uy[]
	, const int width
	, const int height
	, const int obstacles[]
) {
	const int size = width * height;
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


	for (int index = 0; index < size; ++index) {

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


// @TODO: boundary can encode the obstacle information, while being memory efficient
static void lbm_calc_boundary(
	  int boundary[]
	, const int obstacles[]
	, const int width
	, const int height
) {
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
	  const int width
	, const int height
	, const int it
	, const float u_in_now
	, const float om_p
        , const float sum_param
	, const float sub_param
	, float f[]
	, float new_f[]
	, float rho[]
	, float ux[]
        , float uy[]
	, float u_out[]
	, const int boundary[]
	, const int obstacles[]
) {
	#define F(x) f[size * x + index]
	#define NEW_F(x) new_f[size * x + index]


	const int size = width * height;
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


	#pragma omp parallel for
	for (int index = 0; index < size; ++index) {
		const int type = obstacles[index];

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
		const int width
		, const int height
		, float f[]
		, const float new_f[]
		, const int obstacles[]
	      ) {
	#define F(x) f[size * x + index]
	#define NEW_F(x) new_f[size * x + index]


	const int size = width * height;
	const int velocitiesX[9] = {0, 1, 0, -1, 0, 1, -1, -1, 1};
	const int velocitiesY[9] = {0, 0, -1, 0, 1, -1, -1, 1, 1};


	for (int row = 0; row < height; ++row) {
		for (int col = 0; col < width; ++col) {
			const int index = row * width + col;

			if (!(obstacles[index] & IS_OBSTACLE)) {
				// stream for index 0
				F(0) = NEW_F(0);

				// stream for other indices
				for (int i = 1; i < 9; i++) {
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
	}

	#undef F
	#undef NEW_F
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


	obstacles = (int  *) malloc(width * height * sizeof(int));
	ux        = (float *) malloc(width * height * sizeof(float));
	uy        = (float *) malloc(width * height * sizeof(float));
	u_out     = (float *) malloc(width * height * sizeof(float));
	rho       = (float *) malloc(width * height * sizeof(float));
	f         = (float *) malloc(9 * width * height * sizeof(float));
	new_f     = (float *) malloc(9 * width * height * sizeof(float));
	boundary  = (int   *) malloc(4 * width * height * sizeof(int));


	// allocate this memory on the gpu (2MB of data)
	// @TODO: avoid this replication
	glGenBuffers(1, &ssbo_obstacles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_obstacles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(int), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_ux);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_ux);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_uy);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_uy);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_u_out);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_out);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_rho);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rho);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_f);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 9 * width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_new_f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_new_f);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 9 * width * height * sizeof(float), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_boundary);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(int), NULL, GL_DYNAMIC_DRAW);


	// this procedure could be astracted away
	int x, y;
	memset(obstacles, 0, width * height * sizeof(int));
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
	lbm_reset_field(f, rho, ux, uy, width, height, obstacles);


	lbm_texture_id = texture_create(width, height);
	lbm_texture_buffer = (unsigned char *) malloc(3 * width * height * sizeof(unsigned char));


	cs_render_program = compute_program_load(cs_render_src);


	// @TODO: replicate in a programmatic way the sending of data to the gpu
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_obstacles);
	void *ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, obstacles, width * height * sizeof(int));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_out);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, u_out, width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_ux);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, ux, width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_uy);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, uy, width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rho);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, rho, width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_f);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, f, 9 * width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_new_f);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, new_f, 9 * width * height * sizeof(float));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary);
	ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr == NULL) {
		printf("got NULL pointer\n");
	}
	else {
		memcpy(ptr, obstacles, 4 * width * height * sizeof(int));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_obstacles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_u_out);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_obstacles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_u_out);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_ux);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_uy);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_rho);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 8, ssbo_new_f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 9, ssbo_boundary);


	glUseProgram(cs_render_program);
	glUniform2i(glGetUniformLocation(cs_render_program, "shape"), width, height);
}


void lbm_reload() {
	lbm_reset_field(f, rho, ux, uy, width, height, obstacles);
	it = 0;
}


void lbm_step() {
	const float u_in_now = u_in * (1.0 - exp(-(it * it) / double_square_sigma));

	lbm_substep1(width, height, it, u_in_now, omega_plus, sum_param, sub_param, f, new_f, rho, ux, uy, u_out, boundary, obstacles);
	lbm_substep2(width, height, f, new_f, obstacles);

	++it;
}


void lbm_write(FILE *out) {
	if (first_write) {
		fprintf(out, "%d %d\n", width, height);
		first_write = false;
	}

	fprintf(out, "%d\n", it);
	fwrite(u_out, sizeof(float), width * height, out);
}


void lbm_write_on_texture() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);


	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_out);
	void *ptr = glMapBufferRange(GL_SHADER_STORAGE_BUFFER, 0, width * height * sizeof(float), GL_MAP_WRITE_BIT);

	memcpy(ptr, u_out, width * height * sizeof(float));
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);


	glUseProgram(cs_render_program);
	glDispatchCompute(width, height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}
