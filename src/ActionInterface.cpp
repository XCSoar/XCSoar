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

#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "Thread/Mutex.hpp"
#include "MainWindow.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Language/Language.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "Components.hpp"
#include "FLARM/Glue.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "CalculationThread.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Profile/Profile.hpp"
#include "UIState.hpp"
#include "Operation/MessageOperationEnvironment.hpp"

using namespace CommonInterface;

namespace ActionInterface {
  static void SendGetComputerSettings();
}

void
XCSoarInterface::ReceiveGPS()
{
  {
    ScopeLock protect(device_blackboard->mutex);

    ReadBlackboardBasic(device_blackboard->Basic());

    const NMEAInfo &real = device_blackboard->RealState();
    Private::movement_detected = real.alive && real.gps.real &&
      real.MovementDetected();
  }

  BroadcastGPSUpdate();

  if (!Basic().flarm.traffic.IsEmpty())
    /* auto-load FlarmNet when traffic is seen */
    LoadFlarmDatabases();
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
  ActionInterface::SendGetComputerSettings();
  ActionInterface::SendMapSettings();
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
  assert(calculation_thread != nullptr);

  main_window->SetComputerSettings(GetComputerSettings());

  calculation_thread->SetComputerSettings(GetComputerSettings());
  calculation_thread->SetScreenDistanceMeters(main_window->GetProjection().GetScreenDistanceMeters());
}

void
ActionInterface::SetBallast(double ballast, bool to_devices)
{
  // write ballast into settings
  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
  polar.SetBallast(ballast);

  // send to calculation thread and trigger recalculation
  if (protected_task_manager != nullptr)
    protected_task_manager->SetGlidePolar(polar);

  if (calculation_thread != nullptr) {
    calculation_thread->SetComputerSettings(GetComputerSettings());
    calculation_thread->ForceTrigger();
  }

  // send to external devices
  if (to_devices) {
    const Plane &plane = GetComputerSettings().plane;
    if (plane.dry_mass > 0) {
      auto overload = (plane.dry_mass + ballast * plane.max_ballast) /
        plane.dry_mass;

      MessageOperationEnvironment env;
      device_blackboard->SetBallast(ballast, overload, env);
    }
  }
}

void
ActionInterface::SetBugs(double bugs, bool to_devices)
{
  // Write Bugs into settings
  CommonInterface::SetComputerSettings().polar.SetBugs(bugs);
  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;

  // send to calculation thread and trigger recalculation
  if (protected_task_manager != nullptr)
    protected_task_manager->SetGlidePolar(polar);

  if (calculation_thread != nullptr) {
    calculation_thread->SetComputerSettings(GetComputerSettings());
    calculation_thread->ForceTrigger();
  }

  // send to external devices
  if (to_devices) {
    MessageOperationEnvironment env;
    device_blackboard->SetBugs(bugs, env);
  }
}

void
ActionInterface::SetMacCready(double mc, bool to_devices)
{
  // Repeated adjustment of MC with the +/- UI elements could result in
  // an MC which is slightly larger than 0. Since the calculations
  // fundamentally change depending on  "MC == 0" or "MC <> 0" force
  // a 0 for small MC values.
  if (mc < 0.01)
    mc = 0;

  /* update interface settings */

  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
  polar.SetMC(mc);

  /* update InfoBoxes (that might show the MacCready setting) */

  InfoBoxManager::SetDirty();

  /* send to calculation thread and trigger recalculation */

  if (protected_task_manager != nullptr)
    protected_task_manager->SetGlidePolar(polar);

  if (calculation_thread != nullptr) {
    calculation_thread->SetComputerSettings(GetComputerSettings());
    calculation_thread->ForceTrigger();
  }

  /* send to external devices */

  if (to_devices) {
    MessageOperationEnvironment env;
    device_blackboard->SetMC(mc, env);
  }
}

void ActionInterface::SetManualMacCready(double mc, bool to_devices)
{
  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  if (task_behaviour.auto_mc) {
    task_behaviour.auto_mc = false;
    Profile::Set(ProfileKeys::AutoMc, false);
  }

  SetMacCready(mc, to_devices);
}

void
ActionInterface::OffsetManualMacCready(double offset, bool to_devices)
{
  const GlidePolar &polar = GetComputerSettings().polar.glide_polar_task;
  const auto old_mc = polar.GetMC();
  auto mc = old_mc + offset;
  if (mc < 0)
    mc = 0;
  else if (mc > 5)
    mc = 5;

  if (mc != old_mc)
    SetManualMacCready(mc, to_devices);
}

void
ActionInterface::SendMapSettings(const bool trigger_draw)
{
  if (trigger_draw) {
    main_window->UpdateGaugeVisibility();
    InfoBoxManager::ProcessTimer();
  }

  /* Don't show indicator when the gauge is indicating the traffic anyway */
  SetMapSettings().show_flarm_alarm_level =
    !GetUISettings().traffic.enable_gauge;

  main_window->SetMapSettings(GetMapSettings());

  if (trigger_draw) {
    main_window->FullRedraw();
    BroadcastUISettingsUpdate();
  }

  // TODO: trigger refresh if the settings are changed
}

void
ActionInterface::SendUIState(const bool trigger_draw)
{
  main_window->SetUIState(GetUIState());

  if (trigger_draw)
    main_window->FullRedraw();
}

gcc_pure
static unsigned
GetPanelIndex(const UIState &ui_state)
{
  if (ui_state.auxiliary_enabled) {
    unsigned panel = ui_state.auxiliary_index;
    if (panel >= InfoBoxSettings::MAX_PANELS)
      panel = InfoBoxSettings::PANEL_AUXILIARY;
    return panel;
  }
  else if (ui_state.display_mode == DisplayMode::CIRCLING)
    return InfoBoxSettings::PANEL_CIRCLING;
  else if (ui_state.display_mode == DisplayMode::FINAL_GLIDE)
    return InfoBoxSettings::PANEL_FINAL_GLIDE;
  else
    return InfoBoxSettings::PANEL_CRUISE;
}

void
ActionInterface::UpdateDisplayMode()
{
  UIState &state = SetUIState();
  const UISettings &settings = GetUISettings();

  state.display_mode = GetNewDisplayMode(settings.info_boxes, state,
                                         Calculated());
  state.panel_index = GetPanelIndex(state);

  const auto &panel = settings.info_boxes.panels[state.panel_index];
  state.panel_name = gettext(panel.name);
}

void
ActionInterface::SendUIState()
{
  /* force-update all InfoBoxes just in case the display mode has
     changed */
  InfoBoxManager::SetDirty();
  InfoBoxManager::ProcessTimer();

  main_window->SetUIState(GetUIState());
}
