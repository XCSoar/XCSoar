// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlatformStorageHotplugMonitor.hpp"

#if defined(__linux__) && !defined(ANDROID)
#include "linux/LinuxStorageHotplugMonitor.hpp"
#elif defined(_WIN32)
#include "win/WindowsStorageHotplugMonitor.hpp"
#endif

#include <memory>

std::unique_ptr<StorageHotplugMonitor>
CreatePlatformStorageHotplugMonitor([[maybe_unused]] StorageHotplugHandler &handler)
{
#if defined(__linux__) && !defined(ANDROID)
  return std::make_unique<LinuxStorageHotplugMonitor>(handler);
#elif defined(_WIN32)
  return std::make_unique<WindowsStorageHotplugMonitor>(handler);
#else
  return nullptr;
#endif
}
