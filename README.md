# Parallel Image Processing

## Description

The aim of this program is to covert an image of edge data into its original image by iteratively computing the gradient of a pixel compared to its four neighbouring pixels. Input images are expected to be in the `.pgm` format and as such, output images will also be output in this format.

The input image is decomposed into a 2D array of processes, each computing their own iterations to convert the image back to its original. Users can set a maximum number of iterations to covert back to the original image, however the program will end when it has detected that the difference between iterations is smaller than the defined criterion `DELTA`.

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

Or, in parallel,

```bash
$ mpirun -n np edge2image
```

Where `np` is the number of processes to be used.

Users are able to switch between 1D decomposition and 2D decomposition by changing the constant `NDIMS` in the file `source/image_constants.h`. However, it is recommended to recompile all of the source if you are changing `NDIMS`, rather than just the single `image.c` file, i.e.,

```bash
$ make clean
$ make
```

## Configuration File

* MAX_ITERS - the maximum number of iterations the program should do before exiting.
* CHECK_FREQ - the frequency the program should check the pixel values between iterations.
* OUTPUT_FREQ - the frequency at which progress updates should be output.
* DELTA - the desired maximum difference between two iterations for the program to exit.
* VERBOSE - enabling this with 1 will output extra information.
* INPUT_FILENAME - path to the file to be converted.
* OUTPUT_FILEAME - path to where to store the converted image.

## Number of Processes

If you have compiled with `NDIMS=1`, you will be limited to the number of processes you can used. This is due to the use of `MPI_Scatter` to scatter the work across processes. In general, it's better to use `NDIMS=2` which can use an arbitrary number of processes. However, if you are hard set on using `NDIMS=1`, consider using the following number of processes:

```
192 x 128: 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 192.
256 x 192: 1, 2, 4, 8, 16, 32, 64, 128, 256.
512 x 384: 1, 2, 4, 8, 16, 32, 64, 128, 256, 512.
768 x 768: 1, 2, 3, 4, 6, 8, 12, 16, 24, 32, 48, 64, 96, 128, 192, 256, 384, 768.
```
