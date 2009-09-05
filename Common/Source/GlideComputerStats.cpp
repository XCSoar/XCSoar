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

int FastLogNum = 0; // number of points to log at high rate
int LoggerTimeStepCruise=5;
int LoggerTimeStepCircling=1;

void GlideComputerStats::ResetFlight(const bool full)
{
  if (full) {
    flightstats.Reset();
  }
}

void GlideComputerStats::StartTask() {
  flightstats.LegStartTime[0] = Basic().Time;
  flightstats.LegStartTime[1] = Basic().Time;
  // JMW clear thermal climb average on task start
  flightstats.ThermalAverage.Reset();
  flightstats.Task_Speed.Reset();
}


bool GlideComputerStats::DoLogging() {
  static double LogLastTime=0;
  static double StatsLastTime=0;
  double dtLog = 5.0;
  double dtSnail = 2.0;
  double dtStats = 60.0;
  double dtFRecord = 270; // 4.5 minutes (required minimum every 5)

  if(Basic().Time <= LogLastTime) {
    LogLastTime = Basic().Time;
  }
  if(Basic().Time <= StatsLastTime) {
    StatsLastTime = Basic().Time;
  }
  if(Basic().Time <= GetFRecordLastTime()) {
    SetFRecordLastTime(Basic().Time);
  }

  // draw snail points more often in circling mode
  if (Calculated().Circling) {
    dtLog = LoggerTimeStepCircling;
    dtSnail = 1.0;
  } else {
    dtLog = LoggerTimeStepCruise;
    dtSnail = 5.0;
  }
  if (FastLogNum) {
    dtLog = 1.0;
  }

  // prevent bad fixes from being logged or added to OLC store
  static double Longitude_last = 10;
  static double Latitude_last = 10;
  double distance;

  DistanceBearing(Basic().Latitude, Basic().Longitude,
		  Latitude_last, Longitude_last,
		  &distance, NULL);
  Latitude_last = Basic().Latitude;
  Longitude_last = Basic().Longitude;

  if (distance>200.0) {
    return false;
  }

  if (Basic().Time - LogLastTime >= dtLog) {
    double balt = -1;
    if (Basic().BaroAltitudeAvailable) {
      balt = Basic().BaroAltitude;
    } else {
      balt = Basic().Altitude;
    }
    LogPoint(Basic().Latitude , Basic().Longitude , Basic().Altitude,
             balt);
    LogLastTime += dtLog;
    if (LogLastTime< Basic().Time-dtLog) {
      LogLastTime = Basic().Time-dtLog;
    }
    if (FastLogNum) FastLogNum--;
  }

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

  if (snail_trail.CheckAdvance(Basic().Time, dtSnail)) {
    mutexGlideComputer.Lock();
    snail_trail.AddPoint(&Basic(), &Calculated());
    mutexGlideComputer.Unlock();
  }

  if (Calculated().Flying) {
    if (Basic().Time - StatsLastTime >= dtStats) {

      mutexGlideComputer.Lock();
      flightstats.Altitude_Terrain.
        least_squares_update(max(0,
                                 Basic().Time-Calculated().TakeOffTime)/3600.0,
                             Calculated().TerrainAlt);

      flightstats.Altitude.
        least_squares_update(max(0,
                                 Basic().Time-Calculated().TakeOffTime)/3600.0,
                             Calculated().NavAltitude);
      mutexGlideComputer.Unlock();

      StatsLastTime += dtStats;
      if (StatsLastTime< Basic().Time-dtStats) {
        StatsLastTime = Basic().Time-dtStats;
      }
    }
  }
  return true;
}


double GlideComputerStats::GetAverageThermal() const
{
  double mc_stats, mc_current;
  ScopeLock protect(mutexGlideComputer);

  mc_current = GlideComputerBlackboard::GetAverageThermal();
  if (flightstats.ThermalAverage.y_ave>0) {
    if ((mc_current>0) && Calculated().Circling) {
      mc_stats = (flightstats.ThermalAverage.sum_n*flightstats.ThermalAverage.y_ave
		  +mc_current)/(flightstats.ThermalAverage.sum_n+1);
    } else {
      mc_stats = flightstats.ThermalAverage.y_ave;
    }
  } else {
    mc_stats = mc_current;
  }
  return mc_stats;
}



void GlideComputerStats::SaveTaskSpeed(double val) 
{
  mutexGlideComputer.Lock();
  flightstats.Task_Speed.least_squares_update(val);
  mutexGlideComputer.Unlock();
}

void GlideComputerStats::SetLegStart() 
{
  mutexGlideComputer.Lock();
  if (flightstats.LegStartTime[ActiveWayPoint]<0) {
    flightstats.LegStartTime[ActiveWayPoint] = Basic().Time;
  }
  mutexGlideComputer.Unlock();
}


void
GlideComputerStats::OnClimbBase(double StartAlt)
{
  ScopeLock protect(mutexGlideComputer);
  if (flightstats.Altitude_Ceiling.sum_n>0) {
    // only update base if have already climbed, otherwise
    // we will catch the takeoff height as the base.
    
    flightstats.Altitude_Base.
      least_squares_update(max(0,Calculated().ClimbStartTime
			       - Calculated().TakeOffTime)/3600.0,
			   StartAlt);
  }
}


void
GlideComputerStats::OnClimbCeiling()
{
  ScopeLock protect(mutexGlideComputer);
  flightstats.Altitude_Ceiling.
    least_squares_update(max(0,Calculated().CruiseStartTime
			     - Calculated().TakeOffTime)/3600.0,
			 Calculated().CruiseStartAlt);
}


void
GlideComputerStats::OnDepartedThermal()
{
  ScopeLock protect(mutexGlideComputer);
  flightstats.ThermalAverage.
    least_squares_update(Calculated().LastThermalAverage);

#ifdef DEBUG_STATS
  DebugStore("%f %f # thermal stats\n",
	     flightstats.ThermalAverage.m,
	     flightstats.ThermalAverage.b
	     );
#endif
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
