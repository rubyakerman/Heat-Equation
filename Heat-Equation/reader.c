/**
 * @author Roy Ackerman
 */
#include <stdio.h>
#include <malloc.h>
#include <memory.h>
#include <stdbool.h>
#include "calculator.h"
#include "heat_eqn.h"

#define SUCCESS true;
#define FAILURE false;

// ........................................ Error messages ............................... //
const char *READING_FILE_ERR = "Error while reading file.";
const char *ALLOCATING_MEMORY_ERR = "Unable to allocate memory.";
const char *SINGLE_ARG_MSG = "Usage: heatSolve <parameter file>.\n";
const char *FILE_STRUCTURE_ERROR_MSG = "The file structure is invalid.\n";
const char *CREATE_SOURCES_ERROR = "Error while creating sources.\n";
const char *SEPARATOR_OR_CALC_AREA_ERROR = "Error while reading separator\\calculation-area coordinates from file.\n";
const char *SEPARATOR_OR_SOURCES_ERROR = "Error while reading separator\\sources from file.\n";
const char *READ_PRECISION_ERROR = "Error while reading precision from file.\n";
const char *READ_ITERATIONS_ERROR = "Error while reading number of iterations from file.\n";
const char *IS_CYCLE_ERROR = "Error while reading is-cycle value from file.\n";


// ........................................ Error handling ............................... //
const int READING_FILE_ERROR = 1;
const int FILE_STRUCTURE_ERROR = 2;
const int MEMORY_ALLOCATION_ERROR = 3;
const int ILEGAL_NUMBER_ERROR = -1;
const double CONVERSION_ERROR = 0.0;


// ........................................ General constants ............................... //
const char *SEPARATOR = "----\n";
const int SEPARATOR_LENGTH = 4;
const int MIN_MATRIX_INDEX = 0;


// ............................................. Fields .................................... //
size_t gRows, gColumns; // the n,m of the matrix (accordingly)
double gTerminateValue;
unsigned int gIterationNumber;
int gIsCyclic;
source_point *gSources; // A source_pint array
size_t gNumOfSources;
double **grid;

/**
 * Free the source_point array: gSources.
 */
void freeSources()
{
    if (gSources != NULL)
    {
        free(gSources);
    }
}

/**
 * Free the grid array.
 */
void freeGrid()
{
    for (size_t i = 0; i < gRows; ++i)
    {
        if (grid[i] != NULL)
        {
            free(grid[i]);
        }
    }

    if (grid != NULL)
    {
        free(grid);
    }
}

/**
 * Frees the whole memory.
 */
void freeMemory()
{
    // Free the source_point array
    freeSources();

    // Free the grid array
    freeGrid();
}

/**
 * checks weather the coordinate (x,y) is inside the
 * grid-matrix bounds.
 * @param x coordinate
 * @param y coordinate
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool isInsideMatrix(const int x, const int y)
{
    return (x >= MIN_MATRIX_INDEX) && (x < (int)gRows) && (y >= MIN_MATRIX_INDEX) && (y < (int)gColumns);
}

/**
 * Creates the source point 'source'
 * and initialize its x, y.
 * @param source
 * @param x coordinate
 * @param y coordinate
 * @param heatLevel
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool createSource(source_point *source, const int x, const int y, const float heatLevel)
{
    if (isInsideMatrix(x, y))
    {
        source->x = x;
        source->y = y;
        source->value = heatLevel;
        return SUCCESS;
    }

    perror(CREATE_SOURCES_ERROR);
    return FAILURE;
}

/**
 * Creates and initializes the sources array acoording to the input file.
 * @param file
 * @param numOfSources
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool getSources(FILE *file, size_t *numOfSources)
{
    const int NUM_OF_INPUTS_IN_SOURCE = 3;

    int x, y; // coordinates
    float heatLevel;
    size_t sources_counter = 0;

    gSources = (source_point *) malloc(1); // just for realloc
    if (gSources == NULL)
    {
        perror(ALLOCATING_MEMORY_ERR);
        return FAILURE;
    }

    while (fscanf(file, " %d, %d, %f ", &x, &y, &heatLevel) == NUM_OF_INPUTS_IN_SOURCE)
    {
        gSources = realloc(gSources, (sources_counter + 1) * sizeof(source_point));
        if (gSources == NULL)
        {
            perror(ALLOCATING_MEMORY_ERR);
            return FAILURE;
        }

        if (createSource(&gSources[sources_counter++], x, y, heatLevel) == false)
        {
            return FAILURE;
        }

        *numOfSources = sources_counter;
    }

    return SUCCESS;
}

/**
 * Checks weather the 'string' is the SEPARATOR line of the file: "----\n".
 * @param string the string we be compared by the function to the SEPARATOR
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool isSeparator(char *string)
{
    return strncmp(string, SEPARATOR, SEPARATOR_LENGTH - 1) == 0; // 1 more for the '\n'
}

/**
 * Reads the SEPARATOR string "----\n" from the file.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool getSeparator(FILE *file)
{
    const int EXTRA = 2; // will read extra for 'null-terminator' and '\n'.

    char *line = malloc((SEPARATOR_LENGTH + EXTRA) * sizeof(char));
    if (line == NULL)
    {
        perror(ALLOCATING_MEMORY_ERR);
        return FAILURE;
    }

    fgets(line, SEPARATOR_LENGTH + EXTRA, file);
    if (isSeparator(line))
    {
        free(line);
        return SUCCESS;
    }

    free(line);
    return FAILURE;
}

/**
 * checks weather 'c' is a tab or a white-space character.
 * @param c as a character.
 * @return
 */
