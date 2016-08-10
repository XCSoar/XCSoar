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

#include "StatsComputer.hpp"
#include "NMEA/MoreData.hpp"
#include "NMEA/Derived.hpp"

void
StatsComputer::ResetFlight(const bool full)
{
  last_location = GeoPoint::Invalid();
  last_climb_start_time = -1;
  last_cruise_start_time = -1;
  last_thermal_end_time = -1;

  if (full)
    flightstats.Reset();
}

void
StatsComputer::StartTask(const NMEAInfo &basic)
{
  flightstats.StartTask();
}

/**
 * Logs GPS fixes for stats
 * @return True if valid fix (fix distance <= 200m), False otherwise
 */
bool
StatsComputer::DoLogging(const MoreData &basic,
                              const DerivedInfo &calculated)
{
  /// @todo consider putting this sanity check inside Parser
  bool location_jump = basic.location_available && last_location.IsValid() &&
    basic.location.DistanceS(last_location) > 200;

  last_location = basic.location_available
    ? basic.location : GeoPoint::Invalid();

  if (location_jump || !basic.location_available)
    // prevent bad fixes from being logged or added to OLC store
    return false;

  if (calculated.flight.flying &&
      stats_clock.CheckAdvance(basic.time, PERIOD)) {
    flightstats.AddAltitudeTerrain(calculated.flight.flight_time,
                                   calculated.terrain_altitude);

    if (basic.NavAltitudeAvailable())
      flightstats.AddAltitude(calculated.flight.flight_time,
                              basic.nav_altitude,
                              calculated.task_stats.flight_mode_final_glide);

    if (calculated.task_stats.task_valid &&
        calculated.task_stats.inst_speed_slow >= 0)
      flightstats.AddTaskSpeed(calculated.flight.flight_time,
                               calculated.task_stats.inst_speed_slow);
  }

  if (calculated.flight.flying) {
    flightstats.AddClimbRate(calculated.flight.flight_time,
                             calculated.average,
                             calculated.turn_mode == CirclingMode::CLIMB);
  }

  return true;
}

void
StatsComputer::OnClimbBase(const DerivedInfo &calculated)
{
  // nothing to do here now
}

void
StatsComputer::OnClimbCeiling(const DerivedInfo &calculated)
{
  // nothing to do here now
}

/**
 * This function is called when leaving a thermal and handles the
 * calculation of all related statistics
 */
void
StatsComputer::OnDepartedThermal(const DerivedInfo &calculated)
{
  assert(calculated.last_thermal.IsDefined());

  auto t_start = calculated.last_thermal.start_time - calculated.flight.takeoff_time;
  auto t_end = calculated.last_thermal.end_time - calculated.flight.takeoff_time;

  flightstats.AddThermalAverage(t_start, t_end, calculated.last_thermal.lift_rate);

  // ignore failed climbs
  if (calculated.last_thermal.gain<= 0)
    return;

  flightstats.AddClimbCeiling(t_end,
                              calculated.last_thermal.gain
                              + calculated.last_thermal.start_altitude);

  flightstats.AddClimbBase(t_start, calculated.last_thermal.start_altitude);
}

void
StatsComputer::ProcessClimbEvents(const DerivedInfo &calculated)
{
  switch (calculated.turn_mode) {
  case CirclingMode::CLIMB:
    if (calculated.climb_start_time > last_climb_start_time)
      // set altitude for start of circling (as base of climb)
      OnClimbBase(calculated);
    break;

  case CirclingMode::CRUISE:
    if (calculated.cruise_start_time > last_cruise_start_time)
      OnClimbCeiling(calculated);
    break;

  default:
    break;
  }

  if (calculated.last_thermal.IsDefined() &&
      (last_thermal_end_time < 0 ||
       calculated.last_thermal.end_time > last_thermal_end_time))
    OnDepartedThermal(calculated);

  last_climb_start_time = calculated.climb_start_time;
  last_cruise_start_time = calculated.cruise_start_time;
  last_thermal_end_time = calculated.last_thermal.IsDefined()
    ? calculated.last_thermal.end_time
    : -1.;
}
