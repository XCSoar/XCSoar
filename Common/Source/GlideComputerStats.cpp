/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000 - 2009

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

#include "GlideComputer.hpp"
#include "McReady.h"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"
#include "Logger.h"
#include "Math/Earth.hpp"
#include "GPSClock.hpp"


GlideComputerStats::GlideComputerStats():
 log_clock(5.0),
 stats_clock(60.0)
{

}


void GlideComputerStats::ResetFlight(const bool full)
{
  FastLogNum = 0; 
  if (full) {
    flightstats.Reset();
  }
}

void GlideComputerStats::StartTask() {
  flightstats.StartTask(Basic().Time);
}


bool GlideComputerStats::DoLogging() {

  // prevent bad fixes from being logged or added to OLC store
  double distance;
  DistanceBearing(Basic().Latitude, Basic().Longitude,
		  LastBasic().Latitude, LastBasic().Longitude,
		  &distance, NULL);

  if (distance>200.0) {
    return false;
  }

  // draw snail points more often in circling mode
  if (Calculated().Circling) {
    log_clock.set_dt(SettingsComputer().LoggerTimeStepCircling);
    snail_trail.clock.set_dt(1.0);
  } else {
    log_clock.set_dt(SettingsComputer().LoggerTimeStepCruise);
    snail_trail.clock.set_dt(5.0);
  }
  if (FastLogNum) {
    log_clock.set_dt(1.0);
    FastLogNum--;
  }

  if (log_clock.check_advance(Basic().Time)) {
    LogPoint(Basic());
  }

  /* JMW TODO update this code incomplete

  static GPSClock frecord_clock(270.0); // 4.5 minutes (required
					// minimum every 5)

  if (Basic().Time - GetFRecordLastTime() >= dtFRecord)
  {
    if (LogFRecord(Basic().SatelliteIDs,false))
    {  // need F record every 5 minutes so if write fails or
       // constellation is invalid, don't update timer and try again
       // next cycle
      SetFRecordLastTime(GetFRecordLastTime() + dtFRecord);
      // the FRecordLastTime is reset when the logger restarts so it
      // is always at the start of the file
      if (GetFRecordLastTime() < Basic().Time-dtFRecord)
        SetFRecordLastTime(Basic().Time-dtFRecord);
    }
  }
  */

  if (Calculated().Flying) {
    if (snail_trail.clock.check_advance(Basic().Time)) {
      snail_trail.AddPoint(&Basic(), &Calculated());
    }
    if (stats_clock.check_advance(Basic().Time)) {
      flightstats.AddAltitudeTerrain(Basic().Time-Calculated().TakeOffTime,
				     Calculated().TerrainAlt);
      flightstats.AddAltitude(Basic().Time-Calculated().TakeOffTime,
			      Calculated().NavAltitude);
    }
  }
  return true;
}


double GlideComputerStats::GetAverageThermal() 
{
  double mc_current;

  mc_current = GlideComputerBlackboard::GetAverageThermal();
  return flightstats.AverageThermalAdjusted(mc_current,
					    Calculated().Circling);
}



void GlideComputerStats::SaveTaskSpeed(double val) 
{
  flightstats.SaveTaskSpeed(val);
}

void GlideComputerStats::SetLegStart() 
{
  flightstats.SetLegStart(ActiveWayPoint, Basic().Time);
}


void
GlideComputerStats::OnClimbBase(double StartAlt)
{
  flightstats.AddClimbBase(Calculated().ClimbStartTime
			   - Calculated().TakeOffTime, StartAlt);
}


void
GlideComputerStats::OnClimbCeiling()
{
  flightstats.AddClimbCeiling(Calculated().CruiseStartTime
			      -Calculated().TakeOffTime,
			      Calculated().CruiseStartAlt);
}


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
