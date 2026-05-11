// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WaypointFilter.hpp"
#include "Waypoint/Waypoint.hpp"
#include "Engine/Task/Shapes/FAITrianglePointValidator.hpp"
#include "Engine/Waypoint/NameSearch.hpp"
#include "util/Compiler.h"
#include "util/StringUtil.hpp"

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

  case TypeFilter::MOUNTAIN_TOP:
    return waypoint.type == Waypoint::Type::MOUNTAIN_TOP;
  case TypeFilter::MOUNTAIN_PASS:
    return waypoint.type == Waypoint::Type::MOUNTAIN_PASS;
  case TypeFilter::BRIDGE:
    return waypoint.type == Waypoint::Type::BRIDGE;
  case TypeFilter::TUNNEL:
    return waypoint.type == Waypoint::Type::TUNNEL;
  case TypeFilter::TOWER:
    return waypoint.type == Waypoint::Type::TOWER;
  case TypeFilter::POWERPLANT:
    return waypoint.type == Waypoint::Type::POWERPLANT;
  case TypeFilter::OBSTACLE:
    return waypoint.type == Waypoint::Type::OBSTACLE;
  case TypeFilter::THERMAL_HOTSPOT:
    return waypoint.type == Waypoint::Type::THERMAL_HOTSPOT;
  case TypeFilter::MARKER:
    return waypoint.type == Waypoint::Type::MARKER;
  case TypeFilter::VOR:
    return waypoint.type == Waypoint::Type::VOR;
  case TypeFilter::NDB:
    return waypoint.type == Waypoint::Type::NDB;
  case TypeFilter::DAM:
    return waypoint.type == Waypoint::Type::DAM;
  case TypeFilter::CASTLE:
    return waypoint.type == Waypoint::Type::CASTLE;
  case TypeFilter::INTERSECTION:
    return waypoint.type == Waypoint::Type::INTERSECTION;
  case TypeFilter::REPORTING_POINT:
    return waypoint.type == Waypoint::Type::REPORTING_POINT;
  case TypeFilter::PG_TAKEOFF:
    return waypoint.type == Waypoint::Type::PGTAKEOFF;
  case TypeFilter::PG_LANDING:
    return waypoint.type == Waypoint::Type::PGLANDING;

  case TypeFilter::COUNT:
  case TypeFilter::_DYNAMIC_FILE_ID_START:
    // Sentinel values, not actual filter types
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
  /* Substring match against the normalised waypoint name (and
     shortname).  Goes through the shared helper so this filter
     path (within-range + name) agrees with the name-only path
     (Waypoints::VisitNameSubstring). */
  char needle[NAME_SEARCH_BUFFER_SIZE];
  NormalizeSearchString(needle, name);
  return WaypointMatchesNormalisedSubstring(waypoint, needle);
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
