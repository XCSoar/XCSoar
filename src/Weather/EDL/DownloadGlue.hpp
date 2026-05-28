// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/Features.hpp"

#ifdef HAVE_EDL

#include "net/AsyncTask.hpp"
#include "thread/SafeList.hxx"
#include "time/BrokenDateTime.hpp"
#include "ui/event/Notify.hpp"
#include "system/Path.hpp"

#include <exception>
#include <optional>

class CurlGlobal;
class AllocatedPath;

namespace EDL {

enum class DownloadJob : uint8_t {
  OVERLAY,
  PRECACHE_DAY,
};

enum class DownloadOutcome : uint8_t {
  SUCCESS,
  ERROR,
};

struct DownloadNotification {
  DownloadJob job = DownloadJob::OVERLAY;
  DownloadOutcome outcome = DownloadOutcome::SUCCESS;
  std::exception_ptr error;
  std::optional<AllocatedPath> overlay_path;
  unsigned precache_count = 0;

  /** When true, the overlay was applied from cache before notification. */
  bool overlay_from_cache = false;
};

class DownloadListener {
public:
  virtual ~DownloadListener() = default;

  virtual void
  OnDownloadFinished(const DownloadNotification &notification) noexcept = 0;
};

/**
 * Background EDL tile downloads (replaces modal ShowCoDialog usage).
 *
 * Performs HTTP work on the network thread and reports results to
 * #DownloadListener instances on the UI thread.  UI state changes belong
 * in #Glue or other UI-side listeners.
 */
class DownloadGlue final {
  enum class Job {
    NONE,
    OVERLAY,
    PRECACHE_DAY,
  };

  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  Job pending_job = Job::NONE;
  std::exception_ptr completion_error;
  std::optional<AllocatedPath> completed_path;
  unsigned completed_precache_count = 0;

  /** Snapshot for overlay download (read on network thread). */
  BrokenDateTime overlay_forecast_time;
  unsigned overlay_isobar = 0;

  bool shutting_down = false;

  ThreadSafeList<DownloadListener *> listeners;

public:
  explicit DownloadGlue(CurlGlobal &_curl) noexcept;
  ~DownloadGlue() noexcept;

  void BeginShutdown() noexcept;

  [[nodiscard]]
  bool IsBusy() const noexcept {
    return task.IsRunning();
  }

  void AddListener(DownloadListener &listener) noexcept;
  void RemoveListener(DownloadListener &listener) noexcept;

  /**
   * Notify listeners from the UI thread (for example after applying a
   * cached overlay without starting a download).
   */
  void DeliverNotification(DownloadNotification notification) noexcept;

  /**
   * Download the overlay for the given forecast/isobar in the background.
   */
  void StartOverlayDownload(BrokenDateTime forecast_time,
                            unsigned isobar) noexcept;

  /**
   * Download all hours and isobars for one UTC day in the background.
   */
  void StartPrecacheDay(BrokenDateTime day) noexcept;

private:
  Co::InvokeTask RunOverlayDownload();
  Co::InvokeTask RunPrecacheDay(BrokenDateTime day);

  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

  void NotifyListeners(const DownloadNotification &notification) noexcept;
};

} // namespace EDL

#endif /* HAVE_EDL */
