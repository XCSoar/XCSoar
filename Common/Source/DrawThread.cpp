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

#include "DrawThread.hpp"
#include "MapWindow.h"
#include "Gauge/GaugeFLARM.hpp"
#include "Protection.hpp"

/**
 * Main loop of the DrawThread
 */
void
DrawThread::run()
{
  bool bounds_dirty = false;

  // wait for start
  globalRunningEvent.wait();

  map.ExchangeBlackboard();

  // take control
  mutexRun.Lock();

  // first time draw
  map.DrawThreadLoop();

  bounds_dirty = map.SmartBounds(true);
  map.Idle(true);

  while (map.Idle(false)) {};

  // first time draw
  map.DrawThreadLoop();

  // release control
  mutexRun.Unlock();

  do {
    if (drawTriggerEvent.wait(MIN_WAIT_TIME)) {
      map.ExchangeBlackboard();

      // take control
      mutexRun.Lock();

      if (flarm != NULL) {
        if (map.Basic().FLARM_AlarmLevel > 0) {
          flarm->Suppress = false;
        }
        flarm->TrafficPresent(map.Basic().FLARMTraffic);
        flarm->Show(map.SettingsMap().EnableFLARMGauge);
        flarm->Render(map.Basic());
      }

      map.DrawThreadLoop();
      if (map.SmartBounds(false)) {
        // this call is quick
        bounds_dirty = map.Idle(true);
      }

      // release control
      mutexRun.Unlock();

      continue;
    } else if (bounds_dirty) {
      // take control
      mutexRun.Lock();

      bounds_dirty = map.Idle(false);

      // release control
      mutexRun.Unlock();

      continue;
    }
  } while (!closeTriggerEvent.wait(500));
}
