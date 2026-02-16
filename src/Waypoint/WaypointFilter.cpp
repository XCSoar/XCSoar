// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointFilter.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Engine/Task/Shapes/FAITrianglePointValidator.hpp"
#include "util/Compiler.h"

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

  case TypeFilter::FILE:
    // FILE filter is now handled in Matches() with file_num check
    return waypoint.origin == WaypointOrigin::PRIMARY;

  case TypeFilter::MAP:
    return waypoint.origin == WaypointOrigin::MAP;

  case TypeFilter::LAST_USED:
    return false;

  case TypeFilter::_DYNAMIC_FILE_ID_START:
    // This is a sentinel value, not an actual filter type
    gcc_unreachable();
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
  auto direction_error = (bearing - angle).AsDelta().Absolute();

  return direction_error < Angle::Degrees(18);
}

inline bool
WaypointFilter::CompareDirection(const Waypoint &waypoint,
                                 GeoPoint location) const
{
  return CompareDirection(waypoint, direction, location);
}

inline bool
WaypointFilter::CompareName(const Waypoint &waypoint, const char *name)
{
  return StringIsEqualIgnoreCase(waypoint.name.c_str(), name, strlen(name));
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
  // Check file_num filter for FILE type
  if (type_index == TypeFilter::FILE && file_num >= 0) {
    if (waypoint.origin != WaypointOrigin::PRIMARY ||
        waypoint.file_num != static_cast<uint8_t>(file_num))
      return false;
  }

  return CompareType(waypoint, triangle_validator) &&
         (distance <= 0 || CompareName(waypoint)) &&
         CompareDirection(waypoint, location);
}
