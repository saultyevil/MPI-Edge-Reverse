# Parallel Image Processing

## Description

The aim of this program is to covert an image of edge data into its original image by iteratively computing the graident of a pixel compared to its four neighbouring pixels. Input images are expected to be in the `.pgm` format and as such, output images will also be output in this format.

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

## Configuration File

* `MAX_ITERS` - the maximum number of iterations the program should compute for.
* `OUTPUT_FREQ` - how often a progress report should be printed to screen. This is also the frequency at which the program checks to see if the image has been converted sucessfully.
* `DELTA` - the criterion for the maximum change between two iterations for the conversion processes to stop.
* `INPUT_FILENAME` - the path to the edge image.
* `OUTPUT_FILENAME` - the path to where the converted image should be saved.
