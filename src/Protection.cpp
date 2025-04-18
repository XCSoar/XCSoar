// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Protection.hpp"
#include "MainWindow.hpp"
#include "Interface.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Computer/GlideComputer.hpp"
#include "CalculationThread.hpp"
#include "MergeThread.hpp"
#include "Blackboard/DeviceBlackboard.hpp"

#include <cassert>

bool global_running;

void
TriggerMergeThread() noexcept
{
  if (backend_components->merge_thread)
    backend_components->merge_thread->Trigger();
}

/**
 * Triggers a GPS update resulting in a run of the calculation thread
 */
void
TriggerGPSUpdate() noexcept
{
  if (backend_components->calculation_thread)
    backend_components->calculation_thread->Trigger();
}

void
ForceCalculation() noexcept
{
  if (backend_components->calculation_thread)
    backend_components->calculation_thread->ForceTrigger();
}

void
TriggerVarioUpdate() noexcept
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->SendGPSUpdate();
}

void
TriggerMapUpdate() noexcept
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->FullRedraw();
}

void
TriggerCalculatedUpdate() noexcept
{
  assert(CommonInterface::main_window != nullptr);

  CommonInterface::main_window->SendCalculatedUpdate();
}

void
CreateCalculationThread() noexcept
{
  assert(backend_components->glide_computer != nullptr);

  auto &device_blackboard = *backend_components->device_blackboard;

  /* copy settings to DeviceBlackboard */
  device_blackboard.ReadComputerSettings(CommonInterface::GetComputerSettings());

  /* create and run MergeThread, because GlideComputer's first
     iteration depends on MergeThread's results */
  backend_components->merge_thread = std::make_unique<MergeThread>(*backend_components->device_blackboard,
                                                                   backend_components->devices.get());
  backend_components->merge_thread->FirstRun();

  /* copy the MergeThead::FirstRun() results to the
     InterfaceBlackboard because nothing else will initalise some
     important fallback values set by BasicComputer
     (e.g. AttitudeState::heading) */
  CommonInterface::ReadBlackboardBasic(device_blackboard.Basic());

  /* initialise the GlideComputer and run the first iteration */
  auto &glide_computer = *backend_components->glide_computer;
  glide_computer.ReadBlackboard(device_blackboard.Basic());
  glide_computer.ReadComputerSettings(device_blackboard.GetComputerSettings());
  glide_computer.ProcessGPS(true);

  /* copy GlideComputer results to DeviceBlackboard */
  device_blackboard.ReadBlackboard(glide_computer.Calculated());

  backend_components->calculation_thread = std::make_unique<CalculationThread>(device_blackboard, glide_computer);
  backend_components->calculation_thread->SetComputerSettings(CommonInterface::GetComputerSettings());
}

void
SuspendAllThreads() noexcept
{
  assert(CommonInterface::main_window != nullptr);

  /* not suspending MergeThread, because it does not access shared
     unprotected data structures */

  CommonInterface::main_window->SuspendThreads();

  if (backend_components->calculation_thread)
    backend_components->calculation_thread->Suspend();
}

void
ResumeAllThreads() noexcept
{
  assert(CommonInterface::main_window != nullptr);

  if (backend_components->calculation_thread)
    backend_components->calculation_thread->Resume();

  CommonInterface::main_window->ResumeThreads();
}