bool isSpace(const char c)
{
    char SPACE = ' ';
    char TAB = '\t';

    return (c == SPACE) || (c == TAB);
}

/**
 * Moves the file's cursor(pointer) to the NEW_LINE character.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool nextLine(FILE *file)
{
    char NEW_LINE = '\n';

    int nextChar = fgetc(file);
    while (nextChar != NEW_LINE)
    {
        if (!isSpace((char) nextChar))
        {
            return FAILURE;
        }

        nextChar = fgetc(file);
    }

    return SUCCESS;
}

/**
 * Validates we have got positive numbers for
 * the area's rows \ columns.
 * @param n
 * @param m
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool validatesArea(int n, int m)
{
    return (n > MIN_MATRIX_INDEX) && (m > MIN_MATRIX_INDEX);
}

/**
 * Reads the number of the rows & columns from the file
 * by reading the first 2 integers.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool getCalcArea(FILE *file)
{
    const int ROWS_AND_COLS_NUMBERS = 2;

    int n, m; // Rows and columns

    if (fscanf(file, "%d , %d", &n, &m) == ROWS_AND_COLS_NUMBERS && validatesArea(n, m))
    {
        gRows = (size_t) n;
        gColumns = (size_t) m;
        if (nextLine(file) == false)
        {
            return FAILURE;
        }

        return SUCCESS;
    }

    return FAILURE;
}

/**
 * Reads single double number from the file.
 * @param file
 * @return return the number if the file is in the correct format,
 * otherwise return 0.0 .
 */
double readDouble( FILE *file)
{
    const int PRECISION_INPUT_SIZE = 1;
    const int DOUBLE_PRECISION = 15;
    const int MAX_PRECISION = DOUBLE_PRECISION;

    char *terminateValue = (char *)malloc(MAX_PRECISION * sizeof(char));
    if (fscanf(file, "%s", terminateValue) != PRECISION_INPUT_SIZE)
    {
        free(terminateValue);
        perror(READING_FILE_ERR);
        return FAILURE;
    }

    double value = strtod(terminateValue, NULL); // String to double
    free(terminateValue);
    return value;
}

/**
 * Reads the precision 'terminate' value from the file.
 * @param file
 * @return true on success, false otherwise.
 */
