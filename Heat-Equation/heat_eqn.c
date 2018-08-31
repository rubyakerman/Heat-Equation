/*
 * heat_eqn.c
 *
 *  Created on: Apr 18, 2018
 *      Author: OWNER
 */


/**
 * A discrete form of the heat equation.
 */
double heat_eqn(double cell, double right, double top, double left, double bottom)
{
	/*
	 * For simplicity, we have set D = dt = dx = 1;
	 */
	double dphiDx = right + left; // - 2 * cell
	double dphiDy = top + bottom; // - 2 * cell
	return (dphiDx + dphiDy) / 4; // + cell - cancels out.
}

