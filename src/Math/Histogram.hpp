// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"

#include <array>
#include <span>
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

  constexpr std::span<const DoublePoint2D> GetSlots() const noexcept {
    return slots;
  }

private:
  [[gnu::const]]
  size_type SlotNumber(double x) const noexcept;

  void IncrementSlot(size_type i, double mag) noexcept;
};

static_assert(std::is_trivial<Histogram>::value, "type is not trivial");
