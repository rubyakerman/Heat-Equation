/**
 * @author Roy Ackerman
 */
#include <stdbool.h>
#include <stdio.h>
#include <math.h>
#include "calculator.h"

typedef enum Neighbours
{
    RIGHT,
    LEFT,
    UP,
    BOTTOM
} Neighbour;

int gIs_cyclic;
size_t gRows;
size_t gColumns;
source_point *gSources;
size_t gNumOfSources;
double **gGrid;

/**
 * calculates the sum of the matrix grid.
 * @param grid - the matrix
 * @param y - num of rows
 * @param x - num of columns
 * @param sum output parameter contains the sum.
 */
void heatSum(double *sum)
{
    *sum = 0;
    for (size_t row = 0; row < gRows; ++row)                                     ////int instead of size_t
    {
        for (size_t col = 0; col < gColumns; ++col)                              ////////int instead of size_t
        {
            (*sum) += gGrid[row][col];
        }
    }
}

/**
 * Checls weather the coordinate (x,y) is one of the sources.
 * @param row
 * @param col
 * @return true if is, false otherwise.
 */
bool isSource(const size_t row, const size_t col)
{
    for (size_t i = 0; i < gNumOfSources; ++i)                              ////////int instead of size_t
    {
        if (gSources[i].x == (int)row && gSources[i].y == (int)col)        ////////casting to int has been added
        {
            return true;
        }
    }

    return false;
}

/**
 * A custom mod operation uses the floor value instead of rounding to zero.
 * @param numerator is what we has to calculate
 * @param n a filed of the mod we has to calculate as a non-negative number
 * @return MOD(x,N) - the reminder of the division of x / N.
 */
size_t mod(const int denominator, const int numerator)
{
    return (size_t) (((numerator % denominator) + denominator) % denominator);
}

/**
 * checks weather the coordinate (col, row) is out of the grid's matrix.
 * @param row
 * @param col
 * @return true if is, false otherwise.
 */
bool indexOutOfMatrix(const size_t row, const size_t col)
{
    const size_t START_MATRIX_INDEX = 0;
    return (col < START_MATRIX_INDEX) || (row < START_MATRIX_INDEX) || (col >= gColumns) || (row >= gRows);
}


/**
 * Returns the value of the required member.
 */
double getNeighbourValue(size_t row, size_t col, const enum Neighbours neighbour)
{
    const int VALUE_FOR_OUT_OF_MATRIX = 0;

    if (neighbour == RIGHT)
    {
        col++;
    }
    if (neighbour == LEFT)
    {
        col--;
    }
    if (neighbour == UP)
    {
        row++;
    }
    if (neighbour == BOTTOM)
    {
        row--;
    }

    if (gIs_cyclic)
    {
        row = mod((int) gRows, (int) row);
        col = mod((int) gColumns, (int) col);
    }

    if (indexOutOfMatrix(row, col))
    {
        return VALUE_FOR_OUT_OF_MATRIX;
    }
    else
    {
        return gGrid[row][col];
    }
}


/**
 * activates the function 'function' on the coordinate (r,c)
 * and calculates the RIGHT, LEFT, TOP, and BOTTOM as well.
 * @param function
 * @param r the row
 * @param c the column
 */
void activateFunction(const diff_func function, const size_t r, const size_t c)
{
    double cell, onRight, onLeft, onTop, onDown;

    onRight = getNeighbourValue(r, c, RIGHT);
    onLeft = getNeighbourValue(r, c, LEFT);
    onTop = getNeighbourValue(r, c, UP);
    onDown = getNeighbourValue(r, c, BOTTOM);
    cell = gGrid[r][c];

    gGrid[r][c] = function(cell, onRight, onTop, onLeft, onDown);
}

/**
 * performing the heat activity by activates the function
 * 'function' on each one of the matrix.
 * grid-array's cells.
 * @param function.
 */
void heat(const diff_func function)
{

    for (size_t r = 0; r < gRows; r++)
    {
        for (size_t c = 0; c < gColumns; c++)
        {
            if (!isSource(r, c))
            {
                // activate function on the coordinate (r,c)
                activateFunction(function, r, c);
            }
        }
    }

}

/**
 * checks weather we should terminate the
 * loop by iterations or not.
 * @param n_iter
 * @return true if should, otherwise false.
 */
bool isTerminatedByIterations(const int n_iter)
{
    const int NO_ITERATIONS = 0;
    return (n_iter > NO_ITERATIONS);
}

/**
 * Checks weather the precision is good enough
 * by validates the remainder of currSum - prevSum.
 * @param prevSum - the sum of the previous iteration.
 * @param currSum - the sum of the current iteration.
 * @param terminate - the required precision.
 * @return
 */
bool isPrecise(const double prevSum, const double currSum, const double terminate)
{
    return fabs(currSum - prevSum) < terminate;
}

/**
 * Calculates the heat and its dissipation according to the source points 'sources',
 *by activating the function 'function' on the matrix grid.
 * @param function the function to activate
 * @param grid the matrix
 * @param n the rows
 * @param m the columns
 * @param sources array of the heat sources points
 * @param num_sources the number of sources
 * @param terminate the 'epsilon' for detecting the required precision
 * @param n_iter num of iterations
 * @param is_cyclic is it should be cyclic
 * @return the heat reminder of the last iteration
 */
double calculate(diff_func function,
                 double **grid,
                 size_t n, size_t m, source_point *sources, size_t num_sources,
                 double terminate, unsigned int n_iter, int is_cyclic)
{
    //............ global variables initialization .......//
    gIs_cyclic = is_cyclic;
    gRows = n;
    gColumns = m;
    gSources = sources;
    gNumOfSources = num_sources;
    gGrid = grid;

    double prevSum;
    heatSum(&prevSum); // get the heat sum into sum
    double currSum = prevSum;
    if (isTerminatedByIterations(n_iter))
    {
        for (unsigned int i = 0; i < n_iter; ++i)
        {
            prevSum = currSum;
            heat(function); // activate the heat function
            heatSum(&currSum); // get the current heat sum into currentSum
        }
    }
    else
    {
        do
        {
            prevSum = currSum;
            heat(function); // activate the heat function
            heatSum(&currSum); // get the current heat sum into currentSum
        } while (!isPrecise(prevSum, currSum, terminate));
    }

    return fabs(currSum - prevSum);
}

