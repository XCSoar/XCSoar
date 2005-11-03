// leastsqs.c -- Implements a simple linear least squares best fit routine
//
// Written by Curtis Olson, started September 1997.
//
// Copyright (C) 1997  Curtis L. Olson  - http://www.flightgear.org/~curt
//
// This library is free software; you can redistribute it and/or
// modify it under the terms of the GNU Library General Public
// License as published by the Free Software Foundation; either
// version 2 of the License, or (at your option) any later version.
//
// This library is distributed in the hope that it will be useful,
// but WITHOUT ANY WARRANTY; without even the implied warranty of
// MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU
// Library General Public License for more details.
//
// You should have received a copy of the GNU Library General Public
// License along with this library; if not, write to the
// Free Software Foundation, Inc., 59 Temple Place - Suite 330,
// Boston, MA  02111-1307, USA.
//
// $Id: leastsqs.cpp,v 1.5 2005/11/03 20:11:07 samgi Exp $
//
/*
NOTE: Sompe portions copyright as above

Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005  

  	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@bigfoot.com>

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.

}
*/


#include "stdafx.h"

#include <stdio.h>

#include "leastsqs.h"


/* 
Least squares fit:

y = b0 + b1x

     n*sum(xi*yi) - (sum(xi)*sum(yi))
b1 = --------------------------------
     n*sum(xi^2) - (sum(xi))^2


b0 = sum(yi)/n - b1*(sum(xi)/n)
*/


void LeastSquares::Reset() {
  sum_n = 0;
  sum_xi = 0;
  sum_yi = 0;
  sum_xi_2 = 0;
  sum_xi_yi = 0;
  max_error = 0;
  sum_error = 0;
  rms_error = 0;
  y_max = 0;
  y_min = 0;
  x_min = 0;
  x_max = 0;
}


void LeastSquares::least_squares_update(double y) {
  least_squares_update((double)(sum_n+1), y);
}


/* incrimentally update existing values with a new data point */
void LeastSquares::least_squares_update(double x, double y) {
    double error;

    if ((y>y_max) || (!sum_n)) {
      y_max = y;
    }
    if ((y<y_min) || (!sum_n)) {
      y_min = y;
    }

    if ((x>x_max) || (!sum_n)) {
      x_max = x;
    }
    if ((x<x_min) || (!sum_n)) {
      x_min = x;
    }

    // TODO: really should have a circular buffer here
    if (sum_n<MAX_STATISTICS) {
      xstore[sum_n] = x;
      ystore[sum_n] = y;
    }

    ++sum_n;


    sum_xi += x;
    sum_yi += y;
    sum_xi_2 += x * x;
    sum_xi_yi += x * y;

    /* printf("sum(xi)=%.2f  sum(yi)=%.2f  sum(xi^2)=%.2f  sum(xi*yi)=%.2f\n",
	   sum_xi, sum_yi, sum_xi_2, sum_xi_yi); */

    double denom = 	( (double)sum_n * sum_xi_2 - sum_xi * sum_xi );

    if (fabs(denom)>0) {
      m = ( (double)sum_n * sum_xi_yi - sum_xi * sum_yi ) / 
	denom;
    } else {
      m = 0.0;
    }
    b = (sum_yi / (double)sum_n) - (m) * (sum_xi / (double)sum_n);

    /* printf("slope = %.2f  intercept = %.2f\n", *m, *b); */

    y_ave = m*(x_max+x_min)/2.0+b;

    error = y - (m * x + b);
    sum_error += error * error;
    if (fabs(error)>max_error) {
      max_error = error;
    }
}

void LeastSquares::least_squares_error_update() {
  rms_error = sqrt(sum_error/(double)sum_n);
}


/* 
  return the least squares error:

              (y[i] - y_hat[i])^2
              -------------------
                      n
 */

/* 
  return the maximum least squares error:

              (y[i] - y_hat[i])^2
 */


