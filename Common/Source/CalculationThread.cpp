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

#include "CalculationThread.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "Screen/Blank.hpp"
#include "DeviceBlackboard.hpp"

CalculationThread::CalculationThread(GlideComputer *_glide_computer)
  :data_trigger(TEXT("dataTriggerEvent")),
   gps_trigger(TEXT("gpsUpdatedTriggerEvent")),
   glide_computer(_glide_computer) {}

void
CalculationThread::run()
{
  bool need_calculations_slow;

  need_calculations_slow = false;

  // wait for proper startup signal
  globalRunningEvent.wait();

  while (!closeTriggerEvent.test()) {

    if (!data_trigger.wait(MIN_WAIT_TIME))
      continue;

    // update and transfer master info to glide computer
    mutexBlackboard.Lock();
    if (gps_trigger.test()) { 
      // timeout on FLARM objects
      device_blackboard.FLARM_RefreshSlots();
      // lookup known traffic
      device_blackboard.FLARM_ScanTraffic();
      // set system time if necessary
      device_blackboard.SetSystemTime();
      // inform map new data is ready
      drawTriggerEvent.trigger();
    }
    glide_computer->ReadBlackboard(device_blackboard.Basic());
    glide_computer->ReadSettingsComputer(device_blackboard.SettingsComputer());
    glide_computer->ReadMapProjection(device_blackboard.MapProjection());
    mutexBlackboard.Unlock();

    bool calculations_updated = false;

    // Do vario first to reduce audio latency
    if (glide_computer->Basic().VarioAvailable) {
      glide_computer->ProcessVario();
      calculations_updated = true;
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (gps_trigger.test()) {
	glide_computer->ProcessVario();
	TriggerVarioUpdate(); // emulate vario update
	calculations_updated = true;
      }
    }

    if (gps_trigger.test()) {
      if (glide_computer->ProcessGPS()){
        need_calculations_slow = true;
      }
      calculations_updated = true;
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    if (need_calculations_slow) {
      glide_computer->ProcessIdle();
      need_calculations_slow = false;
      calculations_updated = true;
    }

    if (calculations_updated) {
      // values changed, so copy them back now: ONLY CALCULATED INFO
      // should be changed in DoCalculations, so we only need to write
      // that one back (otherwise we may write over new data)
      mutexBlackboard.Lock();
      device_blackboard.ReadBlackboard(glide_computer->Calculated());
      mutexBlackboard.Unlock();
    }

    // reset triggers
    data_trigger.reset();
    gps_trigger.reset();
  }
}
