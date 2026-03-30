// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Dialogs/DataManagement/DataManagement.hpp"
#include "Dialogs/WidgetDialog.hpp"
#include "Widget/RowFormWidget.hpp"
#include "UIGlobals.hpp"
#include "Dialogs/InternalLink.hpp"
#include "Dialogs/Settings/Panels/SiteConfigPanel.hpp"
#include "Look/DialogLook.hpp"
#include "Language/Language.hpp"
#include "Form/Frame.hpp"
#include "Dialogs/FileManager.hpp"
#include "Dialogs/DataManagement/ExportFlightsPanel.hpp"
#include "Dialogs/DataManagement/BackupRestorePanel.hpp"
#include "Dialogs/DataManagement/ImportDataPanel.hpp"
#include "Dialogs/DataManagement/AdvancedFileExplorer.hpp"
#include "Dialogs/Message.hpp"

class DataManagementWidget : public RowFormWidget {
public:
  explicit DataManagementWidget(const DialogLook &look) noexcept
    :RowFormWidget(look) {}

  void Prepare([[maybe_unused]] ContainerWindow &parent,
               [[maybe_unused]] const PixelRect &rc) noexcept override {
    AddButton(_("Navigation & Flight Resources"), [](){ ShowConfigPanel(_("Site files"), CreateSiteConfigPanel); });
    AddButton(_("Download manager"), [](){ ShowFileManager(); });
    AddButton(_("Export flights"), [](){ ShowExportFlightsDialog(); });
    AddButton(_("Import data"), [](){ ShowImportDataDialog(); });
    AddButton(_("Backup manager"), [](){ ShowBackupManagerDialog(); });
    AddButton(_("Advanced File Explorer"), [](){ ShowAdvancedFileExplorerDialog(); });
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
