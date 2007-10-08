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
#include "compatibility.h"
#ifdef OLDPPC
#include "XCSoarProcess.h"
#else
#include "Process.h"
#endif
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
int  AutoMcMode = 0;
bool EnableFAIFinishHeight = false;

// 0: Final glide only
// 1: Set to average if in climb mode
// 2: Average if in climb mode, final glide in final glide mode


static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void PercentCircling(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void MaxHeightGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void EnergyHeightNavAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double maccready);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static bool  InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, BOOL* CrossedStart);
static bool  InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
static bool  InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int i);
//static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void PredictNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

extern void ConditionMonitorsUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

#ifdef DEBUG
#define DEBUGTASKSPEED
#endif

//////////////////

int getFinalWaypoint() {
  int i;
  i=max(-1,min(MAXTASKPOINTS,ActiveWayPoint));
  if (TaskAborted) {
    return i;
  }

  i++;
  LockTaskData();
  while((i<MAXTASKPOINTS) && (Task[i].Index != -1))
    {
      i++;
    }
  UnlockTaskData();
  return i-1;
}


static bool ActiveIsFinalWaypoint() {
  return (ActiveWayPoint == getFinalWaypoint());
}

static void CheckTransitionFinalGlide(NMEA_INFO *Basic,
                                      DERIVED_INFO *Calculated) {
  int FinalWayPoint = getFinalWaypoint();
  // update final glide mode status
  if (((ActiveWayPoint == FinalWayPoint)
       ||(ForceFinalGlide))
      && (ValidTaskPoint(ActiveWayPoint))) {

    if (Calculated->FinalGlide == 0)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE);
    Calculated->FinalGlide = 1;
  } else {
    if (Calculated->FinalGlide == 1)
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_CRUISE);
    Calculated->FinalGlide = 0;
  }

}


static void CheckForceFinalGlide(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // Auto Force Final Glide forces final glide mode
  // if above final glide...
  if (TaskAborted) {
    ForceFinalGlide = false;
  } else {
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
  }
}


double FAIFinishHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int wp) {
  int FinalWayPoint = getFinalWaypoint();
  if (wp== -1) {
    wp = FinalWayPoint;
  }
  double wp_alt;
  if(ValidTaskPoint(wp)) {
    wp_alt = WayPointList[Task[wp].Index].Altitude;
  } else {
    wp_alt = 0;
  }

  if (!TaskAborted && (wp==FinalWayPoint)) {
    if (EnableFAIFinishHeight && !AATEnabled) {
      return max(max(FinishMinHeight, SAFETYALTITUDEARRIVAL)+ wp_alt,
                 Calculated->TaskStartAltitude-1000.0);
    } else {
      return max(FinishMinHeight, SAFETYALTITUDEARRIVAL)+wp_alt;
    }
  } else {
    return wp_alt + SAFETYALTITUDEARRIVAL;
  }
}


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
  double dh_finish = Calculated->NavAltitude-
    FAIFinishHeight(Basic, Calculated, -1);

  // Excess height
  return dh_start*(1.0-d_fraction)+dh_finish*(d_fraction);
}


