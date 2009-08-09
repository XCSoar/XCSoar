/*
Copyright_License {

  XCSoar Glide Computer - http://xcsoar.sourceforge.net/
  Copyright (C) 2000 - 2008

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

  $Id$
}

*/

#include "StdAfx.h"
#include "Defines.h" // VENTA3
#include "Calculations.h"
#include "Dialogs.h"
#include "Parser.h"
#include "compatibility.h"
#ifdef OLDPPC
#include "XCSoarProcess.h"
#else
#include "Process.h"
#endif
#include "Utils.h"
#include "Utils2.h"
#include "externs.h"
#include "McReady.h"
#include "Airspace.h"
#include "AirspaceWarning.h"
#include "Logger.h"
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
#include "device.h"
#ifdef NEWCLIMBAV
#include "ClimbAverageCalculator.h" // JMW new
#endif

WindAnalyser *windanalyser = NULL;
OLCOptimizer olc;
AATDistance aatdistance;
static DERIVED_INFO Finish_Derived_Info;
static VegaVoice vegavoice;
static ThermalLocator thermallocator;
#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2
int AutoWindMode= D_AUTOWIND_CIRCLING;

// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both

bool EnableNavBaroAltitude=false;
int EnableExternalTriggerCruise=false;
bool ExternalTriggerCruise= false;
bool ExternalTriggerCircling= false;
bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;
int  AutoMcMode = 0;
bool EnableFAIFinishHeight = false;
bool BallastTimerActive = false;

// 0: Final glide only
// 1: Set to average if in climb mode
// 2: Average if in climb mode, final glide in final glide mode

#define THERMAL_TIME_MIN 45.0

double CRUISE_EFFICIENCY = 1.0;

static double SpeedHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void LD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Heading(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void CruiseLD(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void AverageThermal(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void Turning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void PercentCircling(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                            const double Rate);
static void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void ThermalGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void MaxHeightGain(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void DoAlternates(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int AltWaypoint); // VENTA3
static void EnergyHeightNavAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      const double this_maccready);
static void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			     const double this_maccready);
static void LDNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double LegToGo);

static void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double this_maccready);
static void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static bool  InFinishSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const int i);
static bool  InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const int i);
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

extern void BallastDump();

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

  if (!TaskIsTemporary() && (wp==FinalWayPoint)) {
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

  double dh_start = Calculated->TaskStartAltitude;

  double dh_finish = FAIFinishHeight(Basic, Calculated, -1);

  // Excess height
  return Calculated->NavAltitude
    - (dh_start*(1.0-d_fraction)+dh_finish*(d_fraction));
}



void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double bearing, distance;
  double lat, lon;
  bool out_of_range;

  // estimate max range (only interested in at most one screen distance away)
  // except we need to scan for terrain base, so 20km search minimum is required
  double mymaxrange = max(20000.0, MapWindow::GetApproxScreenRange());

  Calculated->TerrainBase = Calculated->TerrainAlt;

  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    bearing = (i*360.0)/NUMTERRAINSWEEPS;
    distance = FinalGlideThroughTerrain(bearing,
                                        Basic,
                                        Calculated, &lat, &lon,
                                        mymaxrange, &out_of_range,
					&Calculated->TerrainBase);
    if (out_of_range) {
      FindLatitudeLongitude(Basic->Latitude, Basic->Longitude,
                            bearing,
                            mymaxrange*20,
                            &lat, &lon);
    }
    Calculated->GlideFootPrint[i].x = lon;
    Calculated->GlideFootPrint[i].y = lat;
  }
  Calculated->Experimental = Calculated->TerrainBase;
}


int FinishLine=1;
DWORD FinishRadius=1000;


void RefreshTaskStatistics(void) {
  //  LockFlightData();
  LockTaskData();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  TaskSpeed(&GPS_INFO, &CALCULATED_INFO, MACCREADY);
  IterateEffectiveMacCready(&GPS_INFO, &CALCULATED_INFO);
  UnlockTaskData();
  //  UnlockFlightData();
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

void AnnounceWayPointSwitch(DERIVED_INFO *Calculated, bool do_advance) {
  if (ActiveWayPoint == 0) {
//    InputEvents::processGlideComputer(GCE_TASK_START);
    TCHAR TempTime[40];
    TCHAR TempAlt[40];
    TCHAR TempSpeed[40];
    Units::TimeToText(TempTime, (int)TimeLocal((int)Calculated->TaskStartTime));
    _stprintf(TempAlt, TEXT("%.0f %s"),
              Calculated->TaskStartAltitude*ALTITUDEMODIFY,
              Units::GetAltitudeName());
    _stprintf(TempSpeed, TEXT("%.0f %s"),
             Calculated->TaskStartSpeed*TASKSPEEDMODIFY,
             Units::GetTaskSpeedName());

    TCHAR TempAll[120];
    _stprintf(TempAll, TEXT("\r\nAltitude: %s\r\nSpeed:%s\r\nTime: %s"), TempAlt, TempSpeed, TempTime);

    DoStatusMessage(TEXT("Task Start"), TempAll);

  } else if (Calculated->ValidFinish && IsFinalWaypoint()) {
    InputEvents::processGlideComputer(GCE_TASK_FINISH);
  } else {
    InputEvents::processGlideComputer(GCE_TASK_NEXTWAYPOINT);
  }

  if (do_advance) {
    ActiveWayPoint++;
  }

  SelectedWaypoint = ActiveWayPoint;
  // set waypoint detail to active task WP

  // start logging data at faster rate
  FastLogNum = 5;
}


double LowPassFilter(double y_last, double x_in, double fact) {
  return (1.0-fact)*y_last+(fact)*x_in;
}


void SpeedToFly(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double n;
  // get load factor
  if (Basic->AccelerationAvailable) {
    n = fabs(Basic->Gload);
  } else {
    n = fabs(Calculated->Gload);
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
      GlidePolar::MacCreadyRisk(Calculated->NavAltitude+Calculated->EnergyHeight
                                -SAFETYALTITUDEBREAKOFF-Calculated->TerrainBase,
                                Calculated->MaxThermalHeight,
                                MACCREADY);
  }
  Calculated->MacCreadyRisk = risk_mc;

  if (EnableBlockSTF) {
    delta_mc = risk_mc;
  } else {
    delta_mc = risk_mc-Calculated->NettoVario;
  }

  if (1 || (Calculated->Vario <= risk_mc)) {
    // thermal is worse than mc threshold, so find opt cruise speed

    double VOptnew;

    if (!ValidTaskPoint(ActiveWayPoint) || !Calculated->FinalGlide) {
      // calculate speed as if cruising, wind has no effect on opt speed
      GlidePolar::MacCreadyAltitude(delta_mc,
                                    100.0, // dummy value
                                    Basic->TrackBearing,
                                    0.0,
                                    0.0,
                                    NULL,
                                    &VOptnew,
                                    false,
                                    NULL, 0, CRUISE_EFFICIENCY);
    } else {
      GlidePolar::MacCreadyAltitude(delta_mc,
                                    100.0, // dummy value
                                    Basic->TrackBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    &VOptnew,
                                    true,
                                    NULL, 1.0e6, CRUISE_EFFICIENCY);
    }

    // put low pass filter on VOpt so display doesn't jump around
    // too much
    if (Calculated->Vario <= risk_mc) {
      Calculated->VOpt = max(Calculated->VOpt,
			     GlidePolar::Vminsink*sqrt(n));
    } else {
      Calculated->VOpt = max(Calculated->VOpt,
			     GlidePolar::Vminsink);
    }
    Calculated->VOpt = LowPassFilter(Calculated->VOpt,VOptnew, 0.6);

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
    n = fabs(Basic->Gload);
  } else {
    n = fabs(Calculated->Gload);
  }

  // calculate sink rate of glider for calculating netto vario

  bool replay_disabled = !ReplayLogger::IsEnabled();

  double glider_sink_rate;
  if (Basic->AirspeedAvailable && replay_disabled) {
    glider_sink_rate= GlidePolar::SinkRate(max(GlidePolar::Vminsink,
					       Basic->IndicatedAirspeed), n);
  } else {
    // assume zero wind (Speed=Airspeed, very bad I know)
    // JMW TODO accuracy: adjust for estimated airspeed
    glider_sink_rate= GlidePolar::SinkRate(max(GlidePolar::Vminsink,
					       Basic->Speed), n);
  }
  Calculated->GliderSinkRate = glider_sink_rate;

  if (Basic->NettoVarioAvailable && replay_disabled) {
    Calculated->NettoVario = Basic->NettoVario;
  } else {
    if (Basic->VarioAvailable && replay_disabled) {
      Calculated->NettoVario = Basic->Vario - glider_sink_rate;
    } else {
      Calculated->NettoVario = Calculated->Vario - glider_sink_rate;
    }
  }
}


void AudioVario(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  /* JMW disabled, no longer used
#define AUDIOSCALE 100/7.5  // +/- 7.5 m/s range

  if (
      (Basic->AirspeedAvailable &&
       (Basic->IndicatedAirspeed >= NettoSpeed))
      ||
      (!Basic->AirspeedAvailable &&
       (Basic->Speed >= NettoSpeed))
      ) {

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
  */
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
  static double LastTime = 0;
  static double lastHeading = 0;

  if ((Basic->Speed>0)||(Calculated->WindSpeed>0)) {

    x0 = fastsine(Basic->TrackBearing)*Basic->Speed;
    y0 = fastcosine(Basic->TrackBearing)*Basic->Speed;
    x0 += fastsine(Calculated->WindBearing)*Calculated->WindSpeed;
    y0 += fastcosine(Calculated->WindBearing)*Calculated->WindSpeed;

    Calculated->Heading = AngleLimit360(atan2(x0,y0)*RAD_TO_DEG);

    if (!Calculated->Flying) {
      // don't take wind into account when on ground
      Calculated->Heading = Basic->TrackBearing;
    }

    // calculate turn rate in wind coordinates
    if(Basic->Time > LastTime) {
      double dT = Basic->Time - LastTime;

      Calculated->TurnRateWind = AngleLimit180(Calculated->Heading
                                               - lastHeading)/dT;

      lastHeading = Calculated->Heading;
    }
    LastTime = Basic->Time;

    // calculate estimated true airspeed
    mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
    Calculated->TrueAirspeedEstimated = mag;

    // estimate bank angle (assuming balanced turn)
    double angle = atan(DEG_TO_RAD*Calculated->TurnRateWind*
			Calculated->TrueAirspeedEstimated/9.81);

    Calculated->BankAngle = RAD_TO_DEG*angle;
    Calculated->Gload = 1.0/max(0.001,fabs(cos(angle)));

    // estimate pitch angle (assuming balanced turn)
    Calculated->PitchAngle = RAD_TO_DEG*
      atan2(Calculated->GPSVario-Calculated->Vario,
           Calculated->TrueAirspeedEstimated);

    // update zigzag wind
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
  } else {
    Calculated->Heading = Basic->TrackBearing;
  }

}


void  SetWindEstimate(const double wind_speed,
		      const double wind_bearing,
		      const int quality) {
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

/*
   VENTA3-TODO: somewhere introduce BogusMips concept, in order to know what is the CPU speed
                of the local device, and fine-tune some parameters
 */

  static double LastOptimiseTime = 0;
  static double LastSearchBestTime = 0; // VENTA3
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

 // VENTA3 best landing slow calculation
#if (WINDOWSPC>0)
  if ( (OnBestAlternate == true ) && (Basic->Time > LastSearchBestTime+10.0) ) // VENTA3
#else
  if ( (OnBestAlternate == true ) && (Basic->Time > LastSearchBestTime+BESTALTERNATEINTERVAL) ) // VENTA3
#endif
    {
      LastSearchBestTime = Basic->Time;
      SearchBestAlternate(Basic, Calculated);
    }

    // If using a replay IGC file, current time is in the past and LastFlipBoxTime becomes unreachable!
    if ( LastFlipBoxTime > Basic->Time ) LastFlipBoxTime = Basic->Time;

}


// VENTA3 added radial
void DistanceToHome(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  int home_waypoint = HomeWaypoint;

  if (!ValidWayPoint(home_waypoint)) {
    Calculated->HomeDistance = 0.0;
    Calculated->HomeRadial = 0.0; // VENTA3
    return;
  }

  double w1lat = WayPointList[home_waypoint].Latitude;
  double w1lon = WayPointList[home_waypoint].Longitude;
  double w0lat = Basic->Latitude;
  double w0lon = Basic->Longitude;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  &Calculated->HomeDistance, &Calculated->HomeRadial);

}

