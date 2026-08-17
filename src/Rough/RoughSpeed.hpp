// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

#include <cstdint>

/**
 * Store a rough speed value, when the exact value is not needed.
 *
 * The accuracy is about 16 mm/s. The range is 0 - 1023 m/s.
 */
class RoughSpeed {
  uint16_t value;

  static constexpr double SCALE = 64;
  static constexpr double MAX_MPS = 1023;

  static constexpr uint16_t Import(double x) {
    if (!(x > 0))
      return 0;

    if (x > MAX_MPS)
      x = MAX_MPS;

    return uint16_t(x * SCALE);
  }

  static constexpr double Export(uint16_t x) {
    return double(x) / SCALE;
  }

public:
  RoughSpeed() = default;
  RoughSpeed(double _value):value(Import(_value)) {}

  RoughSpeed &operator=(double other) {
    value = Import(other);
    return *this;
  }

  constexpr operator double() const {
    return Export(value);
  }
};

static_assert(std::is_trivial_v<RoughSpeed>, "type is not trivial");
