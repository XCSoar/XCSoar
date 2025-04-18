// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <array>

/**
 * Differentiating low-pass IIR filter
 * @see http://www.dsprelated.com/showarticle/35.php
 */
class DiffFilter 
{
  std::array<double, 7> x;

public:
  /**
   * Non-initialising default constructor.  To initialise this
   * instance, call Reset().
   */
  DiffFilter() noexcept = default;

  /**
   * Constructor.  Initialises as if fed x_default continuously.
   *
   * @param x_default Default value of input
   */
  DiffFilter(const double x_default) noexcept
  {
    Reset(x_default);
  }

  /**
   * Updates low-pass differentiating filter to calculate filtered
   * output given an input sample
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return Filter output value
   */
  double Update(double x0) noexcept;

  /**
   * Resets filter as if fed value to produce y0
   *
   * @param x0 Steady state value of filter input
   * @param y0 Desired value of differentiated output
   */
  void Reset(double x0=0, double y0=0) noexcept;
};
