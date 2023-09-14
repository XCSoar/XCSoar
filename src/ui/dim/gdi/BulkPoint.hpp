// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project
#pragma once

#include "ui/dim/Point.hpp"

#include <windef.h>

/**
 * A point structure to be used in arrays.
 */
struct BulkPixelPoint : public tagPOINT {
  BulkPixelPoint() = default;

  constexpr BulkPixelPoint(LONG _x, LONG _y)
    :tagPOINT({_x, _y}) {}

  explicit constexpr BulkPixelPoint(const POINT &other):tagPOINT(other) {}

  constexpr BulkPixelPoint(PixelPoint src)
    :tagPOINT({src.x, src.y}) {}

  constexpr operator PixelPoint() const {
    return PixelPoint(x, y);
  }

  constexpr BulkPixelPoint operator+(BulkPixelPoint other) const {
    return { x + other.x, y + other.y };
  }

  constexpr BulkPixelPoint operator-(BulkPixelPoint other) const {
    return { x - other.x, y - other.y };
  }
};
