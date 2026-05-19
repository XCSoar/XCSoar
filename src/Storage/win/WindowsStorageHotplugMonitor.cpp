// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#include "Storage/win/WindowsStorageHotplugMonitor.hpp"
#include "WinHotplugForward.hpp"

#include <windows.h>
#include <dbt.h>

WindowsStorageHotplugMonitor::WindowsStorageHotplugMonitor(
    StorageHotplugHandler &handler)
  : handler_(handler)
{
}

void
WindowsStorageHotplugMonitor::Start() noexcept
{
  Storage::Win::RegisterWindowsHotplugMonitor(this);
}

void
WindowsStorageHotplugMonitor::Stop() noexcept
{
  Storage::Win::UnregisterWindowsHotplugMonitor(this);
}

void
WindowsStorageHotplugMonitor::OnDeviceChange(
    WPARAM wParam, LPARAM lParam) noexcept
{
  if (wParam != DBT_DEVICEARRIVAL &&
      wParam != DBT_DEVICEREMOVECOMPLETE)
    return;

  auto *hdr = reinterpret_cast<DEV_BROADCAST_HDR*>(lParam);
  if (!hdr)
    return;

  if (hdr->dbch_devicetype == DBT_DEVTYP_VOLUME) {
    handler_.OnStorageTopologyChanged();
  }
}

WindowsStorageHotplugMonitor::~WindowsStorageHotplugMonitor()
{
  Stop();
}
