// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#ifdef _WIN32
#include <windows.h>

class WindowsStorageHotplugMonitor;

namespace Storage::Win {
void RegisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept;
void UnregisterWindowsHotplugMonitor(WindowsStorageHotplugMonitor *m) noexcept;
void ForwardDeviceChange(WPARAM wParam, LPARAM lParam) noexcept;
} // namespace Storage::Win

#endif
