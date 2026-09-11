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

static HWND hotplug_hwnd = nullptr;

static LRESULT CALLBACK
HotplugWndProc(HWND hwnd, UINT msg,
               WPARAM wParam, LPARAM lParam) noexcept
{
  if (msg == WM_DEVICECHANGE) {
    ForwardDeviceChange(wParam, lParam);
    return TRUE;
  }

  return DefWindowProc(hwnd, msg, wParam, lParam);
}

static void
EnsureHotplugWindow() noexcept
{
  if (hotplug_hwnd != nullptr)
    return;

  WNDCLASS wc{};
  wc.lpfnWndProc = HotplugWndProc;
  wc.hInstance = GetModuleHandle(nullptr);
  wc.lpszClassName = "XCSoarHotplug";
  RegisterClass(&wc);

  hotplug_hwnd = CreateWindowEx(0, wc.lpszClassName, nullptr, 0,
                                0, 0, 0, 0, HWND_MESSAGE, nullptr,
                                wc.hInstance, nullptr);
}

static void
DestroyHotplugWindow() noexcept
{
  if (hotplug_hwnd == nullptr)
    return;

  DestroyWindow(hotplug_hwnd);
  hotplug_hwnd = nullptr;
}

void
RegisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept
{
  bool create_window = false;
  {
    const std::lock_guard lock(monitors_mutex);
    if (monitor_count < MAX_MONITORS)
      monitors[monitor_count++] = m;
    create_window = monitor_count == 1;
  }

  if (create_window)
    EnsureHotplugWindow();
}

void
UnregisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept
{
  bool destroy_window = false;
  {
    const std::lock_guard lock(monitors_mutex);
    auto *end = monitors.data() + monitor_count;
    auto *it = std::find(monitors.data(), end, m);
    if (it != end) {
      *it = *(end - 1);
      --monitor_count;
    }
    destroy_window = monitor_count == 0;
  }

  if (destroy_window)
    DestroyHotplugWindow();
}

void
ForwardDeviceChange(WPARAM wParam, LPARAM lParam) noexcept
{
  const std::lock_guard lock(monitors_mutex);
  for (unsigned i = 0; i < monitor_count; ++i)
    monitors[i]->OnDeviceChange(wParam, lParam);
}

} // namespace Storage::Win
