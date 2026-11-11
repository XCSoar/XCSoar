// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageFlarmDialog.hpp"
#include "FLARM/ConfigWidget.hpp"
#include "FLARM/RangeStatisticsDialog.hpp"
#include "Components.hpp"
#include "BackendComponents.hpp"
#include "Computer/GlideComputer.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/Error.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Widget/GroupedListWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/State.hpp"
#include "util/StaticString.hxx"

#include <cstring>
#include <string>

class ManageFLARMWidget final : public GroupedListWidget {
  FlarmDevice &device;
  const FlarmVersion version;
  FlarmHardware hardware;
  const FlarmState state;

  void AddFact(const char *caption, const char *value) noexcept {
    if (value == nullptr || *value == '\0')
      return;

    AddItem(caption, {.value = value});
  }

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device,
                    const FlarmVersion &version,
                    FlarmHardware &hardware,
                    const FlarmState &_state) noexcept
    :GroupedListWidget(look), device(_device),
     version(version), hardware(hardware), state(_state) {}

  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
};

static const char *const flarm_config_names[] = {
  "DEVTYPE",
  "CAP",
  "RADIOID",
  NULL
};

void
ManageFLARMWidget::Prepare([[maybe_unused]] ContainerWindow &parent,
                           [[maybe_unused]] const PixelRect &rc) noexcept
{
  /* the caller passes an uninitialised block; a query which gets no
     answer, as in the simulator, must not be shown as a device */
  hardware = {};

  PopupOperationEnvironment env;
  if (device.RequestAllSettings(flarm_config_names, env)) {
    if (const auto x = device.GetSetting("DEVTYPE"))
      hardware.device_type = *x;

    if (const auto x = device.GetSetting("CAP"))
      hardware.capabilities = *x;

    if (const auto x = device.GetSetting("RADIOID")) {
      if (const char *id = strchr(x->c_str(), ','))
        hardware.radio_id = FlarmId::Parse(id + 1, nullptr);
    }

    hardware.available.Update(TimeStamp{FloatDuration{1}});
  }

  AddGroup();

  if (hardware.available) {
    if (!hardware.device_type.empty()) {
      StaticString<64> buffer;
      buffer.clear();
      buffer.UnsafeAppendASCII(hardware.device_type.c_str());
      AddFact(_("Hardware type"), buffer.c_str());
    }

    if (hardware.radio_id.IsDefined()) {
      char tmp_id[10];
      StaticString<64> buffer;
      buffer.clear();
      buffer.UnsafeAppendASCII(hardware.radio_id.Format(tmp_id));
      AddFact(_("Flarm ID"), buffer.c_str());
    }
  }

  if (version.available) {
    if (!version.hardware_version.empty()) {
      StaticString<64> buffer;
      buffer.clear();
      buffer.UnsafeAppendASCII(version.hardware_version.c_str());
      AddFact(_("Hardware version"), buffer.c_str());
    }

    if (!version.software_version.empty()) {
      StaticString<64> buffer;
      buffer.clear();
      buffer.UnsafeAppendASCII(version.software_version.c_str());
      AddFact(_("Firmware version"), buffer.c_str());
    }

    if (!version.obstacle_version.empty()) {
      StaticString<64> buffer;
      buffer.clear();
      buffer.UnsafeAppendASCII(version.obstacle_version.c_str());
      AddFact(_("Obstacle database"), buffer.c_str());
    }
  }

  if (state.available) {
    AddFact(_("Flight state"),
            state.flight == FlarmState::Flight::IN_FLIGHT
            ? _("In flight") : _("On ground"));

    const char *recorder_text;
    switch (state.recorder) {
    case FlarmState::Recorder::RECORDING:
      recorder_text = _("Recording");
      break;
    case FlarmState::Recorder::BARO_ONLY:
      recorder_text = _("Baro only");
      break;
    default:
      recorder_text = _("Off");
      break;
    }
    AddFact(_("IGC recorder"), recorder_text);
  }

  AddItem(C_("Button", "Setup"), [this](){
    const DialogLook &look = UIGlobals::GetDialogLook();
    FLARMConfigWidget widget(look, device, hardware);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), look, "FLARM", widget);
  }, {.chevron = true});

  /* a PowerFLARM keeps statistics of its own; for any other FLARM,
     XCSoar estimates the range from the traffic it reports */
  AddItem(_("Range statistics"), [this](){
    if (hardware.isPowerFlarm())
      FlarmRangeStatisticsDialog(device);
    else if (const auto &glide_computer = backend_components->glide_computer)
      FlarmRangeEstimateDialog(device, glide_computer->GetFlarmRange());
  }, {.chevron = true});

  AddItem(_("Simulation"), [this](){
    ComboList list;
    const auto sim1_label = std::to_string(1) + " - " + _("FLARM collision");
    list.Append(1, "1",
                sim1_label.c_str(),
                _("FLARM aircraft on collision course, "
                  "all alarm levels. 30s."));
    const auto sim2_label = std::to_string(2) + " - " + _("ADS-B collision");
    list.Append(2, "2",
                sim2_label.c_str(),
                _("ADS-B aircraft on collision course, "
                  "all alarm levels. 30s."));
    const auto sim3_label = std::to_string(3) + " - " + _("Mode-S collision");
    list.Append(3, "3",
                sim3_label.c_str(),
                _("Non-directional Mode-S aircraft on "
                  "collision course. 30s."));
    const auto sim4_label = std::to_string(4) + " - " + _("Obstacle");
    list.Append(4, "4",
                sim4_label.c_str(),
                _("Fixed obstacle from database, "
                  "all alarm levels. 30s."));
    const auto sim5_label = std::to_string(5) + " - " + _("Alert Zone");
    list.Append(5, "5",
                sim5_label.c_str(),
                _("Alert Zone fly-through (e.g. active "
                  "skydiving area). 30s."));
    const auto sim6_label = std::to_string(6) + " - " + _("Mixed");
    list.Append(6, "6",
                sim6_label.c_str(),
                _("Multiple traffic types and an alert zone, "
                  "no warnings. 30s."));

    int result = ComboPicker(_("Simulation scenario"), list,
                             nullptr, true);
    if (result < 0)
      return;

    unsigned scenario = list[result].int_value;
    try {
      MessageOperationEnvironment env;
      device.RunSimulation(scenario, env);
    } catch (OperationCancelled) {
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  }, {.chevron = true});

  AddButton(_("Reboot"), [this](){
    try {
      MessageOperationEnvironment env;
      device.Restart(env);
    } catch (OperationCancelled) {
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  });

  GroupedListWidget::Prepare(parent, rc);
}

void
ManageFlarmDialog(Device &device, const FlarmVersion &version,
                  FlarmHardware &hardware, const FlarmState &state)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  auto *widget = new ManageFLARMWidget(look, (FlarmDevice &)device,
                                       version, hardware, state);

  WidgetDialog dialog(WidgetDialog::Floating{}, UIGlobals::GetMainWindow(),
                      look, "FLARM", widget);
  dialog.AddButton(_("Close"), mrCancel);
  dialog.PrepareFloatingList();
  dialog.RefitList();
  dialog.ShowModal();
}
