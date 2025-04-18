// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Airspace.hpp"
#include "AirspaceCRendererSettingsPanel.hpp"
#include "../WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

bool
ShowAirspaceClassRendererSettingsDialog(AirspaceClass selected)
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _("Airspace Renderer Settings"),
                      new AirspaceClassRendererSettingsPanel(selected));
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
  return dialog.GetChanged();
}
