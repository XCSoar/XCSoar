/* Copyright_License {

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

#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "Origin.hpp"
#include "Util/tstring.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatGeoPoint.hpp"
#include "RadioFrequency.hpp"
#include "Runway.hpp"
#include "OS/RunFile.hpp"

#include <forward_list>

class FlatProjection;

/**
 * Class for waypoints.  
 * This is small enough currently to be used with local copies (e.g. in a TaskWaypoint),
 * but this may change if we include airfield details inside.
 *
 * @todo
 * - consider having a static factory method provide the ID automatically
 *   so we know they will be unique.
 */
struct Waypoint {
  enum class Type: uint8_t {
    NORMAL,
    AIRFIELD,
    OUTLANDING,
    MOUNTAIN_TOP,
    MOUNTAIN_PASS,
    BRIDGE,
    TUNNEL,
    TOWER,
    POWERPLANT,
    OBSTACLE,
    THERMAL_HOTSPOT,
    MARKER,
  };

  /**
   * Bitfield structure for Waypoint capabilities
   * Several of these capabilities are not used by XCSoar, but are
   * present for compatibility
   */
  struct Flags {
    /** If waypoint can be used as a turnpoint */
    bool turn_point:1;
    /** If waypoint is to be used as home */
    bool home:1;
    /** If waypoint is marked as a potential start point */
    bool start_point:1;
    /** If waypoint is marked as a potential finish point */
    bool finish_point:1;
    /** If waypoint is watched, i.e. displayed with arrival height in map */
    bool watched:1;

    Flags() = default;

    static constexpr Flags Defaults() {
      return { false, false, false, false, false };
    }
  };

  /** Unique id */
  unsigned id;

  /**
   * The id number as specified in the input file.
   */
  unsigned original_id;

  /** Geodetic location */
  GeoPoint location;

  /** Flat projected location */
  FlatGeoPoint flat_location;

#ifndef NDEBUG
  bool flat_location_initialised;
#endif

  /** Height AMSL (m) of waypoint terrain */
  double elevation;

  /** Main runway */
  Runway runway;

  RadioFrequency radio_frequency;

  /** Type of the waypoint */
  Type type;
  /** Flag types of this waypoint */
  Flags flags;

  /** File number to store waypoint in */
  WaypointOrigin origin;

  /** Name of waypoint */
  tstring name;
  /** Additional comment text for waypoint */
  tstring comment;
  /** Airfield or additional (long) details */
  tstring details;
  /** Additional files to be displayed in the WayointDetails dialog */
  std::forward_list<tstring> files_embed;
#ifdef HAVE_RUN_FILE
  /** Additional files to be opened by external programs */
  std::forward_list<tstring> files_external;
#endif

  /**
   * Constructor for real waypoints
   *
   * @return Uninitialised object
   */
  Waypoint()
    :
#ifndef NDEBUG
     flat_location_initialised(false),
#endif
     runway(Runway::Null()), radio_frequency(RadioFrequency::Null()),
     type(Type::NORMAL), flags(Flags::Defaults()), origin(WaypointOrigin::NONE)
  {
  }

  /**
   * Constructor for real waypoints
   *
   * @return Uninitialised object
   */
  Waypoint(const GeoPoint &_location);

  /** 
   * Determine if waypoint is marked as able to be landed at
   * 
   * @return True if waypoint is landable
   */
  bool
  IsLandable() const
  {
    return (type == Type::AIRFIELD || type == Type::OUTLANDING);
  }

  /**
   * Determine if waypoint is marked as an airport
   *
   * @return True if waypoint is landable
   */
  bool
  IsAirport() const
  {
    return type == Type::AIRFIELD;
  }

  /**
   * Determine if waypoint is marked as a turnpoint
   *
   * @return True if waypoint is landable
   */
  bool
  IsTurnpoint() const
  {
    return flags.turn_point;
  }

  /**
   * Determine if waypoint is marked as a start
   *
   * @return True if waypoint is start
   */
  bool
  IsStartpoint() const
  {
    return flags.start_point;
  }

  /**
   * Determine if waypoint is marked as a finish
   *
   * @return True if waypoint is finish
   */
  bool
  IsFinishpoint() const
  {
    return flags.finish_point;
  }

  /**
   * Equality operator (by id)
   * 
   * @param wp Waypoint object to match against
   *
   * @return true if ids match
   */
  bool
  operator==(const Waypoint&wp) const
  {
    return id == wp.id;
  }

  /**
   * Project geolocation to flat location
   *
   * @param projection the projection to apply
   */
  void Project(const FlatProjection &projection);

  /**
   * Get distance in internal flat projected units (fast)
   *
   * @param f Point to get distance to
   *
   * @return Distance in flat units
   */
  unsigned FlatDistanceTo(const FlatGeoPoint &f) const {
    assert(flat_location_initialised);

    return flat_location.Distance(f);
  }

  /**
   * Determine if a waypoint is close to a given location within
   * a threshold
   *
   * @param location Location to compare to
   * @param range Distance threshold (m)
   *
   * @return True if close to reference location
   */
  bool
  IsCloseTo(const GeoPoint &_location, double range) const;
};

#endif
