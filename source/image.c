#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image.h"

int
main(int argc, char *argv[])
{
    int i, j, iter, n_iters, out_freq, proc, n_procs;
    int nx, ny, nx_proc, ny_proc;
    int *dims, *dim_period, *nbrs;

    char *in_filename = malloc(sizeof(*in_filename) * MAX_LINE);
    char *out_filename = malloc(sizeof(*out_filename) * MAX_LINE);

    double **buff, **master_buff;
    double **old, **new, **edge;

    MPI_Comm cart_comm;
    MPI_Status recv_status;
    MPI_Request proc_request;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(DEFAULT_COMM, &proc);
    MPI_Comm_size(DEFAULT_COMM, &n_procs);

    /*
     * Read in the parameters, filename etc, from an external config file, then
     * read in the image using pgmread provided by our friend David.
     */
    read_int("MAX_ITERS", &n_iters);
    read_int("OUTPUT_FREQ", &out_freq);
    read_string("INPUT_FILENAME", in_filename);
    read_string("OUTPUT_FILENAME", out_filename);
    pgmsize(in_filename, &nx, &ny);

    if (proc == MASTER_PROCESS)
    {
        master_buff = arralloc(sizeof(*master_buff), 2, nx, ny);
        pgmread(in_filename, &master_buff[0][0], nx, ny);
        printf("\n----------------------\n");
        printf("INPUT_FILENAME: %s\nOUTPUT_FILENAME: %s\nRESOLUTION: %d x %d\n",
               in_filename, out_filename, nx, ny);
        printf("MAX_ITERS: %d\nOUTPUT_FREQ: %d\nN_PROCS: %d\n", n_iters,
               out_freq, n_procs);
        printf("----------------------\n\n");
    }

    /*
     * Calculate the boundaries for each process.
     * dims is initialised as being 0 otherwise MPI_Dims_create will likely
     * return garbage. The directions are non-periodic. A cartesian toplogy is
     * created and the neighbouring processes for a process are found using
     * MPI_Cart_shift
     */
    int reorder = 0, disp = 1;
    nbrs = malloc(sizeof(*nbrs) * N_NBRS);
    dims = malloc(sizeof(*dims) * NDIMS);
    dim_period = malloc(sizeof(*dim_period) * NDIMS);

    for (i = 0; i < NDIMS; i++)
    {
        dims[i] = 0;
        dim_period[i] = 0;
    }

    MPI_Dims_create(n_procs, NDIMS, dims);
    MPI_Cart_create(DEFAULT_COMM, NDIMS, dims, dim_period, reorder, &cart_comm);
    MPI_Comm_rank(cart_comm, &proc);
    MPI_Cart_shift(cart_comm, XDIR, disp, &nbrs[LEFT], &nbrs[RIGHT]);
    MPI_Cart_shift(cart_comm, YDIR, disp, &nbrs[DOWN], &nbrs[UP]);

    nx_proc = (int) ceil((double) nx/dims[0]);
    ny_proc = (int) ceil((double) ny/dims[1]);

    /*
     * Allocate memory for all of the arrays -- use arralloc because it keeps
     * array elements contiguous
     */
    buff = arralloc(sizeof(*buff), 2, nx_proc, ny_proc);
    old = arralloc(sizeof(*old), 2, nx_proc+2, ny_proc+2);
    new = arralloc(sizeof(*new), 2, nx_proc+2, ny_proc+2);
    edge = arralloc(sizeof(*edge), 2, nx_proc+2, ny_proc+2);

    /*
     * DON'T USE MPI SCATTER!!!!
     */
    MPI_Scatter(&master_buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, &buff[0][0],
                nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS, cart_comm);

    /*
     * Copy the buffer array (containing the image) to the edge array and
     * initialise the output image as being completely white -- this acts as
     * an initial guess and sets up halo boundary conditions
     */
    for (i = 1; i < nx_proc+1; i++)
    {
        for (j = 1; j < ny_proc+1; j++)
        {
            edge[i][j] = buff[i-1][j-1];
        }
    }

    for (i = 0; i < nx_proc+2; i++)
    {
        for (j = 0; j < ny_proc+2; j++)
        {
            old[i][j] = 255.0;
        }
    }

    if (proc == MASTER_PROCESS)
        printf("\n---- BEGINNING ITERATIONS ----\n\n");

    for (iter = 1; iter <= n_iters; iter++)
    {
        /*
         * Send and recieve halo cells data from left and right
         * neighbouring processes
         */
        MPI_Issend(&old[nx_proc][1], ny_proc, MPI_DOUBLE, nbrs[RIGHT],
                   DEFAULT_TAG, cart_comm, &proc_request);
        MPI_Recv(&old[0][1], ny_proc, MPI_DOUBLE, nbrs[LEFT], DEFAULT_TAG,
                 cart_comm, &recv_status);
        MPI_Wait(&proc_request, &recv_status);

        MPI_Issend(&old[1][1], ny_proc, MPI_DOUBLE, nbrs[LEFT],
                   DEFAULT_TAG, cart_comm, &proc_request);
        MPI_Recv(&old[nx_proc+1][1], ny_proc, MPI_DOUBLE, nbrs[RIGHT],
                 DEFAULT_TAG, cart_comm, &recv_status);
        MPI_Wait(&proc_request, &recv_status);

        /*
         * Send and recieve halo cells data from up and down
         * neighbouring processes
         */
        MPI_Issend(&old[1][ny_proc], nx_proc, MPI_DOUBLE, nbrs[UP],
                   DEFAULT_TAG, cart_comm, &proc_request);
        MPI_Recv(&old[1][0], nx_proc, MPI_DOUBLE, nbrs[DOWN],
                 DEFAULT_TAG, cart_comm, &recv_status);
        MPI_Wait(&proc_request, &recv_status);

        MPI_Issend(&old[1][1], nx_proc, MPI_DOUBLE, nbrs[DOWN],
                   DEFAULT_TAG, cart_comm, &proc_request);
        MPI_Recv(&old[1][ny_proc+1], nx_proc, MPI_DOUBLE, nbrs[UP],
                 DEFAULT_TAG, cart_comm, &recv_status);
        MPI_Wait(&proc_request, &recv_status);

        for (i = 1; i < nx_proc+1; i++)
        {
            for (j = 1; j < ny_proc+1; j++)
            {
                new[i][j] = 0.25 * (old[i-1][j] + old[i+1][j] + old[i][j-1]
                    + old[i][j+1] - edge[i][j]);
            }
        }

        for (i = 1; i < nx_proc+1; i++)
        {
            for (j = 1; j < ny_proc+1; j++)
            {
                old[i][j] = new[i][j];
            }
        }

        if (iter % out_freq == 0)

            /*
             * Check if the iterations should be stopped here
             */

            if (proc == MASTER_PROCESS)
                printf("%d iterations complete.\n", iter);
    }

    if (proc == MASTER_PROCESS)
        printf("\n----- END OF ITERATIONS -----\n\n");

    for (i = 1; i < nx_proc+1; i++)
    {
        for (j = 1; j < ny_proc+1; j++)
        {
            buff[i-1][j-1] = old[i][j];
        }
    }

    free(dims);
    free(dim_period);
    free(nbrs);
    free(old);
    free(new);
    free(edge);

    /*
     * DON'T USE MPI GATHER!!!
     */
    MPI_Gather(&buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, &master_buff[0][0],
               nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS, cart_comm);
    MPI_Finalize();

    free(buff);

    if (proc == MASTER_PROCESS)
    {
        pgmwrite(out_filename, &master_buff[0][0], nx, ny);
    }

    free(master_buff);

    return 0;
}
