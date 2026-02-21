// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageFlarmDialog.hpp"
#include "FLARM/ConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/ComboPicker.hpp"
#include "Dialogs/Error.hpp"
#include "Form/DataField/ComboList.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/State.hpp"

class ManageFLARMWidget final
  : public RowFormWidget {
  enum Controls {
    Setup,
    Reboot,
  };

  FlarmDevice &device;
  const FlarmVersion version;
  FlarmHardware hardware;
  const FlarmState state;

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device,
                    const FlarmVersion &version,
                    FlarmHardware &hardware,
                    const FlarmState &_state)
    :RowFormWidget(look), device(_device),
     version(version), hardware(hardware), state(_state) {}

  /* virtual methods from Widget */
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
  PopupOperationEnvironment env;
  if(device.RequestAllSettings(flarm_config_names, env)) {
    if (const auto x = device.GetSetting("DEVTYPE"))
      hardware.device_type = *x;

    if (const auto x = device.GetSetting("CAP"))
      hardware.capabilities = *x;

    if (const auto x = device.GetSetting("RADIOID")) {
      if (const char *id = strchr(x->c_str(), ',')) {
        hardware.radio_id = FlarmId::Parse(id + 1, nullptr);
      }
    }

    hardware.available.Update(TimeStamp{FloatDuration{1}});
  }

  if (hardware.available) {
    StaticString<64> buffer;

    if (!hardware.device_type.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(hardware.device_type.c_str());
      AddReadOnly(_("Hardware type"), NULL, buffer.c_str());
    }

    if (hardware.radio_id.IsDefined()) {
      char tmp_id[10];    
      buffer.clear();
      buffer.UnsafeAppendASCII(hardware.radio_id.Format(tmp_id));
      AddReadOnly(_("Flarm ID"), NULL, buffer.c_str());
    }
  }

  if (version.available) {
    StaticString<64> buffer;

    if (!version.hardware_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.hardware_version.c_str());
      AddReadOnly(_("Hardware version"), NULL, buffer.c_str());
    }

    if (!version.software_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.software_version.c_str());
      AddReadOnly(_("Firmware version"), NULL, buffer.c_str());
    }

    if (!version.obstacle_version.empty()) {
      buffer.clear();
      buffer.UnsafeAppendASCII(version.obstacle_version.c_str());
      AddReadOnly(_("Obstacle database"), NULL, buffer.c_str());
    }
  }

  if (state.available) {
    AddReadOnly(_("Flight state"), nullptr,
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
    AddReadOnly(_("IGC recorder"), nullptr, recorder_text);
  }

  AddButton(_("Setup"), [this](){
    FLARMConfigWidget widget(GetLook(), device, hardware);
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        "FLARM", widget);
  });

  AddButton(_("Reboot"), [this](){
    try {
      MessageOperationEnvironment env;
      device.Restart(env);
    } catch (OperationCancelled) {
    } catch (...) {
      ShowError(std::current_exception(), _("Error"));
    }
  });

  AddButton(_("Simulation"), [this](){
    ComboList list;
    list.Append(1, "1",
                _("1 - FLARM collision"),
                _("FLARM aircraft on collision course, "
                  "all alarm levels. 30s."));
    list.Append(2, "2",
                _("2 - ADS-B collision"),
                _("ADS-B aircraft on collision course, "
                  "all alarm levels. 30s."));
    list.Append(3, "3",
                _("3 - Mode-S collision"),
                _("Non-directional Mode-S aircraft on "
                  "collision course. 30s."));
    list.Append(4, "4",
                _("4 - Obstacle"),
                _("Fixed obstacle from database, "
                  "all alarm levels. 30s."));
    list.Append(5, "5",
                _("5 - Alert Zone"),
                _("Alert Zone fly-through (e.g. active "
                  "skydiving area). 30s."));
    list.Append(6, "6",
                _("6 - Mixed"),
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
  });
}

void
ManageFlarmDialog(Device &device, const FlarmVersion &version,
                  FlarmHardware &hardware, const FlarmState &state)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      "FLARM",
                      new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                            (FlarmDevice &)device,
                                            version, hardware, state));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
