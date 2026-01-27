// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultiFilePicker.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "WidgetDialog.hpp"
#include "net/http/Features.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER
#include "DownloadFilePicker.hpp"
#include "net/http/DownloadManager.hpp"
#endif

static const TCHAR *
GetFileName(const FileMultiSelectWidget::FileItem &item) noexcept
{
  const auto base = item.path.GetBase();
  return (base != nullptr) ? base.c_str() : item.path.c_str();
}

bool
MultiFilePicker(const TCHAR *caption, MultiFileDataField &df,
                const TCHAR *help_text)
{
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), caption);

  auto list_widget = std::make_unique<FileMultiSelectWidget>(df, GetFileName,
                                                             caption, help_text);

  FileMultiSelectWidget *file_widget = list_widget.get();

  dialog.AddButton(_("Help"), [file_widget]() { file_widget->ShowHelp(); });

#ifdef HAVE_DOWNLOAD_MANAGER
  if (Net::DownloadManager::IsAvailable()) {
    dialog.AddButton(_("Download"), [file_widget, &df]() {
      const auto path = DownloadFilePicker(df.GetFileDataField().GetFileType());
      if (path != nullptr) {
        df.ForceModify(path);
        df.GetFileDataField().Sort();

        file_widget->Refresh();
      }
    });
  }
#endif

  dialog.AddButton(_("Select none"), [file_widget]() {
    file_widget->ClearSelection();
  });
  dialog.AddButton(_("OK"), mrOK);
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.FinishPreliminary(std::move(list_widget));
  int result = dialog.ShowModal();

  if (result == mrOK) {
    auto selected = file_widget->GetSelectedPaths();
    for (const auto &path : df.GetPathFiles())
      df.UnSet(path);
    for (const auto &path : selected)
      df.AddValue(path);
    return true;
  }

  df.Restore();
  return false;
}
