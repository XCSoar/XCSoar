// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Math/Point2D.hpp"

struct PixelPoint : IntPoint2D {
  PixelPoint() = default;

  constexpr PixelPoint(IntPoint2D src) noexcept
    :IntPoint2D(src) {}

  using IntPoint2D::IntPoint2D;

  /**
   * Return a point relative to this one.
   */
  constexpr PixelPoint At(scalar_type delta_x,
                          scalar_type delta_y) const noexcept {
    return {x + delta_x, y + delta_y};
  }
};
