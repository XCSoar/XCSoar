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

#include "CalculationThread.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "Screen/Blank.hpp"
#include "DeviceBlackboard.hpp"
#include "Components.hpp"
#include "GlideSolvers/GlidePolar.hpp"

/**
 * Constructor of the CalculationThread class
 * @param _glide_computer The GlideComputer used for the CalculationThread
 */
CalculationThread::CalculationThread(GlideComputer &_glide_computer)
  :WorkerThread(450, 100, 50), glide_computer(_glide_computer) {
}

void
CalculationThread::SetSettingsComputer(const SETTINGS_COMPUTER &new_value)
{
  ScopeLock protect(mutex);
  settings_computer = new_value;
}

void
CalculationThread::SetScreenDistanceMeters(fixed new_value)
{
  ScopeLock protect(mutex);
  screen_distance_meters = new_value;
}

/**
 * Main loop of the CalculationThread
 */
void
CalculationThread::Tick()
{
  bool gps_updated;

  // update and transfer master info to glide computer
  {
    ScopeLock protect(device_blackboard.mutex);

    gps_updated = device_blackboard.Basic().location_available.Modified(glide_computer.Basic().location_available);

    // Copy data from DeviceBlackboard to GlideComputerBlackboard
    glide_computer.ReadBlackboard(device_blackboard.Basic());
  }

  {
    ScopeLock protect(mutex);
    // Copy settings form SettingsComputerBlackboard to GlideComputerBlackboard
    glide_computer.ReadSettingsComputer(settings_computer);
    // Copy mapprojection from MapProjectionBlackboard to GlideComputerBlackboard
    glide_computer.SetScreenDistanceMeters(screen_distance_meters);
  }

  glide_computer.Expire();

  bool do_idle = false;

  if (gps_updated) {
    // perform idle call if time advanced and slow calculations need to be updated
    do_idle |= glide_computer.ProcessGPS();
  }

  // values changed, so copy them back now: ONLY CALCULATED INFO
  // should be changed in DoCalculations, so we only need to write
  // that one back (otherwise we may write over new data)
  {
    ScopeLock protect(device_blackboard.mutex);
    device_blackboard.ReadBlackboard(glide_computer.Calculated());
  }

  // if (new GPS data)
  if (gps_updated) {
    // inform map new data is ready
    TriggerMapUpdate();
  }

  if (do_idle) {
    // do slow calculations last, to minimise latency
    glide_computer.ProcessIdle();
  }
}
