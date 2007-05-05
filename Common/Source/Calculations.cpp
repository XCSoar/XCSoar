/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2005

        M Roberts (original release)
        Robin Birch <robinb@ruffnready.co.uk>
        Samuel Gisiger <samuel.gisiger@triadis.ch>
        Jeff Goodenough <jeff@enborne.f2s.com>
        Alastair Harrison <aharrison@magic.force9.co.uk>
        Scott Penrose <scottp@dd.com.au>
        John Wharington <jwharington@bigfoot.com>

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

#include "stdafx.h"
#include "Calculations.h"
#include "Dialogs.h"
#include "parser.h"
#include "Utils.h"
#include "Externs.h"
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
#include <tchar.h>
#include "ThermalLocator.h"
#include "windanalyser.h"
#include "Atmosphere.h"
#include "VegaVoice.h"
#include "OnLineContest.h"
#include "AATDistance.h"
#include "NavFunctions.h" // used for team code
#include "Calculations2.h"
#include "Port.h"
#include "WindZigZag.h"

WindAnalyser *windanalyser = NULL;
OLCOptimizer olc;
AATDistance aatdistance;
static DERIVED_INFO Finish_Derived_Info;
static VegaVoice vegavoice;
static ThermalLocator thermallocator;
int AutoWindMode= 1;
#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2

// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both

bool EnableNavBaroAltitude=false;
bool EnableExternalTriggerCruise=false;
bool ExternalTriggerCruise= false;
bool ExternalTriggerCircling= false;
bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;
int    AutoMcMode = 0;
// 0: Final glide only
// 1: Set to average if in climb mode
// 2: Average if in climb mode, final glide in final glide mode

static int getFinalWaypoint(void);
static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void EnergyHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double maccready);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static bool  InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, BOOL* CrossedStart);
static bool  InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
static bool  InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


//////////////////

static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  (void)Basic;
  if (Calculated->TaskDistanceToGo<=0) {
    return 0;
  }

  // Fraction of task distance covered
  double d_fraction = Calculated->TaskDistanceCovered/
    (Calculated->TaskDistanceCovered+Calculated->TaskDistanceToGo);

  // Height relative to start height
  double dh_start = Calculated->NavAltitude-Calculated->TaskStartAltitude;

  // Height relative to finish height
  double dh_finish = Calculated->NavAltitude-SAFETYALTITUDEARRIVAL;

  int FinalWayPoint = getFinalWaypoint();
  if(ValidTaskPoint(FinalWayPoint)) {
    dh_finish -= WayPointList[Task[FinalWayPoint].Index].Altitude;
  }

  // Excess height
  return dh_start*(1.0-d_fraction)+dh_finish*(d_fraction);
}


void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double bearing, distance;
  double lat, lon;
  bool outofrange;

  // estimate max range (only interested in at most one screen distance away)
  double mymaxrange = MapWindow::GetApproxScreenRange();

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    //    bearing = -90+i*180/NUMTERRAINSWEEPS+Basic->TrackBearing;
    bearing = i*360/NUMTERRAINSWEEPS;
    distance = FinalGlideThroughTerrain(bearing,
                                        Basic,
                                        Calculated, &lat, &lon,
                                        mymaxrange, &outofrange);
    if (outofrange) {
      FindLatitudeLongitude(Basic->Latitude, Basic->Longitude,
                            bearing,
                            mymaxrange*20,
                            &lat, &lon);
    }
    Calculated->GlideFootPrint[i].x = lon;
    Calculated->GlideFootPrint[i].y = lat;
  }
}


int FinishLine=1;
DWORD FinishRadius=1000;


void RefreshTaskStatistics(void) {
  LockFlightData();
  LockTaskData();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  TaskSpeed(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  IterateEffectiveMacCready(&GPS_INFO, &CALCULATED_INFO);
  UnlockTaskData();
  UnlockFlightData();
}


int getFinalWaypoint() {
  int i;
  i=max(-1,min(MAXTASKPOINTS,ActiveWayPoint));

  i++;
  LockTaskData();
  while((i<MAXTASKPOINTS) && (Task[i].Index != -1))
    {
      i++;
    }
  UnlockTaskData();
  return i-1;
}


static bool IsFinalWaypoint(void) {
  bool retval;
  LockTaskData();
  if (ValidTaskPoint(ActiveWayPoint) && (Task[ActiveWayPoint+1].Index >= 0)) {
    retval = false;
  } else {
    retval = true;
  }
  UnlockTaskData();
  return retval;
}

extern int FastLogNum; // number of points to log at high rate

void AnnounceWayPointSwitch(DERIVED_INFO *Calculated, bool doadvance) {
  if (ActiveWayPoint == 0) {
    InputEvents::processGlideComputer(GCE_TASK_START);
    // JMW cleared thermal climb average on task start
    flightstats.ThermalAverage.Reset();
  } else if (Calculated->ValidFinish && IsFinalWaypoint()) {
    InputEvents::processGlideComputer(GCE_TASK_FINISH);
  } else {
    InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
  }

  if (doadvance) {
    ActiveWayPoint++;
  }

  SelectedWaypoint = ActiveWayPoint;
  // set waypoint detail to active task WP

  // start logging data at faster rate
  FastLogNum = 5;

  // play sound
  /* Not needed now since can put it in input events if users want it.
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_TASKTURNPOINT"));
  }
  */
}




extern int jmw_demo;

void NettoVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  double n;

  // get load factor

  if (Basic->AccelerationAvailable) {
    n = Basic->Gload;
  } else {
    n = 1.0;
  }

  // calculate sink rate of glider

  double theSinkRate;

  if (Basic->AirspeedAvailable) {
    theSinkRate= GlidePolar::SinkRate(Basic->IndicatedAirspeed, n);
  } else {
    // assume zero wind (Speed=Airspeed, very bad I know)
    theSinkRate= GlidePolar::SinkRate(Basic->Speed, n);
  }

  if (Basic->NettoVarioAvailable && !ReplayLogger::IsEnabled()) {
    Calculated->NettoVario = Basic->NettoVario;
  } else {
    if (Basic->VarioAvailable && !ReplayLogger::IsEnabled()) {
      Calculated->NettoVario = Basic->Vario - theSinkRate;
    } else {
      Calculated->NettoVario = Calculated->Vario - theSinkRate;
    }
  }

  // calculate optimum cruise speed in current track direction
  // this still makes use of mode, so it should agree with
  // Vmcready if the track bearing is the best cruise track
  // this does assume g loading of 1.0

  // this is basically a dolphin soaring calculator

  double dmc;

  if (EnableBlockSTF) {
    dmc = MACCREADY;
  } else {
    dmc = MACCREADY-Calculated->NettoVario;
  }

  if (Calculated->Vario <= MACCREADY) {

    double VOptnew;

    GlidePolar::MacCreadyAltitude(dmc,
                                 100.0, // dummy value
                                 Basic->TrackBearing,
                                 Calculated->WindSpeed,
                                 Calculated->WindBearing,
                                 0,
                                 &VOptnew,
                                 true,
                                 0
                                 );

    // put low pass filter on VOpt so display doesn't jump around
    // too much
    Calculated->VOpt = Calculated->VOpt*0.6+VOptnew*0.4;

  } else {
    // this thermal is better than maccready, so fly at minimum sink
    // speed
    // calculate speed of min sink adjusted for load factor
    Calculated->VOpt = GlidePolar::Vminsink*sqrt(n);
  }

  Calculated->STFMode = false;
  double vdiff;

  if (Basic->AirspeedAvailable) {
    if (Basic->AirspeedAvailable) {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->IndicatedAirspeed+0.01));
    } else {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->Speed+0.01));
    }
    Calculated->STFMode = false;
    if ((Basic->Speed>NettoSpeed)||
        ((Calculated->VOpt>NettoSpeed)&&(Basic->Speed<Calculated->VOpt*1.1))
        ){
      Calculated->STFMode = true;
    }
    // lock on when supernetto climb rate is half mc
    if (dmc< MACCREADY/2.0) {
      Calculated->STFMode = false;
    }
    // lock on in circling
    if (Calculated->Circling) {
      Calculated->STFMode = false;
    }
  }

  Calculated->STFMode = !Calculated->Circling;

}


void AudioVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

#define AUDIOSCALE 100/7.5  // +/- 7.5 m/s range

  if (
      (Basic->AirspeedAvailable &&
       (Basic->IndicatedAirspeed >= NettoSpeed))
      ||
      (!Basic->AirspeedAvailable &&
       (Basic->Speed >= NettoSpeed))
      ) {
    // TODO: slow/smooth switching between netto and not

    //    VarioSound_SetV((short)((Calculated->NettoVario-GlidePolar::minsink)*AUDIOSCALE));

  } else {
    if (Basic->VarioAvailable && !ReplayLogger::IsEnabled()) {
      //      VarioSound_SetV((short)(Basic->Vario*AUDIOSCALE));
    } else {
      //      VarioSound_SetV((short)(Calculated->Vario*AUDIOSCALE));
    }
  }

  double vdiff;

  if (Basic->AirspeedAvailable) {
    if (Basic->AirspeedAvailable) {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->IndicatedAirspeed+0.01));
    } else {
      vdiff = 100*(1.0-Calculated->VOpt/(Basic->Speed+0.01));
    }
    //    VarioSound_SetVAlt((short)(vdiff));
  }

  //  VarioSound_SoundParam();

}


BOOL DoCalculationsVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;

  NettoVario(Basic, Calculated);
#ifndef DISABLEAUDIOVARIO
  AudioVario(Basic, Calculated);
#endif

  // has GPS time advanced?
  if(Basic->Time <= LastTime)
    {
      LastTime = Basic->Time;
      return FALSE;
    }

  LastTime = Basic->Time;

  return TRUE;
}


bool EnableCalibration = false;


void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double x0, y0, mag;

  if ((Basic->Speed>0)||(Calculated->WindSpeed>0)) {
    x0 = fastsine(Basic->TrackBearing)*Basic->Speed;
    y0 = fastcosine(Basic->TrackBearing)*Basic->Speed;
    x0 += fastsine(Calculated->WindBearing)*Calculated->WindSpeed;
    y0 += fastcosine(Calculated->WindBearing)*Calculated->WindSpeed;

    Calculated->Heading = atan2(x0,y0)*RAD_TO_DEG;
    if (Calculated->Heading<0) {
      Calculated->Heading += 360;
    }

    if (!Calculated->Flying) {
      // don't take wind into account when on ground
      Calculated->Heading = Basic->TrackBearing;
    }

#ifdef _SIM_
    if (!Calculated->Circling) {
      mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
      Basic->TrueAirspeed = mag;
    }
#endif

    if (((AutoWindMode & D_AUTOWIND_ZIGZAG)==D_AUTOWIND_ZIGZAG)
        && (!ReplayLogger::IsEnabled())) {
      Vector v;
      double zzwindspeed;
      double zzwindbearing;
      int quality;
      quality = WindZigZagUpdate(Basic, Calculated,
                           &zzwindspeed,
				 &zzwindbearing);
      if (quality>0) {
        v.x = zzwindspeed*cos(zzwindbearing*3.1415926/180.0);
        v.y = zzwindspeed*sin(zzwindbearing*3.1415926/180.0);
        if (windanalyser) {
	  windanalyser->slot_newEstimate(Basic, Calculated, v, quality);
        }
      }
    }

    if (EnableCalibration) {
      mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
      if ((Basic->AirspeedAvailable) && (Basic->IndicatedAirspeed>0)) {

        double k = (mag / max(1.0,Basic->TrueAirspeed));

#ifdef DEBUG
        char buffer[200];
        sprintf(buffer,"%g %g %g %g %g %g %g %g %g # airspeed\r\n",
                Basic->IndicatedAirspeed,
                mag*Basic->IndicatedAirspeed/max(1.0,Basic->TrueAirspeed),
                k,
                Basic->Speed,
                Calculated->WindSpeed,
                Calculated->WindBearing,
                Basic->Gload,
                Basic->BaroAltitude,
                Basic->Altitude);
        DebugStore(buffer);
#endif

      }
    }

  } else {
    Calculated->Heading = Basic->TrackBearing;
  }

}


void  SetWindEstimate(double speed, double bearing, int quality) {
  Vector v;
  v.x = speed*cos(bearing*3.1415926/180.0);
  v.y = speed*sin(bearing*3.1415926/180.0);
  if (windanalyser) {
    windanalyser->slot_newEstimate(&GPS_INFO, &CALCULATED_INFO, v, quality);
  }
}

