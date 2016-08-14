/*
Copyright_License {

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

#include "FlightStatistics.hpp"

void FlightStatistics::Reset() {
  ScopeLock lock(mutex);

  thermal_average.Reset();
  altitude.Reset();
  altitude_base.Reset();
  altitude_ceiling.Reset();
  task_speed.Reset();
  altitude_terrain.Reset();
  vario_circling_histogram.Reset(-7.5,7.5);
  vario_cruise_histogram.Reset(-7.5,7.5);
}

void
FlightStatistics::StartTask()
{
  ScopeLock lock(mutex);
  // JMW clear thermal climb average on task start
  //  thermal_average.Reset();
  vario_circling_histogram.Clear();
  vario_cruise_histogram.Clear();
}

void
FlightStatistics::AddAltitudeTerrain(const double tflight, const double terrainalt)
{
  ScopeLock lock(mutex);
  altitude_terrain.Update(std::max(0., tflight / 3600.),
                          terrainalt);
}

void
FlightStatistics::AddAltitude(const double tflight, const double alt, const bool final_glide)
{
  ScopeLock lock(mutex);

  const double t = std::max(0., tflight / 3600);

  altitude.Update(t, alt);

  // update working ceiling immediately if above
  if (!altitude_ceiling.IsEmpty() && (alt > altitude_ceiling.GetLastY()))
    altitude_ceiling.UpdateConvexPositive(t, alt);

  // update working base immediately if below and not in final glide
  if (!altitude_base.IsEmpty() && (alt < altitude_base.GetLastY()) && !final_glide)
    altitude_base.UpdateConvexNegative(t, alt);
}

double
FlightStatistics::AverageThermalAdjusted(const double mc_current,
                                         const bool circling)
{
  ScopeLock lock(mutex);

  double mc_stats;
  if (! thermal_average.IsEmpty() && (thermal_average.GetAverageY() > 0)) {
    if (mc_current > 0 && circling)
      mc_stats = (thermal_average.GetCount() * thermal_average.GetAverageY() + mc_current) /
        (thermal_average.GetCount() + 1);
    else
      mc_stats = thermal_average.GetAverageY();
  } else {
    mc_stats = mc_current;
  }

  return mc_stats;
}

void
FlightStatistics::AddTaskSpeed(const double tflight, const double val)
{
  ScopeLock lock(mutex);
  task_speed.Update(tflight / 3600, val);
}

void
FlightStatistics::AddClimbBase(const double tflight, const double alt)
{
  ScopeLock lock(mutex);

  // only add base after finished second climb, to avoid having the takeoff height
  // as the base
  //
  if (altitude_ceiling.HasResult())
    altitude_base.UpdateConvexNegative(std::max(0., tflight) / 3600, alt);
}

void
FlightStatistics::AddClimbCeiling(const double tflight, const double alt)
{
  ScopeLock lock(mutex);
  altitude_ceiling.UpdateConvexPositive(std::max(0., tflight) / 3600, alt);
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const double tflight_start,
                                    const double tflight_end, const double v)
{
  ScopeLock lock(mutex);
  thermal_average.Update(std::max(0., tflight_start) / 3600, v,
                         (tflight_end-tflight_start)/3600);
}

void
FlightStatistics::AddClimbRate(const double tflight, const double vario, const bool circling)
{
  ScopeLock lock(mutex);
  if (circling) {
    vario_circling_histogram.UpdateHistogram(vario);
  } else {
    vario_cruise_histogram.UpdateHistogram(vario);
  }
}

double
FlightStatistics::GetMinWorkingHeight() const
{
  if (altitude_base.IsEmpty())
    return 0;

  // working height is average base less one standard deviation, or
  // the minimum encountered if this is higher
  return std::max(altitude_base.GetMinY(), altitude_base.GetAverageY() - sqrt(altitude_base.GetVarY()));
}

double
FlightStatistics::GetMaxWorkingHeight() const
{
  if (altitude_ceiling.IsEmpty())
    return 0;

  // working height is average ceiling plus one standard deviation, or
  // the maximum encountered if this is lower
  return std::max(altitude.GetMaxY(),
                  std::min(altitude_ceiling.GetMaxY(), altitude_ceiling.GetAverageY() + sqrt(altitude_ceiling.GetVarY())));
}

// percentile to look up to determine max/min value
static constexpr double PERCENTILE_VARIO = 0.1;

double
FlightStatistics::GetVarioScalePositive() const
{
  return std::max(vario_circling_histogram.GetPercentile(1-PERCENTILE_VARIO),
                  vario_cruise_histogram.GetPercentile(1-PERCENTILE_VARIO));
}

double
FlightStatistics::GetVarioScaleNegative() const
{
  return std::min(vario_circling_histogram.GetPercentile(PERCENTILE_VARIO),
                  vario_cruise_histogram.GetPercentile(PERCENTILE_VARIO));
}
