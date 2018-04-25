#include <stdlib.h>
#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

int
main(int argc, char *argv[])
{
    int verbose;  // var for enabling verbose output
    int i, j, ii, jj, iter, r_iter;  // vars for iterations
    int n_iters, check_freq, out_freq;       // vars for output
    int nx, ny, nx_proc, ny_proc, proc, n_procs;  // vars for processes
    int x_dim, y_dim, x_dim_start, y_dim_start;  // vars for process domains
    int nx_proc_start, ny_proc_start, x_disp, y_disp;  // vars for keeping track
                                                       // of proc array starts
    int n_requests;  // used for blocking send and recv's

    double pixel_average, pixel_average_procs;  // vars for average pixel
    double delta_stopping, delta_proc, max_delta_proc, max_delta_all_procs;

    char *in_filename = malloc(sizeof(*in_filename) * MAX_LINE);
    char *out_filename = malloc(sizeof(*out_filename) * MAX_LINE);

    /*
     * This is a bit of a hacky fix to help deal with switching between 1D
     * and 2D decomposition :-). We are essentially changing the number of
     * non-blocking comms in use and being tracked
     */
    if (NDIMS == 1)
        n_requests = 4;
    if (NDIMS == 2)
        n_requests = 8;

    MPI_Comm cart_comm;
    MPI_Status recv_status[n_requests], recv_status_output;
    MPI_Request proc_request[n_requests];
    MPI_Datatype send_array_vector, send_halo_vector;

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

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(DEFAULT_COMM, &proc);
    MPI_Comm_size(DEFAULT_COMM, &n_procs);

    double program_begin = MPI_Wtime();

    pgmsize(in_filename, &nx, &ny);
    double **master_buff = arralloc(sizeof(*master_buff), 2, nx, ny);
    pgmread(in_filename, &master_buff[0][0], nx, ny);  // helps when nx %np != 0

    double read_file_begin = MPI_Wtime();

    if (proc == MASTER_PROCESS)
    {
        printf("----------------------\n");
        printf("INPUT_FILENAME: %s\nOUTPUT_FILENAME: %s\nRESOLUTION: %d x %d\n",
               in_filename, out_filename, nx, ny);
        printf("MAX_ITERS: %d\nCHECK_FREQ: %d\nOUTPUT_FREQ: %d\nDELTA: %4.2f\n",
               n_iters, check_freq, out_freq, delta_stopping);
        printf("VERBOSE: %d\nNDIMS: %d\n", verbose, NDIMS);
        printf("----------------------\n");
        printf("N_PROCS: %d\n", n_procs);
        printf("----------------------\n");
    }

    double read_file_end = MPI_Wtime();

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

    /*
     * If NDIMS is too large or small, the program will exit in this function
     */
    cart_comm = create_topology(NDIMS, dims, dim_period, nbrs, coords, nx, ny,
                                &nx_proc_start, &ny_proc_start, &proc, n_procs,
                                reorder, disp);

    /*
     * These variables are used to track the length of buff in the x and y
     * directions for each processes. nx_proc_start and ny_proc_start are used
     * to track where a process should start to take data from in masterbuff.
     */
    nx_proc = nx_proc_start;
    ny_proc = ny_proc_start;

    /*
     * If the dimensions are not perfectly divisible, make the overlapping
     * process smaller so it doesn't extend past the boundary and seg fault etc.
     * Only do this for NDIMS = 2, as it will seg fault with NDIMS = 1 when it
     * reaches MPI_Scatter or MPI_Gather anyway.
     */
     if (NDIMS == 2)
    {
        if (coords[0]*nx_proc_start+nx_proc > nx)
        {
        nx_proc -= (coords[0] * nx_proc_start + nx_proc) - nx;
        }

        if (coords[1]*ny_proc_start+ny_proc > ny)
        {
            ny_proc -= (coords[1] * ny_proc_start + ny_proc) - ny;
        }
    }

    /*
     * Define a MPI vector type for sending sections rows of an array.
     *      send_halo_vector - this will send the halos of a specified dimenion
     */
    MPI_Type_vector(nx_proc, 1, ny_proc+2, MPI_DOUBLE, &send_halo_vector);
    MPI_Type_commit(&send_halo_vector);
    MPI_Type_vector(nx_proc, ny_proc, ny, MPI_DOUBLE, &send_array_vector);
    MPI_Type_commit(&send_array_vector);

    /*
     * Allocate memory for all of the arrays -- use arralloc because it keeps
     * array elements contiguous
     */
    double **buff = arralloc(sizeof(*buff), 2, nx_proc, ny_proc);
    double **old = arralloc(sizeof(*old), 2, nx_proc+2, ny_proc+2);
    double **new = arralloc(sizeof(*new), 2, nx_proc+2, ny_proc+2);
    double **edge = arralloc(sizeof(*edge), 2, nx_proc+2, ny_proc+2);

    if (verbose == TRUE)
    {
        print_n_proc(proc, nx_proc, ny_proc);
        print_dims_coords(dims, coords, proc, n_procs, cart_comm, NDIMS);
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

    double input_begin = MPI_Wtime();

    /*
     * Distribute masterbuff into smaller buffs for each process running
     */
    if (NDIMS == 1)
    {
        /*
         * If the dimensions aren't even, can't use MPI_Scatter so exit
         */
        if ((nx%nx_proc !=0 || ny%ny_proc != 0) & (proc == MASTER_PROCESS))
        {
            printf("\nCan't divide dimensions evenly. Use NDIMS = 2.\n");
            exit(1);
        }

        /*
         * For 1 dimension, just use the good ol' MPI_Scatter
         */
        MPI_Scatter(&master_buff[0][0], nx_proc*ny_proc, MPI_DOUBLE,
                    &buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS,
                    cart_comm);
    }
    else if (NDIMS == 2)
    {
        /*
         * Very inefficent way to handle reading in data to buff. Essentially
         * we do this as using MPI_Recv resulted in truncation errors when nx
         * or ny wasn't divisible by nx_proc or ny_proc. Whilst this is less
         * efficient than a send and receive as used in buff -> masterbuff,
         * it at least works :^)
         */
        x_disp = coords[0] * nx_proc_start;
        y_disp = coords[1] * ny_proc_start;

        for (i = 0; i < nx_proc; i++)
        {
            for (j = 0; j < ny_proc; j++)
            {
                ii = i + x_disp;
                jj = j + y_disp;

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

    double input_end = MPI_Wtime();

    if (proc == MASTER_PROCESS)
        printf("\n---- BEGINNING ITERATIONS ----\n\n");
    /*
     * Compute the iterations -- time them for parallel computation comparison
     */
    double parallel_iters_start = MPI_Wtime();

    for (iter = 1; iter <= n_iters; iter++)
    {
        /*
        * Send and recieve halo cells data from left and right
        * neighbouring processes -- the same as with the 1D case study case --
        * we can send columns of data easily because of how memory is laid
        * out in C.
        */
        MPI_Issend(&old[nx_proc][1], ny_proc, MPI_DOUBLE, nbrs[RIGHT],
                   DEFAULT_TAG, cart_comm, &proc_request[0]);
        MPI_Irecv(&old[0][1], ny_proc, MPI_DOUBLE, nbrs[LEFT], DEFAULT_TAG,
                  cart_comm, &proc_request[1]);
        MPI_Issend(&old[1][1], ny_proc, MPI_DOUBLE, nbrs[LEFT],
                DEFAULT_TAG, cart_comm, &proc_request[2]);
        MPI_Irecv(&old[nx_proc+1][1], ny_proc, MPI_DOUBLE, nbrs[RIGHT],
                  DEFAULT_TAG, cart_comm, &proc_request[3]);
        /*
        * Send and recieve halo cells data from up and down
        * neighbouring processes -- use the halo datatype to send columns.
        */
        if (NDIMS == 2)
        {
            MPI_Issend(&old[1][ny_proc], 1, send_halo_vector, nbrs[UP],
                    DEFAULT_TAG, cart_comm, &proc_request[4]);
            MPI_Irecv(&old[1][0], 1, send_halo_vector, nbrs[DOWN],
                    DEFAULT_TAG, cart_comm, &proc_request[5]);
            MPI_Issend(&old[1][1], 1, send_halo_vector, nbrs[DOWN],
                    DEFAULT_TAG, cart_comm, &proc_request[6]);
            MPI_Irecv(&old[1][ny_proc+1], 1, send_halo_vector, nbrs[UP],
                    DEFAULT_TAG, cart_comm, &proc_request[7]);
        }

        MPI_Waitall(n_requests, proc_request, recv_status);

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
                        printf("Image converted after %d iterations.\n",
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
        if (iter % out_freq == 0)
        {
            /*
             * Calculate the average pixel value over all processes - use
             * MPI_Allreduce to calculate the sum of the average pixel value
             * over all processes
             */
            pixel_average = find_average_pixels(old, nx_proc, ny_proc);

            MPI_Allreduce(&pixel_average, &pixel_average_procs, 1, MPI_DOUBLE,
                          MPI_SUM, cart_comm);

            pixel_average_procs /= n_procs;

            if (iter < n_iters)
            {
                if (proc == MASTER_PROCESS)
                {
                    printf("%d iterations complete.\n", iter);
                    printf("Average pixel value for image: %f.\n\n",
                        pixel_average_procs);
                }

                if (verbose == TRUE)
                    printf("Average pixel value for process %d: %f.\n", proc,
                           pixel_average);
            }
            else if ((iter == n_iters) && (proc == MASTER_PROCESS))
            {
                printf("%d iterations complete.\n", iter);
                printf("\nIterations complete. However, the image may not");
                printf(" be fully processed yet.\n");
                if (verbose == TRUE)
                {
                    printf("max_delta = %f\n", max_delta_all_procs);
                    printf("Average pixel value for image: %f.\n",
                           pixel_average_procs);
                }
            }
        }
    }

    double parallel_iters_end = MPI_Wtime();

    if (proc == MASTER_PROCESS)
        printf("\n----- END OF ITERATIONS -----\n\n");

    double output_begin = MPI_Wtime();

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

    free(dims); free(dim_period); free(coords); free(nbrs); free(old);
    free(new); free(edge);

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
                 * Copy the buff into the master buff for the MASTER PROCESS
                 */
                for (j = 0; j < ny_proc; j++)
                {
                    master_buff[i][j] = buff[i][j];
                }
            }

            /*
             * Receive buff from all other processes which aren't the MASTER
             * process to reconstruct buff
             */
            for (r_iter = MASTER_PROCESS+1; r_iter < n_procs; r_iter++)
            {
                MPI_Recv(
                &master_buff[x_dims[r_iter]*nx_proc_start] \
                    [y_dims[r_iter]*ny_proc_start],
                1, send_array_vector, r_iter, DEFAULT_TAG, cart_comm,
                &recv_status_output);
            }
        }
    }

    double output_end = MPI_Wtime();

    free(x_dims); free(y_dims); free(buff);

    double write_to_file_begin = MPI_Wtime();

    if (proc == MASTER_PROCESS)
        pgmwrite(out_filename, &master_buff[0][0], nx, ny);

    double write_to_file_end = MPI_Wtime();

    free(master_buff); free(out_filename);

    double program_end = MPI_Wtime();

    MPI_Finalize();

    /*
     * Bunch of print commands to print out the timings of the program
     */
    if (proc == MASTER_PROCESS)
    {
        FILE *out_file;

        double parallel_time = parallel_iters_end - parallel_iters_start;
        double time_per_process = parallel_time/n_procs;
        double time_per_iter = parallel_time/(double) iter;
        double file_input = input_end - input_begin;
        double file_output = output_end - output_begin;
        double total_runtime = program_end - program_begin;

        printf("Average time per process: %9.6f seconds.\n", time_per_process);
        printf("Average time per process iter: %9.6f seconds.\n",
                time_per_iter);
        printf("Time required for file input: %9.6f seconds.\n", file_input);
        printf("Time required for file output: %9.6f seconds.\n", file_output);
        printf("\nTotal program runtime: %9.6f seconds.\n", total_runtime);
        printf("Total parallel runtime: %9.6f seconds.\n", parallel_time);

        if ((out_file = fopen("runtime.txt", "a")) == NULL)
        {
            printf("Runtime file can't be opened! Exiting.\n");
            exit(1);
        }

        fprintf(out_file, "%d%d %d %f %f %f %f %f %f\n",
                nx, ny, n_procs, parallel_time, time_per_process, time_per_iter,
                file_input, file_output, total_runtime);

        if (fclose(out_file) != 0)
        {
            printf("Runtime output file couldn't be closed! Exiting.\n");
            exit(1);
        }
    }

    return 0;
}

/* **************************************************************************
 * Use this to print a break message lmao. msg is just a string.
 * ************************************************************************** */

void
print_break(int proc, char *msg)
{
    printf("proc %d break %s\n", proc, msg);
}
