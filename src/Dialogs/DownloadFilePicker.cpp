/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2021 The XCSoar Project
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

#include "DownloadFilePicker.hpp"
#include "WidgetDialog.hpp"
#include "ProgressDialog.hpp"
#include "Message.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "Renderer/TextRowRenderer.hpp"
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "system/Path.hpp"
#include "io/FileLineReader.hpp"
#include "Repository/Glue.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Parser.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "thread/Mutex.hxx"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "util/ConvertString.hpp"

#include <vector>

#include <cassert>

/**
 * This class tracks a download and updates a #ProgressDialog.
 */
class DownloadProgress final : Net::DownloadListener {
  ProgressDialog &dialog;
  ThreadedOperationEnvironment env;
  const Path path_relative;

  UI::PeriodicTimer update_timer{[this]{ Net::DownloadManager::Enumerate(*this); }};

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  bool got_size = false, complete = false, success;

public:
  DownloadProgress(ProgressDialog &_dialog,
                   const Path _path_relative)
    :dialog(_dialog), env(_dialog), path_relative(_path_relative) {
    update_timer.Schedule(std::chrono::seconds(1));
    Net::DownloadManager::AddListener(*this);
  }

  ~DownloadProgress() {
    Net::DownloadManager::RemoveListener(*this);
  }

private:
  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path _path_relative,
                       int64_t size, int64_t position) override {
    if (!complete && path_relative == _path_relative) {
      if (!got_size && size >= 0) {
        got_size = true;
        env.SetProgressRange(uint64_t(size) / 1024u);
      }

      if (got_size)
        env.SetProgressPosition(uint64_t(position) / 1024u);
    }
  }

  void OnDownloadComplete(Path _path_relative,
                          bool _success) override {
    if (!complete && path_relative == _path_relative) {
      complete = true;
      success = _success;
      download_complete_notify.SendNotification();
    }
  }

  void OnDownloadCompleteNotification() noexcept {
    assert(complete);
    dialog.SetModalResult(success ? mrOK : mrCancel);
  }
};

static AllocatedPath
DownloadFile(const char *uri, const char *_base)
{
  assert(Net::DownloadManager::IsAvailable());

  const UTF8ToWideConverter base(_base);
  if (!base.IsValid())
    return nullptr;

  ProgressDialog dialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                        _("Download"));
  dialog.SetText(base);

  dialog.AddCancelButton([&dialog](){ dialog.SetModalResult(mrCancel); });

  const DownloadProgress dp(dialog, Path(base));

  Net::DownloadManager::Enqueue(uri, Path(base));

  int result = dialog.ShowModal();
  if (result != mrOK) {
    Net::DownloadManager::Cancel(Path(base));
    return nullptr;
  }

  return LocalPath(base);
}

class DownloadFilePickerWidget final
  : public ListWidget,
    Net::DownloadListener {

  WidgetDialog &dialog;

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  const FileType file_type;

  unsigned font_height;

  Button *download_button;

  std::vector<AvailableFile> items;

  TextRowRenderer row_renderer;

  /**
   * This mutex protects the attribute "repository_modified".
   */
  mutable Mutex mutex;

  /**
   * Was the repository file modified, and needs to be reloaded by
   * RefreshList()?
   */
  bool repository_modified;

  /**
   * Has the repository file download failed?
   */
  bool repository_failed;

  AllocatedPath path = AllocatedPath(nullptr);

public:
  DownloadFilePickerWidget(WidgetDialog &_dialog, FileType _file_type)
    :dialog(_dialog), file_type(_file_type) {}

  AllocatedPath &&GetPath() {
    return std::move(path);
  }

  void CreateButtons();

protected:
  void RefreshList();

  void UpdateButtons() {
      download_button->SetEnabled(!items.empty());
  }

  void Download();
  void Cancel();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) override;
  void Unprepare() override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem(unsigned index) noexcept override {
    Download();
  }

  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path path_relative,
                       int64_t size, int64_t position) override;
  void OnDownloadComplete(Path path_relative, bool success) override;

  void OnDownloadCompleteNotification() noexcept;
};

void
DownloadFilePickerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font));
  RefreshList();

  Net::DownloadManager::AddListener(*this);
  Net::DownloadManager::Enumerate(*this);

  EnqueueRepositoryDownload();
}

void
DownloadFilePickerWidget::Unprepare()
{
  Net::DownloadManager::RemoveListener(*this);
}

void
DownloadFilePickerWidget::RefreshList()
try {
  {
    const std::lock_guard<Mutex> lock(mutex);
    repository_modified = false;
    repository_failed = false;
  }

  FileRepository repository;

  const auto path = LocalPath(_T("repository"));
  FileLineReaderA reader(path);

  ParseFileRepository(repository, reader);

  items.clear();
  for (auto &i : repository)
    if (i.type == file_type)
      items.emplace_back(std::move(i));

  ListControl &list = GetList();
  list.SetLength(items.size());
  list.Invalidate();

  UpdateButtons();
} catch (const std::runtime_error &e) {
}

void
DownloadFilePickerWidget::CreateButtons()
{
  download_button = dialog.AddButton(_("Download"), [this](){ Download(); });
}

void
DownloadFilePickerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                      unsigned i) noexcept
{
  const auto &file = items[i];

  const UTF8ToWideConverter name(file.GetName());
  row_renderer.DrawTextRow(canvas, rc, name);
}

void
DownloadFilePickerWidget::Download()
{
  assert(Net::DownloadManager::IsAvailable());

  const unsigned current = GetList().GetCursorIndex();
  assert(current < items.size());

  const auto &file = items[current];

  path = DownloadFile(file.GetURI(), file.GetName());
  if (path.IsNull())
    dialog.SetModalResult(mrOK);
}

void
DownloadFilePickerWidget::OnDownloadAdded(Path path_relative,
                                          int64_t size, int64_t position)
{
}

void
DownloadFilePickerWidget::OnDownloadComplete(Path path_relative,
                                             bool success)
{
  const auto name = path_relative.GetBase();
  if (name == nullptr)
    return;

  {
    const std::lock_guard<Mutex> lock(mutex);

    if (name == Path(_T("repository"))) {
      repository_failed = !success;
      if (success)
        repository_modified = true;
    }
  }

  download_complete_notify.SendNotification();
}

void
DownloadFilePickerWidget::OnDownloadCompleteNotification() noexcept
{
  bool repository_modified2, repository_failed2;

  {
    const std::lock_guard<Mutex> lock(mutex);
    repository_modified2 = std::exchange(repository_modified, false);
    repository_failed2 = std::exchange(repository_failed, false);
  }

  if (repository_modified2) {
    if (repository_failed2)
      ShowMessageBox(_("Failed to download the repository index."),
                     _("Error"), MB_OK);
    else
      RefreshList();
  }
}

AllocatedPath
DownloadFilePicker(FileType file_type)
{
  if (!Net::DownloadManager::IsAvailable()) {
    const TCHAR *message =
      _("The file manager is not available on this device.");
    ShowMessageBox(message, _("File Manager"), MB_OK);
    return nullptr;
  }

  WidgetDialog dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
                      UIGlobals::GetDialogLook(), _("Download"));
  DownloadFilePickerWidget widget(dialog, file_type);
  widget.CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.FinishPreliminary(&widget);
  dialog.ShowModal();
  dialog.StealWidget();

  return widget.GetPath();
}