void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // do slow part of calculations (cleanup of caches etc, nothing
  // that changes the state)

  static double LastOptimiseTime = 0;
  static double lastTime = 0;
  if (Basic->Time<= lastTime) {
    lastTime = Basic->Time;
  } else {
    AirspaceWarning(Basic, Calculated);
  }

  if (FinalGlideTerrain)
     TerrainFootprint(Basic, Calculated);

  // moved from MapWindow.cpp
  if(Basic->Time> LastOptimiseTime+0.0)
    {
      LastOptimiseTime = Basic->Time;
      if (!RasterTerrain::DirectAccess) {
	LockTerrainDataCalculations();
	if (terrain_dem_calculations.terraincachemisses > 0){
	  DWORD tm =GetTickCount();
	  terrain_dem_calculations.OptimizeCash();
	  tm = GetTickCount()-tm;
	  tm = GetTickCount();
	}
	terrain_dem_calculations.SetCacheTime();
	UnlockTerrainDataCalculations();
      }
    }
}


void HomeDistance(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  int hwp = HomeWaypoint;
  Calculated->HomeDistance = 0.0;
  if (!ValidWayPoint(hwp)) return;

  double w1lat = WayPointList[hwp].Latitude;
  double w1lon = WayPointList[hwp].Longitude;
  double w0lat = Basic->Latitude;
  double w0lon = Basic->Longitude;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  &Calculated->HomeDistance, NULL);

}

void ResetFlightStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      bool full=true) {
  int i;
  (void)Basic;
  if (full) {
    olc.ResetFlight();
    flightstats.Reset();
    aatdistance.Reset();
    Calculated->FlightTime = 0;
    Calculated->TakeOffTime = 0;

    for (i=0; i<200; i++) {
      Calculated->AverageClimbRate[i]= 0;
      Calculated->AverageClimbRateN[i]= 0;
    }
  }

  Calculated->MaxThermalHeight = 0;
  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    Calculated->ThermalProfileN[i]=0;
    Calculated->ThermalProfileW[i]=0;
  }
  // clear thermal sources for first time.
  for (i=0; i<MAX_THERMAL_SOURCES; i++) {
    Calculated->ThermalSources[i].LiftRate= -1.0;
  }

  if (full) {
    Calculated->ValidFinish = false;
    Calculated->ValidStart = false;
    Calculated->TaskStartTime = 0;
    Calculated->TaskStartSpeed = 0;
    Calculated->TaskStartAltitude = 0;
    Calculated->LegStartTime = 0;
  }
}

void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated, bool doadvance,
               bool doannounce) {
  Calculated->ValidFinish = false;
  Calculated->TaskStartTime = Basic->Time ;
  Calculated->TaskStartSpeed = Basic->Speed;
  Calculated->TaskStartAltitude = Calculated->NavAltitude;
  Calculated->LegStartTime = Basic->Time;

  // JMW TODO: Get time from aatdistance module since this is more accurate

  if (doannounce) {
    AnnounceWayPointSwitch(Calculated, doadvance);
  } else {
    if (doadvance) {
      ActiveWayPoint=1;
      SelectedWaypoint = ActiveWayPoint;
    }
  }
}


void CloseCalculations() {
  if (windanalyser) {
    delete windanalyser;
    windanalyser = NULL;
  }
}



void InitCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  StartupStore(TEXT("InitCalculations\r\n"));
  ResetFlightStats(Basic, Calculated, true);
  LoadCalculationsPersist(Calculated);
  DeleteCalculationsPersist();
  // required to allow fail-safe operation
  // if the persistent file is corrupt and causes a crash

  ResetFlightStats(Basic, Calculated, false);
  Calculated->Flying = false;
  Calculated->Circling = false;
  Calculated->FinalGlide = false;
  int i;
  for (i=0; i<=NUMTERRAINSWEEPS; i++) {
    Calculated->GlideFootPrint[i].x = 0;
    Calculated->GlideFootPrint[i].y = 0;
  }
  Calculated->TerrainWarningLatitude = 0.0;
  Calculated->TerrainWarningLongitude = 0.0;

  if (!windanalyser) {
    windanalyser = new WindAnalyser();

    // seed initial wind store with current conditions
    //JMW TODO SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing, 1);

  }
}


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double maccready;

  // Determine which altitude to use for nav functions
  if (EnableNavBaroAltitude && Basic->BaroAltitudeAvailable) {
    Calculated->NavAltitude = Basic->BaroAltitude;
  } else {
    Calculated->NavAltitude = Basic->Altitude;
  }

  DistanceToNext(Basic, Calculated);
  EnergyHeight(Basic, Calculated);
  AltitudeRequired(Basic, Calculated, MACCREADY);
  Heading(Basic, Calculated);
  TerrainHeight(Basic, Calculated);

  if (TaskAborted) {
    SortLandableWaypoints(Basic, Calculated);
  }
  TaskStatistics(Basic, Calculated, MACCREADY);
  TaskSpeed(Basic, Calculated, MACCREADY);

  HomeDistance(Basic, Calculated);

  if ((Basic->Time != 0) && (Basic->Time <= LastTime))
 // 20060519:sgi added (Basic->Time != 0) dueto alwas return here if no GPS time available
    {

      if ((Basic->Time<LastTime) && (!Basic->NAVWarning)) {
	// Reset statistics.. (probably due to being in IGC replay mode)
        ResetFlightStats(Basic, Calculated);
      }

      LastTime = Basic->Time;
      return FALSE;
    }

  LastTime = Basic->Time;

  double t = DetectStartTime(Basic, Calculated);
  if (t>0) {
    Calculated->FlightTime = t;
  }

  TakeoffLanding(Basic, Calculated);

  if ((Calculated->FinalGlide)
      ||(fabs(Calculated->TaskAltitudeDifference)>30)) {
    FinalGlideAlert(Basic, Calculated);
  }

  if (Calculated->AutoMacCready
      && (!TaskAborted)) {
    DoAutoMacCready(Basic, Calculated);
  }

  Turning(Basic, Calculated);
  Vario(Basic,Calculated);
  LD(Basic,Calculated);
  CruiseLD(Basic,Calculated);
  Average30s(Basic,Calculated);
  AverageThermal(Basic,Calculated);
  ThermalGain(Basic,Calculated);
  LastThermalStats(Basic, Calculated);
  ThermalBand(Basic, Calculated);

  DistanceToNext(Basic, Calculated);

  // do we need to do this twice?
  if (TaskAborted) {

    SortLandableWaypoints(Basic, Calculated);

  } else {

    InSector(Basic, Calculated);

    AATStats(Basic, Calculated);
    TaskStatistics(Basic, Calculated, MACCREADY);
    TaskSpeed(Basic, Calculated, MACCREADY);
    IterateEffectiveMacCready(Basic, Calculated);

#ifdef DEBUGFULL
    if (Calculated->TaskStartTime>0) {
      char buffer[200];

      double emc = EffectiveMacCready(Basic, Calculated);
      sprintf(buffer,"%g %g %g %g %g %g %g %g %g %d # taskspeed\r\n",
              Basic->Time-Calculated->TaskStartTime,
              Calculated->TaskDistanceCovered,
              Calculated->TaskDistanceToGo,
              Calculated->TaskAltitudeRequired,
              Calculated->NavAltitude,
              Calculated->TaskSpeedAchieved,
              Calculated->TaskSpeed,
              MACCREADY,
              emc,
              ActiveWayPoint);
      DebugStore(buffer);
    }
#endif

  }

  AltitudeRequired(Basic, Calculated, MACCREADY);

  CalculateNextPosition(Basic, Calculated);

  CalculateOwnTeamCode(Basic, Calculated);
  CalculateTeammateBearingRange(Basic, Calculated);

  DoLogging(Basic, Calculated);

  vegavoice.Update(Basic, Calculated);

  if (Basic->AirspeedAvailable && Basic->VarioAvailable
      && (!Calculated->Circling) && Basic->AccelerationAvailable) {
    int vi = iround(Basic->IndicatedAirspeed);
    double nerr = fabs(Basic->Gload-1.0);
    if ((vi>0)&&(vi<SAFTEYSPEED)
        &&(Basic->TrueAirspeed>0)
        &&(nerr<0.1)) {
      double v = Basic->Vario*Basic->IndicatedAirspeed/
        Basic->TrueAirspeed;
      Calculated->AverageClimbRate[vi]+= v;
      // TODO: Check this is correct for TAS/IAS
      Calculated->AverageClimbRateN[vi]++;
    }
  }

  return TRUE;
}


void EnergyHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double erat;
  if (Basic->AirspeedAvailable && (Basic->IndicatedAirspeed>0)) {
    erat = Basic->TrueAirspeed/Basic->IndicatedAirspeed;
    Calculated->EnergyHeight =
      max(0, Basic->TrueAirspeed*Basic->TrueAirspeed
       -GlidePolar::Vbestld*GlidePolar::Vbestld*erat*erat)/(9.81*2.0);
  } else {
    Calculated->EnergyHeight = 0.0;
  }

}


void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double LastAlt = 0;
  double Gain;

  if(Basic->Time > LastTime)
    {
      Gain = Calculated->NavAltitude - LastAlt;

      if (!Basic->VarioAvailable || ReplayLogger::IsEnabled()) {
        // estimate value from GPS
        Calculated->Vario = Gain / (Basic->Time - LastTime);
      } else {
        // get value from instrument
        Calculated->Vario = Basic->Vario;
        // we don't bother with sound here as it is polled at a
        // faster rate in the DoVarioCalcs methods
      }

      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;

    }
  else
    {
      LastTime = Basic->Time;
    }
}


void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double Altitude[30];
  static double Vario[30];
  static double NettoVario[30];
  int Elapsed, i;
  long temp;
  double Gain;

  if(Basic->Time > LastTime)
    {
      Elapsed = (int)(Basic->Time - LastTime);
      for(i=0;i<Elapsed;i++)
        {
          temp = (long)LastTime + i;
          temp %=30;

          Altitude[temp] = Calculated->NavAltitude;
	  if (Basic->NettoVarioAvailable) {
	    NettoVario[temp] = Basic->NettoVario;
	  } else {
	    NettoVario[temp] = Calculated->NettoVario;
	  }
	  if (Basic->VarioAvailable) {
	    Vario[temp] = Basic->Vario;
	  } else {
	    Vario[temp] = Calculated->Vario;
	  }
        }
      double Vave = 0;
      double NVave = 0;
      for (i=0; i<30; i++) {
        Vave += Vario[i];
	NVave += NettoVario[i];
      }

      temp = (long)Basic->Time - 1;
      temp = temp%30;
      Gain = Altitude[temp];

      temp = (long)Basic->Time;
      temp = temp%30;
      Gain = Gain - Altitude[temp];

      LastTime = Basic->Time;
      if (Basic->VarioAvailable) {
        Calculated->Average30s = (Calculated->Average30s+Vave/30)/2;
      } else {
        Calculated->Average30s = (Calculated->Average30s+Gain/30)/2;
      }
      Calculated->NettoAverage30s = (Calculated->NettoAverage30s+NVave/30)/2;
    }
  else
    {
      if (Basic->Time<LastTime) {
	// gone back in time
	for (i=0; i<30; i++) {
	  Altitude[i]= 0;
	  Vario[i]=0;
	  NettoVario[i]=0;
	}
	LastTime = Basic->Time;
      }
    }
}

void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Gain;

  if(Basic->Time > Calculated->ClimbStartTime)
    {
      Gain = Calculated->NavAltitude - Calculated->ClimbStartAlt;
      Calculated->AverageThermal  = Gain / (Basic->Time - Calculated->ClimbStartTime);
    }
}

void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Basic->Time > Calculated->ClimbStartTime)
    {
      Calculated->ThermalGain = Calculated->NavAltitude - Calculated->ClimbStartAlt;
    }
}

void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;
  double DistanceFlown;
  double AltLost;
  static double glideangle = 0;

  if(Basic->Time - LastTime >20)
    {
      DistanceBearing(Basic->Latitude, Basic->Longitude, LastLat, LastLon,
                      &DistanceFlown, NULL);
      AltLost = LastAlt - Calculated->NavAltitude;
      if(AltLost > 0)
        {
          Calculated->LD = DistanceFlown / AltLost;
          if(Calculated->LD>999)
            {
              Calculated->LD = 999;
            }
        }
      else if (AltLost < 0) {
        Calculated->LD = DistanceFlown / AltLost;
        if (Calculated->LD<-999) {
          Calculated->LD = 999;
        }
      } else {
        Calculated->LD = 999;
      }

      LastLat = Basic->Latitude;
      LastLon = Basic->Longitude;
      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;
    }

  // LD instantaneous from vario
  if (Basic->VarioAvailable && Basic->AirspeedAvailable) {
    if (Basic->IndicatedAirspeed>1) {
      glideangle = glideangle*0.7-0.3*Basic->Vario/Basic->IndicatedAirspeed;
      if (fabs(glideangle)>=0.001) {
	Calculated->LDvario = 1.0/glideangle;
      } else {
	if (glideangle>=0) {
	  Calculated->LDvario = 999;
	} else {
	  Calculated->LDvario = -999;
	}
      }
    } else {
      Calculated->LDvario = 999;
    }
  } else {
    Calculated->LDvario = 999;
  }
}

