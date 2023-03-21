// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/opengl/Types.hpp"
#include "ui/dim/Point.hpp"
#include "Math/Point2D.hpp"

/**
 * A point structure to be used in arrays.
 */
struct BulkPixelPoint : Point2D<GLvalue, int> {
  BulkPixelPoint() = default;

  constexpr BulkPixelPoint(int _x, int _y) noexcept
    :Point2D(_x, _y) {}

  constexpr BulkPixelPoint(PixelPoint src)
    :Point2D(src.x, src.y) {}

  constexpr operator PixelPoint() const {
    return PixelPoint(x, y);
  }
};
