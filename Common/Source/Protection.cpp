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

#include "XCSoar.h"
#include "Blackboard.hpp"
#include "MainWindow.hpp"
#include "MapWindow.h"
#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Settings.hpp"
#include "SettingsUser.hpp"
#include "Device/Parser.h"
#include "Gauge/GaugeVario.hpp"
#include "Logger.h"
#include "Calculations.h"
#include "Interface.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include <assert.h>

static Trigger gpsUpdatedTriggerEvent(TEXT("gpsUpdatedTriggerEvent"));
static Trigger dataTriggerEvent(TEXT("dataTriggerEvent"));
static Trigger varioTriggerEvent(TEXT("varioTriggerEvent"));
Trigger closeTriggerEvent(TEXT("mapCloseEvent"));
Trigger drawTriggerEvent(TEXT("drawTriggerEvent"),false);
Trigger globalRunningEvent(TEXT("globalRunning"));
Trigger airspaceWarningEvent(TEXT("airspaceWarning"));
Trigger targetManipEvent(TEXT("targetManip"));
Trigger triggerClimbEvent(TEXT("triggerClimb"));

Mutex mutexFlightData;
// protect GPS_INFO, mcready etc,
// should be fast
Mutex mutexGlideComputer;
// protect GlideComputer data

Mutex mutexEventQueue;

Mutex TerrainDataClient::mutexTerrainData;
Mutex MapDataClient::mutexMapData;

Mutex mutexNavBox;
Mutex mutexComm;
Mutex mutexTaskData;

//////////

void TriggerGPSUpdate()
{
  gpsUpdatedTriggerEvent.trigger();
  dataTriggerEvent.trigger();
}

void TriggerVarioUpdate()
{
  varioTriggerEvent.trigger(); // was pulse
}

void TriggerAll(void) {
  dataTriggerEvent.trigger();
  drawTriggerEvent.trigger();
  varioTriggerEvent.trigger();
}

void TriggerRedraws() {
  if (XCSoarInterface::main_window.map.IsDisplayRunning()) {
    if (gpsUpdatedTriggerEvent.test()) {
      drawTriggerEvent.trigger();
    }
  }
}


DWORD InstrumentThread (LPVOID lpvoid) {
	(void)lpvoid;
  // wait for proper startup signal
  while (!XCSoarInterface::main_window.map.IsDisplayRunning()) {
    Sleep(MIN_WAIT_TIME);
  }

  while (!closeTriggerEvent.test()) {

    if (!varioTriggerEvent.wait(MIN_WAIT_TIME))
      continue;

    if (XCSoarInterface::main_window.map.IsDisplayRunning()) {
      if (EnableVarioGauge) {
	
	mutexFlightData.Lock();
	gauge_vario->ReadBlackboardBasic(device_blackboard.Basic());
	gauge_vario->ReadBlackboardCalculated(device_blackboard.Calculated());
	gauge_vario->ReadSettingsComputer(device_blackboard.SettingsComputer());
	mutexFlightData.Unlock();
	gauge_vario->Render();
	
      }
    }

    varioTriggerEvent.reset();
  }
  return 0;
}


DWORD CalculationThread (LPVOID lpvoid) {
	(void)lpvoid;
  bool need_calculations_slow;

  need_calculations_slow = false;

  // wait for proper startup signal
  while (!XCSoarInterface::main_window.map.IsDisplayRunning()) {
    Sleep(MIN_WAIT_TIME);
  }

  while (!closeTriggerEvent.test()) {

    if (!dataTriggerEvent.wait(MIN_WAIT_TIME))
      continue;

    // set timer to determine latency (including calculations)
    if (gpsUpdatedTriggerEvent.test()) {
      //      MapWindow::UpdateTimeStats(true);
    }

    // make local copy before editing...
    mutexFlightData.Lock();
    if (gpsUpdatedTriggerEvent.test()) { // timeout on FLARM objects
      device_blackboard.FLARM_RefreshSlots();
    }
    glide_computer.ReadBlackboard(device_blackboard.Basic());
    glide_computer.ReadSettingsComputer(device_blackboard.SettingsComputer());
    mutexFlightData.Unlock();

    TriggerRedraws(); // just the map

    bool has_vario = glide_computer.Basic().VarioAvailable;

    // Do vario first to reduce audio latency
    if (has_vario) {
      glide_computer.ProcessVario();
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (gpsUpdatedTriggerEvent.test()) {
	glide_computer.ProcessVario();
	TriggerVarioUpdate(); // emulate vario update
      }
    }

    if (gpsUpdatedTriggerEvent.test()) {
      if (glide_computer.ProcessGPS()){
        need_calculations_slow = true;
      }
      InfoBoxManager::SetDirty(true);
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    if (closeTriggerEvent.test())
      break; // drop out on exit

    if (need_calculations_slow) {
      glide_computer.ProcessIdle();
      need_calculations_slow = false;
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    mutexFlightData.Lock();
    device_blackboard.ReadBlackboard(glide_computer.Calculated());
    glide_computer.ReadMapProjection(device_blackboard.MapProjection());
    mutexFlightData.Unlock();

    // reset triggers
    dataTriggerEvent.reset();
    gpsUpdatedTriggerEvent.reset();
  }
  return 0;
}


void CreateCalculationThread(void) {
  HANDLE hCalculationThread;
  DWORD dwCalcThreadID;

  device_blackboard.ReadSettingsComputer(XCSoarInterface::SettingsComputer());

  glide_computer.ReadBlackboard(device_blackboard.Basic());
  glide_computer.ReadSettingsComputer(device_blackboard.SettingsComputer());
  glide_computer.ProcessGPS();

  XCSoarInterface::ReadBlackboardBasic(device_blackboard.Basic());
  XCSoarInterface::ReadBlackboardCalculated(glide_computer.Calculated());
  XCSoarInterface::SendSettingsMap();

  device_blackboard.ReadBlackboard(glide_computer.Calculated());
  if (gauge_vario) {
    gauge_vario->ReadBlackboardBasic(device_blackboard.Basic());
    gauge_vario->ReadBlackboardCalculated(device_blackboard.Calculated());
    gauge_vario->ReadSettingsComputer(device_blackboard.SettingsComputer());
  }

  // Create a read thread for performing calculations
  if ((hCalculationThread =
      CreateThread (NULL, 0,
        (LPTHREAD_START_ROUTINE )CalculationThread,
         0, 0, &dwCalcThreadID)) != NULL)
  {
    SetThreadPriority(hCalculationThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hCalculationThread);
  } else {
    assert(1);
  }

  HANDLE hInstrumentThread;
  DWORD dwInstThreadID;

  if ((hInstrumentThread =
      CreateThread (NULL, 0,
       (LPTHREAD_START_ROUTINE )InstrumentThread,
        0, 0, &dwInstThreadID)) != NULL)
  {
    SetThreadPriority(hInstrumentThread, THREAD_PRIORITY_NORMAL);
    CloseHandle (hInstrumentThread);
  } else {
    assert(1);
  }

}
