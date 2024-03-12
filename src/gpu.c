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


static unsigned int cs_render_program;
static unsigned int cs_substep1_program;
static unsigned int cs_substep2_program;
static unsigned int cs_reset_field_program;

static unsigned int ssbo_obstacles;
static unsigned int ssbo_u_out;
static unsigned int ssbo_ux;
static unsigned int ssbo_uy;
static unsigned int ssbo_rho;
static unsigned int ssbo_f;
static unsigned int ssbo_new_f;
static unsigned int ssbo_boundary;


struct BufferInfo {
	unsigned int *id;
	int size_multiplier;
	void **host_counterpart;
	GLenum usage;
};


// @TODO: experiment with usage flag of obstacles and boundary (readonly)
const struct BufferInfo buffers[] = {
	{&ssbo_obstacles, 1 * sizeof(*obstacles), (void **) &obstacles, GL_DYNAMIC_DRAW},
	{&ssbo_u_out    , 1 * sizeof(*u_out)    , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_ux       , 1 * sizeof(*ux)       , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_uy       , 1 * sizeof(*uy)       , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_rho      , 1 * sizeof(*rho)      , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_f        , 9 * sizeof(*f)        , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_new_f    , 9 * sizeof(*new_f)    , NULL                , GL_DYNAMIC_DRAW},
	{&ssbo_boundary , 4 * sizeof(*boundary) , (void **) &boundary , GL_DYNAMIC_DRAW}
};



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
	boundary  = malloc(4 * width * height * sizeof(*boundary));


	// allocate this memory on the gpu (in the order of 2MB of data)
	for (size_t i = 0; i < sizeof(buffers) / sizeof(*buffers); ++i) {
		glGenBuffers(1, buffers[i].id);
		glBindBufferBase(GL_SHADER_STORAGE_BUFFER, i, *(buffers[i].id));
		glBindBuffer(GL_SHADER_STORAGE_BUFFER, *(buffers[i].id));
		glBufferData(GL_SHADER_STORAGE_BUFFER, buffers[i].size_multiplier * width * height, NULL, buffers[i].usage);
	}
}


void lbm_release_resources() {
	free(obstacles);
	free(boundary);

	for (size_t i = 0; i < sizeof(buffers) / sizeof(*buffers); ++i) {
		glDeleteBuffers(1, buffers[i].id);
	}

	glDeleteProgram(cs_render_program);
	glDeleteProgram(cs_substep1_program);
	glDeleteProgram(cs_substep2_program);
	glDeleteProgram(cs_reset_field_program);
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
	lbm_texture_id = texture_create(width, height);


	cs_render_program = compute_program_load_from_file("shaders/cs_render.glsl");
	cs_substep1_program = compute_program_load_from_file("shaders/cs_substep1.glsl");
	cs_substep2_program = compute_program_load_from_file("shaders/cs_substep2.glsl");
	cs_reset_field_program = compute_program_load_from_file("shaders/cs_reset_field.glsl");


	// sending obstacles and boundary data to gpu
	for (size_t i = 0; i < sizeof(buffers) / sizeof(*buffers); ++i) {
		if (buffers[i].host_counterpart != NULL) {
			glBindBuffer(GL_SHADER_STORAGE_BUFFER, *(buffers[i].id));

			void *ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);
			if (ptr) {
				memcpy(ptr, *(buffers[i].host_counterpart), buffers[i].size_multiplier * width * height);
			}

			glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
		}
	}


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

	glUseProgram(cs_reset_field_program);
	glUniform2i(0, width, height);
	glDispatchCompute(width * height, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}


void lbm_reload() {
	glUseProgram(cs_reset_field_program);
	glDispatchCompute(width * height, 1, 1);
	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

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

	glBindBuffer(GL_SHADER_STORAGE_BUFFER, ssbo_u_out);
	void *ptr = glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_WRITE_ONLY);

	if (ptr) {
		fwrite(ptr, sizeof(float), width * height, out);
	}

	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
}


void lbm_write_on_texture() {
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, lbm_texture_id);

	glUseProgram(cs_render_program);
	glDispatchCompute(width, height, 1);
	glMemoryBarrier(GL_SHADER_IMAGE_ACCESS_BARRIER_BIT);
}


void lbm_close() {
	lbm_release_resources();
}
