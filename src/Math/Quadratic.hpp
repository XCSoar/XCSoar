// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Util.hpp"

#include <cassert>

/**
 * Utility class for efficient solution of quadratic equations
 */
class Quadratic {
  const double da;
  const double b;
  const double denom;

public:
  /**
   * Constructor for quadratic function x^2+b*x+c=0
   *
   * @param _b Value of b
   * @param _c Value of c
   */
  constexpr Quadratic(double _b, double _c) noexcept
    :da(2), b(_b), denom(Square(b) - 4 * _c)
  {
  }

  /**
   * Constructor for quadratic function a*x^2+b*x+c=0
   *
   * @param _a Value of a
   * @param _b Value of b
   * @param _c Value of c
   */
  constexpr Quadratic(double _a, double _b, double _c) noexcept
    :da(2 * _a), b(_b), denom(Square(b) - 2 * da * _c)
  {
  }

  /**
   * Check if all solutions of quadratic are real
   *
   * @return True if quadratic has at least one real solution
   */
  constexpr bool Check() const noexcept {
    if (denom < 0)
      return false;

    if (da == 0)
      return false;

    return true;
  }

  /**
   * Returns largest real solution.  Valid only where check() has passed.
   *
   * @return greater x value of solutions
   */
  double SolutionMax() const noexcept {
    assert(Check());
    return Solution(da > 0);
  }

  /**
   * Returns smallest real solution.  Valid only where check() has passed.
   *
   * @return smallest x value of solutions
   */
  [[gnu::pure]]
  double SolutionMin() const noexcept {
    assert(Check());
    return Solution(da <= 0);
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
  [[gnu::pure]]
  double Solution(const bool positive) const noexcept {
    assert(Check());
    return (-b + (positive ? sqrt(denom) : -sqrt(denom))) / da;
  }
};
