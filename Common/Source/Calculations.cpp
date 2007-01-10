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

WindAnalyser *windanalyser = NULL;
VegaVoice vegavoice;
OLCOptimizer olc;
ThermalLocator thermallocator;
AATDistance aatdistance;

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

#include "Port.h"

#include "WindZigZag.h"

static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void LoadCalculationsPersist(DERIVED_INFO *Calculated);
static void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      bool doadvance, bool doannounce);
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
static void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static bool  InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
static int  InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static int  InAATStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CalculateTeammateBearingRange(NMEA_INFO *Basic, DERIVED_INFO *Calculated) ;


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


//////////////////

static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  (void)Basic;
  if (Calculated->TaskDistanceToGo<=0) {
    return 0;
  }
  double hs = Calculated->TaskDistanceCovered/
    (Calculated->TaskDistanceCovered+Calculated->TaskDistanceToGo);
  double hx = (Calculated->NavAltitude-Calculated->TaskStartAltitude)*(1.0-hs);
  return hx;
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
DWORD lastTeamCodeUpdateTime = GetTickCount();


void RefreshTaskStatistics(void) {
  LockFlightData();
  LockTaskData();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  TaskSpeed(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  UnlockTaskData();
  UnlockFlightData();
}


int getFinalWaypoint() {
  int i;
  i=ActiveWayPoint;

  i++;
  while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
    {
      i++;
    }
  return i-1;
}

int FastLogNum = 0; // number of points to log at high rate


void AddSnailPoint(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!Calculated->Flying) return;

  SnailTrail[SnailNext].Latitude = (float)(Basic->Latitude);
  SnailTrail[SnailNext].Longitude = (float)(Basic->Longitude);
  SnailTrail[SnailNext].Time = Basic->Time;
  SnailTrail[SnailNext].FarVisible = true; // hasn't been filtered out yet.

  if (Basic->NettoVarioAvailable && !(ReplayLogger::IsEnabled())) {
    SnailTrail[SnailNext].Vario = (float)(Basic->NettoVario) ;
  } else {
    SnailTrail[SnailNext].Vario = (float)(Calculated->Vario) ;
  }
  SnailTrail[SnailNext].Colour = -1; // need to have colour calculated
  SnailTrail[SnailNext].Circling = Calculated->Circling;

  SnailNext ++;
  SnailNext %= TRAILSIZE;

}


int LoggerTimeStepCruise=5;
int LoggerTimeStepCircling=1;


void AnnounceWayPointSwitch(DERIVED_INFO *Calculated, bool doadvance) {
  if (ActiveWayPoint == 0) {
    InputEvents::processGlideComputer(GCE_TASK_START);
    // JMW cleared thermal climb average on task start
    flightstats.ThermalAverage.Reset();
  } else if (Calculated->ValidFinish) {
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


void DoLogging(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static double SnailLastTime=0;
  static double LogLastTime=0;
  static double StatsLastTime=0;
  static double OLCLastTime = 0;
  double dtLog = 5.0;
  double dtSnail = 2.0;
  double dtStats = 60.0;
  double dtOLC = 5.0;

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
  if (FastLogNum) {
    dtLog = 1.0;
  }

  // draw snail points more often in circling mode
  if (Calculated->Circling) {
    dtSnail = LoggerTimeStepCircling*1.0;
  } else {
    dtSnail = LoggerTimeStepCruise*1.0;
  }

  if (Basic->Time - LogLastTime >= dtLog) {
    if(LoggerActive) {
      double balt = -1;
      if (Basic->BaroAltitudeAvailable) {
        balt = Basic->BaroAltitude;
      } else {
        balt = Basic->Altitude;
      }
      LogPoint(Basic->Latitude , Basic->Longitude , Basic->Altitude,
               balt);
    }
    LogLastTime += dtLog;
    if (LogLastTime< Basic->Time-dtLog) {
      LogLastTime = Basic->Time-dtLog;
    }
    if (FastLogNum) FastLogNum--;
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
      flightstats.Altitude.
        least_squares_update(Basic->Time/3600.0, 
                             Calculated->NavAltitude);
      StatsLastTime += dtStats;
      if (StatsLastTime< Basic->Time-dtStats) {
        StatsLastTime = Basic->Time-dtStats;
      }
    }

    if (Basic->Time - OLCLastTime >= dtOLC) {
      bool restart;
      restart = olc.addPoint(Basic->Longitude, 
			     Basic->Latitude, 
			     Calculated->NavAltitude,
			     Calculated->WaypointBearing,
			     Basic->Time);
      
      if (restart && EnableOLC) {
	Calculated->ValidFinish = false;
	StartTask(Basic, Calculated, false, false);
	Calculated->ValidStart = true;
      }
      OLCLastTime += dtOLC;
    }
  }
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
	  windanalyser->slot_newEstimate(v, quality);
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
    windanalyser->slot_newEstimate(v, quality);
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
  if ((hwp<0)|| (hwp>=(int)NumberOfWayPoints) || (!WayPointList)) {
    return;
  }
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
  
  if (doannounce) {
    AnnounceWayPointSwitch(Calculated, doadvance);
  } else {
    if (doadvance) {
      ActiveWayPoint++;
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
}


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double maccready;

  if (!windanalyser) {
    windanalyser = new WindAnalyser(Basic, Calculated);

    // seed initial wind store with current conditions
    SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing, 1);

  }

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

  double t = DetectStartTime();
  if (t>0) {
    Calculated->FlightTime = t;
  }

  if ((Calculated->FinalGlide)
      ||(fabs(Calculated->TaskAltitudeDifference)>30)) {
    FinalGlideAlert(Basic, Calculated);
  }

  if (Calculated->AutoMacCready 
      && (!TaskAborted)) {
    DoAutoMacCready(Basic, Calculated);
  }

  TakeoffLanding(Basic, Calculated);
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
    InAATSector(Basic, Calculated);

    AATStats(Basic, Calculated);  
    TaskStatistics(Basic, Calculated, MACCREADY);
    TaskSpeed(Basic, Calculated, MACCREADY);

#ifdef DEBUG
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
  if (Basic->AirspeedAvailable) {
    Calculated->EnergyHeight = 
      max(0, Basic->IndicatedAirspeed*Basic->IndicatedAirspeed
       -GlidePolar::Vbestld*GlidePolar::Vbestld)/(9.81*2.0);
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


void SwitchZoomClimb(bool isclimb, bool left) {
  
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
  }
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    windanalyser->slot_newFlightMode(left, 0);
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
        SwitchZoomClimb(true, LEFT);
        InputEvents::processGlideComputer(GCE_FLIGHTMODE_CLIMB);
      }
    } else {
      // nope, not turning, so go back to cruise
      MODE = CRUISE;
    }
    break;
  case CLIMB:
    if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
      windanalyser->slot_newSample();
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
        
        SwitchZoomClimb(false, LEFT);
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
    windanalyser->slot_Altitude();
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

  if(ActiveWayPoint >=0)
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
  if((ActiveWayPoint >=0)&&(WayPointList))
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

      Calculated->NextAltitudeDifference = 
        Calculated->NavAltitude - (Calculated->NextAltitudeRequired
                           + WayPointList[Task[ActiveWayPoint].Index].Altitude)         + Calculated->EnergyHeight;            
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
    }
  UnlockTaskData();
  //  UnlockFlightData();
}

int InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double AircraftBearing;

  if (!WayPointList) return FALSE;

  if(SectorType==0)
    {
      if(Calculated->WaypointDistance < SectorRadius)
        {
          return TRUE;
        }
    }
  if (SectorType>0)
    {
      DistanceBearing(WayPointList[Task[ActiveWayPoint].Index].Latitude,   
                      WayPointList[Task[ActiveWayPoint].Index].Longitude,
                      Basic->Latitude , 
                      Basic->Longitude,
                      NULL, &AircraftBearing);
      
      AircraftBearing = AircraftBearing - Task[ActiveWayPoint].Bisector ;
      while (AircraftBearing<-180) {
        AircraftBearing+= 360;
      }
      while (AircraftBearing>180) {
        AircraftBearing-= 360;
      }

      if (SectorType==2) {
        // JMW added german rules
        if (Calculated->WaypointDistance<500) {
          return TRUE;
        }
      }
      if( (AircraftBearing >= -45) && (AircraftBearing <= 45))
        {
          if (SectorType==1) {
            if(Calculated->WaypointDistance < 20000)
              {
                return TRUE;
              }
          } else {
            // JMW added german rules
            if(Calculated->WaypointDistance < 10000)
              {
                return TRUE;
              }
          }
        }
    }       
  return FALSE;
}

int InAATTurnSector(double longitude, double latitude,
                    int thepoint)
{
  double AircraftBearing;

  if (!WayPointList) return FALSE;

  double distance;
  DistanceBearing(WayPointList[Task[thepoint].Index].Latitude,
                  WayPointList[Task[thepoint].Index].Longitude,
                  latitude,
                  longitude,
                  &distance, &AircraftBearing);

  if(Task[thepoint].AATType ==  CIRCLE) {
    if(distance < Task[thepoint].AATCircleRadius)
      {
        return TRUE;
      }
  } else if(distance < Task[thepoint].AATSectorRadius) {

    if(Task[thepoint].AATStartRadial 
       < Task[thepoint].AATFinishRadial ) {
      if(
         (AircraftBearing > Task[thepoint].AATStartRadial)
         &&
         (AircraftBearing < Task[thepoint].AATFinishRadial)
         )
        return TRUE;
    }
                
    if(Task[thepoint].AATStartRadial 
       > Task[thepoint].AATFinishRadial ) {
      if(
         (AircraftBearing > Task[thepoint].AATStartRadial)
         ||
         (AircraftBearing < Task[thepoint].AATFinishRadial)
         )
        return TRUE;
    }
  }
  return FALSE;
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


int InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                   int i)
{
  static int LastInSector = FALSE;
  double AircraftBearing;
  double FirstPointDistance;

  if (!WayPointList) return FALSE;

  if (!ValidFinish(Basic, Calculated)) return FALSE;

  // Finish invalid
  if(Task[i].Index == -1)
    {
      return FALSE;
    }

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
      return inrange;
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
        return TRUE;
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

  return FALSE;
}