void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double bearing, distance;
  double lat, lon;
  bool out_of_range;

  // estimate max range (only interested in at most one screen distance away)
  double mymaxrange = MapWindow::GetApproxScreenRange();

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    //    bearing = -90+i*180/NUMTERRAINSWEEPS+Basic->TrackBearing;
    bearing = i*360/NUMTERRAINSWEEPS;
    distance = FinalGlideThroughTerrain(bearing,
                                        Basic,
                                        Calculated, &lat, &lon,
                                        mymaxrange, &out_of_range);
    if (out_of_range) {
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


double LowPassFilter(double y_last, double x_in, double fact) {
  return (1.0-fact)*y_last+(fact)*x_in;
}


void SpeedToFly(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double n;
  // get load factor
  if (Basic->AccelerationAvailable) {
    n = Basic->Gload;
  } else {
    n = 1.0;
  }

  // calculate optimum cruise speed in current track direction
  // this still makes use of mode, so it should agree with
  // Vmcready if the track bearing is the best cruise track
  // this does assume g loading of 1.0

  // this is basically a dolphin soaring calculator

  double delta_mc;
  double risk_mc;
  if (Calculated->TaskAltitudeDifference> -120) {
    risk_mc = MACCREADY;
  } else {
    risk_mc =
      GlidePolar::MacCreadyRisk(Calculated->NavAltitude
                                -SAFETYALTITUDEBREAKOFF-Calculated->TerrainAlt,
                                Calculated->MaxThermalHeight,
                                MACCREADY);
  }

  if (EnableBlockSTF) {
    delta_mc = risk_mc;
  } else {
    delta_mc = risk_mc-Calculated->NettoVario;
  }

  if (Calculated->Vario <= risk_mc) {
    // thermal is worse than mc threshold, so find opt cruise speed

    double VOptnew;

    if (!ValidTaskPoint(ActiveWayPoint) || !Calculated->FinalGlide) {
      // calculate speed as if cruising, wind has no effect on opt speed
      GlidePolar::MacCreadyAltitude(delta_mc,
                                    100.0, // dummy value
                                    Basic->TrackBearing,
                                    0,
                                    0,
                                    0,
                                    &VOptnew,
                                    true,
                                    0
                                    );
    } else {
      GlidePolar::MacCreadyAltitude(delta_mc,
                                    100.0, // dummy value
                                    Basic->TrackBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    &VOptnew,
                                    true,
                                    0
                                    );
    }

    // put low pass filter on VOpt so display doesn't jump around
    // too much
    Calculated->VOpt = LowPassFilter(Calculated->VOpt,VOptnew,0.4);

  } else {
    // this thermal is better than maccready, so fly at minimum sink
    // speed
    // calculate speed of min sink adjusted for load factor
    Calculated->VOpt = GlidePolar::Vminsink*sqrt(n);
  }

  Calculated->STFMode = !Calculated->Circling;
}


void NettoVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  double n;
  // get load factor
  if (Basic->AccelerationAvailable) {
    n = Basic->Gload;
  } else {
    n = 1.0;
  }

  // calculate sink rate of glider for calculating netto vario

  bool replay_disabled = !ReplayLogger::IsEnabled();

  if (Basic->NettoVarioAvailable && replay_disabled) {
    Calculated->NettoVario = Basic->NettoVario;
  } else {

    double glider_sink_rate;
    if (Basic->AirspeedAvailable && replay_disabled) {
      glider_sink_rate= GlidePolar::SinkRate(Basic->IndicatedAirspeed, n);
    } else {
      // assume zero wind (Speed=Airspeed, very bad I know)
      glider_sink_rate= GlidePolar::SinkRate(Basic->Speed, n);
    }
    if (Basic->VarioAvailable && replay_disabled) {
      Calculated->NettoVario = Basic->Vario - glider_sink_rate;
    } else {
      Calculated->NettoVario = Calculated->Vario - glider_sink_rate;
    }
  }
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
  SpeedToFly(Basic, Calculated);
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

    mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
    Calculated->TrueAirspeedEstimated = mag;

    if (((AutoWindMode & D_AUTOWIND_ZIGZAG)==D_AUTOWIND_ZIGZAG)
        && (!ReplayLogger::IsEnabled())) {
      double zz_wind_speed;
      double zz_wind_bearing;
      int quality;
      quality = WindZigZagUpdate(Basic, Calculated,
                                 &zz_wind_speed,
				 &zz_wind_bearing);
      if (quality>0) {
        SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
        Vector v_wind;
        v_wind.x = zz_wind_speed*cos(zz_wind_bearing*3.1415926/180.0);
        v_wind.y = zz_wind_speed*sin(zz_wind_bearing*3.1415926/180.0);
        LockFlightData();
        if (windanalyser) {
	  windanalyser->slot_newEstimate(Basic, Calculated, v_wind, quality);
        }
        UnlockFlightData();
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


void  SetWindEstimate(double wind_speed, double wind_bearing, int quality) {
  Vector v_wind;
  v_wind.x = wind_speed*cos(wind_bearing*3.1415926/180.0);
  v_wind.y = wind_speed*sin(wind_bearing*3.1415926/180.0);
  LockFlightData();
  if (windanalyser) {
    windanalyser->slot_newEstimate(&GPS_INFO, &CALCULATED_INFO,
                                   v_wind, quality);
  }
  UnlockFlightData();
}


void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // do slow part of calculations (cleanup of caches etc, nothing
  // that changes the state)

  static double LastOptimiseTime = 0;
  static double lastTime = 0;
  if (Basic->Time<= lastTime) {
    lastTime = Basic->Time-6;
  } else {
    // calculate airspace warnings every 6 seconds
    AirspaceWarning(Basic, Calculated);
  }

  if (FinalGlideTerrain)
     TerrainFootprint(Basic, Calculated);

  // moved from MapWindow.cpp
  if(Basic->Time> LastOptimiseTime+0.0)
    {
      LastOptimiseTime = Basic->Time;
      RasterTerrain::ServiceCache();
    }
}


void DistanceToHome(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  int home_waypoint = HomeWaypoint;

  if (!ValidWayPoint(home_waypoint)) {
    Calculated->HomeDistance = 0.0;
    return;
  }

  double w1lat = WayPointList[home_waypoint].Latitude;
  double w1lon = WayPointList[home_waypoint].Longitude;
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
    Calculated->timeCruising = 0;
    Calculated->timeCircling = 0;

    Calculated->CruiseStartTime = -1;
    Calculated->ClimbStartTime = -1;

    Calculated->LDFinish = 999;
    Calculated->CruiseLD = 999;
    Calculated->LDNext = 999;
    Calculated->LD = 999;
    Calculated->LDvario = 999;

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
    Calculated->MinAltitude = 0;
    Calculated->MaxHeightGain = 0;
  }
}


bool FlightTimes(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static double LastTime = 0;

  if ((Basic->Time != 0) && (Basic->Time <= LastTime))
    // 20060519:sgi added (Basic->Time != 0) dueto alwas return here
    // if no GPS time available
    {

      if ((Basic->Time<LastTime) && (!Basic->NAVWarning)) {
	// Reset statistics.. (probably due to being in IGC replay mode)
        ResetFlightStats(Basic, Calculated);
      }

      LastTime = Basic->Time;
      return false;
    }

  LastTime = Basic->Time;

  double t = DetectStartTime(Basic, Calculated);
  if (t>0) {
    Calculated->FlightTime = t;
  }

  TakeoffLanding(Basic, Calculated);

  return true;
}


void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated, bool doadvance,
               bool doannounce) {
  Calculated->ValidFinish = false;
  Calculated->TaskStartTime = Basic->Time ;
  Calculated->TaskStartSpeed = Basic->Speed;
  Calculated->TaskStartAltitude = Calculated->NavAltitude;
  Calculated->LegStartTime = Basic->Time;

  Calculated->CruiseStartLat = Basic->Latitude;
  Calculated->CruiseStartLong = Basic->Longitude;
  Calculated->CruiseStartAlt = Calculated->NavAltitude;
  Calculated->CruiseStartTime = Basic->Time;

  // JMW TODO: Get time from aatdistance module since this is more accurate

  // JMW clear thermal climb average on task start
  flightstats.ThermalAverage.Reset();

  // JMW reset time cruising/time circling stats on task start
  Calculated->timeCircling = 0;
  Calculated->timeCruising = 0;

  // reset max height gain stuff on task start
  Calculated->MaxHeightGain = 0;
  Calculated->MinAltitude = 0;

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
  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    Calculated->GlideFootPrint[i].x = 0;
    Calculated->GlideFootPrint[i].y = 0;
  }
  Calculated->TerrainWarningLatitude = 0.0;
  Calculated->TerrainWarningLongitude = 0.0;

  LockFlightData();

  if (!windanalyser) {
    windanalyser = new WindAnalyser();

    // seed initial wind store with current conditions
    //JMW TODO SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing, 1);

  }
  UnlockFlightData();

}


void AverageClimbRate(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (Basic->AirspeedAvailable && Basic->VarioAvailable
      && (!Calculated->Circling)) {

    int vi = iround(Basic->IndicatedAirspeed);

    if ((vi<=0) || (vi>= SAFTEYSPEED)) {
      // out of range
      return;
    }
    if (Basic->AccelerationAvailable) {
      if (fabs(Basic->Gload-1.0)>0.25) {
        // G factor too high
        return;
      }
    }
    if (Basic->TrueAirspeed>0) {

      // TODO: Check this is correct for TAS/IAS

      double ias_to_tas = Basic->IndicatedAirspeed/Basic->TrueAirspeed;
      double w_tas = Basic->Vario*ias_to_tas;

      Calculated->AverageClimbRate[vi]+= w_tas;
      Calculated->AverageClimbRateN[vi]++;
    }
  }
}


void DebugTaskCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
#ifdef DEBUGTASKSPEED
    if (Calculated->TaskStartTime>0) {
      char buffer[200];

      double effective_mc = EffectiveMacCready(Basic, Calculated);
      sprintf(buffer,"%g %g %g %g %g %g %g %g %g %d # taskspeed\r\n",
              Basic->Time-Calculated->TaskStartTime,
              Calculated->TaskDistanceCovered,
              Calculated->TaskDistanceToGo,
              Calculated->TaskAltitudeRequired,
              Calculated->NavAltitude,
              Calculated->TaskSpeedAchieved,
              Calculated->TaskSpeed,
              MACCREADY,
              effective_mc,
              ActiveWayPoint);
      DebugStore(buffer);
    }
#endif
}


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{

  Heading(Basic, Calculated);
  DistanceToNext(Basic, Calculated);
  DistanceToHome(Basic, Calculated);
  EnergyHeightNavAltitude(Basic, Calculated);
  TerrainHeight(Basic, Calculated);
  AltitudeRequired(Basic, Calculated, MACCREADY);
  Vario(Basic,Calculated);

  if (TaskAborted) {
    SortLandableWaypoints(Basic, Calculated);
  }
  TaskStatistics(Basic, Calculated, MACCREADY);
  AATStats(Basic, Calculated);
  TaskSpeed(Basic, Calculated, MACCREADY);

  if (!FlightTimes(Basic, Calculated)) {
    // time hasn't advanced, so don't do calculations requiring an advance
    // or movement
    return FALSE;
  }

  Turning(Basic, Calculated);
  LD(Basic,Calculated);
  CruiseLD(Basic,Calculated);
  Average30s(Basic,Calculated);
  AverageThermal(Basic,Calculated);
  AverageClimbRate(Basic,Calculated);
  ThermalGain(Basic,Calculated);
  LastThermalStats(Basic, Calculated);
  ThermalBand(Basic, Calculated);
  MaxHeightGain(Basic,Calculated);

  PredictNextPosition(Basic, Calculated);
  CalculateOwnTeamCode(Basic, Calculated);
  CalculateTeammateBearingRange(Basic, Calculated);

  if (!TaskAborted) {
    InSector(Basic, Calculated);
    DoAutoMacCready(Basic, Calculated);
    IterateEffectiveMacCready(Basic, Calculated);
    DebugTaskCalculations(Basic, Calculated);
  }

  DoLogging(Basic, Calculated);
  vegavoice.Update(Basic, Calculated);
  ConditionMonitorsUpdate(Basic, Calculated);

  return TRUE;
}


void EnergyHeightNavAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  // Determine which altitude to use for nav functions
  if (EnableNavBaroAltitude && Basic->BaroAltitudeAvailable) {
    Calculated->NavAltitude = Basic->BaroAltitude;
  } else {
    Calculated->NavAltitude = Basic->Altitude;
  }

  double ias_to_tas;
  double V_tas;

  if (Basic->AirspeedAvailable && (Basic->IndicatedAirspeed>0)) {
    ias_to_tas = Basic->TrueAirspeed/Basic->IndicatedAirspeed;
    V_tas = Basic->TrueAirspeed;
  } else {
    ias_to_tas = 1.0;
    V_tas = Calculated->TrueAirspeedEstimated;
  }
  double V_bestld_tas = GlidePolar::Vbestld*ias_to_tas;
  Calculated->EnergyHeight =
    max(0, V_tas*V_tas-V_bestld_tas*V_bestld_tas)/(9.81*2.0);
}


void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double LastAlt = 0;

  if (!Basic->VarioAvailable || ReplayLogger::IsEnabled()) {
    if(Basic->Time <= LastTime) {
      LastTime = Basic->Time;
    } else {
      double Gain = Calculated->NavAltitude - LastAlt;
      // estimate value from GPS
      Calculated->Vario = Gain / (Basic->Time - LastTime);
      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;
    }
  } else {
    // get value from instrument
    Calculated->Vario = Basic->Vario;
    // we don't bother with sound here as it is polled at a
    // faster rate in the DoVarioCalcs methods
  }
}


void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double Altitude[30];
  static double Vario[30];
  static double NettoVario[30];
  int Elapsed, i;
  long index;
  double Gain;

  if(Basic->Time > LastTime)
    {
      Elapsed = (int)(Basic->Time - LastTime);
      for(i=0;i<Elapsed;i++)
        {
          index = (long)LastTime + i;
          index %=30;

          Altitude[index] = Calculated->NavAltitude;
	  if (Basic->NettoVarioAvailable) {
	    NettoVario[index] = Basic->NettoVario;
	  } else {
	    NettoVario[index] = Calculated->NettoVario;
	  }
	  if (Basic->VarioAvailable) {
	    Vario[index] = Basic->Vario;
	  } else {
	    Vario[index] = Calculated->Vario;
	  }
        }

      double Vave = 0;
      double NVave = 0;
      for (i=0; i<30; i++) {
        Vave += Vario[i];
	NVave += NettoVario[i];
      }

      if (!Basic->VarioAvailable) {
        index = ((long)Basic->Time - 1)%30;
        Gain = Altitude[index];

        index = ((long)Basic->Time)%30;
        Gain = Gain - Altitude[index];

        Vave = Gain;
      }
      Calculated->Average30s =
        LowPassFilter(Calculated->Average30s,Vave/30,0.5);
      Calculated->NettoAverage30s =
        LowPassFilter(Calculated->NettoAverage30s,NVave/30,0.5);
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
      }
    }
  LastTime = Basic->Time;
}

void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (Calculated->ClimbStartTime>=0) {
    if(Basic->Time > Calculated->ClimbStartTime)
      {
        double Gain =
          Calculated->NavAltitude - Calculated->ClimbStartAlt;
        Calculated->AverageThermal  =
          Gain / (Basic->Time - Calculated->ClimbStartTime);
      }
  }
}

void MaxHeightGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (!Calculated->Flying) return;

  if (Calculated->MinAltitude>0) {
    double height_gain = Calculated->NavAltitude - Calculated->MinAltitude;
    Calculated->MaxHeightGain = max(height_gain, Calculated->MaxHeightGain);
  } else {
    Calculated->MinAltitude = Calculated->NavAltitude;
  }
  Calculated->MinAltitude = min(Calculated->NavAltitude, Calculated->MinAltitude);
}


void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if (Calculated->ClimbStartTime>=0) {
    if(Basic->Time >= Calculated->ClimbStartTime)
      {
        Calculated->ThermalGain =
          Calculated->NavAltitude - Calculated->ClimbStartAlt;
      }
  }
}


double LimitLD(double LD) {
  if (fabs(LD)>999) {
    return 999;
  } else {
    if ((LD>=0.0)&&(LD<1.0)) {
      LD= 1.0;
    }
    if ((LD<0.0)&&(LD>-1.0)) {
      LD= -1.0;
    }
    return LD;
  }
}


double UpdateLD(double LD, double d, double h, double filter_factor) {
  double glideangle;
  if (LD != 0) {
    glideangle = 1.0/LD;
  } else {
    glideangle = 1.0;
  }
  if (d!=0) {
    glideangle = LowPassFilter(1.0/LD, h/d, filter_factor);
    if (fabs(glideangle) > 0.001) {
      LD = LimitLD(1.0/glideangle);
    } else {
      LD = 999;
    }
  }
  return LD;
}


void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;

  if (Basic->Time<LastTime) {
    LastTime = Basic->Time;
  }
  if(Basic->Time >= LastTime+1.0)
    {
      double DistanceFlown;
      DistanceBearing(Basic->Latitude, Basic->Longitude,
                      LastLat, LastLon,
                      &DistanceFlown, NULL);

      Calculated->LD = UpdateLD(Calculated->LD,
                                DistanceFlown,
                                LastAlt - Calculated->NavAltitude, 0.1);

      LastLat = Basic->Latitude;
      LastLon = Basic->Longitude;
      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;
    }

  // LD instantaneous from vario, updated every reading..
  Calculated->LDvario = 999;
  if (Basic->VarioAvailable && Basic->AirspeedAvailable) {
    Calculated->LDvario = UpdateLD(Calculated->LDvario,
                                   Basic->IndicatedAirspeed,
                                   Basic->Vario,
                                   0.3);
  }
}


void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{

  if(!Calculated->Circling)
    {
      double DistanceFlown;

      if (Calculated->CruiseStartTime<0) {
        Calculated->CruiseStartLat = Basic->Latitude;
        Calculated->CruiseStartLong = Basic->Longitude;
        Calculated->CruiseStartAlt = Calculated->NavAltitude;
        Calculated->CruiseStartTime = Basic->Time;
      } else {

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        Calculated->CruiseStartLat,
                        Calculated->CruiseStartLong, &DistanceFlown, NULL);
        Calculated->CruiseLD =
          UpdateLD(Calculated->CruiseLD,
                   DistanceFlown,
                   Calculated->CruiseStartAlt - Calculated->NavAltitude,
                   0.5);
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

  // this is calculation stuff, leave it there
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    LockFlightData();
    windanalyser->slot_newFlightMode(Basic, Calculated, left, 0);
    UnlockFlightData();
  }

}


