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
void lbm_write(FILE *out);
void lbm_write_on_texture();


#endif
