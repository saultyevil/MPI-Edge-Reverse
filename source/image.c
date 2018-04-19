#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

int
main(int argc, char *argv[])
{
    int i, j, ii, jj, iter, p_iter, r_iter, n_iters, check_freq, out_freq;
    int verbose, nx, ny, nx_proc, ny_proc, proc, n_procs;
    int x_dim, y_dim, x_dim_start, y_dim_start, use_ndim1 = FALSE;
    double delta_stopping, delta_proc, max_delta_proc, max_delta_all_procs;
    char *in_filename = malloc(sizeof(*in_filename) * MAX_LINE);
    char *out_filename = malloc(sizeof(*out_filename) * MAX_LINE);

    MPI_Comm cart_comm;
    MPI_Status recv_status;
    MPI_Datatype send_array_vector, send_halo_vector;
    MPI_Request proc_request;

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
    pgmread(in_filename, &master_buff[0][0], nx, ny);

    if (proc == MASTER_PROCESS)
    {
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
    cart_comm = create_topology(NDIMS, dims, dim_period, nbrs, coords, nx, ny,
                                &nx_proc, &ny_proc, &proc, n_procs,
                                reorder, disp);

    /*
     * When the domain can't be decomposed into a bunch of nice, even squares,
     * the program should resort to using the NDIM = 1 case and decompose the
     * domain into a series of strips
     */
    if (nx % nx_proc != 0 || ny % ny_proc != 0)
    {
        if (proc == MASTER_PROCESS)
        {
            printf("Error when trying to divide into nice, even squares.\n");
            printf("1D domain decomposition to be used instead.\n");
            printf("Verbose mode is also disabled.");
        }

        use_ndim1 = TRUE;
        verbose = FALSE;
        cart_comm = create_topology(1, dims, dim_period, nbrs, coords, nx,
                                    ny, &nx_proc, &ny_proc, &proc, n_procs,
                                    reorder, disp);  // re-create a new comm
    }

    /*
     * Allocate memory for all of the arrays -- use arralloc because it keeps
     * array elements contiguous
     */
    double **buff = arralloc(sizeof(*buff), 2, nx_proc, ny_proc);
    double **old = arralloc(sizeof(*old), 2, nx_proc+2, ny_proc+2);
    double **new = arralloc(sizeof(*new), 2, nx_proc+2, ny_proc+2);
    double **edge = arralloc(sizeof(*edge), 2, nx_proc+2, ny_proc+2);

    /*
     * Define a MPI vector types for sending sections of arrays
     * send_array_vector - this will send a 2D section of the array between
     *                     master buff to buff
     * send_halo_vector - this will send the halos of a specified dimenion
     */
    MPI_Type_vector(nx_proc, ny_proc, ny, MPI_DOUBLE, &send_array_vector);
    MPI_Type_commit(&send_array_vector);
    MPI_Type_vector(nx_proc, 1, ny_proc+2, MPI_DOUBLE, &send_halo_vector);
    MPI_Type_commit(&send_halo_vector);

    if (verbose == TRUE)
    {
        print_n_proc(proc, nx_proc, ny_proc);
        print_dims_coords(dims, coords, proc, n_procs, cart_comm);
    }

    /*
     * Send all of the coordinates to the master process for use when
     * distributing master_buff over buff in all the processes
     */
    int *x_dims = malloc(sizeof(*x_dims) * n_procs);
    int *y_dims = malloc(sizeof(*y_dims) * n_procs);

    MPI_Gather(&coords[0], 1, MPI_INT, x_dims, 1, MPI_INT, MASTER_PROCESS,
               cart_comm);
    MPI_Gather(&coords[1], 1, MPI_INT, y_dims, 1, MPI_INT, MASTER_PROCESS,
               cart_comm);

    double parallel_begin = MPI_Wtime();

    /*
     * Distribute masterbuff into smaller buffs for each process running
     */
    if (NDIMS == 1 || use_ndim1 == TRUE)
    {
        /*
         * For 1 dimension, just use the good ol' MPI_Scatter
         */
        MPI_Scatter(&master_buff[0][0], nx_proc*ny_proc, MPI_DOUBLE,
                    &buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS,
                    cart_comm);
    }
    else if (NDIMS == 2 && use_ndim1 == FALSE)
    {
        /*
         * Very bad and inefficient way of distributing the work :^).
         * Essentially every thread has its own copy of masterbuff and copies
         * a section from masterbuff into buff.
         */
        x_dim_start = coords[0] * nx_proc;
        y_dim_start = coords[1] * ny_proc;

        if (verbose == TRUE)
        {
            printf("proc %d: x_start %d y_start %d\n",
                   proc, x_dim_start, y_dim_start);
        }

        for (i = 0; i < nx_proc; i++)
        {
            for (j = 0; j < ny_proc; j++)
            {
                /*
                 * Need to make sure that when the shift factor x/y_dim_start
                 * is applied, that the bounds of the array are not breached
                 * leading to a seg fault
                 */
                if ((ii = x_dim_start+i) > nx - 1)
                    ii = nx - 1;
                if ((jj = y_dim_start+j) > ny - 1)
                    jj = ny - 1;

                buff[i][j] = master_buff[ii][jj];
            }
        }
    }

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

    /*
     * Compute the iterations -- time them for parallel computation comparison
     */
    double parallel_iters_start = MPI_Wtime();

    if (proc == MASTER_PROCESS)
        printf("\n---- BEGINNING ITERATIONS ----\n\n");

    for (iter = 1; iter <= n_iters; iter++)
    {
        /*
        * Send and recieve halo cells data from left and right
        * neighbouring processes -- the same as with the 1D case study case
        * it just works, so why not re-use it?
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
        * neighbouring processes -- use the halo datatype to send columns
        */
        if (NDIMS == 2 && use_ndim1 == FALSE)
        {
            MPI_Issend(&old[1][ny_proc], 1, send_halo_vector, nbrs[UP],
                    DEFAULT_TAG, cart_comm, &proc_request);
            MPI_Recv(&old[1][0], 1, send_halo_vector, nbrs[DOWN],
                    DEFAULT_TAG, cart_comm, &recv_status);
            MPI_Wait(&proc_request, &recv_status);

            MPI_Issend(&old[1][1], 1, send_halo_vector, nbrs[DOWN],
                    DEFAULT_TAG, cart_comm, &proc_request);
            MPI_Recv(&old[1][ny_proc+1], 1, send_halo_vector, nbrs[UP],
                    DEFAULT_TAG, cart_comm, &recv_status);
            MPI_Wait(&proc_request, &recv_status);
        }

        /*
        * Calculate the value each pixel depending on its neighbouring pixels
        */
        max_delta_proc = 0;  // reset the max delta for this iteration
        for (i = 1; i < nx_proc+1; i++)
        {
            for (j = 1; j < ny_proc+1; j++)
            {
                new[i][j] = 0.25 * (old[i-1][j] + old[i+1][j] + old[i][j-1]
                    + old[i][j+1] - edge[i][j]);

                /*
                * Calculate the difference between the new and old iteration
                * and keep a record of the largest difference
                */
                delta_proc = (double) fabs(new[i][j] - old[i][j]);
                if (delta_proc > max_delta_proc)
                    max_delta_proc = delta_proc;
            }
        }

        /*
        * Set the old values to the new values for next iteration
        */
        for (i = 1; i < nx_proc+1; i++)
        {
            for (j = 1; j < ny_proc+1; j++)
            {
                old[i][j] = new[i][j];
            }
        }

        if (iter % check_freq == 0)
        {
            /*
            * If we haven't reached the maximum number of iterations to be
            * done, check if the image has been converted already. If the
            * largest pixel difference is smaller than the criterion, break
            * the loop
            */
            if (iter != n_iters)
            {
                MPI_Allreduce(&max_delta_proc, &max_delta_all_procs, 1,
                            MPI_DOUBLE, MPI_MAX, cart_comm);
                if (max_delta_all_procs < delta_stopping)
                {
                    if (proc == MASTER_PROCESS)
                    {
                        printf("\nImage converted after %d iterations.\n",
                            iter);
                        if (verbose == TRUE)
                            printf("max_delta = %f\n", max_delta_all_procs);
                    }

                    break;
                }
            }
        }

        /*
        * Print out a progress report - each processes should have an equal
        * work load so printing out the progress report on the master processes
        * should be an accurate for all processes
        */
        if ((iter % out_freq == 0) && (iter < n_iters) && \
            (proc == MASTER_PROCESS))
        {
            printf("%d iterations complete.\n", iter);
        }
        else if ((iter == n_iters) && (proc == MASTER_PROCESS))
        {
            printf("%d iterations complete.\n", iter);
            printf("\nIterations complete. However, the image may not");
            printf(" be fully processed yet.\n");
            if (verbose == TRUE)
                printf("max_delta = %f\n", max_delta_all_procs);
        }
    }

    if (proc == MASTER_PROCESS)
        printf("\n----- END OF ITERATIONS -----\n\n");

    double parallel_iters_end = MPI_Wtime();

    /*
     * Copy the final image into the buffer -- excluding halo cells i.e.
     * copy old into buff
     */
    for (i = 1; i < nx_proc+1; i++)
    {
        for (j = 1; j < ny_proc+1; j++)
        {
            buff[i-1][j-1] = old[i][j];
        }
    }

    free(dims); free(dim_period); free(coords); free(bounds); free(nbrs);
    free(old); free(new); free(edge);

    /*
     * Gather all the buff's back into the masterbuff on the master process
     */
    if (NDIMS == 1 || use_ndim1 == TRUE)
    {
        /*
         * For 1 dimension, just use the good ol' MPI_Gather
         */
        print_break(proc, "gather start");
        MPI_Gather(&buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, &master_buff[0][0],
                   nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS, cart_comm);
        print_break(proc, "gather end");
    }
    else if (NDIMS == 2 && use_ndim1 == FALSE)
    {
        if (proc != MASTER_PROCESS)
        {
            /*
             * All processes apart from the MASTER PROCESS send their version
             * of buff to the MASTER PROCESS
             */
            MPI_Ssend(&buff[0][0], nx_proc*ny_proc, MPI_DOUBLE,
                MASTER_PROCESS, DEFAULT_TAG, cart_comm);
        }
        else
        {
            for (i = 0; i < nx_proc; i++)
            {
                /*
                 * Easy to copy and paste
                 */
                for (j = 0; j < ny_proc; j++)
                {
                    master_buff[i][j] = buff[i][j];
                }
            }

            for (r_iter = 1; r_iter < n_procs; r_iter++)
            {
                MPI_Recv(
                &master_buff[x_dims[r_iter]*nx_proc][y_dims[r_iter]*ny_proc],
                1, send_array_vector, r_iter, DEFAULT_TAG, cart_comm,
                &recv_status);
            }
        }
    }

    double parallel_end = MPI_Wtime();

    free(x_dims); free(y_dims);

    MPI_Finalize();

    free(buff);

    if (proc == MASTER_PROCESS)
    {
        pgmwrite(out_filename, &master_buff[0][0], nx, ny);
        printf("Time required for image conversion: %9.6f seconds.\n\n",
            (parallel_iters_end - parallel_iters_start));
    }

    free(master_buff);
    free(out_filename);

    return 0;
}

void
print_break(int proc, char *msg)
{
    printf("proc %d break %s\n", proc, msg);
}
