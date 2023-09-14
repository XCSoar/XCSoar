// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "SearchPoint.hpp"
#include "Flat/FlatProjection.hpp"

SearchPoint::SearchPoint(const GeoPoint &loc,
                         const FlatProjection &tp) noexcept
  :location(loc), flat_location(tp.ProjectInteger(loc))
#ifndef NDEBUG
  , projected(true)
#endif
{
}

SearchPoint::SearchPoint(const FlatGeoPoint &floc,
                         const FlatProjection &tp) noexcept
  :location(tp.Unproject(floc)), flat_location(floc)
#ifndef NDEBUG
  , projected(true)
#endif
{
}

void
SearchPoint::Project(const FlatProjection &tp) noexcept
{
  flat_location = tp.ProjectInteger(location);

#ifndef NDEBUG
  projected = true;
#endif
}
