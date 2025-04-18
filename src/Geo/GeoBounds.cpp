// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GeoBounds.hpp"

bool
GeoBounds::Extend(const GeoPoint pt) noexcept
{
  if (!pt.IsValid())
    return false;

  if (IsValid()) {
    bool a = longitude.Extend(pt.longitude);
    bool b = latitude.Extend(pt.latitude);
    return a || b;
  } else {
    *this = GeoBounds(pt);
    return true;
  }
}

bool
GeoBounds::IntersectWith(const GeoBounds &other) noexcept
{
  return longitude.IntersectWith(other.longitude) &&
    latitude.IntersectWith(other.latitude);
}

GeoPoint
GeoBounds::GetCenter() const noexcept
{
  if (!IsValid())
    return GeoPoint::Invalid();

  return GeoPoint(longitude.GetMiddle().AsDelta(), latitude.GetMiddle());
}

GeoBounds
GeoBounds::Scale(double factor) const noexcept
{
  if (!IsValid())
    return Invalid();

  Angle diff_lat_half = GetHeight() / 2 * (factor - 1);
  Angle diff_lon_half = GetWidth() / 2 * (factor - 1);

  GeoBounds br = *this;
  br.longitude.end += diff_lon_half;
  br.longitude.start -= diff_lon_half;
  br.latitude.end += diff_lat_half;
  br.latitude.start -= diff_lat_half;

  return br;
}
