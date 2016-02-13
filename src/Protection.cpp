/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Interface.hpp"
#include "Components.hpp"
#include "Computer/GlideComputer.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "Blackboard/DeviceBlackboard.hpp"

#include <assert.h>

bool global_running;

void
TriggerMergeThread()
{
  if (merge_thread != nullptr)
    merge_thread->Trigger();
}

/**
 * Triggers a GPS update resulting in a run of the calculation thread
 */
void
TriggerGPSUpdate()
{
  if (calculation_thread == nullptr)
    return;

  calculation_thread->Trigger();
}

void
ForceCalculation()
{
  if (calculation_thread != nullptr)
    calculation_thread->ForceTrigger();
}

void
TriggerVarioUpdate()
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->SendGPSUpdate();
}

void
TriggerMapUpdate()
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->FullRedraw();
}

void
TriggerCalculatedUpdate()
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->SendCalculatedUpdate();
}

void
CreateCalculationThread()
{
  assert(glide_computer != nullptr);

  /* copy settings to DeviceBlackboard */
  device_blackboard->ReadComputerSettings(CommonInterface::GetComputerSettings());

  /* create and run MergeThread, because GlideComputer's first
     iteration depends on MergeThread's results */
  merge_thread = new MergeThread(*device_blackboard);
  merge_thread->FirstRun();

  /* initialise the GlideComputer and run the first iteration */
  glide_computer->ReadBlackboard(device_blackboard->Basic());
  glide_computer->ReadComputerSettings(device_blackboard->GetComputerSettings());
  glide_computer->ProcessGPS(true);

  /* copy GlideComputer results to DeviceBlackboard */
  device_blackboard->ReadBlackboard(glide_computer->Calculated());

  calculation_thread = new CalculationThread(*glide_computer);
  calculation_thread->SetComputerSettings(CommonInterface::GetComputerSettings());
}

void
SuspendAllThreads()
{
  assert(CommonInterface::main_window != nullptr);
  assert(calculation_thread != nullptr);

  /* not suspending MergeThread, because it does not access shared
     unprotected data structures */

  CommonInterface::main_window->SuspendThreads();
  calculation_thread->Suspend();
}

void
ResumeAllThreads()
{
  assert(calculation_thread != nullptr);
  assert(CommonInterface::main_window != nullptr);

  calculation_thread->Resume();
  CommonInterface::main_window->ResumeThreads();
}
