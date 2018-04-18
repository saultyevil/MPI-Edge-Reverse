#include <stdio.h>

#include "image_constants.h"
#include "image_functions.h"

int
calculate_boundaries(int *bounds, int *proc_coords, int nx_proc, int ny_proc,
    int proc)
{
    bounds[LEFT] = proc_coords[0] * nx_proc;          // left x boundary
    bounds[RIGHT] = (proc_coords[0] + 1) * nx_proc;   // right x boundary
    bounds[UP] = proc_coords[1] * ny_proc;            // top y boundary
    bounds[DOWN] = (proc_coords[1] + 1) * ny_proc;    // bottom y boundary

    return 0;
}
