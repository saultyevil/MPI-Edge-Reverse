#include <stdio.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

int
print_coords(int *coords, int proc, int n_procs)
{
    int proc_iter, v_iter;

    for (proc_iter = 0; proc_iter < n_procs; proc_iter++)
    {
        if (proc == proc_iter)
        {
            printf("Proc %d: coords (", proc);
            for (v_iter = 0; v_iter < NDIMS; v_iter++)
            {
                printf(" %d ", coords[v_iter]);
            }
            printf(")\n");
        }
    }

    return 0;
}

int
print_dims(int *dims, int proc)
{
    int i;

    if (proc == MASTER_PROCESS)
    {
        printf("\nDims (");
        for (i = 0; i < NDIMS; i++)
        {
            printf(" %d ", dims[i]);
        }
        printf(")\n");
    }

    return 0;
}

int
print_boundaries(int *bounds, int *coords, int proc, int n_procs)
{
    int proc_iter, v_iter;

    for (proc_iter = 0; proc_iter < n_procs; proc_iter++)
    {
        if (proc == proc_iter)
        {
            printf("proc %d bounds[LEFT] %d bounds[RIGHT] %d bounds[UP] %d ",
                   proc, bounds[LEFT], bounds[RIGHT], bounds[UP]);
            printf("bounds[DOWN] %d\n", bounds[DOWN]);
        }
    }

    return 0;
}

int
print_dims_coords(int *dims, int *coords, int proc, int n_procs, MPI_Comm comm)
{
    if ((proc == MASTER_PROCESS) && (NDIMS == 1))
        printf("\nWill produce some garbage output for NDIMS = 1.\n");

    print_dims(dims, proc);
    MPI_Barrier(comm);
    print_coords(coords, proc, n_procs);
    MPI_Barrier(comm);

    return 0;
}

int
print_coord_boundaries(int *bounds, int *coords, int proc, int n_procs,
    MPI_Comm comm)
{
    print_boundaries(bounds, coords, proc, n_procs);
    MPI_Barrier(comm);

    return 0;
}
