// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Origin.hpp"
#include "util/tstring.hpp"
#include "Geo/GeoPoint.hpp"
#include "Geo/Flat/FlatGeoPoint.hpp"
#include "RadioFrequency.hpp"
#include "Runway.hpp"
#include "system/RunFile.hpp"

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
    VOR,
    NDB,
    DAM,
    CASTLE,
    INTERSECTION,
    REPORTING_POINT,
    PGTAKEOFF,
    PGLANDING
  };

  /**
   * Bitfield structure for Waypoint capabilities
   * Several of these capabilities are not used by XCSoar, but are
   * present for compatibility
   */
  struct Flags {
    /** If waypoint can be used as a turnpoint */
    bool turn_point:1 = false;
    /** If waypoint is to be used as home */
    bool home:1 = false;
    /** If waypoint is marked as a potential start point */
    bool start_point:1 = false;
    /** If waypoint is marked as a potential finish point */
    bool finish_point:1 = false;
    /** If waypoint is watched, i.e. displayed with arrival height in map */
    bool watched:1 = false;
  };

  /** Geodetic location */
  GeoPoint location;

  /** Flat projected location */
  FlatGeoPoint flat_location;

  /**
   * Height AMSL (m) of waypoint terrain.
   *
   * This field is only usable if #has_elevation is true.
   */
  double elevation;

  /** Short name (code) label of waypoint */
  tstring shortname;

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

  /** Unique id */
  unsigned id;

  /**
   * The id number as specified in the input file.
   */
  unsigned original_id;

  /** Main runway */
  Runway runway = Runway::Null();

  RadioFrequency radio_frequency = RadioFrequency::Null();

  /** Flag types of this waypoint */
  Flags flags;

  /** Type of the waypoint */
  Type type = Type::NORMAL;

  /** File number to store waypoint in */
  WaypointOrigin origin = WaypointOrigin::NONE;

  /**
   * Does the #elevation field contain a value?
   */
  bool has_elevation = false;

#ifndef NDEBUG
  bool flat_location_initialised = false;
#endif

  /**
   * Constructor for real waypoints
   */
  explicit Waypoint(const GeoPoint &_location) noexcept;

  /** 
   * Determine if waypoint is marked as able to be landed at
   * 
   * @return True if waypoint is landable
   */
  constexpr bool IsLandable() const noexcept {
    return (type == Type::AIRFIELD || type == Type::OUTLANDING);
  }

  /**
   * Determine if waypoint is marked as an airport
   *
   * @return True if waypoint is landable
   */
  constexpr bool IsAirport() const noexcept {
    return type == Type::AIRFIELD;
  }

  /**
   * Determine if waypoint is marked as a turnpoint
   *
   * @return True if waypoint is landable
   */
  constexpr bool IsTurnpoint() const noexcept {
    return flags.turn_point;
  }

  /**
   * Determine if waypoint is marked as a start
   *
   * @return True if waypoint is start
   */
  constexpr bool IsStartpoint() const noexcept {
    return flags.start_point;
  }

  /**
   * Determine if waypoint is marked as a finish
   *
   * @return True if waypoint is finish
   */
  constexpr bool IsFinishpoint() const noexcept {
    return flags.finish_point;
  }

  constexpr double GetElevationOrZero() const noexcept {
    return has_elevation ? elevation : 0.;
  }

  /**
   * Equality operator (by id)
   * 
   * @param wp Waypoint object to match against
   *
   * @return true if ids match
   */
   constexpr bool operator==(const Waypoint &wp) const noexcept {
     return id == wp.id;
   }

  /**
   * Project geolocation to flat location
   *
   * @param projection the projection to apply
   */
  void Project(const FlatProjection &projection) noexcept;

  /**
   * Get distance in internal flat projected units (fast)
   *
   * @param f Point to get distance to
   *
   * @return Distance in flat units
   */
  [[gnu::pure]]
  unsigned FlatDistanceTo(const FlatGeoPoint &f) const noexcept {
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
  [[gnu::pure]]
  bool IsCloseTo(const GeoPoint &_location, double range) const noexcept;
};
