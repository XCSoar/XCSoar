// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "AvFilter.hpp"

/**
 * Average/window filter.  
 */
template<unsigned max>
class WindowFilter : public AvFilter<max>
{
  unsigned i = 0;

public:
  /**
   * Updates filter to add sample to buffer
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return True if buffer is full
   *
   */
  bool Update(const double x0) {
    auto &x = this->x;

    assert(i < x.capacity());

    if (!x.full())
      return AvFilter<max>::Update(x0);

    x[i] = x0;
    i = (i + 1) % x.capacity();
    return x.full();
  }

  /**
   * Resets filter (zero samples)
   */
  void Reset() {
    AvFilter<max>::Reset();
    i = 0;
  }
};
