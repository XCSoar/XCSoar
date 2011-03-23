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
#include "Interface.hpp"
#include "MainWindow.hpp"

/**
 * Constructor of the CalculationThread class
 * @param _glide_computer The GlideComputer used for the CalculationThread
 */
CalculationThread::CalculationThread(GlideComputer &_glide_computer)
  :glide_computer(_glide_computer) {}

/**
 * Main loop of the CalculationThread
 */
void
CalculationThread::tick()
{
  const bool gps_updated = true; /* XXX derive from LocationAvailable? */

  // update and transfer master info to glide computer
  {
    ScopeLock protect(mutexBlackboard);

    // if (new GPS data available)
    if (gps_updated)
      device_blackboard.tick();
    else
      device_blackboard.tick_fast();

    // Copy data from DeviceBlackboard to GlideComputerBlackboard
    glide_computer.ReadBlackboard(device_blackboard.Basic());
    // Copy settings form SettingsComputerBlackboard to GlideComputerBlackboard
    glide_computer.ReadSettingsComputer(device_blackboard.SettingsComputer());
    // Copy mapprojection from MapProjectionBlackboard to GlideComputerBlackboard
    glide_computer.SetScreenDistanceMeters(device_blackboard.GetScreenDistanceMeters());
  }

  // if (time advanced and slow calculations need to be updated)
  if (gps_updated && glide_computer.ProcessGPS())
    // do slow calculations
    glide_computer.ProcessIdle();

  // values changed, so copy them back now: ONLY CALCULATED INFO
  // should be changed in DoCalculations, so we only need to write
  // that one back (otherwise we may write over new data)
  {
    ScopeLock protect(mutexBlackboard);
    device_blackboard.ReadBlackboard(glide_computer.Calculated());
    if (device_blackboard.Basic().MacCready != glide_computer.Basic().MacCready)
      device_blackboard.SetMC(glide_computer.Basic().MacCready);
  }

  // if (new GPS data)
  if (gps_updated) {
    // inform map new data is ready
    CommonInterface::main_window.full_redraw();

    if (!glide_computer.Basic().TotalEnergyVarioAvailable)
      // emulate vario update
      TriggerVarioUpdate();
  }
}
