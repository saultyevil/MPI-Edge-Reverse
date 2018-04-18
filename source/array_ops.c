/* **************************************************************************
 * Initialise the arrays -- set edge to be buff and change all of the
 * elements in old to be 255.0 (white).
 * ************************************************************************** */

int
init_arrays(double **edge, double **old, double **buff, int nx_proc,
    int ny_proc)
{
    int i, j;

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

    return 0;
}

/* **************************************************************************
 * Copy the elements of old into buff.
 * ************************************************************************** */

int
copy_to_buff(double **old, double **buff, int nx_proc, int ny_proc)
{
    int i, j;

    for (i = 1; i < nx_proc+1; i++)
    {
        for (j = 1; j < ny_proc+1; j++)
        {
            buff[i-1][j-1] = old[i][j];
        }
    }

    return 0;
}