/*
 * VENTA3 Alternates destinations
 *
 * Used by Alternates and BestAlternate
 *
 * Colors VGR are disabled, but available
 */

void DoAlternates(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int AltWaypoint) { // VENTA3
  if (!ValidWayPoint(AltWaypoint)) {
    return;
  }
  double w1lat = WayPointList[AltWaypoint].Latitude;
  double w1lon = WayPointList[AltWaypoint].Longitude;
  double w0lat = Basic->Latitude;
  double w0lon = Basic->Longitude;
  double *altwp_dist = &WayPointCalc[AltWaypoint].Distance;
  double *altwp_gr   = &WayPointCalc[AltWaypoint].GR;
  double *altwp_arrival = &WayPointCalc[AltWaypoint].AltArriv;
  short  *altwp_vgr  = &WayPointCalc[AltWaypoint].VGR;

  DistanceBearing(w1lat, w1lon,
                  w0lat, w0lon,
                  altwp_dist, NULL);

  double GRsafecalc = Calculated->NavAltitude - (WayPointList[AltWaypoint].Altitude + SAFETYALTITUDEARRIVAL);

  if (GRsafecalc <=0) *altwp_gr = INVALID_GR;
  else {
	*altwp_gr = *altwp_dist / GRsafecalc;
	if ( *altwp_gr >ALTERNATE_MAXVALIDGR || *altwp_gr <0 ) *altwp_gr = INVALID_GR;
	else if ( *altwp_gr <1 ) *altwp_gr = 1;
  }


  // We need to calculate arrival also for BestAlternate, since the last "reachable" could be
  // even 60 seconds old and things may have changed drastically

  *altwp_arrival = CalculateWaypointArrivalAltitude(Basic, Calculated, AltWaypoint);
  if ( (*altwp_arrival - ALTERNATE_OVERSAFETY) >0 ) {
  	if ( *altwp_gr <= (GlidePolar::bestld *SAFELD_FACTOR) ) *altwp_vgr = 1; // full green vgr
  	else
  		if ( *altwp_gr <= GlidePolar::bestld ) *altwp_vgr = 2; // yellow vgr
		else *altwp_vgr =3; // RED vgr
  } else
  {
	*altwp_vgr = 3; // full red
  }


}

void ResetFlightStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      bool full=true) {
  int i;
  (void)Basic;

  CRUISE_EFFICIENCY = 1.0;

  if (full) {
    olc.ResetFlight();
    flightstats.Reset();
    aatdistance.Reset();
    CRUISE_EFFICIENCY = 1.0;
    Calculated->FlightTime = 0;
    Calculated->TakeOffTime = 0;
    Calculated->timeCruising = 0;
    Calculated->timeCircling = 0;
    Calculated->TotalHeightClimb = 0;

    Calculated->CruiseStartTime = -1;
    Calculated->ClimbStartTime = -1;

    Calculated->LDFinish = INVALID_GR;
    Calculated->GRFinish = INVALID_GR;  // VENTA-ADDON GR to final destination
    Calculated->CruiseLD = INVALID_GR;
    Calculated->AverageLD = INVALID_GR;
    Calculated->LDNext = INVALID_GR;
    Calculated->LD = INVALID_GR;
    Calculated->LDvario = INVALID_GR;
    Calculated->AverageThermal = 0;

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


void StartTask(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
	       const bool do_advance,
               const bool do_announce) {
  Calculated->ValidFinish = false;
  Calculated->TaskStartTime = Basic->Time ;
  Calculated->TaskStartSpeed = Basic->Speed;
  Calculated->TaskStartAltitude = Calculated->NavAltitude;
  Calculated->LegStartTime = Basic->Time;
  flightstats.LegStartTime[0] = Basic->Time;
  flightstats.LegStartTime[1] = Basic->Time;

  Calculated->CruiseStartLat = Basic->Latitude;
  Calculated->CruiseStartLong = Basic->Longitude;
  Calculated->CruiseStartAlt = Calculated->NavAltitude;
  Calculated->CruiseStartTime = Basic->Time;

  // JMW TODO accuracy: Get time from aatdistance module since this is
  // more accurate

  // JMW clear thermal climb average on task start
  flightstats.ThermalAverage.Reset();
  flightstats.Task_Speed.Reset();
  Calculated->AverageThermal = 0; // VNT for some reason looked uninitialised
  Calculated->WaypointBearing=0; // VNT TEST

  // JMW reset time cruising/time circling stats on task start
  Calculated->timeCircling = 0;
  Calculated->timeCruising = 0;
  Calculated->TotalHeightClimb = 0;

  // reset max height gain stuff on task start
  Calculated->MaxHeightGain = 0;
  Calculated->MinAltitude = 0;

  if (do_announce) {
    AnnounceWayPointSwitch(Calculated, do_advance);
  } else {
    if (do_advance) {
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
  StartupStore(TEXT("InitCalculations\n"));
  CalibrationInit();
  ResetFlightStats(Basic, Calculated, true);
#ifndef FIVV
  LoadCalculationsPersist(Calculated); // VNT  not for fivv, confusing people
#endif
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
/*
 If you load persistent values, you need at least these reset:
//  Calculated->WindBearing = 0.0; // VENTA3
//  Calculated->LastThermalAverage=0.0; // VENTA7
//  Calculated->ThermalGain=0.0; // VENTA7
 */

  LockFlightData();

  if (!windanalyser) {
    windanalyser = new WindAnalyser();

    //JMW TODO enhancement: seed initial wind store with start conditions
    // SetWindEstimate(Calculated->WindSpeed,Calculated->WindBearing, 1);

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
      if (fabs(fabs(Basic->Gload)-1.0)>0.25) {
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
  if ((Calculated->TaskStartTime>0)
      && (Basic->Time-Calculated->TaskStartTime>0)) {
      if (Calculated->Flying) {

        double effective_mc = EffectiveMacCready(Basic, Calculated);
        DebugStore("%g %g %g %g %g %g %g %g %g %g %d %g %g # taskspeed\r\n",
                Basic->Time-Calculated->TaskStartTime,
                Calculated->TaskDistanceCovered,
                Calculated->TaskDistanceToGo,
                Calculated->TaskAltitudeRequired,
                Calculated->NavAltitude,
                Calculated->TaskSpeedAchieved,
                Calculated->TaskSpeed,
                Calculated->TaskSpeedInstantaneous,
                MACCREADY,
                effective_mc,
                ActiveWayPoint,
                Calculated->DistanceVario,
                Calculated->GPSVario);
      }
    }
#endif
}

extern bool TargetDialogOpen;

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
  if (!TargetDialogOpen) {
    // don't calculate these if optimise function being invoked or
    // target is being adjusted
    TaskStatistics(Basic, Calculated, MACCREADY);
    AATStats(Basic, Calculated);
    TaskSpeed(Basic, Calculated, MACCREADY);
  }

  if (!FlightTimes(Basic, Calculated)) {
    // time hasn't advanced, so don't do calculations requiring an advance
    // or movement
    return FALSE;
  }

  Turning(Basic, Calculated);
  LD(Basic,Calculated);
  CruiseLD(Basic,Calculated);
  Calculated->AverageLD=CalculateLDRotary(&rotaryLD); // AverageLD
  Average30s(Basic,Calculated);
  AverageThermal(Basic,Calculated);
  AverageClimbRate(Basic,Calculated);
  ThermalGain(Basic,Calculated);
  LastThermalStats(Basic, Calculated);
  //  ThermalBand(Basic, Calculated); moved to % circling function
  MaxHeightGain(Basic,Calculated);

  PredictNextPosition(Basic, Calculated);
  CalculateOwnTeamCode(Basic, Calculated);
  CalculateTeammateBearingRange(Basic, Calculated);

  BallastDump();

  if (!TaskIsTemporary()) {
    InSector(Basic, Calculated);
    DoAutoMacCready(Basic, Calculated);
    IterateEffectiveMacCready(Basic, Calculated);
    DebugTaskCalculations(Basic, Calculated);
  }

  // VENTA3 Alternates
  if ( OnAlternate1 == true ) DoAlternates(Basic, Calculated,Alternate1);
  if ( OnAlternate2 == true ) DoAlternates(Basic, Calculated,Alternate2);
  if ( OnBestAlternate == true ) DoAlternates(Basic, Calculated,BestAlternate);

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
  double V_mc_tas = Calculated->VMacCready*ias_to_tas;
  V_tas = max(V_tas, V_bestld_tas);
  double V_target = max(V_bestld_tas, V_mc_tas);
  Calculated->EnergyHeight =
    (V_tas*V_tas-V_target*V_target)/(9.81*2.0);
}



void Vario(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double LastAlt = 0;
  static double LastAltTE = 0;
  static double h0last = 0;

  if(Basic->Time <= LastTime) {
    LastTime = Basic->Time;
  } else {
    double Gain = Calculated->NavAltitude - LastAlt;
    double GainTE = (Calculated->EnergyHeight+Basic->Altitude) - LastAltTE;
    double dT = (Basic->Time - LastTime);
    // estimate value from GPS
    Calculated->GPSVario = Gain / dT;
    Calculated->GPSVarioTE = GainTE / dT;

    double dv = (Calculated->TaskAltitudeDifference-h0last)
      /(Basic->Time-LastTime);
    Calculated->DistanceVario = LowPassFilter(Calculated->DistanceVario,
                                              dv, 0.1);

    h0last = Calculated->TaskAltitudeDifference;

    LastAlt = Calculated->NavAltitude;
    LastAltTE = Calculated->EnergyHeight+Basic->Altitude;
    LastTime = Basic->Time;

  }

  if (!Basic->VarioAvailable || ReplayLogger::IsEnabled()) {
    Calculated->Vario = Calculated->GPSVario;

  } else {
    // get value from instrument
    Calculated->Vario = Basic->Vario;
    // we don't bother with sound here as it is polled at a
    // faster rate in the DoVarioCalcs methods

    CalibrationUpdate(Basic, Calculated);
  }
}

#ifdef NEWCLIMBAV
ClimbAverageCalculator climbAverageCalculator;
void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
	Calculated->Average30s = climbAverageCalculator.GetAverage(Basic->Time, Basic->Altitude, 30);
	Calculated->NettoAverage30s = Calculated->Average30s;
}

#endif

void Average30s(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;
  static double Altitude[30];
  static double Vario[30];
  static double NettoVario[30];
  int Elapsed, i;
  long index = 0;
  double Gain;
  static int num_samples = 0;
  static BOOL lastCircling = false;

  if(Basic->Time > LastTime)
    {

      if (Calculated->Circling != lastCircling) {
        num_samples = 0;
        // reset!
      }
      lastCircling = Calculated->Circling;

      Elapsed = (int)(Basic->Time - LastTime);
      for(i=0;i<Elapsed;i++)
        {
          index = (long)LastTime + i;
          index %= 30;

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

          if (num_samples<30) {
            num_samples ++;
          }

        }

      double Vave = 0;
      double NVave = 0;
      int j;
      for (i=0; i< num_samples; i++) {
        j = (index - i) % 30;
        if (j<0) {
          j += 30;
        }
        Vave += Vario[j];
	NVave += NettoVario[j];
      }
      if (num_samples) {
        Vave /= num_samples;
        NVave /= num_samples;
      }

      if (!Basic->VarioAvailable) {
        index = ((long)Basic->Time - 1)%30;
        Gain = Altitude[index];

        index = ((long)Basic->Time)%30;
        Gain = Gain - Altitude[index];

        Vave = Gain/30;
      }
      Calculated->Average30s =
        LowPassFilter(Calculated->Average30s,Vave,0.8);
      Calculated->NettoAverage30s =
        LowPassFilter(Calculated->NettoAverage30s,NVave,0.8);

#ifdef DEBUGAVERAGER
      if (Calculated->Flying) {
        DebugStore("%d %g %g %g # averager\r\n",
                num_samples,
                Calculated->Vario,
                Calculated->Average30s, Calculated->NettoAverage30s);
      }
#endif

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
          Calculated->NavAltitude+Calculated->EnergyHeight
            - Calculated->ClimbStartAlt;
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
          Calculated->NavAltitude + Calculated->EnergyHeight
          - Calculated->ClimbStartAlt;
      }
  }
}


double LimitLD(double LD) {
  if (fabs(LD)>INVALID_GR) {
    return INVALID_GR;
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
    if (fabs(glideangle) > 1.0/INVALID_GR) {
      LD = LimitLD(1.0/glideangle);
    } else {
      LD = INVALID_GR;
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
    Calculated->LDvario = INVALID_GR;
    Calculated->LD = INVALID_GR;
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

      InsertLDRotary(&rotaryLD,(int)DistanceFlown, (int)Calculated->NavAltitude);

      LastLat = Basic->Latitude;
      LastLon = Basic->Longitude;
      LastAlt = Calculated->NavAltitude;
      LastTime = Basic->Time;
    }

  // LD instantaneous from vario, updated every reading..
  if (Basic->VarioAvailable && Basic->AirspeedAvailable
      && Calculated->Flying) {
    Calculated->LDvario = UpdateLD(Calculated->LDvario,
                                   Basic->IndicatedAirspeed,
                                   -Basic->Vario,
                                   0.3);
  } else {
    Calculated->LDvario = INVALID_GR;
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


#define MinTurnRate  4
#define CruiseClimbSwitch 15
#define ClimbCruiseSwitch 10


void SwitchZoomClimb(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                     bool isclimb, bool left) {

  // this is calculation stuff, leave it there
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    LockFlightData();
    windanalyser->slot_newFlightMode(Basic, Calculated, left, 0);
    UnlockFlightData();
  }

}


void PercentCircling(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                     const double Rate) {
  // JMW circling % only when really circling,
  // to prevent bad stats due to flap switches and dolphin soaring

  if (Calculated->Circling && (Rate>MinTurnRate)) {
    //    timeCircling += (Basic->Time-LastTime);
    Calculated->timeCircling+= 1.0;
    Calculated->TotalHeightClimb += Calculated->GPSVario;
    ThermalBand(Basic, Calculated);
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
  static double StartEnergyHeight = 0;
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

  Rate = AngleLimit180(Basic->TrackBearing-LastTrack)/dT;

  if (dT<2.0) {
    // time step ok

    // calculate acceleration
    dRate = (Rate-LastRate)/dT;

    double dtlead=0.3;
    // integrate assuming constant acceleration, for one second
    Calculated->NextTrackBearing = Basic->TrackBearing
      + dtlead*(Rate+0.5*dtlead*dRate);
    // s = u.t+ 0.5*a*t*t

    Calculated->NextTrackBearing =
      AngleLimit360(Calculated->NextTrackBearing);

  } else {
    // time step too big, so just take it at last measurement
    Calculated->NextTrackBearing = Basic->TrackBearing;
  }

  Calculated->TurnRate = Rate;

  // JMW limit rate to 50 deg per second otherwise a big spike
  // will cause spurious lock on circling for a long time
  if (Rate>50) {
    Rate = 50;
  }
  if (Rate<-50) {
    Rate = -50;
  }

  // average rate, to detect essing
  static double rate_history[60];
  double rate_ave=0;
  for (int i=59; i>0; i--) {
    rate_history[i] = rate_history[i-1];
    rate_ave += rate_history[i];
  }
  rate_history[0] = Rate;
  rate_ave /= 60;

  Calculated->Essing = fabs(rate_ave)*100/MinTurnRate;
  if (fabs(rate_ave)< MinTurnRate*2) {
    //    Calculated->Essing = rate_ave;
  }

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

  PercentCircling(Basic, Calculated, Rate);

  LastTrack = Basic->TrackBearing;

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
      StartEnergyHeight  = Calculated->EnergyHeight;
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
        Calculated->ClimbStartAlt = StartAlt+StartEnergyHeight;
        Calculated->ClimbStartTime = StartTime;

        if (flightstats.Altitude_Ceiling.sum_n>0) {
          // only update base if have already climbed, otherwise
          // we will catch the takeoff height as the base.

          flightstats.Altitude_Base.
            least_squares_update(max(0,Calculated->ClimbStartTime
                                     - Calculated->TakeOffTime)/3600.0,
                                 StartAlt);
        }

        // consider code: InputEvents GCE - Move this to InputEvents
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
      StartEnergyHeight  = Calculated->EnergyHeight;
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

 	InitLDRotary(&rotaryLD);

        flightstats.Altitude_Ceiling.
          least_squares_update(max(0,Calculated->CruiseStartTime
                                   - Calculated->TakeOffTime)/3600.0,
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
          double ThermalGain = Calculated->CruiseStartAlt + Calculated->EnergyHeight
            - Calculated->ClimbStartAlt;

          if (ThermalGain>0) {
            if (ThermalTime>THERMAL_TIME_MIN) {

	      Calculated->LastThermalAverage = ThermalGain/ThermalTime;
	      Calculated->LastThermalGain = ThermalGain;
	      Calculated->LastThermalTime = ThermalTime;

              flightstats.ThermalAverage.
                least_squares_update(Calculated->LastThermalAverage);

#ifdef DEBUG_STATS
              DebugStore("%f %f # thermal stats\n",
                      flightstats.ThermalAverage.m,
                      flightstats.ThermalAverage.b
                      );
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


double AATCloseBearing(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // ensure waypoint goes in direction of track if very close
  double course_bearing;
  DistanceBearing(Task[ActiveWayPoint-1].AATTargetLat,
		  Task[ActiveWayPoint-1].AATTargetLon,
		  Basic->Latitude,
		  Basic->Longitude,
		  NULL, &course_bearing);

  course_bearing = AngleLimit360(course_bearing+
				 Task[ActiveWayPoint].AATTargetOffsetRadial);
  return course_bearing;
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

      if (AATEnabled && !TaskIsTemporary()
	  && (ActiveWayPoint>0) &&
          ValidTaskPoint(ActiveWayPoint+1)) {

        w1lat = Task[ActiveWayPoint].AATTargetLat;
        w1lon = Task[ActiveWayPoint].AATTargetLon;

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        w1lat, w1lon,
                        &Calculated->WaypointDistance,
                        &Calculated->WaypointBearing);

        if (Calculated->WaypointDistance>AATCloseDistance()*3.0) {
          Calculated->ZoomDistance = max(Calculated->WaypointDistance,
                                         Calculated->ZoomDistance);
        } else {
	  Calculated->WaypointBearing = AATCloseBearing(Basic, Calculated);
        }

      } else if ((ActiveWayPoint==0) && (ValidTaskPoint(ActiveWayPoint+1))
                 && (Calculated->IsInSector) &&
		 !TaskIsTemporary()) {

        // JMW set waypoint bearing to start direction if in start sector

        if (AATEnabled) {
          w1lat = Task[ActiveWayPoint+1].AATTargetLat;
          w1lon = Task[ActiveWayPoint+1].AATTargetLon;
        } else {
          w1lat = WayPointList[Task[ActiveWayPoint+1].Index].Latitude;
          w1lon = WayPointList[Task[ActiveWayPoint+1].Index].Longitude;
        }

        DistanceBearing(Basic->Latitude, Basic->Longitude,
                        w1lat, w1lon,
                        NULL,
                        &Calculated->WaypointBearing);
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
                      const double this_maccready)
{
  //  LockFlightData();
  (void)Basic;
  LockTaskData();
  if(ValidTaskPoint(ActiveWayPoint))
    {
      double wp_alt = FAIFinishHeight(Basic, Calculated, ActiveWayPoint);
      double height_above_wp =
	Calculated->NavAltitude + Calculated->EnergyHeight
	- wp_alt;

      Calculated->NextAltitudeRequired =
        GlidePolar::MacCreadyAltitude(this_maccready,
                        Calculated->WaypointDistance,
                        Calculated->WaypointBearing,
                        Calculated->WindSpeed, Calculated->WindBearing,
                        0, 0,
			true,
			NULL, height_above_wp, CRUISE_EFFICIENCY
                        );
      // JMW CHECK FGAMT

	// VENTA6
	if (this_maccready==0 ) Calculated->NextAltitudeRequired0=Calculated->NextAltitudeRequired;
        else
	      Calculated->NextAltitudeRequired0 =
		GlidePolar::MacCreadyAltitude(0,
				Calculated->WaypointDistance,
				Calculated->WaypointBearing,
				Calculated->WindSpeed, Calculated->WindBearing,
				0, 0,
				true,
				NULL, height_above_wp, CRUISE_EFFICIENCY
				);



      Calculated->NextAltitudeRequired += wp_alt;
      Calculated->NextAltitudeRequired0 += wp_alt; // VENTA6

      Calculated->NextAltitudeDifference =
        Calculated->NavAltitude
        + Calculated->EnergyHeight
        - Calculated->NextAltitudeRequired;

      Calculated->NextAltitudeDifference0 =
        Calculated->NavAltitude
        + Calculated->EnergyHeight
        - Calculated->NextAltitudeRequired0;
    }
  else
    {
      Calculated->NextAltitudeRequired = 0;
      Calculated->NextAltitudeDifference = 0;
      Calculated->NextAltitudeDifference0 = 0; // VENTA6
    }
  UnlockTaskData();
  //  UnlockFlightData();
}


bool InTurnSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const int the_turnpoint)
{
  double AircraftBearing;

  if (!ValidTaskPoint(the_turnpoint)) return false;

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
      DistanceBearing(WayPointList[Task[the_turnpoint].Index].Latitude,
                      WayPointList[Task[the_turnpoint].Index].Longitude,
                      Basic->Latitude ,
                      Basic->Longitude,
                      NULL, &AircraftBearing);
      UnlockTaskData();

      AircraftBearing = AircraftBearing - Task[the_turnpoint].Bisector ;
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
            if(Calculated->WaypointDistance < SectorRadius)
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

bool InAATTurnSector(const double longitude, const double latitude,
                    const int the_turnpoint)
{
  double AircraftBearing;
  bool retval = false;

  if (!ValidTaskPoint(the_turnpoint)) {
    return false;
  }

  double distance;
  LockTaskData();
  DistanceBearing(WayPointList[Task[the_turnpoint].Index].Latitude,
                  WayPointList[Task[the_turnpoint].Index].Longitude,
                  latitude,
                  longitude,
                  &distance, &AircraftBearing);

  if(Task[the_turnpoint].AATType ==  CIRCLE) {
    if(distance < Task[the_turnpoint].AATCircleRadius) {
      retval = true;
    }
  } else if(distance < Task[the_turnpoint].AATSectorRadius) {
    if (AngleInRange(Task[the_turnpoint].AATStartRadial,
                     Task[the_turnpoint].AATFinishRadial,
                     AngleLimit360(AircraftBearing), true)) {
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
		    const int i)
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


/*

  Track 'TaskStarted' in Calculated info, so it can be
  displayed in the task status dialog.

  Must be reset at start of flight.

  For multiple starts, after start has been passed, need
  to set the first waypoint to the start waypoint and
  then recalculate task stats.

*/

bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin) {
  bool valid = true;
  if (StartMaxSpeed!=0) {
    if (Basic->AirspeedAvailable) {
      if (Basic->IndicatedAirspeed>(StartMaxSpeed+Margin))
        valid = false;
    } else {
      if (Basic->Speed>(StartMaxSpeed+Margin))
        valid = false;
    }
  }
  return valid;
}

bool ValidStartSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  return ValidStartSpeed(Basic, Calculated, 0);
}

bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated, DWORD Margin) {
  bool valid = true;
  if ((StartMaxHeight!=0)&&(Calculated->TerrainValid)) {
    if (StartHeightRef == 0) {
      if (Calculated->AltitudeAGL>(StartMaxHeight+Margin))
	valid = false;
    } else {
      if (Calculated->NavAltitude>(StartMaxHeight+Margin))
	valid = false;
    }
  }
  return valid;
}

bool InsideStartHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  return InsideStartHeight(Basic, Calculated, 0);
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


static bool InStartSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated, int &index,
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

  bool in_height = true;

  if ((ActiveWayPoint>0)
      && !ValidTaskPoint(ActiveWayPoint+1)) {
    // don't detect start if finish is selected
    retval = false;
    goto OnExit;
  }

// ToLo: do "soft" check for height only
  in_height = InsideStartHeight(Basic, Calculated, StartMaxHeightMargin);

  if ((Task[0].Index != EntryStartSector) && (EntryStartSector>=0)) {
    LastInSector = false;
    EntryStartSector = Task[0].Index;
  }

  isInSector = InStartSector_Internal(Basic, Calculated,
                                      Task[0].Index, Task[0].OutBound,
                                      LastInSector);
  isInSector &= in_height;

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
	retval &= in_height;
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
    // ToLo: we are ready to start even when outside start rules but within margin
    if (ValidStartSpeed(Basic, Calculated, StartMaxSpeedMargin)) {
      ReadyToAdvance(Calculated, false, true);
    }
    // TODO accuracy: monitor start speed throughout time in start sector
  }
  if (StartCrossed) {
    // ToLo: Check weather speed and height are within the rules or not (zero margin)
    if(!IsFinalWaypoint() && ValidStartSpeed(Basic, Calculated) && InsideStartHeight(Basic, Calculated)) {

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
      // JMW TODO accuracy: This causes Vaverage to go bonkers
      // if the user has already passed the start
      // but selects the start

      // Note: pilot must have armed advance
      // for the start to be registered

    // ToLo: If speed and height are outside the rules they must be within the margin...
    } else {

      if ((ActiveWayPoint<=1)
          && !IsFinalWaypoint()
          && (Calculated->ValidStart==false)
          && (Calculated->Flying)) {

        // need to detect bad starts, just to get the statistics
        // in case the bad start is the best available, or the user
        // manually started
        StartTask(Basic, Calculated, false, false);
//        Calculated->ValidStart = false;

        bool startTaskAnyway = false;

        if (ReadyToAdvance(Calculated, true, true)) {
          //DoStatusMessage(TEXT("Start Anyway?"));
          dlgStartTaskShowModal(&startTaskAnyway,
                                Calculated->TaskStartTime,
                                Calculated->TaskStartSpeed,
                                Calculated->TaskStartAltitude);
          if (startTaskAnyway) {
            ActiveWayPoint=0; // enforce this since it may be 1
            StartTask(Basic,Calculated, true, true);
          }
        }

        Calculated->ValidStart = startTaskAnyway;

        if (Calculated->Flying) {
          Calculated->ValidFinish = false;
        }

	// TODO: Display infobox when only a bit over start rules
      }

    }
  }
}


static BOOL CheckRestart(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                         int *LastStartSector) {
  if((Basic->Time - Calculated->TaskStartTime < 3600)
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
      flightstats.LegStartTime[ActiveWayPoint] = Basic->Time;
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
      AddAATPoint(Basic, Calculated, ActiveWayPoint-1);
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
  short Alt = 0;

  RasterTerrain::Lock();
  // want most accurate rounding here
  RasterTerrain::SetTerrainRounding(0,0);
  Alt = RasterTerrain::GetTerrainHeight(Basic->Latitude,
                                        Basic->Longitude);
  RasterTerrain::Unlock();

  if(Alt<0) {
    Alt = 0;
    if (Alt <= TERRAIN_INVALID) {
      Calculated->TerrainValid = false;
    } else {
      Calculated->TerrainValid = true;
    }
    Calculated->TerrainAlt = 0;
  } else {
    Calculated->TerrainValid = true;
    Calculated->TerrainAlt = Alt;
  }
  Calculated->AltitudeAGL = Calculated->NavAltitude - Calculated->TerrainAlt;
  if (!FinalGlideTerrain) {
    Calculated->TerrainBase = Calculated->TerrainAlt;
  }
}


/////////////////////////////////////////

static bool TaskAltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                                 double this_maccready, double *Vfinal,
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

  double height_above_finish = FAIFinishHeight(Basic, Calculated, 0)-
    FAIFinishHeight(Basic, Calculated, -1);

  for(i=MAXTASKPOINTS-2;i>=0;i--) {


    if (!ValidTaskPoint(i) || !ValidTaskPoint(i+1)) continue;

    w1lat = WayPointList[Task[i].Index].Latitude;
    w1lon = WayPointList[Task[i].Index].Longitude;
    w0lat = WayPointList[Task[i+1].Index].Latitude;
    w0lon = WayPointList[Task[i+1].Index].Longitude;

    if (AATEnabled) {
      w1lat = Task[i].AATTargetLat;
      w1lon = Task[i].AATTargetLon;
      if (!isfinal) {
        w0lat = Task[i+1].AATTargetLat;
        w0lon = Task[i+1].AATTargetLon;
      }
    }

    DistanceBearing(w1lat, w1lon,
                    w0lat, w0lon,
                    &LegDistance, &LegBearing);

    *TotalDistance += LegDistance;

    LegAltitude =
      GlidePolar::MacCreadyAltitude(this_maccready,
                                    LegDistance,
                                    LegBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    0,
                                    true,
                                    &LegTime,
				    height_above_finish,
				    CRUISE_EFFICIENCY
                                    );

    // JMW CHECK FGAMT
    height_above_finish-= LegAltitude;

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

  TotalAltitude += FAIFinishHeight(Basic, Calculated, -1);

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
                              double this_maccready)
{
  double mc_val = this_maccready;
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
    } else if (Calculated->AverageThermal>0) {
      // insufficient stats, so use this/last thermal's average
      mc_val = Calculated->AverageThermal;
    }
  }
  return max(0.1, mc_val);

}


void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double this_maccready)
{
  int ifinal;
  static double LastTime = 0;
  static double LastTimeStats = 0;
  double TotalTime=0, TotalDistance=0, Vfinal=0;

  if (!ValidTaskPoint(ActiveWayPoint)) return;
  if (TaskIsTemporary()) return;
  if (Calculated->ValidFinish) return;
  if (!Calculated->Flying) return;

  // in case we leave early due to error
  Calculated->TaskSpeedAchieved = 0;
  Calculated->TaskSpeed = 0;

  if (ActiveWayPoint<=0) { // no task speed before start
    Calculated->TaskSpeedInstantaneous = 0;
    return;
  }

  //  LockFlightData();
  LockTaskData();

  if (TaskAltitudeRequired(Basic, Calculated, this_maccready, &Vfinal,
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
      Calculated->TaskSpeedInstantaneous = 0;
      //?      Calculated->TaskSpeed = 0;
      goto OnExit;
    }

    // JB's task speed...
    double hx = max(0,SpeedHeight(Basic, Calculated));
    double t1mod = t1-hx/MacCreadyOrAvClimbRate(Basic, Calculated, this_maccready);
    // only valid if flown for 5 minutes or more
    if (t1mod>300.0) {
      Calculated->TaskSpeedAchieved = d1/t1mod;
    } else {
      Calculated->TaskSpeedAchieved = d1/t1;
    }
    Calculated->TaskSpeed = Calculated->TaskSpeedAchieved;

    if (Vfinal<=0) {
      // can't reach target at current mc
      goto OnExit;
    }

    // distance that can be usefully final glided from here
    // (assumes average task glide angle of d0/h0)
    // JMW TODO accuracy: make this more accurate by working out final glide
    // through remaining turnpoints.  This will more correctly account
    // for wind.

    dFinal = min(dr, d0*min(1.0,max(0.0,h1/h0)));

    if (Calculated->ValidFinish) {
      dFinal = 0;
    }

    double dc = max(0,dr-dFinal);
    // amount of extra distance to travel in cruise/climb before final glide

    // equivalent distance to end of final glide
    d2 = d1+dFinal;

    // time at end of final glide
    t2 = t1+dFinal/Vfinal;

    // actual task speed achieved so far
    v1 = d1/t1;

#ifdef OLDTASKSPEED
    // average speed to end of final glide from here
    v2 = d2/t2;
    Calculated->TaskSpeed = max(v1,v2);
#else
    // average speed to end of final glide from here, weighted
    // according to how much extra time would be spent in cruise/climb
    // the closer dc (the difference between remaining distance and
    // final glidable distance) gets to zero, the closer v2 approaches
    // the average speed to end of final glide from here
    // in other words, the more we consider the final glide part to have
    // been earned.

    // this will be bogus at fast starts though...
    if (v1>0) {
      v2 = (d1+dc+dFinal)/(t1+dc/v1+dFinal/Vfinal);
    } else {
      v2 = (d1+dFinal)/(t1+dFinal/Vfinal);
    }
    Calculated->TaskSpeed = v2;
#endif

    double konst = 1.1;
    if (TaskModified)
      {
	konst = 1.0;
      }

    double termikLigaPoints = 0;
    if (d1 > 0)
      {
	termikLigaPoints = konst*(0.015*0.001*d1-(400.0/(0.001*d1))+12.0)*v1*3.6*100.0/(double)Handicap;
      }

    Calculated->TermikLigaPoints = termikLigaPoints;

    if(Basic->Time < LastTime) {
      LastTime = Basic->Time;
    } else if (Basic->Time-LastTime >=1.0) {

      double dt = Basic->Time-LastTime;
      LastTime = Basic->Time;

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

      static double dr_last = 0;

      double mc_safe = max(0.1,this_maccready);
      double Vstar = max(1.0,Calculated->VMacCready);
      double vthis = (Calculated->LegDistanceCovered-dr_last)/dt;
      vthis /= AirDensityRatio(Calculated->NavAltitude);

      dr_last = Calculated->LegDistanceCovered;
      double ttg = max(1,Calculated->LegTimeToGo);
      //      double Vav = d0/max(1.0,t0);
      double Vrem = Calculated->LegDistanceToGo/ttg;
      double Vref = // Vav;
	Vrem;
      double sr = -GlidePolar::SinkRate(Vstar);
      double height_diff = max(0,-Calculated->TaskAltitudeDifference);

      if (Calculated->timeCircling>30) {
	mc_safe = max(this_maccready,
		      Calculated->TotalHeightClimb/Calculated->timeCircling);
      }
      // circling percentage during cruise/climb
      double rho_cruise = max(0.0,min(1.0,mc_safe/(sr+mc_safe)));
      double rho_climb = 1.0-rho_cruise;
      double time_climb = height_diff/mc_safe;

      // calculate amount of time in cruise/climb glide
      double rho_c = max(0,min(1,time_climb/ttg));

      if (Calculated->FinalGlide) {
	if (rho_climb>0) {
	  rho_c = max(0,min(1,rho_c/rho_climb));
	}
	if (!Calculated->Circling) {
	  if (Calculated->TaskAltitudeDifference>0) {
	    rho_climb *= rho_c;
	    rho_cruise *= rho_c;
	    // Vref = Vrem;
	  }
	}
      }

      double w_comp = min(10.0,max(-10.0,Calculated->Vario/mc_safe));
      double vdiff = vthis/Vstar + w_comp*rho_cruise + rho_climb;

      if (vthis > SAFTEYSPEED*2) {
	vdiff = 1.0;
	// prevent funny numbers when starting mid-track
      }
      //      Calculated->Experimental = vdiff*100.0;

      vdiff *= Vref;

      if (t1<5) {
        Calculated->TaskSpeedInstantaneous = vdiff;
        // initialise
      } else {
        static int lastActiveWayPoint = 0;
	static double tsi_av = 0;
	static int n_av = 0;
        if ((ActiveWayPoint==lastActiveWayPoint)
	    && (Calculated->LegDistanceToGo>1000.0)
	    && (Calculated->LegDistanceCovered>1000.0)) {

          Calculated->TaskSpeedInstantaneous =
            LowPassFilter(Calculated->TaskSpeedInstantaneous, vdiff, 0.1);

          // update stats
          if(Basic->Time < LastTimeStats) {
            LastTimeStats = Basic->Time;
	    tsi_av = 0;
	    n_av = 0;
          } else if (n_av>=60) {
	    tsi_av/= n_av;
            flightstats.Task_Speed.
              least_squares_update(
                                   max(0,
                                       Basic->Time-Calculated->TaskStartTime)/3600.0,
                                   max(0,min(100.0,tsi_av)));
            LastTimeStats = Basic->Time;
	    tsi_av = 0;
	    n_av = 0;
          }
	  tsi_av += Calculated->TaskSpeedInstantaneous;
	  n_av ++;

        } else {

          Calculated->TaskSpeedInstantaneous =
            LowPassFilter(Calculated->TaskSpeedInstantaneous, vdiff, 0.5);

	  //	  Calculated->TaskSpeedInstantaneous = vdiff;
	  tsi_av = 0;
	  n_av = 0;
	}
        lastActiveWayPoint = ActiveWayPoint;
      }
    }
  }
 OnExit:
  UnlockTaskData();

}


static void CheckFinalGlideThroughTerrain(NMEA_INFO *Basic,
                                          DERIVED_INFO *Calculated,
                                          double LegToGo,
                                          double LegBearing) {

  // Final glide through terrain updates
  if (Calculated->FinalGlide) {

    double lat, lon;
    bool out_of_range;
    double distance_soarable =
      FinalGlideThroughTerrain(LegBearing,
                               Basic, Calculated,
                               &lat,
                               &lon,
                               LegToGo, &out_of_range, NULL);

    if ((!out_of_range)&&(distance_soarable< LegToGo)) {
      Calculated->TerrainWarningLatitude = lat;
      Calculated->TerrainWarningLongitude = lon;
    } else {
      Calculated->TerrainWarningLatitude = 0.0;
      Calculated->TerrainWarningLongitude = 0.0;
    }
  } else {
    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;
  }
}


void LDNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double LegToGo) {
  double height_above_leg = Calculated->NavAltitude+Calculated->EnergyHeight
    - FAIFinishHeight(Basic, Calculated, ActiveWayPoint);

  Calculated->LDNext = UpdateLD(Calculated->LDNext,
                                LegToGo,
                                height_above_leg,
                                0.5);
}

void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                    const double this_maccready)
{

  if (!ValidTaskPoint(ActiveWayPoint) ||
      ((ActiveWayPoint>0) && !ValidTaskPoint(ActiveWayPoint-1))) {

    Calculated->LegSpeed = 0;
    Calculated->LegDistanceToGo = 0;
    Calculated->LegDistanceCovered = 0;
    Calculated->LegTimeToGo = 0;

    if (!AATEnabled) {
      Calculated->AATTimeToGo = 0;
    }

    //    Calculated->TaskSpeed = 0;

    Calculated->TaskDistanceToGo = 0;
    Calculated->TaskDistanceCovered = 0;
    Calculated->TaskTimeToGo = 0;
    Calculated->TaskTimeToGoTurningNow = -1;

    Calculated->TaskAltitudeRequired = 0;
    Calculated->TaskAltitudeDifference = 0;
    Calculated->TaskAltitudeDifference0 = 0;

    Calculated->TerrainWarningLatitude = 0.0;
    Calculated->TerrainWarningLongitude = 0.0;

    Calculated->LDFinish = INVALID_GR;
    Calculated->GRFinish = INVALID_GR; // VENTA-ADDON
    Calculated->LDNext = INVALID_GR;

    Calculated->FinalGlide = 0;
    CheckFinalGlideThroughTerrain(Basic, Calculated,
                                  0.0, 0.0);

    // no task selected, so work things out at current heading

    GlidePolar::MacCreadyAltitude(this_maccready, 100.0,
                                  Basic->TrackBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  &(Calculated->BestCruiseTrack),
                                  &(Calculated->VMacCready),
                                  (Calculated->FinalGlide==1),
                                  NULL, 1.0e6, CRUISE_EFFICIENCY);

    return;
  }

  //  LockFlightData();
  LockTaskData();

  ///////////////////////////////////////////////
  // Calculate Task Distances
  // First calculate distances for this waypoint

  double LegCovered, LegToGo=0;
  double LegDistance, LegBearing=0;
  bool calc_turning_now;

  double w1lat;
  double w1lon;
  double w0lat;
  double w0lon;

  if (AATEnabled && (ActiveWayPoint>0) &&
      !TaskIsTemporary() && (ValidTaskPoint(ActiveWayPoint+1))) {
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

  if (AATEnabled && (ActiveWayPoint>0) && ValidTaskPoint(ActiveWayPoint+1)
      && Calculated->IsInSector && (this_maccready>0.1) &&
      !TaskIsTemporary()) {
    calc_turning_now = true;
  } else {
    calc_turning_now = false;
  }

  if ((ActiveWayPoint<1) || TaskIsTemporary()) {
    LegCovered = 0;
    if (!TaskIsTemporary()) { // RLD if task not started, exclude distance to start point
      LegToGo=0;
    }
   } else {
    if (AATEnabled) {
      // TODO accuracy: Get best range point to here...
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
      // JMW TODO accuracy: legcovered replace this with more accurate version
      // LegDistance -= StartRadius;
      LegCovered = max(0,LegCovered-StartRadius);
    }
  }

  Calculated->LegDistanceToGo = LegToGo;
  Calculated->LegDistanceCovered = LegCovered;
  Calculated->TaskDistanceCovered = LegCovered;

  if (Basic->Time > Calculated->LegStartTime) {
    if (flightstats.LegStartTime[ActiveWayPoint]<0) {
      flightstats.LegStartTime[ActiveWayPoint] = Basic->Time;
    }
    Calculated->LegSpeed = Calculated->LegDistanceCovered
      / (Basic->Time - Calculated->LegStartTime);
  }

  ///////////////////////////////////////////////////
  // Now add distances for start to previous waypoint

  if (!TaskIsTemporary()) {

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

  ///////////////////////////////////////////////////////////

  CheckTransitionFinalGlide(Basic, Calculated);

  // accumulators
  double TaskAltitudeRequired = 0;
  double TaskAltitudeRequired0 = 0;
  Calculated->TaskDistanceToGo = 0;
  Calculated->TaskTimeToGo = 0;
  Calculated->TaskTimeToGoTurningNow = 0;

  double LegTime0;

  // Calculate Final Glide To Finish

  int FinalWayPoint = getFinalWaypoint();

  double height_above_finish = Calculated->NavAltitude+
    Calculated->EnergyHeight-FAIFinishHeight(Basic, Calculated, -1);

  //////////////////
  // Now add it for remaining waypoints
  int task_index= FinalWayPoint;

  double StartBestCruiseTrack = 0;

  if (!TaskIsTemporary()) {
    while ((task_index>ActiveWayPoint) && (ValidTaskPoint(task_index))) {
      double this_LegTimeToGo;
      bool this_is_final = (task_index==FinalWayPoint)
	|| ForceFinalGlide;

      this_is_final = true; // JMW CHECK FGAMT

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

      double LegAltitude = GlidePolar::
	MacCreadyAltitude(this_maccready,
			  NextLegDistance, NextLegBearing,
			  Calculated->WindSpeed,
			  Calculated->WindBearing,
			  0, 0,
			  this_is_final,
			  &this_LegTimeToGo,
			  height_above_finish, CRUISE_EFFICIENCY);

      double LegAltitude0 = GlidePolar::
	MacCreadyAltitude(0,
			  NextLegDistance, NextLegBearing,
			  Calculated->WindSpeed,
			  Calculated->WindBearing,
			  0, 0,
			  true,
			  &LegTime0, 1.0e6, CRUISE_EFFICIENCY
			  );

      if (LegTime0>=0.9*ERROR_TIME) {
	// can't make it, so assume flying at current mc
	LegAltitude0 = LegAltitude;
      }

      TaskAltitudeRequired += LegAltitude;
      TaskAltitudeRequired0 += LegAltitude0;

      Calculated->TaskDistanceToGo += NextLegDistance;
      Calculated->TaskTimeToGo += this_LegTimeToGo;

      if (task_index==1) {
	StartBestCruiseTrack = NextLegBearing;
      }

      if (calc_turning_now) {
	if (task_index == ActiveWayPoint+1) {

	  double NextLegDistanceTurningNow, NextLegBearingTurningNow;
	  double this_LegTimeToGo_turningnow=0;

	  DistanceBearing(Basic->Latitude,
			  Basic->Longitude,
			  w1lat,
			  w1lon,
			  &NextLegDistanceTurningNow,
			  &NextLegBearingTurningNow);

	  GlidePolar::
	    MacCreadyAltitude(this_maccready,
			      NextLegDistanceTurningNow,
			      NextLegBearingTurningNow,
			      Calculated->WindSpeed,
			      Calculated->WindBearing,
			      0, 0,
			      this_is_final,
			      &this_LegTimeToGo_turningnow,
			      height_above_finish, CRUISE_EFFICIENCY);
	  Calculated->TaskTimeToGoTurningNow += this_LegTimeToGo_turningnow;
	} else {
	  Calculated->TaskTimeToGoTurningNow += this_LegTimeToGo;
	}
      }

      height_above_finish-= LegAltitude;

      task_index--;
    }
  }
  ////////////////


  /////// current waypoint, do this last!

  if (AATEnabled && !TaskIsTemporary()
      && (ActiveWayPoint>0) &&
      ValidTaskPoint(ActiveWayPoint+1) && Calculated->IsInSector) {
    if (Calculated->WaypointDistance<AATCloseDistance()*3.0) {
      LegBearing = AATCloseBearing(Basic, Calculated);
    }
  }

  // JMW TODO accuracy: use mc based on risk? no!
  double LegAltitude =
    GlidePolar::MacCreadyAltitude(this_maccready,
                                  LegToGo,
                                  LegBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  &(Calculated->BestCruiseTrack),
                                  &(Calculated->VMacCready),

				  // (Calculated->FinalGlide==1),
				  true,  // JMW CHECK FGAMT

                                  &(Calculated->LegTimeToGo),
                                  height_above_finish, CRUISE_EFFICIENCY);

  double LegAltitude0 =
    GlidePolar::MacCreadyAltitude(0,
                                  LegToGo,
                                  LegBearing,
                                  Calculated->WindSpeed,
                                  Calculated->WindBearing,
                                  0,
                                  0,
                                  true,
                                  &LegTime0, 1.0e6, CRUISE_EFFICIENCY
                                  );

  if (Calculated->IsInSector && (ActiveWayPoint==0) && !TaskIsTemporary()) {
    // set best cruise track to first leg bearing when in start sector
    Calculated->BestCruiseTrack = StartBestCruiseTrack;
  }

  // JMW TODO accuracy: Use safetymc where appropriate

  LDNext(Basic, Calculated, LegToGo);

  if (LegTime0>= 0.9*ERROR_TIME) {
    // can't make it, so assume flying at current mc
    LegAltitude0 = LegAltitude;
  }

  TaskAltitudeRequired += LegAltitude;
  TaskAltitudeRequired0 += LegAltitude0;
  Calculated->TaskDistanceToGo += LegToGo;
  Calculated->TaskTimeToGo += Calculated->LegTimeToGo;

  height_above_finish-= LegAltitude;

  ////////////////

  if (calc_turning_now) {
    Calculated->TaskTimeToGoTurningNow +=
      Basic->Time-Calculated->TaskStartTime;
  } else {
    Calculated->TaskTimeToGoTurningNow = -1;
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

  // VENTA6
  Calculated->NextAltitudeDifference0 = total_energy_height
    - Calculated->NextAltitudeRequired0;

  Calculated->LDFinish = UpdateLD(Calculated->LDFinish,
                                  Calculated->TaskDistanceToGo,
                                  total_energy_height-final_height,
                                  0.5);

  // VENTA-ADDON Classic geometric GR calculation without Total Energy
  /*
   * Paolo Ventafridda> adding a classic standard glide ratio
   * computation based on a geometric path with no total energy and
   * wind. This value is auto limited to a reasonable level which can
   * be useful during flight, currently 200. Over 200, you are no more
   * gliding to the final destination I am afraid, even on an ETA
   * . The infobox value has a decimal point if it is between 1 and
   * 99, otherwise it's a simple integer.
   */
  double GRsafecalc = Calculated->NavAltitude - final_height;
  if (GRsafecalc <=0) Calculated->GRFinish = INVALID_GR;
  else {
    Calculated->GRFinish = Calculated->TaskDistanceToGo / GRsafecalc;
    if ( Calculated->GRFinish >ALTERNATE_MAXVALIDGR || Calculated->GRFinish <0 ) Calculated->GRFinish = INVALID_GR;
    else
      if ( Calculated->GRFinish <1 ) Calculated->GRFinish = 1;
  }
  // END VENTA-ADDON

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
  static bool first_mc = true;

  if (Calculated->FinalGlide && ActiveIsFinalWaypoint()) {
    is_final_glide = true;
  } else {
    first_mc = true;
  }

  double av_thermal = -1;
  if (flightstats.ThermalAverage.y_ave>0) {
    if (Calculated->Circling && (Calculated->AverageThermal>0)) {
      av_thermal = (flightstats.ThermalAverage.y_ave
		*flightstats.ThermalAverage.sum_n
		+ Calculated->AverageThermal)/
	(flightstats.ThermalAverage.sum_n+1);
    } else {
      av_thermal = flightstats.ThermalAverage.y_ave;
    }
  } else if (Calculated->Circling && (Calculated->AverageThermal>0)) {
    // insufficient stats, so use this/last thermal's average
    av_thermal = Calculated->AverageThermal;
  }

  if (!ValidTaskPoint(ActiveWayPoint)) {
    if (av_thermal>0) {
      mc_new = av_thermal;
    }
  } else if ( ((AutoMcMode==0)||(AutoMcMode==2)) && is_final_glide) {

    double time_remaining = Basic->Time-Calculated->TaskStartTime-9000;
    if (EnableOLC
	&& (OLCRules==0)
	&& (Calculated->NavAltitude>Calculated->TaskStartAltitude)
	&& (time_remaining>0)) {

      mc_new = MacCreadyTimeLimit(Basic, Calculated,
				  Calculated->WaypointBearing,
				  time_remaining,
				  Calculated->TaskStartAltitude);

    } else if (Calculated->TaskAltitudeDifference0>0) {

      // only change if above final glide with zero Mc
      // otherwise when we are well below, it will wind Mc back to
      // zero

      double slope =
	(Calculated->NavAltitude + Calculated->EnergyHeight
	 - FAIFinishHeight(Basic, Calculated, ActiveWayPoint))/
	(Calculated->WaypointDistance+1);

      double mc_pirker = PirkerAnalysis(Basic, Calculated,
					Calculated->WaypointBearing,
					slope);
      mc_pirker = max(0.0, mc_pirker);
      if (first_mc) {
	// don't allow Mc to wind down to zero when first achieving
	// final glide; but do allow it to wind down after that
	if (mc_pirker >= mc_new) {
	  mc_new = mc_pirker;
	  first_mc = false;
	} else if (AutoMcMode==2) {
	  // revert to averager based auto Mc
	  if (av_thermal>0) {
	    mc_new = av_thermal;
	  }
	}
      } else {
	mc_new = mc_pirker;
      }
    } else { // below final glide at zero Mc, never achieved final glide
      if (first_mc && (AutoMcMode==2)) {
	// revert to averager based auto Mc
	if (av_thermal>0) {
	  mc_new = av_thermal;
	}
      }
    }
  } else if ( (AutoMcMode==1) || ((AutoMcMode==2)&& !is_final_glide) ) {
    if (av_thermal>0) {
      mc_new = av_thermal;
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
    // MJJ TODO Predict terrain altitude
    Calculated->NextAltitudeAGL = Calculated->NextAltitude - Calculated->TerrainAlt;

}


//////////////////////////////////////////////

bool GlobalClearAirspaceWarnings = false;

// JMW this code is deprecated
bool ClearAirspaceWarnings(const bool acknowledge, const bool ack_all_day) {
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
  double agl;
  double lat;
  double lon;

  if (position_is_predicted) {
    alt = Calculated->NextAltitude;
    agl = Calculated->NextAltitudeAGL;
    lat = Calculated->NextLatitude;
    lon = Calculated->NextLongitude;
  } else {
    if (Basic->BaroAltitudeAvailable) {
      alt = Basic->BaroAltitude;
    } else {
      alt = Basic->Altitude;
    }
    agl = Calculated->AltitudeAGL;
    lat = Basic->Latitude;
    lon = Basic->Longitude;
  }

  // JMW TODO enhancement: FindAirspaceCircle etc should sort results, return
  // the most critical or closest.

  if (AirspaceCircle) {
    for (i=0; i<NumberOfAirspaceCircles; i++) {

      if ((((AirspaceCircle[i].Base.Base != abAGL) && (alt >= AirspaceCircle[i].Base.Altitude))
           || ((AirspaceCircle[i].Base.Base == abAGL) && (agl >= AirspaceCircle[i].Base.AGL)))
          && (((AirspaceCircle[i].Top.Base != abAGL) && (alt < AirspaceCircle[i].Top.Altitude))
           || ((AirspaceCircle[i].Top.Base == abAGL) && (agl < AirspaceCircle[i].Top.AGL)))) {

        if ((MapWindow::iAirspaceMode[AirspaceCircle[i].Type] >= 2) &&
	    InsideAirspaceCircle(lon, lat, i)) {

          AirspaceWarnListAdd(Basic, Calculated, position_is_predicted, 1, i, false);
        }

      }

    }
  }

  // repeat process for areas

  if (AirspaceArea) {
    for (i=0; i<NumberOfAirspaceAreas; i++) {

      if ((((AirspaceArea[i].Base.Base != abAGL) && (alt >= AirspaceArea[i].Base.Altitude))
           || ((AirspaceArea[i].Base.Base == abAGL) && (agl >= AirspaceArea[i].Base.AGL)))
          && (((AirspaceArea[i].Top.Base != abAGL) && (alt < AirspaceArea[i].Top.Altitude))
           || ((AirspaceArea[i].Top.Base == abAGL) && (agl < AirspaceArea[i].Top.AGL)))) {

        if ((MapWindow::iAirspaceMode[AirspaceArea[i].Type] >= 2)
            && InsideAirspaceArea(lon, lat, i)){

          AirspaceWarnListAdd(Basic, Calculated, position_is_predicted, 0, i, false);
        }

      }
    }
  }

  AirspaceWarnListProcess(Basic, Calculated);

  //  UnlockFlightData();

}

//////////////////////////////////////////////

void AATStats_Time(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // Task time to go calculations

  double aat_tasktime_elapsed = Basic->Time - Calculated->TaskStartTime;
  double aat_tasklength_seconds = AATTaskLength*60;

  if (ActiveWayPoint==0) {
    if (Calculated->AATTimeToGo==0) {
      Calculated->AATTimeToGo = aat_tasklength_seconds;
    }
  } else if (aat_tasktime_elapsed>=0) {
    Calculated->AATTimeToGo = max(0,
				  aat_tasklength_seconds
				  - aat_tasktime_elapsed);
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

      double LegToGo=0, TargetLegToGo=0;

      if (i > 0 ) { //RLD only include distance from glider to next leg if we've started the task
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
          MaxDistance = LegToGo + (Task[i].AATCircleRadius );  // ToDo: should be adjusted for angle of max target and for national rules
          MinDistance = LegToGo - (Task[i].AATCircleRadius );
        }
        else
        {
          MaxDistance = LegToGo + (Task[i].AATSectorRadius );  // ToDo: should be adjusted for angle of max target.
          MinDistance = LegToGo;
        }

        TargetDistance = TargetLegToGo;
      }

      i++;
      while(ValidTaskPoint(i)) {
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

	MaxDistance += LegDistance;
	MinDistance += LegDistance;

	if(Task[ActiveWayPoint].AATType == CIRCLE) {
	  // breaking out single Areas increases accuracy for start
	  // and finish

	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance -= StartRadius; // e.g. Sports 2009 US Rules A116.3.2.  To Do: This should be configured multiple countries
	    MinDistance -= StartRadius;
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance -= FinishRadius; // To Do: This can be configured for finish rules
	    MinDistance -= FinishRadius;
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	    MinDistance -= (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	} else { // not circle (pie slice)
	  // sector at start of (i)th leg
	  if (i-1 == 0) {// first leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for start rules
	  } else { // not first leg of task
	    MaxDistance += (Task[i-1].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }

	  // sector at end of ith leg
	  if (!ValidTaskPoint(i+1)) {// last leg of task
	    // add nothing
	    MaxDistance += 0; // To Do: This can be configured for finish rules
	  } else { // not last leg of task
	    MaxDistance += (Task[i].AATCircleRadius);  //ToDo: should be adjusted for angle of max target
	  }
	}
	TargetDistance += TargetLegDistance;
	i++;
      }

      // JMW TODO accuracy: make these calculations more accurate, because
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

  // JMW TODO accuracy: Should really work out dt here,
  //           but i'm assuming constant time steps
  double dheight = Calculated->NavAltitude
    -SAFETYALTITUDEBREAKOFF
    -Calculated->TerrainBase; // JMW EXPERIMENTAL

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
     NULL);

  WayPointCalc[i].Distance = wDistance; // VENTA3
  WayPointCalc[i].Bearing  = wBearing; // VENTA3
  WayPointCalc[i].AltReqd  = AltReqd;  // VENTA3

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

      if ((scan_airports_slot==0) &&
	  ((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT)) {
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
                                       &out_of_range, NULL);

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
    if ((HomeWaypoint>=0) && (SortedLandableIndex[i] == HomeWaypoint)) {
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
  // TODO accuracy: make this more robust by making use of terrain height data
  // if available

  if ((time_on_ground<=10)||(ReplayLogger::IsEnabled())) {
    // Don't allow 'OnGround' calculations if in IGC replay mode
    Calculated->OnGround = FALSE;
  }

  if (!Calculated->Flying) {
    // detect takeoff
    if (time_in_flight>10) {
      Calculated->Flying = TRUE;
      WasFlying=true; // VENTA3
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
      // Do not reset QFE after landing.
      if (!WasFlying) QFEAltitudeOffset=GPS_INFO.Altitude; // VENTA3 Automatic QFE
    }
  } else {
    // detect landing
    if (time_in_flight==0) {
      // have been stationary for a minute
      InputEvents::processGlideComputer(GCE_LANDING);

      // JMWX  restore data calculated at finish so
      // user can review flight as at finish line

      // VENTA3 TODO maybe reset WasFlying to false, so that QFE is reset
      // though users can reset by hand anyway anytime..

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



void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // nothing yet.
}


int FindFlarmSlot(int flarmId)
{
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++)
    {
      if (GPS_INFO.FLARM_Traffic[z].ID == flarmId)
	{
	  return z;
	}
    }
  return -1;
}

int FindFlarmSlot(TCHAR *flarmCN)
{
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++)
    {
      if (_tcscmp(GPS_INFO.FLARM_Traffic[z].Name, flarmCN) == 0)
	{
	  return z;
	}
    }
  return -1;
}

bool IsFlarmTargetCNInRange()
{
  bool FlarmTargetContact = false;
  for(int z = 0; z < FLARM_MAX_TRAFFIC; z++)
    {
      if (GPS_INFO.FLARM_Traffic[z].ID != 0)
	{
	  if (GPS_INFO.FLARM_Traffic[z].ID == TeamFlarmIdTarget)
	    {
	      TeamFlarmIdTarget = GPS_INFO.FLARM_Traffic[z].ID;
	      FlarmTargetContact = true;
	      break;
	    }
	}
    }
  return FlarmTargetContact;
}


 int BallastSecsToEmpty = 120;

 void BallastDump ()
 {
   static double BallastTimeLast = -1;

   if (BallastTimerActive) {
         // JMW only update every 5 seconds to stop flooding the devices
     if (GPS_INFO.Time > BallastTimeLast+5) {
       double BALLAST_last = BALLAST;
       double dt = GPS_INFO.Time - BallastTimeLast;
       double percent_per_second = 1.0/max(10.0, BallastSecsToEmpty);
       BALLAST -= dt*percent_per_second;
       if (BALLAST<0) {
         BallastTimerActive = false;
         BALLAST = 0.0;
       }
       if (fabs(BALLAST-BALLAST_last)>0.05) { // JMW update on 5 percent!
         GlidePolar::SetBallast();
         devPutBallast(devA(), BALLAST);
         devPutBallast(devB(), BALLAST);
       }
       BallastTimeLast = GPS_INFO.Time;
     }
   } else {
     BallastTimeLast = GPS_INFO.Time;
   }
 }



/*
 * ===========================================
 * VENTA3 SearchBestAlternate() beta
 * based on SortLandableWaypoints and extended
 * by Paolo Ventafridda
 * ===========================================
 */
#ifdef DEBUG
#define DEBUG_BESTALTERNATE
#endif
#define MAXBEST 10      // max number of reachable landing points
			// searched for, among a preliminar list of
			// MAXBEST * 2 - CPU HOGGING ALERT!

void SearchBestAlternate(NMEA_INFO *Basic,
			 DERIVED_INFO *Calculated)
{
  int SortedLandableIndex[MAXBEST];
  double SortedArrivalAltitude[MAXBEST];
  int SortedApproxDistance[MAXBEST*2];
  int SortedApproxIndex[MAXBEST*2];
  int i, k, l;
  double arrival_altitude;
  int active_bestalternate_on_entry=-1;
  int bestalternate=-1;

#ifdef DEBUG_BESTALTERNATE
  TCHAR ventabuffer[200];
#endif

  if (!WayPointList) return;

  /*
   * VENTA3 search in range of optimum gliding capability
   * and in any case within an acceptable distance, say 100km.
   * Anything else is not considered, since we want a safe landing not a long glide.
   * Preferred waypoints and home are nevertheless checked in any case later.
   * Notice that if you are over 100km away from the nearest non-preferred landing point you can't
   * expect a computer to be helpful in case of troubles.
   *
   * ApproxDistance is in km, very approximate
   */

  double searchrange=(GPS_INFO.Altitude-SAFETYALTITUDEARRIVAL)* GlidePolar::bestld /1000;
  if (searchrange <= 0)
    searchrange=2; // lock to home airport at once
  if (searchrange > ALTERNATE_MAXRANGE)
    searchrange=ALTERNATE_MAXRANGE;

  LockTaskData();
  active_bestalternate_on_entry = BestAlternate;

  // Do preliminary fast search
  int scx_aircraft, scy_aircraft;
  LatLon2Flat(Basic->Longitude, Basic->Latitude, &scx_aircraft, &scy_aircraft);

  // Clear search lists
  for (i=0; i<MAXBEST*2; i++) {
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

    // Size a reasonable distance, wide enough VENTA3
    if ( approx_distance > searchrange ) continue;

    // see if this fits into slot
    for (k=0; k< MAXBEST*2; k++)  {

      if (((approx_distance < SortedApproxDistance[k])
           // wp is closer than this one
	   || (SortedApproxIndex[k]== -1))   // or this one isn't filled
          && (SortedApproxIndex[k]!= i))    // and not replacing with same
        {
	  // ok, got new biggest, put it into the slot.
          for (l=MAXBEST*2-1; l>k; l--) {
            if (l>0) {
	      SortedApproxDistance[l] = SortedApproxDistance[l-1];
	      SortedApproxIndex[l] = SortedApproxIndex[l-1];
            }
          }

          SortedApproxDistance[k] = approx_distance;
          SortedApproxIndex[k] = i;
          k=MAXBEST*2;
        }
    } // for k
  } // for i

#ifdef DEBUG_BESTALTERNATE
  FILE *fp;
  if ( (fp=_tfopen(_T("DEBUG.TXT"),_T("a"))) != NULL )  {
    _stprintf(ventabuffer,TEXT("==================\n"));
    fprintf(fp,"%S",ventabuffer);
    _stprintf(ventabuffer,TEXT("[GPSTIME=%02d:%02d:%02d] Altitude=%dm searchrange=%dKm Curr.Best=%d\n\n"),
	     GPS_INFO.Hour, GPS_INFO.Minute, GPS_INFO.Second,
	     (int)GPS_INFO.Altitude, (int)searchrange, BestAlternate);
    fprintf(fp,"%S",ventabuffer);
    for ( int dbug=0; dbug<MAXBEST*2; dbug++) {
      if ( SortedApproxIndex[dbug] <0 ) _stprintf(ventabuffer,_T("%d=empty\n"), dbug);
      else
        _stprintf(ventabuffer,TEXT("%d=%s(%d)\n"), dbug,
		 WayPointList[SortedApproxIndex[dbug]].Name, SortedApproxDistance[dbug] );
      fprintf(fp,"%S",ventabuffer);
    }
    fclose(fp);
  } else
    DoStatusMessage(_T("CANNOT OPEN DEBUG FILE"));
#endif


  // Now do detailed search
  for (i=0; i<MAXBEST; i++) {
    SortedLandableIndex[i]= -1;
    SortedArrivalAltitude[i] = 0;
  }

  bool found_reachable_airport = false;

  for (int scan_airports_slot=0;
       scan_airports_slot<2;
       scan_airports_slot++) {

    if (found_reachable_airport ) {
      continue; // don't bother filling the rest of the list
    }

    for (i=0; i<MAXBEST*2; i++) {
      if (SortedApproxIndex[i]<0) { // ignore invalid points
        continue;
      }

      if ((scan_airports_slot==0) &&
	  ((WayPointList[SortedApproxIndex[i]].Flags & AIRPORT) != AIRPORT)) {
        // we are in the first scan, looking for airports only
        continue;
      }

      arrival_altitude =
        CalculateWaypointArrivalAltitude(Basic,
                                         Calculated,
                                         SortedApproxIndex[i]);

      WayPointCalc[SortedApproxIndex[i]].AltArriv = arrival_altitude;
      // This is holding the real arrival value

      /*
       * We can't use degraded polar here, but we can't accept an
       * arrival 1m over safety.  That is 2m away from being
       * unreachable! So we higher this value to 100m.
       */
      arrival_altitude -= ALTERNATE_OVERSAFETY;

      if (scan_airports_slot==0) {
        if (arrival_altitude<0) {
          // in first scan, this airport is unreachable, so ignore it.
          continue;
        } else
          // this airport is reachable
          found_reachable_airport = true;
      }

      // see if this fits into slot
      for (k=0; k< MAXBEST; k++) {
        if (((arrival_altitude > SortedArrivalAltitude[k])
             // closer than this one
             ||(SortedLandableIndex[k]== -1))
            // or this one isn't filled
	    &&(SortedLandableIndex[k]!= i))  // and not replacing
	  // with same
          {
	    /*
	      #ifdef DEBUG_BESTALTERNATE
	      _stprintf(ventabuffer,TEXT("SAI[i=%d]=%s   SLI[k=%d]=n%d"), i, WayPointList[SortedApproxIndex[i]].Name,
	      k, SortedLandableIndex[k] );
	      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	        {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	      #endif
	    */
            double wp_distance, wp_bearing;
            DistanceBearing(Basic->Latitude , Basic->Longitude ,
                            WayPointList[SortedApproxIndex[i]].Latitude,
                            WayPointList[SortedApproxIndex[i]].Longitude,
                            &wp_distance, &wp_bearing);

	    WayPointCalc[SortedApproxIndex[i]].Distance = wp_distance;
	    WayPointCalc[SortedApproxIndex[i]].Bearing = wp_bearing;

            bool out_of_range;
            double distance_soarable =
              FinalGlideThroughTerrain(wp_bearing, Basic, Calculated,
                                       NULL,
                                       NULL,
                                       wp_distance,
                                       &out_of_range, NULL);

            if ((distance_soarable>= wp_distance)||(arrival_altitude<0)) {
              // only put this in the index if it is reachable
              // and doesn't go through terrain, OR, if it is unreachable
              // it doesn't matter if it goes through terrain because
              // pilot has to climb first anyway

              // ok, got new biggest, put it into the slot.
              for (l=MAXBEST-1; l>k; l--) {
                if (l>0) {
                  SortedArrivalAltitude[l] = SortedArrivalAltitude[l-1];
                  SortedLandableIndex[l] = SortedLandableIndex[l-1];
                }
              }

              SortedArrivalAltitude[k] = arrival_altitude;
              SortedLandableIndex[k] = SortedApproxIndex[i];
              k=MAXBEST;
            }
          } // if (((arrival_altitude > SortedArrivalAltitude[k]) ...
	/*
	  #ifdef DEBUG_BESTALTERNATE
	  else {
	  _stprintf(ventabuffer,TEXT("  SAI[i=%d]=%s   SLI[k=%d]=n%d"), i, WayPointList[SortedApproxIndex[i]].Name,
	  k, SortedLandableIndex[k] );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}

	  }
	  #endif
	*/
      } // for (k=0; k< MAXBEST; k++) {
    }
  }

  // extended part by Paolo

#ifdef DEBUG_BESTALTERNATE
  if ( (fp=_tfopen(_T("DEBUG.TXT"),_T("a"))) != NULL )  {
    _stprintf(ventabuffer,_T("\nLandable:\n"));
    fprintf(fp,"%S",ventabuffer);
    for ( int dbug=0; dbug<MAXBEST; dbug++) {
      if ( SortedLandableIndex[dbug] <0 ) {
	_stprintf(ventabuffer,_T("%d=empty\n"), dbug);
	fprintf(fp,"%S",ventabuffer);
      }
      else
	{
          _stprintf(ventabuffer,_T("%d=%s (%dm)"), dbug,
		   WayPointList[SortedLandableIndex[dbug]].Name, (int)SortedArrivalAltitude[dbug] );
	  fprintf(fp,"%S",ventabuffer);
	  if ( SortedLandableIndex[dbug] == HomeWaypoint )
	    _stprintf(ventabuffer,_T(":HOME") );
	  else
	    if ( WayPointCalc[SortedLandableIndex[dbug]].Preferred == TRUE )
	      _stprintf(ventabuffer,_T(":PREF") );
	    else
	      _stprintf(ventabuffer,_T("") );
	  fprintf(fp,"%S\n",ventabuffer);
	}

    }
    fclose(fp);
  } else
    DoStatusMessage(_T("CANNOT OPEN DEBUG FILE"));
#endif

  bestalternate=-1;  // reset the good choice
  double safecalc = Calculated->NavAltitude - SAFETYALTITUDEARRIVAL;
  static double grpolar = GlidePolar::bestld *SAFELD_FACTOR;
  int curwp, curbestairport=-1, curbestoutlanding=-1;
  double curgr=0, curbestgr=INVALID_GR;
  if ( safecalc <= 0 ) {
    /*
     * We're under the absolute safety altitude at MSL, can't be any better elsewhere!
     * Use the closer, hopefully you are landing on your airport
     */
#ifdef DEBUG_BESTALTERNATE
    _stprintf(ventabuffer,TEXT("Under safety at MSL, using closer"));
    if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
    // DoStatusMessage(ventabuffer);
#endif

  } else
    for (k=0;  k< MAXBEST; k++) {
      curwp = SortedLandableIndex[k];

      if ( !ValidWayPoint(curwp) ) {
	//#ifdef DEBUG_BESTALTERNATE
	//		_stprintf(ventabuffer,TEXT("k=%d skip invalid waypoint curwp=%d"), k, curwp );
	//		if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	//#endif
	continue;
	// break;  // that list is unsorted !
      }

      // At the first unsafe landing, stop searching down the list and use the best found or the first
      double grsafe=safecalc - WayPointList[curwp].Altitude;
      if ( grsafe <= 0 ) {
	/*
	 * We're under the safety altitude for this waypoint.
	 */
	/*
	  #ifdef DEBUG_BESTALTERNATE
	  _stprintf(ventabuffer,TEXT("N.%d=%s under safety."),k, WayPointList[curwp].Name);
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL){;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	  DoStatusMessage(ventabuffer);
	  #endif
	*/
	break;
	//continue;
      }

      WayPointCalc[curwp].GR = WayPointCalc[curwp].Distance / grsafe; grsafe = WayPointCalc[curwp].GR;
      curgr=WayPointCalc[curwp].GR;

      if ( grsafe > grpolar ) {
	/*
	 * Over degraded polar GR for this waypoint
	 */
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("k=%d %s grsafe=%d > grpolar=%d, skipping. "),
		 k, WayPointList[curwp].Name, (int)grsafe, (int)grpolar );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	continue;
	// break;
      }

      // Anything now is within degraded glide ratio, so if our homewaypoint is safely reachable then
      // attempt to lock it even if we already have a valid best, even if it is preferred and even
      // if it has a better GR

      if ( (HomeWaypoint >= 0) && (curwp == HomeWaypoint) ) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("k=%d locking Home"), k);
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	bestalternate = curwp;
	break;
      }

      // If we already found a preferred, stop searching for anything but home

      if ( bestalternate >= 0 && WayPointCalc[bestalternate].Preferred) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("Ignoring:[k=%d]%s because current best <%s> is a PREF"), k,
		 WayPointList[curwp].Name, WayPointList[bestalternate].Name);
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	continue;
      }

      // VENTA5 TODO: extend search on other preferred, choosing the closer one

      // Preferred list has priority, first found is taken (could be smarted)

      if ( WayPointCalc[ curwp ].Preferred ) {
	bestalternate=curwp;
#ifdef DEBUG_BESTALTERNATE
	_stprintf(ventabuffer,TEXT("k=%d PREFERRED bestalternate=%d,%s"), k,curwp,
		 WayPointList[curwp].Name );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	// DoStatusMessage(ventabuffer);
#endif
	continue;
      }

      // else we remember the best landable GR found so far. We shall use this in case
      // at the end of the search no home and no preferred were found.

      if ( curgr < curbestgr ) {
	if ( ( WayPointList[curwp].Flags & AIRPORT) == AIRPORT) {
	  curbestairport=curwp;
	  curbestgr=curgr; // ONLY FOR AIRPORT! NOT FOR OUTLANDINGS!!
#ifdef DEBUG_BESTALTERNATE
          _stprintf(ventabuffer,TEXT("[k=%d]<%s> (curgr=%d < curbestgr=%d) set as bestairport"),
		   k, WayPointList[curwp].Name, (int)curgr, (int)curbestgr );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	}
	else {
	  curbestoutlanding=curwp;
#ifdef DEBUG_BESTALTERNATE
          _stprintf(ventabuffer,TEXT("[k=%d]<%s> (curgr=%d < curbestgr=%d) set as bestoutlanding"),
		   k, WayPointList[curwp].Name, (int)curgr, (int)curbestgr );
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	}
      }

      continue;

    }

  if ( bestalternate <0 ) {

    if ( curbestairport >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("--> no bestalternate, choosing airport <%s> with gr=%d"),
	       WayPointList[curbestairport].Name, (int)curbestgr );
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
      // DoStatusMessage(ventabuffer);
#endif
      bestalternate=curbestairport;
    } else {
      if ( curbestoutlanding >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
        _stprintf(ventabuffer,TEXT("--> no bestalternate, choosing outlanding <%s> with gr=%d"),
		 WayPointList[curbestoutlanding].Name, (int)curbestgr );
	if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	// DoStatusMessage(ventabuffer);
#endif
	bestalternate=curbestoutlanding;
      } else {
	/*
	 * Here we are in troubles, nothing really reachable, but we
	 * might still be lucky to be within the "yellow" glide
	 * path. In any case we select the best arrival altitude place
	 * available, even if it is "red".
	 */
	if ( ValidWayPoint(SortedLandableIndex[0]) ) {
	  bestalternate=SortedLandableIndex[0];
#ifdef DEBUG_BESTALTERNATE
	  _stprintf(ventabuffer,TEXT("--> No bestalternate was found, and no good airport or outlanding!\n    Setting first available: <%s>"),
		   WayPointList[bestalternate].Name);
	  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
	  // DoStatusMessage(ventabuffer);
#endif
	} else {
	  /*
	   * Else the Landable list is EMPTY, although we might be
	   * near to a landable point but the terrain obstacles look
	   * too high (or the DEM resolution is not high enough to
	   * show a passage).
	   *
	   * Still the old BestAlternate could simply be out of range,
	   * but reachable...  These values have certainly just been
	   * calculated by DoAlternates() , so they are usable.
	   */
	  // Attempt to use the old best, but check there's one.. it
	  // might be empty for the first run
	  if ( ValidWayPoint(active_bestalternate_on_entry) )
	    {
	      bestalternate=active_bestalternate_on_entry;
	      if ( WayPointCalc[bestalternate].AltArriv <0 ) {
#ifdef DEBUG_BESTALTERNATE
		_stprintf(ventabuffer,TEXT("Landable list is empty and old bestalternate <%s> has Arrival=%d <0, NO good."),
			 WayPointList[bestalternate].Name, (int) WayPointCalc[bestalternate].AltArriv);
		if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		// Pick up the closest!
		if ( ValidWayPoint( SortedApproxIndex[0]) ) {
		  bestalternate=SortedApproxIndex[0];
#ifdef DEBUG_BESTALTERNATE
		  _stprintf(ventabuffer,
			   TEXT(".. using the closer point found: <%s> distance=~%d Km, you need to climb!"),
			   WayPointList[bestalternate].Name, SortedApproxDistance[0]);
		  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		} else {
		  /// CRITIC POINT
		  // Otherwise ..
		  // nothing, either keep the old best or set it empty
		  // Put here "PULL-UP! PULL-UP! Boeing cockpit voice sound and possibly shake the stick.
#ifdef DEBUG_BESTALTERNATE
		  _stprintf(ventabuffer,TEXT("Out of ideas..good luck"));
		  if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		    {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		}
	      } else
		{
		  // MapWindow2 is checking for reachables separately,
		  // se let's see if this closest is reachable
		  if ( ValidWayPoint( SortedApproxIndex[0] )) {
		    if ( WayPointList[SortedApproxIndex[0]].Reachable ) {
		      bestalternate = SortedApproxIndex[0];
#ifdef DEBUG_BESTALTERNATE
		      _stprintf(ventabuffer,TEXT("Closer point found: <%s> distance=~%d Km, Reachable with arrival at %d!"),
			       WayPointList[bestalternate].Name, SortedApproxDistance[0], (int) WayPointList[bestalternate].AltArivalAGL);
		      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		    } else
		      {
#ifdef DEBUG_BESTALTERNATE
			_stprintf(ventabuffer,TEXT("Closer point found: <%s> distance=~%d Km, UNReachable"),
				 WayPointList[bestalternate].Name, SortedApproxDistance[0]);
			if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			  {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		      }
		  } else
		    {
#ifdef DEBUG_BESTALTERNATE
		      _stprintf(ventabuffer,TEXT("Landable list is empty, no Closer Approx, but old best %s is still reachable (arrival=%d)"),
			       WayPointList[bestalternate].Name, (int)WayPointCalc[bestalternate].AltArriv);
		      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
			{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
		    }
		}
	    } else
	    {
	      /// CRITIC POINT
#ifdef DEBUG_BESTALTERNATE
	      _stprintf(ventabuffer,TEXT("Landable list is empty, and NO valid old bestalternate"));
	      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
		{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
	    }
	}
	/*
	 * Don't make any sound at low altitudes, pilot is either taking off
	 * or landing, or searching for an immediate outlanding.  Do not disturb.
	 * If safetyaltitude is 300m, then below 500m be quiet.
	 * If there was no active alternate on entry, and nothing was found, then we
	 * better be quiet since probably the user had already been alerted previously
	 * and now he is low..
	 */
	if ( bestalternate >0 &&
	     ((safecalc-WayPointList[bestalternate].Altitude) >ALTERNATE_QUIETMARGIN)) {
	  if ( WayPointList[bestalternate].AltArivalAGL <100 )
	    AlertBestAlternate(2);
	  //	if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_RED"));
	}
      }
    }
  }

  /*
   * If still invalid, it should mean we are just taking off
   * in this case no problems, we set the very first bestalternate of the day as the home
   * trusting the user to be home really!
   */

  if ( bestalternate < 0 ) {
    if ( HomeWaypoint >= 0 ) {
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("BESTALTERNATE HOME (%s)"),
	       WayPointList[HomeWaypoint].Name );
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
      //DoStatusMessage(ventabuffer);
#endif
      bestalternate=HomeWaypoint;
    }
  } else {
    // If still invalid, i.e. not -1, then there's a big problem
    if ( !ValidWayPoint(bestalternate) ) {
      //if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_RED"));
      AlertBestAlternate(2);
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("WARNING ERROR INVALID BEST=%d"),bestalternate);
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
      DoStatusMessage(_T("WARNING ERROR INVALID BEST!"));
      // todo: immediate disable function
    }
  }

  if (active_bestalternate_on_entry != bestalternate) {
    BestAlternate = bestalternate;
    if ( bestalternate >0 && ((safecalc-WayPointList[bestalternate].Altitude) >ALTERNATE_QUIETMARGIN))
      AlertBestAlternate(1);
    //		if (EnableSoundModes) PlayResource(TEXT("IDR_WAV_GREEN"));
  }

  UnlockTaskData();
}

/*
 * Do not disturb too much. Play alert sound only once every x minutes, not more.
 */
void AlertBestAlternate(short soundmode) {
#ifdef DEBUG_BESTALTERNATE
  TCHAR ventabuffer[100];
  FILE *fp;
#endif

  static double LastAlertTime=0;

  if ( GPS_INFO.Time > LastAlertTime + 180.0 ) {
    if (EnableSoundModes) {
      LastAlertTime = GPS_INFO.Time;
      switch (soundmode) {
      case 0:
	break;
      case 1:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_GREEN"));
#endif
	break;
      case 2:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_RED"));
#endif
	break;
      case 11:
#ifndef DISABLEAUDIO
	PlayResource(TEXT("IDR_WAV_GREEN"));
	PlayResource(TEXT("IDR_WAV_GREEN"));
#endif
	break;
      default:
	break;
      }
#ifdef DEBUG_BESTALTERNATE
      _stprintf(ventabuffer,TEXT("(PLAYED ALERT SOUND, soundmode=%d)"), soundmode);
      if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
	{;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
    }
  } else {
#ifdef DEBUG_BESTALTERNATE
    _stprintf(ventabuffer,TEXT("(QUIET, NO PLAY ALERT SOUND, soundmode=%d)"), soundmode);
    if ((fp=_tfopen(_T("DEBUG.TXT"),_T("a")))!= NULL)
      {;fprintf(fp,"%S\n",ventabuffer);fclose(fp);}
#endif
  }
}

