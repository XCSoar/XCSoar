// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "GeoPoint.hpp"

class GeoBounds;

/**
 * A quadrilateral on earth's surface.
 *
 * The four vertices describe the location of a planar rectangle
 * (e.g. a #Bitmap) on earth's surface.
 */
struct GeoQuadrilateral {
  GeoPoint top_left, top_right, bottom_left, bottom_right;

  static constexpr GeoQuadrilateral Undefined() noexcept {
    return {GeoPoint::Invalid(), GeoPoint::Invalid(),
        GeoPoint::Invalid(), GeoPoint::Invalid()};
  }

  constexpr bool IsDefined() const noexcept {
    return top_left.IsValid();
  }

  constexpr bool Check() const noexcept {
    return top_left.Check() && top_right.Check() &&
      bottom_left.Check() && bottom_right.Check();
  }

  [[gnu::pure]]
  GeoBounds GetBounds() const noexcept;
};