bool InStartSector_Internal(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                           int Index, 
                           double OutBound, 
                           bool &LastInSector)
{
  (void)Calculated;
  if (!WayPointList) return false;
  if(Index == -1) {
    return false;
  }

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
  if (!inrange) {
    LastInSector = false;
  }

  if(StartLine==0) { 
    // Start Circle 
    if (inrange) {
      LastInSector = true;
    }
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

    if (LastInSector) {
      // previously approaching the start line
      if (!approaching) {
        // now moving away from start line
        LastInSector = false;
        return true;
      }
    } else {
      if (approaching) {
        // now approaching the start line
        LastInSector = true;
      }
    }
    
  } else {
    LastInSector = false;
  }

  return false;
}


bool InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int &index)
{
  static bool LastInSector = false;
  
  bool isInSector= false;
  bool retval;

  if (!Calculated->Flying) {
    return false;
  }

  if ((ActiveWayPoint>0) 
      &&(Task[ActiveWayPoint+1].Index < 0)) {
    // don't detect start if finish is selected
    return false;
  }

  retval = InStartSector_Internal(Basic, Calculated, 
                                  Task[0].Index, Task[0].OutBound,
                                  LastInSector);
  isInSector = retval;
  if (retval) {
    index = Task[0].Index;
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
        if (retval) {
          index = StartPoints[i].Index;
        }
      }
    }
  }

  if (isInSector) {
    // change start waypoint to the one we are in the sector of
    if (Task[0].Index != index) {
      Task[0].Index = index;
      RefreshTask();
    }
  }

  return isInSector;
}


