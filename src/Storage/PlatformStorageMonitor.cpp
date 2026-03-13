// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlatformStorageMonitor.hpp"

#if defined(ANDROID)
#include "android/AndroidStorageMonitor.hpp"
#include "Android/SAFHelper.hpp"
#include "Android/Main.hpp"
#elif defined(__linux__)
#include "linux/LinuxStorageMonitor.hpp"
#elif defined(_WIN32)
#include "win/WindowsStorageMonitor.hpp"
#endif

#include <memory>

std::unique_ptr<StorageMonitor>
CreatePlatformStorageMonitor()
{
#if defined(ANDROID)
  if (saf_helper != nullptr)
    return std::make_unique<AndroidStorageMonitor>(*saf_helper);
  return nullptr;
#elif defined(__linux__)
  return std::make_unique<LinuxStorageMonitor>();
#elif defined(_WIN32)
  return std::make_unique<WindowsStorageMonitor>();
#else
  return nullptr;
#endif
}
