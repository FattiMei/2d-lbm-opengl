#ifndef __LBM_H__
#define __LBM_H__


#include <stdio.h>


extern int width;
extern int height;
extern float *u_out;


void lbm_setup(FILE *in);
void lbm_step();
void lbm_write(FILE *out);

/*

class Lbm {
	public:
		Lbm(int width_, int height_, float reynolds_, float inlet_, bool obstacles_[]);
		Lbm(FILE *in);
		void step();
		void render(int window_width, int window_height);
		void write(std::ofstream &out);
		int get_frame_count();
		void debug(std::ostream &out = std::cout);

	private:
		int it;
		bool first_write = true;
		std::unique_ptr<Texture> texture;

		void render_on_texture();
};
*/


#endif
