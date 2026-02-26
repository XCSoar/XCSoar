// SPDX-License-Identifier: GPL-2.0-or-later
// Copyright The XCSoar Project

#pragma once

#include "Storage/StorageDevice.hpp"

#include <memory>
#include <string>
#include <vector>

enum class StorageEvent {
  Available,
  Removed,
  AccessGranted,
  COUNT
};

struct StorageEventInfo {
  StorageEvent type = StorageEvent::COUNT;
  std::vector<std::shared_ptr<StorageDevice>> devices;
  [[nodiscard]] std::string Format() const;
};

class StorageEventListener {
public:
  virtual ~StorageEventListener() = default;

  /** Called on the main/UI thread when a storage semantic event occurs. */
  virtual void OnStorageEvent(const StorageEventInfo &info) noexcept = 0;
};
