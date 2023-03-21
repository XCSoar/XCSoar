// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "ProgressGlue.hpp"
#include "ProgressWindow.hpp"
#include "ui/window/SingleWindow.hpp"
#include "UIGlobals.hpp"
#include "time/PeriodClock.hpp"

static ProgressWindow *global_progress_window;

/**
 * This clock throttles screen updates.
 */
static PeriodClock throttle_clock;

void
ProgressGlue::Create(const TCHAR *text) noexcept
{
  UIGlobals::GetMainWindow().RefreshSize();

  if (global_progress_window == nullptr)
    global_progress_window = new ProgressWindow(UIGlobals::GetMainWindow());

  global_progress_window->SetMessage(text);

  UIGlobals::GetMainWindow().Refresh();
  throttle_clock.Reset();
}

void
ProgressGlue::Move(const PixelRect &rc) noexcept
{
  if (global_progress_window == nullptr)
    return;

  global_progress_window->Move(rc);
  throttle_clock.Reset();
}

void
ProgressGlue::Close() noexcept
{
  delete global_progress_window;
  global_progress_window = nullptr;
}

void
ProgressGlue::Step() noexcept
{
  if (global_progress_window == nullptr)
    return;

  if (!throttle_clock.CheckUpdate(std::chrono::milliseconds(200)))
    return;

  global_progress_window->Step();
  UIGlobals::GetMainWindow().RefreshSize();
  UIGlobals::GetMainWindow().Refresh();
}

void
ProgressGlue::SetValue(unsigned value) noexcept
{
  if (global_progress_window == nullptr)
    return;

  if (!throttle_clock.CheckUpdate(std::chrono::milliseconds(200)))
    return;

  global_progress_window->SetValue(value);
  UIGlobals::GetMainWindow().RefreshSize();
  UIGlobals::GetMainWindow().Refresh();
}

void
ProgressGlue::SetRange(unsigned value) noexcept
{
  if (global_progress_window == nullptr)
    return;

  global_progress_window->SetRange(0, value);
  throttle_clock.Reset();
}

void
ProgressGlue::SetStep(int step) noexcept
{
  if (global_progress_window == nullptr)
    return;

  global_progress_window->SetStep(step);
  throttle_clock.Reset();
}
