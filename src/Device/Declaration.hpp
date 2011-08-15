/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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

#include "Navigation/GeoPoint.hpp"
#include "Engine/Waypoint/Waypoint.hpp"
#include "Util/StaticString.hpp"
#include "Compiler.h"

#include <vector>
#include <tchar.h>

class OrderedTask;
class OrderedTaskPoint;

struct Declaration {
  struct TurnPoint {
    enum shape {
      CYLINDER,
      SECTOR,
      LINE,
    };

    Waypoint waypoint;

    enum shape shape;

    unsigned radius;

    TurnPoint(const Waypoint &_waypoint)
      :waypoint(_waypoint), shape(CYLINDER), radius(1500) {}
    TurnPoint(const OrderedTaskPoint &tp);
  };

  StaticString<64> PilotName;
  StaticString<32> AircraftType;
  StaticString<32> AircraftReg;
  StaticString<8> CompetitionId;
  std::vector<TurnPoint> TurnPoints;

  Declaration(const OrderedTask* task);

  void append(const Waypoint &waypoint) {
    TurnPoints.push_back(waypoint);
  }

  const Waypoint &get_waypoint(unsigned i) const {
    return TurnPoints[i].waypoint;
  }

  const Waypoint &get_first_waypoint() const {
    return TurnPoints.front().waypoint;
  }

  const Waypoint &get_last_waypoint() const {
    return TurnPoints.back().waypoint;
  }

  gcc_pure
  const TCHAR *get_name(const unsigned i) const {
    return TurnPoints[i].waypoint.name.c_str();
  }

  const GeoPoint &get_location(const unsigned i) const {
    return TurnPoints[i].waypoint.location;
  }

  gcc_pure
  unsigned size() const {
    return TurnPoints.size();
  }
};

#endif
