/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "WaypointFilter.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Task/FAITrianglePointValidator.hpp"
#include "Compatibility/string.h"

bool
WaypointFilter::CompareType(const Waypoint &waypoint, TypeFilter type,
                            const FAITrianglePointValidator &triangle_validator)
{
  switch (type) {
  case TypeFilter::ALL:
    return true;

  case TypeFilter::AIRPORT:
    return waypoint.IsAirport();

  case TypeFilter::LANDABLE:
    return waypoint.IsLandable();

  case TypeFilter::TURNPOINT:
    return waypoint.IsTurnpoint();

  case TypeFilter::START:
    return waypoint.IsStartpoint();

  case TypeFilter::FINISH:
    return waypoint.IsFinishpoint();

  case TypeFilter::FAI_TRIANGLE_LEFT:
    return triangle_validator.IsFAITrianglePoint(waypoint, false);

  case TypeFilter::FAI_TRIANGLE_RIGHT:
    return triangle_validator.IsFAITrianglePoint(waypoint, true);

  case TypeFilter::FILE_1:
    return waypoint.file_num == 1;

  case TypeFilter::FILE_2:
    return waypoint.file_num == 2;

  case TypeFilter::LAST_USED:
    return false;
  }

  /* not reachable */
  return false;
}

bool
WaypointFilter::CompareType(const Waypoint &waypoint,
                            const FAITrianglePointValidator &triangle_validator) const
{
  return CompareType(waypoint, type_index, triangle_validator);
}

bool
WaypointFilter::CompareDirection(const Waypoint &waypoint, Angle angle,
                                     GeoPoint location)
{
  if (negative(angle.Native()))
    return true;

  auto bearing = location.Bearing(waypoint.location);
  fixed direction_error = (bearing - angle).AsDelta().AbsoluteDegrees();

  return direction_error < fixed(18);
}

bool
WaypointFilter::CompareDirection(const Waypoint &waypoint,
                                 GeoPoint location) const
{
  return CompareDirection(waypoint, direction, location);
}

bool
WaypointFilter::CompareName(const Waypoint &waypoint, const TCHAR *name)
{
  return StringIsEqualIgnoreCase(waypoint.name.c_str(), name, _tcslen(name));
}

bool
WaypointFilter::CompareName(const Waypoint &waypoint) const
{
  return CompareName(waypoint, name);
}

bool
WaypointFilter::Matches(const Waypoint &waypoint, GeoPoint location,
                        const FAITrianglePointValidator &triangle_validator) const
{
  return CompareType(waypoint, triangle_validator) &&
         (!positive(distance) || CompareName(waypoint)) &&
         CompareDirection(waypoint, location);
}
