/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef _HISTOGRAM_H
#define _HISTOGRAM_H

#include "Math/Point2D.hpp"
#include "util/ConstBuffer.hxx"

#include <array>
#include <type_traits>

class Histogram
{
  static constexpr std::size_t NUM_SLOTS = 61;
  static constexpr double SPREAD = 0.15;

  unsigned n_pts;
  double m;
  double b;

  double x_min, x_max, y_max;

  std::array<DoublePoint2D, NUM_SLOTS> slots;

  using size_type = decltype(slots)::size_type;

public:
  bool empty() const noexcept {
    return n_pts == 0;
  }

  /**
   * Add a new data point to the values and convex solution
   *
   * @param x x-Value of the new data point
   */
  void UpdateHistogram(double x) noexcept;

  /**
   * Initialise the histogram, with specified range
   */
  void Reset(double smin, double smax) noexcept;

  /**
   * Clear counters
   */
  void Clear() noexcept;

  constexpr double GetMinX() const noexcept {
    return x_min;
  }

  constexpr double GetMaxX() const noexcept {
    return x_max;
  }

  constexpr double GetMaxY() const noexcept {
    return y_max;
  }

  /**
   * Return the x value associated with the cumulative percentile value,
   * counted from lowest up.
   */
  double GetPercentile(double p) const noexcept;

  constexpr ConstBuffer<DoublePoint2D> GetSlots() const noexcept {
    return {slots.data(), slots.size()};
  }

private:
  [[gnu::const]]
  size_type SlotNumber(double x) const noexcept;

  void IncrementSlot(size_type i, double mag) noexcept;
};

static_assert(std::is_trivial<Histogram>::value, "type is not trivial");

#endif // _HISTOGRAM_H
