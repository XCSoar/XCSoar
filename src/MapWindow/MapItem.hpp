/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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
#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "Task/ObservationZones/ObservationZonePoint.hpp"
#include "Markers/Marker.hpp"
#include "FLARM/Traffic.hpp"
#include "FLARM/Friends.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "Weather/Features.hpp"
#include "Engine/Route/ReachResult.hpp"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

enum class TaskPointType : uint8_t;

class AbstractAirspace;
struct Waypoint;

struct MapItem
{
  enum Type {
    LOCATION,
    ARRIVAL_ALTITUDE,
    SELF,
    TASK_OZ,
#ifdef HAVE_NOAA
    WEATHER,
#endif
    AIRSPACE,
    MARKER,
    THERMAL,
    WAYPOINT,
    TRAFFIC,
  } type;

protected:
  MapItem(Type _type):type(_type) {}

public:
  /* we need this virtual dummy destructor, because there is code that
     "deletes" MapItem objects without knowing that it's really a
     TaskOZMapItem */
  virtual ~MapItem() {}
};

struct LocationMapItem: public MapItem
{
  GeoVector vector;
  short elevation;

  LocationMapItem(const GeoVector &_vector, short _elevation)
    :MapItem(LOCATION), vector(_vector), elevation(_elevation) {}
};

/**
 * An indirect MapItem that shows at what altitude the clicked location can
 * be reached in straight glide and around terrain obstacles.
 */
struct ArrivalAltitudeMapItem: public MapItem
{
  /** Elevation of the point in MSL */
  RoughAltitude elevation;

  /** Arrival altitudes [m MSL] */
  ReachResult reach;

  ArrivalAltitudeMapItem(RoughAltitude _elevation,
                         ReachResult _reach)
    :MapItem(ARRIVAL_ALTITUDE),
     elevation(_elevation), reach(_reach) {}
};

struct SelfMapItem: public MapItem
{
  GeoPoint location;
  Angle bearing;

  SelfMapItem(const GeoPoint &_location, const Angle _bearing)
    :MapItem(SELF), location(_location), bearing(_bearing) {}
};

struct TaskOZMapItem: public MapItem
{
  int index;
  const ObservationZonePoint *oz;
  TaskPointType tp_type;
  const Waypoint &waypoint;

  TaskOZMapItem(int _index, const ObservationZonePoint &_oz,
                TaskPointType _tp_type, const Waypoint &_waypoint)
    :MapItem(TASK_OZ), index(_index), oz(_oz.Clone()),
     tp_type(_tp_type), waypoint(_waypoint) {}

  ~TaskOZMapItem() {
    delete oz;
  }
};

struct AirspaceMapItem: public MapItem
{
  const AbstractAirspace *airspace;

  AirspaceMapItem(const AbstractAirspace &_airspace)
    :MapItem(AIRSPACE), airspace(&_airspace) {}
};

struct WaypointMapItem: public MapItem
{
  const Waypoint &waypoint;

  WaypointMapItem(const Waypoint &_waypoint)
    :MapItem(WAYPOINT), waypoint(_waypoint) {}
};

struct MarkerMapItem: public MapItem
{
  unsigned id;
  Marker marker;

  MarkerMapItem(unsigned _id, const Marker &_marker)
    :MapItem(MARKER), id(_id), marker(_marker) {}
};

#ifdef HAVE_NOAA
struct WeatherStationMapItem: public MapItem
{
  NOAAStore::iterator station;

  WeatherStationMapItem(const NOAAStore::iterator &_station)
    :MapItem(WEATHER), station(_station) {}
};
#endif

struct TrafficMapItem: public MapItem
{
  FlarmId id;
  FlarmFriends::Color color;

  TrafficMapItem(const FlarmTraffic &_traffic, FlarmFriends::Color _color)
    :MapItem(TRAFFIC), id(_traffic.id), color(_color) {}
};

struct ThermalMapItem: public MapItem
{
  ThermalSource thermal;

  ThermalMapItem(const ThermalSource &_thermal)
    :MapItem(THERMAL), thermal(_thermal) {}
};

#endif
