// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

/**
 * Implements a filter that maintains a convex shape
 */

#pragma once

#include "Math/LeastSquares.hpp"

#include <cassert>
#include <type_traits>

class ConvexFilter: public LeastSquares
{
public:

  /**
   * Add a new data point to the values and convex solution
   *
   * @param x x-Value of the new data point
   * @param y y-Value of the new data point
   */
  void UpdateConvexPositive(double x, double y) noexcept {
    UpdateConvex(x, y, 1);
  }
  void UpdateConvexNegative(double x, double y) noexcept {
    UpdateConvex(x, y, -1);
  }

  double GetLastY() const noexcept {
    assert(!IsEmpty());

    return GetSlots()[GetCount() - 1].y;
  }

private:
  void UpdateConvex(double x, double y, int csign) noexcept;
};

static_assert(std::is_trivial<ConvexFilter>::value, "type is not trivial");
