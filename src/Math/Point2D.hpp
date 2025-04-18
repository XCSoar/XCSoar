// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

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

  constexpr bool operator==(const Point2D<T, PT> &) const noexcept = default;

  constexpr Point2D<T, PT> operator+(Point2D<T, PT> other) const noexcept {
    return { scalar_type(x + other.x), scalar_type(y + other.y) };
  }

  constexpr Point2D<T, PT> operator-(Point2D<T, PT> other) const noexcept {
    return { scalar_type(x - other.x), scalar_type(y - other.y) };
  }

  constexpr Point2D<T, PT> operator-() const noexcept
    requires(std::is_signed_v<T>) {
    return { static_cast<T>(-x), static_cast<T>(-y) };
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

  [[gnu::pure]]
  double Magnitude() const noexcept {
    return std::hypot(x, y);
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
using DoublePoint2D = Point2D<double>;
using FloatPoint2D = Point2D<float>;

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
