/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2015 The XCSoar Project
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

#include "Math/XYDataStore.hpp"

#include <type_traits>

class Histogram: public XYDataStore
{
  unsigned n_pts;
  double m;
  double b;
  void IncrementSlot(unsigned i, double mag);

public:

  /**
   * Add a new data point to the values and convex solution
   *
   * @param x x-Value of the new data point
   */
  void UpdateHistogram(double x);

  /**
   * Initialise the histogram, with specified range
   */
  void Reset(double smin, double smax);

  /**
   * Clear counters
   */
  void Clear();

  /**
   * Retrieve total number of points accumulated
   */
  unsigned GetAccumulator() const {
    return n_pts;
  }

  /**
   * Return the x value associated with the cumulative percentile value,
   * counted from lowest up.
   */
  double GetPercentile(const double p) const;
};

static_assert(std::is_trivial<Histogram>::value, "type is not trivial");

#endif // _HISTOGRAM_H
