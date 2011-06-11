/* Copyright_License {

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

#ifndef WAYPOINT_HPP
#define WAYPOINT_HPP

#include "Util/tstring.hpp"
#include "Navigation/GeoPoint.hpp"
#include "RadioFrequency.hpp"
#include "Runway.hpp"

#ifdef DO_PRINT
#include <iostream>
#endif

class TaskProjection;

/**
 * Class for waypoints.  
 * This is small enough currently to be used with local copies (e.g. in a TaskWayPoint),
 * but this may change if we include airfield details inside.
 *
 * @todo
 * - consider having a static factory method provide the ID automatically
 *   so we know they will be unique.
 */
class Waypoint {
public:
  friend class Serialiser;

  enum Type {
    wtNormal,
    wtAirfield,
    wtOutlanding,
    wtMountainTop,
    wtBridge,
    wtTunnel,
    wtTower,
    wtPowerPlant,
  };

  /**
   * Bitfield structure for Waypoint capabilities
   * Several of these capabilities are not used by XCSoar, but are
   * present for compatibility
   */
  struct Flags {
    /** If waypoint can be used as a turnpoint */
    bool TurnPoint:1;
    /** If waypoint is to be used as home */
    bool Home:1;
    /** If waypoint is marked as a potential start point */
    bool StartPoint:1;
    /** If waypoint is marked as a potential finish point */
    bool FinishPoint:1;
    /** If waypoint is watched, i.e. displayed with arrival height in map */
    bool Watched:1;

    /**
     * Set default flags (all off except turnpoint)
     *
     * @param turnpoint Whether the waypoint is a turnpoint
     */
    void SetDefaultFlags(bool turnpoint);
  };

  /**
   * Constructor for real waypoints
   *
   * @param is_turnpoint Whether newly created waypoint is a turnpoint
   * @return Uninitialised object
   */
  Waypoint(const GeoPoint &_location, const bool is_turnpoint = false);

  /** Unique id */
  unsigned id;

  /**
   * The id number as specified in the input file.
   */
  unsigned original_id;

  /** Geodetic location */
  GeoPoint Location;
  /** Height AMSL (m) of waypoint terrain */
  fixed Altitude;

  /** Main runway */
  Runway runway;

  RadioFrequency radio_frequency;

  /** Type of the waypoint */
  enum Type Type;
  /** Flag types of this waypoint */
  struct Flags Flags;
  /** File number to store waypoint in (0,1), -1 to delete/ignore */
  int8_t FileNum;

  /** Name of waypoint */
  tstring Name;
  /** Additional comment text for waypoint */
  tstring Comment;
  /** Airfield or additional (long) details */
  tstring Details;

  /** 
   * Determine if waypoint is marked as able to be landed at
   * 
   * @return True if waypoint is landable
   */
  bool
  IsLandable() const
  {
    return (Type == wtAirfield || Type == wtOutlanding);
  }

  /**
   * Determine if waypoint is marked as an airport
   *
   * @return True if waypoint is landable
   */
  bool
  IsAirport() const
  {
    return Type == wtAirfield;
  }

  /**
   * Determine if waypoint is marked as a turnpoint
   *
   * @return True if waypoint is landable
   */
  bool
  IsTurnpoint() const
  {
    return Flags.TurnPoint;
  }

  /**
   * Determine if waypoint is marked as a start
   *
   * @return True if waypoint is start
   */
  bool
  IsStartpoint() const
  {
    return Flags.StartPoint;
  }

  /**
   * Determine if waypoint is marked as a finish
   *
   * @return True if waypoint is finish
   */
  bool
  IsFinishpoint() const
  {
    return Flags.FinishPoint;
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
   * Determine if a waypoint is close to a given location within
   * a threshold
   *
   * @param location Location to compare to
   * @param range Distance threshold (m)
   *
   * @return True if close to reference location
   */
  bool
  IsCloseTo(const GeoPoint &location, const fixed range) const;

public:
#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, const Waypoint& wp);
#endif
};

#endif
