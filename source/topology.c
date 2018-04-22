#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

/* **************************************************************************
 * Creates the cartesian topology -- currently returns the communicator due
 * to issues with me not knowing how to use pointers to return and use a
 * communicator
 * ************************************************************************** */

MPI_Comm
create_topology(int ndims, int *dims, int *dim_period, int *nbrs, int *coords,
    int nx, int ny, int *nx_proc, int *ny_proc, int *proc, int n_procs,
    int reorder, int displacement)
{
    int i;
    MPI_Comm cart_comm;

    for (i = 0; i < ndims; i++)
    {
        dims[i] = 0;
        dim_period[i] = 0;
    }

    MPI_Dims_create(n_procs, ndims, dims);
    MPI_Cart_create(DEFAULT_COMM, ndims, dims, dim_period, reorder, &cart_comm);
    MPI_Comm_rank(cart_comm, proc);
    MPI_Cart_coords(cart_comm, *proc, ndims, coords);
    MPI_Cart_shift(cart_comm, XDIR, displacement, &nbrs[LEFT], &nbrs[RIGHT]);
    MPI_Cart_shift(cart_comm, YDIR, displacement, &nbrs[DOWN], &nbrs[UP]);

    *nx_proc = (int) ceil((double) nx/dims[0]);

    if (ndims == 1)
    {
        *ny_proc = ny;  // legacy 1D option :-)
    }
    else if (ndims == 2)
    {
        *ny_proc = (int) ceil((double) ny/dims[1]);
    }
    else
    {
        printf("ndims %d is too large. Exiting.\n\n", ndims);
        MPI_Finalize();
        exit(1);
    }

    return cart_comm;
}
