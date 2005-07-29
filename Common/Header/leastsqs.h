/**
 * \file leastsqs.hxx
 * Implements a simple linear least squares best fit routine.
 */

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
// $Id: leastsqs.h,v 1.1 2005/07/29 15:02:42 jwharington Exp $


#ifndef _LEASTSQS_H
#define _LEASTSQS_H


#ifndef __cplusplus                                                          
# error This library requires C++
#endif                                   

#define MAX_STATISTICS 1000

class LeastSquares {

 public:

  double sum_xi, sum_yi, sum_xi_2, sum_xi_yi;
  int sum_n;
  double m;
  double b;
  double sum_error;

  double rms_error;
  double max_error;
  double y_max;
  double y_min;
  double x_min;
  double x_max;

  double y_ave;

  double xstore[MAX_STATISTICS];
  double ystore[MAX_STATISTICS];

  LeastSquares() {
    Reset();
  }

  void Reset();

/**
Classical least squares fit:

\f[
    y = b_0 + b_1 * x
\f]

\f[
    b_1 = \frac{n * \sum_0^{i-1} (x_i*y_i) - \sum_0^{i-1} x_i* \sum_0^{i-1} y_i}
          {n*\sum_0^{i-1} x_i^2 - (\sum_0^{i-1} x_i)^2}
\f]

\f[
    b_0 = \frac{\sum_0^{i-1} y_i}{n} - b_1 * \frac{\sum_0^{i-1} x_i}{n}
\f]
*/


/**
 * Incrimentally update existing values with a new data point.
 */
  void least_squares_update(double x, double y);

  // assume x is the sample number
  void least_squares_update(double y);

  void least_squares_error_update();


/**
  @return the least squares error:.
\f[

    \frac{(y_i - \hat{y}_i)^2}{n}
\f]
*/


/**
  @return the maximum least squares error.

\f[
    (y_i - \hat{y}_i)^2
\f]
*/

};

#endif // _LEASTSQS_H