void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;
  double DistanceFlown;
  double AltLost;


  if(!Calculated->Circling)
    {

       DistanceBearing(Basic->Latitude, Basic->Longitude,
                       Calculated->CruiseStartLat,
                       Calculated->CruiseStartLong, &DistanceFlown, NULL);
      AltLost = Calculated->CruiseStartAlt - Calculated->NavAltitude;
      if(AltLost > 0)
        {
          Calculated->CruiseLD = DistanceFlown / AltLost;
          if(Calculated->CruiseLD>999)
            {
              Calculated->CruiseLD = 999;
            }
        }
      else if (AltLost <0) {
        Calculated->CruiseLD = DistanceFlown / AltLost;
        if(Calculated->CruiseLD< -999)
          {
            Calculated->CruiseLD = 999;
          }
      } else {
        Calculated->CruiseLD = 999;
      }
    }
}

#define CRUISE 0
#define WAITCLIMB 1
#define CLIMB 2
#define WAITCRUISE 3


double MinTurnRate = 4 ; //10;
double CruiseClimbSwitch = 15;
double ClimbCruiseSwitch = 15;


void SwitchZoomClimb(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                     bool isclimb, bool left) {

  static double CruiseMapScale = 10;
  static double ClimbMapScale = 0.25;
  static bool last_isclimb = false;
  // if AutoZoom

  // JMW
  /* Not needed now since can put it in input events if users want it.
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_DRIP"));
  }
  */

  /* this is data reprenentation stuff move it to mapsindow.cpp

  if (CircleZoom) {
    if (isclimb != last_isclimb) {
      if (isclimb) {
        // save cruise scale
        CruiseMapScale = MapWindow::MapScale;
        // switch to climb scale
        MapWindow::RequestMapScale = MapWindow::LimitMapScale(ClimbMapScale);
        MapWindow::BigZoom = true;
      } else {
        // leaving climb
        // save cruise scale
        ClimbMapScale = MapWindow::MapScale;
        MapWindow::RequestMapScale = MapWindow::LimitMapScale(CruiseMapScale);
        // switch to climb scale
        MapWindow::BigZoom = true;
      }
      last_isclimb = isclimb;
    } else {
      // nothing to do.
    }
  } */

  // this is calculation stuff, leave it there

  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    windanalyser->slot_newFlightMode(Basic, Calculated, left, 0);
  }

}


void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTrack = 0;
  static double StartTime  = 0;
  static double StartLong = 0;
  static double StartLat = 0;
  static double StartAlt = 0;
  static double LastTime = 0;
  static double timeCircling = 0;
  static double timeCruising = 0;
  static int MODE = CRUISE;
  static bool LEFT = FALSE;
  double Rate;
  static double LastRate=0;
  double dRate;
  double dT;

  if (!Calculated->Flying) return;

  if(Basic->Time <= LastTime) {
    LastTime = Basic->Time;
    return;
  }
  dT = Basic->Time - LastTime;
  Rate = Basic->TrackBearing-LastTrack;
  while (Rate>180) {
    Rate-= 360;
  }
  while (Rate<-180) {
    Rate+= 360;
  }
  Rate = Rate / dT;

  if (dT<2.0) {
    // time step ok

    // calculate acceleration
    dRate = (Rate-LastRate)/dT;

    double dtlead=0.3;
    // integrate assuming constant acceleration, for one second
    Calculated->NextTrackBearing = Basic->TrackBearing
      + dtlead*(Rate+0.5*dtlead*dRate);
    // s = u.t+ 0.5*a*t*t

    if (Calculated->NextTrackBearing<0) {
      Calculated->NextTrackBearing+= 360;
    }
    if (Calculated->NextTrackBearing>=360) {
      Calculated->NextTrackBearing-= 360;
    }

  } else {
    // time step too big, so just take it at last measurement
    Calculated->NextTrackBearing = Basic->TrackBearing;
  }

  Calculated->TurnRate = Rate;

  Rate = 0.3*Rate+0.7*LastRate; // low pass filter
  LastRate = Rate;

  // JMW added percent climb calculator

  if (Calculated->Circling) {
    //    timeCircling += (Basic->Time-LastTime);
    timeCircling+= 1.0;
  } else {
    //    timeCruising += (Basic->Time-LastTime);
    timeCruising+= 1.0;
  }

  if (timeCruising+timeCircling>0) {
    Calculated->PercentCircling =
      100.0*(timeCircling)/(timeCruising+timeCircling);
  } else {
    Calculated->PercentCircling = 0.0;
  }

  if(Rate <0)
    {
      if (LEFT) {
        // OK, already going left
      } else {
        LEFT = true;
      }
      Rate *= -1;
    } else {
    if (!LEFT) {
      // OK, already going right
    } else {
      LEFT = false;
    }
  }

  LastTime = Basic->Time;
  LastTrack = Basic->TrackBearing;

//  double temp = StartTime;

  bool forcecruise = false;
  bool forcecircling = false;
  if (EnableExternalTriggerCruise && !(ReplayLogger::IsEnabled())) {
    if (ExternalTriggerCruise && ExternalTriggerCircling) {
      // this should never happen
      ExternalTriggerCircling = false;
    }
    forcecruise = ExternalTriggerCruise;
    forcecircling = ExternalTriggerCircling;
  }

  switch(MODE) {
  case CRUISE:
    if((Rate >= MinTurnRate)||(forcecircling)) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Calculated->NavAltitude;
      MODE = WAITCLIMB;
    }
    if (forcecircling) {
      MODE = WAITCLIMB;
    } else {
      break;
    }
  case WAITCLIMB:
    if (forcecruise) {
      MODE = CRUISE;
      break;
    }
    if((Rate >= MinTurnRate)||(forcecircling)) {
      if( ((Basic->Time  - StartTime) > CruiseClimbSwitch)|| forcecircling) {
        Calculated->Circling = TRUE;
        // JMW Transition to climb
        MODE = CLIMB;
        Calculated->ClimbStartLat = StartLat;
        Calculated->ClimbStartLong = StartLong;
        Calculated->ClimbStartAlt = StartAlt;
        Calculated->ClimbStartTime = StartTime;

        if (flightstats.Altitude_Ceiling.sum_n>0) {
          // only update base if have already climbed, otherwise
          // we will catch the takeoff height as the base.

          flightstats.Altitude_Base.
            least_squares_update(Calculated->ClimbStartTime/3600.0,
                                 Calculated->ClimbStartAlt);
        }

        // TODO InputEvents GCE - Move this to InputEvents
        // Consider a way to take the CircleZoom and other logic
        // into InputEvents instead?
        // JMW: NO.  Core functionality must be built into the
        // main program, unable to be overridden.
        SwitchZoomClimb(Basic, Calculated, true, LEFT);
        InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
      }
    } else {
      // nope, not turning, so go back to cruise
      MODE = CRUISE;
    }
    break;
  case CLIMB:
    if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
      windanalyser->slot_newSample(Basic, Calculated);
    }

    if((Rate < MinTurnRate)||(forcecruise)) {
      StartTime = Basic->Time;
      StartLong = Basic->Longitude;
      StartLat  = Basic->Latitude;
      StartAlt  = Calculated->NavAltitude;
      // JMW Transition to cruise, due to not properly turning
      MODE = WAITCRUISE;
    }
    if (forcecruise) {
      MODE = WAITCRUISE;
    } else {
      break;
    }
  case WAITCRUISE:
    if (forcecircling) {
      MODE = CLIMB;
      break;
    }
    if((Rate < MinTurnRate) || forcecruise) {
      if( ((Basic->Time  - StartTime) > ClimbCruiseSwitch) || forcecruise) {
        Calculated->Circling = FALSE;

        // Transition to cruise
        MODE = CRUISE;
        Calculated->CruiseStartLat = StartLat;
        Calculated->CruiseStartLong = StartLong;
        Calculated->CruiseStartAlt = StartAlt;
        Calculated->CruiseStartTime = StartTime;

        flightstats.Altitude_Ceiling.
          least_squares_update(Calculated->CruiseStartTime/3600.0,
                               Calculated->CruiseStartAlt);

        SwitchZoomClimb(Basic, Calculated, false, LEFT);
        InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
      }

      //if ((Basic->Time  - StartTime) > ClimbCruiseSwitch/3) {
      // reset thermal locator if changing thermal cores
      // thermallocator.Reset();
      //}

    } else {
      // JMW Transition back to climb, because we are turning again
      MODE = CLIMB;
    }
    break;
  default:
    // error, go to cruise
    MODE = CRUISE;
  }
  // generate new wind vector if altitude changes or a new
  // estimate is available
  if (AutoWindMode>0) {
    windanalyser->slot_Altitude(Basic, Calculated);
  }

  if (EnableThermalLocator) {
    if (Calculated->Circling) {
      thermallocator.AddPoint(Basic->Time, Basic->Longitude, Basic->Latitude,
			      Calculated->NettoVario);
      thermallocator.Update(Basic->Time, Basic->Longitude, Basic->Latitude,
			    Calculated->WindSpeed, Calculated->WindBearing,
			    Basic->TrackBearing,
			    &Calculated->ThermalEstimate_Longitude,
			    &Calculated->ThermalEstimate_Latitude,
			    &Calculated->ThermalEstimate_W,
			    &Calculated->ThermalEstimate_R);
    } else {
      Calculated->ThermalEstimate_W = 0;
      Calculated->ThermalEstimate_R = -1;
      thermallocator.Reset();
    }
  }

  // update atmospheric model
  CuSonde::updateMeasurements(Basic, Calculated);

}


static void ThermalSources(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double ground_longitude;
  double ground_latitude;
  double ground_altitude;
  thermallocator.
    EstimateThermalBase(
			Calculated->ThermalEstimate_Longitude,
			Calculated->ThermalEstimate_Latitude,
			Calculated->NavAltitude,
			Calculated->LastThermalAverage,
			Calculated->WindSpeed,
			Calculated->WindBearing,
			&ground_longitude,
			&ground_latitude,
			&ground_altitude
			);

  if (ground_altitude>0) {
    double tbest=0;
    int ibest=0;

    for (int i=0; i<MAX_THERMAL_SOURCES; i++) {
      if (Calculated->ThermalSources[i].LiftRate<0.0) {
	ibest = i;
	break;
      }
      double dt = Basic->Time - Calculated->ThermalSources[i].Time;
      if (dt> tbest) {
	tbest = dt;
	ibest = i;
      }
    }
    Calculated->ThermalSources[ibest].LiftRate = Calculated->LastThermalAverage;
    Calculated->ThermalSources[ibest].Latitude = ground_latitude;
    Calculated->ThermalSources[ibest].Longitude = ground_longitude;
    Calculated->ThermalSources[ibest].GroundHeight = ground_altitude;
    Calculated->ThermalSources[ibest].Time = Basic->Time;
  }
}


static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static int LastCircling = FALSE;
  double ThermalGain;
  double ThermalTime;

  if((Calculated->Circling == FALSE) && (LastCircling == TRUE))
    {
      ThermalGain = Calculated->CruiseStartAlt - Calculated->ClimbStartAlt;
      ThermalTime = Calculated->CruiseStartTime - Calculated->ClimbStartTime;

      if(ThermalTime >0)
        {
          Calculated->LastThermalAverage = ThermalGain/ThermalTime;
          Calculated->LastThermalGain = ThermalGain;
          Calculated->LastThermalTime = ThermalTime;

          if (Calculated->LastThermalAverage>0) {
            flightstats.ThermalAverage.
              least_squares_update(Calculated->LastThermalAverage);

#ifdef DEBUG
            char Temp[100];
            sprintf(Temp,"%f %f # thermal stats\n",
                    flightstats.ThermalAverage.m,
                    flightstats.ThermalAverage.b
                    );
            DebugStore(Temp);
#endif
	    if (EnableThermalLocator && (ThermalTime>60)) {
	      ThermalSources(Basic, Calculated);
	    }
	  }
	}
    }
  LastCircling = Calculated->Circling;
}

void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!WayPointList) return;

  //  LockFlightData();
  LockTaskData();

  if((ActiveWayPoint >=0) && ValidTaskPoint(ActiveWayPoint))
    {
      double w1lat, w1lon;
      double w0lat, w0lon;

      w0lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
      DistanceBearing(Basic->Latitude, Basic->Longitude,
                      w0lat, w0lon,
                      &Calculated->WaypointDistance,
                      &Calculated->WaypointBearing);

      Calculated->ZoomDistance = Calculated->WaypointDistance;

      if (AATEnabled && !TaskAborted) {
        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        w1lat, w1lon,
                        &Calculated->WaypointDistance,
                        &Calculated->WaypointBearing);

        Calculated->ZoomDistance = max(Calculated->WaypointDistance,
                                       Calculated->ZoomDistance);

      }
    }
  else
    {
      Calculated->ZoomDistance = 0;
      Calculated->WaypointDistance = 0;
      Calculated->WaypointBearing = 0;
    }
  UnlockTaskData();
  //  UnlockFlightData();
}


