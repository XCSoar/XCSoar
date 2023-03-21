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
    OUTPUT_MODE,
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
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override {

      AddFloat(N_("Volume"), nullptr,
             _T("%.2f"),
             _T("%.2f"),
               0, 1.0, 0.1, true, 0);

      static constexpr StaticEnumChoice modes[] = {
        { 0, _T("BlueFlyVario") },
        { 1, _T("LK8EX1") },
        { 2, _T("LX") },
        { 3, _T("FlyNet") },
        { 0 }
      };

      AddEnum(N_("Output mode"), nullptr, modes);

      AddButton(_("Save"), [this](){
        bool _changed = false;
        dialog.GetWidget().Save(_changed);
      });
  }

  void Show(const PixelRect &rc) noexcept override {
    params = device.GetSettings();

    LoadValue(VOLUME, params.volume);
    LoadValueEnum(OUTPUT_MODE, params.output_mode);

    RowFormWidget::Show(rc);
  }

  bool Save(bool &changed) noexcept override {
    PopupOperationEnvironment env;

    changed |= SaveValue(VOLUME, params.volume);
    changed |= SaveValueEnum(OUTPUT_MODE, params.output_mode);

    try {
      device.WriteDeviceSettings(params, env);
    } catch (OperationCancelled) {
      return false;
    } catch (...) {
      ShowError(std::current_exception(), _T("BlueFly Vario"));
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
                      _T("BlueFly Vario"),
                      new BlueFlyConfigurationWidget(look, dialog, device));

  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
