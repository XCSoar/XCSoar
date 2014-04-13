/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2014 The XCSoar Project
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

#ifndef XCSOAR_POINT2D_HPP
#define XCSOAR_POINT2D_HPP

#include <type_traits>
#include <cmath>
#include <cstdlib>

template<typename T>
struct Point2D {
  typedef T scalar_type;

  scalar_type x, y;

  Point2D() = default;

  constexpr Point2D(scalar_type _x, scalar_type _y):x(_x), y(_y) {}

  constexpr bool operator==(const Point2D<T> &other) const {
    return x == other.x && y == other.y;
  }

  constexpr bool operator!=(const Point2D<T> &other) const {
    return !(*this == other);
  }

  constexpr Point2D<T> operator+(Point2D<T> other) const {
    return { scalar_type(x + other.x), scalar_type(y + other.y) };
  }

  constexpr Point2D<T> operator-(Point2D<T> other) const {
    return { scalar_type(x - other.x), scalar_type(y - other.y) };
  }

  Point2D<T> &operator+=(Point2D<T> other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  Point2D<T> &operator-=(Point2D<T> other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }
};

struct FloatPoint : Point2D<float> {
  /**
   * Type to be used by vector math.
   */
  typedef float product_type;

  FloatPoint() = default;

  template<typename... Args>
  constexpr FloatPoint(Args&&... args):Point2D<float>(args...) {}
};

template<typename P, typename RT=typename P::scalar_type>
static inline RT
DotProduct(P a, P b)
{
  static_assert(std::is_base_of<Point2D<typename P::scalar_type>, P>::value,
                "Must be Point2D");

  return RT(a.x) * RT(b.x) + RT(a.y) * RT(b.y);
}

template<typename P, typename RT=typename P::scalar_type>
static inline RT
CrossProduct(P a, P b)
{
  static_assert(std::is_base_of<Point2D<typename P::scalar_type>, P>::value,
                "Must be Point2D");

  return RT(a.x) * RT(b.y) - RT(b.x) * RT(a.y);
}

template<typename P>
static constexpr inline P
Normal(P a, P b)
{
  static_assert(std::is_base_of<Point2D<typename P::scalar_type>, P>::value,
                "Must be Point2D");

  return P(a.y - b.y, b.x - a.x);
}

/**
 * Calculates the "manhattan distance" or "taxicab distance".
 */
template<typename P, typename RT=typename P::scalar_type>
static inline RT
ManhattanDistance(P a, P b)
{
  static_assert(std::is_base_of<Point2D<typename P::scalar_type>, P>::value,
                "Must be Point2D");

  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

#endif
