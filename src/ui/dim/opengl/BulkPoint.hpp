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
  constexpr BulkPixelPoint() noexcept = default;

  constexpr BulkPixelPoint(int _x, int _y) noexcept
    :Point2D(_x, _y) {}

  constexpr BulkPixelPoint(PixelPoint src) noexcept
    :Point2D(src) {}

  constexpr BulkPixelPoint(Point2D<GLvalue, int> src) noexcept
    :Point2D(src) {}

  constexpr operator PixelPoint() const noexcept {
    return PixelPoint(x, y);
  }
};
