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
#include "GlideState.hpp"
#include <math.h>
#include "Util/Quadratic.hpp"
#include "Navigation/SpeedVector.hpp"

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
    Quadratic(dwcostheta, wind_speed_squared - V * V)
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
  if (wind.is_non_zero()) {
    // only need to solve if positive wind speed
    return AverageSpeedSolver(head_wind_doubled, wind_speed_squared, Veff).Solve();
  }

  return Veff;
}

// dummy task
GlideState::GlideState(const GeoVector &vector, const fixed htarget,
                       fixed altitude, const SpeedVector wind) :
  vector(vector),
  min_height(htarget),
  altitude_difference(altitude - min_height)
{
  CalcSpeedups(wind);
}

void
GlideState::CalcSpeedups(const SpeedVector _wind)
{
  if (_wind.is_non_zero()) {
    wind = _wind;
    effective_wind_angle = wind.bearing.Reciprocal() - vector.Bearing;
    wind_speed_squared = wind.norm * wind.norm;
    head_wind = -wind.norm * effective_wind_angle.cos();
    head_wind_doubled = fixed_two * head_wind;
  } else {
    wind.bearing = Angle::zero();
    wind.norm = fixed_zero;
    effective_wind_angle = Angle::zero();
    head_wind = fixed_zero;
    wind_speed_squared = fixed_zero;
    head_wind_doubled = fixed_zero;
  }
}

fixed
GlideState::DriftedDistance(const fixed t_cl) const
{
  if (wind.is_zero())
    return vector.Distance;

  const Angle wd = wind.bearing.Reciprocal();
  fixed sinwd, coswd;
  wd.sin_cos(sinwd, coswd);

  const Angle tb = vector.Bearing;
  fixed sintb, costb;
  tb.sin_cos(sintb, costb);

  const fixed aw = wind.norm * t_cl;
  const fixed dx = vector.Distance * sintb - aw * sinwd;
  const fixed dy = vector.Distance * costb - aw * coswd;

  return hypot(dx, dy);

  // ??   task.Bearing = RAD_TO_DEG*(atan2(dx,dy));
}
