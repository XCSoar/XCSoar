// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/Dialogs.h"
#include "Dialogs/WidgetDialog.hpp"
#include "WindSettingsPanel.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

void
ShowWindSettingsDialog()
{
  WindSettingsPanel *panel = new WindSettingsPanel(true, false, true);
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _("Wind Settings"), panel);
  panel->SetClearManualButton(dialog.AddButton(_("Clear"), [panel](){
    panel->ClearManual();
  }));

  dialog.AddButton(_("Close"), mrOK);
  dialog.ShowModal();
}
