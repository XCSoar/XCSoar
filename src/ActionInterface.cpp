// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ActionInterface.hpp"
#include "Interface.hpp"
#include "thread/Mutex.hxx"
#include "MainWindow.hpp"
#include "Projection/MapWindowProjection.hpp"
#include "Language/Language.hpp"
#include "InfoBoxes/InfoBoxManager.hpp"
#include "FLARM/Glue.hpp"
#include "Device/MultipleDevices.hpp"
#include "Blackboard/DeviceBlackboard.hpp"
#include "CalculationThread.hpp"
#include "Task/ProtectedTaskManager.hpp"
#include "Profile/Profile.hpp"
#include "UIState.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"

using namespace CommonInterface;

namespace ActionInterface {
static void
SendGetComputerSettings() noexcept;
}

void
XCSoarInterface::ReceiveGPS() noexcept
{
  {
    auto &device_blackboard = *backend_components->device_blackboard;
    const std::lock_guard lock{device_blackboard.mutex};

    ReadBlackboardBasic(device_blackboard.Basic());

    const NMEAInfo &real = device_blackboard.RealState();
    Private::movement_detected = real.alive && real.gps.real &&
      real.MovementDetected();
  }

  BroadcastGPSUpdate();

  if (!Basic().flarm.traffic.IsEmpty())
    /* auto-load FlarmNet when traffic is seen */
    LoadFlarmDatabases();
}

void
XCSoarInterface::ReceiveCalculated() noexcept
{
  {
    auto &device_blackboard = *backend_components->device_blackboard;
    const std::lock_guard lock{device_blackboard.mutex};

    ReadBlackboardCalculated(device_blackboard.Calculated());
    device_blackboard.ReadComputerSettings(GetComputerSettings());
  }

  BroadcastCalculatedUpdate();
}

void
XCSoarInterface::ExchangeBlackboard() noexcept
{
  ExchangeDeviceBlackboard();
  ActionInterface::SendGetComputerSettings();
  ActionInterface::SendMapSettings();
}

void
XCSoarInterface::ExchangeDeviceBlackboard() noexcept
{
  auto &device_blackboard = *backend_components->device_blackboard;
  const std::lock_guard lock{device_blackboard.mutex};
  device_blackboard.ReadComputerSettings(GetComputerSettings());
}

void
ActionInterface::SendGetComputerSettings() noexcept
{
  assert(backend_components->calculation_thread != nullptr);

  main_window->SetComputerSettings(GetComputerSettings());

  backend_components->calculation_thread->SetComputerSettings(GetComputerSettings());
  backend_components->calculation_thread->SetScreenDistanceMeters(main_window->GetProjection().GetScreenDistanceMeters());
}

void
ActionInterface::SetBallast(double ballast, bool to_devices) noexcept
{
  // write ballast into settings
  GlidePolar &polar = SetComputerSettings().polar.glide_polar_task;
  polar.SetBallast(ballast);

  // send to calculation thread and trigger recalculation
  backend_components->SetTaskPolar(GetComputerSettings().polar);

  // send to external devices
  if (to_devices && backend_components->devices) {
    const Plane &plane = GetComputerSettings().plane;
    if (plane.empty_mass > 0) {
      auto dry_mass = plane.empty_mass + polar.GetCrewMass();
      auto overload = (dry_mass + ballast * plane.max_ballast) /
                      plane.polar_shape.reference_mass;

      MessageOperationEnvironment env;
      backend_components->devices->PutBallast(ballast, overload, env);
    }
  }
}

void
ActionInterface::SetBugs(double bugs, bool to_devices) noexcept
{
  // Write Bugs into settings
  CommonInterface::SetComputerSettings().polar.SetBugs(bugs);

  // send to calculation thread and trigger recalculation
  backend_components->SetTaskPolar(GetComputerSettings().polar);

  // send to external devices
  if (to_devices && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutBugs(bugs, env);
  }
}

void
ActionInterface::SetMacCready(double mc, bool to_devices) noexcept
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
  backend_components->SetTaskPolar(GetComputerSettings().polar);

  /* send to external devices */

  if (to_devices && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutMacCready(mc, env);
  }
}

void ActionInterface::SetManualMacCready(double mc, bool to_devices) noexcept
{
  TaskBehaviour &task_behaviour = CommonInterface::SetComputerSettings().task;
  if (task_behaviour.auto_mc) {
    task_behaviour.auto_mc = false;
    Profile::Set(ProfileKeys::AutoMc, false);
  }

  SetMacCready(mc, to_devices);
}

