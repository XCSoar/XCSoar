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
#ifndef GLIDERESULT_HPP
#define GLIDERESULT_HPP

#include "Navigation/Geometry/GeoVector.hpp"
#include "Util/TypeTraits.hpp"
#include "Compiler.h"

#ifdef DO_PRINT
#include <iostream>
#endif

struct GlideState;
struct AircraftState;

/**
 * Class used to represent a solution to a glide task
 */
struct GlideResult {

  /**
   * Results of glide calculations.  Used to provide feedback if
   * fails due to insufficient MC value etc.
   */
  enum class Validity : uint8_t {
    /** Solution is achievable */
    RESULT_OK = 0,
    /** Wind is too strong to allow progress */
    RESULT_WIND_EXCESSIVE,
    /** Expected climb rate is too low to allow progress */
    RESULT_MACCREADY_INSUFFICIENT,
    /** Solution not computed or algorithm failed */
    RESULT_NOSOLUTION
  };

  /**
   * Head wind component [m/s] in cruise.  Immutable input value.
   */
  fixed head_wind;

  /**
   * Optimal speed to fly in cruise [m/s].  Immutable input value.
   */
  fixed v_opt;

#ifndef NDEBUG
  /**
   * The altitude of the aircraft at the beginning of this leg [m
   * MSL].  Immutable input value.
   *
   * This attribute shall aid debugging, and will be removed once we
   * are certain the MacCready code is stable.
   */
  fixed start_altitude;
#endif

  /**
   * Altitude [m above MSL] of target.  Immutable input value.
   */
  fixed min_height;

  /**
   * Cruise vector of this result.  Usually, this equals the remaining
   * vector (see ElementStat::vector_remaining), but if a solution is
   * not achievable, this is the portion of the remaining vector that
   * is achievable with straight cruise (may be an empty vector).
   */
  GeoVector vector;

  /** Distance to go before final glide (m) */
  fixed distance_to_final;
  /** Track bearing in cruise for optimal drift compensation (deg true) */
  Angle cruise_track_bearing;

  /**
   * Total height to be climbed [m relative].
   */
  fixed height_climb;

  /**
   * Total height that will lost during straight glide along this
   * solution.
   */
  fixed height_glide;

  /** Time to complete task (s) */
  fixed time_elapsed;
  /** Equivalent time to recover glided height (s) at MC */
  fixed time_virtual;

  /**
   * Height above/below final glide for this task [m relative].
   */
  fixed altitude_difference;

  fixed effective_wind_speed;
  Angle effective_wind_angle;

  /** Solution validity */
  Validity validity;

  /** Construct an uninitialised object. */
  GlideResult() = default;

  /**
   * Constructor with partial initialisation for a particular
   * task.  This copies task information so that the resulting instance
   * contains everything required for navigation and the GlideState
   * instance can be destroyed.
   *
   * @param task Task for which glide result will be calculated
   * @param V Optimal speed to fly
   *
   * @return Blank glide result
   */
  GlideResult(const GlideState &task, const fixed V);

  bool IsDefined() const {
    return validity != Validity::RESULT_NOSOLUTION;
  }

  /**
   * Calculate additional items (CruiseTrackBearing and AltitudeRequired) that were
   * deferred.
   */
  void CalcDeferred();

  /**
   * Check whether aircraft can finish this task without
   * further climb.
   *
   * @return True if aircraft is at or above final glide
   */
  gcc_pure
  bool IsFinalGlide() const;

  /**
   * Convenience function, returns location of start of final glide component
   */
  gcc_pure
  GeoPoint FinalGlideStartLocation(const GeoPoint &location) const;

  /**
   * Check whether task is partially achievable.  It will
   * fail if the wind is excessive for the current MC value.
   *
   * @return True if task is at least partially achievable
   */
  bool
  IsOk() const
  {
    return validity == Validity::RESULT_OK;
  }

  /**
   * Check whether task is achievable (optionally entirely on final glide)
   *
   * @return True if target is reachable
   */
  gcc_pure
  bool IsAchievable() const {
    return IsOk();
  }

  /**
   * Absolute altitude required to solve this task [m MSL].  This is
   * the current aircraft altitude plus #altitude_difference.
   */
  gcc_pure
  fixed GetRequiredAltitude() const {
    return min_height + height_glide;
  }

  /**
   * Returns the altitude of the aircraft at the beginning of this leg
   * [m MSL], as specified in the MacCready calculation input
   * parameters.
   */
  gcc_pure
  fixed GetStartAltitude() const {
    return GetRequiredAltitude() + altitude_difference;
  }

  /**
   * Returns the calculated altitude of the aircraft at the end of
   * this leg, not assuming any climbs [m MSL].  It may be below the
   * safety altitude or even below terrain.
   *
   * @param start_altitude the current aircraft altitude
   */
  gcc_pure
  fixed GetArrivalAltitude(fixed start_altitude) const {
    return start_altitude - height_glide;
  }

  /**
   * Returns the calculated altitude of the aircraft at the end of
   * this leg, not assuming any climbs [m MSL].  It may be below the
   * safety altitude or even below terrain.
   */
  gcc_pure
  fixed GetArrivalAltitude() const {
    return GetArrivalAltitude(GetStartAltitude());
  }

  /**
   * Adds another GlideResult to this.  This is used to
   * accumulate GlideResults for a sequence of task segments.
   * The order is important.
   *
   * @param s2 The other glide result segment
   */
  void Add(const GlideResult &s2);

  /**
   * Calculate virtual inverse speed of solution.  This is defined as
   * the (time elapsed plus the time required to recover the altitude
   * expended in cruise) divided by the distance divided.
   *
   * @param inv_mc Inverse of MC value (s/m), negative if MC is zero
   *
   * @return Inverse of Virtual speed (s/m)
   */
  fixed CalcVInvSpeed(const fixed inv_mc);

  /**
   * Find the gradient of this solution relative to ground.
   * (does not use up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  gcc_pure
  fixed GlideAngleGround() const;

  /**
   * Find the gradient of the target relative to ground
   * (uses up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  gcc_pure
  fixed DestinationAngleGround() const;

  /** Reset/clear the solution */
  void Reset();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const GlideResult& gl);
#endif

private:
  /**
   * Calculate cruise track bearing from internal variables.
   * This is expensive so is only done on demand.
   */
  void CalcCruiseBearing();
};

static_assert(is_trivial<GlideResult>::value, "type is not trivial");

#endif
