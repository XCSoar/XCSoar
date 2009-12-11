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

#include "XCSoar.h"
#include "MainWindow.hpp"
#include "MapWindow.h"
#include "Protection.hpp"
#include "SettingsUser.hpp"
#include "Device/Parser.h"
#include "Gauge/GaugeVario.hpp"
#include "Calculations.h"
#include "Interface.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include "CalculationThread.hpp"
#include "InstrumentThread.hpp"

#include <assert.h>

Trigger closeTriggerEvent(TEXT("mapCloseEvent"));
Trigger drawTriggerEvent(TEXT("drawTriggerEvent"),false);
Trigger globalRunningEvent(TEXT("globalRunning"));
Trigger airspaceWarningEvent(TEXT("airspaceWarning"));
Trigger targetManipEvent(TEXT("targetManip"));
Trigger triggerClimbEvent(TEXT("triggerClimb"));

Mutex mutexBlackboard;
// protect GPS_INFO, maccready etc,

/**
 * Triggers a GPS update resulting in a run of the calculation thread
 */
void TriggerGPSUpdate()
{
  calculation_thread->trigger_gps();
  calculation_thread->trigger_data();
}

void TriggerVarioUpdate()
{
  instrument_thread->trigger_vario();
}

void TriggerAll(void) {
  calculation_thread->trigger_data();
  drawTriggerEvent.trigger();
  instrument_thread->trigger_vario();
}

#include "DeviceBlackboard.hpp"


void CreateCalculationThread(void) {
  device_blackboard.ReadSettingsComputer(XCSoarInterface::SettingsComputer());

  glide_computer.ReadBlackboard(device_blackboard.Basic());
  glide_computer.ReadSettingsComputer(device_blackboard.SettingsComputer());
  glide_computer.ProcessGPS();

  XCSoarInterface::ExchangeBlackboard();

  device_blackboard.ReadBlackboard(glide_computer.Calculated());

  GaugeVario *gauge_vario = XCSoarInterface::main_window.vario;
  if (gauge_vario) {
    gauge_vario->ReadBlackboardBasic(device_blackboard.Basic());
    gauge_vario->ReadBlackboardCalculated(device_blackboard.Calculated());
    gauge_vario->ReadSettingsComputer(device_blackboard.SettingsComputer());
  }

  // Create a read thread for performing calculations
  calculation_thread = new CalculationThread(&glide_computer);
  instrument_thread = new InstrumentThread(gauge_vario);
}
