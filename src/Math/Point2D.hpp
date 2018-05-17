/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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

#include "Compiler.h"

#include <type_traits>
#include <cmath>
#include <cstdlib>

template<typename T, typename PT=T>
struct Point2D {
  typedef T scalar_type;

  /**
   * Type to be used by vector math.
   */
  typedef PT product_type;

  scalar_type x, y;

  Point2D() = default;

  constexpr Point2D(scalar_type _x, scalar_type _y):x(_x), y(_y) {}

  constexpr bool operator==(const Point2D<T, PT> &other) const {
    return x == other.x && y == other.y;
  }

  constexpr bool operator!=(const Point2D<T, PT> &other) const {
    return !(*this == other);
  }

  constexpr Point2D<T, PT> operator+(Point2D<T, PT> other) const {
    return { scalar_type(x + other.x), scalar_type(y + other.y) };
  }

  constexpr Point2D<T, PT> operator-(Point2D<T, PT> other) const {
    return { scalar_type(x - other.x), scalar_type(y - other.y) };
  }

  Point2D<T, PT> &operator+=(Point2D<T, PT> other) {
    x += other.x;
    y += other.y;
    return *this;
  }

  Point2D<T, PT> &operator-=(Point2D<T, PT> other) {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  constexpr product_type MagnitudeSquared() const {
    return PT(x) * PT(x) + PT(y) * PT(y);
  }
};

struct UnsignedPoint2D : Point2D<unsigned> {
  UnsignedPoint2D() = default;

  constexpr UnsignedPoint2D(unsigned _x, unsigned _y):Point2D<unsigned>(_x, _y) {}
};

static_assert(std::is_trivial<UnsignedPoint2D>::value, "type is not trivial");

struct IntPoint2D : Point2D<int> {
  IntPoint2D() = default;

  constexpr IntPoint2D(int _x, int _y):Point2D<int>(_x, _y) {}
};

static_assert(std::is_trivial<IntPoint2D>::value, "type is not trivial");

struct DoublePoint2D : Point2D<double> {
  DoublePoint2D() = default;

  constexpr DoublePoint2D(double _x, double _y):Point2D<double>(_x, _y) {}

  gcc_pure
  double Magnitude() const {
    return hypot(x, y);
  }
};

static_assert(std::is_trivial<DoublePoint2D>::value, "type is not trivial");

struct FloatPoint2D : Point2D<float> {
  FloatPoint2D() = default;

  constexpr FloatPoint2D(float _x, float _y):Point2D<float>(_x, _y) {}

  gcc_pure
  float Magnitude() const {
    return hypotf(x, y);
  }
};

static_assert(std::is_trivial<FloatPoint2D>::value, "type is not trivial");

template<typename P>
struct IsPoint2D : std::is_base_of<Point2D<typename P::scalar_type,
                                           typename P::product_type>, P> {
};

template<typename P>
struct EnableIfPoint2D : std::enable_if<IsPoint2D<P>::value> {
};

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type>
static constexpr inline P
operator+(P a, P b)
{
  return P(a.x + b.x, a.y + b.y);
}

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type>
static constexpr inline P
operator-(P a, P b)
{
  return P(a.x - b.x, a.y - b.y);
}

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type,
         typename Z,
         typename=typename std::enable_if<std::is_arithmetic<Z>::value>::type>
static constexpr inline P
operator*(P a, Z z)
{
  return P(a.x * z, a.y * z);
}

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type,
         typename Z,
         typename=typename std::enable_if<std::is_arithmetic<Z>::value>::type>
static constexpr inline P
operator/(P a, Z z)
{
  return P(a.x / z, a.y / z);
}

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type>
static constexpr inline RT
DotProduct(P a, P b)
{
  return RT(a.x) * RT(b.x) + RT(a.y) * RT(b.y);
}

template<typename P, typename RT=typename P::product_type,
         typename=typename EnableIfPoint2D<P>::type>
static constexpr inline RT
CrossProduct(P a, P b)
{
  return RT(a.x) * RT(b.y) - RT(b.x) * RT(a.y);
}

/**
 * Calculates the "manhattan distance" or "taxicab distance".
 */
template<typename P, typename RT=typename P::scalar_type,
         typename=typename EnableIfPoint2D<P>::type>
static inline RT
ManhattanDistance(P a, P b)
{
  return std::abs(a.x - b.x) + std::abs(a.y - b.y);
}

#endif
