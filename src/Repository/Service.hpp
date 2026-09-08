// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "FileRepository.hpp"
#include "net/http/DownloadManager.hpp"
#include "system/Path.hpp"
#include "ui/event/Notify.hpp"
#include "thread/Mutex.hxx"

#include <vector>

namespace Repository {

class Listener {
public:
  virtual void OnRepositoryRefreshed(bool success) noexcept = 0;
};

enum class RefreshResult {
  STARTED,
  IN_PROGRESS,
  FRESH,
  UNAVAILABLE,
};

/**
 * Owns the trusted main repository snapshot and its refresh lifecycle.
 * All public methods and listener callbacks run on the UI thread.
 */
class Service final : private Net::DownloadListener {
  FileRepository repository;
  std::vector<Listener *> listeners;
  UI::Notify completion_notify{[this]{ OnCompletionNotification(); }};
  Mutex mutex;
  bool completion_pending = false;
  bool completion_success = false;
  bool refreshing = false;
  bool shutting_down = false;
  bool has_snapshot = false;

  void OnCompletionNotification() noexcept;

  void OnDownloadAdded(Path, int64_t, int64_t) noexcept override {}
  void OnDownloadComplete(Path path_relative) noexcept override;
  void OnDownloadError(Path path_relative,
                       std::exception_ptr error) noexcept override;

public:
  Service();
  ~Service() noexcept;

  void AddListener(Listener &listener);
  void RemoveListener(Listener &listener) noexcept;

  RefreshResult Refresh(bool force) noexcept;
  void BeginShutdown() noexcept;

  [[gnu::pure]] bool HasFreshSnapshot() const noexcept;
  [[gnu::pure]] const FileRepository &GetRepository() const noexcept {
    return repository;
  }
};

} // namespace Repository
