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

#ifndef AIRSPACE_AIRCRAFT_PERFORMANCE_HPP
#define AIRSPACE_AIRCRAFT_PERFORMANCE_HPP

#include "Math/fixed.hpp"
#include "GlideSolvers/GlidePolar.hpp"
#include "Util/AircraftStateFilter.hpp"
#include "Compiler.h"

/**
 *  Class used for simplified/idealised performace
 *  of aircraft speed as a function of glide slope.
 */
class AirspaceAircraftPerformance {
protected:
  fixed m_tolerance_vertical; /**< Tolerance in vertical max speeds (m/s) */

public:

/** 
 * Default constructor.
 * Note that search mechanism will fail if descent_rate and climb_rate are zero
 * without tolerance being positive.
 * 
 * @param tolerance Tolerance of vertical speeds (m/s)
 */
  AirspaceAircraftPerformance(const fixed tolerance=fixed_zero):m_tolerance_vertical(tolerance) {};

/** 
 * Set tolerance of vertical speeds
 * 
 * @param val New value of tolerance, positive (m/s)
 */
  void set_tolerance_vertical(const fixed val) {
    m_tolerance_vertical = val;
  }

/** 
 * Return nominal speed
 * 
 * @return Nominal cruise speed (m/s)
 */
  gcc_pure
  virtual fixed get_cruise_speed() const = 0;

/** 
 * Return nominal descent rate
 * 
 * @return Nominal descent speed (m/s, positive down)
 */
  gcc_pure
  virtual fixed get_cruise_descent() const = 0;

/** 
 * Return descent rate limit (above nominal descent rate) 
 * 
 * @return Max descent speed (m/s, positive down)
 */
  gcc_pure
  virtual fixed get_descent_rate() const = 0;

/** 
 * Return climb rate limit (above nominal descent rate) 
 * 
 * @return Max climb rate (m/s, positive up)
 */
  gcc_pure
  virtual fixed get_climb_rate() const = 0;

/** 
 * Return maximum speed achievable by this model
 * 
 * @return Speed (m/s)
 */
  gcc_pure
  virtual fixed get_max_speed() const = 0;

/** 
 * Find minimum intercept time to a point
 * 
 * @param distance Distance to point (m)
 * @param dh Relative height of observer above point (m)
 * 
 * @return Time to intercept (s) or -1 if failed
 */
  gcc_pure
  virtual fixed solution_general(const fixed& distance,
                                 const fixed& dh) const;

/** 
 * Find time to intercept a target with a height band, set distance
 * 
 * @param distance Lateral distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param base Height of base (m)
 * @param top  Height of top (m)
 * @param intercept_alt If intercept possible, this is the soonest height
 * 
 * @return Time of intercept (s)
 */
  fixed solution_vertical(const fixed& distance,
                          const fixed& altitude,
                          const fixed& base,
                          const fixed& top,
                          fixed& intercept_alt) const;

/** 
 * Find time to intercept a target with a distance band, set height 
 * 
 * @param distance_min Min distance to travel (m)
 * @param distance_max Max distance to travel (m)
 * @param altitude Altitude of observer (m)
 * @param h  Height of target (m)
 * @param intercept_distance If intercept possible, this is the distance to the soonest point
 * 
 * @return Time of intercept (s)
 */
  fixed solution_horizontal(const fixed& distance_min,
                            const fixed& distance_max,
                            const fixed& altitude,
                            const fixed& h,
                            fixed& intercept_distance) const;

private:
  gcc_pure
  virtual bool solution_exists(const fixed& distance_min,
                               const fixed& distance_max,
                               const fixed& h_min,
                               const fixed& h_max) const;
};


/** 
 * Simplified aircraft performance model used for testing of
 * airspace warning system with minimal dependencies.
 * 
 */
