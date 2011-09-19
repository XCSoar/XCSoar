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

#ifndef XCSOAR_MAP_ITEM_HPP
#define XCSOAR_MAP_ITEM_HPP

#include "Util/StaticArray.hpp"
#include "Engine/Navigation/GeoPoint.hpp"
#include "Engine/Task/Tasks/BaseTask/ObservationZonePoint.hpp"
#include "Engine/Task/Tasks/BaseTask/TaskPoint.hpp"

class AbstractAirspace;
struct Waypoint;

struct MapItem
{
  enum Type {
    SELF,
    TASK_OZ,
    AIRSPACE,
    WAYPOINT,
  } type;

protected:
  MapItem(Type _type):type(_type) {}
};

struct SelfMapItem: public MapItem
{
  GeoPoint location;
  Angle bearing;

  SelfMapItem(const GeoPoint &_location, const Angle &_bearing)
    :MapItem(SELF), location(_location), bearing(_bearing) {}
};

struct TaskOZMapItem: public MapItem
{
  int index;
  const ObservationZonePoint *oz;
  TaskPoint::Type tp_type;
  const Waypoint &waypoint;

  TaskOZMapItem(int _index, const ObservationZonePoint &_oz,
                TaskPoint::Type _tp_type, const Waypoint &_waypoint)
    :MapItem(TASK_OZ), index(_index), oz(_oz.clone()),
     tp_type(_tp_type), waypoint(_waypoint) {}

  ~TaskOZMapItem() {
    delete oz;
  }
};

struct AirspaceMapItem: public MapItem
{
  const AbstractAirspace *airspace;

  AirspaceMapItem(const AbstractAirspace *_airspace)
    :MapItem(AIRSPACE), airspace(_airspace) {}
};

struct WaypointMapItem: public MapItem
{
  const Waypoint &waypoint;

  WaypointMapItem(const Waypoint &_waypoint)
    :MapItem(WAYPOINT), waypoint(_waypoint) {}
};

#endif
