// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/DataManagement.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/FileManager.hpp"
#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/Message.hpp"

class DataManagementWidget : public RowFormWidget {
public:
  explicit DataManagementWidget(const DialogLook &look) noexcept
    :RowFormWidget(look) {}

  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override {
    AddButton(_("Download manager"), [](){ ShowFileManager(); });
    AddButton(_("Export flights"), [](){ ShowExportFlightsDialog(); });
    AddButton(_("Import data"), [](){ 
      ShowMessageBox(_("Import data is not available."),
                     _("Import data"), MB_OK);
    });
    AddButton(_("Backup manager"), [](){ 
      ShowMessageBox(_("Backup manager is not available."),
                     _("Backup manager"), MB_OK);
    });
  }
};

void
ShowDataManagementDialog()
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  TWidgetDialog<DataManagementWidget> dlg(
    WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
    look, _("Data Management"));

  dlg.AddButton(_("Back"), dlg.MakeModalResultCallback(mrCancel));
  dlg.SetWidget(look);
  dlg.ShowModal();
}
