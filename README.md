# Mandelbrot Set Generator

This program generates the Mandelbrot set, a famous fractal in mathematics. It is coded completely in C.

## Installation

To compile the code, follow these steps:

1. Navigate to the project directory: `cd mandelbrot`
2. Compile the code: `gcc mandelbrot.c -o mandelbrot`
3. Run the program, run `./mandelbrot [img_dim] [engine_count] [UL_X] [UL_Y]`, according to the following specifications:
    - img_dim: the width and height of image, in pixels
    - engine_count: how many engines we create
    - UL_X: the top left corner x-coordinate for the complex plane
    - UL_Y: the top left corner y-coordinate for the complex plane
    - mandel_dim: the width/height of the mandelbrot set, such that the bottom right corner is (UL_X + mandel_dim, UL_Y + mandel_dim)

## Generating Output
All output is placed by default into the mandeloutput.bmp file.
mandelbrotoutput.bmp is already included in the repository as pre-filled example of a working print.