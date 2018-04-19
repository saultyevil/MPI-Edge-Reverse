# Parallel Image Processing

## Description

The aim of this program is to covert an image of edge data into its original image by iteratively computing the gradient of a pixel compared to its four neighbouring pixels. Input images are expected to be in the `.pgm` format and as such, output images will also be output in this format.

The input image is decomposed into a 2D array of processes, each computing their own iterations to convert the image back to its original. Users can set a maximum number of iterations to covert back to the original image, however the program will end when it has detected that the difference between iterations is smaller than a defined criterion.

## Building

To build the program, use the included Makefile. You may need to edit the Makefile's C compiler to be able to build on your machine, i.e. use `mpicc` on your laptop/PC. To invoke the Makefile and build the program, type the following into a terminal window.

```bash
$ make
```

## Usage

To use program once it is built, you will need to configure the configuration file `edge2image.ini`. You should only need to change the parameters and update the path to the input and output images. Test images are included in the directory `edge_images`. The program can then be called as follows.

```bash
$ ./edge2image
```

Users are able to switch between 1-D decomposition and 2-D decomposition by changing the constant `NDIMS` in the file `source/image_constants.h`. However, for this to be successful it is recommended to recompile all of the source (my Makefile isn't smart enough to compensate for this... yet.).

```bash
$ make clean
$ make
```

## Configuration File

* `MAX_ITERS` - the maximum number of iterations to compute.
* `CHECK_FREQ` - how often the program should check to see if the program has converted the image to a specified accuracy.
* `OUTPUT_FREQ` - how often the progress report should print, i.e. how often it should print how many iterations are complete.
* `DELTA` - the stopping criterion. If the largest difference between two pixels is smaller than this value, the image has reached the desired accuracy between iterations.
* `INPUT_FILENAME` - the path to the input image.
* `OUTPUT_FILENAME` - the path to the output image.
* `VERBOSE` - enable to see more information printed to the screen.

## Acceptable process number

In general it is best to use either 1, 2, 4, 8, 16, 32 or 64 processes. 128 can also work for the bigger images provided. Check the list below for more available process numbers for each image.

```
x[0] = 192.0 y[0] = 128.0: both divisible by 1
x[0] = 192.0 y[0] = 128.0: both divisible by 2
x[0] = 192.0 y[0] = 128.0: both divisible by 4
x[0] = 192.0 y[0] = 128.0: both divisible by 8
x[0] = 192.0 y[0] = 128.0: both divisible by 16
x[0] = 192.0 y[0] = 128.0: both divisible by 32
x[0] = 192.0 y[0] = 128.0: both divisible by 64

x[1] = 256.0 y[1] = 192.0: both divisible by 1
x[1] = 256.0 y[1] = 192.0: both divisible by 2
x[1] = 256.0 y[1] = 192.0: both divisible by 4
x[1] = 256.0 y[1] = 192.0: both divisible by 8
x[1] = 256.0 y[1] = 192.0: both divisible by 16
x[1] = 256.0 y[1] = 192.0: both divisible by 32
x[1] = 256.0 y[1] = 192.0: both divisible by 64

x[2] = 512.0 y[2] = 384.0: both divisible by 1
x[2] = 512.0 y[2] = 384.0: both divisible by 2
x[2] = 512.0 y[2] = 384.0: both divisible by 4
x[2] = 512.0 y[2] = 384.0: both divisible by 8
x[2] = 512.0 y[2] = 384.0: both divisible by 16
x[2] = 512.0 y[2] = 384.0: both divisible by 32
x[2] = 512.0 y[2] = 384.0: both divisible by 64
x[2] = 512.0 y[2] = 384.0: both divisible by 128

x[3] = 768.0 y[3] = 768.0: both divisible by 1
x[3] = 768.0 y[3] = 768.0: both divisible by 2
x[3] = 768.0 y[3] = 768.0: both divisible by 3
x[3] = 768.0 y[3] = 768.0: both divisible by 4
x[3] = 768.0 y[3] = 768.0: both divisible by 6
x[3] = 768.0 y[3] = 768.0: both divisible by 8
x[3] = 768.0 y[3] = 768.0: both divisible by 12
x[3] = 768.0 y[3] = 768.0: both divisible by 16
x[3] = 768.0 y[3] = 768.0: both divisible by 24
x[3] = 768.0 y[3] = 768.0: both divisible by 32
x[3] = 768.0 y[3] = 768.0: both divisible by 48
x[3] = 768.0 y[3] = 768.0: both divisible by 64
x[3] = 768.0 y[3] = 768.0: both divisible by 96
x[3] = 768.0 y[3] = 768.0: both divisible by 128
x[3] = 768.0 y[3] = 768.0: both divisible by 192
x[3] = 768.0 y[3] = 768.0: both divisible by 256
x[3] = 768.0 y[3] = 768.0: both divisible by 384
x[3] = 768.0 y[3] = 768.0: both divisible by 768
```
