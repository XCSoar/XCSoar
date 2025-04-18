// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"
#include "Math/Shift.hpp"

/**
 * A point within a RasterMap.
 */
struct RasterLocation : UnsignedPoint2D {
  using UnsignedPoint2D::UnsignedPoint2D;

  constexpr RasterLocation operator>>(unsigned bits) const noexcept {
    return RasterLocation(x >> bits, y >> bits);
  }

  constexpr RasterLocation operator<<(unsigned bits) const noexcept {
    return RasterLocation(x << bits, y << bits);
  }

  constexpr RasterLocation RoundingRightShift(unsigned bits) const noexcept {
    return RasterLocation(::RoundingRightShift(x, bits),
                          ::RoundingRightShift(y, bits));
  }
};

struct SignedRasterLocation : IntPoint2D {
  using IntPoint2D::IntPoint2D;

  constexpr SignedRasterLocation(RasterLocation other) noexcept
    :IntPoint2D(other.x, other.y) {}

  constexpr operator RasterLocation() const noexcept {
    return RasterLocation(x, y);
  }

  constexpr SignedRasterLocation operator>>(int bits) const noexcept {
    return SignedRasterLocation(x >> bits, y >> bits);
  }

  constexpr SignedRasterLocation operator<<(int bits) const noexcept {
    return SignedRasterLocation(x << bits, y << bits);
  }

  constexpr SignedRasterLocation RoundingRightShift(unsigned bits) const noexcept {
    return SignedRasterLocation(::RoundingRightShift(x, bits),
                                ::RoundingRightShift(y, bits));
  }
};
