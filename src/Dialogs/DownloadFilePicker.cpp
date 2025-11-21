// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFilePicker.hpp"
#include "Error.hpp"
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
#include "system/FileUtil.hpp"
#include "io/FileLineReader.hpp"
#include "Repository/Glue.hpp"
#include "Repository/FileRepository.hpp"
#include "Repository/Parser.hpp"
#include "Repository/AvailableFile.hpp"
#include "net/http/Features.hpp"
#include "net/http/DownloadManager.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"
#include "thread/Mutex.hxx"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "util/ConvertString.hpp"
#include "util/StringAPI.hxx"
#include "time/BrokenDateTime.hpp"

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

  std::exception_ptr error;

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

  void Rethrow() const {
    if (error)
      std::rethrow_exception(error);
  }

private:
  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path _path_relative,
                       int64_t size, int64_t position) noexcept override {
    if (!complete && path_relative == _path_relative) {
      if (!got_size && size >= 0) {
        got_size = true;
        env.SetProgressRange(uint64_t(size) / 1024u);
      }

      if (got_size)
        env.SetProgressPosition(uint64_t(position) / 1024u);
    }
  }

  void OnDownloadComplete(Path _path_relative) noexcept override {
    if (!complete && path_relative == _path_relative) {
      complete = true;
      success = true;
      download_complete_notify.SendNotification();
    }
  }

  void OnDownloadError(Path _path_relative,
                       std::exception_ptr _error) noexcept override {
    if (!complete && path_relative == _path_relative) {
      complete = true;
      success = false;
      error = std::move(_error);
      download_complete_notify.SendNotification();
    }
  }

  void OnDownloadCompleteNotification() noexcept {
    assert(complete);
    dialog.SetModalResult(success ? mrOK : mrCancel);
  }
};

/**
 * Check if a file needs to be updated based on its modification date.
 * Returns true if file doesn't exist or is older than remote update_date.
 */
[[gnu::pure]]
static bool
NeedsUpdate(const AvailableFile *remote_file)
{
  if (remote_file == nullptr)
    return false;

  const auto path = LocalPath(Path(remote_file->GetName()));
  if (!File::Exists(path))
    return true;

  if (!remote_file->update_date.IsPlausible())
    /* No update date available, assume it needs updating */
    return true;

  BrokenDate local_changed = BrokenDateTime{File::GetLastModification(path)};
  return local_changed < remote_file->update_date;
}

/**
 * Throws on error.
 * @param remote_file optional metadata for currency checking
 */
static AllocatedPath
DownloadFile(const char *uri, const char *_base,
             const AvailableFile *remote_file = nullptr)
{
  assert(Net::DownloadManager::IsAvailable());

  const UTF8ToWideConverter base(_base);
  if (!base.IsValid())
    return nullptr;

  auto final_path = LocalPath(base);

  /* Check if file exists and is current */
  if (remote_file != nullptr && !NeedsUpdate(remote_file))
    return final_path;

  ProgressDialog dialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                        _("Download"));
  dialog.SetText(base);

  dialog.AddCancelButton();

  const DownloadProgress dp(dialog, Path(base));

  Net::DownloadManager::Enqueue(uri, Path(base));

  int result = dialog.ShowModal();
  if (result != mrOK) {
    Net::DownloadManager::Cancel(Path(base));
    dp.Rethrow();
    return nullptr;
  }

  return final_path;
}

class DownloadFilePickerWidget final
  : public ListWidget,
    Net::DownloadListener {

  WidgetDialog &dialog;

  const FileType file_type;

  unsigned font_height;

  Button *download_button;

  std::vector<AvailableFile> items;

  TextRowRenderer row_renderer;

  AllocatedPath path;

public:
  DownloadFilePickerWidget(WidgetDialog &_dialog, FileType _file_type)
    :dialog(_dialog), file_type(_file_type) {}

  AllocatedPath &&GetPath() {
    return std::move(path);
  }

  void CreateButtons();

protected:
  void RefreshList(const FileRepository *repository = nullptr);

  void UpdateButtons() {
      download_button->SetEnabled(!items.empty());
  }

  void Download();
  void Cancel();

public:
  /* virtual methods from class Widget */
  void Prepare(ContainerWindow &parent, const PixelRect &rc) noexcept override;
  void Unprepare() noexcept override;

  /* virtual methods from class ListItemRenderer */
  void OnPaintItem(Canvas &canvas, const PixelRect rc,
                   unsigned idx) noexcept override;

  /* virtual methods from class ListCursorHandler */
  bool CanActivateItem([[maybe_unused]] unsigned index) const noexcept override {
    return true;
  }

  void OnActivateItem([[maybe_unused]] unsigned index) noexcept override {
    Download();
  }

  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path path_relative,
                       int64_t size, int64_t position) noexcept override;
  void OnDownloadComplete(Path path_relative) noexcept override;
  void OnDownloadError(Path path_relative,
                       std::exception_ptr error) noexcept override;
};

