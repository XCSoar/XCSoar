// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Weather/xctherm/XCThermDownloadJob.hpp"
#include "co/InvokeTask.hxx"
#include "net/AsyncTask.hpp"
#include "ui/event/Notify.hpp"

#include <exception>
#include <functional>
#include <memory>

class CurlGlobal;

/**
 * Runs #RunXCThermDownload on the network #EventLoop and notifies the
 * UI thread when complete (EDL DownloadGlue pattern).
 */
class XCThermDownloadGlue final {
  CurlGlobal &curl;
  Net::AsyncTask task;
  UI::Notify complete_notify{[this]{ OnCompleteNotify(); }};

  std::shared_ptr<XCThermDownloadJob> job;
  std::function<void(std::shared_ptr<XCThermDownloadJob>)> on_finished;
  std::function<void()> on_index_fetched;
  bool index_fetch = false;
  std::exception_ptr completion_error;

  Co::InvokeTask RunDownload();
  Co::InvokeTask RunIndexFetch();
  void OnCompletion(std::exception_ptr error) noexcept;
  void OnCompleteNotify() noexcept;

public:
  explicit XCThermDownloadGlue(CurlGlobal &_curl) noexcept;

  ~XCThermDownloadGlue() noexcept { BeginShutdown(); }

  XCThermDownloadGlue(const XCThermDownloadGlue &) = delete;
  XCThermDownloadGlue &operator=(const XCThermDownloadGlue &) = delete;

  [[nodiscard]]
  bool IsRunning() const noexcept { return task.IsRunning(); }

  void BeginShutdown() noexcept;

  void Start(std::shared_ptr<XCThermDownloadJob> new_job,
             std::function<void(std::shared_ptr<XCThermDownloadJob>)> &&finished);

  /**
   * Fetch index.json on the network thread when not already loaded.
   * @p finished runs on the UI thread (no-op if a job is already running).
   */
  void StartIndexFetch(std::function<void()> &&finished);

  void RequestCancel() noexcept;

  /**
   * Cancel an in-flight job and drop the UI completion handler (e.g.
   * dialog closed). Does not stop the shared #Net::AsyncTask — use
   * #BeginShutdown() from #NetComponents on app exit.
   */
  void Abandon() noexcept;
};

/**
 * Shared #XCThermDownloadGlue from #net_components (UI thread only).
 */
[[gnu::pure]]
XCThermDownloadGlue *
GetXCThermDownloadGlue() noexcept;