void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  //  LockFlightData();
  (void)Basic;
  LockTaskData();
  if(ValidTaskPoint(ActiveWayPoint))
    {
      Calculated->NextAltitudeRequired =
        GlidePolar::MacCreadyAltitude(maccready,
                        Calculated->WaypointDistance,
                        Calculated->WaypointBearing,
                        Calculated->WindSpeed, Calculated->WindBearing,
                        0, 0, (ActiveWayPoint == getFinalWaypoint()),
                        0
                        // ||
                        // (Calculated->TaskAltitudeDifference>30)
                        // JMW TODO!!!!!!!!!
                        );

      Calculated->NextAltitudeRequired =
        Calculated->NextAltitudeRequired + SAFETYALTITUDEARRIVAL ;

      double wpAlt = 0.0f;
      if (ValidTaskPoint(ActiveWayPoint))
        wpAlt = WayPointList[Task[ActiveWayPoint].Index].Altitude;
      Calculated->NextAltitudeDifference =
        Calculated->NavAltitude
        - (Calculated->NextAltitudeRequired+wpAlt)
        + Calculated->EnergyHeight;
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
    }
  UnlockTaskData();
  //  UnlockFlightData();
}


bool InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int thepoint)
{
  double AircraftBearing;

  if (!ValidTaskPoint(thepoint)) return false;

  if(SectorType==0)
    {
      if(Calculated->WaypointDistance < SectorRadius)
        {
          return true;
        }
    }
  if (SectorType>0)
    {
      LockTaskData();
      DistanceBearing(WayPointList[Task[thepoint].Index].Latitude,
                      WayPointList[Task[thepoint].Index].Longitude,
                      Basic->Latitude ,
                      Basic->Longitude,
                      NULL, &AircraftBearing);
      UnlockTaskData();

      AircraftBearing = AircraftBearing - Task[thepoint].Bisector ;
      while (AircraftBearing<-180) {
        AircraftBearing+= 360;
      }
      while (AircraftBearing>180) {
        AircraftBearing-= 360;
      }

      if (SectorType==2) {
        // JMW added german rules
        if (Calculated->WaypointDistance<500) {
          return true;
        }
      }
      if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
        {
          if (SectorType==1) {
            if(Calculated->WaypointDistance < 20000)
              {
                return true;
              }
          } else {
            // JMW added german rules
            if(Calculated->WaypointDistance < 10000)
              {
                return true;
              }
          }
        }
    }
  return false;
}

bool InAATTurnSector(double longitude, double latitude,
                    int thepoint)
{
  double AircraftBearing;
  bool retval = false;

  if (!ValidTaskPoint(thepoint)) {
    return false;
  }

  double distance;
  LockTaskData();
  DistanceBearing(WayPointList[Task[thepoint].Index].Latitude,
                  WayPointList[Task[thepoint].Index].Longitude,
                  latitude,
                  longitude,
                  &distance, &AircraftBearing);

  if(Task[thepoint].AATType ==  CIRCLE) {
    if(distance < Task[thepoint].AATCircleRadius)
      {
        retval = true;
        goto OnExit;
      }
  } else if(distance < Task[thepoint].AATSectorRadius) {

    if(Task[thepoint].AATStartRadial
       < Task[thepoint].AATFinishRadial ) {
      if(
         (AircraftBearing > Task[thepoint].AATStartRadial)
         &&
         (AircraftBearing < Task[thepoint].AATFinishRadial)
         ) {
        retval = true;
        goto OnExit;
      }
    }

    if(Task[thepoint].AATStartRadial
       > Task[thepoint].AATFinishRadial ) {
      if(
         (AircraftBearing > Task[thepoint].AATStartRadial)
         ||
         (AircraftBearing < Task[thepoint].AATFinishRadial)
         ) {
        retval = true;
        goto OnExit;
      }
    }
  }
OnExit:
  UnlockTaskData();
  return retval;
}


bool ValidFinish(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  (void)Basic;
  if ((FinishMinHeight>0)
      &&(Calculated->TerrainValid)
      &&(Calculated->AltitudeAGL<FinishMinHeight)) {
    return false;
  } else {
    return true;
  }
}


bool InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                   int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;
  bool retval = false;

  if (!WayPointList) return FALSE;

  if (!ValidFinish(Basic, Calculated)) return FALSE;

  // Finish invalid
  if (!ValidTaskPoint(i)) return FALSE;

  LockTaskData();

  // distance from aircraft to start point
  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  WayPointList[Task[i].Index].Latitude,
                  WayPointList[Task[i].Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);
  bool inrange = false;
  inrange = (FirstPointDistance<FinishRadius);
  if (!inrange) {
    LastInSector = false;
  }

  if(!FinishLine) // Start Circle
    {
      retval = inrange;
      goto OnExit;
    }

  // Finish line
  AircraftBearing = AircraftBearing - Task[i].InBound ;

  while (AircraftBearing<-180) {
    AircraftBearing+= 360;
  }
  while (AircraftBearing>180) {
    AircraftBearing-= 360;
  }
  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(FinishLine==1) { // Finish line
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = !((AircraftBearing >= 135) || (AircraftBearing <= -135));
  }

  if (inrange) {

    if (LastInSector) {
      // previously approaching the finish line
      if (!approaching) {
        // now moving away from finish line
        LastInSector = false;
        retval = TRUE;
        goto OnExit;
      }
    } else {
      if (approaching) {
        // now approaching the finish line
        LastInSector = true;
      }
    }

  } else {
    LastInSector = false;
  }
 OnExit:
  UnlockTaskData();
  return retval;
}


bool InStartSector_Internal(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                           int Index,
                           double OutBound,
                           bool &LastInSector)
{
  (void)Calculated;
  if (!ValidWayPoint(Index)) return false;

  // No Task Loaded

  double AircraftBearing;
  double FirstPointDistance;

  // distance from aircraft to start point
  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  WayPointList[Index].Latitude,
                  WayPointList[Index].Longitude,
                  &FirstPointDistance,
                  &AircraftBearing);

  bool inrange = false;
  inrange = (FirstPointDistance<StartRadius);

  if(StartLine==0) {
    // Start Circle
    return inrange;
  }

  // Start Line
  AircraftBearing = AircraftBearing - OutBound ;
  while (AircraftBearing<-180) {
    AircraftBearing+= 360;
  }
  while (AircraftBearing>180) {
    AircraftBearing-= 360;
  }
  // JMW bugfix, was Bisector, which is invalid

  bool approaching;
  if(StartLine==1) { // Start line
    approaching = ((AircraftBearing >= -90) && (AircraftBearing <= 90));
  } else {
    // FAI 90 degree
    approaching = ((AircraftBearing >= -45) && (AircraftBearing <= 45));
  }

  if (inrange) {
    return approaching;
  } else {
    // cheat fail of last because exited from side
    LastInSector = false;
  }

  return false;
}


bool InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int &index,
                   BOOL *CrossedStart)
{
  static bool LastInSector = false;
  static int EntryStartSector = index;

  bool isInSector= false;
  bool retval=false;

  if (!Calculated->Flying) {
    return false;
  }
  if (!ValidTaskPoint(ActiveWayPoint))
    return false;
  if (!ValidTaskPoint(0))
    return false;

  LockTaskData();

  if ((ActiveWayPoint>0)
      && !ValidTaskPoint(ActiveWayPoint+1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

  if ((Task[0].Index != EntryStartSector) && (EntryStartSector>=0)) {
    LastInSector = false;
    EntryStartSector = Task[0].Index;
  }

  isInSector = InStartSector_Internal(Basic, Calculated,
                                      Task[0].Index, Task[0].OutBound,
                                      LastInSector);

  *CrossedStart = LastInSector && !isInSector;
  LastInSector = isInSector;
  if (*CrossedStart) {
    goto OnExit;
  }

  if (EnableMultipleStartPoints) {
    for (int i=0; i<MAXSTARTPOINTS; i++) {
      if (StartPoints[i].Active && (StartPoints[i].Index>=0)
          && (StartPoints[i].Index != Task[0].Index)) {

        retval = InStartSector_Internal(Basic, Calculated,
                                        StartPoints[i].Index,
                                        StartPoints[i].OutBound,
                                        StartPoints[i].InSector);
        isInSector |= retval;
        index = StartPoints[i].Index;
        *CrossedStart = StartPoints[i].InSector && !retval;
        StartPoints[i].InSector = retval;
        if (*CrossedStart) {
          if (Task[0].Index != index) {
            Task[0].Index = index;
            LastInSector = false;
            EntryStartSector = index;
            RefreshTask();
          }
          goto OnExit;
        }

      }
    }
  }

 OnExit:

  UnlockTaskData();
  return isInSector;
}

#define AUTOADVANCE_MANUAL 0
#define AUTOADVANCE_AUTO 1
#define AUTOADVANCE_ARM 2
#define AUTOADVANCE_ARMSTART 3

bool ReadyToStart(DERIVED_INFO *Calculated) {
  if (!Calculated->Flying) {
    return false;
  }
  if (AutoAdvance== AUTOADVANCE_AUTO) {
    return true;
  }
  if ((AutoAdvance== AUTOADVANCE_ARM) || (AutoAdvance==AUTOADVANCE_ARMSTART)) {
    if (AdvanceArmed) {
      return true;
    }
  }
  return false;
}


bool ReadyToAdvance(DERIVED_INFO *Calculated, bool reset=true, bool restart=false) {
  static int lastReady = -1;
  static int lastActive = -1;
  bool say_ready = false;

  // 0: Manual
  // 1: Auto
  // 2: Arm
  // 3: Arm start

  if (!Calculated->Flying) {
    lastReady = -1;
    lastActive = -1;
    return false;
  }

  if (AutoAdvance== AUTOADVANCE_AUTO) {
    if (reset) AdvanceArmed = false;
    return true;
  }
  if (AutoAdvance== AUTOADVANCE_ARM) {
    if (AdvanceArmed) {
      if (reset) AdvanceArmed = false;
      return true;
    } else {
      say_ready = true;
    }
  }
  if (AutoAdvance== AUTOADVANCE_ARMSTART) {
	  if ((ActiveWayPoint == 0) || restart) {
      if (!AdvanceArmed) {
        say_ready = true;
      } else if (reset) {
        AdvanceArmed = false;
        return true;
      }
    }
  }

  // see if we've gone back a waypoint (e.g. restart)
  if (ActiveWayPoint < lastActive) {
    lastReady = -1;
  }
  lastActive = ActiveWayPoint;

  if (say_ready) {
    if (ActiveWayPoint != lastReady) {
      InputEvents::processGlideComputer(GCE_ARM_READY);
      lastReady = ActiveWayPoint;
    }
  }
  return false;
}


/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

bool ValidStart(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  bool valid = true;
  if ((StartMaxHeight!=0)&&(Calculated->TerrainValid)) {
    if (Calculated->AltitudeAGL>StartMaxHeight)
      valid = false;
  }
  if (StartMaxSpeed!=0) {
    if (Basic->AirspeedAvailable) {
      if (Basic->IndicatedAirspeed>StartMaxSpeed)
        valid = false;
    } else {
      if (Basic->Speed>StartMaxSpeed)
        valid = false;
    }
  }
  return valid;
}


static void CheckStart(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                       int *LastStartSector) {
  BOOL StartCrossed;

  if (InStartSector(Basic,Calculated,*LastStartSector, &StartCrossed)) {
    Calculated->IsInSector = true;

    if (ReadyToStart(Calculated)) {
      aatdistance.AddPoint(Basic->Longitude,
                           Basic->Latitude,
                           0);
    }
    if (ValidStart(Basic, Calculated)) {
      ReadyToAdvance(Calculated, false, true);
    }
    // TODO monitor start speed throughout time in start sector
  }
  if (StartCrossed) {
    if(!IsFinalWaypoint() && ValidStart(Basic, Calculated)) {
      Calculated->ValidStart = true;
      if (ReadyToAdvance(Calculated, true, true)) {
        ActiveWayPoint=0; // enforce this since it may be 1
        StartTask(Basic,Calculated, true, true);
      }
      if (Calculated->Flying) {
        Calculated->ValidFinish = false;
      }
      // JMW TODO: This causes Vaverage to go bonkers
      // if the user has already passed the start
      // but selects the start

      // Note: pilot must have armed advance
      // for the start to be registered
    }
  }
}


static BOOL CheckRestart(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                         int *LastStartSector) {
  if((Basic->Time - Calculated->TaskStartTime < 600)
     &&(ActiveWayPoint<=1)) {
    /*
    BOOL StartCrossed;
    if(InStartSector(Basic, Calculated, *LastStartSector, &StartCrossed)) {
      Calculated->IsInSector = true;

      // this allows restart if returned to start sector before
      // 10 minutes after task start
      ActiveWayPoint = 0;
      return TRUE;
    }
    */
    CheckStart(Basic, Calculated, LastStartSector);
  }
  return FALSE;
}


static void CheckFinish(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
    Calculated->IsInSector = true;
    aatdistance.AddPoint(Basic->Longitude,
                         Basic->Latitude,
                         ActiveWayPoint);
    if (!Calculated->ValidFinish) {
      Calculated->ValidFinish = true;
      AnnounceWayPointSwitch(Calculated, false);

      // JMWX save calculated data
      memcpy(&Finish_Derived_Info, Calculated, sizeof(DERIVED_INFO));
    }
  }
}


static void AddAATPoint(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                        int taskwaypoint) {
  bool insector = false;
  if (taskwaypoint>0) {
    if (AATEnabled) {
      insector = InAATTurnSector(Basic->Longitude,
                                 Basic->Latitude, taskwaypoint);
    } else {
      insector = InTurnSector(Basic, Calculated, taskwaypoint);
    }
    if(insector) {
      if (taskwaypoint == ActiveWayPoint) {
        Calculated->IsInSector = true;
      }
      aatdistance.AddPoint(Basic->Longitude,
                           Basic->Latitude,
                           taskwaypoint);
    }
  }
}


static void CheckInSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  if (ActiveWayPoint>0) {
    AddAATPoint(Basic, Calculated, ActiveWayPoint-1);
  }
  AddAATPoint(Basic, Calculated, ActiveWayPoint);

  // JMW Start bug XXX

  if (aatdistance.HasEntered(ActiveWayPoint)) {
    if (ReadyToAdvance(Calculated, true, false)) {
      AnnounceWayPointSwitch(Calculated, true);
      Calculated->LegStartTime = Basic->Time;
    }
    if (Calculated->Flying) {
      Calculated->ValidFinish = false;
    }
  }
}