void
DownloadFilePickerWidget::Prepare(ContainerWindow &parent,
                                  const PixelRect &rc) noexcept
{
  const DialogLook &look = UIGlobals::GetDialogLook();

  CreateList(parent, look, rc,
             row_renderer.CalculateLayout(*look.list.font));

  /* Try to load repository first */
  FileRepository repository;
  const auto repository_path = LocalPath(_T("repository"));
  bool repository_loaded = false;
  
  if (File::Exists(repository_path)) {
    try {
      FileLineReaderA reader(repository_path);
      ParseFileRepository(repository, reader);
      repository_loaded = true;
    } catch (const std::runtime_error &e) {
    }
  }

  /* Download repository if it doesn't exist.
     Note: The repository file itself is not an entry in the repository,
     so we can't check currency. Always download if missing. */
  if (!repository_loaded) {
    try {
      /* Download repository using unified DownloadFile() */
      const auto downloaded_path = DownloadFile("http://download.xcsoar.org/repository",
                                                 "repository", nullptr);
      if (downloaded_path != nullptr) {
        /* Reload repository after download */
        FileLineReaderA reader(repository_path);
        ParseFileRepository(repository, reader);
        repository_loaded = true;
      }
    } catch (...) {
      /* Continue anyway, will show empty list */
    }
  }

  /* Refresh list with loaded repository */
  if (repository_loaded) {
    RefreshList(&repository);
  } else {
    RefreshList();
  }

  Net::DownloadManager::AddListener(*this);
}

void
DownloadFilePickerWidget::Unprepare() noexcept
{
  Net::DownloadManager::RemoveListener(*this);
}

void
DownloadFilePickerWidget::RefreshList(const FileRepository *repository_param)
try {
  FileRepository repository;

  if (repository_param != nullptr) {
    repository = *repository_param;
  } else {
    const auto path = LocalPath(_T("repository"));
    FileLineReaderA reader(path);
    ParseFileRepository(repository, reader);
  }

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

  UpdateButtons();
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

  try {
    path = DownloadFile(file.GetURI(), file.GetName(), &file);
    if (path != nullptr)
      dialog.SetModalResult(mrOK);
  } catch (...) {
    ShowError(std::current_exception(), _("Error"));
  }
}

void
DownloadFilePickerWidget::OnDownloadAdded([[maybe_unused]] Path path_relative,
                                          [[maybe_unused]] int64_t size,
                                          [[maybe_unused]] int64_t position) noexcept
{
}

void
DownloadFilePickerWidget::OnDownloadComplete([[maybe_unused]] Path path_relative) noexcept
{
  /* Repository is now downloaded synchronously in Prepare(), so we don't
     need special handling here. Other downloads are handled by DownloadProgress. */
}

void
DownloadFilePickerWidget::OnDownloadError([[maybe_unused]] Path path_relative,
                                          [[maybe_unused]] std::exception_ptr error) noexcept
{
  /* Repository is now downloaded synchronously in Prepare(), so we don't
     need special handling here. Other downloads are handled by DownloadProgress. */
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

  TWidgetDialog<DownloadFilePickerWidget>
    dialog(WidgetDialog::Full{}, UIGlobals::GetMainWindow(),
           UIGlobals::GetDialogLook(), _("Download"));
  dialog.AddButton(_("Cancel"), mrCancel);
  dialog.SetWidget(dialog, file_type);
  dialog.GetWidget().CreateButtons();
  dialog.ShowModal();

  return dialog.GetWidget().GetPath();
}
