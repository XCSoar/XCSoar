/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2011 The XCSoar Project
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
  ScreenDistanceMeters(fixed(50000)),
  m_task(task)
{
}

/**
 * Resets the GlideComputerBlackboard
 * @param full Reset all data?
 */
void
GlideComputerBlackboard::ResetFlight(const bool full)
{
  /*
    \todo also need to call flight_state_reset() on Basic() ?

    calculated_info.Flying = false;
    if (full) {
      calculated_info.FlightTime = 0;
      calculated_info.TakeOffTime = 0;
    }
  */

  gps_info.ResetFlight(full);
  calculated_info.ResetFlight(full);
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
  calculated_info = Finish_Derived_Info;

  // \todo restore flying state
  //  SetBasic().flying_state = flying_state;
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