bool getPrecision(FILE *file)
{
    gTerminateValue = readDouble(file);  // Reads the next double from file
    if (gTerminateValue == CONVERSION_ERROR)
    {
        perror(READ_PRECISION_ERROR);
        return FAILURE;
    }

    if (nextLine(file) == false)
    {
        perror(READ_PRECISION_ERROR);
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Reads single int number from the file.
 * @param file
 * @return return the number if the file is in the correct format,
 * otherwise return 0.0 .
 */
int readInt(FILE *file)
{
    const int PRECISION_INPUT_SIZE = 1;

    int integer;
    if (fscanf(file, "%d", &integer) != PRECISION_INPUT_SIZE)
    {
        return ILEGAL_NUMBER_ERROR;
    }

    return integer;
}

/**
 * Reads the n-iter parameter from file.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool getIteration(FILE *file)
{
    const int MIN_ITERATIONS_NUMBER = 0;

    int iterations = readInt(file); // Reads the next int from file
    if (iterations < MIN_ITERATIONS_NUMBER)
    {
        perror(READ_ITERATIONS_ERROR);
        return FAILURE;
    }
    gIterationNumber = (unsigned int) iterations;

    if (nextLine(file) == false)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Validates there is no more unwanted characters in the file.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool validateRest(FILE *file)
{
    char NEW_LINE = '\n';

    int nextChar = fgetc(file);
    while (nextChar != NEW_LINE && nextChar != EOF)
    {
        if (!isSpace((char) nextChar))
        {
            perror(FILE_STRUCTURE_ERROR_MSG);
            return FAILURE;
        }

        nextChar = fgetc(file);
    }

    return SUCCESS;
}

/**
 * Reads the is_cyclic parameter from file.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool getIsCyclic(FILE *file)
{
    int isCyclic = readInt(file); // Reads the next int from file
    if (isCyclic != true && isCyclic != false)
    {
        perror(IS_CYCLE_ERROR);
        return FAILURE;
    }
    gIsCyclic = isCyclic;

    // Validates there is no more characters in the file
    if (validateRest(file) == false)
    {
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Parses the file, reads its input,
 * and keeps it inside the approperiate fields.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool parseFile(FILE *file)
{
    // Reads the size of the calculation area into m,n
    if (getCalcArea(file) == false)
    {
        return FAILURE;
    }

    // Reads the separator
    if (getSeparator(file) == false)
    {
        perror(SEPARATOR_OR_CALC_AREA_ERROR);
        return FAILURE;
    }

    // Reads the source points from file
    if (getSources(file, &gNumOfSources) == false)
    {
        freeSources();
        return FAILURE;
    }

    // Reads the separator
    if (getSeparator(file) == false)
    {
        perror(SEPARATOR_OR_SOURCES_ERROR);
        freeSources();
        return FAILURE;
    }

    // Reads the precision to gTerminateValue
    if (getPrecision(file) == false)
    {
        freeSources();
        return FAILURE;
    }

    // Reads the iteration number for n_iter
    if (getIteration(file) == false)
    {
        freeSources();
        return FAILURE;
    }

    // Reads the is_cyclic to gTerminateValue
    if (getIsCyclic(file) == false)
    {
        freeSources();
        return FAILURE;
    }

    return SUCCESS;
}

/**
 * Creates the grid & initializes its values to NO_HEAT.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool createGrid()
{
    const int NO_HEAT = 0;

    grid = malloc(gRows * sizeof(double *));
    if (grid == NULL)
    {
        return FAILURE;
    }

    for (size_t i = 0; i < gRows; ++i)
    {
        grid[i] = malloc(gColumns * sizeof(double));
        if (grid[i] == NULL)
        {
            return FAILURE;
        }
    }

    // Initializing the matrix to 0 heat
    for (size_t j = 0; j < gRows; ++j)
    {
        for (size_t i = 0; i < gColumns; ++i)
        {
            grid[j][i] = NO_HEAT;
        }
    }

    return SUCCESS;
}

/**
 * Initializes the grid with the sources's values
 * we got from the file.
 * @param file
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
void initializeGrid()
{
    for (size_t i = 0; i < gNumOfSources; ++i)
    {
        grid[gSources[i].x][gSources[i].y] = gSources[i].value;
    }
}

/**
 * Prints the grid array.
 * @param precisionResult
 */
void printGrid(const double precisionResult)
{
    const char *NEW_LINE = "\n";
    printf("%lf\n", precisionResult);

    for (size_t i = 0; i < gRows; ++i)
    {
        for (size_t j = 0; j < gColumns; ++j)
        {
            printf("%2.4lf,", grid[i][j]); // 2.4 stands for the correct precision
        }

        printf("%s", NEW_LINE);
    }
}

/**
 * Calculates the heat and its dissipation inside the grid array,
 * it is done by using the calculator and the function heat_eqn.
 */
void calculateHeat()
{
    double precisionResult;

    do
    {
        precisionResult = calculate(heat_eqn, grid, gRows, gColumns,
                                    gSources, gNumOfSources, gTerminateValue,
                                                        gIterationNumber, gIsCyclic);
        printGrid(precisionResult);
    } while (precisionResult >= gTerminateValue);
}

/**
 * Validates the number of arguments we've got.
 * @param args
 * @return SUCCESS if succeed, otherwise return FAILURE.
 */
bool validateArgs(int args)
{
    const int NUM_OF_ARGS = 2;

    if (args == NUM_OF_ARGS)
    {
        return SUCCESS;
    }

    perror(SINGLE_ARG_MSG);
    return FAILURE;
}

int main(int argc, char *argv[])
{
    const int SUCCESSFULLY = 0;
    const int FILE_PATH = 1;

    if (!validateArgs(argc))
    {
        perror(READING_FILE_ERR);
        return (READING_FILE_ERROR);
    }

    char *filePath = argv[FILE_PATH];
    FILE *file = fopen(filePath, "r");
    if (file == NULL)
    {
        perror(READING_FILE_ERR);
        return READING_FILE_ERROR;
    }

    // ................... Parsing file ...................... //
    if (parseFile(file) == false)
    {
        fclose(file);
        perror(FILE_STRUCTURE_ERROR_MSG);
        return FILE_STRUCTURE_ERROR;
    }

    // ................ Creates the grid matrix .............. //
    if (createGrid() == false)
    {
        fclose(file);
        perror(ALLOCATING_MEMORY_ERR);
        return MEMORY_ALLOCATION_ERROR;
    }

    // ........ Initializing with the sources's values ....... //
    initializeGrid();

    // ...Calculates the heat points using the calculator ... //
    calculateHeat();

    fclose(file);
    freeMemory();
    return (SUCCESSFULLY);
}
