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
#include "GlueMapWindow.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "Gauge/GaugeThermalAssistant.hpp"
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
  device_blackboard.ReadMapProjection(map.VisibleProjection());
  mutexBlackboard.Unlock();
}

/**
 * Main loop of the DrawThread
 */
void
DrawThread::run()
{
  set_low_priority();

  // bounds_dirty maintains the status of whether the map
  // bounds have changed and there are pending idle calls
  // to be run in the map.

  // wait until the startup is finished
  running.wait();

  // Get data from the DeviceBlackboard
  ExchangeBlackboard();

  // first time draw
  map.DrawThreadLoop();

  bool bounds_dirty = true;
  while (bounds_dirty)
    bounds_dirty = map.Idle();

  // second time draw
  map.DrawThreadLoop();

  // circle until application is closed
  while (true) {
    if (!bounds_dirty)
      trigger.wait();

    if (!bounds_dirty || trigger.wait(MIN_WAIT_TIME)) {
      trigger.reset();

      // take control (or wait for the resume())
      running.wait();

      /* got the "stop" trigger? */
      if (is_stopped())
        break;

      // Get data from the DeviceBlackboard
      ExchangeBlackboard();

      if (flarm != NULL)
        flarm->Update(map.SettingsMap().EnableFLARMGauge, map.Basic(),
                      map.SettingsComputer());

      if (ta != NULL)
        ta->Update(map.SettingsMap().EnableTAGauge, map.Basic().Heading,
                   map.Calculated());

      // Draw the moving map
      map.DrawThreadLoop();

      if (trigger.test()) {
        // interrupt re-calculation of bounds if there was a 
        // request made.  Since we will re-enter, we know the remainder
        // of this code will be called anyway.
        continue;
      }

      bounds_dirty = map.Idle();
    } else if (bounds_dirty) {
      // take control (or wait for the resume())
      running.wait();

      /* got the "stop" trigger? */
      if (is_stopped())
        break;

      bounds_dirty = map.Idle();
    }
  }
}
