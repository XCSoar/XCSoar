// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

#include <cstdint>

/**
 * Store an rough speed value, when the exact value is not needed.
 *
 * The accuracy is about 2mm/s. The range is 0 - 127 m/s.
 */
class RoughSpeed {
  uint16_t value;

  static constexpr uint16_t Import(double x) {
    return (uint16_t)(x * 512);
  }

  static constexpr double Export(uint16_t x) {
    return double(x) / 512;
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
