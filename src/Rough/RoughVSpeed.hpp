// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

#include <cstdint>

/**
 * Store an rough vertical speed value, when the exact value is not
 * needed.
 *
 * The accuracy is 1/256 m/s.
 */
class RoughVSpeed {
  int16_t value;

  constexpr
  static int16_t Import(double x) {
    return (int16_t)(x * 256);
  }

  constexpr
  static double Export(int16_t x) {
    return x / 256.;
  }

public:
  RoughVSpeed() = default;

  constexpr
  RoughVSpeed(double _value):value(Import(_value)) {}

  RoughVSpeed &operator=(double other) {
    value = Import(other);
    return *this;
  }

  constexpr
  operator double() const {
    return Export(value);
  }
};

static_assert(std::is_trivial_v<RoughVSpeed>, "type is not trivial");
