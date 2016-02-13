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

#include "BlueFlyDialogs.hpp"
#include "Device/Driver/BlueFly/Internal.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"

class BlueFlyConfigurationWidget final
  : public RowFormWidget, private ActionListener {
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
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override {

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

      AddButton(_("Save"), *this, SAVE);
  }

  void Show(const PixelRect &rc) override {
    device.GetSettings(params);

    LoadValue(VOLUME, params.volume);
    LoadValueEnum(OUTPUT_MODE, params.output_mode);

    RowFormWidget::Show(rc);
  }

  bool Save(bool &changed) override {
    PopupOperationEnvironment env;

    changed |= SaveValue(VOLUME, params.volume);
    changed |= SaveValue(OUTPUT_MODE, params.output_mode);

    device.WriteDeviceSettings(params, env);

    return true;
  }

private:
  /* virtual methods from ActionListener */
  void OnAction(int id) override {
    bool _changed = false;

    switch (id) {
    case SAVE:
      dialog.GetWidget().Save(_changed);
      break;
    }
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

  WidgetDialog dialog(look);

  dialog.CreateAuto(UIGlobals::GetMainWindow(), _T("BlueFly Vario"),
                    new BlueFlyConfigurationWidget(look, dialog, device));

  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
