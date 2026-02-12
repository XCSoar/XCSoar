// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "DownloadFileModal.hpp"
#include "ProgressDialog.hpp"
#include "UIGlobals.hpp"
#include "Look/DialogLook.hpp"
#include "LocalPath.hpp"
#include "net/http/DownloadManager.hpp"
#include "Operation/ThreadedOperationEnvironment.hpp"
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

AllocatedPath
DownloadFileModal(const char *caption, const char *uri, const char *base)
{
  assert(Net::DownloadManager::IsAvailable());

  if (base == nullptr)
    return nullptr;

  ProgressDialog dialog(UIGlobals::GetMainWindow(), UIGlobals::GetDialogLook(),
                        caption);
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

  return LocalPath(base);
}
