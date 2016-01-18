/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#ifndef ZERO_FINDER_HPP
#define ZERO_FINDER_HPP

#include "Compiler.h"

#include <assert.h>

/**
 * Zero finding and minimisation search algorithm
 *
 * Can be given initial value as hint of solution.
 *
 */
class ZeroFinder {
protected:
  /** min value of search range */
  const double xmin;
  /** max value of search range */
  const double xmax;
  /** search tolerance in x */
  const double tolerance;

public:
  /**
   * Constructor of zero finder search algorithm
   *
   * @param _xmin Minimum allowable value of x
   * @param _xmax Maximum allowable value of x
   * @param _tolerance Absolute tolerance of solution (in x)
   */
  ZeroFinder(const double _xmin, const double _xmax, const double _tolerance)
    :xmin(_xmin), xmax(_xmax), tolerance(_tolerance)
  {
    assert(xmin < xmax);
  }

  /**
   * Abstract method for function to be minimised or root-solved
   *
   * @param x Value of x
   *
   * @return f(x)
   */
  virtual double f(const double x) = 0;

  /**
   * Find closest value of x that produces f(x)=0
   * Method used is a variant of a bisector search.
   * To enforce search, set xstart outside range 
   *
   * @param xstart Initial guess of x
   *
   * @return x value of best solution
   */
  gcc_pure
  double find_zero(const double xstart);

  /**
   * Find value of x that minimises f(x)
   * Method used is a variant of a bisector search.
   * To enforce search, set xstart outside range 
   *
   * @param xstart Initial guess of x
   *
   * @return x value of best solution
   */
  gcc_pure
  double find_min(const double xstart);

private:
  gcc_pure
  double find_zero_actual(const double xstart);

  gcc_pure
  double find_min_actual(const double xstart);

  /**
   * Tolerance in f of minimisation routine at x
   */
  gcc_pure
  double tolerance_actual_min(const double x) const;

  /**
   * Tolerance in f of zero finding routine at x
   */
  gcc_pure
  double tolerance_actual_zero(const double x) const;

  /**
   * Test whether solution is within tolerance
   *
   * @param xstart Initial value of x
   * @param tol_act Actual tolerance of routine evaluated at xstart
   *
   * @return true if no search required (xstart is good)
   */
  gcc_pure
  bool solution_within_tolerance(const double xstart,
                                 const double tol_act);

};

#endif
