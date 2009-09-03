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

#include "GlideComputer.hpp"
#include "McReady.h"
#include "WindZigZag.h"
#include "windanalyser.h"
#include <math.h>
#include "ReplayLogger.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "Units.hpp"
#include "SettingsComputer.hpp"
#include "Math/LowPassFilter.hpp"
#include "Math/Earth.hpp"
#include "Math/Geometry.hpp"
#include "Math/FastMath.h"
#include "RasterTerrain.h"
#include "Interface.hpp"
#include "Calibration.hpp"

GlideComputerAirData::GlideComputerAirData()
{
  InitLDRotary(&rotaryLD);

  //JMW TODO enhancement: seed initial wind store with start conditions
  // SetWindEstimate(calculated_info.WindSpeed, calculated_info.WindBearing, 1);
}

void GlideComputerAirData::ResetFlight(const bool full) 
{
  GlidePolar::SetCruiseEfficiency(1.0);
}

void GlideComputerAirData::Initialise() 
{
  CalibrationInit();
}


void GlideComputerAirData::ProcessBasic() {
  Heading();
  EnergyHeightNavAltitude();
  TerrainHeight();
  Vario();
}


void GlideComputerAirData::ProcessVertical() {
  LD();
  CruiseLD();
  calculated_info.AverageLD=CalculateLDRotary(&calculated_info, &rotaryLD); // AverageLD
  Average30s();

  AverageClimbRate();
  AverageThermal();
  ThermalGain();
}


#define D_AUTOWIND_CIRCLING 1
#define D_AUTOWIND_ZIGZAG 2

int AutoWindMode= D_AUTOWIND_CIRCLING;
// 0: Manual
// 1: Circling
// 2: ZigZag
// 3: Both

void GlideComputerAirData::DoWindZigZag() {
  // update zigzag wind
  if (((AutoWindMode & D_AUTOWIND_ZIGZAG)==D_AUTOWIND_ZIGZAG)
      && (!ReplayLogger::IsEnabled())) {
    double zz_wind_speed;
    double zz_wind_bearing;
    int quality;
    quality = WindZigZagUpdate(&gps_info, &calculated_info,
			       &zz_wind_speed,
			       &zz_wind_bearing);
    if (quality>0) {
      SetWindEstimate(zz_wind_speed, zz_wind_bearing, quality);
      Vector v_wind;
      v_wind.x = zz_wind_speed*cos(zz_wind_bearing*DEG_TO_RAD);
      v_wind.y = zz_wind_speed*sin(zz_wind_bearing*DEG_TO_RAD);

      windanalyser.slot_newEstimate(&gps_info, 
				    &calculated_info, v_wind, quality);
    }
  }
}


void GlideComputerAirData::DoWindCirclingMode(const bool left) {
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    windanalyser.slot_newFlightMode(&gps_info, 
				    &calculated_info,
				    left, 0);
  }
}


void GlideComputerAirData::DoWindCirclingSample() {
  if ((AutoWindMode & D_AUTOWIND_CIRCLING)==D_AUTOWIND_CIRCLING) {
    windanalyser.slot_newSample(&gps_info, 
				&calculated_info);
  }
}


void GlideComputerAirData::DoWindCirclingAltitude() {
  if (AutoWindMode>0) {
    windanalyser.slot_Altitude(&gps_info, 
			       &calculated_info);
    mutexGlideComputer.Unlock();
  }
}


void GlideComputerAirData::SetWindEstimate(const double wind_speed,
					    const double wind_bearing,
					    const int quality) {
  Vector v_wind;
  v_wind.x = wind_speed*cos(wind_bearing*3.1415926/180.0);
  v_wind.y = wind_speed*sin(wind_bearing*3.1415926/180.0);
  mutexGlideComputer.Lock();
  windanalyser.slot_newEstimate(&gps_info, 
				&calculated_info,
				v_wind, quality);
}


///////////


