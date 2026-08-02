// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BlueFlyDialogs.hpp"
#include "Device/Driver/BlueFly/Internal.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"

class BlueFlyConfigurationWidget final
  : public RowFormWidget {
  enum BlueFlyWidgets {
    VOLUME,
    AUDIO_WHEN_CONNECTED,
    LIFT_THRESHOLD,
    LIFT_OFF_THRESHOLD,
    SINK_THRESHOLD,
    SINK_OFF_THRESHOLD,
    OUTPUT_MODE,
    OUTPUT_FREQUENCY,
    SAVE,
  };

  WidgetDialog &dialog;
  BlueFlyDevice &device;
  BlueFlyDevice::BlueFlySettings params;

public:
  BlueFlyConfigurationWidget(const DialogLook &look, WidgetDialog &_dialog,
                             BlueFlyDevice &_device)
    :RowFormWidget(look), dialog(_dialog), device(_device) {}

  /* virtual methods from Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override {

      AddFloat(N_("Volume"), nullptr,
               "%.2f", "%.2f",
               0, 1.0, 0.1, true, 0);

      AddBoolean(N_("Audio when connected"), nullptr, false);

      AddFloat(NC_("Setting", "Lift threshold"), nullptr,
               "%.2f m/s", "%.2f",
               0, BlueFlyDevice::BlueFlySettings::THRESHOLD_MAX,
               0.05, true, 0.2);

      AddFloat(NC_("Setting", "Lift off threshold"), nullptr,
               "%.2f m/s", "%.2f",
               0, BlueFlyDevice::BlueFlySettings::THRESHOLD_MAX,
               0.05, true, 0.05);

      AddFloat(NC_("Setting", "Sink threshold"), nullptr,
               "%.2f m/s", "%.2f",
               0, BlueFlyDevice::BlueFlySettings::THRESHOLD_MAX,
               0.05, true, 0.2);

      AddFloat(NC_("Setting", "Sink off threshold"), nullptr,
               "%.2f m/s", "%.2f",
               0, BlueFlyDevice::BlueFlySettings::THRESHOLD_MAX,
               0.05, true, 0.05);

      static constexpr StaticEnumChoice modes[] = {
        { 0, "BlueFlyVario" },
        { 1, "LK8EX1" },
        { 2, "LX" },
        { 3, "FlyNet" },
        { 4, "None" },
        { 5, "BFV" },
        { 6, "BFX" },
        { 0 }
      };

      AddEnum(N_("Output mode"), nullptr, modes);

      AddInteger(NC_("Setting", "Output frequency"),
                 _("Divisor of the 20 ms hardware tick (1 = 50 Hz, 10 = 5 Hz)."),
                 "%d", "%d",
                 BlueFlyDevice::BlueFlySettings::OUTPUT_FREQUENCY_MIN,
                 BlueFlyDevice::BlueFlySettings::OUTPUT_FREQUENCY_MAX,
                 1, 1);

      AddButton(_("Save"), [this](){
        bool _changed = false;
        dialog.GetWidget().Save(_changed);
      });
  }

  void Show(const PixelRect &rc) noexcept override {
    params = device.GetSettings();

    LoadValue(VOLUME, params.volume);
    LoadValue(AUDIO_WHEN_CONNECTED, params.audio_when_connected);
    LoadValue(LIFT_THRESHOLD, params.lift_threshold);
    LoadValue(LIFT_OFF_THRESHOLD, params.lift_off_threshold);
    LoadValue(SINK_THRESHOLD, params.sink_threshold);
    LoadValue(SINK_OFF_THRESHOLD, params.sink_off_threshold);
    LoadValueEnum(OUTPUT_MODE, params.output_mode);
    params.output_frequency =
      BlueFlyDevice::BlueFlySettings::ExportOutputFrequency(
        params.output_frequency);
    LoadValue(OUTPUT_FREQUENCY, params.output_frequency);

    RowFormWidget::Show(rc);
  }

  bool Save(bool &changed) noexcept override {
    PopupOperationEnvironment env;

    changed |= SaveValue(VOLUME, params.volume);
    changed |= SaveValue(AUDIO_WHEN_CONNECTED,
                         params.audio_when_connected);
    changed |= SaveValue(LIFT_THRESHOLD, params.lift_threshold);
    changed |= SaveValue(LIFT_OFF_THRESHOLD, params.lift_off_threshold);
    changed |= SaveValue(SINK_THRESHOLD, params.sink_threshold);
    changed |= SaveValue(SINK_OFF_THRESHOLD, params.sink_off_threshold);
    changed |= SaveValueEnum(OUTPUT_MODE, params.output_mode);
    changed |= SaveValueInteger(OUTPUT_FREQUENCY,
                                params.output_frequency);

    try {
      device.WriteDeviceSettings(params, env);
    } catch (OperationCancelled) {
      return false;
    } catch (...) {
      ShowError(std::current_exception(), "BlueFly Vario");
      return false;
    }

    return true;
  }
};

/**
 * Request all parameter values from the BlueFly Vario.
 */
static void
RequestAll(BlueFlyDevice &device)
{
  PopupOperationEnvironment env;
  int retry = 3;

  while (retry--) {
    device.RequestSettings(env);
    if (device.WaitForSettings(500))
      break;
  }
}

void
dlgConfigurationBlueFlyVarioShowModal(Device &_device)
{
  BlueFlyDevice &device = (BlueFlyDevice &)_device;

  RequestAll(device);

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look,
                      "BlueFly Vario",
                      new BlueFlyConfigurationWidget(look, dialog, device));

  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
