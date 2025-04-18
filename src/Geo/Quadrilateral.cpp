// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Quadrilateral.hpp"
#include "GeoBounds.hpp"

#include <algorithm>

GeoBounds
GeoQuadrilateral::GetBounds() const noexcept
{
  // TODO: not wraparound-safe

  const auto longitude = std::minmax({top_left.longitude, top_right.longitude, bottom_left.longitude, bottom_right.longitude});
  const auto latitude = std::minmax({top_left.latitude, top_right.latitude, bottom_left.latitude, bottom_right.latitude});

  return GeoBounds(GeoPoint(longitude.first, latitude.second),
                   GeoPoint(longitude.second, latitude.first));
}
