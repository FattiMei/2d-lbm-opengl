# 2d-lbm-opengl

This project started as a fork of [2d-lbm-cuda](https://github.com/AndreaTorti-01/2d-lbm-cuda) and the initial development efforts were done [here](https://github.com/FattiMei/2d-lbm-cuda). Now it has become an independent identity with very ambitious goals.


## Goals
 * heavily refactor the original implementation to decouple the computation from the rendering
 * design a coherent API
 * allow multiple lbm implementations (cpu based, compute shaders, opencl)
 * port the application to opengl 4.6
 * add imgui integration


## Vision
I want to make scientific software that is portable, interactive and uses modern GPUs for real time rendering of experiments.


## Steps in development
 * ImGui interactivity
   - start/stop the iterations
   - reload the experiment
   - count the FPS
 * make rendering on the texture a method of the lbm class
 * optimize the cpu computation


## Dependencies
 * opengl es 2.0 (will eventually be ported to opengl 4.6)
 * glfw3 (libglfw3-dev for ubuntu and debian systems)


## Usage
`make run`


## Atomic bug
Right now there is a serious bug with the compiling and executing the application: every time I open the executable there is a chance to get a segfault and I don't know why. The next development steps will fix this issue, see you from the other side.