void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static int LastStartSector = -1;

  if (ActiveWayPoint<0) return;

  LockTaskData();

  Calculated->IsInSector = false;

  if(ActiveWayPoint == 0) {
    CheckStart(Basic, Calculated, &LastStartSector);
  } else {
    if(IsFinalWaypoint()) {
      LastStartSector = -1;
      CheckFinish(Basic, Calculated);
    } else {
      CheckRestart(Basic, Calculated, &LastStartSector);
      if (ActiveWayPoint>0) {
        CheckInSector(Basic, Calculated);
        LastStartSector = -1;
      }
    }
  }
  UnlockTaskData();
}


///////////////////////////////////
#include "RasterTerrain.h"

RasterTerrain terrain_dem_graphics;
RasterTerrain terrain_dem_calculations;





static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Alt = 0;

  LockTerrainDataCalculations();
  // want most accurate rounding here
  terrain_dem_calculations.SetTerrainRounding(0,0);
  Alt = terrain_dem_calculations.
    GetTerrainHeight(Basic->Latitude , Basic->Longitude);
  UnlockTerrainDataCalculations();

  if(Alt<0) {
    Alt = 0;
    Calculated->TerrainValid = false;
    Calculated->TerrainAlt = 0;
  } else {
    Calculated->TerrainValid = true;
    Calculated->TerrainAlt = Alt;
  }
  Calculated->AltitudeAGL = Calculated->NavAltitude - Calculated->TerrainAlt;

}


/////////////////////////////////////////

static bool TaskAltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                                 double maccready, double *Vfinal,
                                 double *TotalTime, double *TotalDistance,
                                 int *ifinal)
{
  int i;
  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;
  double LegTime, LegDistance, LegBearing, LegAltitude;
  bool retval = false;

  // Calculate altitude required from start of task

  bool isfinal=true;
  LegAltitude = 0;
  double TotalAltitude = 0;
  *TotalTime = 0; *TotalDistance = 0;
  *ifinal = 0;
  LockTaskData();
  for(i=MAXTASKPOINTS-2;i>=0;i--) {
    if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

    w1lat = WayPointList[Task[i].Index].Latitude;
    w1lon = WayPointList[Task[i].Index].Longitude;
    w0lat = WayPointList[Task[i+1].Index].Latitude;
    w0lon = WayPointList[Task[i+1].Index].Longitude;

    if (AATEnabled) {
      w1lat = Task[i].AATTargetLat;
      w1lon = Task[i].AATTargetLon;
      w0lat = Task[i+1].AATTargetLat;
      w0lon = Task[i+1].AATTargetLon;
    }

    DistanceBearing(w1lat, w1lon,
                    w0lat, w0lon,
                    &LegDistance, &LegBearing);

    *TotalDistance += LegDistance;

    LegAltitude =
      GlidePolar::MacCreadyAltitude(maccready,
                                    LegDistance,
                                    LegBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    0,
                                    isfinal,
                                    &LegTime
                                    );

    TotalAltitude += LegAltitude;

    if (LegTime<0) {
      UnlockTaskData();
      return false;
    } else {
      *TotalTime += LegTime;
    }
    if (isfinal) {
      *ifinal = i+1;
      if (LegTime>0) {
        *Vfinal = LegDistance/LegTime;
      }
    }
    isfinal = false;
  }

  if (*ifinal==0) {
    retval = false;
    goto OnExit;
  }

  TotalAltitude += SAFETYALTITUDEARRIVAL;

  if (!ValidTaskPoint(*ifinal)) {
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = false;
  } else {
    TotalAltitude += WayPointList[Task[*ifinal].Index].Altitude;
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = true;
  }
 OnExit:
  UnlockTaskData();
  return retval;
}


void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  int ifinal;
  static double LastTime = 0;
  static double v2last = 0;
  static double t1last = 0;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;

  if (Calculated->ValidFinish) return;

  // in case we leave early due to error
  Calculated->TaskSpeedAchieved = 0;
  Calculated->TaskSpeed = 0;
  Calculated->TaskSpeedInstantaneous = 0;

  if (ActiveWayPoint==0) { // no task speed before start
    return;
  }

  //  LockFlightData();
  LockTaskData();

  if (TaskAltitudeRequired(Basic, Calculated, maccready, &Vfinal,
                           &TotalTime, &TotalDistance, &ifinal)) {

    double t0 = TotalTime;
    // total time expected for task

    double t1 = Basic->Time-Calculated->TaskStartTime;
    // time elapsed since start

    double d0 = TotalDistance;
    // total task distance

    double d1 = Calculated->TaskDistanceCovered;
    // actual distance covered

    double dr = Calculated->TaskDistanceToGo;
    // distance remaining

    double t2;
    // equivalent time elapsed after final glide

    double d2;
    // equivalent distance travelled after final glide

    double hf = SAFETYALTITUDEARRIVAL +
      WayPointList[Task[ifinal].Index].Altitude;

    double h0 = Calculated->TaskAltitudeRequiredFromStart-hf;
    // total height required from start (takes safety arrival alt
    // and finish waypoint altitude into account)

    double h1 = max(0,Calculated->NavAltitude-hf);
    // height above target

    double dFinal;
    // final glide distance

    // equivalent speed
    double v2, v1;

    if ((t1<=0) || (d1<=0) || (d0<=0) || (t0<=0) || (h0<=0)) {
      // haven't started yet or not a real task
      goto OnExit;
    }

    // JB's task speed...
    double hx = SpeedHeight(Basic, Calculated);
    double t1mod = t1-hx/max(0.1,maccready);
    // only valid if flown for 5 minutes or more
    if (t1mod>300.0) {
      Calculated->TaskSpeedAchieved = d1/t1mod;
    } else {
      Calculated->TaskSpeedAchieved = d1/t1;
    }

    if (Vfinal<=0) {
      // can't reach target at current mc
      goto OnExit;
    }

    // distance that can be usefully final glided from here
    // (assumes average task glide angle of d0/h0)
    dFinal = min(dr, d0*min(1.0,h1/h0));

    // equivalent distance to end of final glide
    d2 = d1+dFinal;

    // time at end of final glide
    t2 = t1+dFinal/Vfinal;

    // actual task speed achieved so far
    v1 = d1/t1;

    // average speed to end of final glide from here
    v2 = d2/t2;

    Calculated->TaskSpeed = max(v1,v2);

    if(Basic->Time <= LastTime) {
      LastTime = Basic->Time;
    } else if (Basic->Time-LastTime >=1.0) {

      // Calculate contribution to average task speed.
      // This is equal to the change in virtual distance
      // divided by the time step

      // This is a novel concept.
      // When climbing at the MC setting, this number should
      // be similar to the estimated task speed.
      // When climbing slowly or when flying off-course,
      // this number will drop.
      // In cruise at the optimum speed in zero lift, this
      // number will be similar to the estimated task speed.

      // A low pass filter is applied so it doesn't jump around
      // too much when circling.

      // If this number is higher than the overall task average speed,
      // it means that the task average speed is increasing.

      // When cruising in sink, this number will decrease.
      // When cruising in lift, this number will increase.

      // Therefore, it shows well whether at any time the glider
      // is wasting time.

      double delta_d2 = (v2*t1-v2last*t1last);
      double vdiff = delta_d2/(Basic->Time-LastTime);
      Calculated->TaskSpeedInstantaneous =
        Calculated->TaskSpeedInstantaneous*0.98+0.02*vdiff;
      v2last = v2;
      t1last = t1;
      LastTime = Basic->Time;
    }
  }
 OnExit:
  UnlockTaskData();

}


void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                    double maccready)
{
  int i;
  double LegCovered, LegToGo=0;
  double LegDistance, LegBearing=0;
  double LegAltitude;
  double TaskAltitudeRequired = 0;
  double TaskAltitudeRequired0 = 0;
  double LegAltitude0, LegTime0;
  static bool fgtt = false;
  bool fgttnew = false;

  if (!WayPointList) return;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  //  LockFlightData();
  LockTaskData();

  ////////////
  // Calculate Task Distances
  if(ActiveWayPoint >=1)
    {

      // First calculate distances for this waypoint

      // JMW added support for target in AAT

      w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
      w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
      w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;

      if (AATEnabled) {
        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;

        // TODO: Get best range point to here...
        w0lat = Task[ActiveWayPoint-1].AATTargetLat;
        w0lon = Task[ActiveWayPoint-1].AATTargetLon;

      }

      DistanceBearing(w1lat,
                      w1lon,
                      w0lat,
                      w0lon,
                      &LegDistance, NULL);

      DistanceBearing(Basic->Latitude,
                      Basic->Longitude,
                      w1lat,
                      w1lon,
                      &LegToGo, NULL);

      // LegCovered = max(0,LegDistance - LegToGo);
      LegCovered = ProjectedDistance(w0lon, w0lat,
                                     w1lon, w1lat,
                                     Basic->Longitude,
                                     Basic->Latitude);

      if ((StartLine==0) && (ActiveWayPoint==1)) {
        // Correct speed calculations for radius
        // JMW TODO: replace this with more accurate version
        // LegDistance -= StartRadius;
        LegCovered = max(0,LegCovered-StartRadius);
      }

      Calculated->LegDistanceToGo = LegToGo;
      Calculated->LegDistanceCovered = LegCovered;
      Calculated->TaskDistanceCovered = LegCovered;
      if(Basic->Time > Calculated->LegStartTime) {
        Calculated->LegSpeed = Calculated->LegDistanceCovered
          / (Basic->Time - Calculated->LegStartTime);
      } else {
        // will this ever occur?
        Calculated->LegSpeed = 0;
      }

      // Now add distances for start to previous waypoint

      for(i=0;i<ActiveWayPoint-1;i++)
        {
          if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i+1].Index].Latitude;
          w0lon = WayPointList[Task[i+1].Index].Longitude;

          if (AATEnabled) {
            w1lat = Task[i].AATTargetLat;
            w1lon = Task[i].AATTargetLon;
            w0lat = Task[i+1].AATTargetLat;
            w0lon = Task[i+1].AATTargetLon;
          }

          DistanceBearing(w1lat,
                          w1lon,
                          w0lat,
                          w0lon,
                          &LegDistance, NULL);


          Calculated->TaskDistanceCovered += LegDistance;
        }

      if (AATEnabled) {
        // JMW added correction for distance covered
        Calculated->TaskDistanceCovered =
          aatdistance.DistanceCovered(Basic->Longitude,
                                      Basic->Latitude,
                                      ActiveWayPoint);
      }

      if(Basic->Time > Calculated->TaskStartTime) {
        Calculated->TaskSpeed =
          Calculated->TaskDistanceCovered
          / (Basic->Time - Calculated->TaskStartTime);
        // note this is refined later in TaskSpeed()
      }
    }
  else
    {
      // haven't started task yet
      Calculated->TaskSpeed = 0;
      Calculated->TaskDistanceCovered = 0;
    }

  //////////////////////

  // Calculate Final Glide To Finish
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;

