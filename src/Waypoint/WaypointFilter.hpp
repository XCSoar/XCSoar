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

#ifndef XCSOAR_WAYPOINT_LIST_FILTER_HPP
#define XCSOAR_WAYPOINT_LIST_FILTER_HPP

#include "Util/StaticString.hxx"
#include "Math/Angle.hpp"

#include <stdint.h>
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

  gcc_pure
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

#endif
