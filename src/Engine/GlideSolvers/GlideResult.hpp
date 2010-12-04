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

#ifdef DO_PRINT
#include <iostream>
#endif

struct GlideState;
struct AIRCRAFT_STATE;

/**
 * Class used to represent a solution to a glide task
 */
struct GlideResult {

  /**
   * Results of glide calculations.  Used to provide feedback if
   * fails due to insufficient MC value etc.
   */
  enum GlideResult_t {
    RESULT_OK = 0,                 /**< Solution is achievable */
    RESULT_PARTIAL,                /**< Solution is partially achievable */
    RESULT_WIND_EXCESSIVE,         /**< Wind is too strong to allow progress */
    RESULT_MACCREADY_INSUFFICIENT, /**< Expected climb rate is too low to allow progress */
    RESULT_NOSOLUTION              /**< Solution not computed or algorithm failed */
  };

  /**
   * Construct an uninitialised object.
   */
  GlideResult() {}

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

  bool defined() const {
    return Solution != RESULT_NOSOLUTION;
  }

  /**
   * Calculate additional items (CruiseTrackBearing and AltitudeRequired) that were
   * deferred.
   * @param state State from which this solution was obtained
   */
  void calc_deferred(const AIRCRAFT_STATE &state);

  /**
   * Check whether aircraft can finish this task without
   * further climb.
   *
   * @return True if aircraft is at or above final glide
   */
  bool is_final_glide() const;

  /**
   * Convenience function, returns location of start of final glide component
   */
  GeoPoint location_at_final(const GeoPoint &location) const;

  /**
   * Check whether task is partially achievable.  It will
   * fail if the wind is excessive for the current MC value.
   *
   * @return True if task is at least partially achievable
   */
  bool
  ok_or_partial() const
  {
    return (Solution == RESULT_OK) || (Solution == RESULT_PARTIAL);
  }

  /**
   * Check whether task is achievable (optionally entirely on final glide)
   *
   * @param final_glide Whether no further climb allowed
   *
   * @return True if target is reachable
   */
  bool glide_reachable(const bool final_glide=true) const;

  /**
   * Adds another GlideResult to this.  This is used to
   * accumulate GlideResults for a sequence of task segments.
   * The order is important.
   *
   * @param s2 The other glide result segment
   */
  void add(const GlideResult &s2);

  /**
   * Calculate virtual speed of solution.  This is defined as
   * the distance divided by the time elapsed plus the time required
   * to recover the altitude expended in cruise.
   *
   * @param inv_mc Inverse of MC value (s/m), negative if MC is zero
   *
   * @return Virtual speed (m/s)
   */
  fixed calc_vspeed(const fixed inv_mc);

  /**
   * Find the gradient of this solution relative to ground.
   * (does not use up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  fixed glide_angle_ground() const;

  /**
   * Find the gradient of the target relative to ground
   * (uses up AltitudeDifference)
   *
   * @return Glide gradient (positive down), or inf if no distance to travel.
   */
  fixed destination_angle_ground() const;

  /**
   * Reset/clear the solution
   */
  void reset();

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const GlideResult& gl);
#endif

  GeoVector Vector;            /**< Distance/bearing of task achievable */
  fixed DistanceToFinal;       /**< Distance to go before final glide (m) */
  Angle CruiseTrackBearing;    /**< Track bearing in cruise for optimal drift compensation (deg true) */
  fixed VOpt;                  /**< Optimal speed to fly in cruise (m/s) */
  fixed HeightClimb;           /**< Height to be climbed (m) */
  fixed HeightGlide;           /**< Height that will be glided (m) */
  fixed TimeElapsed;           /**< Time to complete task (s) */
  fixed TimeVirtual;           /**< Equivalent time to recover glided height (s) at MC */
  fixed AltitudeDifference;    /**< Height above/below final glide for this task (m) */
  fixed AltitudeRequired;      /**< Height required to solve this task (m) */
  fixed EffectiveWindSpeed;    /**< (internal) */
  Angle EffectiveWindAngle;    /**< (internal) */
  fixed HeadWind;              /**< Head wind component (m/s) in cruise */
  GlideResult_t Solution;      /**< Solution validity */
  fixed MinHeight;             /**< Height (m above MSL) of end */

private:
  /**
   * Calculate cruise track bearing from internal variables.
   * This is expensive so is only done on demand.
   */
  void calc_cruise_bearing();
};

#endif
