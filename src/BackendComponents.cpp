// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackendComponents.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Computer/GlideComputer.hpp"
#include "Logger/Logger.hpp"
#include "Logger/NMEALogger.hpp"
#include "Logger/GlueFlightLogger.hpp"
#include "Replay/Replay.hpp"
#include "MergeThread.hpp"
#include "CalculationThread.hpp"

BackendComponents::BackendComponents() noexcept
  :device_blackboard(new DeviceBlackboard())
{
}

BackendComponents::~BackendComponents() noexcept = default;

ProtectedAirspaceWarningManager *
BackendComponents::GetAirspaceWarnings() noexcept
{
  return glide_computer
    ? &glide_computer->GetAirspaceWarnings()
    : nullptr;
}

void
BackendComponents::SetTaskPolar(const PolarSettings &settings) noexcept
{
  if (protected_task_manager)
    protected_task_manager->SetGlidePolar(settings.glide_polar_task);

  if (calculation_thread) {
    calculation_thread->SetPolarSettings(settings);
    calculation_thread->ForceTrigger();
  }
}
