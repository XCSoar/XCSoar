/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

	M Roberts (original release)
	Robin Birch <robinb@ruffnready.co.uk>
	Samuel Gisiger <samuel.gisiger@triadis.ch>
	Jeff Goodenough <jeff@enborne.f2s.com>
	Alastair Harrison <aharrison@magic.force9.co.uk>
	Scott Penrose <scottp@dd.com.au>
	John Wharington <jwharington@gmail.com>
	Lars H <lars_hn@hotmail.com>
	Rob Dunning <rob@raspberryridgesheepfarm.com>
	Russell King <rmk@arm.linux.org.uk>
	Paolo Ventafridda <coolwind@email.it>
	Tobias Lohner <tobias@lohner-net.de>
	Mirek Jezek <mjezek@ipplc.cz>
	Max Kellermann <max@duempel.org>
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "Logger.hpp"
#include "Math/Earth.hpp"
#include "GPSClock.hpp"
#ifdef OLD_TASK
#include "Task.h"
#endif

GlideComputerStats::GlideComputerStats() :
  log_clock(5.0),
  stats_clock(60.0)
{

}

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
  if (Distance(Basic().Location, LastBasic().Location) > 200.0)
    // prevent bad fixes from being logged or added to OLC store
    return false;

  // log points more often in circling mode
  if (Calculated().Circling) {
    log_clock.set_dt(SettingsComputer().LoggerTimeStepCircling);
  } else {
    log_clock.set_dt(SettingsComputer().LoggerTimeStepCruise);
  }

  if (FastLogNum) {
    log_clock.set_dt(1.0);
    FastLogNum--;
  }

  if (log_clock.check_advance(Basic().Time))
    logger.LogPoint(Basic());

  if (Basic().flight.Flying) {
    if (stats_clock.check_advance(Basic().Time)) {
      flightstats.AddAltitudeTerrain(Basic().flight.FlightTime,
          Calculated().TerrainAlt);
      flightstats.AddAltitude(Basic().flight.FlightTime,
                              Basic().NavAltitude);
      flightstats.SaveTaskSpeed(Calculated().task_stats.total.remaining_effective.get_speed_incremental());
    }
  }

  return true;
}

double
GlideComputerStats::GetAverageThermal()
{
  double mc_current;

  mc_current = GlideComputerBlackboard::GetAverageThermal();
  return flightstats.AverageThermalAdjusted(mc_current, Calculated().Circling);
}

void
GlideComputerStats::OnClimbBase(double StartAlt)
{
  flightstats.AddClimbBase(Calculated().ClimbStartTime
                           - Basic().flight.TakeOffTime, StartAlt);
}

void
GlideComputerStats::OnClimbCeiling()
{
  flightstats.AddClimbCeiling(Calculated().CruiseStartTime
                              - Basic().flight.TakeOffTime,
                              Calculated().CruiseStartAlt);
}

/**
 * This function is called when leaving a thermal and handles the
 * calculation of all related statistics
 */
void
GlideComputerStats::OnDepartedThermal()
{
  flightstats.AddThermalAverage(Calculated().LastThermalAverage);
}

void
GlideComputerStats::Initialise()
{

}

void
GlideComputerStats::SetFastLogging()
{
  FastLogNum = 5;
}
