/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

  This program is free software; you can redistribute it and/or
  modify it under the terms of the GNU General Public License
  as published by the Free Software Foundation; either version 2
  of the License, or (at your option) any later version.

  This program is distributed in the hope that it will be useful,
  but WITHOUT ANY WARRANTY; without even the implied warranty of
  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
  GNU General Public License for more details.

  You should have received a copy of the GNU General Public License
  along with this program; if not, write to the Free Software
  Foundation, Inc., 59 Temple Place - Suite 330, Boston, MA  02111-1307, USA.
}
*/

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
FormatLongitude(Angle longitude, TCHAR *buffer, size_t size)
{
  return FormatLongitude(longitude, buffer, size, user_coordinate_format);
}

bool
FormatLatitude(Angle latitude, TCHAR *buffer, size_t size)
{
  return FormatLatitude(latitude, buffer, size, user_coordinate_format);
}

TCHAR *
FormatGeoPoint(const GeoPoint &location, TCHAR *buffer, size_t size,
               TCHAR separator)
{
  return FormatGeoPoint(location, buffer, size, user_coordinate_format,
                        separator);
}
