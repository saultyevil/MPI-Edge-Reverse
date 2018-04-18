#include <stdio.h>
#include <math.h>
#include <mpi.h>

#include "image_constants.h"
#include "image_functions.h"

/* **************************************************************************
 * Computes all the iterations required to convert the image
 * ************************************************************************** */

int
compute_iterations(double **old, double **new, double **edge, int *nbrs,
    int nx_proc, int ny_proc, int proc, int max_iters, double delta_stop,
    int check_freq, int out_freq, int verbose, MPI_Comm cart_comm)
{
    int i, j, iter;
    double delta_proc, max_delta_proc, max_delta_all_procs;

    MPI_Status recv_status;
    MPI_Request proc_request;

    if (proc == MASTER_PROCESS)
        printf("\n---- BEGINNING ITERATIONS ----\n\n");

    for (iter = 1; iter <= max_iters; iter++)
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
        // MPI_Issend(&old[1][ny_proc], nx_proc, MPI_DOUBLE, nbrs[UP],
        //            DEFAULT_TAG, cart_comm, &proc_request);
        // MPI_Recv(&old[1][0], nx_proc, MPI_DOUBLE, nbrs[DOWN],
        //          DEFAULT_TAG, cart_comm, &recv_status);
        // MPI_Wait(&proc_request, &recv_status);

        // MPI_Issend(&old[1][1], nx_proc, MPI_DOUBLE, nbrs[DOWN],
        //            DEFAULT_TAG, cart_comm, &proc_request);
        // MPI_Recv(&old[1][ny_proc+1], nx_proc, MPI_DOUBLE, nbrs[UP],
        //          DEFAULT_TAG, cart_comm, &recv_status);
        // MPI_Wait(&proc_request, &recv_status);

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
            if (iter != max_iters)
            {
                MPI_Allreduce(&max_delta_proc, &max_delta_all_procs, 1,
                            MPI_DOUBLE, MPI_MAX, cart_comm);
                if (max_delta_all_procs < delta_stop)
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
        if ((iter % out_freq == 0) && (iter < max_iters) && \
            (proc == MASTER_PROCESS))
        {
            printf("%d iterations complete.\n", iter);
        }
        else if ((iter == max_iters) && (proc == MASTER_PROCESS))
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

    return 0;
}
