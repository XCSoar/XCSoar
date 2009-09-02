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
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "Device/device.h"
#include "InputEvents.h"
#include "McReady.h"
#include "Math/LowPassFilter.hpp"
#include "Math/FastMath.h"
#include "Math/Geometry.hpp"
#include "XCSoar.h"
#include "ReplayLogger.hpp"
#include <math.h>
#include "Atmosphere.h"
#include "ThermalLocator.h"
#include "GlideComputer.hpp"
#include "Protection.hpp"

#define CRUISE 0
#define WAITCLIMB 1
#define CLIMB 2
#define WAITCRUISE 3

#define MinTurnRate  4
#define CruiseClimbSwitch 15
#define ClimbCruiseSwitch 10
#define THERMAL_TIME_MIN 45.0


static void ThermalBand(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoWindCirclingMode(NMEA_INFO *Basic, DERIVED_INFO *Calculated,
			bool left);
void DoWindCirclingSample(NMEA_INFO *Basic, DERIVED_INFO *Calculated);
void DoWindCirclingAltitude(NMEA_INFO *Basic, DERIVED_INFO *Calculated);


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
    forcecircling = triggerClimbEvent.test();
    forcecruise = !forcecircling;
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

        if (GlideComputer::flightstats.Altitude_Ceiling.sum_n>0) {
          // only update base if have already climbed, otherwise
          // we will catch the takeoff height as the base.

          GlideComputer::flightstats.Altitude_Base.
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

 	InitLDRotary(&GlideComputer::rotaryLD);

        GlideComputer::flightstats.Altitude_Ceiling.
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
      GlideComputer::thermallocator.AddPoint(Basic->Time, Basic->Longitude, Basic->Latitude,
			      Calculated->NettoVario);
      GlideComputer::thermallocator.Update(Basic->Time, Basic->Longitude, Basic->Latitude,
			    Calculated->WindSpeed, Calculated->WindBearing,
			    Basic->TrackBearing,
			    &Calculated->ThermalEstimate_Longitude,
			    &Calculated->ThermalEstimate_Latitude,
			    &Calculated->ThermalEstimate_W,
			    &Calculated->ThermalEstimate_R);
    } else {
      Calculated->ThermalEstimate_W = 0;
      Calculated->ThermalEstimate_R = -1;
      GlideComputer::thermallocator.Reset();
    }
  }

  // update atmospheric model
  CuSonde::updateMeasurements(Basic, Calculated);

}


static void ThermalSources(NMEA_INFO *Basic, DERIVED_INFO *Calculated) {
  double ground_longitude;
  double ground_latitude;
  double ground_altitude;

  mutexGlideComputer.Lock();
  GlideComputer::thermallocator.
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
  mutexGlideComputer.Unlock();

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


void LastThermalStats(NMEA_INFO *Basic, DERIVED_INFO *Calculated)
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

	      mutexGlideComputer.Lock();
              GlideComputer::flightstats.ThermalAverage.
                least_squares_update(Calculated->LastThermalAverage);

#ifdef DEBUG_STATS
              DebugStore("%f %f # thermal stats\n",
                      GlideComputer::flightstats.ThermalAverage.m,
                      GlideComputer::flightstats.ThermalAverage.b
                      );
#endif
	      mutexGlideComputer.Unlock();
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


