// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ManageFlarmDialog.hpp"
#include "FLARM/ConfigWidget.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Widget/RowFormWidget.hpp"
#include "Widget/VScrollWidget.hpp"
#include "UIGlobals.hpp"
#include "Language/Language.hpp"
#include "Operation/MessageOperationEnvironment.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "Operation/Cancelled.hpp"
#include "Device/Driver/FLARM/Device.hpp"
#include "FLARM/Version.hpp"
#include "FLARM/Hardware.hpp"
#include "FLARM/FlightState.hpp"
#include "FLARM/Id.hpp"
#include "Interface.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Screen/Layout.hpp"
#include "Widget/ListWidget.hpp"
#include "Look/DialogLook.hpp"
#include "util/StaticString.hxx"
#include "util/Macros.hpp"
#include "time/Stamp.hpp"

class ManageFLARMWidget final
  : public RowFormWidget {
  enum Controls {
    Setup,
    Reboot,
  };

  FlarmDevice &device;
  const FlarmVersion version;
  FlarmHardware hardware;
  const FlarmFlightState &flight_state;

  StaticString<64> buffer;

  void AddReadOnlyASCII(const TCHAR *label, const char *value) noexcept {
    if (value == nullptr || *value == '\0')
      return;
    buffer.clear();
    buffer.UnsafeAppendASCII(value);
    AddReadOnly(label, NULL, buffer.c_str());
  }

  template<typename... Args>
  void AddReadOnlyFormatted(const TCHAR *label, const TCHAR *help_text,
                            const TCHAR *format, Args&&... args) noexcept {
    buffer.clear();
    buffer.Format(format, args...);
    AddReadOnly(label, help_text, buffer.c_str());
  }

  void AddReadOnlyString(const TCHAR *label, const TCHAR *value) noexcept {
    if (value == nullptr || *value == _T('\0'))
      return;
    buffer = value;
    AddReadOnly(label, NULL, buffer.c_str());
  }

public:
  ManageFLARMWidget(const DialogLook &look, FlarmDevice &_device,
                    const FlarmVersion &version,
                    FlarmHardware &hardware,
                    const FlarmFlightState &_flight_state)
    :RowFormWidget(look), device(_device), version(version), hardware(hardware), flight_state(_flight_state) {}

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

  // Detect device type at the beginning by requesting device information
  device.RequestAllSettings(flarm_config_names, env);

  if (const auto x = device.GetSetting("DEVTYPE"))
    hardware.device_type = *x;

  if (const auto x = device.GetSetting("CAP"))
    hardware.capabilities = *x;

  if (const auto x = device.GetSetting("RADIOID")) {
    if (const char *id = strchr(x->c_str(), ',')) {
      const FlarmId parsed_id = FlarmId::Parse(id + 1, nullptr);
      if (parsed_id.IsDefined())
        hardware.radio_id = parsed_id;
    }
  }

  if (!hardware.device_type.empty() || hardware.radio_id.IsDefined())
    hardware.available.Update(TimeStamp{FloatDuration{1}});

  // Device type is now detected and available for use in the rest of the dialog

  if (hardware.available) {
    if (!hardware.device_type.empty())
      AddReadOnlyASCII(_("Hardware type"), hardware.device_type.c_str());

    if (hardware.radio_id.IsDefined()) {
      char tmp_id[10];
      AddReadOnlyASCII(_("Flarm ID"), hardware.radio_id.Format(tmp_id));
    }
  }

  if (version.available) {
    if (!version.hardware_version.empty())
      AddReadOnlyASCII(_("Hardware version"), version.hardware_version.c_str());

    if (!version.software_version.empty())
      AddReadOnlyASCII(_("Firmware version"), version.software_version.c_str());

    if (!version.obstacle_version.empty())
      AddReadOnlyASCII(_("Obstacle database"), version.obstacle_version.c_str());

    AddReadOnlyFormatted(_("Protocol version"),
                         _("Detected FLARM data port protocol version"),
                         _T("%u"), version.protocol_version);
  }

  if (flight_state.available) {
    switch (flight_state.flight_state) {
    case FlarmFlightState::FlightState::IN_FLIGHT:
      AddReadOnlyString(_("Flight state"), _("In flight"));
      break;
    case FlarmFlightState::FlightState::ON_GROUND:
      AddReadOnlyString(_("Flight state"), _("On ground"));
      break;
    default:
      AddReadOnlyString(_("Flight state"), _("Unknown"));
      break;
    }

    switch (flight_state.recorder_state) {
    case FlarmFlightState::FlightRecorderState::OFF:
      AddReadOnlyString(_("IGC recorder"), _("OFF"));
      break;
    case FlarmFlightState::FlightRecorderState::RECORDING:
      AddReadOnlyString(_("IGC recorder"), _("Recording"));
      break;
    case FlarmFlightState::FlightRecorderState::BAROMETRIC_ONLY:
      AddReadOnlyString(_("IGC recorder"), _("Barometric only"));
      break;
    default:
      AddReadOnlyString(_("IGC recorder"), _("Unknown"));
      break;
    }
  }

  AddButton(_("Setup"), [this](){
    auto config_widget = std::make_unique<FLARMConfigWidget>(GetLook(), device, hardware);
    VScrollWidget scroll_widget(std::move(config_widget), GetLook());
    DefaultWidgetDialog(UIGlobals::GetMainWindow(), GetLook(),
                        _T("FLARM"), scroll_widget);
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

  if (hardware.isPowerFlarm()) {
    AddButton(_("Reset CARP"), [this]() {
      try {
        MessageOperationEnvironment env;
        device.SendResetCARP(env);
      } catch (OperationCancelled) {
      } catch (...) {
        ShowError(std::current_exception(), _("Error"));
      }
    });

    AddButton(_("Start demo scenario"), [this]() {
      PopupOperationEnvironment env;

      struct ScenarioInfo
      {
        unsigned number;
        const TCHAR *description;
      };
      static const ScenarioInfo scenarios[] = {
        {1, _("FLARM traffic collision (30s)")},
        {2, _("ADS-B traffic collision (30s)")},
        {3, _("Mode-S traffic collision (30s)")},
        {4, _("Fixed obstacle collision (30s)")},
        {5, _("Alert Zone (30s)")},
        {6, _("Mixed scenario (30s)")},
      };

      WidgetDialog dialog(WidgetDialog::Full{},
                          UIGlobals::GetMainWindow(),
                          GetLook(),
                          _("Select Demo Scenario"));

      class ScenarioListWidget : public ListWidget
      {
        FlarmDevice &device;
        OperationEnvironment &env;
        WidgetDialog &dialog;
        const ScenarioInfo *scenarios;
        const unsigned scenario_count;
        TextRowRenderer row_renderer;

      public:
        ScenarioListWidget(FlarmDevice &_device,
                           OperationEnvironment &_env,
                           WidgetDialog &_dialog,
                           const ScenarioInfo *_scenarios,
                           unsigned _scenario_count)
            : device(_device), env(_env), dialog(_dialog),
              scenarios(_scenarios), scenario_count(_scenario_count)
        {
        }

        void Prepare(ContainerWindow &parent,
                     const PixelRect &rc) noexcept override
        {
          const DialogLook &look = UIGlobals::GetDialogLook();
          ListControl &list = CreateList(
            parent, look, rc, row_renderer.CalculateLayout(*look.list.font));
          list.SetLength(scenario_count);
          list.SetCursorIndex(0);
        }

        void OnPaintItem(Canvas &canvas,
                         const PixelRect rc,
                         unsigned idx) noexcept override
        {
          row_renderer.DrawTextRow(canvas, rc, scenarios[idx].description);
        }

      private:
        bool SendScenario(unsigned idx) noexcept
        {
          if (idx >= scenario_count)
            return false;

          try {
            device.SendDemoScenario(scenarios[idx].number, env);
            dialog.SetModalResult(mrOK);
            return true;
          } catch (OperationCancelled) {
            // User cancelled, don't show error
            return false;
          } catch (...) {
            ShowError(std::current_exception(), _("Error"));
            return false;
          }
        }

      public:
        void OnActivateItem(unsigned idx) noexcept override
        {
          SendScenario(idx);
        }

        void ActivateSelected() noexcept
        {
          SendScenario(GetList().GetCursorIndex());
        }
      };

      auto widget =
        std::make_unique<ScenarioListWidget>(device, env, dialog, scenarios,
                                             ARRAY_SIZE(scenarios));
      ScenarioListWidget *widget_ptr = widget.get();
      dialog.FinishPreliminary(widget.release());
      dialog.AddButton(_("OK"),
                       [widget_ptr]() { widget_ptr->ActivateSelected(); });
      dialog.AddButton(_("Cancel"), mrCancel);
      dialog.ShowModal();
    });
  }
}

void
ManageFlarmDialog(Device &device,
                  const FlarmVersion &version,
                  FlarmHardware &hardware,
                  const FlarmFlightState &flight_state)
{
  WidgetDialog dialog(WidgetDialog::Auto{},
                      UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _T("FLARM"),
                      new ManageFLARMWidget(UIGlobals::GetDialogLook(),
                                            static_cast<FlarmDevice &>(device),
                                            version,
                                            hardware,
                                            flight_state));
  dialog.AddButton(_("Close"), mrCancel);
  dialog.ShowModal();
}
