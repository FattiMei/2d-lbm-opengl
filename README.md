# 2d-lbm-cuda


## Why this fork
This fork is an exercise in software development:
 * reading source code
 * understanding the design choices of @AndreaTorti-01
 * writing a CPU version of CUDA code
 * (explore openMP optimizations)
 * render the experiment results in real time with openGL
 * porting the CUDA code to openGL shaders (or openCL)
 * tinker the parameters of the simulation in real time


## Steps
 1. Makefile
 2. Producing reference output on a CUDA machine (google colab environment)
 3. Write equivalent serial implementation
 4. Make the serial implementation more readable
 5. Restore (and maybe refactor) the CUDA implementation to its initial state
 6. Understand the numerical difference between serial and CUDA implementation
 7. Think about the rendering part


## Dependencies
 * opengl es 2.0 (will eventually be ported to opengl 4.6)
 * glfw (libglfw-dev)


## Usage
`make run`


## Output file format
Plain text and binary format:

```
width height
iteration_count
[width * height fp32 values]
iteration_count
[width * height fp32 values]
...
```

Since the floating point values represent the state in a 2D matrix, we have to care about the row-major / column-major (TODO)


## Discrepancy between serial and CUDA version
From the A/B testing I found that the `step1` function yields different results for CUDA and serial versions, I suspect that this is due to fp32 to fp64 implicit conversions in the computation. The initial error is in the order of 1e-07 and it grows at a steady pace (expected since the computation a somewhat unstable process). There will be no effort in further studies about this topic.


## Rendering
This is the most difficult part of the application, but the concept is very simple: when an iteration of lbm is finished, the `u_out` array is mapped to a texture (we manually have to color the texels) and the texture is rendered by the standard combination of vertex + fragment shaders. Eventually the rendering on texture will be done by compute shaders, substituting the serial lbm code.
