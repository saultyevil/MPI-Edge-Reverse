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
    int i, j, ii, jj, iter, p_iter, s_iter, r_iter;  // vars for iterations
    int n_iters, check_freq, out_freq;       // vars for output
    int nx, ny, nx_proc, ny_proc, proc, n_procs;  // vars for processes
    int x_dim, y_dim, x_dim_start, y_dim_start;  // vars for process domains

    double pixel_average, pixel_average_procs;  // vars for average pixel
    double delta_stopping, delta_proc, max_delta_proc, max_delta_all_procs;

    char *in_filename = malloc(sizeof(*in_filename) * MAX_LINE);
    char *out_filename = malloc(sizeof(*out_filename) * MAX_LINE);

    MPI_Comm cart_comm;
    MPI_Status recv_status;
    MPI_Request proc_request;
    MPI_Datatype send_array_vector, send_halo_vector;

    MPI_Init(NULL, NULL);
    MPI_Comm_rank(DEFAULT_COMM, &proc);
    MPI_Comm_size(DEFAULT_COMM, &n_procs);

    double program_begin = MPI_Wtime();

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

    double read_file_begin = MPI_Wtime();

    if (proc == MASTER_PROCESS)
    {
        pgmread(in_filename, &master_buff[0][0], nx, ny);
        printf("\n----------------------\n");
        printf("INPUT_FILENAME: %s\nOUTPUT_FILENAME: %s\nRESOLUTION: %d x %d\n",
               in_filename, out_filename, nx, ny);
        printf("MAX_ITERS: %d\nCHECK_FREQ: %d\nOUTPUT_FREQ: %d\nDELTA: %4.2f\n",
               n_iters, check_freq, out_freq, delta_stopping);
        printf("VERBOSE: %d\nNDIMS: %d\n\nN_PROCS: %d\n", verbose, NDIMS,
               n_procs);
        printf("----------------------\n");
    }

    double read_file_end = MPI_Wtime();

    free(in_filename);

    double topology_begin = MPI_Wtime();

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
                                &nx_proc, &ny_proc, &proc, n_procs,
                                reorder, disp);

    /*
     * When the domain can't be decomposed into a bunch of nice, even squares,
     * the program should report to use the NDIM = 1 case and decompose the
     * domain into a series of strips
     */
    if ((nx % nx_proc != 0 || ny % ny_proc != 0) && (proc == MASTER_PROCESS))
    {
        printf("\nImage is not cleanly divisible into an equal squares.\n");
        printf("Try using NDIMS = 1 in image_constants.h or change the");
        printf(" number of processes in use. A list of working process sizes");
        printf(" can be found in README.md.\n\n");
        MPI_Finalize();
        exit(1);
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

    double topology_end = MPI_Wtime();
    double parallel_begin = MPI_Wtime();
    double input_begin = MPI_Wtime();

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
        /*
         * The master process will send parts of masterbuff to the other
         * processes buff's synchronously. This is to ensure that each process
         * gets their buffer populated
         */
        if (proc == MASTER_PROCESS)
        {
            for (i = 0; i < nx_proc; i++)
            {
                for (j = 0; j < ny_proc; j++)
                {
                    buff[i][j] = master_buff[i][j];
                }
            }

            /*
             * Sending to each process other than root
             */
            for (s_iter = MASTER_PROCESS+1; s_iter < n_procs; s_iter++)
            {
                MPI_Ssend(
                &master_buff[x_dims[s_iter]*nx_proc][y_dims[s_iter]*ny_proc],
                1, send_array_vector, s_iter, DEFAULT_TAG, cart_comm);
            }
        }
        else
        {
            MPI_Recv(&buff[0][0], nx_proc*ny_proc, MPI_DOUBLE, MASTER_PROCESS,
                     DEFAULT_TAG, cart_comm, &recv_status);
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
        if (NDIMS == 2)
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
             * Calculate the average pixel value over all processes
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

    if (proc == MASTER_PROCESS)
        printf("\n----- END OF ITERATIONS -----\n\n");

    double parallel_iters_end = MPI_Wtime();
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
                 * Copy the buff into the master buff for MASTER PROCESS
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
                &master_buff[x_dims[r_iter]*nx_proc][y_dims[r_iter]*ny_proc],
                1, send_array_vector, r_iter, DEFAULT_TAG, cart_comm,
                &recv_status);
            }
        }
    }

    double output_end = MPI_Wtime();
    double parallel_end = MPI_Wtime();

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

        double parallel_time = parallel_end-parallel_begin;
        double time_per_process = (parallel_iters_end-parallel_iters_start)/ \
            n_procs;
        double time_per_iter = n_procs * (parallel_iters_end - \
            parallel_iters_start)/(nx_proc * ny_proc);
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

        if ((out_file = fopen("runtimes.txt", "a")) == NULL)
        {
            printf("Can't open file.\n");
            exit(1);
        }

        fprintf(out_file, "%d%d %d %f %f %f %f %f %f\n",
                nx, ny, n_procs, parallel_time, time_per_process, time_per_iter,
                file_input, file_output, total_runtime);

        if (fclose(out_file) != 0)
        {
            printf("File couldn't be closed.\n");
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
