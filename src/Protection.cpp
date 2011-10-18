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
#include "Gauge/GlueGaugeVario.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "DrawThread.hpp"

#include <assert.h>

Flag globalRunningEvent;

void
TriggerMergeThread()
{
  if (merge_thread != NULL)
    merge_thread->Trigger();
}

/**
 * Triggers a GPS update resulting in a run of the calculation thread
 */
void TriggerGPSUpdate()
{
  if (calculation_thread == NULL)
    return;

  calculation_thread->Trigger();
}

void TriggerVarioUpdate()
{
  GlueGaugeVario *gauge_vario = CommonInterface::main_window.vario;
  if (gauge_vario != NULL)
    gauge_vario->invalidate_blackboard();
}

void
TriggerMapUpdate()
{
  CommonInterface::main_window.full_redraw();
}

void
TriggerCalculatedUpdate()
{
  CommonInterface::main_window.SendCalculatedUpdate();
}

#include "DeviceBlackboard.hpp"


void CreateCalculationThread(void) {
  assert(glide_computer != NULL);

  device_blackboard->ReadSettingsComputer(XCSoarInterface::SettingsComputer());

  glide_computer->ReadBlackboard(device_blackboard->Basic());
  glide_computer->ReadSettingsComputer(device_blackboard->SettingsComputer());
  glide_computer->ProcessGPS();

  device_blackboard->ReadBlackboard(glide_computer->Calculated());

  // Create a read thread for performing calculations
  merge_thread = new MergeThread(*device_blackboard);
  merge_thread->Trigger();

  calculation_thread = new CalculationThread(*glide_computer);
}

void
SuspendAllThreads()
{
  /* not suspending MergeThread, because it does not access shared
     unprotected data structures */

#ifndef ENABLE_OPENGL
  draw_thread->Suspend();
#endif
  calculation_thread->Suspend();
}

void
ResumeAllThreads()
{
  calculation_thread->Resume();
#ifndef ENABLE_OPENGL
  draw_thread->Resume();
#endif
}
