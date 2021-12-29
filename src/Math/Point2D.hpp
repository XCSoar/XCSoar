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

#ifndef XCSOAR_POINT2D_HPP
#define XCSOAR_POINT2D_HPP

#include <type_traits>
#include <cmath>

template<typename T, typename PT=T>
requires(std::is_arithmetic_v<T> && std::is_arithmetic_v<PT>)
struct Point2D {
  using scalar_type = T;

  /**
   * Type to be used by vector math.
   */
  using product_type = PT;

  scalar_type x, y;

  Point2D() = default;

  constexpr Point2D(scalar_type _x, scalar_type _y) noexcept
    :x(_x), y(_y) {}

  /**
   * This constructor allows casting from a different Point2D
   * template instantiation.
   */
  template<typename OT, typename OPT>
  explicit constexpr Point2D(const Point2D<OT, OPT> &src) noexcept
    :x(static_cast<scalar_type>(src.x)),
     y(static_cast<scalar_type>(src.y)) {}

  constexpr bool operator==(const Point2D<T, PT> &other) const noexcept {
    return x == other.x && y == other.y;
  }

  constexpr bool operator!=(const Point2D<T, PT> &other) const noexcept {
    return !(*this == other);
  }

  constexpr Point2D<T, PT> operator+(Point2D<T, PT> other) const noexcept {
    return { scalar_type(x + other.x), scalar_type(y + other.y) };
  }

  constexpr Point2D<T, PT> operator-(Point2D<T, PT> other) const noexcept {
    return { scalar_type(x - other.x), scalar_type(y - other.y) };
  }

  Point2D<T, PT> &operator+=(Point2D<T, PT> other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }

  Point2D<T, PT> &operator-=(Point2D<T, PT> other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }

  constexpr product_type Area() const noexcept {
    return product_type(x) * product_type(y);
  }

  constexpr product_type MagnitudeSquared() const noexcept {
    return PT(x) * PT(x) + PT(y) * PT(y);
  }
};

struct UnsignedPoint2D : Point2D<unsigned> {
  UnsignedPoint2D() = default;
  using Point2D::Point2D;

  constexpr UnsignedPoint2D(Point2D<unsigned> src) noexcept
    :Point2D(src) {}
};

static_assert(std::is_trivial<UnsignedPoint2D>::value, "type is not trivial");

using IntPoint2D = Point2D<int>;

struct DoublePoint2D : Point2D<double> {
  DoublePoint2D() = default;
  using Point2D::Point2D;

  [[gnu::pure]]
  double Magnitude() const noexcept {
    return hypot(x, y);
  }
};

static_assert(std::is_trivial<DoublePoint2D>::value, "type is not trivial");

struct FloatPoint2D : Point2D<float> {
  FloatPoint2D() = default;
  using Point2D::Point2D;

  [[gnu::pure]]
  float Magnitude() const noexcept {
    return hypotf(x, y);
  }
};

static_assert(std::is_trivial<FloatPoint2D>::value, "type is not trivial");

template<typename P>
concept AnyPoint2D = std::is_base_of_v<Point2D<typename P::scalar_type,
                                               typename P::product_type>,
                                       P>;

template<AnyPoint2D P, typename RT=typename P::product_type>
constexpr P
operator+(P a, P b) noexcept
{
  return P(a.x + b.x, a.y + b.y);
}

template<AnyPoint2D P>
constexpr P
operator-(P a, P b) noexcept
{
  return P(a.x - b.x, a.y - b.y);
}

template<AnyPoint2D P, typename Z>
requires(std::is_arithmetic_v<Z>)
constexpr P
operator*(P a, Z z) noexcept
{
  return P(a.x * z, a.y * z);
}

template<AnyPoint2D P, typename Z>
requires(std::is_arithmetic_v<Z>)
constexpr P
operator/(P a, Z z) noexcept
{
  return P(a.x / z, a.y / z);
}

template<AnyPoint2D P, typename RT=typename P::product_type>
constexpr RT
DotProduct(P a, P b) noexcept
{
  return RT(a.x) * RT(b.x) + RT(a.y) * RT(b.y);
}

template<AnyPoint2D P, typename RT=typename P::product_type>
constexpr RT
CrossProduct(P a, P b) noexcept
{
  return RT(a.x) * RT(b.y) - RT(b.x) * RT(a.y);
}

/**
 * Calculates the "manhattan distance" or "taxicab distance".
 */
template<AnyPoint2D P, typename RT=typename P::scalar_type>
constexpr RT
ManhattanDistance(P a, P b) noexcept
{
  /* this function is similar to std::abs(), but is constexpr and
     works with unsigned types */
  auto AbsoluteDifference = [](RT a, RT b) -> RT {
    return a < b ? b - a : a - b;
  };

  return AbsoluteDifference(a.x, b.x) + AbsoluteDifference(a.y, b.y);
}

#endif
