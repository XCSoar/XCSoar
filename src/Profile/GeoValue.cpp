// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Map.hpp"
#include "Geo/GeoPoint.hpp"
#include "util/NumberParser.hpp"
#include "util/StaticString.hxx"

bool
ProfileMap::GetGeoPoint(std::string_view key, GeoPoint &value) const noexcept
{
  const char *p = Get(key);
  if (p == nullptr)
    return false;

  char *endptr;
  double longitude = ParseDouble(p, &endptr);
  if (endptr == p || *endptr != ' ' ||
      longitude < -180.0 || longitude > 180.0)
    return false;

  p = endptr + 1;
  double latitude = ParseDouble(p, &endptr);
  if (endptr == p || *endptr != '\0' ||
      latitude < -90.0 || latitude > 90.0)
    return false;

  value.longitude = Angle::Degrees(longitude);
  value.latitude = Angle::Degrees(latitude);
  return true;
}

void
ProfileMap::SetGeoPoint(std::string_view key, const GeoPoint &value) noexcept
{
  NarrowString<128> buffer;
  buffer.UnsafeFormat("%f %f",
                      (double)value.longitude.Degrees(),
                      (double)value.latitude.Degrees());
  Set(key, buffer);
}
