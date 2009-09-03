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
#include "XCSoar.h"
#include "Protection.hpp"
#include "Interface.hpp"
#include "Units.hpp"
#include "Message.h"
#include "Blackboard.hpp"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Calibration.hpp"
#include "McReady.h"
#include "Logger.h"
#include "LocalTime.hpp"
#include "ReplayLogger.hpp"
#include "Math/FastMath.h"
#include <math.h>
#include "InputEvents.h"
#include "Message.h"
#include "TeamCodeCalculation.h"
#include <tchar.h>
#include "GlideComputer.hpp"
#include "Math/NavFunctions.hpp" // used for team code
#include "Calculations2.h"
#ifdef NEWCLIMBAV
#include "ClimbAverageCalculator.h" // JMW new
#endif
#include "Math/Geometry.hpp"
#include "Math/Earth.hpp"
#include "Math/Pressure.h"
#include "WayPoint.hpp"
#include "LogFile.hpp"
#include "BestAlternate.hpp"
#include "Persist.hpp"
#include "Airspace.h"
#include "CalculationsAirspace.hpp"
#include "CalculationsAbort.hpp"
#include "ConditionMonitor.hpp"

bool EnableNavBaroAltitude=false;
int EnableExternalTriggerCruise=false;
bool ForceFinalGlide= false;
bool AutoForceFinalGlide= false;
bool EnableFAIFinishHeight = false;

int FinishLine=1;
DWORD FinishRadius=1000;

bool WasFlying = false; // VENTA3 used by auto QFE: do not reset QFE
			//   if previously in flight. So you can check
			//   QFE on the ground, otherwise it turns to
			//   zero at once!
extern int FastLogNum; // number of points to log at high rate

