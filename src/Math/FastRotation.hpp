// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Angle.hpp"
#include "Point2D.hpp"

#include <algorithm>
#include <cstdint>
#include <limits>

/**
 * Rotate coordinates around the zero origin.
 */
class FastRotation {
  double cost = 1, sint = 0;

public:
  using Point = DoublePoint2D;

  FastRotation() noexcept = default;

  /**
   * @param angle an angle between 0 and 360
   */
  FastRotation(Angle angle) noexcept
    :cost(angle.fastcosine()), sint(angle.fastsine()) {}

  /**
   * Rotates the point (xin, yin).
   *
   * @param x X value
   * @param y Y value
   * @return the rotated coordinates
   */
  constexpr Point Rotate(Point p) const noexcept {
    return {
      p.x * cost - p.y * sint,
      p.y * cost + p.x * sint,
    };
  }

  template<AnyPoint2D P>
  constexpr P Rotate(P p) const noexcept {
    return Rotate(Point{p});
  }
};

/**
 * Same as #FastRotation, but works with integer coordinates.
 */
class FastIntegerRotation {
public:
  static constexpr int SHIFT = 10;
  static constexpr int ONE = 1 << SHIFT;
  static constexpr int HALF = ONE / 2;

private:
  int cost = ONE, sint = 0;

  friend class FastRowRotation;

public:
  using Point = IntPoint2D;

  FastIntegerRotation() noexcept = default;

  FastIntegerRotation(Angle angle) noexcept
    :cost(angle.ifastcosine()), sint(angle.ifastsine()) {}

  void Scale(int multiply, int divide=1) noexcept {
    // Use int64_t for intermediate calculations to prevent overflow
    // This is especially important on high-DPI devices where values can be large
    const int64_t cost64 = static_cast<int64_t>(cost) * multiply / divide;
    const int64_t sint64 = static_cast<int64_t>(sint) * multiply / divide;

    // Clamp to int range to prevent overflow
    constexpr int64_t MAX_INT = static_cast<int64_t>(std::numeric_limits<int>::max());
    constexpr int64_t MIN_INT = static_cast<int64_t>(std::numeric_limits<int>::min());
    cost = static_cast<int>(std::clamp(cost64, MIN_INT, MAX_INT));
    sint = static_cast<int>(std::clamp(sint64, MIN_INT, MAX_INT));
  }

  constexpr Point RotateRaw(Point p) const noexcept {
    return {
      p.x * cost - p.y * sint,
      p.y * cost + p.x * sint,
    };
  }

  /**
   * Rotates the point (xin, yin).
   *
   * @param x X value
   * @param y Y value
   * @return the rotated coordinates
   */
  constexpr Point Rotate(Point p) const noexcept {
    const auto raw = RotateRaw(p);
    return {
      (raw.x + HALF) >> SHIFT,
      (raw.y + HALF) >> SHIFT,
    };
  }

  template<AnyPoint2D P>
  constexpr P Rotate(P p) const noexcept {
    return Rotate(Point{p});
  }
};

/**
 * Similar to FastIntegerRotation, but supports scanning one screen
 * row (y is constant).
 */
class FastRowRotation {
  FastIntegerRotation fir;
  int y_cost, y_sint;

public:
  using Point = FastIntegerRotation::Point;

  constexpr FastRowRotation(const FastIntegerRotation &_fir, int y) noexcept
    :fir(_fir),
     y_cost(y * fir.cost + 512), y_sint(y * fir.sint - 512) {}

  constexpr Point Rotate(int x) const noexcept {
    return Point((x * fir.cost - y_sint + 512) >> 10,
                 (y_cost + x * fir.sint + 512) >> 10);
  }
};
