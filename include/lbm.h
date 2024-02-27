#ifndef __LBM_H__
#define __LBM_H__


#include <cstdio>


extern int width,
	   height,
	   max_it;


extern float reynolds,
	     u_in;


extern float nu,
	     tau,
	     sigma,
	     double_square_sigma,
	     lambda_trt,
	     tau_minus,
	     omega_plus,
	     omega_minus,
	     sub_param,
	     sum_param;


extern int *boundary;
extern bool *obstacles;
extern float *ux,
	     *uy,
	     *f,
	     *new_f,
	     *rho,
	     *u_out;


void lbm_init(
	  float f[]
	, float rho[]
	, float ux[]
	, float uy[]
	, const int width
	, const int height
	, const bool obstacles[]
);


void lbm_calc_boundary(
	  int boundary[]
	, const bool obstacles[]
	, const int width
	, const int height
);


void lbm_setup(FILE *in);
void lbm_step(int it);
void lbm_dump_solution(FILE *out, int it);


class Lbm {
	public:
		Lbm(int width_, int height_, float reynolds_, float inlet_, bool obstacles_[]);
		Lbm(FILE *in);
		void step();
		void write(FILE *out);
		int get_frame_count();

	private:
		int it;
		bool first_write = true;
};


#endif
