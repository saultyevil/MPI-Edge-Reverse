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
 * I """stole""" from one of my other projects
 */
int read_double(char par_string[], double *parameter);
int read_int(char par_string[], int *parameter);
int read_string(char par_string[], char *parameter);

/* **************************************************************************
 * CONSTANT DEFINITIONS
 * ************************************************************************** */

/*
 * Constants for MPI -- defines default tag and communicator and anything
 * else, such as the master process
 */
#define DEFAULT_COMM MPI_COMM_WORLD
#define DEFAULT_TAG 0
#define MASTER_PROCESS 0

/*
 * Constants for Cartesian topology -- MPI_Cart_create, MPI_Dim_create etc
 */
#define NDIMS 2
#define XDIR 1
#define YDIR 0

/*
 * Constants for an array containing the neighbouring processes of each process
 */
#define N_NBRS 4
#define UP 0
#define DOWN 1
#define LEFT 2
#define RIGHT 3

/*
 * Constants for parameter IO
 */
#define INI_FILE "image.ini"
#define NO_PAR_CONST -1
#define STRING_NO_PAR_CONST '\0'
#define MAX_LINE 128
