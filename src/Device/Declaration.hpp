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

#ifndef XCSOAR_DEVICE_DECLARATION_HPP
#define XCSOAR_DEVICE_DECLARATION_HPP

#include "Engine/Waypoint/Waypoint.hpp"
#include "Util/StaticString.hxx"
#include "Compiler.h"

#include <vector>
#include <tchar.h>

struct LoggerSettings;
struct Plane;
class OrderedTask;
class OrderedTaskPoint;

struct Declaration {
  struct TurnPoint {
    enum Shape {
      CYLINDER,
      SECTOR,
      LINE,
      DAEC_KEYHOLE
    };

    Waypoint waypoint;
    Shape shape;
    unsigned radius;

    TurnPoint(const Waypoint &_waypoint)
      :waypoint(_waypoint), shape(CYLINDER), radius(1500) {}
    TurnPoint(const OrderedTaskPoint &tp);
  };

  StaticString<64> pilot_name;
  StaticString<32> aircraft_type;
  StaticString<32> aircraft_registration;
  StaticString<8> competition_id;
  std::vector<TurnPoint> turnpoints;

  Declaration(const LoggerSettings &logger_settings, const Plane &plane,
              const OrderedTask* task);

  void Append(const Waypoint &waypoint) {
    turnpoints.push_back(waypoint);
  }

  const Waypoint &GetWaypoint(unsigned i) const {
    return turnpoints[i].waypoint;
  }

  const Waypoint &GetFirstWaypoint() const {
    return turnpoints.front().waypoint;
  }

  const Waypoint &GetLastWaypoint() const {
    return turnpoints.back().waypoint;
  }

  gcc_pure
  const TCHAR *GetName(const unsigned i) const {
    return turnpoints[i].waypoint.name.c_str();
  }

  const GeoPoint &GetLocation(const unsigned i) const {
    return turnpoints[i].waypoint.location;
  }

  gcc_pure
  unsigned Size() const {
    return turnpoints.size();
  }
};

#endif
