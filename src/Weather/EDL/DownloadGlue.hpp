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

class DownloadListener {
public:
  virtual ~DownloadListener() = default;

  virtual void OnDownloadFinished() noexcept = 0;
};

/**
 * Background EDL tile downloads (replaces modal ShowCoDialog usage).
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
   * Apply a cached tile or download the current forecast/isobar in the
   * background.
   */
  void RequestOverlayRefresh() noexcept;

  /**
   * Download all hours and isobars for one UTC day in the background.
   */
  void RequestPrecacheDay(BrokenDateTime day) noexcept;

private:
  Co::InvokeTask RunOverlayDownload();
  Co::InvokeTask RunPrecacheDay(BrokenDateTime day);

  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

  void NotifyListeners() noexcept;
};

} // namespace EDL

#endif /* HAVE_EDL */
