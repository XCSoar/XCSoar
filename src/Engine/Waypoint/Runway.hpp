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

#ifndef XCSOAR_RUNWAY_HPP
#define XCSOAR_RUNWAY_HPP

#include "Math/Angle.hpp"
#include "Math/Util.hpp"

#include <stdint.h>

class Runway {
  /** Main runway direction in degrees (0-359, -1 unknown) */
  int16_t direction;

  /** Main runway length in m (0 for unknown) */
  uint16_t length;

  constexpr Runway(int _direction, unsigned _length)
    :direction(_direction), length(_length) {}

public:
  /**
   * No initialisation.
   */
  Runway() = default;

  /**
   * Construct an empty instance.  Its IsDefined() method will return
   * false.
   */
  static constexpr Runway Null() {
    return { -1, 0 };
  }

  bool IsDirectionDefined() const {
    return direction >= 0;
  }

  bool IsLengthDefined() const {
    return length > 0;
  }

  void Clear() {
    ClearDirection();
    length = 0;
  }

  void ClearDirection() {
    direction = -1;
  }

  void SetDirection(Angle _direction) {
    direction = iround(_direction.AsBearing().Degrees());
  }

  void SetDirectionDegrees(unsigned degrees) {
    assert(degrees < 360);

    direction = degrees;
  }

  gcc_pure
  Angle GetDirection() const {
    assert(IsDirectionDefined());

    return Angle::Degrees(direction);
  }

  gcc_pure
  unsigned GetDirectionDegrees() const {
    assert(IsDirectionDefined());

    return direction;
  }

  gcc_pure
  unsigned GetDirectionName() const {
    assert(IsDirectionDefined());

    return (direction + 5) / 10;
  }

  void SetLength(unsigned _length) {
    length = _length;
  }

  gcc_pure
  unsigned GetLength() const {
    assert(IsLengthDefined());

    return length;
  }
};

#endif
