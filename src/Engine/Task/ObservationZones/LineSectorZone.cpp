// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "LineSectorZone.hpp"
#include "Geo/Flat/FlatLine.hpp"
#include "Geo/Flat/FlatProjection.hpp"

GeoPoint
LineSectorZone::GetNearestPoint(const FlatProjection &projection,
                                const GeoPoint &location) const noexcept
{
  const FlatLine line{projection.ProjectFloat(GetSectorStart()),
                      projection.ProjectFloat(GetSectorEnd())};

  /* SetLength() and the constructor are public: guard
     ProjectedRatio() against a division by zero on a gate of zero
     length */
  if (line.GetSquaredDistance() <= 0)
    return GeoPoint::Invalid();

  const auto ratio = line.ProjectedRatio(projection.ProjectFloat(location));
  return projection.Unproject(line.InterpolateClip(ratio));
}

double
LineSectorZone::ScoreAdjustment() const noexcept
{
  return 0;
}
