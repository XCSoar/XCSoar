// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MultiFilePicker.hpp"
#include "Form/DataField/MultiFile.hpp"
#include "Language/Language.hpp"
#include "UIGlobals.hpp"
#include "Widget/FileMultiSelectWidget.hpp"
#include "Widget/StaticHelpTextWidget.hpp"
#include "WidgetDialog.hpp"
#include "net/http/Features.hpp"

#include <functional>

#ifdef HAVE_DOWNLOAD_MANAGER
#include "DownloadFilePicker.hpp"
#include "net/http/DownloadManager.hpp"
#endif

static const char *
GetFileName(const FileMultiSelectWidget::FileItem &item) noexcept
{
  const auto base = item.path.GetBase();
  return (base != nullptr) ? base.c_str() : item.path.c_str();
}

bool
MultiFilePicker(const char *caption, MultiFileDataField &df,
                const char *help_text)
{
  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), caption);

  auto list_widget = std::make_unique<FileMultiSelectWidget>(df, GetFileName,
                                                             caption, help_text);

  FileMultiSelectWidget *file_widget = list_widget.get();

  std::unique_ptr<Widget> widget = std::move(list_widget);

  if (help_text != nullptr)
    widget = std::make_unique<StaticHelpTextWidget>(std::move(widget),
                                                    help_text);

  dialog.AddButton(_("OK"), mrOK);

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

  // Create button first; centralise caption logic in `UpdateButtons`.
  Button *select_button = dialog.AddButton("", [](){});

  std::function<void()> UpdateButtons = [file_widget, select_button]() {
    select_button->SetCaption(file_widget->GetSelectedPaths().empty()
                               ? _("Select all") : _("Select none"));
  };

  select_button->SetCallback([file_widget, UpdateButtons]() mutable {
    if (file_widget->GetSelectedPaths().empty())
      file_widget->SetAllSelected(true);
    else
      file_widget->ClearSelection();
    UpdateButtons();
  });
  dialog.AddButton(_("Cancel"), mrCancel);

  // Ensure initial caption is correct and register callback.
  UpdateButtons();
  file_widget->SetSelectionChangedCallback(UpdateButtons);

  dialog.FinishPreliminary(std::move(widget));

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
