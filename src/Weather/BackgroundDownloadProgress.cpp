// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackgroundDownloadProgress.hpp"

#include "Interface.hpp"
#include "MainWindow.hpp"
#include "Operation/PluggableOperationEnvironment.hpp"
#include "Widget/ProgressWidget.hpp"
#include "time/PeriodClock.hpp"

BackgroundDownloadProgress BackgroundDownloadProgress::instance;

void
BackgroundDownloadProgress::ShowOnMainThread(const char *text) noexcept
{
  if (widget != nullptr) {
    env->SetText(text);
    return;
  }

  env = std::make_unique<PluggableOperationEnvironment>();
  widget = new ProgressWidget(*env, text);
  displayed_range = pending_range.load();
  displayed_position = pending_position.load();

  if (CommonInterface::main_window == nullptr)
    return;

  CommonInterface::main_window->SetTopWidget(widget);

  if (displayed_range > 0) {
    env->SetProgressRange(displayed_range);
    env->SetProgressPosition(displayed_position);
  }
}

void
BackgroundDownloadProgress::HideOnMainThread() noexcept
{
  if (widget == nullptr)
    return;

  if (CommonInterface::main_window == nullptr) {
    widget = nullptr;
    env.reset();
    return;
  }

  CommonInterface::main_window->SetTopWidget(nullptr);
  widget = nullptr;
  env.reset();
  displayed_range = 0;
  displayed_position = 0;
}

void
BackgroundDownloadProgress::ApplyPendingProgress() noexcept
{
  if (env == nullptr)
    return;

  if (text_pending) {
    text_pending = false;
    env->SetText(pending_text);
  }

  const unsigned range = pending_range.load();
  const unsigned position = pending_position.load();

  if (range != displayed_range) {
    displayed_range = range;
    env->SetProgressRange(range);
  }

  if (position != displayed_position) {
    displayed_position = position;
    env->SetProgressPosition(position);
  }
}

void
BackgroundDownloadProgress::OnProgressNotify() noexcept
{
  static PeriodClock throttle;

  if (!throttle.CheckUpdate(std::chrono::milliseconds(200)))
    return;

  if (active_sessions.load() == 0)
    return;

  ApplyPendingProgress();
}

void
BackgroundDownloadProgress::Begin(const char *text) noexcept
{
  pending_text = text;

  const unsigned prev = active_sessions.fetch_add(1);
  if (prev == 0)
    ShowOnMainThread(text);
  else if (env != nullptr)
    env->SetText(text);
  else {
    text_pending = true;
    progress_notify.SendNotification();
  }
}

void
BackgroundDownloadProgress::End() noexcept
{
  unsigned expected = active_sessions.load();
  while (expected > 0) {
    if (active_sessions.compare_exchange_weak(expected, expected - 1)) {
      if (expected == 1)
        HideOnMainThread();
      return;
    }
  }
}

void
BackgroundDownloadProgress::ForceHide() noexcept
{
  active_sessions.store(0, std::memory_order_relaxed);
  progress_notify.ClearNotification();
}

void
BackgroundDownloadProgress::SetText(const char *text) noexcept
{
  pending_text = text;
  text_pending = true;
  progress_notify.SendNotification();
}

void
BackgroundDownloadProgress::SetProgressRange(unsigned range) noexcept
{
  pending_range.store(range);
  progress_notify.SendNotification();
}

void
BackgroundDownloadProgress::SetProgressPosition(unsigned position) noexcept
{
  pending_position.store(position);
  progress_notify.SendNotification();
}
