#ifndef ZERO_FINDER_HPP
#define ZERO_FINDER_HPP

/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
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

#include "Math/fixed.hpp"

/**
 * Zero finding and minimisation search algorithm
 */
class ZeroFinder {
public:
  /**
   * Constructor of zero finder search algorithm
   *
   * @param _xmin Minimum allowable value of x
   * @param _xmax Maximum allowable value of x
   * @param _tolerance Absolute tolerance of solution (in x)
   */
  ZeroFinder(const fixed _xmin, const fixed _xmax, const fixed _tolerance) :
    xmin(_xmin), xmax(_xmax), tolerance(_tolerance)
  {
  }

  /**
   * Abstract method for function to be minimised or root-solved
   *
   * @param x Value of x
   *
   * @return f(x)
   */
  virtual fixed f(const fixed x) = 0;

  /**
   * Find closest value of x that produces f(x)=0
   * Method used is a variant of a bisector search.
   *
   * @param xstart Initial value of x (not used)
   *
   * @return x value of best solution
   */
  fixed find_zero(const fixed xstart);

  /**
   * Find value of x that minimises f(x)
   * Method used is a variant of a bisector search.
   *
   * @param xstart Initial value of x (not used)
   *
   * @return x value of best solution
   */
  fixed find_min(const fixed xstart);

protected:
  /** min value of search range */
  const fixed xmin;
  /** max value of search range */
  const fixed xmax;
  /** search tolerance */
  const fixed tolerance;

private:
  /** machine tolerance */
  static const fixed epsilon;
  /** sqrt of machine tolerance */
  static const fixed sqrt_epsilon;
  static const fixed r;
};

#endif
