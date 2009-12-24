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

#include "CalculationThread.hpp"
#include "GlideComputer.hpp"
#include "Protection.hpp"
#include "Screen/Blank.hpp"
#include "DeviceBlackboard.hpp"

/**
 * Constructor of the CalculationThread class
 * @param _glide_computer The GlideComputer used for the CalculationThread
 */
CalculationThread::CalculationThread(GlideComputer *_glide_computer)
  :running(_T("CalculationThread::running"), true),
   data_trigger(_T("dataTriggerEvent")),
   gps_trigger(TEXT("gpsUpdatedTriggerEvent")),
   glide_computer(_glide_computer) {}

/**
 * Main loop of the CalculationThread
 */
void
CalculationThread::run()
{
  bool need_calculations_slow;

  need_calculations_slow = false;

  // main loop until stop signal is tested
  while (!closeTriggerEvent.test()) {
    running.wait();

    // wait until the data_trigger is triggered
    // or MIN_WAIT_TIME has passed
    if (!data_trigger.wait(MIN_WAIT_TIME))
      continue;

    const bool gps_updated = gps_trigger.test();

    // update and transfer master info to glide computer
    mutexBlackboard.Lock();
    {
      const GlidePolar& glide_polar = glide_computer->get_glide_polar();

      // if (new GPS data available)
      if (gps_updated) {
        device_blackboard.tick(glide_polar);                
      } else {
        device_blackboard.tick_fast(glide_polar);
      }
      
      // Copy data from DeviceBlackboard to GlideComputerBlackboard
      glide_computer->ReadBlackboard(device_blackboard.Basic());
      // Copy settings form SettingsComputerBlackboard to GlideComputerBlackboard
      glide_computer->ReadSettingsComputer(device_blackboard.SettingsComputer());
      // Copy mapprojection from MapProjectionBlackboard to GlideComputerBlackboard
      glide_computer->ReadMapProjection(device_blackboard.MapProjection());
      
    }
    mutexBlackboard.Unlock();

    // if (new GPS data)
    if (gps_updated) {

      // inform map new data is ready
      drawTriggerEvent.trigger();

      if (!glide_computer->Basic().VarioAvailable) {
        TriggerVarioUpdate(); // emulate vario update
      }

      // process GPS data
      // if (time advanced)
      if (glide_computer->ProcessGPS()){
        need_calculations_slow = true;
      }
    }

    // drop out on exit event
    if (closeTriggerEvent.test())
      break;

    // if (time advanced and slow calculations need to be updated)
    if (need_calculations_slow) {
      // do slow calculations
      glide_computer->ProcessIdle();
      need_calculations_slow = false;
    }

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    mutexBlackboard.Lock();
    {
      device_blackboard.ReadBlackboard(glide_computer->Calculated());
    }
    mutexBlackboard.Unlock();

    // reset triggers
    data_trigger.reset();
    gps_trigger.reset();
  }
}