bool ReadyToAdvance(DERIVED_INFO *Calculated) {

  // 0: Manual
  // 1: Auto
  // 2: Arm
  // 3: Arm start

  if (!Calculated->Flying) 
    return false;

  if (AutoAdvance==1) {  // auto
    AdvanceArmed = false;
    return true;
  }
  if ((AutoAdvance==2)&&(AdvanceArmed)) { // arm
    AdvanceArmed = false;
    return true;
  }
  if (AutoAdvance==3) { // arm start
    if (ActiveWayPoint>0) { // past start
      AdvanceArmed = false;
      return true;
    }
    if (AdvanceArmed) { // on start
      AdvanceArmed = false;
      return true;
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




void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;
  static int LastStartSector = -1;

  if(AATEnabled)
    return;

  //  LockFlightData();
  LockTaskData();

  Calculated->IsInSector = false;

  if(ActiveWayPoint == 0) {
    if (Calculated->Flying) {
      Calculated->ValidFinish = false;
    }
    if (InStartSector(Basic,Calculated,LastStartSector)) {
      Calculated->IsInSector = true;
      StartSectorEntered = TRUE;
      // TODO monitor start speed throughout time in start sector
    } else {
      if((StartSectorEntered == TRUE)&&
	 (ValidStart(Basic, Calculated))) {
	if(ActiveWayPoint < MAXTASKPOINTS) {
	  if(Task[ActiveWayPoint+1].Index >= 0) {
	    Calculated->ValidStart = true;
	    if (ReadyToAdvance(Calculated)) {
	      StartTask(Basic,Calculated, true, true);
	    }
            if (Calculated->Flying) {
              Calculated->ValidFinish = false;
            }
	    StartSectorEntered = FALSE;
	    // JMW TODO: This causes Vaverage to go bonkers
	    // if the user has already passed the start
	    // but selects the start
	    
	    // Note: pilot must have armed advance
	    // for the start to be registered
	  }
	}
      }
    }
  } else if(ActiveWayPoint >0) {
    // This allows restart if within 10 minutes of previous start
    if (InStartSector(Basic,Calculated,LastStartSector)) {
      Calculated->IsInSector = true;
      if(Basic->Time - Calculated->TaskStartTime < 600) {
	if (ReadyToAdvance(Calculated)) {
	  AdvanceArmed = false;   
	  Calculated->TaskStartTime = 0;
	  ActiveWayPoint = 0;
	  AnnounceWayPointSwitch(Calculated, false);
	  StartSectorEntered = TRUE;
	}
        if (Calculated->Flying) {
          Calculated->ValidFinish = false;
        }
      }
    } 
    if(ActiveWayPoint < MAXTASKPOINTS) {
      if(Task[ActiveWayPoint+1].Index >= 0) {
	if(InTurnSector(Basic,Calculated)) {
	  Calculated->IsInSector = true;
	  Calculated->LegStartTime = Basic->Time;
          if (Calculated->Flying) {
            Calculated->ValidFinish = false;
          }
	  if (ReadyToAdvance(Calculated)) {
	    ActiveWayPoint++;
	    AnnounceWayPointSwitch(Calculated, false);
	  }
	  UnlockTaskData();
	  //	  UnlockFlightData();
	  
	  return;
	}
      } else {
	if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
	  Calculated->IsInSector = true;
	  if (!Calculated->ValidFinish) {
            Calculated->ValidFinish = true;
            AnnounceWayPointSwitch(Calculated, false);
	  }
	} else {
	  //      TaskFinished = false;
	}
      }
    }
  }                   
  UnlockTaskData();
  //  UnlockFlightData();
}


void InAATSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static BOOL StartSectorEntered = FALSE;
  static int LastStartSector = -1;

  if(!AATEnabled)
    return;

  //  LockFlightData();
  LockTaskData();

  Calculated->IsInSector = false;

  if(ActiveWayPoint == 0) {
    if (Calculated->Flying) {
      Calculated->ValidFinish = false;
    }
    if(InStartSector(Basic, Calculated, LastStartSector)) {
      aatdistance.AddPoint(Basic->Longitude,
                           Basic->Latitude,
                           0);
      Calculated->IsInSector = true;
      StartSectorEntered = TRUE;
    } else {
      if((StartSectorEntered == TRUE)&&
         (ValidStart(Basic, Calculated))) {
        if(ActiveWayPoint < MAXTASKPOINTS) {
          if(Task[ActiveWayPoint+1].Index >= 0) {
            Calculated->ValidStart = true;
            if (Calculated->Flying) {
              Calculated->ValidFinish = false;
            }
            if (ReadyToAdvance(Calculated)) {
              StartTask(Basic, Calculated, true, true);
            }
            StartSectorEntered = FALSE;
            // JMW TODO: make sure this is valid for manual start
          }
        }
      } 
    }
  } else if(ActiveWayPoint >0) {
    if(InStartSector(Basic, Calculated, LastStartSector)) {
      Calculated->IsInSector = true;
      if(Basic->Time - Calculated->TaskStartTime < 600) {
        // this allows restart if returned to start sector before
        // 10 minutes after task start
        if (ReadyToAdvance(Calculated)) {
          AdvanceArmed = false;     
          Calculated->TaskStartTime = 0;
          ActiveWayPoint = 0;
          AnnounceWayPointSwitch(Calculated, false);
          StartSectorEntered = TRUE;
        }
        if (Calculated->Flying) {
          Calculated->ValidFinish = false;
        }
      }
    }
    if(ActiveWayPoint < MAXTASKPOINTS) {

      if((ActiveWayPoint>1)&& (Task[ActiveWayPoint].Index >= 0)) {
        if(InAATTurnSector(Basic->Longitude,
                           Basic->Latitude, ActiveWayPoint-1)) {

          // Still within previous sector
          aatdistance.AddPoint(Basic->Longitude,
                               Basic->Latitude,
                               ActiveWayPoint-1);
        }
      }

      if(Task[ActiveWayPoint+1].Index >= 0) {
        if(InAATTurnSector(Basic->Longitude,
                           Basic->Latitude, ActiveWayPoint)) {

          aatdistance.AddPoint(Basic->Longitude,
                               Basic->Latitude,
                               ActiveWayPoint);
          Calculated->IsInSector = true;
          Calculated->LegStartTime = Basic->Time;
        }
        if (aatdistance.HasEntered(ActiveWayPoint)) {
          
          if (ReadyToAdvance(Calculated)) {
            AdvanceArmed = false;             
            AnnounceWayPointSwitch(Calculated, true);
          }
          if (Calculated->Flying) {
            Calculated->ValidFinish = false;
          }
          UnlockTaskData();
          //            UnlockFlightData();
          return;
        }
      } else {
        if (InFinishSector(Basic,Calculated, ActiveWayPoint)) {
          Calculated->IsInSector = true;
          aatdistance.AddPoint(Basic->Longitude,
                               Basic->Latitude,
                               ActiveWayPoint);
          if (!Calculated->ValidFinish) {
            Calculated->ValidFinish = true;
            AnnounceWayPointSwitch(Calculated, false);
          }
        } else {
          //      TaskFinished = false;
        }
      }       
    }
  }
  UnlockTaskData();
  //  UnlockFlightData();
}


//////////////////////////////////////////////////////





///////////////////////////////////
#include "RasterTerrain.h"

RasterTerrain terrain_dem_graphics;
RasterTerrain terrain_dem_calculations;

void OpenTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::OpenTerrain();
  terrain_dem_graphics.ClearTerrainCache();
  terrain_dem_calculations.ClearTerrainCache();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


void CloseTerrain(void) {
  LockTerrainDataCalculations();
  LockTerrainDataGraphics();
  RasterTerrain::CloseTerrain();
  UnlockTerrainDataCalculations();
  UnlockTerrainDataGraphics();
}


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

