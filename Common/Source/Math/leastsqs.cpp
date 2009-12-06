/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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


#include "Math/leastsqs.h"
#include "XCSoar.h"
#include <stdio.h>
#include <math.h>

/*
Least squares fit:

  y = b0 + b1x

        n * sum(xi * yi) - (sum(xi) * sum(yi))
  b1 = ----------------------------------------
             n * sum(xi^2) - sum(xi)^2


  b0 = sum(yi) / n - b1 * (sum(xi) / n)

*/

/*
return the least squares error:

   (y[i] - y_hat[i])^2
  ---------------------
          n
*/

/*
return the maximum least squares error:

  (y[i] - y_hat[i])^2

*/

void
LeastSquares::Reset()
{
  sum_n = 0;
  sum_xi = 0;
  sum_yi = 0;
  sum_xi_2 = 0;
  sum_xi_yi = 0;
  max_error = 0;
  sum_error = 0;
  rms_error = 0;
  sum_weights = 0;
  y_max = 0;
  y_min = 0;
  x_min = 0;
  x_max = 0;
  y_ave = 0;
}

void
LeastSquares::least_squares_update(double y)
{
  least_squares_update((double)(sum_n+1), y);
}

void
LeastSquares::least_squares_add(double x, double y, double weight)
{
  if ((y > y_max) || (!sum_n)) {
    y_max = y;
  }
  if ((y < y_min) || (!sum_n)) {
    y_min = y;
  }

  if ((x > x_max) || (!sum_n)) {
    x_max = x;
  }
  if ((x < x_min) || (!sum_n)) {
    x_min = x;
  }

  // TODO code: really should have a circular buffer here
  if (sum_n < MAX_STATISTICS) {
    xstore[sum_n] = (float)x;
    ystore[sum_n] = (float)y;
    weightstore[sum_n] = (float)weight;
  }

  ++sum_n;

  sum_weights += weight;

  double xw = x * weight;
  double yw = y * weight;

  sum_xi += xw;
  sum_yi += yw;
  sum_xi_2 += xw * xw;
  sum_xi_yi += xw * yw;
}

void
LeastSquares::least_squares_update()
{
  double denom = (sum_weights * sum_xi_2 - sum_xi * sum_xi);

  if (fabs(denom) > 0.0) {
    m = (sum_weights * sum_xi_yi - sum_xi * sum_yi) / denom;
  } else {
    m = 0.0;
  }
  b = (sum_yi - m * sum_xi) / sum_weights;

  y_ave = m * (x_max + x_min) / 2.0 + b;
}

/* incrementally update existing values with a new data point */
void
LeastSquares::least_squares_update(double x, double y, double weight)
{
  least_squares_add(x, y, weight);
  least_squares_update();

  double error;
  error = y - (m * x + b);
  sum_error += error * error * weight;
  if (fabs(error) > max_error) {
    max_error = error;
  }
}

void LeastSquares::least_squares_error_update() {
  rms_error = sqrt(sum_error/sum_weights);
}
