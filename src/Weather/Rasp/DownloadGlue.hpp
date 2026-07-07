// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "net/http/Features.hpp"

#ifdef HAVE_DOWNLOAD_MANAGER

#include "net/http/DownloadManager.hpp"
#include "ui/event/Notify.hpp"
#include "ui/event/PeriodicTimer.hpp"

#include <atomic>
#include <exception>

class Path;

/**
 * Background update of the configured RASP file via #Net::DownloadManager,
 * using the same out-of-date rules as the download manager UI.
 */
class RaspDownloadGlue final {
  enum class PendingCompletion : uint8_t {
    NONE,
    REPOSITORY,
    RASP_RELOAD,
    RASP_ERROR,
  };

  UI::Notify download_notify{[this]{ OnDownloadNotify(); }};
  UI::PeriodicTimer progress_timer{[this]{ PollDownloadProgress(); }};
  bool listener_registered = false;
  bool progress_active = false;
  std::atomic<bool> pending_show_progress{false};
  std::atomic<PendingCompletion> pending_completion{PendingCompletion::NONE};

  void OnDownloadNotify() noexcept;
  void PollDownloadProgress() noexcept;
  void EnsureProgressActive() noexcept;
  void StopProgress() noexcept;
  void ReloadConfiguredRasp() noexcept;
  bool QueueConfiguredRaspUpdate(bool force_repository,
                                 bool force_rasp_download,
                                 const char *error_message) noexcept;

  static bool IsRaspDownload(Path path_relative) noexcept;
  static bool IsTrackedDownload(Path path_relative) noexcept;

public:
  RaspDownloadGlue() noexcept = default;
  ~RaspDownloadGlue() noexcept;

  void Initialise() noexcept;
  void BeginShutdown() noexcept;

  /**
   * Refresh repository metadata if needed, then download the configured
   * RASP file when it is out of date (UI thread).
   */
  void RequestUpdateIfOutOfDate() noexcept;

  /**
   * Refresh repository metadata, then download the configured RASP file
   * immediately (UI thread).
   */
  void RequestConfiguredRaspUpdate() noexcept;

private:
  class Listener final : public Net::DownloadListener {
    RaspDownloadGlue &owner;

  public:
    explicit Listener(RaspDownloadGlue &_owner) noexcept :owner(_owner) {}

    void OnDownloadAdded(Path path_relative, int64_t size,
                         int64_t position) noexcept override;

    void OnDownloadComplete(Path path_relative) noexcept override;

    void OnDownloadError(Path path_relative,
                         std::exception_ptr error) noexcept override;
  };

  Listener listener{*this};
};

RaspDownloadGlue *
GetRaspDownloadGlue() noexcept;

/**
 * Queue repository/RASP downloads when the configured file is out of
 * date.  Safe to call from any page; downloads continue via
 * #Net::DownloadManager while the listener lives in #NetComponents.
 */
void
RequestConfiguredRaspUpdateIfOutOfDate() noexcept;

/**
 * Download the configured RASP file immediately via
 * #Net::DownloadManager (UI thread).
 */
void
RequestConfiguredRaspUpdate() noexcept;

#endif /* HAVE_DOWNLOAD_MANAGER */