static void
Heading(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void
EnergyHeightNavAltitude(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void
DistanceToHome(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);


static void
TakeoffLanding(const NMEA_INFO *Basic, DERIVED_INFO *Calculated);

#include "CalculationsClimb.hpp"
#include "CalculationsTask.hpp"
#include "CalculationsVertical.hpp"
#include "CalculationsTerrain.hpp"
#include "CalculationsWind.hpp"

/////////////////////////////////////////////////////////////////////////////////////


double
FAIFinishHeight(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated, int wp)
{
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

    Message::AddMessage(TEXT("Task Start"), TempAll);

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

void
DoCalculationsSlow(const NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                   double screen_distance)
{
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

  TerrainFootprint(Basic, Calculated, screen_distance);

  DoBestAlternateSlow(Basic, Calculated);

}


static void
ResetFlightStats(DERIVED_INFO *Calculated, bool full=true)
{
  int i;

  GlidePolar::SetCruiseEfficiency(1.0);

  if (full) {

    mutexGlideComputer.Lock();
    GlideComputer::olc.ResetFlight();
    GlideComputer::flightstats.Reset();
    GlideComputer::aatdistance.Reset();
    mutexGlideComputer.Unlock();

    GlidePolar::SetCruiseEfficiency(1.0);
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


bool
FlightTimes(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  static double LastTime = 0;

  if ((Basic->Time != 0) && (Basic->Time <= LastTime))
    // 20060519:sgi added (Basic->Time != 0) dueto alwas return here
    // if no GPS time available
    {

      if ((Basic->Time<LastTime) && (!Basic->NAVWarning)) {
	// Reset statistics.. (probably due to being in IGC replay mode)
        ResetFlightStats(Calculated);
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
  memset( Basic, 0, sizeof(GPS_INFO));
  memset( Calculated, 0,sizeof(CALCULATED_INFO));

  GPS_INFO.NAVWarning = true; // default, no gps at all!
  GPS_INFO.SwitchState.AirbrakeLocked = false;
  GPS_INFO.SwitchState.FlapPositive = false;
  GPS_INFO.SwitchState.FlapNeutral = false;
  GPS_INFO.SwitchState.FlapNegative = false;
  GPS_INFO.SwitchState.GearExtended = false;
  GPS_INFO.SwitchState.Acknowledge = false;
  GPS_INFO.SwitchState.Repeat = false;
  GPS_INFO.SwitchState.SpeedCommand = false;
  GPS_INFO.SwitchState.UserSwitchUp = false;
  GPS_INFO.SwitchState.UserSwitchMiddle = false;
  GPS_INFO.SwitchState.UserSwitchDown = false;
  GPS_INFO.SwitchState.VarioCircling = false;

  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);
  GPS_INFO.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  GPS_INFO.Year  = pda_time.wYear;
  GPS_INFO.Month = pda_time.wMonth;
  GPS_INFO.Day	 = pda_time.wDay;
  GPS_INFO.Hour  = pda_time.wHour;
  GPS_INFO.Minute = pda_time.wMinute;
  GPS_INFO.Second = pda_time.wSecond;

#ifdef _SIM_
  #if _SIM_STARTUPSPEED
  GPS_INFO.Speed = _SIM_STARTUPSPEED;
  #endif
  #if _SIM_STARTUPALTITUDE
  GPS_INFO.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif

  ///////////////

  CalibrationInit();
  ResetFlightStats(Calculated, true);
#ifndef FIVV
  LoadCalculationsPersist(Calculated); // VNT  not for fivv, confusing people
#endif
  DeleteCalculationsPersist();
  // required to allow fail-safe operation
  // if the persistent file is corrupt and causes a crash

  ResetFlightStats(Calculated, false);
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
  mutexGlideComputer.Lock();
  InitLDRotary(&GlideComputer::rotaryLD);
  mutexGlideComputer.Unlock();
}


BOOL
DoCalculations(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double mc = GlidePolar::GetMacCready();
  double ce = GlidePolar::GetCruiseEfficiency();

  Heading(Basic, Calculated);
  DistanceToNext(Basic, Calculated);
  DistanceToHome(Basic, Calculated);
  EnergyHeightNavAltitude(Basic, Calculated);
  TerrainHeight(Basic, Calculated);
  AltitudeRequired(Basic, Calculated, mc, ce);
  Vario(Basic,Calculated);

  if (TaskAborted) {
    SortLandableWaypoints(Basic, Calculated);
  }
  if (!targetManipEvent.test()) {
    // don't calculate these if optimise function being invoked or
    // target is being adjusted
    TaskStatistics(Basic, Calculated, mc, ce);
    AATStats(Basic, Calculated);
    TaskSpeed(Basic, Calculated, mc, ce);
  }

  if (!FlightTimes(Basic, Calculated)) {
    // time hasn't advanced, so don't do calculations requiring an advance
    // or movement
#if defined(_SIM_)
    return (EnableBestAlternate && ReplayLogger::IsEnabled());
    // VENTA3, needed for BestAlternate SIM
#else
    return FALSE;
#endif
  }

  Turning(Basic, Calculated);

  LD(Basic,Calculated);
  CruiseLD(Basic,Calculated);
  Calculated->AverageLD=CalculateLDRotary(Calculated, &GlideComputer::rotaryLD); // AverageLD
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
    GlideComputer::DoAutoMacCready(Basic, Calculated, mc);
    IterateEffectiveMacCready(Basic, Calculated);
  }

  // VENTA3 Alternates
  if ( EnableAlternate1 == true ) DoAlternates(Basic, Calculated,Alternate1);
  if ( EnableAlternate2 == true ) DoAlternates(Basic, Calculated,Alternate2);
  if ( EnableBestAlternate == true ) DoAlternates(Basic, Calculated,BestAlternate);

  GlideComputer::DoLogging(Basic, Calculated);
  GlideComputer::vegavoice.Update(Basic, Calculated);
  ConditionMonitorsUpdate(Basic, Calculated);

  return TRUE;
}


/////////////////////


static void
Heading(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
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

static void
DistanceToHome(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
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

static void
EnergyHeightNavAltitude(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
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


////////////////////////////////


static void
DoAutoQNH(const NMEA_INFO *Basic, const DERIVED_INFO *Calculated)
{
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

static void
TakeoffLanding(const NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
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
      ResetFlightStats(Calculated);

      Calculated->TakeOffTime= Basic->Time;

      // save stats in case we never finish
      memcpy(&GlideComputer::Finish_Derived_Info, Calculated, sizeof(DERIVED_INFO));

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
        memcpy(Calculated, &GlideComputer::Finish_Derived_Info, sizeof(DERIVED_INFO));
        Calculated->FlightTime = flighttime;
        Calculated->TakeOffTime = takeofftime;
      }
      Calculated->Flying = FALSE;
    }

  }
}



void
IterateEffectiveMacCready(const NMEA_INFO *Basic,
                          const DERIVED_INFO *Calculated)
{
  // nothing yet.
}


///////////////////////////////////////////////

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


void RefreshTaskStatistics(void) {
  const double mc = GlidePolar::GetMacCready();
  const double ce = GlidePolar::GetCruiseEfficiency();

  mutexGlideComputer.Lock();
  mutexFlightData.Lock();
  mutexTaskData.Lock();
  TaskStatistics(&GPS_INFO, &CALCULATED_INFO, mc, ce);
  AATStats(&GPS_INFO, &CALCULATED_INFO);
  TaskSpeed(&GPS_INFO, &CALCULATED_INFO, mc, ce);
  IterateEffectiveMacCready(&GPS_INFO, &CALCULATED_INFO);
  mutexTaskData.Unlock();
  mutexFlightData.Unlock();
  mutexGlideComputer.Unlock();
}


