// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Math/Util.hpp"

#include <cstdint>

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

  [[gnu::pure]]
  Angle GetDirection() const {
    assert(IsDirectionDefined());

    return Angle::Degrees(direction);
  }

  [[gnu::pure]]
  unsigned GetDirectionDegrees() const {
    assert(IsDirectionDefined());

    return direction;
  }

  [[gnu::pure]]
  unsigned GetDirectionName() const {
    assert(IsDirectionDefined());

    return (direction + 5) / 10;
  }

  void SetLength(unsigned _length) {
    length = _length;
  }

  [[gnu::pure]]
  unsigned GetLength() const {
    assert(IsLengthDefined());

    return length;
  }
};
