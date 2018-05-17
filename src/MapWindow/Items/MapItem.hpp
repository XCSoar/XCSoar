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

#ifndef XCSOAR_MAP_ITEM_HPP
#define XCSOAR_MAP_ITEM_HPP

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "FLARM/FlarmId.hpp"
#include "FLARM/Color.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "Weather/Features.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Tracking/SkyLines/Features.hpp"
#include "Util/StaticString.hxx"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

#include <tchar.h>

enum class TaskPointType : uint8_t;

class AbstractAirspace;
class ObservationZonePoint;

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
    THERMAL,
    WAYPOINT,
    TRAFFIC,
#ifdef HAVE_SKYLINES_TRACKING
    SKYLINES_TRAFFIC,
#endif
    OVERLAY,
    RASP,
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
  /**
   * Magic value for "unknown elevation".
   */
  static constexpr double UNKNOWN_ELEVATION = -1e5;

  /**
   * All elevation values below this threshold are considered unknown.
   */
  static constexpr double UNKNOWN_ELEVATION_THRESHOLD = -1e4;

  GeoVector vector;

  /**
   * Terrain elevation of the point.  If that is unknown, it is nan().
   */
  double elevation;

  LocationMapItem(const GeoVector &_vector, double _elevation)
    :MapItem(LOCATION), vector(_vector), elevation(_elevation) {}

  bool HasElevation() const {
    return elevation > UNKNOWN_ELEVATION_THRESHOLD;
  }
};

/**
 * An indirect MapItem that shows at what altitude the clicked location can
 * be reached in straight glide and around terrain obstacles.
 */
struct ArrivalAltitudeMapItem: public MapItem
{
  /**
   * Magic value for "unknown elevation".
   */
  static constexpr double UNKNOWN_ELEVATION = -1e5;

  /**
   * All elevation values below this threshold are considered unknown.
   */
  static constexpr double UNKNOWN_ELEVATION_THRESHOLD = -1e4;

  /**
   * Terrain elevation of the point in MSL.  If that is unknown, it is
   * nan().
   */
  double elevation;

  /** Arrival altitudes [m MSL] */
  ReachResult reach;

  /** Safety height (m) */
  double safety_height;


  ArrivalAltitudeMapItem(double _elevation,
                         ReachResult _reach,
                         double _safety_height)
    :MapItem(ARRIVAL_ALTITUDE),
     elevation(_elevation), reach(_reach), safety_height(_safety_height) {}

  bool HasElevation() const {
    return elevation > UNKNOWN_ELEVATION_THRESHOLD;
  }
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
  WaypointPtr waypoint;

  TaskOZMapItem(int _index, const ObservationZonePoint &_oz,
                TaskPointType _tp_type, WaypointPtr &&_waypoint);
  virtual ~TaskOZMapItem();
};

struct AirspaceMapItem: public MapItem
{
  const AbstractAirspace *airspace;

  AirspaceMapItem(const AbstractAirspace &_airspace)
    :MapItem(AIRSPACE), airspace(&_airspace) {}
};

struct WaypointMapItem: public MapItem
{
  WaypointPtr waypoint;

  WaypointMapItem(const WaypointPtr &_waypoint)
    :MapItem(WAYPOINT), waypoint(_waypoint) {}
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
  FlarmColor color;

  TrafficMapItem(FlarmId _id, FlarmColor _color)
    :MapItem(TRAFFIC), id(_id), color(_color) {}
};

#ifdef HAVE_SKYLINES_TRACKING

struct SkyLinesTrafficMapItem : public MapItem
{
  uint32_t id, time_of_day_ms;

  int altitude;

  StaticString<40> name;

  SkyLinesTrafficMapItem(uint32_t _id, uint32_t _time_of_day_ms,
                         int _altitude,
                         const TCHAR *_name)
    :MapItem(SKYLINES_TRAFFIC), id(_id), time_of_day_ms(_time_of_day_ms),
     altitude(_altitude),
     name(_name) {}
};

#endif

struct ThermalMapItem: public MapItem
{
  ThermalSource thermal;

  ThermalMapItem(const ThermalSource &_thermal)
    :MapItem(THERMAL), thermal(_thermal) {}
};

#endif
