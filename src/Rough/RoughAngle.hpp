// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <type_traits>

#include <cstdint>

/**
 * Store an rough angle, when the exact value is not needed.
 *
 * The accuracy is about 0.02 degrees.
 */
class RoughAngle {
  int16_t value;

  static constexpr int16_t Import(Angle x) {
    return (int16_t)(x.Radians() * 4096);
  }

  static constexpr Angle Export(int16_t x) {
    return Angle::Radians(x / 4096.);
  }

  constexpr
  RoughAngle(int16_t _value):value(_value) {}

public:
  RoughAngle() = default;
  RoughAngle(Angle _value):value(Import(_value)) {}

  RoughAngle &operator=(Angle other) {
    value = Import(other);
    return *this;
  }

  operator Angle() const {
    return Export(value);
  }

  constexpr
  RoughAngle operator-(RoughAngle other) const {
    return RoughAngle(value - other.value);
  }
};

static_assert(std::is_trivial<RoughAngle>::value, "type is not trivial");
