// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <windows.h>

#include "Storage/StorageHotplugMonitor.hpp"

class WindowsStorageHotplugMonitor final : public StorageHotplugMonitor {
public:
  explicit WindowsStorageHotplugMonitor(StorageHotplugHandler &handler);
  ~WindowsStorageHotplugMonitor() override;

  void Start() noexcept override;
  void Stop() noexcept override;

  // Called from main window WndProc
  void OnDeviceChange(WPARAM wParam, LPARAM lParam) noexcept;

private:
  StorageHotplugHandler &handler_;
};