void PercentCircling(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  if (Calculated->Circling) {
    //    timeCircling += (Basic->Time-LastTime);
    Calculated->timeCircling+= 1.0;
  } else {
    //    timeCruising += (Basic->Time-LastTime);
    Calculated->timeCruising+= 1.0;
  }

  if (Calculated->timeCruising+Calculated->timeCircling>1) {
    Calculated->PercentCircling =
      100.0*(Calculated->timeCircling)/(Calculated->timeCruising+
                                        Calculated->timeCircling);
  } else {
    Calculated->PercentCircling = 0.0;
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
  LastTime = Basic->Time;
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

  Rate = LowPassFilter(LastRate,Rate,0.3);
  LastRate = Rate;

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

  PercentCircling(Basic, Calculated);

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
            least_squares_update(max(0,Calculated->ClimbStartTime/3600.0
                                     - flightstats.Altitude.xstore[0]),
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
      LockFlightData();
      windanalyser->slot_newSample(Basic, Calculated);
      UnlockFlightData();
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
          least_squares_update(max(0,Calculated->CruiseStartTime/3600.0
                                   - flightstats.Altitude.xstore[0]),
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
    LockFlightData();
    windanalyser->slot_Altitude(Basic, Calculated);
    UnlockFlightData();
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
    Calculated->ThermalSources[ibest].LiftRate =
      Calculated->LastThermalAverage;
    Calculated->ThermalSources[ibest].Latitude = ground_latitude;
    Calculated->ThermalSources[ibest].Longitude = ground_longitude;
    Calculated->ThermalSources[ibest].GroundHeight = ground_altitude;
    Calculated->ThermalSources[ibest].Time = Basic->Time;
  }
}


static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static int LastCircling = FALSE;

  if((Calculated->Circling == FALSE) && (LastCircling == TRUE)
     && (Calculated->ClimbStartTime>=0))
    {
      double ThermalTime = Calculated->CruiseStartTime
        - Calculated->ClimbStartTime;

      if(ThermalTime >0)
        {
          double ThermalGain = Calculated->CruiseStartAlt
            - Calculated->ClimbStartAlt;
          Calculated->LastThermalAverage = ThermalGain/ThermalTime;
          Calculated->LastThermalGain = ThermalGain;
          Calculated->LastThermalTime = ThermalTime;

          if (Calculated->LastThermalAverage>0) {
            if (ThermalTime>60.0) {
              flightstats.ThermalAverage.
                least_squares_update(Calculated->LastThermalAverage);

#ifdef DEBUG_STATS
              char Temp[100];
              sprintf(Temp,"%f %f # thermal stats\n",
                      flightstats.ThermalAverage.m,
                      flightstats.ThermalAverage.b
                      );
              DebugStore(Temp);
#endif
              if (EnableThermalLocator) {
                ThermalSources(Basic, Calculated);
              }
            }
	  }
	}
    }
  LastCircling = Calculated->Circling;
}


void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  //  LockFlightData();
  LockTaskData();

  if(ValidTaskPoint(ActiveWayPoint))
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


void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      double maccready)
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

      Calculated->NextAltitudeRequired +=
        FAIFinishHeight(Basic, Calculated, ActiveWayPoint);

      Calculated->NextAltitudeDifference =
        Calculated->NavAltitude
        - Calculated->NextAltitudeRequired
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
    if(distance < Task[thepoint].AATCircleRadius) {
      retval = true;
    }
  } else if(distance < Task[thepoint].AATSectorRadius) {
    if (AngleInRange(Task[thepoint].AATStartRadial,
                     Task[thepoint].AATFinishRadial,
                     AircraftBearing)) {
      retval = true;
    }
  }

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
  AircraftBearing = AngleLimit180(AircraftBearing - Task[i].InBound);

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
  AircraftBearing = AngleLimit180(AircraftBearing - OutBound);

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

  if (!Calculated->Flying ||
      !ValidTaskPoint(ActiveWayPoint) ||
      !ValidTaskPoint(0))
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
    } else {
      // JMW fixed 20070528
      if (ActiveWayPoint>0) {
        if (reset) AdvanceArmed = false;
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
  BOOL StartCrossed= false;

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

      // This is set whether ready to advance or not, because it will
      // appear in the flight log, so if it's valid, it's valid.
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
    } else {

      if ((ActiveWayPoint<=1)
          && !IsFinalWaypoint()
          && (Calculated->ValidStart==false)
          && (Calculated->Flying)) {

        // need to detect bad starts, just to get the statistics
        // in case the bad start is the best available, or the user
        // manually started
        StartTask(Basic, Calculated, false, false);
        Calculated->ValidStart = false;
      }

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

      // JMWX save calculated data at finish
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



static void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double Alt = 0;

  RasterTerrain::Lock();
  // want most accurate rounding here
  RasterTerrain::SetTerrainRounding(0,0);
  Alt = RasterTerrain::GetTerrainHeight(Basic->Latitude,
                                        Basic->Longitude);
  RasterTerrain::Unlock();

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

  TotalAltitude += FAIFinishHeight(Basic, Calculated, getFinalWaypoint());

  if (!ValidTaskPoint(*ifinal)) {
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = false;
  } else {
    Calculated->TaskAltitudeRequiredFromStart = TotalAltitude;
    retval = true;
  }
 OnExit:
  UnlockTaskData();
  return retval;
}


double MacCreadyOrAvClimbRate(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                              double maccready)
{
  double mc_val = maccready;
  bool is_final_glide = false;

  if (Calculated->FinalGlide) {
    is_final_glide = true;
  }

  // when calculating 'achieved' task speed, need to use Mc if
  // not in final glide, or if in final glide mode and using
  // auto Mc, use the average climb rate achieved so far.

  if ((mc_val<0.1) ||
      (Calculated->AutoMacCready &&
       ((AutoMcMode==0) ||
        ((AutoMcMode==2)&&(is_final_glide))
        ))
      ) {

    if (flightstats.ThermalAverage.y_ave>0) {
      mc_val = flightstats.ThermalAverage.y_ave;
    }
  }
  return max(0.1, mc_val);

}


void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, double maccready)
{
  int ifinal;
  static double LastTime = 0;
  static double v2last = 0;
  static double t1last = 0;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;
  if (TaskAborted) return;
  if (Calculated->ValidFinish) return;
  if (!Calculated->Flying) return;

  // in case we leave early due to error
  Calculated->TaskSpeedAchieved = 0;
  Calculated->TaskSpeed = 0;
  Calculated->TaskSpeedInstantaneous = 0;

  if (ActiveWayPoint<=0) { // no task speed before start
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

    double hf = FAIFinishHeight(Basic, Calculated, -1);

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
    double t1mod = t1-hx/MacCreadyOrAvClimbRate(Basic, Calculated, maccready);
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

    if (Calculated->ValidFinish) {
      dFinal = 0;
    }

    // equivalent distance to end of final glide
    d2 = d1+dFinal;

    // time at end of final glide
    t2 = t1+dFinal/Vfinal;

    // actual task speed achieved so far
    v1 = d1/t1;

    // average speed to end of final glide from here
    v2 = d2/t2;

    Calculated->TaskSpeed = max(v1,v2);

    if(Basic->Time < LastTime) {
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
        LowPassFilter(Calculated->TaskSpeedInstantaneous, vdiff, 0.02);

      v2last = v2;
      t1last = t1;
      LastTime = Basic->Time;
    }
  }
 OnExit:
  UnlockTaskData();

}


