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

#include "CalculationsVertical.hpp"
#include "XCSoar.h"
#include "externs.h"
#include "Math/FastMath.h"
#include "WayPoint.hpp"
#include "Math/Earth.hpp"
#include "Math/LowPassFilter.hpp"
#include "McReady.h"

ldrotary_s rotaryLD;

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
