#ifndef __LBM_H__
#define __LBM_H__


#include <stdio.h>
#include <stdbool.h>


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
void lbm_hotfix_variables(int *external_width, int *external_height, float **external_u_out, bool **external_obstacles);
void lbm_step(int it);
void lbm_dump_solution(FILE *out, int it);


#endif