void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  int i;
  double LegTime, LegDistance, LegBearing, LegAltitude, TotalTime, 
    TotalDistance;
  static double LastTime = 0;
  static double v2last = 0;
  static double t1last = 0;

  if (!WayPointList) return;

  if (Calculated->ValidFinish) return;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;
  double Vfinal=0;

  //  LockFlightData();
  LockTaskData();

  // Calculate altitude required from start of task

  bool isfinal=true;
  int ifinal=0;
  LegAltitude = 0;
  double TotalAltitude = 0;
  TotalTime = 0; TotalDistance = 0;
  bool invalid = false;
  for(i=MAXTASKPOINTS-2;i>=0;i--) {
    if (Task[i].Index<0) continue;
    if (Task[i+1].Index<0) continue;
    
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

    TotalDistance += LegDistance;
    
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
      invalid = true;
    } else {
      TotalTime += LegTime;
    }
    if (isfinal) {
      ifinal = i+1;
      if (LegTime>0) {
        Vfinal = LegDistance/LegTime;
      }
    }
    isfinal = false;
  }

  if ((ifinal==0)||(invalid)) {
    UnlockTaskData();
    return;
  }

  TotalAltitude += SAFETYALTITUDEARRIVAL 
    + WayPointList[Task[ifinal].Index].Altitude;
  
  Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;

  // now calculate the stats

  Calculated->TaskSpeedAchieved = 0;

  if((ActiveWayPoint >=1)&&(!invalid)) {

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
    
    double h0 = Calculated->TaskAltitudeRequiredFromStart;
    // total height required from start

//    double h2 = Calculated->TaskAltitudeRequired-Calculated->NavAltitude;
    // actual height required

    double h1 = Calculated->NavAltitude-SAFETYALTITUDEARRIVAL 
      + WayPointList[Task[ifinal].Index].Altitude;
    // height above target

    // equivalent speed
    double v2;

    // JB's task speed...
    if ((t1>0)&&(d1>0)) {
      v2 = d1/t1;
      double hx = SpeedHeight(Basic, Calculated);
      double t1mod = t1-hx/max(0.1,MACCREADY);
      // only valid if flown for 5 minutes or more
      if (t1mod>300.0) {
        Calculated->TaskSpeedAchieved = d1/t1mod;
      } else {
        Calculated->TaskSpeedAchieved = d1/t1;
      }
    }

    if ((t1>0)&&(d0>0)&&(t0>0)&&(h1>0)&&(h0>0)&&(Vfinal>0)) {

      d2 = d1+min(dr, d0*(h1/h0));
      // distance that can be usefully final glided from here

      // time at end of final glide
      t2 = t1+(d2-d1)/Vfinal;

      // average speed to end of final glide from here
      v2 = d2/t2;

      Calculated->TaskSpeed = max(d1/t1,v2);
      
      if(Basic->Time <= LastTime) {
        LastTime = Basic->Time;
      } else {
        if (Basic->Time-LastTime >=1.0) {

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
    }
  }

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

  if (Calculated->ValidFinish) return;

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
      
      Calculated->LegDistanceCovered = LegCovered;
      Calculated->TaskDistanceCovered = LegCovered;   
      Calculated->LegDistanceToGo = LegToGo;
      
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
          if (Task[i].Index<0) continue;
          if (Task[i+1].Index<0) continue;

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
  
  ////////////

  // Calculate Final Glide To Finish
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;

//  double FinalAltitude = 0;
  int FinalWayPoint = getFinalWaypoint();

  if(ActiveWayPoint >=0)
    {
      i=ActiveWayPoint;

      // update final glide mode status
      if (
          ((ActiveWayPoint == FinalWayPoint)&&(ActiveWayPoint>=0))
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
                                      &(Calculated->LegTimeToGo)
                                      );

      LegAltitude0 = 
        GlidePolar::MacCreadyAltitude(0, 
                                      LegToGo, 
                                      LegBearing, 
                                      Calculated->WindSpeed, 
                                      Calculated->WindBearing,
                                      0,
                                      0,
                                      (Calculated->FinalGlide==1),
                                      &LegTime0
                                      );
      if (LegTime0>=1e5) {
        LegAltitude0 = LegAltitude;
      }

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

      if (Calculated->FinalGlide) {
        double dha = Calculated->NavAltitude - 
          LegAltitude - SAFETYALTITUDEARRIVAL;
        if (dha<0) {
          Calculated->LegTimeToGo += -dha/max(0.1,MACCREADY);
        }
      }

      TaskAltitudeRequired = LegAltitude;
      TaskAltitudeRequired0 = LegAltitude0;
      Calculated->TaskDistanceToGo = LegToGo;
      Calculated->TaskTimeToGo = Calculated->LegTimeToGo;

      double LegTimeToGo;

      if((Calculated->NavAltitude - LegAltitude - SAFETYALTITUDEARRIVAL) > 0)
        {
          Calculated->LDNext = 
            Calculated->TaskDistanceToGo 
            / (Calculated->NavAltitude 
               - LegAltitude - SAFETYALTITUDEARRIVAL)  ;
        }
      else
        {
          Calculated->LDNext = 999;
        }

      i++;
      while((Task[i].Index != -1) && (i<MAXTASKPOINTS) && (!TaskAborted))
        {

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
                              (i==FinalWayPoint),
                              &LegTimeToGo);

          LegAltitude0 = GlidePolar::
            MacCreadyAltitude(0, 
                              LegDistance, LegBearing, 
                              Calculated->WindSpeed, 
                              Calculated->WindBearing, 
                              0, 0,
                              (i==FinalWayPoint),
                              &LegTime0);

          if (LegTime0>=1e5) {
            LegAltitude0 = LegAltitude;
          }

          TaskAltitudeRequired += LegAltitude;
          TaskAltitudeRequired0 += LegAltitude0;

          if (i==FinalWayPoint) {
            double dha = Calculated->NavAltitude - 
              LegAltitude - SAFETYALTITUDEARRIVAL;
            if (dha<0) {
              LegTimeToGo += -dha/max(0.1,MACCREADY);
            }
          }

          Calculated->TaskDistanceToGo += LegDistance;
          Calculated->TaskTimeToGo += LegTimeToGo;      
                        
          i++;
        }

      Calculated->TaskAltitudeRequired = TaskAltitudeRequired 
        + SAFETYALTITUDEARRIVAL + WayPointList[Task[i-1].Index].Altitude;

      TaskAltitudeRequired0 += SAFETYALTITUDEARRIVAL 
        + WayPointList[Task[i-1].Index].Altitude;

      Calculated->TaskAltitudeDifference = Calculated->NavAltitude 
        - (Calculated->TaskAltitudeRequired) 
        + Calculated->EnergyHeight;

      Calculated->TaskAltitudeDifference0 = Calculated->NavAltitude 
        - (TaskAltitudeRequired0) 
        + Calculated->EnergyHeight;

      if (Calculated->TaskAltitudeDifference>0) {
        if (!fgtt && fgttnew) {
          InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
          fgtt = true;
        }
      } else {
        fgtt = false;
      }

      double temp = Calculated->NavAltitude 
        - WayPointList[Task[i-1].Index].Altitude  
        + Calculated->EnergyHeight - SAFETYALTITUDEARRIVAL;

      if(  (temp) > 0)
        {
          Calculated->LDFinish = Calculated->TaskDistanceToGo /(temp);
        }
      else
        {
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

    if (Task[ActiveWayPoint].Index != -1) {

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
//  double TaskAltitudeRequired = 0;

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

  if(ActiveWayPoint >=0)
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

      if(Task[ActiveWayPoint].AATType == CIRCLE)
        {
          MaxDistance = LegToGo + (Task[i].AATCircleRadius * 2);
          MinDistance = LegToGo - (Task[i].AATCircleRadius * 2);
        }
      else
        {
          MaxDistance = LegToGo + (Task[ActiveWayPoint].AATSectorRadius * 2);
          MinDistance = LegToGo;
        }

      TargetDistance = TargetLegToGo;

      i++;
      while((Task[i].Index != -1) && (i<MAXTASKPOINTS))
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



//////////////////////////////////////////////////////////
// Final glide through terrain and footprint calculations

void ExitFinalGlideThroughTerrain() {
  UnlockTerrainDataCalculations();
}


double FinalGlideThroughTerrain(double bearing, NMEA_INFO *Basic, 
                                DERIVED_INFO *Calculated,
                                double *retlat, double *retlon,
                                double maxrange,
				bool *outofrange) 
{
  double irange = GlidePolar::MacCreadyAltitude(MACCREADY, 
						// should this be zero?
						1.0, bearing, 
						Calculated->WindSpeed, 
						Calculated->WindBearing, 
						0, 0, true, 0);
  if (retlat && retlon) {
    *retlat = Basic->Latitude;
    *retlon = Basic->Longitude;
  }
  *outofrange = false;

  if ((irange<=0.0)||(Calculated->NavAltitude<=0)) {
    // can't make progress in this direction at the current windspeed/mc
    return 0;
  }

  double glidemaxrange = Calculated->NavAltitude/irange;

  // returns distance one would arrive at altitude in straight glide
  // first estimate max range at this altitude
  double lat, lon;
  double latlast, lonlast;
  double h=0.0, dh=0.0;
//  int imax=0;
  double dhlast=0;
  double altitude;
 
  LockTerrainDataCalculations();

  // calculate terrain rounding factor


  FindLatitudeLongitude(Basic->Latitude, Basic->Longitude, 0, 
                        glidemaxrange/NUMFINALGLIDETERRAIN, &lat, &lon);

  double Xrounding = fabs(lon-Basic->Longitude)/2;
  double Yrounding = fabs(lat-Basic->Latitude)/2;
  terrain_dem_calculations.SetTerrainRounding(Xrounding, Yrounding);

  altitude = Calculated->NavAltitude;
  h =  terrain_dem_calculations.GetTerrainHeight(lat, lon); 
  dh = altitude - h - SAFETYALTITUDETERRAIN;
  if (dh<0) {
    ExitFinalGlideThroughTerrain();
    return 0;
  }

  latlast = Basic->Latitude;
  lonlast = Basic->Longitude;

  // find grid
  double dlat, dlon;
  FindLatitudeLongitude(Basic->Latitude, Basic->Longitude, bearing, 
                        glidemaxrange, &dlat, &dlon);
  dlat -= Basic->Latitude;
  dlon -= Basic->Longitude;

  for (int i=0; i<=NUMFINALGLIDETERRAIN; i++) {
    double fi = (i*1.0)/NUMFINALGLIDETERRAIN;
    // fraction of glidemaxrange

    if ((maxrange>0)&&(fi>maxrange/glidemaxrange)) {
      // early exit
      *outofrange = true;
      ExitFinalGlideThroughTerrain();
      return maxrange;
    }

    altitude = (1.0-fi)*Calculated->NavAltitude;

    // find lat, lon of point of interest

    lat = Basic->Latitude+dlat*fi;
    lon = Basic->Longitude+dlon*fi;

    // find height over terrain
    h =  terrain_dem_calculations.GetTerrainHeight(lat, lon); 

    dh = altitude - h - SAFETYALTITUDETERRAIN;

    if ((dh<=0)&&(dhlast>=0)) {
      double f;
      if (dhlast-dh>0) {
        f = (-dhlast)/(dh-dhlast);
      } else {
        f = 0.0;
      }
      if (retlat && retlon) {
        *retlat = latlast*(1.0-f)+lat*f;
        *retlon = lonlast*(1.0-f)+lon*f;
      }
      ExitFinalGlideThroughTerrain();
      double distance;
      DistanceBearing(Basic->Latitude, Basic->Longitude, lat, lon,
                      &distance, NULL);
      return distance;
    }
    dhlast = dh;
    latlast = lat;
    lonlast = lon;
  }

  *outofrange = true;
  ExitFinalGlideThroughTerrain();
  return glidemaxrange;
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
    if (ActiveWayPoint>=0) {
      if (Task[ActiveWayPoint].Index>=0) {
        if (SortedLandableIndex[i] == Task[ActiveWayPoint].Index) {
          foundActiveWayPoint = i;
        }
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
    if (ActiveWayPoint>=0){
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
    if (Task[i].Index>= 0) {
      WayPointList[Task[i].Index].InTask = true;
    }
  }

  if (newclosest) {
    if ((Task[0].Index != lastclosest) && (Task[0].Index>=0)) {
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
      if (StartPoints[i].Active && (StartPoints[i].Index>=0)) {
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
    }
    if (ntimeonground>10) {
      Calculated->OnGround = TRUE;
      DoAutoQNH(Basic, Calculated);
    }
  } else {
    // detect landing
    if (ntimeinflight==0) {
      // have been stationary for a minute
      Calculated->Flying = FALSE;
      InputEvents::processGlideComputer(GCE_LANDING);
    }

  }
}


////////////

double PirkerAnalysis(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double bearing,
                      double GlideSlope) {


//  bool maxfound = false;
//  bool first = true;
  double pmc = 0.0;
  double htarget = GlideSlope;
  double h;
  double dh= 1.0;
  double pmclast = 5.0;
  double dhlast = -1.0;
  double pmczero = 0.0;
 
  (void)Basic;

  while (pmc<10.0) {

    h = GlidePolar::MacCreadyAltitude(pmc, 
                                      1.0, // unit distance
				      bearing, 
                                      Calculated->WindSpeed, 
                                      Calculated->WindBearing, 
                                      0, 0, true, 0);

    dh = (htarget-h); 
    // height difference, how much we have compared to 
    // how much we need at that speed.
    //   dh>0, we can afford to speed up

    if (dh==dhlast) {
      // same height, must have hit max speed.
      if (dh>0) {
        return pmclast;
      } else {
        return 0.0;
      }
    }

    if ((dh<=0)&&(dhlast>0)) {
      double f = (-dhlast)/(dh-dhlast);
      pmczero = pmclast*(1.0-f)+f*pmc;
      return pmczero;
    }
    dhlast = dh;
    pmclast = pmc;

    pmc += 0.5;
  }
  return -1.0; // no solution found, unreachable without further climb
}


double MacCreadyTimeLimit(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			  double bearing,
			  double timeremaining,
			  double hfinal) {

  // find highest Mc to achieve greatest distance in remaining time and height
  (void)Basic;

  double timetogo;
  double mc;
  double mcbest = 0.0;
  double dbest = 0.0;

  for (mc=0; mc<10.0; mc+= 0.1) {

    double hunit = GlidePolar::MacCreadyAltitude(mc, 
						 1.0, // unit distance
						 bearing, 
						 Calculated->WindSpeed, 
						 Calculated->WindBearing,
						 NULL,
						 NULL,
						 1, // final glide
						 &timetogo);
    if (timetogo>0) {
      double p = timeremaining/timetogo;    
      double hspent = hunit*p;    
      double dh = Calculated->NavAltitude-hspent-hfinal;    
      double d = 1.0*p;
      
      if ((d>dbest) && (dh>=0)) {
	mcbest = mc;
      }
    }
  }
  return mcbest;
}



void CalculateOwnTeamCode(NMEA_INFO *Basic, DERIVED_INFO *Calculated) 
{
  if (!WayPointList) return;
  if (TeamCodeRefWaypoint < 0) return;
  //if (lastTeamCodeUpdateTime + 10000 > GetTickCount()) return;

	
  double distance = 0;
  double bearing = 0;
  TCHAR code[10];
	
  lastTeamCodeUpdateTime = GetTickCount();

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
	
  // TODO: ask Lars why this one
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

  wcsncpy(Calculated->OwnTeamCode, code, 5);
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

  // TODO: ask Lars why this one

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

      // TODO ....change the result of CalcTeammateBearingRange to do this !
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

static TCHAR szCalculationsPersistFileName[MAX_PATH]= TEXT("\0");

void DeleteCalculationsPersist(void) {
  DeleteFile(szCalculationsPersistFileName);
}

void LoadCalculationsPersist(DERIVED_INFO *Calculated) {
  if (szCalculationsPersistFileName[0]==0) {
#ifdef GNAV
    LocalPath(szCalculationsPersistFileName,
              TEXT("persist/xcsoar-persist.log"));    
#else
    LocalPath(szCalculationsPersistFileName,
              TEXT("xcsoar-persist.log"));    
#endif
  }

  StartupStore(TEXT("LoadCalculationsPersist\r\n"));

  HANDLE hFile;
  DWORD dwBytesWritten;
  DWORD size, sizein;
  hFile = CreateFile(szCalculationsPersistFileName,
                     GENERIC_READ,0,(LPSECURITY_ATTRIBUTES)NULL,
                     OPEN_EXISTING,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile!=INVALID_HANDLE_VALUE ) {
    size = sizeof(DERIVED_INFO);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,Calculated,size,&dwBytesWritten,(OVERLAPPED*)NULL);

    size = sizeof(Statistics);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,&flightstats,size,&dwBytesWritten,(OVERLAPPED*)NULL);

    size = sizeof(OLCData);
    ReadFile(hFile,&sizein,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    if (sizein != size) { CloseHandle(hFile); return; }
    ReadFile(hFile,&olc.data,size,&dwBytesWritten,(OVERLAPPED*)NULL);   

    CloseHandle(hFile);
  }
}


void SaveCalculationsPersist(DERIVED_INFO *Calculated) {
  HANDLE hFile;
  DWORD dwBytesWritten;
  DWORD size;

  StartupStore(TEXT("SaveCalculationsPersist\r\n"));

  hFile = CreateFile(szCalculationsPersistFileName,
                     GENERIC_WRITE,0,(LPSECURITY_ATTRIBUTES)NULL,
                     CREATE_ALWAYS,FILE_ATTRIBUTE_NORMAL,NULL);
  if(hFile!=INVALID_HANDLE_VALUE ) {
    size = sizeof(DERIVED_INFO);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,Calculated,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    size = sizeof(Statistics);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,&flightstats,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    size = sizeof(OLCData);
    WriteFile(hFile,&size,sizeof(DWORD),&dwBytesWritten,(OVERLAPPED*)NULL);
    WriteFile(hFile,&olc.data,size,&dwBytesWritten,(OVERLAPPED*)NULL);
    CloseHandle(hFile);
  }

}


//////////

double EffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  int i;
  double LegCovered, LegDistance;

  if (!WayPointList) return 0;
  if (Calculated->ValidFinish) return 0;
  if (ActiveWayPoint <1) return 0;
  if (Calculated->TaskStartTime<0) return 0;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  LockTaskData();

  double mce = 0.0;
  double found_mce = 0.0;

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
    
//    double altreqfinal = 
      GlidePolar::MacCreadyAltitude(mce,
                                    LegCovered,
                                    bearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0, NULL, 
                                    isfinal, 
                                    &time_this);
    
    if (time_this<0) continue;
    
    /*
      if (isfinal) {
      double dha = Calculated->NavAltitude - 
      altreqfinal - SAFETYALTITUDEARRIVAL;
      if (dha<0) {
      time_this += -dha/max(0.1, mce);
      }
      }
    */
    
    time_total = time_this;
    
    // Now add distances for start to previous waypoint
    
    for(i=0;i<ActiveWayPoint-1;i++) {
      if (Task[i].Index<0) continue;
      if (Task[i+1].Index<0) continue;
      
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
