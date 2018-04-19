/* **************************************************************************
 * Find the average value of an array -- without the surrounding halo
 * ************************************************************************** */

double
find_average_pixels(double **array, int nx, int ny)
{
    int i, j;
    double average, array_sum = 0;

    for (i = 1; i < nx; i++)
    {
        for (j = 1; j < ny; j++)
        {
            array_sum += array[i][j];
        }
    }

    average = array_sum/(nx * ny);

    return average;
}