void GlideComputerAirData::AverageClimbRate()
{
  if (gps_info.AirspeedAvailable && gps_info.VarioAvailable
      && (!calculated_info.Circling)) {

    int vi = iround(gps_info.IndicatedAirspeed);

    if ((vi<=0) || (vi>= SAFTEYSPEED)) {
      // out of range
      return;
    }
    if (gps_info.AccelerationAvailable) {
      if (fabs(fabs(gps_info.Gload)-1.0)>0.25) {
        // G factor too high
        return;
      }
    }
    if (gps_info.TrueAirspeed>0) {
      // TODO: Check this is correct for TAS/IAS
      double ias_to_tas = gps_info.IndicatedAirspeed/gps_info.TrueAirspeed;
      double w_tas = gps_info.Vario*ias_to_tas;

      calculated_info.AverageClimbRate[vi]+= w_tas;
      calculated_info.AverageClimbRateN[vi]++;
    }
  }
}


#ifdef NEWCLIMBAV
ClimbAverageCalculator climbAverageCalculator;
void GlideComputerAirData::Average30s()
{
  calculated_info.Average30s = climbAverageCalculator.GetAverage(gps_info.Time, 
								 gps_info.Altitude, 30);
  calculated_info.NettoAverage30s = calculated_info.Average30s;
}

#endif

