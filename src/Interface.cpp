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

#include "Interface.hpp"
#include "Thread/Mutex.hpp"
#include "MainWindow.hpp"
#include "MapWindow/GlueMapWindow.hpp"
#include "Language/Language.hpp"
#include "Dialogs/Message.hpp"
#include "StatusMessage.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "InfoBoxes/InfoBoxLayout.hpp"
#include "Screen/Layout.hpp"
#include "Asset.hpp"
#include "Components.hpp"
#include "DrawThread.hpp"
#include "Gauge/GlueGaugeVario.hpp"
#include "Gauge/GaugeFLARM.hpp"
#include "PeriodClock.hpp"
#include "LogFile.hpp"
#include "DeviceBlackboard.hpp"
#include "CalculationThread.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "UIState.hpp"

UIState CommonInterface::ui_state;

bool CommonInterface::movement_detected = false;

bool ActionInterface::force_shutdown = false;

InterfaceBlackboard CommonInterface::blackboard;
StatusMessageList CommonInterface::status_messages;
MainWindow CommonInterface::main_window(status_messages);

bool
CommonInterface::IsPanning()
{
  const GlueMapWindow *map = main_window.GetMapIfActive();
  return map != NULL && map->IsPanning();

}

void
XCSoarInterface::ReceiveGPS()
{
  {
    ScopeLock protect(device_blackboard->mutex);

    ReadBlackboardBasic(device_blackboard->Basic());

    const NMEAInfo &real = device_blackboard->RealState();
    movement_detected = real.connected && real.gps.real &&
      real.MovementDetected();
  }

  BroadcastGPSUpdate();
}

void
XCSoarInterface::ReceiveCalculated()
{
  {
    ScopeLock protect(device_blackboard->mutex);

    ReadBlackboardCalculated(device_blackboard->Calculated());
    device_blackboard->ReadComputerSettings(GetComputerSettings());
  }

  BroadcastCalculatedUpdate();
}

void
XCSoarInterface::ExchangeBlackboard()
{
  ExchangeDeviceBlackboard();
  SendGetComputerSettings();
  SendMapSettings();
}

void
XCSoarInterface::ExchangeDeviceBlackboard()
{
  ScopeLock protect(device_blackboard->mutex);

  device_blackboard->ReadComputerSettings(GetComputerSettings());
}

void
ActionInterface::SendGetComputerSettings()
{
  assert(calculation_thread != NULL);

  main_window.SetComputerSettings(GetComputerSettings());

  calculation_thread->SetComputerSettings(GetComputerSettings());
  calculation_thread->SetScreenDistanceMeters(main_window.GetProjection().GetScreenDistanceMeters());
}

void
ActionInterface::SetMacCready(fixed mc, bool to_devices)
{
  /* update interface settings */

  GlidePolar &polar = SetComputerSettings().glide_polar_task;
  polar.SetMC(mc);

  /* update InfoBoxes (that might show the MacCready setting) */

  InfoBoxManager::SetDirty();

  /* send to calculation thread and trigger recalculation */

  if (protected_task_manager != NULL)
    protected_task_manager->SetGlidePolar(polar);

  if (calculation_thread != NULL) {
    calculation_thread->SetComputerSettings(GetComputerSettings());
    calculation_thread->Trigger();
  }

  /* send to external devices */

  if (to_devices)
    device_blackboard->SetMC(mc);
}

void
ActionInterface::SendMapSettings(const bool trigger_draw)
{
  // trigger_draw: asks for an immediate exchange of blackboard data
  // (via ProcessTimer()) rather than waiting for the idle timer every 500ms

  if (trigger_draw) {
    DisplayModes();
    InfoBoxManager::ProcessTimer();
  }

  main_window.SetMapSettings(GetMapSettings());

  if (trigger_draw) {
    main_window.full_redraw();
    BroadcastUISettingsUpdate();
  }

  // TODO: trigger refresh if the settings are changed
}

void
ActionInterface::SignalShutdown(bool force)
{
  force_shutdown = force;
  main_window.close(); // signals close
}

bool
XCSoarInterface::CheckShutdown()
{
  if (force_shutdown)
    return true;

  return MessageBoxX(_("Quit program?"), _T("XCSoar"),
                     MB_YESNO | MB_ICONQUESTION) == IDYES;
}

void
ActionInterface::DisplayModes()
{
  main_window.UpdateGaugeVisibility();
}
