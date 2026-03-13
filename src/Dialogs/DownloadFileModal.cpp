// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFileModal.hpp"
#include "ProgressDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "LocalPath.hpp"
#include "net/http/DownloadManager.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
#include "thread/Mutex.hxx"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <cassert>
#include <exception>

/**
 * Tracks a single file download and updates a #ProgressDialog.
 * Register before calling Net::DownloadManager::Enqueue().
 */
class DownloadProgress final : Net::DownloadListener {
  ProgressDialog &dialog;
  ThreadedOperationEnvironment env;
  const Path path_relative;

  UI::PeriodicTimer update_timer{[this]{ Net::DownloadManager::Enumerate(*this); }};

  UI::Notify download_complete_notify{[this]{ OnDownloadCompleteNotification(); }};

  mutable Mutex mutex;

  std::exception_ptr error;

  bool got_size = false, complete = false, success = false;

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
    std::exception_ptr e;
    {
      const std::lock_guard lock{mutex};
      e = error;
    }
    if (e)
      std::rethrow_exception(e);
  }

private:
  /* virtual methods from class Net::DownloadListener */
  void OnDownloadAdded(Path _path_relative,
                       int64_t size, int64_t position) noexcept override {
    bool set_range = false, set_position = false;
    {
      const std::lock_guard lock{mutex};
      if (complete || path_relative != _path_relative)
        return;
      if (!got_size && size >= 0) {
        got_size = true;
        set_range = true;
      }
      set_position = got_size;
    }

    if (set_range)
      env.SetProgressRange(uint64_t(size) / 1024u);
    if (set_position)
      env.SetProgressPosition(uint64_t(position) / 1024u);
  }

  void OnDownloadComplete(Path _path_relative) noexcept override {
    {
      const std::lock_guard lock{mutex};
      if (complete || path_relative != _path_relative)
        return;
      complete = true;
      success = true;
    }
    download_complete_notify.SendNotification();
  }

  void OnDownloadError(Path _path_relative,
                       std::exception_ptr _error) noexcept override {
    {
      const std::lock_guard lock{mutex};
      if (complete || path_relative != _path_relative)
        return;
      complete = true;
      success = false;
      error = std::move(_error);
    }
    download_complete_notify.SendNotification();
  }

  void OnDownloadCompleteNotification() noexcept {
    bool s;
    {
      const std::lock_guard lock{mutex};
      assert(complete);
      s = success;
    }
    dialog.SetModalResult(s ? mrOK : mrCancel);
  }
};

AllocatedPath
DownloadFileModal(const char *caption, const char *uri, const char *base)
{
  assert(Net::DownloadManager::IsAvailable());

  if (uri == nullptr || base == nullptr)
    return nullptr;

  ProgressDialog dialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                        caption);

  const auto display_name = Path(base).GetBase();
  dialog.SetText(display_name != nullptr ? display_name.c_str() : base);

  dialog.AddCancelButton();

  DownloadProgress dp(dialog, Path(base));
  Net::DownloadManager::Enqueue(uri, Path(base));

  int result = dialog.ShowModal();
  if (result != mrOK) {
    Net::DownloadManager::Cancel(Path(base));
    dp.Rethrow();
    return nullptr;
  }

  return LocalPath(base);
}
