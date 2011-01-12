/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2010 The XCSoar Project
  A detailed list of copyright holders can be found in the file "AUTHORS".

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

#include "GlideComputerBlackboard.hpp"
#include "Protection.hpp"
#include "SettingsComputer.hpp"
#include "NMEA/Info.hpp"
#include "NMEA/Derived.hpp"
#include "GlideRatio.hpp"
#include "Defines.h"

GlideComputerBlackboard::GlideComputerBlackboard(ProtectedTaskManager &task):
  m_task(task)
{
}

/**
 * Initializes the GlideComputerBlackboard
 */
void GlideComputerBlackboard::Initialise()
{
}

/**
 * Resets the GlideComputerBlackboard
 * @param full Reset all data?
 */
void
GlideComputerBlackboard::ResetFlight(const bool full)
{
  unsigned i;

  /*
    \todo also need to call flight_state_reset() on Basic() ?

    calculated_info.Flying = false;
    if (full) {
      calculated_info.FlightTime = 0;
      calculated_info.TakeOffTime = 0;
    }
  */

  if (full) {
    gps_info.Time = fixed_zero;
    gps_info.BaroAltitudeAvailable = false;
    gps_info.BaroAltitudeOrigin = NMEA_INFO::BARO_ALTITUDE_UNKNOWN;

    calculated_info.timeCruising = fixed_zero;
    calculated_info.timeCircling = fixed_zero;
    calculated_info.TotalHeightClimb = fixed_zero;

    calculated_info.CruiseStartTime = fixed_minus_one;
    calculated_info.ClimbStartTime = fixed_minus_one;

    calculated_info.CruiseLD = fixed(INVALID_GR);
    calculated_info.AverageLD = fixed(INVALID_GR);
    calculated_info.LD = fixed(INVALID_GR);
    calculated_info.LDvario = fixed(INVALID_GR);
    calculated_info.ThermalAverage = fixed_zero;

    for (i = 0; i < 200; i++) {
      calculated_info.AverageClimbRate[i] = fixed_zero;
      calculated_info.AverageClimbRateN[i] = 0;
    }

    calculated_info.MinAltitude = fixed_zero;
    calculated_info.MaxHeightGain = fixed_zero;
  }

  calculated_info.thermal_band.clear();

  // clear thermal sources for first time.
  for (i = 0; i < MAX_THERMAL_SOURCES; i++) {
    calculated_info.ThermalSources[i].LiftRate = fixed_minus_one;
  }

  calculated_info.Circling = false;
  for (int i = 0; i <= TERRAIN_ALT_INFO::NUMTERRAINSWEEPS; i++) {
    calculated_info.GlideFootPrint[i].Longitude = Angle::native(fixed_zero);
    calculated_info.GlideFootPrint[i].Latitude = Angle::native(fixed_zero);
  }
  calculated_info.TerrainWarning = false;

  // If you load persistent values, you need at least these reset:
  calculated_info.LastThermalAverage = fixed_zero;
  calculated_info.LastThermalAverageSmooth = fixed_zero;
  calculated_info.ThermalGain = fixed_zero;
}

/**
 * Starts the task on the GlideComputerBlackboard
 */
void
GlideComputerBlackboard::StartTask()
{
  calculated_info.CruiseStartLocation = gps_info.Location;
  calculated_info.CruiseStartAlt = gps_info.NavAltitude;
  calculated_info.CruiseStartTime = gps_info.Time;

  // JMW reset time cruising/time circling stats on task start
  calculated_info.timeCircling = fixed_zero;
  calculated_info.timeCruising = fixed_zero;
  calculated_info.TotalHeightClimb = fixed_zero;

  // reset max height gain stuff on task start
  calculated_info.MaxHeightGain = fixed_zero;
  calculated_info.MinAltitude = fixed_zero;
}

void
GlideComputerBlackboard::SaveFinish()
{
  // JMW save calculated data at finish
  Finish_Derived_Info = calculated_info;
}

void
GlideComputerBlackboard::RestoreFinish()
{
  FLYING_STATE flying_state = Basic().flight;

  calculated_info = Finish_Derived_Info;

  // \todo restore flying state
  //  SetBasic().flying_state = flying_state;
}

/**
 * Returns the average vertical speed in the current thermal
 * @return Average vertical speed in the current thermal
 */
fixed
GlideComputerBlackboard::GetAverageThermal() const
{
  return max(fixed_zero, calculated_info.ThermalAverage);
}

/**
 * Retrieves GPS data from the DeviceBlackboard
 * @param nmea_info New GPS data
 */
void
GlideComputerBlackboard::ReadBlackboard(const NMEA_INFO &nmea_info)
{
  _time_retreated = false;

  if (!positive(gps_info.Time) || nmea_info.Time < gps_info.Time) {
    // backwards in time, so reset last
    last_gps_info = nmea_info;
    last_calculated_info = calculated_info;
    _time_retreated = true;
  } else if (nmea_info.Time > gps_info.Time) {
    // forwards in time, so save state
    last_gps_info = gps_info;
    last_calculated_info = calculated_info;
  }

  gps_info = nmea_info;

  // if time hasn't advanced, don't copy last calculated
}

/**
 * Retrieves settings from the DeviceBlackboard
 * @param settings New settings
 */
void
GlideComputerBlackboard::ReadSettingsComputer(const SETTINGS_COMPUTER &settings)
{
  settings_computer = settings;
}
