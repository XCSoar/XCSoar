/* Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2013 The XCSoar Project
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

#include "GlideState.hpp"
#include "Math/Quadratic.hpp"

/**
 * Quadratic function solver for MacCready theory constraint equation
 * 
 * - document this equation!
 */
class AverageSpeedSolver: public Quadratic
{
public:
  /**
   * Constructor.
   *
   * @param task Task to initialse solver for
   * @param V Speed (m/s)
   *
   * @return Initialised object (not solved)
   */
  AverageSpeedSolver(const fixed dwcostheta, const fixed wind_speed_squared,
                     const fixed V) :
    Quadratic(dwcostheta, wind_speed_squared - sqr(V))
  {
  }

  /**
   * Find ground speed from task and wind
   *
   * @return Ground speed during cruise (m/s)
   */
  gcc_pure
  fixed
  Solve() const
  {
    if (Check())
      /// @todo check this is correct for all theta
      return SolutionMax();

    return -fixed_one;
  }
};

fixed
GlideState::CalcAverageSpeed(const fixed Veff) const
{
  if (wind.IsNonZero()) {
    // only need to solve if positive wind speed
    return AverageSpeedSolver(Double(head_wind), wind_speed_squared, Veff).Solve();
  }

  return Veff;
}

// dummy task
GlideState::GlideState(const GeoVector &vector, const fixed htarget,
                       fixed altitude, const SpeedVector wind) :
  vector(vector),
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
    wind_speed_squared = sqr(wind.norm);
    head_wind = -wind.norm * effective_wind_angle.cos();
  } else {
    wind = SpeedVector::Zero();
    effective_wind_angle = Angle::Zero();
    head_wind = fixed_zero;
    wind_speed_squared = fixed_zero;
  }
}

fixed
GlideState::DriftedDistance(const fixed time) const
{
  if (wind.IsZero())
    return vector.distance;

  // Distance that the wine travels in the given #time
  const fixed distance_wind = wind.norm * time;
  // Direction of the wind
  auto sc_wind = wind.bearing.Reciprocal().SinCos();
  const fixed sin_wind = sc_wind.first, cos_wind = sc_wind.second;

  // Distance to the target
  const fixed distance_task = vector.distance;
  // Direction to the target
  auto sc_task = vector.bearing.SinCos();
  const fixed sin_task = sc_task.first, cos_task = sc_task.second;

  // X-/Y-Components of the resulting vector
  const fixed dx = distance_task * sin_task - distance_wind * sin_wind;
  const fixed dy = distance_task * cos_task - distance_wind * cos_wind;

  return MediumHypot(dx, dy);

  // ??   task.Bearing = RAD_TO_DEG*(atan2(dx,dy));
}
