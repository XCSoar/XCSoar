// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "GlideState.hpp"
#include "Math/Quadratic.hpp"

/**
 * Quadratic function solver for MacCready theory constraint equation
 *
 * - document this equation!
 */
class AverageSpeedSolver : public Quadratic {
public:
  /**
   * Constructor.
   *
   * @param task Task to initialse solver for
   * @param V Speed (m/s)
   *
   * @return Initialised object (not solved)
   */
  AverageSpeedSolver(const double dwcostheta, const double wind_speed_squared,
                     const double V)
    :Quadratic(dwcostheta, wind_speed_squared - Square(V)) {}

  /**
   * Find ground speed from task and wind
   *
   * @return Ground speed during cruise (m/s)
   */
  [[gnu::pure]]
  double Solve() const {
    if (Check())
      /// @todo check this is correct for all theta
      return SolutionMax();

    return -1;
  }
};

double
GlideState::CalcAverageSpeed(const double Veff) const
{
  if (wind.IsNonZero()) {
    // only need to solve if positive wind speed
    return AverageSpeedSolver(2 * head_wind, wind_speed_squared, Veff).Solve();
  }

  return Veff;
}

// dummy task
GlideState::GlideState(const GeoVector &vector, const double htarget,
                       double altitude, const SpeedVector wind)
  :vector(vector),
   min_arrival_altitude(htarget),
   altitude_difference(altitude - min_arrival_altitude)
{
  CalcSpeedups(wind);
}

void
GlideState::CalcSpeedups(const SpeedVector _wind)
{
  if (_wind.IsNonZero()) {
    wind = _wind;
    effective_wind_angle = wind.bearing.Reciprocal() - vector.bearing;
    wind_speed_squared = Square(wind.norm);
    head_wind = -wind.norm * effective_wind_angle.cos();
  } else {
    wind = SpeedVector::Zero();
    effective_wind_angle = Angle::Zero();
    head_wind = 0;
    wind_speed_squared = 0;
  }
}

double
GlideState::DriftedDistance(const FloatDuration time) const
{
  if (wind.IsZero())
    return vector.distance;

  // Distance that the wine travels in the given #time
  const auto distance_wind = wind.norm * time.count();
  // Direction of the wind
  auto sc_wind = wind.bearing.Reciprocal().SinCos();
  const auto sin_wind = sc_wind.first, cos_wind = sc_wind.second;

  // Distance to the target
  const auto distance_task = vector.distance;
  // Direction to the target
  auto sc_task = vector.bearing.SinCos();
  const auto sin_task = sc_task.first, cos_task = sc_task.second;

  // X-/Y-Components of the resulting vector
  const auto dx = distance_task * sin_task - distance_wind * sin_wind;
  const auto dy = distance_task * cos_task - distance_wind * cos_wind;

  return hypot(dx, dy);

  // ??   task.Bearing = RAD_TO_DEG*(atan2(dx,dy));
}
