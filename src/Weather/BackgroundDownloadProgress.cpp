// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "BackgroundDownloadProgress.hpp"

#include "Interface.hpp"
#include "Message.hpp"
#include "time/PeriodClock.hpp"

BackgroundDownloadProgress BackgroundDownloadProgress::instance;

void
BackgroundDownloadProgress::ShowOnMainThread(const char *text) noexcept
{
  if (CommonInterface::main_window == nullptr)
    return;

  const void *key = this;

  if (shown) {
    Message::SetProgressText(key, text);
    return;
  }

  Message::ShowProgress(text, key);
  shown = true;
  displayed_range = pending_range.load();
  displayed_position = pending_position.load();

  if (displayed_range > 0) {
    Message::SetProgressRange(key, displayed_range);
    Message::SetProgressPosition(key, displayed_position);
  }
}

void
BackgroundDownloadProgress::HideOnMainThread() noexcept
{
  if (!shown)
    return;

  Message::ClearProgress(this);
  shown = false;
  displayed_range = 0;
  displayed_position = 0;
}

void
BackgroundDownloadProgress::ApplyPendingProgress() noexcept
{
  if (!shown)
    return;

  StaticString<128> text;
  bool update_text = false;
  {
    const std::lock_guard lock{text_mutex};
    if (text_pending) {
      text_pending = false;
      update_text = true;
      text = pending_text;
    }
  }

  const void *key = this;

  if (update_text)
    Message::SetProgressText(key, text);

  const unsigned range = pending_range.load();
  const unsigned position = pending_position.load();

  if (range != displayed_range) {
    displayed_range = range;
    Message::SetProgressRange(key, range);
  }

  if (position != displayed_position) {
    displayed_position = position;
    Message::SetProgressPosition(key, position);
  }
}

void
BackgroundDownloadProgress::ProcessPendingVisibility() noexcept
{
  if (pending_hide.load(std::memory_order_relaxed)) {
    if (active_sessions.load(std::memory_order_relaxed) == 0) {
      pending_hide.store(false, std::memory_order_relaxed);
      HideOnMainThread();
    }
  }

  if (pending_show.load(std::memory_order_relaxed) &&
      active_sessions.load(std::memory_order_relaxed) > 0) {
    StaticString<128> text;
    {
      const std::lock_guard lock{text_mutex};
      pending_show.store(false, std::memory_order_relaxed);
      text = pending_text;
    }

    ShowOnMainThread(text);
  }
}

void
BackgroundDownloadProgress::OnProgressNotify() noexcept
{
  ProcessPendingVisibility();

  static PeriodClock throttle;

  if (!throttle.CheckUpdate(std::chrono::milliseconds(200)))
    return;

  if (active_sessions.load(std::memory_order_relaxed) == 0)
    return;

  ApplyPendingProgress();
}

void
BackgroundDownloadProgress::Begin(const char *text) noexcept
{
  if (text == nullptr)
    text = "";

  const unsigned prev = active_sessions.fetch_add(1);
  {
    const std::lock_guard lock{text_mutex};
    pending_text = text;
    if (prev == 0)
      pending_show.store(true, std::memory_order_relaxed);
    else
      text_pending = true;
  }

  progress_notify.SendNotification();
}

void
BackgroundDownloadProgress::End() noexcept
{
  unsigned expected = active_sessions.load(std::memory_order_relaxed);
  while (expected > 0) {
    if (active_sessions.compare_exchange_weak(expected, expected - 1)) {
      if (expected == 1)
        pending_hide.store(true, std::memory_order_relaxed);
      progress_notify.SendNotification();
      return;
    }
  }
}

void
BackgroundDownloadProgress::ForceHide() noexcept
{
  active_sessions.store(0, std::memory_order_relaxed);
  pending_show.store(false, std::memory_order_relaxed);
  pending_hide.store(false, std::memory_order_relaxed);
  progress_notify.ClearNotification();
  pending_hide.store(true, std::memory_order_relaxed);
  progress_notify.SendNotification();
}

void
BackgroundDownloadProgress::SetText(const char *text) noexcept
{
  if (text == nullptr)
    text = "";

  {
    const std::lock_guard lock{text_mutex};
    pending_text = text;
    text_pending = true;
  }

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
