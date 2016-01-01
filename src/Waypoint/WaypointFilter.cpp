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

#include "WaypointFilter.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Engine/Task/Shapes/FAITrianglePointValidator.hpp"

inline bool
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

  case TypeFilter::USER:
    return waypoint.origin == WaypointOrigin::USER;

  case TypeFilter::FILE_1:
    return waypoint.origin == WaypointOrigin::PRIMARY;

  case TypeFilter::FILE_2:
    return waypoint.origin == WaypointOrigin::ADDITIONAL;

  case TypeFilter::MAP:
    return waypoint.origin == WaypointOrigin::MAP;

  case TypeFilter::LAST_USED:
    return false;
  }

  gcc_unreachable();
  return false;
}

inline bool
WaypointFilter::CompareType(const Waypoint &waypoint,
                            const FAITrianglePointValidator &triangle_validator) const
{
  return CompareType(waypoint, type_index, triangle_validator);
}

inline bool
WaypointFilter::CompareDirection(const Waypoint &waypoint, Angle angle,
                                     GeoPoint location)
{
  if (angle.IsNegative())
    return true;

  auto bearing = location.Bearing(waypoint.location);
  auto direction_error = (bearing - angle).AsDelta().AbsoluteDegrees();

  return direction_error < 18;
}

inline bool
WaypointFilter::CompareDirection(const Waypoint &waypoint,
                                 GeoPoint location) const
{
  return CompareDirection(waypoint, direction, location);
}

inline bool
WaypointFilter::CompareName(const Waypoint &waypoint, const TCHAR *name)
{
  return StringIsEqualIgnoreCase(waypoint.name.c_str(), name, _tcslen(name));
}

inline bool
WaypointFilter::CompareName(const Waypoint &waypoint) const
{
  return CompareName(waypoint, name);
}

bool
WaypointFilter::Matches(const Waypoint &waypoint, GeoPoint location,
                        const FAITrianglePointValidator &triangle_validator) const
{
  return CompareType(waypoint, triangle_validator) &&
         (distance <= 0 || CompareName(waypoint)) &&
         CompareDirection(waypoint, location);
}
