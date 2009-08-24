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

#include "Calculations2.h"
#include "Calculations.h"
#include "XCSoar.h"
#include "Dialogs.h"
#include "Utils.h"
#include "SettingsTask.hpp"
#include "externs.h"
#include "McReady.h"
#include "Airspace.h"
#include "Logger.h"
#include <windows.h>
#include <math.h>
#include "InputEvents.h"
#include "Message.h"
#include "RasterTerrain.h"
#include "TeamCodeCalculation.h"
#include "Process.h"
#include "Math/Earth.hpp"
#include "PeriodClock.hpp"
#include "Math/Pressure.h"
#include "WayPoint.hpp"
#include "SnailTrail.hpp"
#include "LogFile.hpp"

#include <tchar.h>

#include "ThermalLocator.h"
#include "windanalyser.h"
#include "Atmosphere.h"

#include "Audio/VegaVoice.h"

#include "OnLineContest.h"
#include "AATDistance.h"

#include "NavFunctions.h" // used for team code

#include <stdio.h>

extern OLCOptimizer olc;

int FastLogNum = 0; // number of points to log at high rate

void AddSnailPoint(NMEA_INFO *Basic, DERIVED_INFO *Calculated); // from SnailTrail.cpp


int LoggerTimeStepCruise=5;
int LoggerTimeStepCircling=1;

void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static double SnailLastTime=0;
  static double LogLastTime=0;
  static double StatsLastTime=0;
  static double OLCLastTime = 0;
  double dtLog = 5.0;
  double dtSnail = 2.0;
  double dtStats = 60.0;
  double dtOLC = 5.0;
  double dtFRecord = 270; // 4.5 minutes (required minimum every 5)

  if(Basic->Time <= LogLastTime) {
    LogLastTime = Basic->Time;
  }
  if(Basic->Time <= SnailLastTime)  {
    SnailLastTime = Basic->Time;
  }
  if(Basic->Time <= StatsLastTime) {
    StatsLastTime = Basic->Time;
  }
  if(Basic->Time <= OLCLastTime) {
    OLCLastTime = Basic->Time;
  }
  if(Basic->Time <= GetFRecordLastTime()) {
    SetFRecordLastTime(Basic->Time);
  }

  // draw snail points more often in circling mode
  if (Calculated->Circling) {
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

  DistanceBearing(Basic->Latitude, Basic->Longitude,
		  Latitude_last, Longitude_last,
		  &distance, NULL);
  Latitude_last = Basic->Latitude;
  Longitude_last = Basic->Longitude;

  if (distance>200.0) {
    return;
  }

  if (Basic->Time - LogLastTime >= dtLog) {
    double balt = -1;
    if (Basic->BaroAltitudeAvailable) {
      balt = Basic->BaroAltitude;
    } else {
      balt = Basic->Altitude;
    }
    LogPoint(Basic->Latitude , Basic->Longitude , Basic->Altitude,
             balt);
    LogLastTime += dtLog;
    if (LogLastTime< Basic->Time-dtLog) {
      LogLastTime = Basic->Time-dtLog;
    }
    if (FastLogNum) FastLogNum--;
  }

  if (Basic->Time - GetFRecordLastTime() >= dtFRecord)
  {
    if (LogFRecord(Basic->SatelliteIDs,false))
    {  // need F record every 5 minutes
       // so if write fails or constellation is invalid, don't update timer and try again next cycle
      SetFRecordLastTime(GetFRecordLastTime() + dtFRecord);
      // the FRecordLastTime is reset when the logger restarts so it is always at the start of the file
      if (GetFRecordLastTime() < Basic->Time-dtFRecord)
        SetFRecordLastTime(Basic->Time-dtFRecord);
    }
  }

  if (Basic->Time - SnailLastTime >= dtSnail) {
    AddSnailPoint(Basic, Calculated);
    SnailLastTime += dtSnail;
    if (SnailLastTime< Basic->Time-dtSnail) {
      SnailLastTime = Basic->Time-dtSnail;
    }
  }

  if (Calculated->Flying) {
    if (Basic->Time - StatsLastTime >= dtStats) {

      flightstats.Altitude_Terrain.
        least_squares_update(max(0,
                                 Basic->Time-Calculated->TakeOffTime)/3600.0,
                             Calculated->TerrainAlt);

      flightstats.Altitude.
        least_squares_update(max(0,
                                 Basic->Time-Calculated->TakeOffTime)/3600.0,
                             Calculated->NavAltitude);
      StatsLastTime += dtStats;
      if (StatsLastTime< Basic->Time-dtStats) {
        StatsLastTime = Basic->Time-dtStats;
      }
    }

    if (Calculated->Flying && (Basic->Time - OLCLastTime >= dtOLC)) {
      bool restart;
      restart = olc.addPoint(Basic->Longitude,
			     Basic->Latitude,
			     Calculated->NavAltitude,
			     Calculated->WaypointBearing,
			     Basic->Time-Calculated->TakeOffTime);

      if (restart && EnableOLC) {
	Calculated->ValidFinish = false;
	StartTask(Basic, Calculated, false, false);
	Calculated->ValidStart = true;
      }
      OLCLastTime += dtOLC;
    }
  }
}




