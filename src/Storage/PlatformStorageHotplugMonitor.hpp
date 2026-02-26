// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include <memory>

class StorageHotplugHandler;
class StorageHotplugMonitor;

/**
 * Factory for the platform-specific storage hotplug monitor implementation.
 */
std::unique_ptr<StorageHotplugMonitor>
CreatePlatformStorageHotplugMonitor(StorageHotplugHandler &handler);
