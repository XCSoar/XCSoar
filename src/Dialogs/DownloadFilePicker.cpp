/*
Copyright_License {

  XCSoar Glide Computer - http://www.xcsoar.org/
  Copyright (C) 2000-2016 The XCSoar Project
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
#include "Form/Button.hpp"
#include "Widget/ListWidget.hpp"
#include "Screen/Canvas.hpp"
#include "Screen/Layout.hpp"
#include "Language/Language.hpp"
#include "LocalPath.hpp"
#include "OS/Path.hpp"
#include "IO/FileLineReader.hpp"
#include "Repository/Glue.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Parser.hpp"
#include "Net/HTTP/Features.hpp"
#include "Net/HTTP/DownloadManager.hpp"
#include "Event/Notify.hpp"
#include "Thread/Mutex.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "Util/ConvertString.hpp"

#include <vector>

#include <assert.h>

/**
 * This class tracks a download and updates a #ProgressDialog.
 */
class DownloadProgress final : Timer, Net::DownloadListener, Notify {
  ProgressDialog &dialog;
  ThreadedOperationEnvironment env;
  const Path path_relative;

  bool got_size = false, complete = false, success;

public:
  DownloadProgress(ProgressDialog &_dialog,
                   const Path _path_relative)
    :dialog(_dialog), env(_dialog), path_relative(_path_relative) {
    Timer::Schedule(std::chrono::seconds(1));
    Net::DownloadManager::AddListener(*this);
  }

  ~DownloadProgress() {
    Net::DownloadManager::RemoveListener(*this);
    Timer::Cancel();
  }

private:
  /* virtual methods from class Timer */
  void OnTimer() override {
    Net::DownloadManager::Enumerate(*this);
  }

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
      Notify::SendNotification();
    }
  }

  /* virtual methods from class Notify */
  void OnNotification() override {
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
    Net::DownloadListener, Notify,
    ActionListener {
  enum Buttons {
    DOWNLOAD,
  };

  WidgetDialog &dialog;

  const FileType file_type;

  unsigned font_height;

  Button *download_button;

  std::vector<AvailableFile> items;

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
  void OnPaintItem(Canvas &canvas, const PixelRect rc, unsigned idx) override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem(unsigned index) const override {
    return true;
  }

  void OnActivateItem(unsigned index) override {
    Download();
  }

  /* virtual methods from class ActionListener */
  void OnAction(int id) override;

  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path path_relative,
                       int64_t size, int64_t position) override;
  void OnDownloadComplete(Path path_relative, bool success) override;

  /* virtual methods from class Notify */
  void OnNotification() override;
};

void
DownloadFilePickerWidget::Prepare(ContainerWindow &parent, const PixelRect &rc)
{
  const DialogLook &look = UIGlobals::GetDialogLook();
  const unsigned margin = Layout::GetTextPadding();
  font_height = look.list.font->GetHeight();

  unsigned row_height = std::max(3u * margin + 2u * font_height,
                                 Layout::GetMaximumControlHeight());
  CreateList(parent, look, rc, row_height);
  RefreshList();

  Net::DownloadManager::AddListener(*this);
  Net::DownloadManager::Enumerate(*this);

  EnqueueRepositoryDownload();
}

void
DownloadFilePickerWidget::Unprepare()
{
  Net::DownloadManager::RemoveListener(*this);

  ClearNotification();

  DeleteWindow();
}

void
DownloadFilePickerWidget::RefreshList()
try {
  mutex.Lock();
  repository_modified = false;
  repository_failed = false;
  mutex.Unlock();

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
  download_button = dialog.AddButton(_("Download"), *this, DOWNLOAD);
}

void
DownloadFilePickerWidget::OnPaintItem(Canvas &canvas, const PixelRect rc,
                                      unsigned i)
{
  const auto &file = items[i];

  const unsigned margin = Layout::GetTextPadding();

  const UTF8ToWideConverter name(file.GetName());
  canvas.DrawText(rc.left + margin, rc.top + margin, name);
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
DownloadFilePickerWidget::OnAction(int id)
{
  switch (id) {
  case DOWNLOAD:
    Download();
    break;
  }
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

  mutex.Lock();

  if (name == Path(_T("repository"))) {
    repository_failed = !success;
    if (success)
      repository_modified = true;
  }

  mutex.Unlock();

  SendNotification();
}

void
DownloadFilePickerWidget::OnNotification()
{
  mutex.Lock();
  bool repository_modified2 = repository_modified;
  repository_modified = false;
  const bool repository_failed2 = repository_failed;
  repository_failed = false;
  mutex.Unlock();

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

  WidgetDialog dialog(UIGlobals::GetDialogLook());
  DownloadFilePickerWidget widget(dialog, file_type);
  dialog.CreateFull(UIGlobals::GetMainWindow(), _("Download"), &widget);
  widget.CreateButtons();
  dialog.AddButton(_("Cancel"), mrCancel);

  dialog.ShowModal();
  dialog.StealWidget();

  return widget.GetPath();
}
