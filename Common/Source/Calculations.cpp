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

extern void ConditionMonitorsUpdate(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

static void TakeoffLanding(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

#include "CalculationsClimb.hpp"
#include "CalculationsTask.hpp"
#include "CalculationsTerrain.hpp"

// now in CalculationsAirspace.cpp
void PredictNextPosition(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void AirspaceWarning(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsAbort.cpp
void SortLandableWaypoints(NMEA_INFO *Basic, DERIVED_INFO *Calculated);

// now in CalculationsBallast.cpp
void BallastDump(NMEA_INFO *Basic);

/////////////////////////////////////////////////////////////////////////////////////




void DoCalculationsSlow(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			double screen_distance) {
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


void ResetFlightStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
                      bool full=true) {
  glide_computer.ResetFlight(full);
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


void InitCalculations() {
  StartupStore(TEXT("InitCalculations\n"));
  ///////////////
  glide_computer.Initialise();
}


BOOL DoCalculations(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
{
  double mc = GlidePolar::GetMacCready();
  double ce = GlidePolar::GetCruiseEfficiency();

  // glide_computer.processGPS();
  glide_computer.ProcessGPS();

  if (TaskAborted) { // should be in slow!
    SortLandableWaypoints(Basic, Calculated);
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

  glide_computer.ProcessVertical();

  LastThermalStats(Basic, Calculated);
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

////////////////////////////////


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



void IterateEffectiveMacCready(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
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


