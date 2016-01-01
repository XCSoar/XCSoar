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

#ifndef AV_FILTER_HPP
#define AV_FILTER_HPP

#include "Util/StaticArray.hxx"
#include "Compiler.h"

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
  gcc_pure
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

#endif
