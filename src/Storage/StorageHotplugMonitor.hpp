// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class StorageHotplugHandler {
public:
  virtual ~StorageHotplugHandler() = default;

  /**
   * Called when the platform detects a storage
   * topology change.
   * Must be executed on the main thread.
   */
  virtual void OnStorageTopologyChanged() noexcept = 0;
};

class StorageHotplugMonitor {
public:
  virtual ~StorageHotplugMonitor() = default;

  virtual void Start() noexcept = 0;
  virtual void Stop() noexcept = 0;
};
