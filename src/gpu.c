#include "lbm.h"
#include "texture.h"
#include "shader.h"
#include "glad.h"
#include <stdlib.h>
#include <stdbool.h>
#include <math.h>
#include <string.h>


int width;
int height;
unsigned int lbm_texture_id;


static int it = 0;
static bool first_write = true;


static unsigned int cs_render_program;
static unsigned int cs_substep1_program;
static unsigned int cs_substep2_program;


static unsigned int ssbo_obstacles;
static unsigned int ssbo_u_out;
static unsigned int ssbo_ux;
static unsigned int ssbo_uy;
static unsigned int ssbo_rho;
static unsigned int ssbo_f;
static unsigned int ssbo_new_f;
static unsigned int ssbo_boundary;


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
static float *u_out,
	     *ux,
	     *uy,
	     *f,
	     *new_f,
	     *rho;


// this variable was a bool, now it has become an int and we will use the possible values to store information about obstacles and walls in a bitfield
// since we can't have byte data type in glsl we must declare obstacles as int
#define IS_OBSTACLE 1
#define TOP_WALL 2
#define BOTTOM_WALL 4
#define LEFT_WALL 8
#define RIGHT_WALL 16

static int *obstacles;


static void lbm_reset_field(float f[], float rho[], float u_out[], float ux[], float uy[], const int width, const int height, const int obstacles[]) {
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

// @TODO: boundary can encode the obstacle information, while being memory efficient
static void lbm_calc_boundary(int boundary[], const int obstacles[], const int width, const int height) {
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


void lbm_allocate_resources() {
	obstacles = malloc(    width * height * sizeof(*obstacles));
	ux        = malloc(    width * height * sizeof(*ux));
	uy        = malloc(    width * height * sizeof(*uy));
	u_out     = malloc(    width * height * sizeof(*u_out));
	rho       = malloc(    width * height * sizeof(*rho));
	f         = malloc(9 * width * height * sizeof(*f));
	new_f     = malloc(9 * width * height * sizeof(*new_f));
	boundary  = malloc(4 * width * height * sizeof(*boundary));


	// allocate this memory on the gpu (2MB of data)
	// @TODO: avoid this replication
	glGenBuffers(1, &ssbo_obstacles);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_obstacles);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(*obstacles), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_ux);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_ux);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(*ux), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_uy);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_uy);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(*uy), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_u_out);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_out);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(*u_out), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_rho);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_rho);
	glBufferData(GL_SHADER_STORAGE_BUFFER, width * height * sizeof(*rho), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_f);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 9 * width * height * sizeof(*f), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_new_f);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_new_f);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 9 * width * height * sizeof(*new_f), NULL, GL_DYNAMIC_DRAW);

	glGenBuffers(1, &ssbo_boundary);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_boundary);
	glBufferData(GL_SHADER_STORAGE_BUFFER, 4 * width * height * sizeof(*boundary), NULL, GL_DYNAMIC_DRAW);
}


void lbm_release_resources() {

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
	memset(obstacles, 0, width * height * sizeof(*obstacles));
	while (fscanf(in, "%d %d\n", &x, &y) == 2) {
		obstacles[x + y * width] |= IS_OBSTACLE;
	}
	fclose(in);


	// populate obstacles wall information
	const int size = width * height;

	for (int i = 0; i < width; ++i) {
		obstacles[i]            |= TOP_WALL;
		obstacles[size - 1 - i] |= BOTTOM_WALL;
	}


	for (int j = 0; j < height; ++j) {
		obstacles[j * width] |= LEFT_WALL;
		obstacles[j * width + width - 1] |= RIGHT_WALL;
	}


	lbm_calc_boundary(boundary, obstacles, width, height);
	lbm_reset_field(f, rho, u_out, ux, uy, width, height, obstacles);


	lbm_texture_id = texture_create(width, height);


	cs_render_program = compute_program_load_from_file("shaders/cs_render.glsl");
	cs_substep1_program = compute_program_load_from_file("shaders/cs_substep1.glsl");
	cs_substep2_program = compute_program_load_from_file("shaders/cs_substep2.glsl");


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
		memcpy(ptr, boundary, 4 * width * height * sizeof(int));
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	}


	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, ssbo_obstacles);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 1, ssbo_u_out);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 2, ssbo_ux);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 3, ssbo_uy);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 4, ssbo_rho);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 5, ssbo_f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 6, ssbo_new_f);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 7, ssbo_boundary);


	// passing uniforms that will be constant in all execution
	glUseProgram(cs_render_program);
	glUniform2i(glGetUniformLocation(cs_render_program, "shape"), width, height);

	glUseProgram(cs_substep1_program);
	glUniform2i(0, width, height);
	glUniform1f(2, omega_plus);
	glUniform1f(3, sum_param);
	glUniform1f(4, sub_param);

	glUseProgram(cs_substep2_program);
	glUniform2i(0, width, height);
}


void lbm_reload() {
	lbm_reset_field(f, rho, u_out, ux, uy, width, height, obstacles);
	it = 0;
}


void lbm_step() {
	const float u_in_now = u_in * (1.0 - exp(-(it * it) / double_square_sigma));

	glUseProgram(cs_substep1_program);
	glUniform1f(1, u_in_now);
	glDispatchCompute(width * height, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	glUseProgram(cs_substep2_program);
	glDispatchCompute(width * height, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

	++it;
}


void lbm_write_on_file(FILE *out) {
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

	glUseProgram(cs_render_program);
	glDispatchCompute(width, height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


// @TODO: fill
void lbm_close() {

}
