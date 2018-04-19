#include <stdio.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

int
print_coords(int *coords, int proc, int n_procs, int ndims)
{
    int proc_iter, v_iter;

    for (proc_iter = 0; proc_iter < n_procs; proc_iter++)
    {
        if (proc == proc_iter)
        {
            printf("Proc %d: coords (", proc);
            for (v_iter = 0; v_iter < ndims; v_iter++)
            {
                printf(" %d ", coords[v_iter]);
            }
            printf(")\n");
        }
    }

    return 0;
}

int
print_dims(int *dims, int proc, int ndims)
{
    int i;

    if (proc == MASTER_PROCESS)
    {
        printf("Dims (");
        for (i = 0; i < ndims; i++)
        {
            printf(" %d ", dims[i]);
        }
        printf(")\n");
    }

    return 0;
}

int
print_dims_coords(int *dims, int *coords, int proc, int n_procs, MPI_Comm comm,
    int ndims)
{
    if ((proc == MASTER_PROCESS) && (ndims == 1))
        printf("\nCan produce some garbage output for ndims = 1.\n");

    print_dims(dims, proc, ndims);
    MPI_Barrier(comm);
    print_coords(coords, proc, n_procs, ndims);
    MPI_Barrier(comm);

    return 0;
}

int
print_n_proc(int proc, int nx, int ny)
{
    if (proc == MASTER_PROCESS)
    {
        printf("\nnx_proc %d ny_proc %d\n", nx, ny);
    }

    return 0;
}