/////////

static PeriodClock last_team_code_update;
DWORD lastTeamCodeUpdateTime = GetTickCount();

void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;

  if (!last_team_code_update.check_update(10000))
    return;


  double distance = 0;
  double bearing = 0;
  TCHAR code[10];

  /*
  distance =  Distance(WayPointList[TeamCodeRefWaypoint].Latitude,
  	WayPointList[TeamCodeRefWaypoint].Longitude,
  	Basic->Latitude,
  	Basic->Longitude);

  bearing = Bearing(WayPointList[TeamCodeRefWaypoint].Latitude,
  	WayPointList[TeamCodeRefWaypoint].Longitude,
  	Basic->Latitude,
  	Basic->Longitude);
  */

  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude,
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude,
                  Basic->Longitude,
                  &bearing, &distance);

  GetTeamCode(code, bearing, distance);

  Calculated->TeammateBearing = bearing;
  Calculated->TeammateRange = distance;

  //// calculate lat/long for team-mate position
  //double teammateBearing = GetTeammateBearingFromRef(TeammateCode);
  //double teammateRange = GetTeammateRangeFromRef(TeammateCode);

  //Calculated->TeammateLongitude = FindLongitude(

  _tcsncpy(Calculated->OwnTeamCode, code, 5);
}


void CalculateTeammateBearingRange(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static bool InTeamSector = false;

  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;

  double ownDistance = 0;
  double ownBearing = 0;
  double mateDistance = 0;
  double mateBearing = 0;

  //ownBearing = Bearing(Basic->Latitude, Basic->Longitude,
  //	WayPointList[TeamCodeRefWaypoint].Latitude,
  //	WayPointList[TeamCodeRefWaypoint].Longitude);
  //
  //ownDistance =  Distance(Basic->Latitude, Basic->Longitude,
  //	WayPointList[TeamCodeRefWaypoint].Latitude,
  //	WayPointList[TeamCodeRefWaypoint].Longitude);

  /*
  ownBearing = Bearing(WayPointList[TeamCodeRefWaypoint].Latitude,
                       WayPointList[TeamCodeRefWaypoint].Longitude,
                       Basic->Latitude,
                       Basic->Longitude
                       );
  //
  ownDistance =  Distance(WayPointList[TeamCodeRefWaypoint].Latitude,
                          WayPointList[TeamCodeRefWaypoint].Longitude,
                          Basic->Latitude,
                          Basic->Longitude
                          );
  */

  LL_to_BearRange(WayPointList[TeamCodeRefWaypoint].Latitude,
                  WayPointList[TeamCodeRefWaypoint].Longitude,
                  Basic->Latitude,
                  Basic->Longitude,
                  &ownBearing, &ownDistance);

  if (TeammateCodeValid)
    {

      //mateBearing = Bearing(Basic->Latitude, Basic->Longitude, TeammateLatitude, TeammateLongitude);
      //mateDistance = Distance(Basic->Latitude, Basic->Longitude, TeammateLatitude, TeammateLongitude);

      CalcTeammateBearingRange(ownBearing, ownDistance,
                               TeammateCode,
                               &mateBearing, &mateDistance);

      // TODO code ....change the result of CalcTeammateBearingRange to do this !
      if (mateBearing > 180)
        {
          mateBearing -= 180;
        }
      else
        {
          mateBearing += 180;
        }


      Calculated->TeammateBearing = mateBearing;
      Calculated->TeammateRange = mateDistance;

      FindLatitudeLongitude(Basic->Latitude,
                            Basic->Longitude,
                            mateBearing,
                            mateDistance,
                            &TeammateLatitude,
                            &TeammateLongitude);

      if (mateDistance < 100 && InTeamSector==false)
        {
          InTeamSector=true;
          InputEvents::processGlideComputer(GCE_TEAM_POS_REACHED);
        }
      else if (mateDistance > 300)
        {
          InTeamSector = false;
        }
    }
  else
    {
      Calculated->TeammateBearing = 0;
      Calculated->TeammateRange = 0;
    }

}