class AirspaceAircraftPerformanceSimple:
  public AirspaceAircraftPerformance 
{
  fixed v_ld;
  fixed s_ld;
  fixed climb_rate;
  fixed descent_rate;

/** 
 * Constructor.  Initialises current to experimental values.
 * Intended to be specialised for a real aircraft performance model.
 * 
 */
protected:
  AirspaceAircraftPerformanceSimple():v_ld(30.0),s_ld(2.0),
                                      climb_rate(10.0),
                                      descent_rate(10.0)
    {};

public:
  virtual fixed get_cruise_speed() const {
    return v_ld;
  }

  virtual fixed get_cruise_descent() const {
    return s_ld;
  }

  virtual fixed get_climb_rate() const {
    return climb_rate;
  }

  virtual fixed get_descent_rate() const {
    return s_ld;
  }

  virtual fixed get_max_speed() const {
    return v_ld;
  }
};

/**
 * Specialisation of AirspaceAircraftPerformance
 * based on simplified theoretical MC cross-country speeds.
 * Assumes cruise at best LD (ignoring wind) for current MC setting,
 * climb rate at MC setting, with direct descent possible at sink rate
 * of cruise.
 */
class AirspaceAircraftPerformanceGlide: 
  public AirspaceAircraftPerformance
{
public:
/** 
 * Constructor.
 * 
 * @param polar Polar to take data from
 * 
 * @return Initialised object
 */
  AirspaceAircraftPerformanceGlide(const GlidePolar& polar):
    m_glide_polar(polar) {

  };

  virtual fixed get_cruise_speed() const {
    return m_glide_polar.GetVBestLD();
  }

  virtual fixed get_cruise_descent() const {
    return m_glide_polar.GetSBestLD();
  }

  virtual fixed get_climb_rate() const {
    return m_glide_polar.GetMC();
  }

  virtual fixed get_descent_rate() const {
    return m_glide_polar.GetSMax();
  }

  virtual fixed get_max_speed() const {
    return m_glide_polar.GetVMax();
  }

protected:
  const GlidePolar &m_glide_polar; /**< Glide polar used for speed model */
};


/**
 * Specialisation of AirspaceAircraftPerformance based on
 * low pass filtered aircraft state --- effectively producing
 * potential solutions at average speed in the averaged direction
 * at the averaged climb rate.
 */
class AirspaceAircraftPerformanceStateFilter: 
  public AirspaceAircraftPerformance
{
  const AircraftStateFilter &m_state_filter;

public:
/** 
 * Constructor.
 * 
 * @param filter Filter to retrieve state information from
 * 
 * @return Initialised object
 */
  AirspaceAircraftPerformanceStateFilter(const AircraftStateFilter& filter):
    AirspaceAircraftPerformance(fixed(0.01)),
    m_state_filter(filter) {

  }

  virtual fixed get_cruise_speed() const {
    return m_state_filter.GetSpeed();
  }

  virtual fixed get_cruise_descent() const {
    return -m_state_filter.GetClimbRate();
  }

  virtual fixed get_climb_rate() const {
    return m_state_filter.GetClimbRate();
  }

  virtual fixed get_descent_rate() const {
    return -m_state_filter.GetClimbRate();
  }

  virtual fixed get_max_speed() const {
    return m_state_filter.GetSpeed();
  }
};


class TaskManager;

/**
 * Specialisation of AirspaceAircraftPerformance
 * for tasks where part of the path is in cruise, part in
 * final glide.  This is intended to be used temporarily only.
 *
 * This simplifies the path by assuming flight is constant altitude
 * or descent to the task point elevation.
 */
class AirspaceAircraftPerformanceTask: 
  public AirspaceAircraftPerformance
{
  fixed m_v;
  fixed m_cruise_descent;
  fixed m_max_descent;
  fixed m_climb_rate;

public:
/** 
 * Constructor.
 * 
 * @param state Aircraft state at query
 * @param polar Polar
 * @param task Task to retrieve plan from
 * 
 * @return Initialised object
 */
  AirspaceAircraftPerformanceTask(const AircraftState &state,
                                  const GlidePolar& polar,
                                  const TaskManager& task);

  fixed get_cruise_speed() const;

  fixed get_cruise_descent() const;

  fixed get_climb_rate() const;

  fixed get_descent_rate() const;

  fixed get_max_speed() const;
};


#endif
