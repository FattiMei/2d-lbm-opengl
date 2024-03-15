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


#ifdef LBM_INTERNALS
// static void lbm_write_on_file_internal();
// static void lbm_reset_field();
// static void lbm_populate_obstacles();
// static void lbm_calc_boundary();
// static void lbm_substep1(float u_in_now);
// static void lbm_substep2();
// static void lbm_allocate_resources();
// static void lbm_release_resources();
#endif


#endif
