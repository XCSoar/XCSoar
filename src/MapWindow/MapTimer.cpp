// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "MapTimer.hpp"

#include <mutex>

namespace MapTimer {

namespace {

std::mutex mutex;
bool visible = false;
bool running = false;
std::chrono::steady_clock::time_point started{};
std::chrono::milliseconds accumulated{0};

std::chrono::seconds
ElapsedLocked() noexcept
{
  auto ms = accumulated;
  if (running)
    ms += std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started);
  return std::chrono::duration_cast<std::chrono::seconds>(ms);
}

} // anonymous namespace

bool
IsVisible() noexcept
{
  const std::lock_guard lock{mutex};
  return visible;
}

bool
IsRunning() noexcept
{
  const std::lock_guard lock{mutex};
  return running;
}

void
SetVisible(bool v) noexcept
{
  const std::lock_guard lock{mutex};
  visible = v;
}

void
ToggleVisible() noexcept
{
  const std::lock_guard lock{mutex};
  visible = !visible;
}

void
ToggleRunning() noexcept
{
  const std::lock_guard lock{mutex};
  if (!visible)
    return;

  if (running) {
    accumulated += std::chrono::duration_cast<std::chrono::milliseconds>(
      std::chrono::steady_clock::now() - started);
    running = false;
  } else {
    started = std::chrono::steady_clock::now();
    running = true;
  }
}

void
Reset() noexcept
{
  const std::lock_guard lock{mutex};
  running = false;
  accumulated = std::chrono::milliseconds{0};
}

std::chrono::seconds
GetElapsed() noexcept
{
  const std::lock_guard lock{mutex};
  return ElapsedLocked();
}

} // namespace MapTimer
