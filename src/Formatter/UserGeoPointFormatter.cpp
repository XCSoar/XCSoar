// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UserGeoPointFormatter.hpp"
#include "GeoPointFormatter.hpp"
#include "Math/Angle.hpp"
#include "Geo/GeoPoint.hpp"

/**
 * This is a copy of FormatSettings::coordinate_format.  We need it
 * because this library is not allowed to access a blackboard, because
 * it may be run from either the main thread or the DrawThread.
 */
static CoordinateFormat user_coordinate_format = CoordinateFormat::DDMMSS;

void
SetUserCoordinateFormat(CoordinateFormat _fmt)
{
  user_coordinate_format = _fmt;
}

bool
FormatLongitude(Angle longitude, char *buffer, size_t size)
{
  return FormatLongitude(longitude, buffer, size, user_coordinate_format);
}

bool
FormatLatitude(Angle latitude, char *buffer, size_t size)
{
  return FormatLatitude(latitude, buffer, size, user_coordinate_format);
}

char *
FormatGeoPoint(const GeoPoint &location, char *buffer, size_t size,
               char separator)
{
  return FormatGeoPoint(location, buffer, size, user_coordinate_format,
                        separator);
}
