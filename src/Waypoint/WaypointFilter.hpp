// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "Math/Angle.hpp"

#include <cstdint>
struct Waypoint;
struct GeoPoint;
class FAITrianglePointValidator;

enum class TypeFilter : uint8_t {
  ALL = 0,
  AIRPORT,
  LANDABLE,
  TURNPOINT,
  START,
  FINISH,
  FAI_TRIANGLE_LEFT,
  FAI_TRIANGLE_RIGHT,
  USER,
  FILE,
  MAP,
  LAST_USED,

  /* One filter per Waypoint::Type value not already covered by
     the categorical filters above (AIRFIELD via AIRPORT,
     OUTLANDING via LANDABLE, NORMAL effectively via TURNPOINT). */
  MOUNTAIN_TOP,
  MOUNTAIN_PASS,
  BRIDGE,
  TUNNEL,
  TOWER,
  POWERPLANT,
  OBSTACLE,
  THERMAL_HOTSPOT,
  MARKER,
  VOR,
  NDB,
  DAM,
  CASTLE,
  INTERSECTION,
  REPORTING_POINT,
  PG_TAKEOFF,
  PG_LANDING,

  /** Sentinel: number of real TypeFilter values (excluding the
      dynamic file ID range below). */
  COUNT,

  /**
   * Reserved range for dynamic entries in UI:
   * IDs 100+ are used for individual waypoint files in the filter dropdown.
   * DO NOT add enum values >= 100 to avoid conflicts!
   */
  _DYNAMIC_FILE_ID_START = 100,
};

static_assert((unsigned)TypeFilter::COUNT < (unsigned)TypeFilter::_DYNAMIC_FILE_ID_START,
              "TypeFilter enum values must be < 100 to avoid collision with dynamic file IDs");

struct WaypointFilter
{
  static constexpr size_t NAME_LENGTH = 10;

  StaticString<NAME_LENGTH + 1> name;

  double distance;
  Angle direction;
  TypeFilter type_index;

  /**
   * When type_index is FILE, this specifies which file to filter by.
   * -1 = all PRIMARY files, 0+ = specific file index
   */
  int file_num = -1;

  void Clear() {
    name.clear();
    distance = 0;
    direction = Angle::Native(-1);
    type_index = TypeFilter::ALL;
    file_num = -1;
  }

  [[gnu::pure]]
  bool Matches(const Waypoint &waypoint, GeoPoint location,
               const FAITrianglePointValidator &triangle_validator) const;

private:
  static bool CompareType(const Waypoint &waypoint, TypeFilter type,
                          const FAITrianglePointValidator &triangle_validator);

  bool CompareType(const Waypoint &waypoint,
                   const FAITrianglePointValidator &triangle_validator) const;

  static bool CompareDirection(const Waypoint &waypoint, Angle angle,
                               GeoPoint location);

  bool CompareDirection(const Waypoint &waypoint, GeoPoint location) const;

  static bool CompareName(const Waypoint &waypoint, const char *name);

  bool CompareName(const Waypoint &waypoint) const;
};
