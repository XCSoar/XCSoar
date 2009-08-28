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
#include "SettingsComputer.hpp"
#include "SettingsTask.hpp"
#include "Settings.hpp"
#include "Device/device.h"
#include "McReady.h"
#include "Math/LowPassFilter.hpp"
#include "XCSoar.h"
#include "ReplayLogger.hpp"
#include <math.h>
#include "Calculations2.h"


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


void SpeedToFly(NMEA_INFO *Basic, DERIVED_INFO *Calculated, 
		const double mc_setting,
		const double cruise_efficiency) {
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
    risk_mc = mc_setting;
  } else {
    risk_mc =
      GlidePolar::MacCreadyRisk(Calculated->NavAltitude+Calculated->EnergyHeight
                                -SAFETYALTITUDEBREAKOFF-Calculated->TerrainBase,
                                Calculated->MaxThermalHeight,
                                mc_setting);
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
                                    NULL, 0, cruise_efficiency);
    } else {
      GlidePolar::MacCreadyAltitude(delta_mc,
                                    100.0, // dummy value
                                    Basic->TrackBearing,
                                    Calculated->WindSpeed,
                                    Calculated->WindBearing,
                                    0,
                                    &VOptnew,
                                    true,
                                    NULL, 1.0e6, cruise_efficiency);
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

// int          NettoSpeed = 1000;

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
  const double mc = GlidePolar::GetMacCready();
  const double ce = GlidePolar::GetCruiseEfficiency();

  NettoVario(Basic, Calculated);
  SpeedToFly(Basic, Calculated, mc, ce);
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


