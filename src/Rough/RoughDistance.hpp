/*
Copyright_License {

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

#ifndef XCSOAR_ROUGH_DISTANCE_HPP
#define XCSOAR_ROUGH_DISTANCE_HPP

#include <type_traits>

#include <stdint.h>

/**
 * Store an rough distance value, when the exact value is not needed.
 *
 * The accuracy is 1m.
 */
class RoughDistance {
  uint32_t value;

public:
  RoughDistance() = default;

  constexpr
  RoughDistance(double _value):value(_value) {}

  RoughDistance &operator=(double other) {
    value = (uint32_t)other;
    return *this;
  }

  constexpr
  operator double() const {
    return double(value);
  }

  constexpr
  bool operator <(const RoughDistance other) const {
    return value < other.value;
  }

  constexpr
  bool operator >(const RoughDistance other) const {
    return value > other.value;
  }

  constexpr
  bool IsZero() const {
    return value == 0;
  }
};

static_assert(std::is_trivial<RoughDistance>::value, "type is not trivial");

#endif
