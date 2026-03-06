// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageMonitor.hpp"

#include <memory>
#include <vector>

class SAFHelper;

/**
 * Android-specific storage monitor that enumerates removable volumes
 * (SD cards, USB OTG sticks) via the Android StorageManager /
 * Storage Access Framework.
 *
 * The internal primary volume is intentionally excluded because
 * XCSoar already has direct file access to its own app-private
 * directory on internal storage via Context.getExternalFilesDir().
 */
class AndroidStorageMonitor : public StorageMonitor {
  SAFHelper &saf_;

public:
  explicit AndroidStorageMonitor(SAFHelper &saf) noexcept;

  std::vector<std::shared_ptr<StorageDevice>> Enumerate() override;
};
