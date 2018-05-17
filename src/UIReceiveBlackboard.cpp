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
UIReceiveSensorData()
{
  XCSoarInterface::ReceiveGPS();

  ApplyVegaSwitches();

  bool modified = ApplyExternalSettings();

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
