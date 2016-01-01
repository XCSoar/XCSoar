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

#include "Database.hpp"
#include "Geo/GeoPoint.hpp"

#include <stdlib.h>

GeoPoint
Volkslogger::Waypoint::GetLocation() const
{
  uint32_t ll = ((latitude[0] & 0x7f) << 16) |
    (latitude[1] << 8) | latitude[2];
  auto lat = ll / 60000.;
  if (latitude[0] & 0x80)
    lat = -lat;

  ll = (longitude[0] << 16) |
    (longitude[1] << 8) | longitude[2];
  auto lon = ll / 60000.;
  if (type_and_longitude_sign & 0x80)
    lon = -lon;

  return GeoPoint(Angle::Degrees(lon), Angle::Degrees(lat));
}

void
Volkslogger::Waypoint::SetLocation(GeoPoint gp)
{
  uint32_t llat = labs((long)(gp.latitude.Degrees() * 60000));
  uint32_t llon = labs((long)(gp.longitude.Degrees() * 60000));

  if (gp.longitude.IsNegative())
    type_and_longitude_sign |= 0x80;
  else
    type_and_longitude_sign &= ~0x80;

  latitude[0] = llat >> 16;
  if (gp.latitude.IsNegative())
    latitude[0] |= 0x80;
  latitude[1] = llat >> 8;
  latitude[2] = llat;

  longitude[0] = llon >> 16;
  longitude[1] = llon >> 8;
  longitude[2] = llon;
}
