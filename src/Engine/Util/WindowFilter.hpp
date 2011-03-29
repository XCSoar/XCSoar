/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#ifndef WINDOW_FILTER_HPP
#define WINDOW_FILTER_HPP

#include "AvFilter.hpp"

/**
 * Average/window filter.  
 */
template<unsigned max>
class WindowFilter : public AvFilter<max>
{
public:
  /**
   * Updates filter to add sample to buffer
   *
   * @param x0 Input (pre-filtered) value at sample time
   *
   * @return True if buffer is full
   *
   */
  bool update(const fixed x0) {
    StaticArray<fixed, max> &x = this->x;

    assert(i < x.capacity());

    if (!x.full())
      return AvFilter<max>::update(x0);

    x[i] = x0;
    i = (i + 1) % x.capacity();
    return x.full();
  }

  /**
   * Resets filter (zero samples)
   */
  void reset() {
    AvFilter<max>::reset();
    i = 0;
  }

private:
  unsigned i;
};

#endif
