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

#ifndef DIFF_FILTER_HPP
#define DIFF_FILTER_HPP

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
  DiffFilter() = default;

  /**
   * Constructor.  Initialises as if fed x_default continuously.
   *
   * @param x_default Default value of input
   */
  DiffFilter(const double x_default)
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
  double Update(double x0);

  /**
   * Resets filter as if fed value to produce y0
   *
   * @param x0 Steady state value of filter input
   * @param y0 Desired value of differentiated output
   */
  void Reset(double x0=0, double y0=0);
};


#endif