void
ActionInterface::OffsetManualMacCready(double offset, bool to_devices) noexcept
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
ActionInterface::SendMapSettings(const bool trigger_draw) noexcept
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
ActionInterface::SendUIState(const bool trigger_draw) noexcept
{
  main_window->SetUIState(GetUIState());

  if (trigger_draw)
    main_window->FullRedraw();
}

[[gnu::pure]]
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
ActionInterface::UpdateDisplayMode() noexcept
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
ActionInterface::SendUIState() noexcept
{
  /* force-update all InfoBoxes just in case the display mode has
     changed */
  InfoBoxManager::SetDirty();
  InfoBoxManager::ProcessTimer();

  main_window->SetUIState(GetUIState());
}

void
ActionInterface::SetActiveFrequency(const RadioFrequency freq,
                                    const TCHAR *freq_name,
                                    bool to_devices) noexcept
{
  assert(freq.IsDefined());

  /* update interface settings */

  SetComputerSettings().radio.active_frequency = freq;
  if(freq_name != nullptr) {
    SetComputerSettings().radio.active_name = freq_name;
  }
  else {
    SetComputerSettings().radio.active_name.clear();
  }

  /* update InfoBoxes (that might show the ActiveFrequency setting) */

  InfoBoxManager::SetDirty();

  /* send to external devices */

  if (to_devices && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutActiveFrequency(freq, freq_name, env);
  }
}

void
ActionInterface::SetStandbyFrequency(const RadioFrequency freq,
                                     const TCHAR *freq_name,
                                     bool to_devices) noexcept
{
  assert(freq.IsDefined());

  /* update interface settings */

  SetComputerSettings().radio.standby_frequency = freq;
  if(freq_name != nullptr) {
    SetComputerSettings().radio.standby_name = freq_name;
  }
  else {
    SetComputerSettings().radio.standby_name.clear();
  }

  /* update InfoBoxes (that might show the ActiveFrequency setting) */

  InfoBoxManager::SetDirty();

  /* send to external devices */

  if (to_devices && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutStandbyFrequency(freq, freq_name, env);
  }
}

void
ActionInterface::OffsetActiveFrequency(double offset_khz,
                                       bool to_devices) noexcept
{
  RadioFrequency new_active_freq = SetComputerSettings().radio.active_frequency;
  if(new_active_freq.IsDefined()) {
    new_active_freq.OffsetKiloHertz(offset_khz);
    if(new_active_freq.IsDefined()) {
      ActionInterface::SetActiveFrequency(new_active_freq, nullptr, to_devices);
    }
  }
}

void
ActionInterface::OffsetStandbyFrequency(double offset_khz,
                                        bool to_devices) noexcept
{
  RadioFrequency new_standby_freq = SetComputerSettings().radio.standby_frequency;
  if(new_standby_freq.IsDefined()) {
    new_standby_freq.OffsetKiloHertz(offset_khz);
    if(new_standby_freq.IsDefined()) {
      ActionInterface::SetStandbyFrequency(new_standby_freq, nullptr, to_devices);
    }
  }
}

void
ActionInterface::ExchangeRadioFrequencies(bool to_devices) noexcept
{
  const auto radio_settings = SetComputerSettings().radio;

  if(radio_settings.active_frequency.IsDefined() &&
     radio_settings.standby_frequency.IsDefined()) {
    const auto old_active_freq = radio_settings.active_frequency;
    const auto old_active_freq_name = radio_settings.active_name;

    ActionInterface::SetActiveFrequency(radio_settings.standby_frequency, radio_settings.standby_name, to_devices);
    ActionInterface::SetStandbyFrequency(old_active_freq, old_active_freq_name, to_devices);
  }
}

void
ActionInterface::SetTransponderCode(TransponderCode code, bool to_devices) noexcept
{
  assert(code.IsDefined());

  /* update interface settings */
  SetComputerSettings().transponder.transponder_code = code;

  /* update InfoBoxes (that might show the code setting) */
  InfoBoxManager::SetDirty();

  /* send to external devices */
  if (to_devices && backend_components->devices) {
    MessageOperationEnvironment env;
    backend_components->devices->PutTransponderCode(code, env);
  }
}