//  double FinalAltitude = 0;
  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = Calculated->NavAltitude-SAFETYALTITUDEARRIVAL;
  if (ValidTaskPoint(FinalWayPoint)) {
    height_above_finish -=
      WayPointList[Task[FinalWayPoint].Index].Altitude;
  }

  if(ValidTaskPoint(ActiveWayPoint))
    {
      i=ActiveWayPoint;

      // update final glide mode status
      if (
          (ActiveWayPoint == FinalWayPoint)
          ||(ForceFinalGlide)
          ||(TaskAborted)) {
        // JMW on final glide
        if (Calculated->FinalGlide == 0)
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
        Calculated->FinalGlide = 1;
      } else {
        if (Calculated->FinalGlide == 1)
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
        Calculated->FinalGlide = 0;
      }

      if (AATEnabled) {

        DistanceBearing(Basic->Latitude ,
                        Basic->Longitude ,
                        Task[i].AATTargetLat,
                        Task[i].AATTargetLon,
                        &LegToGo, &LegBearing);

      } else {

        if (Task[i].Index>=0) {

          DistanceBearing(Basic->Latitude ,
                          Basic->Longitude ,
                          WayPointList[Task[i].Index].Latitude,
                          WayPointList[Task[i].Index].Longitude,
                          &LegToGo, &LegBearing);
        }
      }

      LegAltitude =
        GlidePolar::MacCreadyAltitude(maccready,
                                      LegToGo,
                                      LegBearing,
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      &(Calculated->BestCruiseTrack),
                                      &(Calculated->VMacCready),
                                      (Calculated->FinalGlide==1),
                                      &(Calculated->LegTimeToGo),
                                      height_above_finish);

      LegAltitude0 =
        GlidePolar::MacCreadyAltitude(0,
                                      LegToGo,
                                      LegBearing,
                                      Calculated->WindSpeed,
                                      Calculated->WindBearing,
                                      0,
                                      0,
                                      true,
                                      &LegTime0
                                      );

      if (LegTime0>=1e5) {
        // can't make it, so assume flying at current mc
        LegAltitude0 = LegAltitude;
      }

      // Final glide through terrain updates
      if (Calculated->FinalGlide) {

        double lat, lon;
	bool outofrange;
        double distancesoarable =
          FinalGlideThroughTerrain(LegBearing, Basic, Calculated,
                                   &lat,
                                   &lon,
                                   LegToGo, &outofrange);

        if ((distancesoarable< LegToGo)&&(!outofrange)) {
          // JMW TODO issue terrain warning (audio?)
          Calculated->TerrainWarningLatitude = lat;
          Calculated->TerrainWarningLongitude = lon;

          fgttnew = true;
        } else {
          Calculated->TerrainWarningLatitude = 0.0;
          Calculated->TerrainWarningLongitude = 0.0;
          fgtt = false;
        }

      } else {
        Calculated->TerrainWarningLatitude = 0.0;
        Calculated->TerrainWarningLongitude = 0.0;
        fgtt = false;
      }

      double height_below_leg = LegAltitude - Calculated->NavAltitude +
        SAFETYALTITUDEARRIVAL;

      /*

      if (Calculated->FinalGlide) {
        if (height_below_leg>0) {
          Calculated->LegTimeToGo += height_below_leg/max(0.1,MACCREADY);
          // (need to stop and climb before finish)
        }
      }
      */

      TaskAltitudeRequired = LegAltitude;
      TaskAltitudeRequired0 = LegAltitude0;
      Calculated->TaskDistanceToGo = LegToGo;
      Calculated->TaskTimeToGo = Calculated->LegTimeToGo;

      if(height_below_leg < 0) {
        Calculated->LDNext = -Calculated->TaskDistanceToGo/height_below_leg;
      } else {
        Calculated->LDNext = 999;
      }

      i++;
      while(ValidTaskPoint(i) && (!TaskAborted))
        {

          double this_LegTimeToGo;
          bool this_is_final = (i==FinalWayPoint);

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i-1].Index].Latitude;
          w0lon = WayPointList[Task[i-1].Index].Longitude;

          if (AATEnabled) {
            w1lat = Task[i].AATTargetLat;
            w1lon = Task[i].AATTargetLon;
            w0lat = Task[i-1].AATTargetLat;
            w0lon = Task[i-1].AATTargetLon;
          }

          DistanceBearing(w0lat,
                          w0lon,
                          w1lat,
                          w1lon,
                          &LegDistance, &LegBearing);

          LegAltitude = GlidePolar::
            MacCreadyAltitude(maccready,
                              LegDistance, LegBearing,
                              Calculated->WindSpeed,
                              Calculated->WindBearing,
                              0, 0,
                              this_is_final || ForceFinalGlide,
                              &this_LegTimeToGo,
                              height_above_finish);

          LegAltitude0 = GlidePolar::
            MacCreadyAltitude(0,
                              LegDistance, LegBearing,
                              Calculated->WindSpeed,
                              Calculated->WindBearing,
                              0, 0,
                              true,
                              &LegTime0
                              );

          if (LegTime0>=1e5) {
            // can't make it, so assume flying at current mc
            LegAltitude0 = LegAltitude;
          }

          TaskAltitudeRequired += LegAltitude;
          TaskAltitudeRequired0 += LegAltitude0;

          /*
          double height_below_leg = LegAltitude - Calculated->NavAltitude +
            SAFETYALTITUDEARRIVAL;

          if (this_is_final) {
            if (height_below_leg>0) {
              this_LegTimeToGo += height_below_leg/max(0.1,MACCREADY);
              // (need to stop and climb before finish)
            }
          }
          */

          Calculated->TaskDistanceToGo += LegDistance;
          Calculated->TaskTimeToGo += this_LegTimeToGo;

          i++;
        }

      double final_h = SAFETYALTITUDEARRIVAL
        + WayPointList[Task[FinalWayPoint].Index].Altitude;

      double te_height = Calculated->NavAltitude + Calculated->EnergyHeight;

      Calculated->TaskAltitudeRequired = TaskAltitudeRequired + final_h;

      TaskAltitudeRequired0 += final_h;

      Calculated->TaskAltitudeDifference = te_height
        - Calculated->TaskAltitudeRequired;

      Calculated->TaskAltitudeDifference0 = te_height
        - TaskAltitudeRequired0;

      if (Calculated->TaskAltitudeDifference>0) {
        if (!fgtt && fgttnew) {
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
          fgtt = true;
        }
      } else {
        fgtt = false;
      }

      double te_diff = te_height - final_h;
      if(te_diff > 0) {
        Calculated->LDFinish = Calculated->TaskDistanceToGo/te_diff;
      } else {
        Calculated->LDFinish = 999;
      }

      // Auto Force Final Glide forces final glide mode
      // if above final glide...
      if (AutoForceFinalGlide) {
	if (!Calculated->FinalGlide) {
	  if (Calculated->TaskAltitudeDifference>120) {
	    ForceFinalGlide = true;
	  } else {
	    ForceFinalGlide = false;
	  }
	} else {
	  if (Calculated->TaskAltitudeDifference<-120) {
	    ForceFinalGlide = false;
	  } else {
	    ForceFinalGlide = true;
	  }
	}
      }

    } else {
    // no task selected, so work things out at current heading

    Calculated->FinalGlide = 0;

    GlidePolar::MacCreadyAltitude(maccready, 100.0, Basic->TrackBearing,
                                 Calculated->WindSpeed,
                                 Calculated->WindBearing,
                                 &(Calculated->BestCruiseTrack),
                                 &(Calculated->VMacCready),
                                  (Calculated->FinalGlide==1),
                                 0);

  }

  UnlockTaskData();
  //  UnlockFlightData();

}


static bool ActiveIsFinalWaypoint() {
  return (ActiveWayPoint == getFinalWaypoint());
}


void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double tad=0.0;
  static double dmc=0.0;

  bool isfinalglide = false;

  //  LockFlightData();
  LockTaskData();

  double mcnew = MACCREADY;

  if (Calculated->FinalGlide && ActiveIsFinalWaypoint()) {
    isfinalglide = true;
  }

  if ((AutoMcMode==0)||((AutoMcMode==2)&& isfinalglide)) {

    if (ValidTaskPoint(ActiveWayPoint)) {

      double timeremaining = Basic->Time-Calculated->TaskStartTime-9000;
      if (EnableOLC
	  && (OLCRules==0)
	  && (Calculated->NavAltitude>Calculated->TaskStartAltitude)
	  && (timeremaining>0)) {

	mcnew = MacCreadyTimeLimit(Basic, Calculated,
				   Calculated->WaypointBearing,
				   timeremaining,
				   Calculated->TaskStartAltitude);

      } else {
        if (Calculated->TaskAltitudeDifference0>0) {
          double slope =
            (Calculated->NavAltitude
             - SAFETYALTITUDEARRIVAL
             - WayPointList[Task[ActiveWayPoint].Index].Altitude)/
            (Calculated->WaypointDistance+1);

          double mcp = PirkerAnalysis(Basic, Calculated,
                                      Calculated->WaypointBearing,
                                      slope);
          if (mcp>0) {
            mcnew = mcp;
          } else {
            mcnew = 0.0;
          }
        } else {
          // no change, below final glide with zero Mc
          mcnew = MACCREADY;
        }
      }
    }
  } else if ((AutoMcMode==1)||((AutoMcMode==2)&&(!isfinalglide))) {

    if (flightstats.ThermalAverage.y_ave>0) {
      mcnew = flightstats.ThermalAverage.y_ave;
    }

  }

  MACCREADY = 0.85*MACCREADY+0.15*mcnew;

  UnlockTaskData();
  //  UnlockFlightData();

}

void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL BelowGlide = TRUE;
  static double delayedTAD = 0.0;
  (void)Basic;
  delayedTAD = 0.95*delayedTAD+0.05*Calculated->TaskAltitudeDifference;

  if(BelowGlide == TRUE)
    {
      if(delayedTAD > 50)
        {
          BelowGlide = FALSE;
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_ABOVE);
        }
    }
  else
    {
      if(delayedTAD < 50)
        {
          BelowGlide = TRUE;

          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_BELOW);
        }
    }
}

extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;


void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Calculated->Circling)
    {
      Calculated->NextLatitude = Basic->Latitude;
      Calculated->NextLongitude = Basic->Longitude;
      Calculated->NextAltitude =
        Calculated->NavAltitude + Calculated->Average30s * 30;
    }
  else
    {
      FindLatitudeLongitude(Basic->Latitude,
                            Basic->Longitude,
                            Basic->TrackBearing,
                            Basic->Speed*WarningTime,
                            &Calculated->NextLatitude,
                            &Calculated->NextLongitude);

      if (Basic->BaroAltitudeAvailable) {
        Calculated->NextAltitude =
          Basic->BaroAltitude + Calculated->Average30s * WarningTime;
      } else {
        Calculated->NextAltitude =
          Calculated->NavAltitude + Calculated->Average30s * WarningTime;
      }
    }
}


//////////////////////////////////////////////

bool GlobalClearAirspaceWarnings = false;

bool ClearAirspaceWarnings(bool ack, bool allday) {
  unsigned int i;
  if (ack) {
    GlobalClearAirspaceWarnings = true;
    if (AirspaceCircle) {
      for (i=0; i<NumberOfAirspaceCircles; i++) {
        if (AirspaceCircle[i].WarningLevel>0) {
          AirspaceCircle[i].Ack.AcknowledgementTime = GPS_INFO.Time;
          if (allday) {
            AirspaceCircle[i].Ack.AcknowledgedToday = true;
          }
          AirspaceCircle[i].WarningLevel = 0;
        }
      }
    }
    if (AirspaceArea) {
      for (i=0; i<NumberOfAirspaceAreas; i++) {
        if (AirspaceArea[i].WarningLevel>0) {
          AirspaceArea[i].Ack.AcknowledgementTime = GPS_INFO.Time;
          if (allday) {
            AirspaceArea[i].Ack.AcknowledgedToday = true;
          }
          AirspaceArea[i].WarningLevel = 0;
        }
      }
    }
    return Message::Acknowledge(MSG_AIRSPACE);
  }
  return false;
}

// new style airspace warnings
// defined in AirspaceWarning.cpp

void AirspaceWarnListAdd(NMEA_INFO *Basic, int Sequence, bool Predicted, bool IsCircle, int AsIdx);
void AirspaceWarnListProcess(NMEA_INFO *Basic);


