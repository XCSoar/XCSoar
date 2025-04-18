// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticArray.hxx"

#include <numeric>
#include <cassert>

/**
 * Average/bucket filter.  When filter is full, can return samples
 */
template<unsigned max>
class AvFilter 
{
protected:
  /** Values stored */
  StaticArray<double, max> x;

public:
  unsigned capacity() const {
    return x.capacity();
  }

  /**
   * Updates filter to add sample to buffer
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return True if buffer is full
   */
  bool Update(const double x0) {
    if (!x.full())
      x.append(x0);

    return x.full();
  }

  /**
   * Calculate average from samples
   *
   * @return Average value in buffer
   */
  [[gnu::pure]]
  double Average() const {
    assert(!x.empty());

    return std::accumulate(x.begin(), x.end(), 0.0) / x.size();
  }

  /**
   * Resets filter (zero samples)
   */
  void Reset() {
    x.clear();
  }
};
