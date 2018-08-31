/*
 * calculator.h
 *
 *  Created on: Apr 17, 2018
 *      Author: OWNER
 */
#ifndef CALCULATOR_H
#define CALCULATOR_H

#include <stdlib.h>

/**
 * Structure to hold heat sources.
 */
typedef struct
{
	int x, y;
	double value;
} source_point;

/**
 * Useful typedef.
 */
typedef double (*diff_func)(double cell, double right, double top, double left, double bottom);

/**
 * Calculator function. Applies the given function to every point in the grid iteratively for n_iter loops,
 * or until the cumulative difference is below terminate (if n_iter is 0).
 */
double calculate(diff_func function, double ** grid, size_t n, size_t m, source_point * sources, size_t num_sources, double terminate, unsigned int n_iter, int is_cyclic);

#endif

