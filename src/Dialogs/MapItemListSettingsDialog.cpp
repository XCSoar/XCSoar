// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapItemListDialog.hpp"
#include "MapItemListSettingsPanel.hpp"
#include "WidgetDialog.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"

void
ShowMapItemListSettingsDialog()
{
  WidgetDialog dialog(WidgetDialog::Auto{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(),
                      _("Map Item List Settings"),
                      new MapItemListSettingsPanel());
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.ShowModal();
}
