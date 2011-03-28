/*
Copyright_License {

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

#include "FlightStatistics.hpp"

void FlightStatistics::Reset() {
  ScopeLock lock(mutexStats);

  ThermalAverage.Reset();
  Altitude.Reset();
  Altitude_Base.Reset();
  Altitude_Ceiling.Reset();
  Task_Speed.Reset();
  Altitude_Terrain.Reset();
}

void
FlightStatistics::StartTask()
{
  ScopeLock lock(mutexStats);
  // JMW clear thermal climb average on task start
  ThermalAverage.Reset();
  Task_Speed.Reset();
}

void
FlightStatistics::AddAltitudeTerrain(const fixed tflight, const fixed terrainalt)
{
  ScopeLock lock(mutexStats);
  Altitude_Terrain.LeastSquaresUpdate(max(fixed_zero, tflight / 3600),
                                      terrainalt);
}

void
FlightStatistics::AddAltitude(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);
  Altitude.LeastSquaresUpdate(max(fixed_zero, tflight / 3600), alt);
}

fixed
FlightStatistics::AverageThermalAdjusted(const fixed mc_current,
                                         const bool circling)
{
  ScopeLock lock(mutexStats);

  fixed mc_stats;
  if (ThermalAverage.y_ave > fixed_zero) {
    if (mc_current > fixed_zero && circling)
      mc_stats = (ThermalAverage.sum_n * ThermalAverage.y_ave + mc_current) /
                 (ThermalAverage.sum_n + 1);
    else
      mc_stats = ThermalAverage.y_ave;
  } else {
    mc_stats = mc_current;
  }

  return mc_stats;
}

void
FlightStatistics::AddTaskSpeed(const fixed tflight, const fixed val)
{
  ScopeLock lock(mutexStats);
  Task_Speed.LeastSquaresUpdate(tflight / 3600, std::max(fixed_zero,val));
}

void
FlightStatistics::AddClimbBase(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);

  if (Altitude_Ceiling.sum_n > 0)
    // only update base if have already climbed, otherwise
    // we will catch the takeoff height as the base.
    Altitude_Base.LeastSquaresUpdate(max(fixed_zero, tflight) / 3600, alt);
}

void
FlightStatistics::AddClimbCeiling(const fixed tflight, const fixed alt)
{
  ScopeLock lock(mutexStats);
  Altitude_Ceiling.LeastSquaresUpdate(max(fixed_zero, tflight) / 3600, alt);
}

/**
 * Adds a thermal to the ThermalAverage calculator
 * @param v Average climb speed of the last thermal
 */
void
FlightStatistics::AddThermalAverage(const fixed v)
{
  ScopeLock lock(mutexStats);
  ThermalAverage.LeastSquaresUpdate(v);
}