static void CheckFinalGlideThroughTerrain(NMEA_INFO *Basic,
                                          DERIVED_INFO *Calculated,
                                          double LegToGo,
                                          double LegBearing) {

  static bool glide_through_terrain_last = false;
  bool glide_through_terrain_now = false;
  // Final glide through terrain updates
  if (Calculated->FinalGlide) {

    double lat, lon;
    bool out_of_range;
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               Basic, Calculated,
                               &lat,
                               &lon,
                               LegToGo, &out_of_range);

    if ((!out_of_range)&&(distance_soarable< LegToGo)) {
      // JMW TODO issue terrain warning (audio?)
      Calculated->TerrainWarningLatitude = lat;
      Calculated->TerrainWarningLongitude = lon;

      glide_through_terrain_now = true;
    } else {
      Calculated->TerrainWarningLatitude = 0.0;
      Calculated->TerrainWarningLongitude = 0.0;
      glide_through_terrain_last = false;
    }

  } else {
    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;
    glide_through_terrain_last = false;
  }

  if (Calculated->TaskAltitudeDifference>0) {
    if (!glide_through_terrain_last && glide_through_terrain_now) {
      InputEvents::processGlideComputer(GCE_FLIGHTMODE_FINALGLIDE_TERRAIN);
      glide_through_terrain_last = true;
    }
  } else {
    glide_through_terrain_last = false;
  }
}


void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                    double maccready)
{

  if (!ValidTaskPoint(ActiveWayPoint) ||
      ((ActiveWayPoint>0) && !ValidTaskPoint(ActiveWayPoint-1))) {

    Calculated->LegSpeed = 0;
    Calculated->LegDistanceToGo = 0;
    Calculated->LegDistanceCovered = 0;
    Calculated->LegTimeToGo = 0;

    //    Calculated->TaskSpeed = 0;

    Calculated->TaskDistanceToGo = 0;
    Calculated->TaskDistanceCovered = 0;
    Calculated->TaskTimeToGo = 0;

    Calculated->TaskAltitudeRequired = 0;
    Calculated->TaskAltitudeDifference = 0;
    Calculated->TaskAltitudeDifference0 = 0;

    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;

    Calculated->LDFinish = 999;
    Calculated->LDNext = 999;

    Calculated->FinalGlide = 0;
    CheckFinalGlideThroughTerrain(Basic, Calculated,
                                  0.0, 0.0);

    // no task selected, so work things out at current heading

    GlidePolar::MacCreadyAltitude(maccready, 100.0,
                                  Basic->TrackBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  &(Calculated->BestCruiseTrack),
                                  &(Calculated->VMacCready),
                                  (Calculated->FinalGlide==1),
                                  0);

    return;
  }

  //  LockFlightData();
  LockTaskData();

  ///////////////////////////////////////////////
  // Calculate Task Distances
  // First calculate distances for this waypoint

  double LegCovered, LegToGo=0;
  double LegDistance, LegBearing=0;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  if (AATEnabled && (ActiveWayPoint>0) && (!TaskAborted)) {
    w1lat = Task[ActiveWayPoint].AATTargetLat;
    w1lon = Task[ActiveWayPoint].AATTargetLon;
  } else {
    w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
  }

  DistanceBearing(Basic->Latitude,
                  Basic->Longitude,
                  w1lat,
                  w1lon,
                  &LegToGo, &LegBearing);

  if ((ActiveWayPoint<1) || TaskAborted) {
    LegCovered = 0;
  } else {
    if (AATEnabled) {
      // TODO: Get best range point to here...
      w0lat = Task[ActiveWayPoint-1].AATTargetLat;
      w0lon = Task[ActiveWayPoint-1].AATTargetLon;
    } else {
      w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
      w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;
    }

    DistanceBearing(w1lat,
                    w1lon,
                    w0lat,
                    w0lon,
                    &LegDistance, NULL);

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
  }

  Calculated->LegDistanceToGo = LegToGo;
  Calculated->LegDistanceCovered = LegCovered;
  Calculated->TaskDistanceCovered = LegCovered;

  if (Basic->Time > Calculated->LegStartTime) {
    Calculated->LegSpeed = Calculated->LegDistanceCovered
      / (Basic->Time - Calculated->LegStartTime);
  }

  ///////////////////////////////////////////////////
  // Now add distances for start to previous waypoint

  if (!TaskAborted) {

    if (!AATEnabled) {
      for(int i=0;i< ActiveWayPoint-1; i++)
        {
          if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

          w1lat = WayPointList[Task[i].Index].Latitude;
          w1lon = WayPointList[Task[i].Index].Longitude;
          w0lat = WayPointList[Task[i+1].Index].Latitude;
          w0lon = WayPointList[Task[i+1].Index].Longitude;

          DistanceBearing(w1lat,
                          w1lon,
                          w0lat,
                          w0lon,
                          &LegDistance, NULL);
          Calculated->TaskDistanceCovered += LegDistance;
        }
    } else if (ActiveWayPoint>0) {
      // JMW added correction for distance covered
      Calculated->TaskDistanceCovered =
        aatdistance.DistanceCovered(Basic->Longitude,
                                    Basic->Latitude,
                                    ActiveWayPoint);
    }
  }

  //////////////////////

  // Calculate Final Glide To Finish
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;

  // double FinalAltitude = 0;
  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = Calculated->NavAltitude-
    FAIFinishHeight(Basic, Calculated, -1);

  CheckTransitionFinalGlide(Basic, Calculated);

  // JMW TODO: use mc based on risk?
  double LegAltitude =
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

  double LegTime0;
  double LegAltitude0 =
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
  // JMW XXX TODO: Use safetymc

  if (LegTime0>=1e5) {
    // can't make it, so assume flying at current mc
    LegAltitude0 = LegAltitude;
  }

  double TaskAltitudeRequired = LegAltitude;
  double TaskAltitudeRequired0 = LegAltitude0;
  Calculated->TaskDistanceToGo = LegToGo;
  Calculated->TaskTimeToGo = Calculated->LegTimeToGo;

  double height_above_leg = Calculated->NavAltitude
    - LegAltitude;

  if (Calculated->FinalGlide) {
    height_above_leg -= FAIFinishHeight(Basic, Calculated, -1);
  } else {
    height_above_leg -= FAIFinishHeight(Basic, Calculated, ActiveWayPoint);
  }

  Calculated->LDNext = UpdateLD(Calculated->LDNext,
                                Calculated->TaskDistanceToGo,
                                height_above_leg,
                                0.5);

  int task_index= ActiveWayPoint+1;

  while((!TaskAborted) && ValidTaskPoint(task_index))
    {

      double this_LegTimeToGo;
      bool this_is_final = (task_index==FinalWayPoint)
        || ForceFinalGlide;

      if (AATEnabled) {
        w1lat = Task[task_index].AATTargetLat;
        w1lon = Task[task_index].AATTargetLon;
        w0lat = Task[task_index-1].AATTargetLat;
        w0lon = Task[task_index-1].AATTargetLon;
      } else {
        w1lat = WayPointList[Task[task_index].Index].Latitude;
        w1lon = WayPointList[Task[task_index].Index].Longitude;
        w0lat = WayPointList[Task[task_index-1].Index].Latitude;
        w0lon = WayPointList[Task[task_index-1].Index].Longitude;
      }

      double NextLegDistance, NextLegBearing;

      DistanceBearing(w0lat,
                      w0lon,
                      w1lat,
                      w1lon,
                      &NextLegDistance, &NextLegBearing);

      LegAltitude = GlidePolar::
        MacCreadyAltitude(maccready,
                          NextLegDistance, NextLegBearing,
                          Calculated->WindSpeed,
                          Calculated->WindBearing,
                          0, 0,
                          this_is_final,
                          &this_LegTimeToGo,
                          height_above_finish);

      LegAltitude0 = GlidePolar::
        MacCreadyAltitude(0,
                          NextLegDistance, NextLegBearing,
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

      Calculated->TaskDistanceToGo += NextLegDistance;
      Calculated->TaskTimeToGo += this_LegTimeToGo;

      task_index++;
    }

  double final_height = FAIFinishHeight(Basic, Calculated, -1);

  double total_energy_height = Calculated->NavAltitude
    + Calculated->EnergyHeight;

  Calculated->TaskAltitudeRequired = TaskAltitudeRequired + final_height;

  TaskAltitudeRequired0 += final_height;

  Calculated->TaskAltitudeDifference = total_energy_height
    - Calculated->TaskAltitudeRequired;

  Calculated->TaskAltitudeDifference0 = total_energy_height
    - TaskAltitudeRequired0;

  Calculated->LDFinish = UpdateLD(Calculated->LDFinish,
                                  Calculated->TaskDistanceToGo,
                                  total_energy_height-final_height,
                                  0.5);

  CheckFinalGlideThroughTerrain(Basic, Calculated,
                                LegToGo, LegBearing);

  CheckForceFinalGlide(Basic, Calculated);

  UnlockTaskData();
  //  UnlockFlightData();

}


void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  bool is_final_glide = false;

  if (!Calculated->AutoMacCready) return;

  //  LockFlightData();
  LockTaskData();

  double mc_new = MACCREADY;

  if (Calculated->FinalGlide && ActiveIsFinalWaypoint()) {
    is_final_glide = true;
  }

  if ((AutoMcMode==0)||((AutoMcMode==2)&& is_final_glide)) {

    if (ValidTaskPoint(ActiveWayPoint)) {

      double time_remaining = Basic->Time-Calculated->TaskStartTime-9000;
      if (EnableOLC
	  && (OLCRules==0)
	  && (Calculated->NavAltitude>Calculated->TaskStartAltitude)
	  && (time_remaining>0)) {

	mc_new = MacCreadyTimeLimit(Basic, Calculated,
				   Calculated->WaypointBearing,
				   time_remaining,
				   Calculated->TaskStartAltitude);

      } else {
        if (Calculated->TaskAltitudeDifference0>0) {

          // only change if above final glide with zero Mc
          // otherwise when we are well below, it will wind Mc back to
          // zero

          double slope =
            (Calculated->NavAltitude
             - FAIFinishHeight(Basic, Calculated, ActiveWayPoint))/
            (Calculated->WaypointDistance+1);

          double mc_pirker = PirkerAnalysis(Basic, Calculated,
                                            Calculated->WaypointBearing,
                                            slope);
          if (mc_pirker>0) {
            mc_new = mc_pirker;
          } else {
            mc_new = 0.0;
          }
        }
      }
    }
  } else if ((AutoMcMode==1)||((AutoMcMode==2)&&(!is_final_glide))) {

    if (flightstats.ThermalAverage.y_ave>0) {
      mc_new = flightstats.ThermalAverage.y_ave;
    }

  }

  MACCREADY = LowPassFilter(MACCREADY,mc_new,0.15);

  UnlockTaskData();
  //  UnlockFlightData();

}


