// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "ui/dim/Point.hpp"

#include <windef.h>

/**
 * A point structure to be used in arrays.
 */
struct BulkPixelPoint : public tagPOINT {
  constexpr BulkPixelPoint() noexcept = default;

  constexpr BulkPixelPoint(LONG _x, LONG _y) noexcept
    :tagPOINT({_x, _y}) {}

  explicit constexpr BulkPixelPoint(const POINT &other) noexcept
    :tagPOINT(other) {}

  constexpr BulkPixelPoint(PixelPoint src) noexcept
    :tagPOINT({src.x, src.y}) {}

  constexpr operator PixelPoint() const noexcept {
    return PixelPoint(x, y);
  }

  constexpr BulkPixelPoint operator+(BulkPixelPoint other) const noexcept {
    return { x + other.x, y + other.y };
  }

  constexpr BulkPixelPoint operator-(BulkPixelPoint other) const noexcept {
    return { x - other.x, y - other.y };
  }

  constexpr BulkPixelPoint operator-() const noexcept {
    return { -x, -y };
  }

  constexpr BulkPixelPoint &operator+=(BulkPixelPoint other) noexcept {
    x += other.x;
    y += other.y;
    return *this;
  }

  constexpr BulkPixelPoint &operator-=(BulkPixelPoint other) noexcept {
    x -= other.x;
    y -= other.y;
    return *this;
  }
};
