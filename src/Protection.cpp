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

#include "Protection.hpp"
#include "MainWindow.hpp"
#include "MapWindow.hpp"
#include "SettingsMap.hpp"
#include "Gauge/GaugeVario.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "GlideComputer.hpp"
#include "CalculationThread.hpp"
#include "InstrumentThread.hpp"
#include "DrawThread.hpp"

#include <assert.h>

Trigger globalRunningEvent;
Trigger airspaceWarningEvent;

Mutex mutexBlackboard;
// protect GPS_INFO, maccready etc,

/**
 * Triggers a GPS update resulting in a run of the calculation thread
 */
void TriggerGPSUpdate()
{
  if (calculation_thread == NULL)
    return;

  calculation_thread->trigger_data();
}

void TriggerVarioUpdate()
{
  if (instrument_thread == NULL)
    return;

  instrument_thread->trigger();
}

#include "DeviceBlackboard.hpp"


void CreateCalculationThread(void) {
  assert(glide_computer != NULL);

  device_blackboard.ReadSettingsComputer(XCSoarInterface::SettingsComputer());

  glide_computer->ReadBlackboard(device_blackboard.Basic());
  glide_computer->ReadSettingsComputer(device_blackboard.SettingsComputer());
  glide_computer->ProcessGPS();

  XCSoarInterface::ExchangeBlackboard();

  device_blackboard.ReadBlackboard(glide_computer->Calculated());

  GaugeVario *gauge_vario = XCSoarInterface::main_window.vario;
  if (gauge_vario != NULL)
    instrument_thread = new InstrumentThread(*gauge_vario);

  // Create a read thread for performing calculations
  calculation_thread = new CalculationThread(*glide_computer);
}

void
SuspendAllThreads()
{
#ifndef ENABLE_OPENGL
  draw_thread->suspend();
#endif
  calculation_thread->suspend();
}

void
ResumeAllThreads()
{
  calculation_thread->resume();
#ifndef ENABLE_OPENGL
  draw_thread->resume();
#endif
}
