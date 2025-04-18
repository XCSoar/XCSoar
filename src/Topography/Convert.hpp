// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "shapelib/mapprimitive.h"

constexpr GeoBounds
ImportRect(const rectObj r)
{
  return GeoBounds(GeoPoint(Angle::Degrees(r.minx),
                            Angle::Degrees(r.maxy)),
                   GeoPoint(Angle::Degrees(r.maxx),
                            Angle::Degrees(r.miny)));
}

[[gnu::pure]]
static inline rectObj
ConvertRect(const GeoBounds &br)
{
  rectObj dest;
  dest.minx = br.GetWest().Degrees();
  dest.maxx = br.GetEast().Degrees();
  dest.miny = br.GetSouth().Degrees();
  dest.maxy = br.GetNorth().Degrees();
  return dest;
}
