// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigurationDialog.hpp"
#include "Device/Driver/Stratux/Driver.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Error.hpp"
#include "Form/DataField/Enum.hpp"
#include "Language/Language.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"

class StratuxConfigurationWidget final
  : public RowFormWidget {
  enum StratuxWidgets {
    HRANGE,
    VRANGE,
    SAVE,
  };

  WidgetDialog &dialog;
  StratuxDevice &device;

public:
  StratuxConfigurationWidget(const DialogLook &look, WidgetDialog &_dialog,
                             StratuxDevice &_device)
    :RowFormWidget(look), dialog(_dialog), device(_device) {}

  /* virtual methods from Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override {

  AddInteger(_("Horizontal Range [m]"), nullptr, _T("%d m"), _T("%d"), 0, 20000, 1000, true, 0);
  AddInteger(_("Vertical Range [m]"), nullptr, _T("%d m"), _T("%d"), 0, 15000, 1000, true, 0);

      AddButton(_("Save"), [this](){
        bool _changed = false;
        dialog.GetWidget().Save(_changed);
      });
  }
};

/**
 * Request all parameter values from the Stratux Vario.
 */

void
ManageStratuxDialog(Device &_device)
{
  StratuxDevice &device = (StratuxDevice &)_device;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look,
                      _T("Stratux"),
                      new StratuxConfigurationWidget(look, dialog, device));

  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
