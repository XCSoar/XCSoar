/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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

#include <iostream>
#include "Navigation/Geometry/GeoVector.hpp"

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
    RESULT_OK = 0,              /**< Solution is achievable */
    RESULT_PARTIAL,             /**< Solution is partially achievable */
    RESULT_WIND_EXCESSIVE,      /**< Wind is too strong to allow progress */
    RESULT_MACCREADY_INSUFFICIENT, /**< Expected climb rate is too low to allow progress */
    RESULT_NOSOLUTION           /**< Solution not computed or algorithm failed */
  };

/** 
 * Dummy constructor for null result.  Used as default
 * return value for failed/trivial tasks
 * 
 * @return Initialised null result
 */
  GlideResult();

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
  GlideResult(const GlideState &task, 
               const fixed V);

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
 * Check whether task is partially achievable.  It will
 * fail if the wind is excessive for the current MC value.
 * 
 * @return True if task is at least partially achievable
 */
  bool ok_or_partial() const {
    return (Solution == RESULT_OK)
      || (Solution == RESULT_PARTIAL);
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
 * Find the gradient of this solution relative to ground
 * 
 * @return Glide gradient (positive down), or zero if no distance to travel.
 */
  fixed glide_angle_ground() const;

#ifdef DO_PRINT
  friend std::ostream& operator<< (std::ostream& o, 
                                   const GlideResult& gl);
#endif

  GeoVector Vector;             /**< Distance/bearing of task achievable */
  fixed DistanceToFinal;       /**< Distance to go before final glide (m) */
  fixed CruiseTrackBearing;    /**< Track bearing in cruise for optimal drift compensation (deg true) */
  fixed VOpt;                  /**< Optimal speed to fly in cruise (m/s) */
  fixed HeightClimb;           /**< Height to be climbed (m) */
  fixed HeightGlide;           /**< Height that will be glided (m) */
  fixed TimeElapsed;           /**< Time to complete task (s) */
  fixed TimeVirtual;           /**< Equivalent time to recover glided height (s) at MC */
  fixed AltitudeDifference;    /**< Height above/below final glide for this task (m) */
  fixed AltitudeRequired;      /**< Height required to solve this task (m) */
  fixed EffectiveWindSpeed;    /**< (internal) */
  fixed EffectiveWindAngle;    /**< (internal) */
  fixed HeadWind;              /**< Head wind component (m/s) in cruise */
  GlideResult_t Solution;       /**< Solution validity */

private:

/** 
 * Calculate cruise track bearing from internal variables.
 * This is expensive so is only done on demand.
 */
  void calc_cruise_bearing();


};

#endif
