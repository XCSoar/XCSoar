/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#ifndef XCSOAR_TIME_STAMP_HPP
#define XCSOAR_TIME_STAMP_HPP

#include "FloatDuration.hxx"

struct NMEAInfo;

/**
 * This class describes a time point during a flight.  It is used for
 * XCSoar's calculations.  Internally, it is the duration since
 * midnight of the day the flight started.
 */
class TimeStamp {
  FloatDuration value;

public:
  TimeStamp() noexcept = default;

  explicit constexpr TimeStamp(FloatDuration _value) noexcept
    :value(_value) {}

  static constexpr TimeStamp Undefined() noexcept {
    return TimeStamp{FloatDuration{-1}};
  }

  constexpr bool IsDefined() const noexcept {
    return value.count() >= 0;
  }

  constexpr FloatDuration ToDuration() const noexcept {
    return value;
  }

  template<typename T>
  constexpr auto Cast() const noexcept {
    return std::chrono::duration_cast<T>(value);
  }

  constexpr bool operator==(TimeStamp other) const noexcept {
    return value == other.value;
  }

  constexpr bool operator!=(TimeStamp other) const noexcept {
    return value != other.value;
  }

  constexpr bool operator<(TimeStamp other) const noexcept {
    return value < other.value;
  }

  constexpr bool operator<=(TimeStamp other) const noexcept {
    return value <= other.value;
  }

  constexpr bool operator>(TimeStamp other) const noexcept {
    return value > other.value;
  }

  constexpr bool operator>=(TimeStamp other) const noexcept {
    return value >= other.value;
  }

  constexpr FloatDuration operator-(TimeStamp other) const noexcept {
    return value - other.value;
  }

  constexpr auto operator+(FloatDuration delta) const noexcept {
    return TimeStamp{value + delta};
  }

  constexpr auto operator-(FloatDuration delta) const noexcept {
    return TimeStamp{value - delta};
  }

  auto &operator+=(FloatDuration delta) noexcept {
    value += delta;
    return *this;
  }
};

static inline FloatDuration
fdim(TimeStamp a, TimeStamp b) noexcept
{
  return fdim(a.ToDuration(), b.ToDuration());
}

#endif