extern int AIRSPACEWARNINGS;
extern int WarningTime;
extern int AcknowledgementTime;


void PredictNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  if(Calculated->Circling)
    {
      Calculated->NextLatitude = Basic->Latitude;
      Calculated->NextLongitude = Basic->Longitude;
      Calculated->NextAltitude =
        Calculated->NavAltitude + Calculated->Average30s * WarningTime;
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

// JMW this code is deprecated
bool ClearAirspaceWarnings(bool acknowledge, bool ack_all_day) {
  unsigned int i;
  if (acknowledge) {
    GlobalClearAirspaceWarnings = true;
    if (AirspaceCircle) {
      for (i=0; i<NumberOfAirspaceCircles; i++) {
        if (AirspaceCircle[i].WarningLevel>0) {
          AirspaceCircle[i].Ack.AcknowledgementTime = GPS_INFO.Time;
          if (ack_all_day) {
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
          if (ack_all_day) {
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


void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated){
  unsigned int i;

  if(!AIRSPACEWARNINGS)
      return;

  static bool position_is_predicted = false;

  //  LockFlightData(); Not necessary, airspace stuff has its own locking

  if (GlobalClearAirspaceWarnings == true) {
    GlobalClearAirspaceWarnings = false;
    Calculated->IsInAirspace = false;
  }

  position_is_predicted = !position_is_predicted;
  // every second time step, do predicted position rather than
  // current position

  double alt;
  double lat;
  double lon;

  if (position_is_predicted) {
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

      if ((alt >= AirspaceCircle[i].Base.Altitude )
          && (alt < AirspaceCircle[i].Top.Altitude)) {


        if (InsideAirspaceCircle(lon, lat, i)
            && (MapWindow::iAirspaceMode[AirspaceCircle[i].Type] >= 2)){

          AirspaceWarnListAdd(Basic, position_is_predicted, 1, i, false);
        }

      }

    }
  }

  // repeat process for areas

  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {

      if ((alt >= AirspaceArea[i].Base.Altitude )
          && (alt < AirspaceArea[i].Top.Altitude)) {

        if ((MapWindow::iAirspaceMode[AirspaceArea[i].Type] >= 2)
            && InsideAirspaceArea(lon, lat, i)){

          AirspaceWarnListAdd(Basic, position_is_predicted, 0, i, false);
        }

      }
    }
  }

  AirspaceWarnListProcess(Basic);

  //  UnlockFlightData();

}

//////////////////////////////////////////////

void AATStats_Time(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // Task time to go calculations

  double aat_tasktime_elapsed = Basic->Time - Calculated->TaskStartTime;
  double aat_tasklength_seconds = AATTaskLength*60;

  if ((ActiveWayPoint==0)&&(Calculated->AATTimeToGo==0)) {
    Calculated->AATTimeToGo = aat_tasklength_seconds;
  }

  if((aat_tasktime_elapsed>=0) && (ActiveWayPoint >0)) {
    Calculated->AATTimeToGo =
      min(aat_tasklength_seconds,
          max(0,aat_tasklength_seconds - aat_tasktime_elapsed));
  }

  if(ValidTaskPoint(ActiveWayPoint) && (Calculated->AATTimeToGo>0)) {
    Calculated->AATMaxSpeed =
      Calculated->AATMaxDistance / Calculated->AATTimeToGo;
    Calculated->AATMinSpeed =
      Calculated->AATMinDistance / Calculated->AATTimeToGo;
    Calculated->AATTargetSpeed =
      Calculated->AATTargetDistance / Calculated->AATTimeToGo;
  }
}


void AATStats_Distance(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  int i;
  double MaxDistance, MinDistance, TargetDistance;

  //  LockFlightData();
  LockTaskData();

  MaxDistance = 0; MinDistance = 0; TargetDistance = 0;
  // Calculate Task Distances

  if(ValidTaskPoint(ActiveWayPoint))
    {
      i=ActiveWayPoint;

      double LegToGo, TargetLegToGo;

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
          double LegDistance, TargetLegDistance;

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
              MaxDistance += LegDistance +
                Task[ActiveWayPoint].AATSectorRadius * 2;
              MinDistance += LegDistance;
            }
          TargetDistance += TargetLegDistance;
          i++;
        }

      // JMW TODO: make these calculations more accurate, because
      // currently they are very approximate.

      Calculated->AATMaxDistance = MaxDistance;
      Calculated->AATMinDistance = MinDistance;
      Calculated->AATTargetDistance = TargetDistance;
    }
  UnlockTaskData();
  //  UnlockFlightData();
}


void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{

  if (!WayPointList
      || !AATEnabled
      || Calculated->ValidFinish) return ;

  AATStats_Distance(Basic, Calculated);
  AATStats_Time(Basic, Calculated);

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

    // moved beyond ceiling, so redistribute buckets
    double max_thermal_height_new;
    double tmpW[NUMTHERMALBUCKETS];
    int tmpN[NUMTHERMALBUCKETS];
    double h;

    // calculate new buckets so glider is below max
    double hbuk = Calculated->MaxThermalHeight/NUMTHERMALBUCKETS;

    max_thermal_height_new = max(1, Calculated->MaxThermalHeight);
    while (max_thermal_height_new<dheight) {
      max_thermal_height_new += hbuk;
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
      j = iround(NUMTHERMALBUCKETS*h/max_thermal_height_new);

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
    Calculated->MaxThermalHeight= max_thermal_height_new;
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

  // Do preliminary fast search, by converting to screen coordinates
  int sc_x, sc_y;
  LatLon2Flat(WayPointList[i].Longitude,
              WayPointList[i].Latitude, &sc_x, &sc_y);
  int dx, dy;
  dx = scx_aircraft-sc_x;
  dy = scy_aircraft-sc_y;

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
  double arrival_altitude;
  int active_waypoint_on_entry;

  if (!WayPointList) return;

  //  LockFlightData();
  LockTaskData();
  active_waypoint_on_entry = ActiveWayPoint;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  // Clear search lists
  for (i=0; i<MAXTASKPOINTS*2; i++) {
    SortedApproxIndex[i]= -1;
    SortedApproxDistance[i] = 0;
  }

  for (i=0; i<(int)NumberOfWayPoints; i++) {
    if (!(((WayPointList[i].Flags & AIRPORT) == AIRPORT) ||
          ((WayPointList[i].Flags & LANDPOINT) == LANDPOINT))) {
      continue; // ignore non-landable fields
    }

    int approx_distance =
      CalculateWaypointApproxDistance(scx_aircraft, scy_aircraft, i);

    // see if this fits into slot
    for (k=0; k< MAXTASKPOINTS*2; k++)  {

      if (((approx_distance < SortedApproxDistance[k])
           // wp is closer than this one
          || (SortedApproxIndex[k]== -1))   // or this one isn't filled
          && (SortedApproxIndex[k]!= i))    // and not replacing with same
        {
            // ok, got new biggest, put it into the slot.
          for (l=MAXTASKPOINTS*2-1; l>k; l--) {
            if (l>0) {
                SortedApproxDistance[l] = SortedApproxDistance[l-1];
                SortedApproxIndex[l] = SortedApproxIndex[l-1];
            }
          }

          SortedApproxDistance[k] = approx_distance;
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

  bool found_reachable_airport = false;

  for (int scan_airports_slot=0;
       scan_airports_slot<2;
       scan_airports_slot++) {

    if (found_reachable_airport) {
      continue; // don't bother filling the rest of the list
    }

    for (i=0; i<MAXTASKPOINTS*2; i++) {
      if (SortedApproxIndex[i]<0) { // ignore invalid points
        continue;
      }

      if (((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT) &&
          (scan_airports_slot==0)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic,
                                         Calculated,
                                         SortedApproxIndex[i]);

      if (scan_airports_slot==0) {
        if (arrival_altitude<0) {
          // in first scan, this airport is unreachable, so ignore it.
          continue;
        } else {
          // this airport is reachable
          found_reachable_airport = true;
        }
      }

      // see if this fits into slot
      for (k=0; k< MAXTASKPOINTS; k++) {
        if (((arrival_altitude > SortedArrivalAltitude[k])
             // closer than this one
             ||(SortedLandableIndex[k]== -1))
            // or this one isn't filled
             &&(SortedLandableIndex[k]!= i))  // and not replacing
                                              // with same
          {

            double wp_distance, wp_bearing;
            DistanceBearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude,
                            &wp_distance, &wp_bearing);

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, Basic, Calculated,
                                       NULL,
                                       NULL,
                                       wp_distance,
                                       &out_of_range);

            if ((distance_soarable>= wp_distance)||(arrival_altitude<0)) {
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

              SortedArrivalAltitude[k] = arrival_altitude;
              SortedLandableIndex[k] = SortedApproxIndex[i];
              k=MAXTASKPOINTS;
            }
          }
      }
    }
  }

  // now we have a sorted list.
  // check if current waypoint or home waypoint is in the sorted list
  int found_active_waypoint = -1;
  int found_home_waypoint = -1;
  for (i=0; i<MAXTASKPOINTS; i++) {
    if (ValidTaskPoint(ActiveWayPoint)) {
      if (SortedLandableIndex[i] == Task[ActiveWayPoint].Index) {
        found_active_waypoint = i;
      }
    }
    if ((SortedLandableIndex[i] == HomeWaypoint)&&(HomeWaypoint>=0)) {
      found_home_waypoint = i;
    }
  }

  if ((found_home_waypoint == -1)&&(HomeWaypoint>=0)) {
    // home not found in top list, so see if we can sneak it in

    arrival_altitude = CalculateWaypointArrivalAltitude(Basic,
                                                        Calculated,
                                                        HomeWaypoint);
    if (arrival_altitude>0) {
      // only put it in if reachable
      SortedLandableIndex[MAXTASKPOINTS-2] = HomeWaypoint;
    }
  }

  bool new_closest_waypoint = false;

  if (found_active_waypoint != -1) {
    ActiveWayPoint = found_active_waypoint;
  } else {
    // if not found, keep on field or set active waypoint to closest
    if (ValidTaskPoint(ActiveWayPoint)){
      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic, Calculated,
                                         Task[ActiveWayPoint].Index);
    } else {
      arrival_altitude = 0;
    }
    if (arrival_altitude <= 0){   // last active is no more reachable,
                                  // switch to new closest
      new_closest_waypoint = true;
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

  int last_closest_waypoint=0;
  if (new_closest_waypoint) {
    last_closest_waypoint = Task[0].Index;
  }

  for (i=0; i<MAXTASKPOINTS; i++){
    Task[i].Index = SortedLandableIndex[i];
    if (ValidTaskPoint(i)) {
      WayPointList[Task[i].Index].InTask = true;
    }
  }

  if (new_closest_waypoint) {
    if ((Task[0].Index != last_closest_waypoint) && ValidTaskPoint(0)) {
      double last_wp_distance= 10000.0;
      if (last_closest_waypoint>=0) {
        DistanceBearing(WayPointList[Task[0].Index].Latitude,
                        WayPointList[Task[0].Index].Longitude,
                        WayPointList[last_closest_waypoint].Latitude,
                        WayPointList[last_closest_waypoint].Longitude,
                        &last_wp_distance, NULL);
      }
      if (last_wp_distance>2000.0) {
        // don't display the message unless the airfield has moved by more
        // than 2 km
        DoStatusMessage(TEXT("Closest Airfield Changed!"));
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

  if (active_waypoint_on_entry != ActiveWayPoint){
    SelectedWaypoint = ActiveWayPoint;
  }
  UnlockTaskData();
  //  UnlockFlightData();
}


void ResumeAbortTask(int set) {
  static int Task_saved[MAXTASKPOINTS];
  static int active_waypoint_saved= -1;
  static bool aat_enabled_saved= false;
  int i;
  int active_waypoint_on_entry;
  bool task_aborted_on_entry = TaskAborted;

  //  LockFlightData();
  LockTaskData();
  active_waypoint_on_entry = ActiveWayPoint;

  if (set == 0)
    TaskAborted = !TaskAborted;
  else if (set > 0)
    TaskAborted = true;
  else if (set < 0)
    TaskAborted = false;

  if (task_aborted_on_entry != TaskAborted) {
    if (TaskAborted) {

      // save current task in backup

      for (i=0; i<MAXTASKPOINTS; i++) {
        Task_saved[i]= Task[i].Index;
      }
      active_waypoint_saved = ActiveWayPoint;
      if (AATEnabled) {
        aat_enabled_saved = true;
      } else {
        aat_enabled_saved = false;
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
        Task[i].Index = Task_saved[i];
      }
      ActiveWayPoint = active_waypoint_saved;
      AATEnabled = aat_enabled_saved;

      RefreshTask();
    }
  }

  if (active_waypoint_on_entry != ActiveWayPoint){
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
}


void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  static int time_in_flight = 0;
  static int time_on_ground = 0;

  if (Basic->Speed>1.0) {
    // stop system from shutting down if moving
    InterfaceTimeoutReset();
  }
  if (!Basic->NAVWarning) {
    if (Basic->Speed> TAKEOFFSPEEDTHRESHOLD) {
      time_in_flight++;
      time_on_ground=0;
    } else {
      if ((Calculated->AltitudeAGL<300)&&(Calculated->TerrainValid)) {
        time_in_flight--;
      } else if (!Calculated->TerrainValid) {
        time_in_flight--;
      }
      time_on_ground++;
    }
  }

  time_in_flight = min(60, max(0,time_in_flight));
  time_on_ground = min(30, max(0,time_on_ground));

  // JMW logic to detect takeoff and landing is as follows:
  //   detect takeoff when above threshold speed for 10 seconds
  //
  //   detect landing when below threshold speed for 30 seconds
  //
  // TODO: make this more robust by making use of terrain height data
  // if available

  if ((time_on_ground<=10)||(ReplayLogger::IsEnabled())) {
    // Don't allow 'OnGround' calculations if in IGC replay mode
    Calculated->OnGround = FALSE;
  }

  if (!Calculated->Flying) {
    // detect takeoff
    if (time_in_flight>10) {
      Calculated->Flying = TRUE;
      InputEvents::processGlideComputer(GCE_TAKEOFF);
      // reset stats on takeoff
      ResetFlightStats(Basic, Calculated);

      Calculated->TakeOffTime= Basic->Time;

      // save stats in case we never finish
      memcpy(&Finish_Derived_Info, Calculated, sizeof(DERIVED_INFO));

    }
    if (time_on_ground>10) {
      Calculated->OnGround = TRUE;
      DoAutoQNH(Basic, Calculated);
    }
  } else {
    // detect landing
    if (time_in_flight==0) {
      // have been stationary for a minute
      InputEvents::processGlideComputer(GCE_LANDING);

      // JMWX  restore data calculated at finish so
      // user can review flight as at finish line

      if (Calculated->ValidFinish) {
        double flighttime = Calculated->FlightTime;
        double takeofftime = Calculated->TakeOffTime;
        memcpy(Calculated, &Finish_Derived_Info, sizeof(DERIVED_INFO));
        Calculated->FlightTime = flighttime;
        Calculated->TakeOffTime = takeofftime;
      }
      Calculated->Flying = FALSE;
    }

  }
}


//////////

void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // nothing yet.
}

double EffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {

  int i;

  if (Calculated->ValidFinish) return 0;
  if (ActiveWayPoint==0) return 0; // no e mc before start
  if (!Calculated->ValidStart) return 0;
  if (Calculated->TaskStartTime<0) return 0;

  if (!ValidTaskPoint(ActiveWayPoint)
      || !ValidTaskPoint(ActiveWayPoint-1)) return 0;

  LockTaskData();

  double mc_effective = 0.0;
  double mc_effective_found = 10.0;

  bool is_final = ActiveIsFinalWaypoint();

  for (mc_effective=0.1; mc_effective<10.0; mc_effective+= 0.1) {

    double time_total=0;
    double time_this;
    double leg_covered, leg_distance, leg_bearing;

    double total_distance= 0; // used for testing

    double w1lat = WayPointList[Task[ActiveWayPoint].Index].Latitude;
    double w1lon = WayPointList[Task[ActiveWayPoint].Index].Longitude;
    double w0lat = WayPointList[Task[ActiveWayPoint-1].Index].Latitude;
    double w0lon = WayPointList[Task[ActiveWayPoint-1].Index].Longitude;

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
                    &leg_distance, &leg_bearing);

    leg_covered = ProjectedDistance(w0lon, w0lat,
                                   w1lon, w1lat,
                                   Basic->Longitude,
                                   Basic->Latitude);

    total_distance += leg_covered;

    if ((StartLine==0) && (ActiveWayPoint==1)) {
      // Correct speed calculations for radius
      // JMW TODO: replace this with more accurate version
      // leg_distance -= StartRadius;
      leg_covered = max(0.1,leg_covered-StartRadius);
    }

    GlidePolar::MacCreadyAltitude(mc_effective,
                                  leg_covered,
                                  leg_bearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  0, NULL,
                                  is_final,
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
                      &leg_distance, &leg_bearing);

      if ((StartLine==0) && (i==1)) {
        // Correct speed calculations for radius
        // JMW TODO: replace this with more accurate version
        // leg_distance -= StartRadius;
        leg_distance = max(0.1,leg_distance-StartRadius);
      }

      total_distance += leg_distance;

      GlidePolar::MacCreadyAltitude(mc_effective,
                                    leg_distance,
                                    leg_bearing,
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

    double time_climb = (SpeedHeight(Basic, Calculated))/mc_effective;
    time_total += time_climb;

    if (time_total<0) continue;

    double telapsed = Basic->Time-Calculated->TaskStartTime;

    if (time_total<telapsed) {
      mc_effective_found = mc_effective;
      break;
    }

  }

  UnlockTaskData();

  return mc_effective_found;
}
