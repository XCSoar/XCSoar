// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Util.hpp"

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
  RoughAltitude() noexcept = default;

  explicit constexpr RoughAltitude(short _value) noexcept
    :value(_value) {}

  explicit constexpr RoughAltitude(int _value) noexcept
    :value((short)_value) {}

  RoughAltitude(double _value) noexcept {
    value = iround(_value);
  }

  /**
   * Create a representation of the largest possible value.
   */
  static constexpr RoughAltitude Max() noexcept {
    return RoughAltitude((short)SHRT_MAX);
  }

  RoughAltitude &operator=(short other) noexcept {
    value = other;
    return *this;
  }

  RoughAltitude &operator=(int other) noexcept {
    value = (short)other;
    return *this;
  }

  RoughAltitude &operator=(double other) noexcept {
    value = iround(other);
    return *this;
  }

  explicit constexpr operator short() const noexcept {
    return value;
  }

  explicit constexpr operator int() const noexcept {
    return value;
  }

  constexpr operator double() const noexcept {
    return double(value);
  }

  constexpr bool IsPositive() const noexcept {
    return value > 0;
  }

  constexpr bool IsNegative() const noexcept {
    return value < 0;
  }

  friend constexpr auto operator<=>(RoughAltitude,
                                    RoughAltitude) noexcept = default;

  constexpr RoughAltitude operator+(const RoughAltitude other) const noexcept {
    return RoughAltitude(value + other.value);
  }

  constexpr RoughAltitude operator-(const RoughAltitude other) const noexcept {
    return RoughAltitude(value - other.value);
  }

  constexpr double operator*(const double other) const noexcept {
    return value * other;
  }

  constexpr double operator/(const double other) const noexcept {
    return double(value) / other;
  }

  constexpr double operator/(const RoughAltitude other) const noexcept {
    return double(value) / other.value;
  }

  constexpr RoughAltitude &operator+=(const RoughAltitude other) noexcept {
    value += other.value;
    return *this;
  }

  constexpr RoughAltitude &operator-=(const RoughAltitude other) noexcept {
    value -= other.value;
    return *this;
  }
};

static constexpr
double operator*(const double a, const RoughAltitude b) noexcept
{
  return b * a;
}

static_assert(std::is_trivial_v<RoughAltitude>, "type is not trivial");
