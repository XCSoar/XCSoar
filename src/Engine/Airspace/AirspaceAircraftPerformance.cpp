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

#include "AirspaceAircraftPerformance.hpp"
#include "Math/ZeroFinder.hpp"

#include <algorithm>

#define BIG 1000000.

double
AirspaceAircraftPerformance::SolutionGeneral(double distance, double dh) const
{
  const auto t_cruise = distance > 0
    ? distance / GetCruiseSpeed()
    : double(0);
  const auto h_descent = dh - t_cruise * GetCruiseDescent();

  if (fabs(h_descent) < 1)
    return t_cruise;

  if (h_descent > 0) {
    // descend steeper than best glide

    auto mod_descent_rate = GetDescentRate() + vertical_tolerance;

    if (mod_descent_rate <= 0)
      return BIG;

    const auto t_descent = h_descent / mod_descent_rate;
    return std::max(t_cruise, t_descent);

  }

  // require climb

  auto mod_climb_rate = GetClimbRate() + vertical_tolerance;

  if (mod_climb_rate <= 0)
    return BIG;

  const auto t_climb = -h_descent / mod_climb_rate;
  return t_cruise + t_climb;
}

/**
 * Utility class to scan for height difference that produces
 * minimum arrival time intercept with a vertical line
 */
class AirspaceAircraftInterceptVertical final : public ZeroFinder {
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
                                    double distance, double alt,
                                    double h_min, double h_max)
    :ZeroFinder(h_min, h_max, 1),
     m_perf(aap), m_distance(distance), m_alt(alt),
     m_h_min(h_min) {}

  double f(const double h) {
    return m_perf.SolutionGeneral(m_distance, m_alt-h);
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param h Altitude of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  double solve(double &h) {
    auto h_this = find_min(m_h_min);
    auto t = f(h_this);
    if (t < BIG) {
      h = h_this;
      return t;
    }
    return -1;
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const double m_distance;
  const double m_alt;
  const double m_h_min;
};

double
AirspaceAircraftPerformance::SolutionVertical(double distance, double altitude,
                                              double base, double top,
                                              double &intercept_alt) const
{
  if (!SolutionExists(distance, altitude, base, top))
    return -1;

  if (top <= base) {
    // unique solution
    auto t_this = SolutionGeneral(distance, altitude - top);
    if (t_this < BIG) {
      intercept_alt = top;
      return t_this;
    }
    return -1;
  }

  AirspaceAircraftInterceptVertical aaiv(*this, distance, altitude, base, top);
  return aaiv.solve(intercept_alt);
}

/**
 * Utility class to scan for distance that produces
 * minimum arrival time intercept with a horizontal line
 */
class AirspaceAircraftInterceptHorizontal final : public ZeroFinder {
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
                                      double distance_min, double distance_max,
                                      double dh)
    :ZeroFinder(distance_min, distance_max, 1),
     m_perf(aap), m_d_min(distance_min), m_dh(dh) {}

  double f(const double distance) {
    return m_perf.SolutionGeneral(distance, m_dh);
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param distance Distance of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  double solve(double &distance) {
    auto distance_this = find_min(m_d_min);
    auto t = f(distance_this);
    if (t < BIG) {
      distance = distance_this;
      return t;
    }
    return -1;
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const double m_d_min;
  const double m_dh;
};

double 
AirspaceAircraftPerformance::SolutionHorizontal(double distance_min,
                                                double distance_max,
                                                double altitude, double h,
                                                double &intercept_distance) const
{
  if (!SolutionExists(distance_max, altitude, h, h))
    return -1;

  const auto dh = altitude - h;

  if (distance_max <= distance_min) {
    // unique solution
    auto t_this = SolutionGeneral(distance_max, dh);
    if (t_this != BIG) {
      intercept_distance = distance_max;
      return t_this;
    }
    return -1;
  }
  AirspaceAircraftInterceptHorizontal aaih(*this, distance_min, distance_max, dh);
  return aaih.solve(intercept_distance);
}
                                      
/*
TODO: write a sorter/visitor so that we can visit airspaces in increasing
  order of arrival time (plus other criteria). 
 */

bool 
AirspaceAircraftPerformance::SolutionExists(double distance_max,
                                            double altitude,
                                            double h_min, double h_max) const
{
  if (altitude - h_max > 0 &&
      std::max(GetCruiseDescent(), GetDescentRate()) + vertical_tolerance <= 0)
    return false;

  if (h_min-altitude > 0 &&
      std::max(GetClimbRate(), -GetCruiseDescent()) + vertical_tolerance <= 0)
    return false;

  if (distance_max > 0 && GetCruiseSpeed() <= 0)
    return false;

  return true;
}
