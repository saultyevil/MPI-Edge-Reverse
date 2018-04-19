#include <mpi.h>

/* **************************************************************************
 * FUNCTION DEFINITIONS
 * ************************************************************************** */

/*
 * Functions for IO -- provided by David Henty
 */
void pgmsize (char *filename, int *nx, int *ny);
void pgmread (char *filename, void *vx, int nx, int ny);
void pgmwrite(char *filename, void *vx, int nx, int ny);

/*
 * Functions for allocating N-dimensional arrays in C which are contiguous
 * in memory -- Provided by David Henty who got the code from someone else who
 * probably got it from someone else...
 */
void *arralloc(size_t size, int ndim, ...);

/*
 * Functions for taking in variable from external parameter files -- some code
 * I """borrowed""" from one of my other projects
 */
int read_double(char *par_string, double *parameter);
int read_int(char *par_string, int *parameter);
int read_string(char *par_string, char *parameter);

/*
 * Function used to split up the main function
 */
MPI_Comm create_topology(int ndims, int *dims, int *dim_period, int *nbrs,
         int *coords, int nx, int ny, int *nx_proc, int *ny_proc, int *proc,
         int n_procs, int reorder, int displacement);

/*
 * Functions for verbose printing
 */
int print_coords(int *coords, int proc, int n_procs);
int print_dims(int *dims, int proc);
int print_boundaries(int *bounds, int *coords, int proc, int n_procs);
int print_dims_coords(int *dims, int *coords, int proc, int n_procs,
    MPI_Comm comm);
int print_coord_boundaries(int *bounds, int *coords, int proc, int n_procs,
    MPI_Comm comm);
int print_n_proc(int proc, int nx, int ny);

/*
 * Functions for debugging
 */
void print_break(int proc, char *msg);
