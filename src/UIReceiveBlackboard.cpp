// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "UIReceiveBlackboard.hpp"
#include "Interface.hpp"
#include "ActionInterface.hpp"
#include "ApplyVegaSwitches.hpp"
#include "ApplyExternalSettings.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Device/MultipleDevices.hpp"
#include "Input/TaskEventObserver.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Components.hpp"

static TaskEventObserver task_event_observer;

void
UIReceiveSensorData(OperationEnvironment &env)
{
  XCSoarInterface::ReceiveGPS();

  ApplyVegaSwitches();

  bool modified = ApplyExternalSettings(env);

  /*
   * Update the infoboxes if no location is available
   *
   * (if the location is available the CalculationThread will send the
   * Command::CALCULATED_UPDATE message which will update them)
   */
  if (modified || !CommonInterface::Basic().location_available) {
    InfoBoxManager::SetDirty();
    InfoBoxManager::ProcessTimer();
  }
}

void
UIReceiveCalculatedData()
{
  XCSoarInterface::ReceiveCalculated();

  ActionInterface::UpdateDisplayMode();
  ActionInterface::SendUIState();

  if (devices != nullptr)
    devices->NotifyCalculatedUpdate(CommonInterface::Basic(),
                                    CommonInterface::Calculated());

  {
    const ProtectedTaskManager::Lease lease(*protected_task_manager);
    task_event_observer.Check(lease);
  }
}
