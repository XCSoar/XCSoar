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
  flightstats.LegStartTime[0] = gps_info.Time;
  flightstats.LegStartTime[1] = gps_info.Time;
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

  if(gps_info.Time <= LogLastTime) {
    LogLastTime = gps_info.Time;
  }
  if(gps_info.Time <= StatsLastTime) {
    StatsLastTime = gps_info.Time;
  }
  if(gps_info.Time <= GetFRecordLastTime()) {
    SetFRecordLastTime(gps_info.Time);
  }

  // draw snail points more often in circling mode
  if (calculated_info.Circling) {
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

  DistanceBearing(gps_info.Latitude, gps_info.Longitude,
		  Latitude_last, Longitude_last,
		  &distance, NULL);
  Latitude_last = gps_info.Latitude;
  Longitude_last = gps_info.Longitude;

  if (distance>200.0) {
    return false;
  }

  if (gps_info.Time - LogLastTime >= dtLog) {
    double balt = -1;
    if (gps_info.BaroAltitudeAvailable) {
      balt = gps_info.BaroAltitude;
    } else {
      balt = gps_info.Altitude;
    }
    LogPoint(gps_info.Latitude , gps_info.Longitude , gps_info.Altitude,
             balt);
    LogLastTime += dtLog;
    if (LogLastTime< gps_info.Time-dtLog) {
      LogLastTime = gps_info.Time-dtLog;
    }
    if (FastLogNum) FastLogNum--;
  }

  if (gps_info.Time - GetFRecordLastTime() >= dtFRecord)
  {
    if (LogFRecord(gps_info.SatelliteIDs,false))
    {  // need F record every 5 minutes so if write fails or
       // constellation is invalid, don't update timer and try again
       // next cycle
      SetFRecordLastTime(GetFRecordLastTime() + dtFRecord);
      // the FRecordLastTime is reset when the logger restarts so it
      // is always at the start of the file
      if (GetFRecordLastTime() < gps_info.Time-dtFRecord)
        SetFRecordLastTime(gps_info.Time-dtFRecord);
    }
  }

  if (snail_trail.CheckAdvance(gps_info.Time, dtSnail)) {
    mutexGlideComputer.Lock();
    snail_trail.AddPoint(&gps_info, &calculated_info);
    mutexGlideComputer.Unlock();
  }

  if (calculated_info.Flying) {
    if (gps_info.Time - StatsLastTime >= dtStats) {

      mutexGlideComputer.Lock();
      flightstats.Altitude_Terrain.
        least_squares_update(max(0,
                                 gps_info.Time-calculated_info.TakeOffTime)/3600.0,
                             calculated_info.TerrainAlt);

      flightstats.Altitude.
        least_squares_update(max(0,
                                 gps_info.Time-calculated_info.TakeOffTime)/3600.0,
                             calculated_info.NavAltitude);
      mutexGlideComputer.Unlock();

      StatsLastTime += dtStats;
      if (StatsLastTime< gps_info.Time-dtStats) {
        StatsLastTime = gps_info.Time-dtStats;
      }
    }
  }
  return true;
}


double GlideComputerStats::GetAverageThermal() {
  double mc_val;
  mutexGlideComputer.Lock();
  if (flightstats.ThermalAverage.y_ave>0) {
    mc_val = flightstats.ThermalAverage.y_ave;
  } else {
    mc_val = GlideComputerBlackboard::GetAverageThermal();
  }
  mutexGlideComputer.Unlock();
  return mc_val;
}



void GlideComputerStats::SaveTaskSpeed(double val) 
{
  mutexGlideComputer.Lock();
  flightstats.Task_Speed.least_squares_update(val);
  mutexGlideComputer.Unlock();
}
