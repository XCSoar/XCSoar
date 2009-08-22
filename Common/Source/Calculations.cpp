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

#include "Calculations.h"
#include "Defines.h" // VENTA3
#include "Dialogs.h"
#include "Process.h"
#include "Utils.h"
#include "Utils2.h"
#include "externs.h"
#include "McReady.h"
#include "Logger.h"
#include "Math/FastMath.h"
#include <math.h>
#include "InputEvents.h"
#include "Message.h"
#include "TeamCodeCalculation.h"
#include <tchar.h>
#include "Atmosphere.h"
#include "Audio/VegaVoice.h"
#include "OnLineContest.h"
#include "AATDistance.h"
#include "NavFunctions.h" // used for team code
#include "Calculations2.h"
#ifdef NEWCLIMBAV
#include "ClimbAverageCalculator.h" // JMW new
#endif
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Pressure.h"
#include "Math/LowPassFilter.hpp"
#include "WayPoint.hpp"
#include "LogFile.hpp"
#include "BestAlternate.hpp"
#include "Persist.hpp"
#include "GlideRatio.hpp"

OLCOptimizer olc;
AATDistance aatdistance;
DERIVED_INFO Finish_Derived_Info;
static VegaVoice vegavoice;

#include "ThermalLocator.h"
static ThermalLocator thermallocator;

bool EnableNavBaroAltitude=false;
int EnableExternalTriggerCruise=false;
bool ExternalTriggerCruise= false;
bool ExternalTriggerCircling= false;
bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;
bool EnableFAIFinishHeight = false;

int FinishLine=1;
DWORD FinishRadius=1000;

extern int FastLogNum; // number of points to log at high rate

#define THERMAL_TIME_MIN 45.0

double CRUISE_EFFICIENCY = 1.0;

extern void ConditionMonitorsUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

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
static void EnergyHeightNavAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

void DistanceToNext(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// task stuff
void TaskSpeed(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
	       const double this_maccready);
void AltitudeRequired(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
		      const double this_maccready);
void TaskStatistics(NMEA_INFO *Basic, DERIVED_INFO *Calculated, const double this_maccready);
void InSector(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void AATStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
//static void FinalGlideAlert(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsAutoMc.cpp
void DoAutoMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsAirspace.cpp
void PredictNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsTerrain.cpp
void TerrainHeight(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void TerrainFootprint(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void CheckFinalGlideThroughTerrain(NMEA_INFO *Basic,
				   DERIVED_INFO *Calculated,
				   double LegToGo,
				   double LegBearing);

// now in CalculationsWind.cpp
void InitialiseCalculationsWind();
void CloseCalculationsWind();
void DoWindZigZag(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoWindCirclingMode(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			bool left);
void DoWindCirclingSample(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoWindCirclingAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsAbort.cpp
void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsBallast.cpp
void BallastDump(NMEA_INFO *Basic);

/////////////////////////////////////////////////////////////////////////////////////


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

    DoWindZigZag(Basic, Calculated);

  } else {
    Calculated->Heading = Basic->TrackBearing;
  }

}


void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  // do slow part of calculations (cleanup of caches etc, nothing
  // that changes the state)

/*
   VENTA3-TODO: somewhere introduce BogusMips concept, in order to know what is the CPU speed
                of the local device, and fine-tune some parameters
 */

  static double lastTime = 0;
  if (Basic->Time<= lastTime) {
    lastTime = Basic->Time-6;
  } else {
    // calculate airspace warnings every 6 seconds
    AirspaceWarning(Basic, Calculated);
  }

  TerrainFootprint(Basic, Calculated);

  DoBestAlternateSlow(Basic, Calculated);

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




void CloseCalculations() {
  CloseCalculationsWind();
}



void InitCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  StartupStore(TEXT("InitCalculations\n"));
  memset( Calculated, 0,sizeof(CALCULATED_INFO));

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

  InitialiseCalculationsWind();
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

  BallastDump(Basic);

  if (!TaskIsTemporary()) {
    InSector(Basic, Calculated);
    DoAutoMacCready(Basic, Calculated);
    IterateEffectiveMacCready(Basic, Calculated);
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
  DoWindCirclingMode(Basic, Calculated, left);
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
    DoWindCirclingSample(Basic, Calculated);

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
  DoWindCirclingAltitude(Basic, Calculated);

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