void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated){
  unsigned int i;

//  TCHAR text[1024];
  bool inside;

  if(!AIRSPACEWARNINGS)
      return;

  static bool predicted = false;
  static int  UpdateSequence = 0;

  //  LockFlightData(); Not necessary, airspace stuff has its own locking

  if (GlobalClearAirspaceWarnings == true) {
    GlobalClearAirspaceWarnings = false;
    Calculated->IsInAirspace = false;
  }

  predicted = !predicted;
  // every second time step, do predicted position rather than
  // current position

  if (!predicted){
    if (++UpdateSequence > 1000)
      UpdateSequence = 1;
  }

  double alt;
  double lat;
  double lon;

  if (predicted) {
    alt = Calculated->NextAltitude;
    lat = Calculated->NextLatitude;
    lon = Calculated->NextLongitude;
  } else {
    if (Basic->BaroAltitudeAvailable) {
      alt = Basic->BaroAltitude;
    } else {
      alt = Basic->Altitude;
    }
    lat = Basic->Latitude;
    lon = Basic->Longitude;
  }

  // JMW TODO: FindAirspaceCircle etc should sort results, return
  // the most critical or closest.

  if (AirspaceCircle) {
    for (i=0; i<NumberOfAirspaceCircles; i++) {
      inside = false;

      if ((alt >= AirspaceCircle[i].Base.Altitude )
          && (alt < AirspaceCircle[i].Top.Altitude)) {


        if (InsideAirspaceCircle(lon, lat, i)
            && (MapWindow::iAirspaceMode[AirspaceCircle[i].Type] >= 2)){
          AirspaceWarnListAdd(Basic, UpdateSequence, predicted, 1, i);
        }

      }

    }
  }

  // repeat process for areas

  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {
      inside = false;

      if ((alt >= AirspaceArea[i].Base.Altitude )
          && (alt < AirspaceArea[i].Top.Altitude)) {

        if ((MapWindow::iAirspaceMode[AirspaceArea[i].Type] >= 2)
            && InsideAirspaceArea(lon, lat, i)){
          AirspaceWarnListAdd(Basic, UpdateSequence, predicted, 0, i);
        }

      }
    }
  }

  AirspaceWarnListProcess(Basic);

  //  UnlockFlightData();

}

//////////////////////////////////////////////

void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Temp;
  int i;
  double MaxDistance, MinDistance, TargetDistance;
  double LegToGo, LegDistance, TargetLegToGo, TargetLegDistance;

  if (!WayPointList) return ;

  if(!AATEnabled)
    {
      return;
    }

  if (Calculated->ValidFinish) return;

  //  LockFlightData();
  LockTaskData();

  Temp = Basic->Time - Calculated->TaskStartTime;

  if ((ActiveWayPoint==0)&&(Calculated->AATTimeToGo==0)) {
    Calculated->AATTimeToGo = AATTaskLength*60;
  }

  if((Temp >=0)&&(ActiveWayPoint >0))
    {
      Calculated->AATTimeToGo = (AATTaskLength*60) - Temp;
      if(Calculated->AATTimeToGo <= 0)
        Calculated->AATTimeToGo = 0;
      if(Calculated->AATTimeToGo >= (AATTaskLength * 60) )
        Calculated->AATTimeToGo = (AATTaskLength * 60);
    }

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  //  Calculated->TaskDistanceToGo = 0;
  // JMW: not sure why this is here?

  if(ValidTaskPoint(ActiveWayPoint))
    {
      i=ActiveWayPoint;

      DistanceBearing(Basic->Latitude , Basic->Longitude ,
                      WayPointList[Task[i].Index].Latitude,
                      WayPointList[Task[i].Index].Longitude,
                      &LegToGo, NULL);

      DistanceBearing(Basic->Latitude , Basic->Longitude ,
                      Task[i].AATTargetLat,
                      Task[i].AATTargetLon,
                      &TargetLegToGo, NULL);

      if(Task[i].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (Task[i].AATCircleRadius * 2);
          MinDistance = LegToGo - (Task[i].AATCircleRadius * 2);
        }
      else
        {
          MaxDistance = LegToGo + (Task[i].AATSectorRadius * 2);
          MinDistance = LegToGo;
        }

      TargetDistance = TargetLegToGo;

      i++;
      while(ValidTaskPoint(i))
        {
          DistanceBearing(WayPointList[Task[i].Index].Latitude,
                          WayPointList[Task[i].Index].Longitude,
                          WayPointList[Task[i-1].Index].Latitude,
                          WayPointList[Task[i-1].Index].Longitude,
                          &LegDistance, NULL);

          DistanceBearing(Task[i].AATTargetLat,
                          Task[i].AATTargetLon,
                          Task[i-1].AATTargetLat,
                          Task[i-1].AATTargetLon,
                          &TargetLegDistance, NULL);

          if(Task[ActiveWayPoint].AATType == CIRCLE)
            {
              MaxDistance += LegDistance + (Task[i].AATCircleRadius * 2);
              MinDistance += LegDistance- (Task[i].AATCircleRadius * 2);
            }
          else
            {
              MaxDistance += LegDistance + (Task[ActiveWayPoint].AATSectorRadius * 2);
              MinDistance += LegDistance;
            }
          TargetDistance += TargetLegDistance;
          i++;
        }

      // JMW TODO: make this more accurate, because currently it is
      // very approximate.

      Calculated->AATMaxDistance = MaxDistance;
      Calculated->AATMinDistance = MinDistance;
      Calculated->AATTargetDistance = TargetDistance;

      if(Calculated->AATTimeToGo >0)
        {
          Calculated->AATMaxSpeed =
            Calculated->AATMaxDistance / Calculated->AATTimeToGo;
          Calculated->AATMinSpeed =
            Calculated->AATMinDistance / Calculated->AATTimeToGo;
          Calculated->AATTargetSpeed =
            Calculated->AATTargetDistance / Calculated->AATTimeToGo;
        }
    }
  UnlockTaskData();
  //  UnlockFlightData();
}


void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  if(Basic->Time <= LastTime)
    {
      LastTime = Basic->Time;
      return;
    }
  LastTime = Basic->Time;

  // JMW TODO: Should really work out dt here,
  //           but i'm assuming constant time steps
  double dheight = Calculated->NavAltitude
    -SAFETYALTITUDEBREAKOFF
    -Calculated->TerrainAlt;

  int index, i, j;

  if (dheight<0) {
    return; // nothing to do.
  }
  if (Calculated->MaxThermalHeight==0) {
    Calculated->MaxThermalHeight = dheight;
  }

  // only do this if in thermal and have been climbing
  if ((!Calculated->Circling)||(Calculated->Average30s<0)) return;

  //  LockFlightData();

  if (dheight > Calculated->MaxThermalHeight) {

    /* JMW oh, it is a bit annoying really
  if (EnableSoundTask) {
    PlayResource(TEXT("IDR_WAV_BEEPBWEEP"));
    }
    */

    // moved beyond ceiling, so redistribute buckets
    double mthnew;
    double tmpW[NUMTHERMALBUCKETS];
    int tmpN[NUMTHERMALBUCKETS];
    double h;

    // calculate new buckets so glider is below max
    double hbuk = Calculated->MaxThermalHeight/NUMTHERMALBUCKETS;

    mthnew = max(1, Calculated->MaxThermalHeight);
    while (mthnew<dheight) {
      mthnew += hbuk;
    }

    // reset counters
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      tmpW[i]= 0.0;
      tmpN[i]= 0;
    }
    // shift data into new buckets
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      h = (i)*(Calculated->MaxThermalHeight)/(NUMTHERMALBUCKETS);
      // height of center of bucket
      j = iround(NUMTHERMALBUCKETS*h/mthnew);

      if (j<NUMTHERMALBUCKETS) {
        if (Calculated->ThermalProfileN[i]>0) {
          tmpW[j] += Calculated->ThermalProfileW[i];
          tmpN[j] += Calculated->ThermalProfileN[i];
        }
      }
    }
    for (i=0; i<NUMTHERMALBUCKETS; i++) {
      Calculated->ThermalProfileW[i]= tmpW[i];
      Calculated->ThermalProfileN[i]= tmpN[i];
    }
    Calculated->MaxThermalHeight= mthnew;
  }

  index = min(NUMTHERMALBUCKETS-1,
	      iround(NUMTHERMALBUCKETS*(dheight/max(1.0,
		     Calculated->MaxThermalHeight))));

  Calculated->ThermalProfileW[index]+= Calculated->Vario;
  Calculated->ThermalProfileN[index]++;
  //  UnlockFlightData();

}





//////////////////////////////////////////////////////////////////


void LatLon2Flat(double lon, double lat, int *scx, int *scy) {
  *scx = (int)(lon*fastcosine(lat)*100);
  *scy = (int)(lat*100);
}

int CalculateWaypointApproxDistance(int scx_aircraft, int scy_aircraft,
                                    int i) {

  // Do preliminary fast search
  int scx, scy;
  LatLon2Flat(WayPointList[i].Longitude,
              WayPointList[i].Latitude, &scx, &scy);
  int dx, dy;
  dx = scx_aircraft-scx;
  dy = scy_aircraft-scy;

  return isqrt4(dx*dx+dy*dy);
}



double CalculateWaypointArrivalAltitude(NMEA_INFO *Basic,
                                        DERIVED_INFO *Calculated,
                                        int i) {
  double AltReqd;
  double wDistance, wBearing;

  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  WayPointList[i].Latitude,
                  WayPointList[i].Longitude,
                  &wDistance, &wBearing);

  AltReqd = GlidePolar::MacCreadyAltitude
    (GlidePolar::AbortSafetyMacCready(),
     wDistance,
     wBearing,
     Calculated->WindSpeed,
     Calculated->WindBearing,
     0,
     0,
     true,
     0);

  return ((Calculated->NavAltitude) - AltReqd
          - WayPointList[i].Altitude - SAFETYALTITUDEARRIVAL);
}



