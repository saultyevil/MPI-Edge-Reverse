#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

int
main(int argc, char *argv[])
{
    int iter, n_iters, check_freq, out_freq, verbose;
    int nx, ny, nx_proc, ny_proc, proc, n_procs;
    double delta_stopping;
    char *in_filename = malloc(sizeof(*in_filename) * MAX_LINE);
    char *out_filename = malloc(sizeof(*out_filename) * MAX_LINE);

    MPI_Comm cart_comm;

    /*
     * Begin the main program
     */
    MPI_Init(NULL, NULL);
    MPI_Comm_rank(DEFAULT_COMM, &proc);
    MPI_Comm_size(DEFAULT_COMM, &n_procs);

    /*
     * Read in the parameters, filename etc, from an external config file, then
     * read in the image using pgmread provided by our friend David.
     */
    read_int("MAX_ITERS", &n_iters);
    read_int("CHECK_FREQ", &check_freq);
    read_int("OUTPUT_FREQ", &out_freq);
    read_int("VERBOSE", &verbose);
    read_double("DELTA", &delta_stopping);
    read_string("INPUT_FILENAME", in_filename);
    read_string("OUTPUT_FILENAME", out_filename);

    pgmsize(in_filename, &nx, &ny);
    double **master_buff = arralloc(sizeof(*master_buff), 2, nx, ny);

    if (proc == MASTER_PROCESS)
    {
        pgmread(in_filename, &master_buff[0][0], nx, ny);
        printf("\n----------------------\n");
        printf("INPUT_FILENAME: %s\nOUTPUT_FILENAME: %s\nRESOLUTION: %d x %d\n",
               in_filename, out_filename, nx, ny);
        printf("MAX_ITERS: %d\nCHECK_FREQ: %d\nOUTPUT_FREQ: %d\nDELTA: %4.2f\n",
               n_iters, check_freq, out_freq, delta_stopping);
        printf("VERBOSE: %d\nNDIMS: %d\nN_PROCS: %d\n", verbose, NDIMS,
               n_procs);
        printf("----------------------\n");
    }

    free(in_filename);

    /*
     * Calculate the boundaries for each process.
     * dims is initialised as being 0 otherwise MPI_Dims_create will likely
     * return garbage. The directions are non-periodic. A cartesian toplogy is
     * created and the neighbouring processes for a process are found using
     * MPI_Cart_shift
     */
    int reorder = 0, disp = 1;
    int *nbrs = malloc(sizeof(*nbrs) * N_NBRS);
    int *dims = malloc(sizeof(*dims) * NDIMS);
    int *dim_period = malloc(sizeof(*dim_period) * NDIMS);
    int *coords = malloc(sizeof(*coords) * NDIMS);
    int *bounds = malloc(sizeof(*bounds) * N_NBRS);

    /*
     * If NDIMS is too large or small, the program will exit in this function
     */
    cart_comm = create_topology(dims, dim_period, nbrs, coords, nx, ny,
                                &nx_proc, &ny_proc, &proc, n_procs,
                                reorder, disp);

    /*
     * Print out the dimensions and coordinates of each rank if verbose output
     * is used.
     */
    if (verbose == TRUE)
        print_dims_coords(dims, coords, proc, n_procs, cart_comm);

    /*
     * Allocate memory for all of the arrays -- use arralloc because it keeps
     * array elements contiguous
     */
    double **buff = arralloc(sizeof(*buff), 2, nx_proc, ny_proc);
    double **old = arralloc(sizeof(*old), 2, nx_proc+2, ny_proc+2);
    double **new = arralloc(sizeof(*new), 2, nx_proc+2, ny_proc+2);
    double **edge = arralloc(sizeof(*edge), 2, nx_proc+2, ny_proc+2);

    /*
     * Distribute masterbuff into smaller buffs for each process running
     */
    if (NDIMS == 1)
    {
        /*
         * For 1 dimension, just use the good ol' MPI_Scatter
         */
        MPI_Scatter(&master_buff[0][0], nx_proc*ny_proc, MPI_DOUBLE,
                    &buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS,
                    cart_comm);
    }
    else if (NDIMS == 2)
    {
        calculate_boundaries(bounds, coords, nx_proc, ny_proc, proc);

        if (verbose == TRUE)
            print_coord_boundaries(bounds, coords, proc, n_procs, cart_comm);
    }

    /*
     * Copy the buffer array (containing the image) to the edge array and
     * initialise the output image as being completely white -- this acts as
     * an initial guess and sets up halo boundary conditions
     */
    init_arrays(edge, old, buff, nx_proc, ny_proc);

    /*
     * Compute the iterations -- time them for parallel computation comparison
     */
    double parallel_start = MPI_Wtime();

    compute_iterations(old, new, edge, nbrs, nx_proc, ny_proc, proc, n_iters,
        delta_stopping, check_freq, out_freq, verbose, cart_comm);

    double parallel_end = MPI_Wtime();

    /*
     * Copy the image into the buffer -- excluding halo cells
     */
    copy_to_buff(old, buff, nx_proc, ny_proc);

    free(dims);
    free(dim_period);
    free(coords);
    free(bounds);
    free(nbrs);
    free(old);
    free(new);
    free(edge);

    /*
     * Gather all the buff's back into the masterbuff on the master process
     */
    if (NDIMS == 1)
    {
        /*
         * For 1 dimension, just use the good ol' MPI_Gather
         */
        MPI_Gather(&buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, &master_buff[0][0],
                   nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS, cart_comm);
    }
    else if (NDIMS == 2)
    {
        if (proc == MASTER_PROCESS)
        {
            printf("NDIMS: %d: not implemented yet!\n", NDIMS);
        }
    }

    MPI_Finalize();
    free(buff);

    if (proc == MASTER_PROCESS)
    {
        pgmwrite(out_filename, &master_buff[0][0], nx, ny);
        printf("Time required for image conversion: %9.6f seconds.\n\n",
            (parallel_end - parallel_start));
        free(master_buff);
    }

    free(out_filename);

    return 0;
}
