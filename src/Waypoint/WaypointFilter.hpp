// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "util/StaticString.hxx"
#include "Math/Angle.hpp"

#include <cstdint>
#include <tchar.h>

struct Waypoint;
struct GeoPoint;
class FAITrianglePointValidator;

enum class TypeFilter: uint8_t {
  ALL = 0,
  AIRPORT,
  LANDABLE,
  TURNPOINT,
  START,
  FINISH,
  FAI_TRIANGLE_LEFT,
  FAI_TRIANGLE_RIGHT,
  USER,
  FILE_1,
  FILE_2,
  MAP,
  LAST_USED,
};

struct WaypointFilter
{
  static constexpr size_t NAME_LENGTH = 10;

  StaticString<NAME_LENGTH + 1> name;

  double distance;
  Angle direction;
  TypeFilter type_index;

  void Clear() {
    name.clear();
    distance = 0;
    direction = Angle::Native(-1);
    type_index = TypeFilter::ALL;
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

  static bool CompareName(const Waypoint &waypoint, const TCHAR *name);

  bool CompareName(const Waypoint &waypoint) const;
};
