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

#ifndef XCSOAR_DERIVE_WINDOW_FILTER_HPP
#define XCSOAR_DERIVE_WINDOW_FILTER_HPP

#include "Util/OverwritingRingBuffer.hpp"
#include "Compiler.h"

/**
 * A filter that stores a certain amount of samples and calculates the
 * derivative of the first and the last sample.  The motivation to
 * write this one was to answer the question "what was the average
 * task speed over the last 60 minutes?"
 */
template<unsigned N>
class DifferentialWindowFilter {
  struct Sample {
    double x, y;
  };

  OverwritingRingBuffer<Sample, N> buffer;

public:
  void Clear() {
    buffer.clear();
  }

  bool IsEmpty() const {
    return buffer.empty();
  }

  double GetFirstX() const {
    return buffer.peek().x;
  }

  double GetLastX() const {
    return buffer.last().x;
  }

  /**
   * Returns the difference between the first and the last X sample.
   * Must not be called on an empty object.
   */
  double GetDeltaX() const {
    return GetLastX() - GetFirstX();
  }

  /**
   * Same as GetDeltaX(), but returns -1 if the object is empty.
   */
  double GetDeltaXChecked() const {
    return IsEmpty() ? -1. : GetDeltaX();
  }

  double GetDeltaY() const {
    return buffer.last().y - buffer.peek().y;
  }

  /**
   * Add a new sample.
   */
  void Push(double x, double y) {
    buffer.push({x, y});
  }

  /**
   * Does this object have enough data to calculate the derivative of
   * at least the specified delta?
   */
  gcc_pure
  bool HasEnoughData(double min_delta_x) const {
    return !IsEmpty() && GetDeltaX() >= min_delta_x;
  }

  /**
   * Calculate the average dy/dx over the whole window.  This may only
   * be called after HasEnoughData() has returned true.
   */
  gcc_pure
  double DeriveAverage() const {
    auto delta_x = GetDeltaX();
    auto delta_y = GetDeltaY();
    return delta_y / delta_x;
  }
};

#endif
