#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <math.h>
#include <string.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"
#include "lbm.h"
#include "window.h"
#include "experiment.h"


#define MAX_FRAMES_SAVED 40


int main(int argc, char *argv[]) {
	if (argc != 3) {
		fprintf(stderr, "Invalid command line arguments\n");
		fprintf(stderr, "Usage: serial <input filename> <output filename>\n");

		return -1;
	}


	const char *input_filename = argv[1];
	const char *output_filename = argv[2];


	FILE *in  = fopen(input_filename,  "r");
	std::ofstream out(output_filename, std::ios::binary);


	if (in == NULL) {
		fprintf(stderr, "Could not open input file %s\n", input_filename);
		return 1;
	}


	Lbm lbm(in);


	int window_width = 800;
	int window_height = 600;

	if (window_init("lbm on opengl", window_width, window_height) != 0) {
		window_close();
		exit(EXIT_FAILURE);
	}

	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO(); (void) io;
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;

	ImGui::StyleColorsDark();
	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init("#version 100");

	experiment_init(window_width, window_height);
	window_set_callbacks();

	int frames_saved = 0;
	(void) frames_saved;

	while (!window_should_close()) {
		glfwGetFramebufferSize(window, &window_width, &window_height);

//		if (lbm.get_frame_count() % 100 == 0) {
//			if (frames_saved < MAX_FRAMES_SAVED) {
//				lbm.write(out);
//				++frames_saved;
//			}
//			else {
//				break;
//			}
//		}


		ImGui_ImplOpenGL3_NewFrame();
		ImGui_ImplGlfw_NewFrame();
		ImGui::NewFrame();

		experiment_resize(window_width, window_height);
		experiment_render();
		lbm.step();

		ImGui::ShowDemoWindow();

		{
			ImGui::Begin("lbm debug window");
			ImGui::Text("Application average %.3f ms/frame (%.1f FPS)", 1000.0f / io.Framerate, io.Framerate);
			ImGui::End();
		}

		ImGui::Render();
		ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

		window_swap_buffers();
		window_poll_events();
	}

	ImGui_ImplOpenGL3_Shutdown();
	ImGui_ImplGlfw_Shutdown();
	ImGui::DestroyContext();

	window_close();

	return 0;
}
