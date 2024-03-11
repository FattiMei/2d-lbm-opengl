#ifndef __LBM_H__
#define __LBM_H__


#include <stdio.h>


extern int width;
extern int height;
extern float *u_out;
extern unsigned int lbm_texture_id;


void lbm_init(FILE *in);
void lbm_step();
void lbm_reload();
void lbm_write_on_texture();
void lbm_write_on_file(FILE *out);
void lbm_close();


#ifdef LBM_IMPLEMENTATION

void lbm_reset_field();
void lbm_calc_boundary();
void lbm_substep1();
void lbm_substep2();
void lbm_allocate_resources();
void lbm_release_resources();
void lbm_write_on_file_internal(FILE *out);

#endif


#endif
