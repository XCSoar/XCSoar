// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Quadrilateral.hpp"
#include "GeoBounds.hpp"

GeoBounds
GeoQuadrilateral::GetBounds() const noexcept
{
  GeoBounds bounds = GeoBounds::Invalid();

  /* Normalize longitudes so antimeridian wrap is detected correctly
     (ScreenToGeo can yield values outside ±180° while panning). */
  for (GeoPoint p : {top_left, top_right, bottom_left, bottom_right}) {
    p.Normalize();
    bounds.Extend(p);
  }

  return bounds;
}
