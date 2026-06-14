// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Operation/ProgressListener.hpp"
#include "ui/event/Notify.hpp"
#include "util/StaticString.hxx"

#include <atomic>
#include <memory>

class PluggableOperationEnvironment;
class ProgressWidget;

/**
 * Top-of-map green progress bar for long-running weather downloads
 * (same widget style as terrain loading).
 *
 * Thread-safe progress updates from the network thread; UI changes
 * are marshalled to the main thread.
 */
class BackgroundDownloadProgress final : public ProgressListener {
  std::unique_ptr<PluggableOperationEnvironment> env;
  ProgressWidget *widget = nullptr;
  UI::Notify progress_notify{[this]{ OnProgressNotify(); }};

  std::atomic<unsigned> active_sessions{0};
  std::atomic<unsigned> pending_range{0};
  std::atomic<unsigned> pending_position{0};
  std::atomic<bool> pending_show{false};
  std::atomic<bool> pending_hide{false};
  StaticString<128> pending_text;
  bool text_pending = false;

  unsigned displayed_range = 0;
  unsigned displayed_position = 0;

  void OnProgressNotify() noexcept;

  void ProcessPendingVisibility() noexcept;
  void ShowOnMainThread(const char *text) noexcept;
  void HideOnMainThread() noexcept;
  void ApplyPendingProgress() noexcept;

public:
  static BackgroundDownloadProgress instance;

  static BackgroundDownloadProgress &Get() noexcept {
    return instance;
  }

  /**
   * Show the bar (main thread).  Nested calls increment an internal
   * counter; the bar stays visible until the last matching #End().
   */
  void Begin(const char *text) noexcept;

  /**
   * Hide the bar once all sessions have ended (main thread).
   */
  void End() noexcept;

  /**
   * Hide immediately and reset session count (shutdown).
   */
  void ForceHide() noexcept;

  /**
   * Update the label (any thread).
   */
  void SetText(const char *text) noexcept;

  /* virtual methods from class ProgressListener */
  void SetProgressRange(unsigned range) noexcept override;
  void SetProgressPosition(unsigned position) noexcept override;
};
