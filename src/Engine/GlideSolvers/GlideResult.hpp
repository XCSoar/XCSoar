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
#ifndef GLIDERESULT_HPP
#define GLIDERESULT_HPP

#include "Geo/GeoVector.hpp"
#include "GlideSettings.hpp"
#include "Compiler.h"

#include <type_traits>

#include <stdint.h>

struct AircraftState;
struct GlideState;
class GlidePolar;

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
    OK = 0,
    /** Wind is too strong to allow progress */
    WIND_EXCESSIVE,
    /** Expected climb rate is too low to allow progress */
    MACCREADY_INSUFFICIENT,
    /** Solution not computed or algorithm failed */
    NO_SOLUTION
  };

  /**
   * Head wind component [m/s] in cruise.  Immutable input value.
   */
  double head_wind;

  /**
   * Optimal speed to fly in cruise [m/s].  Immutable input value.
   */
  double v_opt;

#ifndef NDEBUG
  /**
   * The altitude of the aircraft at the beginning of this leg [m
   * MSL].  Immutable input value.
   *
   * This attribute shall aid debugging, and will be removed once we
   * are certain the MacCready code is stable.
   */
  double start_altitude;
#endif

  /**
   * The minimum altitude for arrival at the target (i.e. target
   * altitude plus safety margin).  Immutable input value.
   */
  double min_arrival_altitude;

  /**
   * Cruise vector of this result.  Usually, this equals the remaining
   * vector (see ElementStat::vector_remaining), but if a solution is
   * not achievable, this is the portion of the remaining vector that
   * is achievable with straight cruise (may be an empty vector).
   */
  GeoVector vector;

  /**
   * The minimum arrival altitude, assuming pure glide.  This is
   * usually the same as #min_arrival_altitude, but may differ on
   * multi-leg calculations when there is an obstacle.
   */
  double pure_glide_min_arrival_altitude;

  /**
   * The total height that would be glided straight along the vector
   * if the target were reachable by pure glide (i.e. ignoring current
   * aircraft altitude, minimum arrival altitude and cruise
   * efficiency).  Wind is considered, but not additional wind drift
   * while circling.
   *
   * This attribute is only valid when validity==OK.
   */
  double pure_glide_height;

  /**
   * The height above/below final glide, assuming pure glide.
   */
  double pure_glide_altitude_difference;

  /**
   * Track bearing in cruise for optimal drift compensation.
   *
   * This attribute is only valid when validity==OK.
   */
  Angle cruise_track_bearing;

  /**
   * Total height to be climbed [m relative].
   */
  double height_climb;

  /**
   * Total height that will lost during straight glide along this
   * solution.
   */
  double height_glide;

  /** Time to complete task (s) */
  double time_elapsed;
  /** Equivalent time to recover glided height (s) at MC */
  double time_virtual;

  /**
   * Height above/below final glide for this task [m relative].
   */
  double altitude_difference;

  double effective_wind_speed;
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
  GlideResult(const GlideState &task, double V);

  bool IsDefined() const {
    return validity != Validity::NO_SOLUTION;
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
   * Check whether task is partially achievable.  It will
   * fail if the wind is excessive for the current MC value.
   *
   * @return True if task is at least partially achievable
   */
  bool
  IsOk() const
  {
    return validity == Validity::OK;
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
   * Altitude required at the start of the task to solve it in
   * pure glide [m MSL].
   */
  gcc_pure
  double GetRequiredAltitude() const {
    return pure_glide_min_arrival_altitude + pure_glide_height;
  }

  /**
   * Returns the altitude of the aircraft at the beginning of this leg
   * [m MSL], as specified in the MacCready calculation input
   * parameters.
   */
  gcc_pure
  double GetStartAltitude() const {
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
  double GetArrivalAltitude(double start_altitude) const {
    return start_altitude - pure_glide_height;
  }

  /**
   * Returns the calculated altitude of the aircraft at the end of
   * this leg, not assuming any climbs [m MSL].  It may be below the
   * safety altitude or even below terrain.
   */
  gcc_pure
  double GetArrivalAltitude() const {
    return GetArrivalAltitude(GetStartAltitude());
  }

  /**
   * Altitude required at the start of the task, considering wind
   * drift while circling [m MSL].  This is a theoretical value,
   * because this altitude will probably never actually be reached.
   */
  gcc_pure
  double GetRequiredAltitudeWithDrift() const {
    return min_arrival_altitude + height_glide;
  }

  /**
   * Like GetArrivalAltitude(), but considers wind drift while
   * circling.
   *
   * @param start_altitude the current aircraft altitude
   */
  gcc_pure
  double GetArrivalAltitudeWithDrift(double start_altitude) const {
    return start_altitude - height_glide;
  }

  /**
   * Calculate the height above/below final glide, assuming pure
   * glide.
   *
   * @param start_altitude the current aircraft altitude
   */
  gcc_pure
  double GetPureGlideAltitudeDifference(double start_altitude) const {
    return start_altitude - GetRequiredAltitude();
  }

  gcc_pure
  double SelectAltitudeDifference(const GlideSettings &settings) const {
    return settings.predict_wind_drift
      ? altitude_difference
      : pure_glide_altitude_difference;
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
   * Find the gradient of this solution relative to ground.
   * (does not use up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  gcc_pure
  double GlideAngleGround() const;

  /**
   * Find the gradient of the target relative to ground
   * (uses up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  gcc_pure
  double DestinationAngleGround() const;

  /** Reset/clear the solution */
  void Reset();

  /**
   * Calculate instantaneous task speed according to enhanced Pirker
   * algorithm using ground speed along track and vario value.  See
   * code in InstantSpeed.cpp for algorithm details.
   *
   * @return instantaneous speed (m/s)
   */
  double InstantSpeed(const AircraftState &aircraft, const GlideResult& leg,
                      const GlidePolar& glide_polar);

private:
  /**
   * Calculate cruise track bearing from internal variables.
   * This is expensive so is only done on demand.
   */
  void CalcCruiseBearing();
};

static_assert(std::is_trivial<GlideResult>::value, "type is not trivial");

#endif
