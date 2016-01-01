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

#ifndef XCSOAR_ROUGH_ALTITUDE_HPP
#define XCSOAR_ROUGH_ALTITUDE_HPP

#include "Math/Util.hpp"
#include "Compiler.h"

#include <type_traits>

#include <limits.h>

/**
 * Store an rough altitude value, when the exact value is not needed.
 *
 * The accuracy is 1m, and the range is -32768..32767.
 */
class RoughAltitude {
  short value;

public:
  RoughAltitude() = default;

  explicit constexpr
  RoughAltitude(short _value):value(_value) {}

  explicit constexpr
  RoughAltitude(int _value):value((short)_value) {}

  RoughAltitude(double _value) {
    value = iround(_value);
  }

  /**
   * Create a representation of the largest possible value.
   */
  constexpr
  static RoughAltitude Max() {
    return RoughAltitude((short)SHRT_MAX);
  }

  RoughAltitude &operator=(short other) {
    value = other;
    return *this;
  }

  RoughAltitude &operator=(int other) {
    value = (short)other;
    return *this;
  }

  RoughAltitude &operator=(double other) {
    value = iround(other);
    return *this;
  }

  explicit constexpr
  operator short() const {
    return value;
  }

  explicit constexpr
  operator int() const {
    return value;
  }

  constexpr
  operator double() const {
    return double(value);
  }

  constexpr
  bool IsPositive() const {
    return value > 0;
  }

  constexpr
  bool IsNegative() const {
    return value < 0;
  }

  constexpr
  bool operator ==(const RoughAltitude other) const {
    return value == other.value;
  }

  constexpr
  bool operator !=(const RoughAltitude other) const {
    return value != other.value;
  }

  constexpr
  bool operator <(const RoughAltitude other) const {
    return value < other.value;
  }

  constexpr
  bool operator <=(const RoughAltitude other) const {
    return value <= other.value;
  }

  constexpr
  bool operator >(const RoughAltitude other) const {
    return value > other.value;
  }

  constexpr
  bool operator >=(const RoughAltitude other) const {
    return value >= other.value;
  }

  constexpr
  RoughAltitude operator+(const RoughAltitude other) const {
    return RoughAltitude(value + other.value);
  }

  constexpr
  RoughAltitude operator-(const RoughAltitude other) const {
    return RoughAltitude(value - other.value);
  }

  gcc_pure
  double operator*(const double other) const {
    return value * other;
  }

  gcc_pure
  double operator/(const double other) const {
    return double(value) / other;
  }

  gcc_pure
  double operator/(const RoughAltitude other) const {
    return double(value) / other.value;
  }

  RoughAltitude &operator+=(const RoughAltitude other) {
    value += other.value;
    return *this;
  }

  RoughAltitude &operator-=(const RoughAltitude other) {
    value -= other.value;
    return *this;
  }
};

gcc_pure
static inline
double operator*(const double a, const RoughAltitude b) {
  return b * a;
}

static_assert(std::is_trivial<RoughAltitude>::value, "type is not trivial");

#endif
