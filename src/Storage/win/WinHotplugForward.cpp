// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "WinHotplugForward.hpp"
#include "WindowsStorageHotplugMonitor.hpp"

#include <algorithm>
#include <array>
#include <mutex>

namespace Storage::Win {

static constexpr unsigned MAX_MONITORS = 4;

static std::mutex monitors_mutex;
static std::array<WindowsStorageHotplugMonitor *, MAX_MONITORS> monitors{};
static unsigned monitor_count = 0;

void
RegisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept
{
  const std::lock_guard lock(monitors_mutex);
  if (monitor_count < MAX_MONITORS)
    monitors[monitor_count++] = m;
}

void
UnregisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept
{
  const std::lock_guard lock(monitors_mutex);
  auto *end = monitors.data() + monitor_count;
  auto *it = std::find(monitors.data(), end, m);
  if (it != end) {
    *it = *(end - 1);
    --monitor_count;
  }
}

void
ForwardDeviceChange(WPARAM wParam, LPARAM lParam) noexcept
{
  const std::lock_guard lock(monitors_mutex);
  for (unsigned i = 0; i < monitor_count; ++i)
    monitors[i]->OnDeviceChange(wParam, lParam);
}

} // namespace Storage::Win
