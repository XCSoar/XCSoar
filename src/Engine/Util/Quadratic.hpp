/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef QUADRATIC_HPP
#define QUADRATIC_HPP

#include "Math/fixed.hpp"
#include "Compiler.h"

#include <assert.h>

/**
 * Utility class for efficient solution of quadratic equations
 */
class Quadratic {
  const fixed da;
  const fixed b;
  const fixed denom;

public:
  /**
   * Constructor for quadratic function x^2+b*x+c=0
   *
   * @param _b Value of b
   * @param _c Value of c
   */
  Quadratic(const fixed _b, const fixed _c) :
    da(fixed_two), b(_b), denom(b * b - fixed_four * _c)
  {
  }

  /**
   * Constructor for quadratic function a*x^2+b*x+c=0
   *
   * @param _a Value of a
   * @param _b Value of b
   * @param _c Value of c
   */
  Quadratic(const fixed _a, const fixed _b, const fixed _c) :
    da(fixed_two * _a), b(_b), denom(b * b - fixed_two * da * _c)
  {
  }

  /**
   * Check if all solutions of quadratic are real
   *
   * @return True if quadratic has at least one real solution
   */
  gcc_pure bool
  Check() const
  {
    if (negative(denom))
      return false;

    if (da == fixed_zero)
      return false;

    return true;
  }

  /**
   * Returns largest real solution.  Valid only where check() has passed.
   *
   * @return greater x value of solutions
   */
  gcc_pure fixed
  SolutionMax() const
  {
    assert(Check());
    return positive(da) ? Solution(true) : Solution(false);
  }

  /**
   * Returns smallest real solution.  Valid only where check() has passed.
   *
   * @return smallest x value of solutions
   */
  gcc_pure fixed
  SolutionMin() const
  {
    assert(Check());
    return positive(da) ? Solution(false) : Solution(true);
  }

private:
  /**
   * Calculate solution of quadratic equation using relation:
   *   x = (-b +/- sqrt(b^2-4*a*c))/(2*a)
   *
   * @param positive whether positive or negative sqrt is used
   *
   * @return x value of solution
   */
  gcc_pure fixed
  Solution(const bool positive) const
  {
    assert(Check());
    return (-b + (positive ? sqrt(denom) : -sqrt(denom))) / da;
  }
};

#endif
