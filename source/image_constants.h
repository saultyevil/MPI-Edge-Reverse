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
 * For NDIMS 1: set XDIR = 0, YDIR = 0
 * For NDIMS 2: set XDIR = 0, YDIR = 1
 */
#define NDIMS 2
#define XDIR 0
#define YDIR 1

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
#define INI_FILE "edge2image.ini"
#define NO_PAR_CONST -1
#define STRING_NO_PAR_CONST '\0'
#define MAX_LINE 128

/*
 * Other constants
 */
#define TRUE 1
#define FALSE 0
