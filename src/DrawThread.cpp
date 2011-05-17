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

#include "DrawThread.hpp"
#include "GlueMapWindow.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
#include "Protection.hpp"
#include "DeviceBlackboard.hpp"

#ifndef ENABLE_OPENGL

/**
 * Main loop of the DrawThread
 */
void
DrawThread::Run()
{
  SetLowPriority();

  // bounds_dirty maintains the status of whether the map
  // bounds have changed and there are pending idle calls
  // to be run in the map.

  // wait until the startup is finished
  running.Wait();

  // Get data from the DeviceBlackboard
  map.ExchangeBlackboard();

  bool bounds_dirty = true;

  // circle until application is closed
  while (true) {
    if (!bounds_dirty)
      trigger.Wait();

    if (!bounds_dirty || trigger.Wait(MIN_WAIT_TIME)) {
      // take control (or wait for the resume())
      running.Wait();

      /* got the "stop" trigger? */
      if (CheckStopped())
        break;

      trigger.Reset();

      // Get data from the DeviceBlackboard
      map.ExchangeBlackboard();

      // Draw the moving map
      map.repaint();

      if (trigger.Test()) {
        // interrupt re-calculation of bounds if there was a 
        // request made.  Since we will re-enter, we know the remainder
        // of this code will be called anyway.
        continue;
      }

      bounds_dirty = map.Idle();
    } else if (bounds_dirty) {
      // take control (or wait for the resume())
      running.Wait();

      /* got the "stop" trigger? */
      if (CheckStopped())
        break;

      bounds_dirty = map.Idle();
    }
  }
}

#endif