void GlideComputerAirData::Average30s()
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

  if(gps_info.Time > LastTime)
    {

      if (calculated_info.Circling != lastCircling) {
        num_samples = 0;
        // reset!
      }
      lastCircling = calculated_info.Circling;

      Elapsed = (int)(gps_info.Time - LastTime);
      for(i=0;i<Elapsed;i++)
        {
          index = (long)LastTime + i;
          index %= 30;

          Altitude[index] = calculated_info.NavAltitude;
	  if (gps_info.NettoVarioAvailable) {
	    NettoVario[index] = gps_info.NettoVario;
	  } else {
	    NettoVario[index] = calculated_info.NettoVario;
	  }
	  if (gps_info.VarioAvailable) {
	    Vario[index] = gps_info.Vario;
	  } else {
	    Vario[index] = calculated_info.Vario;
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

      if (!gps_info.VarioAvailable) {
        index = ((long)gps_info.Time - 1)%30;
        Gain = Altitude[index];

        index = ((long)gps_info.Time)%30;
        Gain = Gain - Altitude[index];

        Vave = Gain/30;
      }
      calculated_info.Average30s =
        LowPassFilter(calculated_info.Average30s,Vave,0.8);
      calculated_info.NettoAverage30s =
        LowPassFilter(calculated_info.NettoAverage30s,NVave,0.8);

#ifdef DEBUGAVERAGER
      if (calculated_info.Flying) {
        DebugStore("%d %g %g %g # averager\r\n",
                num_samples,
                calculated_info.Vario,
                calculated_info.Average30s, calculated_info.NettoAverage30s);
      }
#endif

    }
  else
    {
      if (gps_info.Time<LastTime) {
	// gone back in time
	for (i=0; i<30; i++) {
	  Altitude[i]= 0;
	  Vario[i]=0;
	  NettoVario[i]=0;
	}
      }
    }
  LastTime = gps_info.Time;
}

void GlideComputerAirData::AverageThermal()
{
  if (calculated_info.ClimbStartTime>=0) {
    if(gps_info.Time > calculated_info.ClimbStartTime)
      {
        double Gain =
          calculated_info.NavAltitude+calculated_info.EnergyHeight
            - calculated_info.ClimbStartAlt;
        calculated_info.AverageThermal  =
          Gain / (gps_info.Time - calculated_info.ClimbStartTime);
      }
  }
}

void GlideComputerAirData::MaxHeightGain()
{
  if (!calculated_info.Flying) return;

  if (calculated_info.MinAltitude>0) {
    double height_gain = calculated_info.NavAltitude - calculated_info.MinAltitude;
    calculated_info.MaxHeightGain = max(height_gain, calculated_info.MaxHeightGain);
  } else {
    calculated_info.MinAltitude = calculated_info.NavAltitude;
  }
  calculated_info.MinAltitude = min(calculated_info.NavAltitude, calculated_info.MinAltitude);
}


void GlideComputerAirData::ThermalGain()
{
  if (calculated_info.ClimbStartTime>=0) {
    if(gps_info.Time >= calculated_info.ClimbStartTime)
      {
        calculated_info.ThermalGain =
          calculated_info.NavAltitude + calculated_info.EnergyHeight
          - calculated_info.ClimbStartAlt;
      }
  }
}



void GlideComputerAirData::LD()
{
  static double LastLat = 0;
  static double LastLon = 0;
  static double LastTime = 0;
  static double LastAlt = 0;

  if (gps_info.Time<LastTime) {
    LastTime = gps_info.Time;
    calculated_info.LDvario = INVALID_GR;
    calculated_info.LD = INVALID_GR;
  }
  if(gps_info.Time >= LastTime+1.0)
    {
      double DistanceFlown;
      DistanceBearing(gps_info.Latitude, gps_info.Longitude,
                      LastLat, LastLon,
                      &DistanceFlown, NULL);

      calculated_info.LD = UpdateLD(calculated_info.LD,
                                DistanceFlown,
                                LastAlt - calculated_info.NavAltitude, 0.1);

      InsertLDRotary(&calculated_info, 
		     &rotaryLD,(int)DistanceFlown, (int)calculated_info.NavAltitude);

      LastLat = gps_info.Latitude;
      LastLon = gps_info.Longitude;
      LastAlt = calculated_info.NavAltitude;
      LastTime = gps_info.Time;
    }

  // LD instantaneous from vario, updated every reading..
  if (gps_info.VarioAvailable && gps_info.AirspeedAvailable
      && calculated_info.Flying) {
    calculated_info.LDvario = UpdateLD(calculated_info.LDvario,
                                   gps_info.IndicatedAirspeed,
                                   -gps_info.Vario,
                                   0.3);
  } else {
    calculated_info.LDvario = INVALID_GR;
  }
}


void GlideComputerAirData::CruiseLD()
{
  if(!calculated_info.Circling)
    {
      double DistanceFlown;

      if (calculated_info.CruiseStartTime<0) {
        calculated_info.CruiseStartLat = gps_info.Latitude;
        calculated_info.CruiseStartLong = gps_info.Longitude;
        calculated_info.CruiseStartAlt = calculated_info.NavAltitude;
        calculated_info.CruiseStartTime = gps_info.Time;
      } else {

        DistanceBearing(gps_info.Latitude, gps_info.Longitude,
                        calculated_info.CruiseStartLat,
                        calculated_info.CruiseStartLong, &DistanceFlown, NULL);
        calculated_info.CruiseLD =
          UpdateLD(calculated_info.CruiseLD,
                   DistanceFlown,
                   calculated_info.CruiseStartAlt - calculated_info.NavAltitude,
                   0.5);
      }
    }
}

//////


void GlideComputerAirData::Heading()
{
  double x0, y0, mag;
  static double LastTime = 0;
  static double lastHeading = 0;

  if ((gps_info.Speed>0)||(calculated_info.WindSpeed>0)) {

    x0 = fastsine(gps_info.TrackBearing)*gps_info.Speed;
    y0 = fastcosine(gps_info.TrackBearing)*gps_info.Speed;
    x0 += fastsine(calculated_info.WindBearing)*calculated_info.WindSpeed;
    y0 += fastcosine(calculated_info.WindBearing)*calculated_info.WindSpeed;

    calculated_info.Heading = AngleLimit360(atan2(x0,y0)*RAD_TO_DEG);

    if (!calculated_info.Flying) {
      // don't take wind into account when on ground
      calculated_info.Heading = gps_info.TrackBearing;
    }

    // calculate turn rate in wind coordinates
    if(gps_info.Time > LastTime) {
      double dT = gps_info.Time - LastTime;

      calculated_info.TurnRateWind = AngleLimit180(calculated_info.Heading
                                               - lastHeading)/dT;

      lastHeading = calculated_info.Heading;
    }
    LastTime = gps_info.Time;

    // calculate estimated true airspeed
    mag = isqrt4((unsigned long)(x0*x0*100+y0*y0*100))/10.0;
    calculated_info.TrueAirspeedEstimated = mag;

    // estimate bank angle (assuming balanced turn)
    double angle = atan(DEG_TO_RAD*calculated_info.TurnRateWind*
			calculated_info.TrueAirspeedEstimated/9.81);

    calculated_info.BankAngle = RAD_TO_DEG*angle;
    calculated_info.Gload = 1.0/max(0.001,fabs(cos(angle)));

    // estimate pitch angle (assuming balanced turn)
    calculated_info.PitchAngle = RAD_TO_DEG*
      atan2(calculated_info.GPSVario-calculated_info.Vario,
           calculated_info.TrueAirspeedEstimated);

    DoWindZigZag();

  } else {
    calculated_info.Heading = gps_info.TrackBearing;
  }
}


void GlideComputerAirData::EnergyHeightNavAltitude()
{
  // Determine which altitude to use for nav functions
  if (EnableNavBaroAltitude && gps_info.BaroAltitudeAvailable) {
    calculated_info.NavAltitude = gps_info.BaroAltitude;
  } else {
    calculated_info.NavAltitude = gps_info.Altitude;
  }

  double ias_to_tas;
  double V_tas;

  if (gps_info.AirspeedAvailable && (gps_info.IndicatedAirspeed>0)) {
    ias_to_tas = gps_info.TrueAirspeed/gps_info.IndicatedAirspeed;
    V_tas = gps_info.TrueAirspeed;
  } else {
    ias_to_tas = 1.0;
    V_tas = calculated_info.TrueAirspeedEstimated;
  }
  double V_bestld_tas = GlidePolar::Vbestld*ias_to_tas;
  double V_mc_tas = calculated_info.VMacCready*ias_to_tas;
  V_tas = max(V_tas, V_bestld_tas);
  double V_target = max(V_bestld_tas, V_mc_tas);
  calculated_info.EnergyHeight =
    (V_tas*V_tas-V_target*V_target)/(9.81*2.0);
}


void GlideComputerAirData::TerrainHeight()
{
  short Alt = 0;

  terrain.Lock();
  // want most accurate rounding here
  terrain.SetTerrainRounding(0,0);
  Alt = terrain.GetTerrainHeight(gps_info.Latitude,
				 gps_info.Longitude);
  terrain.Unlock();

  if(Alt<0) {
    Alt = 0;
    if (Alt <= TERRAIN_INVALID) {
      calculated_info.TerrainValid = false;
    } else {
      calculated_info.TerrainValid = true;
    }
    calculated_info.TerrainAlt = 0;
  } else {
    calculated_info.TerrainValid = true;
    calculated_info.TerrainAlt = Alt;
  }
  calculated_info.AltitudeAGL = calculated_info.NavAltitude - calculated_info.TerrainAlt;

  if (!FinalGlideTerrain) {
    calculated_info.TerrainBase = calculated_info.TerrainAlt;
  }
}


void GlideComputerAirData::Vario()
{
  static double LastTime = 0;
  static double LastAlt = 0;
  static double LastAltTE = 0;
  static double h0last = 0;

  if(gps_info.Time <= LastTime) {
    LastTime = gps_info.Time;
  } else {
    double Gain = calculated_info.NavAltitude - LastAlt;
    double GainTE = (calculated_info.EnergyHeight+gps_info.Altitude) - LastAltTE;
    double dT = (gps_info.Time - LastTime);
    // estimate value from GPS
    calculated_info.GPSVario = Gain / dT;
    calculated_info.GPSVarioTE = GainTE / dT;

    double dv = (calculated_info.TaskAltitudeDifference-h0last)
      /(gps_info.Time-LastTime);
    calculated_info.DistanceVario = LowPassFilter(calculated_info.DistanceVario,
                                              dv, 0.1);

    h0last = calculated_info.TaskAltitudeDifference;

    LastAlt = calculated_info.NavAltitude;
    LastAltTE = calculated_info.EnergyHeight+gps_info.Altitude;
    LastTime = gps_info.Time;

  }

  if (!gps_info.VarioAvailable || ReplayLogger::IsEnabled()) {
    calculated_info.Vario = calculated_info.GPSVario;

  } else {
    // get value from instrument
    calculated_info.Vario = gps_info.Vario;
    // we don't bother with sound here as it is polled at a
    // faster rate in the DoVarioCalcs methods

    CalibrationUpdate(&gps_info, &calculated_info);
  }
}
