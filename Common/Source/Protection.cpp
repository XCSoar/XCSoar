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
#include "MapWindow.h"
#include "Trigger.hpp"
#include "Protection.hpp"
#include "InfoBoxManager.h"
#include "Settings.hpp"
#include "SettingsComputer.hpp"
#include "SettingsUser.hpp"
#include "Device/Parser.h"
#include "GaugeVarioAltA.h"
#include "Logger.h"
#include "Calculations.h"

#include <assert.h>

static Trigger gpsUpdatedTriggerEvent(TEXT("gpsUpdatedTriggerEvent"));
static Trigger dataTriggerEvent(TEXT("dataTriggerEvent"));
static Trigger varioTriggerEvent(TEXT("varioTriggerEvent"));
Trigger closeTriggerEvent(TEXT("mapCloseEvent"));
Trigger drawTriggerEvent(TEXT("drawTriggerEvent"));
Trigger globalRunningEvent(TEXT("globalRunning"));
Trigger airspaceWarningEvent(TEXT("airspaceWarning"));

Mutex mutexFlightData;
// protect GPS_INFO, mcready etc,
// should be fast
Mutex mutexGlideComputer;
// protect GlideComputer data

Mutex mutexEventQueue;
Mutex mutexTerrainData;
Mutex mutexMapData;
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
  if (MapWindow::IsDisplayRunning()) {
    if (gpsUpdatedTriggerEvent.test()) {
      MapWindow::dirtyEvent.trigger();
      if (!drawTriggerEvent.test()) {
	drawTriggerEvent.trigger();
      }
      // only ask for redraw if the thread was waiting,
      // this causes the map thread to try to synchronise
      // with the calculation thread, which is desirable
      // to reduce latency
      // it also ensures that if the display is lagging,
      // it will have a chance to catch up.
    }
  }
}


DWORD InstrumentThread (LPVOID lpvoid) {
	(void)lpvoid;
  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!closeTriggerEvent.test()) {

    if (!varioTriggerEvent.test()) {
      Sleep(100);
      continue;
    }

    if (MapWindow::IsDisplayRunning()) {
      if (EnableVarioGauge) {
	GaugeVario::Render();
      }
    }

    varioTriggerEvent.reset();
  }
  return 0;
}


DWORD CalculationThread (LPVOID lpvoid) {
	(void)lpvoid;
  bool need_calculations_slow;

  NMEA_INFO     tmp_GPS_INFO;
  DERIVED_INFO  tmp_CALCULATED_INFO;

  need_calculations_slow = false;

  // wait for proper startup signal
  while (!MapWindow::IsDisplayRunning()) {
    Sleep(100);
  }

  while (!closeTriggerEvent.test()) {

    if (!dataTriggerEvent.test()) {
      Sleep(100);
      continue;
    }

    // set timer to determine latency (including calculations)
    if (gpsUpdatedTriggerEvent.test()) {
      //      MapWindow::UpdateTimeStats(true);
    }

    // make local copy before editing...
    mutexFlightData.Lock();
    if (gpsUpdatedTriggerEvent.test()) { // timeout on FLARM objects
      FLARM_RefreshSlots(&GPS_INFO);
    }
    memcpy(&tmp_GPS_INFO,&GPS_INFO,sizeof(NMEA_INFO));
    memcpy(&tmp_CALCULATED_INFO,&CALCULATED_INFO,sizeof(DERIVED_INFO));

    bool has_vario = GPS_INFO.VarioAvailable;
    mutexFlightData.Unlock();

    // Do vario first to reduce audio latency
    if (has_vario) {
      if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {

      }
      // assume new vario data has arrived, so infoboxes
      // need to be redrawn
    } else {
      // run the function anyway, because this gives audio functions
      // if no vario connected
      if (gpsUpdatedTriggerEvent.test()) {
	if (DoCalculationsVario(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)) {
	}
	TriggerVarioUpdate(); // emulate vario update
      }
    }

    if (gpsUpdatedTriggerEvent.test()) {
      if(DoCalculations(&tmp_GPS_INFO,&tmp_CALCULATED_INFO)){
	MapWindow::dirtyEvent.trigger();
        need_calculations_slow = true;
      }
      InfoBoxesSetDirty(true);
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    TriggerRedraws();

    if (closeTriggerEvent.test())
      break; // drop out on exit

    if (need_calculations_slow) {
      DoCalculationsSlow(&tmp_GPS_INFO,&tmp_CALCULATED_INFO);
      need_calculations_slow = false;
    }

    if (closeTriggerEvent.test())
      break; // drop out on exit

    // values changed, so copy them back now: ONLY CALCULATED INFO
    // should be changed in DoCalculations, so we only need to write
    // that one back (otherwise we may write over new data)
    mutexFlightData.Lock();
    memcpy(&CALCULATED_INFO,&tmp_CALCULATED_INFO,sizeof(DERIVED_INFO));
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
