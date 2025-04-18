// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

#include <cstdint>

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

static_assert(std::is_trivial_v<RoughDistance>, "type is not trivial");
