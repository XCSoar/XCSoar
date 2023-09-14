// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Geo/GeoPoint.hpp"
#include "Geo/GeoVector.hpp"
#include "FLARM/Id.hpp"
#include "FLARM/Color.hpp"
#include "NMEA/ThermalLocator.hpp"
#include "Weather/Features.hpp"
#include "Engine/Waypoint/Ptr.hpp"
#include "Engine/Airspace/Ptr.hpp"
#include "Engine/Route/ReachResult.hpp"
#include "Tracking/SkyLines/Features.hpp"
#include "util/StaticString.hxx"

#ifdef HAVE_NOAA
#include "Weather/NOAAStore.hpp"
#endif

#include <chrono>

#include <tchar.h>

enum class TaskPointType : uint8_t;

class ObservationZonePoint;

struct MapItem
{
  enum class Type {
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
  virtual ~MapItem() noexcept = default;
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
    :MapItem(Type::LOCATION), vector(_vector), elevation(_elevation) {}

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
    :MapItem(Type::ARRIVAL_ALTITUDE),
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
    :MapItem(Type::SELF), location(_location), bearing(_bearing) {}
};

struct TaskOZMapItem: public MapItem
{
  int index;
  std::unique_ptr<ObservationZonePoint> oz;
  TaskPointType tp_type;
  WaypointPtr waypoint;

  TaskOZMapItem(int _index, const ObservationZonePoint &_oz,
                TaskPointType _tp_type, WaypointPtr &&_waypoint);
  ~TaskOZMapItem() noexcept override;
};

struct AirspaceMapItem: public MapItem
{
  ConstAirspacePtr airspace;

  template<typename T>
  explicit AirspaceMapItem(T &&_airspace) noexcept
    :MapItem(Type::AIRSPACE), airspace(std::forward<T>(_airspace)) {}
};

struct WaypointMapItem: public MapItem
{
  WaypointPtr waypoint;

  WaypointMapItem(const WaypointPtr &_waypoint)
    :MapItem(Type::WAYPOINT), waypoint(_waypoint) {}
};

#ifdef HAVE_NOAA
struct WeatherStationMapItem: public MapItem
{
  NOAAStore::iterator station;

  WeatherStationMapItem(const NOAAStore::iterator &_station)
    :MapItem(Type::WEATHER), station(_station) {}
};
#endif

struct TrafficMapItem: public MapItem
{
  FlarmId id;
  FlarmColor color;

  TrafficMapItem(FlarmId _id, FlarmColor _color)
    :MapItem(Type::TRAFFIC), id(_id), color(_color) {}
};

#ifdef HAVE_SKYLINES_TRACKING

struct SkyLinesTrafficMapItem : public MapItem
{
  using Time = std::chrono::duration<uint_least32_t, std::chrono::milliseconds::period>;

  uint32_t id;

  Time time_of_day;

  int altitude;

  StaticString<40> name;

  SkyLinesTrafficMapItem(uint32_t _id, Time _time_of_day_ms,
                         int _altitude,
                         const TCHAR *_name)
    :MapItem(Type::SKYLINES_TRAFFIC), id(_id), time_of_day(_time_of_day_ms),
     altitude(_altitude),
     name(_name) {}
};

#endif

struct ThermalMapItem: public MapItem
{
  ThermalSource thermal;

  ThermalMapItem(const ThermalSource &_thermal)
    :MapItem(Type::THERMAL), thermal(_thermal) {}
};
