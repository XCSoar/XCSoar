// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "AirspaceAircraftPerformance.hpp"
#include "Math/ZeroFinder.hpp"

#include <algorithm>

static constexpr FloatDuration BIG{1000000.};

FloatDuration
AirspaceAircraftPerformance::SolutionGeneral(double distance, double dh) const noexcept
{
  const FloatDuration t_cruise{distance > 0
    ? distance / GetCruiseSpeed()
    : double(0)};
  const auto h_descent = dh - t_cruise.count() * GetCruiseDescent();

  if (fabs(h_descent) < 1)
    return t_cruise;

  if (h_descent > 0) {
    // descend steeper than best glide

    auto mod_descent_rate = GetDescentRate() + vertical_tolerance;

    if (mod_descent_rate <= 0)
      return BIG;

    const FloatDuration t_descent{h_descent / mod_descent_rate};
    return std::max(t_cruise, t_descent);

  }

  // require climb

  auto mod_climb_rate = GetClimbRate() + vertical_tolerance;

  if (mod_climb_rate <= 0)
    return BIG;

  const FloatDuration t_climb{-h_descent / mod_climb_rate};
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

  double f(const double h) noexcept override {
    return m_perf.SolutionGeneral(m_distance, m_alt - h).count();
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param h Altitude of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  FloatDuration solve(double &h) noexcept {
    auto h_this = find_min(m_h_min);
    FloatDuration t{f(h_this)};
    if (t < BIG) {
      h = h_this;
      return t;
    }
    return FloatDuration{-1};
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const double m_distance;
  const double m_alt;
  const double m_h_min;
};

FloatDuration
AirspaceAircraftPerformance::SolutionVertical(double distance, double altitude,
                                              double base, double top,
                                              double &intercept_alt) const noexcept
{
  if (!SolutionExists(distance, altitude, base, top))
    return FloatDuration{-1};

  if (top <= base) {
    // unique solution
    auto t_this = SolutionGeneral(distance, altitude - top);
    if (t_this < BIG) {
      intercept_alt = top;
      return t_this;
    }
    return FloatDuration{-1};
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

  double f(const double distance) noexcept override {
    return m_perf.SolutionGeneral(distance, m_dh).count();
  }

  /**
   * Find distance of minimum time intercept with line
   *
   * @param distance Distance of intercept to be set if solution found (m)
   *
   * @return Time of arrival (or -1 if no solution found)
   */
  FloatDuration solve(double &distance) noexcept {
    auto distance_this = find_min(m_d_min);
    FloatDuration t{f(distance_this)};
    if (t < BIG) {
      distance = distance_this;
      return t;
    }
    return FloatDuration{-1};
  }

private:
  const AirspaceAircraftPerformance &m_perf;
  const double m_d_min;
  const double m_dh;
};

FloatDuration
AirspaceAircraftPerformance::SolutionHorizontal(double distance_min,
                                                double distance_max,
                                                double altitude, double h,
                                                double &intercept_distance) const noexcept
{
  if (!SolutionExists(distance_max, altitude, h, h))
    return FloatDuration{-1};

  const auto dh = altitude - h;

  if (distance_max <= distance_min) {
    // unique solution
    auto t_this = SolutionGeneral(distance_max, dh);
    if (t_this != BIG) {
      intercept_distance = distance_max;
      return t_this;
    }
    return FloatDuration{-1};
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
