/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000, 2001, 2002, 2003, 2004, 2005, 2006, 2007, 2008, 2009

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
	Tobias Bieniek <tobias.bieniek@gmx.de>

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
#include "MapWindow.h"
#include "Gauge/GaugeFLARM.hpp"
#include "Protection.hpp"
#include "DeviceBlackboard.hpp"

void
DrawThread::ExchangeBlackboard()
{
  /* send device data to the MapWindow */
  mutexBlackboard.Lock();
  map.ReadBlackboard(device_blackboard.Basic(), device_blackboard.Calculated(),
                     device_blackboard.SettingsComputer(),
                     device_blackboard.SettingsMap());
  mutexBlackboard.Unlock();

  /* recalculate the MapWindow projection */
  map.UpdateProjection();

  /* return MapWindow projection to the device_blackboard */
  mutexBlackboard.Lock();
  device_blackboard.ReadMapProjection(map.MapProjection());
  mutexBlackboard.Unlock();
}

/**
 * Main loop of the DrawThread
 */
void
DrawThread::run()
{
  bool bounds_dirty = false;

  // wait until the startup is finished
  running.wait();

  // Get data from the DeviceBlackboard
  ExchangeBlackboard();

  // first time draw
  map.DrawThreadLoop();

  bounds_dirty = map.SmartBounds(true);
  map.Idle(true);

  while (map.Idle(false)) {};

  // second time draw
  map.DrawThreadLoop();

  // QUESTION TB: any reason for the do-loop here instead of while?
  // circle until application is closed
  do {
    if (drawTriggerEvent.wait(MIN_WAIT_TIME)) {

      // take control (or wait for the resume())
      running.wait();

      // Get data from the DeviceBlackboard
      ExchangeBlackboard();

      if (flarm != NULL) {
        // If FLARM alarm level higher then 0
        if (map.Basic().FLARM_AlarmLevel > 0) {
          // Show FLARM gauge and do not care about suppression
          flarm->Suppress = false;
        }

        flarm->TrafficPresent(map.Basic().FLARMTraffic);
        flarm->Show(map.SettingsMap().EnableFLARMGauge);

        // Draw/Render the FLARM gauge
        flarm->Render(map.Basic());
      }

      // Draw the moving map
      map.DrawThreadLoop();

      if (drawTriggerEvent.test()) {
        // interrupt re-calculation of bounds if there was a 
        // request made.  Since we will re-enter, we know the remainder
        // of this code will be called anyway.
        continue;
      }

      if (map.SmartBounds(false)) {
        // this call is quick
        bounds_dirty = map.Idle(true);
      }

      continue;

    // QUESTION TB: what's the point of bounds_dirty?!
    } else if (bounds_dirty) {
      // take control (or wait for the resume())
      running.wait();

      bounds_dirty = map.Idle(false);

      continue;
    }
  } while (!closeTriggerEvent.wait(MIN_WAIT_TIME));
}