void SortLandableWaypoints(NMEA_INFO *Basic,
                           DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXTASKPOINTS];
  double SortedArrivalAltitude[MAXTASKPOINTS];
  int SortedApproxDistance[MAXTASKPOINTS*2];
  int SortedApproxIndex[MAXTASKPOINTS*2];
  int i, k, l;
  double aa;
  int ai;
  int lastActiveWayPoint;

  if (!WayPointList) return;

  //  LockFlightData();
  LockTaskData();
  lastActiveWayPoint = ActiveWayPoint;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  for (i=0; i<MAXTASKPOINTS*2; i++) {
      SortedApproxIndex[i]= -1;
      SortedApproxDistance[i] = 0;
  }

  for (i=0; i<(int)NumberOfWayPoints; i++) {
    if (!(((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
          ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    ai = CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, i);

    // see if this fits into slot
    for (k=0; k< MAXTASKPOINTS*2; k++)  {
      bool closer = (ai < SortedApproxDistance[k]);

      if ((closer ||  // closer than this one
           (SortedApproxIndex[k]== -1)) &&  // or this one isn't filled
          (SortedApproxIndex[k]!= i)) // and not replacing with same
        {
            // ok, got new biggest, put it into the slot.
          for (l=MAXTASKPOINTS*2-1; l>k; l--) {
            if (l>0) {
                SortedApproxDistance[l] = SortedApproxDistance[l-1];
                SortedApproxIndex[l] = SortedApproxIndex[l-1];
            }
          }

          SortedApproxDistance[k] = ai;
          SortedApproxIndex[k] = i;
          k=MAXTASKPOINTS*2;
        }
    }
  }

  // Now do detailed search
  for (i=0; i<MAXTASKPOINTS; i++) {
    SortedLandableIndex[i]= -1;
    SortedArrivalAltitude[i] = 0;
  }

  int scanairportsfirst;
  bool foundreachableairport = false;

  for (scanairportsfirst=0; scanairportsfirst<2; scanairportsfirst++) {
    if (foundreachableairport) {
      continue; // don't bother filling the rest of the list
    }

    for (i=0; i<MAXTASKPOINTS*2; i++) {
      if (SortedApproxIndex[i]<0) { // ignore invalid points
        continue;
      }

      if (((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT) &&
          (scanairportsfirst==0)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      aa = CalculateWaypointArrivalAltitude(Basic,
                                            Calculated,
                                            SortedApproxIndex[i]);

      if (scanairportsfirst==0) {
        if (aa<0) {
          // in first scan, this airport is unreachable, so ignore it.
          continue;
        } else {
          // this airport is reachable
          foundreachableairport = true;
        }
      }

      // see if this fits into slot
      for (k=0; k< MAXTASKPOINTS; k++) {
        if (((aa > SortedArrivalAltitude[k]) ||// closer than this one
             (SortedLandableIndex[k]== -1)) && // or this one
            // isn't filled
            (SortedLandableIndex[k]!= i))  // and not replacing
                                           // with same
          {

            double LegToGo, LegBearing;
            DistanceBearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude,
                            &LegToGo, &LegBearing);

            bool outofrange;
            double distancesoarable =
              FinalGlideThroughTerrain(LegBearing, Basic, Calculated,
                                       NULL,
                                       NULL,
                                       LegToGo,
                                       &outofrange);

            if ((distancesoarable>= LegToGo)||(aa<0)) {
              // only put this in the index if it is reachable
              // and doesn't go through terrain, OR, if it is unreachable
              // it doesn't matter if it goes through terrain because
              // pilot has to climb first anyway

              // ok, got new biggest, put it into the slot.
              for (l=MAXTASKPOINTS-1; l>k; l--) {
                if (l>0) {
                  SortedArrivalAltitude[l] = SortedArrivalAltitude[l-1];
                  SortedLandableIndex[l] = SortedLandableIndex[l-1];
                }
              }

              SortedArrivalAltitude[k] = aa;
              SortedLandableIndex[k] = SortedApproxIndex[i];
              k=MAXTASKPOINTS;
            }
          }
      }
    }
  }

  // now we have a sorted list.
  // check if current waypoint or home waypoint is in the sorted list
  int foundActiveWayPoint = -1;
  int foundHomeWaypoint = -1;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(ActiveWayPoint)) {
      if (SortedLandableIndex[i] == Task[ActiveWayPoint].Index) {
        foundActiveWayPoint = i;
      }
    }
    if ((SortedLandableIndex[i] == HomeWaypoint)&&(HomeWaypoint>=0)) {
      foundHomeWaypoint = i;
    }
  }

  if ((foundHomeWaypoint == -1)&&(HomeWaypoint>=0)) {
    // home not found in top list, so see if we can sneak it in

    aa = CalculateWaypointArrivalAltitude(Basic,
                                          Calculated,
                                          HomeWaypoint);
    if (aa>0) {
      // only put it in if reachable
      SortedLandableIndex[MAXTASKPOINTS-2] = HomeWaypoint;
    }
  }

  bool newclosest = false;

  if (foundActiveWayPoint != -1) {
    ActiveWayPoint = foundActiveWayPoint;
  } else {
    // if not found, keep on field or set active waypoint to closest
    if (ValidTaskPoint(ActiveWayPoint)){
      aa = CalculateWaypointArrivalAltitude(Basic, Calculated,
                                            Task[ActiveWayPoint].Index);
    } else {
      aa = 0;
    }
    if (aa <= 0){   // last active is no more reachable, switch to
      // new closest
      newclosest = true;
      ActiveWayPoint = 0;
    } else {
      // last active is reachable but not in list, add to end of
      // list (or overwrite laste one)
      if (ActiveWayPoint>=0){
        for (i=0; i<MAXTASKPOINTS-1; i++) {     // find free slot
          if (SortedLandableIndex[i] == -1)     // free slot found (if
                                                // not, i index the
                                                // last entry of the
                                                // list)
            break;
        }
        SortedLandableIndex[i] = Task[ActiveWayPoint].Index;
        ActiveWayPoint = i;
      }
    }
  }

  // set new waypoints in task

  for (i=0; i<(int)NumberOfWayPoints; i++) {
    WayPointList[i].InTask = false;
  }

  int lastclosest=0;
  if (newclosest) {
    lastclosest = Task[0].Index;
  }

  for (i=0; i<MAXTASKPOINTS; i++){
    Task[i].Index = SortedLandableIndex[i];
    if (ValidTaskPoint(i)) {
      WayPointList[Task[i].Index].InTask = true;
    }
  }

  if (newclosest) {
    if ((Task[0].Index != lastclosest) && ValidTaskPoint(0)) {
      double distance= 10000.0;
      if (lastclosest>=0) {
        DistanceBearing(WayPointList[Task[0].Index].Latitude,
                        WayPointList[Task[0].Index].Longitude,
                        WayPointList[lastclosest].Latitude,
                        WayPointList[lastclosest].Longitude,
                        &distance, NULL);
      }
      if (distance>2000.0) {
        // don't display the message unless the airfield has moved by more
        // than 2 km
        DoStatusMessage(gettext(TEXT("Closest Airfield Changed!")));
      }

    }
  }

  if (EnableMultipleStartPoints) {
    for (i=0; i<MAXSTARTPOINTS; i++) {
      if (StartPoints[i].Active && (ValidWayPoint(StartPoints[i].Index))) {
        WayPointList[StartPoints[i].Index].InTask = true;
      }
    }
  }

  if (lastActiveWayPoint != ActiveWayPoint){
    SelectedWaypoint = ActiveWayPoint;
  }
  UnlockTaskData();
  //  UnlockFlightData();
}


void ResumeAbortTask(int set) {
  static int OldTask[MAXTASKPOINTS];
  static int OldActiveWayPoint= -1;
  static bool OldAATEnabled= false;
  int i;
  int lastActiveWayPoint;

  bool oldTaskAborted = TaskAborted;

  //  LockFlightData();
  LockTaskData();
  lastActiveWayPoint = ActiveWayPoint;

  if (set == 0)
    TaskAborted = !TaskAborted;
  else if (set > 0)
    TaskAborted = true;
  else if (set < 0)
    TaskAborted = false;

  if (oldTaskAborted != TaskAborted) {
    if (TaskAborted) {

      // save current task in backup

      for (i=0; i<MAXTASKPOINTS; i++) {
        OldTask[i]= Task[i].Index;
      }
      OldActiveWayPoint = ActiveWayPoint;
      if (AATEnabled) {
        OldAATEnabled = true;
      } else {
        OldAATEnabled = false;
      }

      // force new waypoint to be the closest
      ActiveWayPoint = -1;

      // force AAT off
      AATEnabled = false;

      // set MacCready
      if (!GlidePolar::AbortSafetyUseCurrent)  // 20060520:sgi added
        MACCREADY = min(MACCREADY,GlidePolar::AbortSafetyMacCready());

    } else {

      // reload backup task

      for (i=0; i<MAXTASKPOINTS; i++) {
        Task[i].Index = OldTask[i];
      }
      ActiveWayPoint = OldActiveWayPoint;
      AATEnabled = OldAATEnabled;

      RefreshTask();
    }
  }

  if (lastActiveWayPoint != ActiveWayPoint){
    SelectedWaypoint = ActiveWayPoint;
  }

  UnlockTaskData();
  //  UnlockFlightData();

}


#define TAKEOFFSPEEDTHRESHOLD (0.5*GlidePolar::Vminsink)

void DoAutoQNH(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static int done_autoqnh = 0;

  // Reject if already done
  if (done_autoqnh==10) return;

  // Reject if in IGC logger mode
  if (ReplayLogger::IsEnabled()) return;

  // Reject if no valid GPS fix
  if (Basic->NAVWarning) return;

  // Reject if no baro altitude
  if (!Basic->BaroAltitudeAvailable) return;

  // Reject if terrain height is invalid
  if (!Calculated->TerrainValid) return;

  if (Basic->Speed<TAKEOFFSPEEDTHRESHOLD) {
    done_autoqnh++;
  } else {
    done_autoqnh= 0; // restart...
  }

  if (done_autoqnh==10) {
    double fixaltitude = Calculated->TerrainAlt;

    QNH = FindQNH(Basic->BaroAltitude, fixaltitude);
    AirspaceQnhChangeNotify(QNH);
  }
  // TODO: Save last QNH setting, so it gets restored if
  // program is re-started in flight?
}


void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static int ntimeinflight = 0;
  static int ntimeonground = 0;

  if (Basic->Speed>1.0) {
    // stop system from shutting down if moving
    InterfaceTimeoutReset();
  }
  if (!Basic->NAVWarning) {
    if (Basic->Speed> TAKEOFFSPEEDTHRESHOLD) {
      ntimeinflight++;
      ntimeonground=0;
    } else {
      if ((Calculated->AltitudeAGL<300)&&(Calculated->TerrainValid)) {
        ntimeinflight--;
      } else if (!Calculated->TerrainValid) {
        ntimeinflight--;
      }
      ntimeonground++;
    }
  }

  ntimeinflight = min(60, max(0,ntimeinflight));
  ntimeonground = min(30, max(0,ntimeonground));

  // JMW logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds
  //
  // TODO: make this more robust by making use of terrain height data
  // if available

  if ((ntimeonground<=10)||(ReplayLogger::IsEnabled())) {
    // Don't allow 'OnGround' calculations if in IGC replay mode
    Calculated->OnGround = FALSE;
  }

  if (!Calculated->Flying) {
    // detect takeoff
    if (ntimeinflight>10) {
      Calculated->Flying = TRUE;
      InputEvents::processGlideComputer(GCE_TAKEOFF);
      // reset stats on takeoff
      ResetFlightStats(Basic, Calculated);

      Calculated->TakeOffTime= Basic->Time;

      // save stats in case we never finish
      memcpy(&Finish_Derived_Info, Calculated, sizeof(DERIVED_INFO));

    }
    if (ntimeonground>10) {
      Calculated->OnGround = TRUE;
      DoAutoQNH(Basic, Calculated);
    }
  } else {
    // detect landing
    if (ntimeinflight==0) {
      // have been stationary for a minute
      InputEvents::processGlideComputer(GCE_LANDING);

      // JMWX  restore data calculated at finish so
      // user can review flight as at finish line

      double flighttime = Calculated->FlightTime;
      double takeofftime = Calculated->TakeOffTime;
      memcpy(Calculated, &Finish_Derived_Info, sizeof(DERIVED_INFO));
      Calculated->FlightTime = flighttime;
      Calculated->TakeOffTime = takeofftime;

      Calculated->Flying = FALSE;
    }

  }
}


////////////






//////////

void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // nothing yet.
}

double EffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  int i;
  double LegCovered, LegDistance;

  if (Calculated->ValidFinish) return 0;

  if (ActiveWayPoint==0) return 0; // no e mc before start
  if (!Calculated->ValidStart) return 0;
  if (Calculated->TaskStartTime<0) return 0;

  if (!ValidTaskPoint(ActiveWayPoint) || !ValidTaskPoint(ActiveWayPoint-1)) return 0;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  LockTaskData();

  double mce = 0.0;
  double found_mce = 10.0;

  bool isfinal = ActiveIsFinalWaypoint();

  for (mce=0.1; mce<10.0; mce+= 0.1) {

    double time_total=0;
    double time_this;
    double bearing;

    double total_distance= 0; // used for testing

    w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
    w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
    w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;

    if (AATEnabled) {
      w1lat = Task[ActiveWayPoint].AATTargetLat;
      w1lon = Task[ActiveWayPoint].AATTargetLon;

      // TODO: Get best range point to here...
      w0lat = Task[ActiveWayPoint-1].AATTargetLat;
      w0lon = Task[ActiveWayPoint-1].AATTargetLon;

    }

    DistanceBearing(w0lat,
                    w0lon,
                    w1lat,
                    w1lon,
                    &LegDistance, &bearing);

    LegCovered = ProjectedDistance(w0lon, w0lat,
                                   w1lon, w1lat,
                                   Basic->Longitude,
                                   Basic->Latitude);

    total_distance += LegCovered;

    if ((StartLine==0) && (ActiveWayPoint==1)) {
      // Correct speed calculations for radius
      // JMW TODO: replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0.1,LegCovered-StartRadius);
    }

    GlidePolar::MacCreadyAltitude(mce,
                                  LegCovered,
                                  bearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  0, NULL,
                                  isfinal,
                                  &time_this);

    if (time_this<0) continue;

    time_total = time_this;

    // Now add distances for start to previous waypoint

    for(i=0;i<ActiveWayPoint-1;i++) {
      if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

      w1lat = WayPointList[Task[i].Index].Latitude;
      w1lon = WayPointList[Task[i].Index].Longitude;
      w0lat = WayPointList[Task[i+1].Index].Latitude;
      w0lon = WayPointList[Task[i+1].Index].Longitude;

      if (AATEnabled) {
        w1lat = Task[i].AATTargetLat;
        w1lon = Task[i].AATTargetLon;
        w0lat = Task[i+1].AATTargetLat;
        w0lon = Task[i+1].AATTargetLon;
      }

      DistanceBearing(w1lat,
                      w1lon,
                      w0lat,
                      w0lon,
                      &LegDistance, &bearing);

      if ((StartLine==0) && (i==1)) {
        // Correct speed calculations for radius
        // JMW TODO: replace this with more accurate version
        // LegDistance -= StartRadius;
        LegDistance = max(0.1,LegDistance-StartRadius);
      }

      total_distance += LegDistance;

      GlidePolar::MacCreadyAltitude(mce,
                                    LegDistance,
                                    bearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0, NULL,
                                    0,
                                    &time_this);

      if (time_this<0) {
        time_total= time_this;
      } else {
        if (time_total>=0) {
          time_total += time_this;
        }
      }

    }

    if (time_total<0) continue;

    // add time for climb from start height
    // note this is similar to what is achieved on the task speed achieved
    // calculator in TaskSpeed()

    double time_climb = (SpeedHeight(Basic, Calculated))/mce;
    time_total += time_climb;

    if (time_total<0) continue;

    double telapsed = Basic->Time-Calculated->TaskStartTime;

    if (time_total<telapsed) {
      found_mce = mce;
      break;
    }

  }

  UnlockTaskData();

  return found_mce;
}
