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

#ifndef XCSOAR_MATH_FASTROTATION_HPP
#define XCSOAR_MATH_FASTROTATION_HPP

#include "Math/Angle.hpp"
#include "Point2D.hpp"

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

  template<typename P, typename=std::enable_if_t<std::is_base_of_v<Point, P>>>
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
    cost = cost * multiply / divide;
    sint = sint * multiply / divide;
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

  template<typename P, typename=std::enable_if_t<std::is_base_of_v<Point, P>>>
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

#endif
