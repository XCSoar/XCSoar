// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ConfigurationDialog.hpp"
#include "Device/Driver/Stratux/Driver.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Dialogs/Message.hpp"
#include "Dialogs/Error.hpp"
#include "Language/Language.hpp"
#include "Profile/Profile.hpp"
#include "Operation/Cancelled.hpp"
#include "Operation/PopupOperationEnvironment.hpp"
#include "UIGlobals.hpp"
#include "Widget/RowFormWidget.hpp"

class StratuxConfigurationWidget final
  : public RowFormWidget {

  enum StratuxWidgets {
    HRANGE,
    VRANGE,
  };

  WidgetDialog &dialog [[maybe_unused]];
  StratuxDevice &device [[maybe_unused]];
  StratuxDevice::StratuxSettings settings;

public:
  StratuxConfigurationWidget(const DialogLook &look, WidgetDialog &_dialog,
                             StratuxDevice &_device)
    :RowFormWidget(look), dialog(_dialog), device(_device) {}

  /* virtual methods from Widget */
  void Prepare([[maybe_unused]] ContainerWindow &parent, [[maybe_unused]] const PixelRect &rc) noexcept override {

    LoadFromProfile(settings);

    AddInteger(_("Horizontal Range"), nullptr, "%d m", "%d", 4000, 20000, 1000, settings.hrange);
    AddInteger(_("Vertical Range"), nullptr, "%d m", "%d", 1000, 4000, 1000, settings.vrange);
  }

  bool Save(bool &_changed) noexcept override {
    PopupOperationEnvironment env;
    bool changed = false;

    changed |= SaveValueInteger(HRANGE, settings.hrange);
    changed |= SaveValueInteger(VRANGE, settings.vrange);

    SaveToProfile(settings);
    Profile::Save();

    _changed |= changed;
    if (_changed) ShowMessageBox(_("Changes to configuration saved. Restart XCSoar to apply changes."),
                    "", MB_OK);
    return true;
  }
};

void
ManageStratuxDialog(Device &_device)
{
  StratuxDevice &device = (StratuxDevice &)_device;

  const DialogLook &look = UIGlobals::GetDialogLook();

  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      look,
                      "Stratux Setup",
                      new StratuxConfigurationWidget(look, dialog, device));

  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}
