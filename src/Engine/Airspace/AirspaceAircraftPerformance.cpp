/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2012 The XCSoar Project
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

#include "AirspaceAircraftPerformance.hpp"
#include "Math/ZeroFinder.hpp"
#include "GlideSolvers/GlideResult.hpp"

#include <assert.h>

#define fixed_big fixed(1000000)

fixed 
AirspaceAircraftPerformance::SolutionGeneral(fixed distance, fixed dh) const
{
  const fixed t_cruise =
      positive(distance) ? distance / GetCruiseSpeed() : fixed(0);
  const fixed h_descent = dh - t_cruise * GetCruiseDescent();

  if (fabs(h_descent) < fixed(1))
    return t_cruise;

  if (positive(h_descent)) {
    // descend steeper than best glide

    fixed mod_descent_rate = GetDescentRate() + vertical_tolerance;

    if (!positive(mod_descent_rate))
      return fixed_big;

    const fixed t_descent = h_descent / mod_descent_rate;
    return std::max(t_cruise, t_descent);

  }

  // require climb

  fixed mod_climb_rate = GetClimbRate() + vertical_tolerance;

  if (!positive(mod_climb_rate))
    return fixed_big;

  const fixed t_climb = -h_descent / mod_climb_rate;
  return t_cruise + t_climb;
}

/**
 * Utility class to scan for height difference that produces
 * minimum arrival time intercept with a vertical line
 */
class AirspaceAircraftInterceptVertical: 
  public ZeroFinder 
{
public:
  /**
   * Constructor
   *
   * @param aap Performance model
   * @param distance Distance to line (m)
   * @param alt Altitude of observer (m)
   * @param h_min Height of base of line (m)
   * @param h_max Height of top of line (m)
   *
   * @return Initialised object
   */
  AirspaceAircraftInterceptVertical(const AirspaceAircraftPerformance &aap,
                                    fixed distance, fixed alt,
                                    fixed h_min, fixed h_max)
    :ZeroFinder(h_min, h_max, fixed(1)),
     m_perf(aap), m_distance(distance), m_alt(alt),
     m_h_min(h_min) {}

  fixed f(const fixed h) {
    return m_perf.SolutionGeneral(m_distance, m_alt-h);
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param h Altitude of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  fixed solve(fixed &h) {
    fixed h_this = find_min(m_h_min);
    fixed t = f(h_this);
    if (t < fixed_big) {
      h = h_this;
      return t;
    }
    return fixed(-1);
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const fixed m_distance;
  const fixed m_alt;
  const fixed m_h_min;
};

fixed 
AirspaceAircraftPerformance::SolutionVertical(fixed distance, fixed altitude,
                                              fixed base, fixed top,
                                              fixed &intercept_alt) const
{
  if (!SolutionExists(distance, altitude, base, top))
    return fixed(-1);

  if (top <= base) {
    // unique solution
    fixed t_this = SolutionGeneral(distance, altitude - top);
    if (t_this < fixed_big) {
      intercept_alt = top;
      return t_this;
    }
    return fixed(-1);
  }

  AirspaceAircraftInterceptVertical aaiv(*this, distance, altitude, base, top);
  return aaiv.solve(intercept_alt);
}

/**
 * Utility class to scan for distance that produces
 * minimum arrival time intercept with a horizontal line
 */
class AirspaceAircraftInterceptHorizontal: 
  public ZeroFinder 
{
public:
  /**
   * Constructor
   *
   * @param aap Performance model
   * @param distance_min Distance to line start (m)
   * @param distance_max Distance to line end (m)
   * @param dh Height difference between observer and line
   *
   * @return Initialised object
   */
  AirspaceAircraftInterceptHorizontal(const AirspaceAircraftPerformance &aap,
                                      fixed distance_min, fixed distance_max,
                                      fixed dh)
    :ZeroFinder(distance_min, distance_max, fixed(1)),
     m_perf(aap), m_d_min(distance_min), m_dh(dh) {}

  fixed f(const fixed distance) {
    return m_perf.SolutionGeneral(distance, m_dh);
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param distance Distance of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  fixed solve(fixed &distance) {
    fixed distance_this = find_min(m_d_min);
    fixed t = f(distance_this);
    if (t < fixed_big) {
      distance = distance_this;
      return t;
    }
    return fixed(-1);
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const fixed m_d_min;
  const fixed m_dh;
};

fixed 
AirspaceAircraftPerformance::SolutionHorizontal(fixed distance_min,
                                                fixed distance_max,
                                                fixed altitude, fixed h,
                                                fixed &intercept_distance) const
{
  if (!SolutionExists(distance_max, altitude, h, h))
    return fixed(-1);

  const fixed dh = altitude - h;

  if (distance_max <= distance_min) {
    // unique solution
    fixed t_this = SolutionGeneral(distance_max, dh);
    if (t_this != fixed_big) {
      intercept_distance = distance_max;
      return t_this;
    }
    return fixed(-1);
  }
  AirspaceAircraftInterceptHorizontal aaih(*this, distance_min, distance_max, dh);
  return aaih.solve(intercept_distance);
}
                                      
/*
TODO: write a sorter/visitor so that we can visit airspaces in increasing
  order of arrival time (plus other criteria). 
 */

bool 
AirspaceAircraftPerformance::SolutionExists(fixed distance_max,
                                            fixed altitude,
                                            fixed h_min, fixed h_max) const
{
  if (positive(altitude - h_max) &&
      !positive(std::max(GetCruiseDescent(), GetDescentRate())
                + vertical_tolerance))
    return false;

  if (positive(h_min-altitude) &&
      !positive(std::max(GetClimbRate(), -GetCruiseDescent())
                + vertical_tolerance))
    return false;

  if (positive(distance_max) && !positive(GetCruiseSpeed()))
    return false;

  return true;
}

AirspaceAircraftPerformanceTask::AirspaceAircraftPerformanceTask(const GlidePolar &polar,
                                                                 const GlideResult &solution)
{
  const fixed time_remaining = solution.time_elapsed;

  if (solution.IsOk() && positive(time_remaining)) {
    const fixed leg_distance = solution.vector.distance;
    m_v = leg_distance / time_remaining;
    if (positive(solution.height_climb)) {
      m_cruise_descent = -solution.height_climb / time_remaining;
      m_climb_rate = polar.GetMC();
    } else {
      m_cruise_descent = solution.height_glide / time_remaining;
      m_climb_rate = fixed(0);
    }
  } else {
    m_v = fixed(1);
    m_cruise_descent = fixed(0);
    m_climb_rate = fixed(0);
  }
  m_max_descent = polar.GetSBestLD();

  SetVerticalTolerance(fixed(0.001));
}

fixed 
AirspaceAircraftPerformanceTask::GetCruiseSpeed() const
{
  return m_v;
}

fixed 
AirspaceAircraftPerformanceTask::GetCruiseDescent() const
{
  return m_cruise_descent;
}

fixed 
AirspaceAircraftPerformanceTask::GetClimbRate() const
{
  return m_climb_rate;
}

fixed 
AirspaceAircraftPerformanceTask::GetDescentRate() const
{
  return m_max_descent;
}

fixed 
AirspaceAircraftPerformanceTask::GetMaxSpeed() const
{
  return m_v;
}
