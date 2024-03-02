#include <stdio.h>
#include <stdlib.h>
#include "lbm.h"


#define MAX_FRAMES_SAVED 40


int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Invalid command line arguments\n");
		fprintf(stderr, "Usage: ./headless <input filename> <binary output filename>\n");
		exit(EXIT_FAILURE);
	}


	FILE *in  = fopen(argv[1], "r");
	FILE *out = fopen(argv[2], "w");


	if (in == NULL) {
		fprintf(stderr, "Could not open input file %s\n", argv[1]);
		exit(EXIT_FAILURE);
	}


	if (out == NULL) {
		fprintf(stderr, "Could not open output file %s\n", argv[2]);
		fclose(in);
		exit(EXIT_FAILURE);
	}


	lbm_setup(in);


	for (int it = 0, frames_saved = 0; frames_saved < MAX_FRAMES_SAVED; ++it) {
		if (it % 100 == 0) {
			lbm_write(out);
			++frames_saved;
		}

		lbm_step(it);
	}


	fclose(out);


	return 0;
}