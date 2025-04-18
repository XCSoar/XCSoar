// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "OLCTriangleRules.hpp"
#include "Geo/Flat/FlatProjection.hpp"

OLCTriangleValidator
OLCTriangleRules::MakeValidator(const FlatProjection &projection,
                                const GeoPoint &reference) noexcept
{
  // note: this is _not_ the breakepoint between small and large triangles,
  // but a slightly lower value used for relaxed large triangle checking.
  const unsigned large_threshold_flat =
    projection.ProjectRangeInteger(reference, large_threshold_m) * 0.99;

  return OLCTriangleValidator(large_threshold_flat);
}
