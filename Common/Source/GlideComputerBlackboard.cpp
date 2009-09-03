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
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.h"
#include "NMEA/Derived.hpp"

#include "Persist.hpp"

void GlideComputerBlackboard::ResetFlight(const bool full) {
  unsigned i;
  if (full) {
    calculated_info.FlightTime = 0;
    calculated_info.TakeOffTime = 0;
    calculated_info.timeCruising = 0;
    calculated_info.timeCircling = 0;
    calculated_info.TotalHeightClimb = 0;

    calculated_info.CruiseStartTime = -1;
    calculated_info.ClimbStartTime = -1;

    calculated_info.LDFinish = INVALID_GR;
    calculated_info.GRFinish = INVALID_GR;  // VENTA-ADDON GR to final destination
    calculated_info.CruiseLD = INVALID_GR;
    calculated_info.AverageLD = INVALID_GR;
    calculated_info.LDNext = INVALID_GR;
    calculated_info.LD = INVALID_GR;
    calculated_info.LDvario = INVALID_GR;
    calculated_info.AverageThermal = 0;

    for (i=0; i<200; i++) {
      calculated_info.AverageClimbRate[i]= 0;
      calculated_info.AverageClimbRateN[i]= 0;
    }

    calculated_info.ValidFinish = false;
    calculated_info.ValidStart = false;
    calculated_info.TaskStartTime = 0;
    calculated_info.TaskStartSpeed = 0;
    calculated_info.TaskStartAltitude = 0;
    calculated_info.LegStartTime = 0;
    calculated_info.MinAltitude = 0;
    calculated_info.MaxHeightGain = 0;
  }

  calculated_info.MaxThermalHeight = 0;
  for (i=0; i<NUMTHERMALBUCKETS; i++) {
    calculated_info.ThermalProfileN[i]=0;
    calculated_info.ThermalProfileW[i]=0;
  }
  // clear thermal sources for first time.
  for (i=0; i<MAX_THERMAL_SOURCES; i++) {
    calculated_info.ThermalSources[i].LiftRate= -1.0;
  }

  calculated_info.Flying = false;
  calculated_info.Circling = false;
  calculated_info.FinalGlide = false;
  for (int i=0; i<=NUMTERRAINSWEEPS; i++) {
    calculated_info.GlideFootPrint[i].x = 0;
    calculated_info.GlideFootPrint[i].y = 0;
  }
  calculated_info.TerrainWarningLatitude = 0.0;
  calculated_info.TerrainWarningLongitude = 0.0;

  // If you load persistent values, you need at least these reset:
  calculated_info.WindBearing = 0.0; 
  calculated_info.LastThermalAverage=0.0;
  calculated_info.ThermalGain=0.0;
}


void GlideComputerBlackboard::StartTask() {
  calculated_info.ValidFinish = false;
  calculated_info.TaskStartTime = gps_info.Time ;
  calculated_info.TaskStartSpeed = gps_info.Speed;
  calculated_info.TaskStartAltitude = calculated_info.NavAltitude;
  calculated_info.LegStartTime = gps_info.Time;

  calculated_info.CruiseStartLat = gps_info.Latitude;
  calculated_info.CruiseStartLong = gps_info.Longitude;
  calculated_info.CruiseStartAlt = calculated_info.NavAltitude;
  calculated_info.CruiseStartTime = gps_info.Time;

  // JMW TODO accuracy: Get time from aatdistance module since this is
  // more accurate

  calculated_info.AverageThermal = 0; // VNT for some reason looked uninitialised
  calculated_info.WaypointBearing=0; // VNT TEST

  // JMW reset time cruising/time circling stats on task start
  calculated_info.timeCircling = 0;
  calculated_info.timeCruising = 0;
  calculated_info.TotalHeightClimb = 0;

  // reset max height gain stuff on task start
  calculated_info.MaxHeightGain = 0;
  calculated_info.MinAltitude = 0;
}


void GlideComputerBlackboard::SaveFinish()
{
  // JMW save calculated data at finish
  memcpy(&Finish_Derived_Info, &calculated_info, sizeof(DERIVED_INFO));
}

void GlideComputerBlackboard::Initialise()
{
  memset( &gps_info, 0, sizeof(NMEA_INFO));
  memset( &calculated_info, 0, sizeof(DERIVED_INFO));

  gps_info.NAVWarning = true; // default, no gps at all!
  gps_info.SwitchState.AirbrakeLocked = false;
  gps_info.SwitchState.FlapPositive = false;
  gps_info.SwitchState.FlapNeutral = false;
  gps_info.SwitchState.FlapNegative = false;
  gps_info.SwitchState.GearExtended = false;
  gps_info.SwitchState.Acknowledge = false;
  gps_info.SwitchState.Repeat = false;
  gps_info.SwitchState.SpeedCommand = false;
  gps_info.SwitchState.UserSwitchUp = false;
  gps_info.SwitchState.UserSwitchMiddle = false;
  gps_info.SwitchState.UserSwitchDown = false;
  gps_info.SwitchState.VarioCircling = false;

  SYSTEMTIME pda_time;
  GetSystemTime(&pda_time);
  gps_info.Time  = pda_time.wHour*3600+pda_time.wMinute*60+pda_time.wSecond;
  gps_info.Year  = pda_time.wYear;
  gps_info.Month = pda_time.wMonth;
  gps_info.Day	 = pda_time.wDay;
  gps_info.Hour  = pda_time.wHour;
  gps_info.Minute = pda_time.wMinute;
  gps_info.Second = pda_time.wSecond;

#ifdef _SIM_
  #if _SIM_STARTUPSPEED
  gps_info.Speed = _SIM_STARTUPSPEED;
  #endif
  #if _SIM_STARTUPALTITUDE
  gps_info.Altitude = _SIM_STARTUPALTITUDE;
  #endif
#endif
}


double GlideComputerBlackboard::GetAverageThermal() {
  return max(0.0,calculated_info.AverageThermal);
}
