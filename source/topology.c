#include <math.h>
#include <mpi.h>

#include "image.h"

/* **************************************************************************
 * Creates the cartesian topology -- currently returns the communicator due
 * to issues with me not knowing how to use pointers to return and use a
 * communicator
 * ************************************************************************** */

MPI_Comm
create_topology(int *dims, int *dim_period, int *nbrs, int nx, int ny,
    int *nx_proc, int *ny_proc, int *proc, int n_procs, int reorder,
    int displacement)
{
    int i;
    MPI_Comm cart_comm;

    for (i = 0; i < NDIMS; i++)
    {
        dims[i] = 0;
        dim_period[i] = 0;
    }

    MPI_Dims_create(n_procs, NDIMS, dims);
    MPI_Cart_create(DEFAULT_COMM, NDIMS, dims, dim_period, reorder, &cart_comm);
    MPI_Comm_rank(cart_comm, proc);
    MPI_Cart_shift(cart_comm, XDIR, displacement, &nbrs[LEFT], &nbrs[RIGHT]);
    MPI_Cart_shift(cart_comm, YDIR, displacement, &nbrs[DOWN], &nbrs[UP]);

    *nx_proc = (int) ceil((double) nx/dims[0]);
    if (NDIMS == 1)
        *ny_proc = ny;  // legacy debug option :-)
    else
        *ny_proc = (int) ceil((double) ny/dims[1]);

    return cart_comm;
}
