/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#ifndef XCSOAR_STATE_CLOCK_HPP
#define XCSOAR_STATE_CLOCK_HPP

#include "Math/fixed.hpp"

#include <algorithm>

#include <assert.h>

/**
 * Track how much time the aircraft was in a certain state
 * (e.g. "moving").  It provides tools to decrement the measured
 * duration while the aircraft was not in that state.
 *
 * @param max_value the maximum value; at this peak, the countdown
 * will start, and after this duration, the state will be cleared
 * @param max_delta the maximum delta accepted by Add() and
 * Subtract(), to avoid hiccups due to temporary GPS outage
 */
template<unsigned max_value, unsigned max_delta>
class StateClock {
  fixed value;

public:
  void Clear() {
    value = fixed(0);
  }

  bool IsDefined() const {
    assert(!negative(value));

    return positive(value);
  }

private:
  static fixed LimitDelta(fixed delta) {
    assert(!negative(delta));

    return std::min(delta, fixed(max_delta));
  }

public:
  void Add(fixed delta) {
    assert(!negative(value));
    assert(value <= fixed(max_value));

    value += LimitDelta(delta);
    if (value > fixed(max_value))
      value = fixed(max_value);
  }

  void Subtract(fixed delta) {
    assert(!negative(value));
    assert(value <= fixed(max_value));

    value -= LimitDelta(delta);
    if (negative(value))
      value = fixed(0);
  }

  bool operator>=(fixed other) const {
    assert(positive(other));
    assert(!negative(value));
    assert(value <= fixed(max_value));

    return value >= other;
  }
};

#endif
