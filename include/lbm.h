#ifndef __LBM_H__
#define __LBM_H__


#include <stdio.h>


extern unsigned int lbm_texture_id;


void lbm_init(FILE *in);
void lbm_step();
void lbm_reload();
void lbm_write_on_texture();
void lbm_write_on_file(FILE *out);
void lbm_close();


#endif
