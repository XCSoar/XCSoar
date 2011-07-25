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

#include "GlideComputerStats.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Logger/Logger.hpp"
#include "Math/Earth.hpp"
#include "GPSClock.hpp"

GlideComputerStats::GlideComputerStats() :
  log_clock(fixed(5)),
  stats_clock(fixed(60)),
  logger(NULL) {}

void
GlideComputerStats::ResetFlight(const bool full)
{
  FastLogNum = 0;
  if (full)
    flightstats.Reset();
}

void
GlideComputerStats::StartTask()
{
  flightstats.StartTask();
}

/**
 * Logs GPS fixes for stats
 * @return True if valid fix (fix distance <= 200m), False otherwise
 */
bool
GlideComputerStats::DoLogging()
{
  /// @todo consider putting this sanity check inside Parser
  if (Distance(Basic().location, LastBasic().location) > fixed(200))
    // prevent bad fixes from being logged or added to OLC store
    return false;

  // log points more often in circling mode
  if (Calculated().circling)
    log_clock.set_dt(fixed(SettingsComputer().LoggerTimeStepCircling));
  else
    log_clock.set_dt(fixed(SettingsComputer().LoggerTimeStepCruise));

  if (FastLogNum) {
    log_clock.set_dt(fixed_one);
    FastLogNum--;
  }

  if (log_clock.check_advance(Basic().time) && logger != NULL)
      logger->LogPoint(Basic());

  if (Calculated().flight.flying &&
      stats_clock.check_advance(Basic().time)) {
    flightstats.AddAltitudeTerrain(Calculated().flight.flight_time,
                                   Calculated().terrain_altitude);
    flightstats.AddAltitude(Calculated().flight.flight_time,
                            Basic().NavAltitude);
    flightstats.AddTaskSpeed(Calculated().flight.flight_time,
                             Calculated().task_stats.get_pirker_speed());
  }

  return true;
}

void
GlideComputerStats::OnClimbBase(fixed StartAlt)
{
  flightstats.AddClimbBase(Calculated().climb_start_time -
                           Calculated().flight.takeoff_time, StartAlt);
}

void
GlideComputerStats::OnClimbCeiling()
{
  flightstats.AddClimbCeiling(Calculated().cruise_start_time -
                              Calculated().flight.takeoff_time,
                              Calculated().cruise_start_altitude);
}

/**
 * This function is called when leaving a thermal and handles the
 * calculation of all related statistics
 */
void
GlideComputerStats::OnDepartedThermal()
{
  assert(Calculated().last_thermal.IsDefined());

  flightstats.AddThermalAverage(Calculated().last_thermal.lift_rate);
}

void
GlideComputerStats::ProcessClimbEvents()
{
  if (time_retreated())
    return;

  const DerivedInfo &calculated = Calculated();
  const DerivedInfo &last_calculated = LastCalculated();

  switch (calculated.turn_mode) {
  case CLIMB:
    if (calculated.climb_start_time > last_calculated.climb_start_time)
      // set altitude for start of circling (as base of climb)
      OnClimbBase(calculated.turn_start_altitude);
    break;

  case CRUISE:
    if (calculated.cruise_start_time > last_calculated.cruise_start_time)
      OnClimbCeiling();
    break;

  default:
    break;
  }

  if (calculated.last_thermal.IsDefined() &&
      (!last_calculated.last_thermal.IsDefined() ||
       calculated.last_thermal.end_time > last_calculated.last_thermal.end_time))
    OnDepartedThermal();
}

void
GlideComputerStats::SetFastLogging()
{
  FastLogNum = 5;
}