/////////////////////////////////////////////////////////////////////

#define NUM_CAL_SPEED 25
#define NUM_CAL_VARIO 101
#define NUM_CAL_VSPEED 50

static double calibration_tevario_val[NUM_CAL_SPEED][NUM_CAL_VARIO];
static unsigned int calibration_tevario_num[NUM_CAL_SPEED][NUM_CAL_VARIO];
static double calibration_speed_val[NUM_CAL_VSPEED];
static unsigned int calibration_speed_num[NUM_CAL_VSPEED];


void CalibrationInit(void) {
  int i, j;
  for (i=0; i< NUM_CAL_SPEED; i++) {
    for (j=0; j< NUM_CAL_VARIO; j++) {
      calibration_tevario_val[i][j] = 0;
      calibration_tevario_num[i][j] = 0;
    }
  }
  for (i=0; i< NUM_CAL_VSPEED; i++) {
    calibration_speed_val[i] = 0;
    calibration_speed_num[i] = 0;
  }
}


void CalibrationSave(void) {
  int i, j;
  double v, w = 0, wav;
  StartupStore(TEXT("Calibration data for TE vario\n"));
  for (i=0; i< NUM_CAL_SPEED; i++) {
    for (j=0; j< NUM_CAL_VARIO; j++) {
      if (calibration_tevario_num[i][j]>0) {
        v = i*2.0+20.0;
        w = (j-50.0)/10.0;
        wav = calibration_tevario_val[i][j]/calibration_tevario_num[i][j];
        StartupStore(TEXT("%g %g %g %d\n"), v, w, wav,
                  calibration_tevario_num[i][j]);
      }
    }
  }
  StartupStore(TEXT("Calibration data for ASI\n"));
  for (i=0; i< NUM_CAL_VSPEED; i++) {
    if (calibration_speed_num[i]>0) {
      v = i+20.0;
      wav = calibration_speed_val[i]/calibration_speed_num[i];
      StartupStore(TEXT("%g %g %g %d\n"), v, w, wav,
                calibration_speed_num[i]);
    }
  }
}


void CalibrationUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  if (!Calculated->Flying) return;
  if ((!Basic->AirspeedAvailable) || (Basic->TrueAirspeed<=0)) {
    return;
  }
  double ias_to_tas = Basic->TrueAirspeed/
    max(1.0,Basic->IndicatedAirspeed);

  // Vario calibration info
  int index_te_vario = lround(Calculated->GPSVarioTE*10)+50;
  int index_speed = lround((Basic->TrueAirspeed-20)/2);
  if (index_te_vario < 0)
    return;
  if (index_te_vario >= NUM_CAL_VARIO)
    return;
  if (index_speed<0)
    return;
  if (index_speed>= NUM_CAL_SPEED)
    return;

  calibration_tevario_val[index_speed][index_te_vario] +=
    Basic->Vario*ias_to_tas;
  calibration_tevario_num[index_speed][index_te_vario] ++;

  // ASI calibration info
  int index_vspeed = lround((Basic->TrueAirspeed-20));
  if (index_vspeed<0)
    return;
  if (index_vspeed>= NUM_CAL_VSPEED)
    return;

  calibration_speed_val[index_vspeed] += Calculated->TrueAirspeedEstimated;
  calibration_speed_num[index_vspeed] ++;

}

//////////////////////


