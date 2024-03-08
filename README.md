# 2d-lbm-opengl

This project started as a fork of [2d-lbm-cuda](https://github.com/AndreaTorti-01/2d-lbm-cuda) and the initial development efforts were done [here](https://github.com/FattiMei/2d-lbm-cuda). Now it has become an independent identity with very ambitious goals.


## Goals
 * heavily refactor the original implementation to decouple the computation from the rendering
 * design a coherent API
 * allow multiple lbm implementations (cpu based, compute shaders, opencl)
 * port the application to opengl 4.3
 * add imgui integration


## Vision
I want to make scientific software that is portable, interactive and uses modern GPUs for real time rendering of experiments.


## Interoperability
It is particularly difficult to port the lbm functions to compute shaders. Here is a list of problems:
 * cumbersome declaration of buffer objects
 * parameters of kernels have to be manually passed as uniforms
 * kernel compilation errors are obscure


## Steps in development
 * Port the lbm computation to compute shaders (not fun)
 * OpenGL <-> OpenCL interop (no thank you)
 * Experiment with different work group size to maximize performance
 * ImGui interactivity (paused until compute shaders are solved)
   - start/stop the iterations (already implemented)
   - reload the experiment (yes)
   - count the FPS


## Dependencies
 * opengl 4.3
 * glfw3 (libglfw3-dev for ubuntu and debian systems)


## Usage
Out of the box: `make run`
