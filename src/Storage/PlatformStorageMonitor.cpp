// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "PlatformStorageMonitor.hpp"

#if defined(HAVE_POSIX)
#include "posix/LinuxStorageMonitor.hpp"
#elif defined(_WIN32)
#include "win/WindowsStorageMonitor.hpp"
#endif

#include <memory>

std::unique_ptr<StorageMonitor>
CreatePlatformStorageMonitor()
{
#if defined(HAVE_POSIX)
  return std::make_unique<LinuxStorageMonitor>();
#elif defined(_WIN32)
  return std::make_unique<WindowsStorageMonitor>();
#else
  return nullptr;
#endif
}
